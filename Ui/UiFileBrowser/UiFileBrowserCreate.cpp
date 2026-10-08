// File creation policy and native image encoding. WIC keeps the generic browser
// free of Imaging and avoids adding a conflicting JPEG library to its hosts.
#include "UiFileBrowserCreate.h"
#include "UiFileBrowserNative.h"
#ifndef PLATFORM_WIN32
#include <fcntl.h>
#include <unistd.h>
#endif
namespace Upp
{
namespace
{
bool ValidLeaf(const String &name)
{
    if(name.IsEmpty() || name == "." || name == ".." || name.EndsWith(".") || name.EndsWith(" "))
        return false;
    for(char ch : name)
        if((byte)ch < 32 || ch == '/' || ch == '\\' || ch == ':' || ch == '<' || ch == '>' || ch == '"' ||
           ch == '|' || ch == '?' || ch == '*')
            return false;
    String stem = ToUpper(name.Left(name.Find('.') < 0 ? name.GetCount() : name.Find('.')));
    if(stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL")
        return false;
    if(stem.GetCount() == 4 && (stem.StartsWith("COM") || stem.StartsWith("LPT")) && stem[3] >= '1' &&
       stem[3] <= '9')
        return false;
    return true;
}

// Keep the native handle until commit: a collision never truncates another
// file, and an encode/write failure cannot leave an empty placeholder behind.
class NewFile
{
public:
    NewFile() = default;
    NewFile(const NewFile &) = delete;
    NewFile &operator=(const NewFile &) = delete;
#ifdef PLATFORM_WIN32
    HANDLE handle = INVALID_HANDLE_VALUE;
    Vector<char16> wide;
    bool Open(const String &path)
    {
        wide = ToUtf16(path);
        wide.Add(0);
        handle = CreateFileW(reinterpret_cast<const wchar_t *>(wide.Begin()), GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                             nullptr);
        return handle != INVALID_HANDLE_VALUE;
    }
    bool Write(const String &data)
    {
        DWORD done = 0;
        return WriteFile(handle, data.Begin(), data.GetCount(), &done, nullptr) &&
               done == (DWORD)data.GetCount();
    }
    ~NewFile()
    {
        if(handle == INVALID_HANDLE_VALUE)
            return;
        // Mark for deletion before closing the handle, including on older Win32
        // SDK targets used by U++. No other writer can open this unfinished file.
        if(!committed)
            DeleteFileW(reinterpret_cast<const wchar_t *>(wide.Begin()));
        CloseHandle(handle);
    }
#else
    int handle = -1;
    String path;
    bool Open(const String &name)
    {
        path = name;
        handle = open(~path, O_WRONLY | O_CREAT | O_EXCL, 0666);
        return handle >= 0;
    }
    bool Write(const String &data)
    {
        int done = 0;
        while(done < data.GetCount())
        {
            auto n = write(handle, data.Begin() + done, data.GetCount() - done);
            if(n <= 0)
                return false;
            done += (int)n;
        }
        return true;
    }
    ~NewFile()
    {
        if(handle < 0)
            return;
        if(!committed)
            unlink(~path);
        close(handle);
    }
#endif
    void Commit() { committed = true; }

private:
    bool committed = false;
};

#ifdef PLATFORM_WIN32
using UiFileBrowserNative::ComOwner;
bool EncodeStarter(const UiFileBrowserCreateRequest &request, String &data)
{
    UiFileBrowserNative::ComApartment apartment(COINIT_APARTMENTTHREADED);
    if(!apartment.IsReady())
        return false;
    ComOwner<IWICImagingFactory> factory;
    ComOwner<IStream> stream;
    ComOwner<IWICBitmapEncoder> encoder;
    ComOwner<IWICBitmapFrameEncode> frame;
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                               IID_PPV_ARGS(factory.Out()))) ||
       FAILED(CreateStreamOnHGlobal(nullptr, TRUE, stream.Out())) ||
       FAILED(factory->CreateEncoder(request.kind == UiFileBrowserCreateKind::Png ? GUID_ContainerFormatPng
                                                                                  : GUID_ContainerFormatJpeg,
                                     nullptr, encoder.Out())) ||
       FAILED(encoder->Initialize(stream.value, WICBitmapEncoderNoCache)) ||
       FAILED(encoder->CreateNewFrame(frame.Out(), nullptr)) || FAILED(frame->Initialize(nullptr)) ||
       FAILED(frame->SetSize(request.size.cx, request.size.cy)))
        return false;
    WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
    if(FAILED(frame->SetPixelFormat(&format)) || !IsEqualGUID(format, GUID_WICPixelFormat24bppBGR))
        return false;
    // Stream rows instead of allocating a full source image; 4K uses a 12 KiB row.
    Vector<byte> row;
    row.SetCount(request.size.cx * 3);
    for(int y = 0; y < request.size.cy; ++y)
    {
        int mix = request.gradient && request.size.cy > 1 ? y * 255 / (request.size.cy - 1) : 0;
        Color c = Blend(request.colour, White(), mix);
        for(int x = 0; x < request.size.cx; ++x)
        {
            row[x * 3] = c.GetB();
            row[x * 3 + 1] = c.GetG();
            row[x * 3 + 2] = c.GetR();
        }
        if(FAILED(frame->WritePixels(1, row.GetCount(), row.GetCount(), row.Begin())))
            return false;
    }
    if(FAILED(frame->Commit()) || FAILED(encoder->Commit()))
        return false;
    STATSTG info{};
    if(FAILED(stream->Stat(&info, STATFLAG_NONAME)) || info.cbSize.QuadPart > 128 * 1024 * 1024)
        return false;
    HGLOBAL memory;
    if(FAILED(GetHGlobalFromStream(stream.value, &memory)))
        return false;
    void *bytes = GlobalLock(memory);
    if(!bytes)
        return false;
    data = String(static_cast<const char *>(bytes), (int)info.cbSize.QuadPart);
    GlobalUnlock(memory);
    return true;
}
#endif
} // namespace
bool CreateFileBrowserItem(const String &folder, const UiFileBrowserCreateRequest &request,
                           String &created_path, String &error)
{
    created_path.Clear();
    error.Clear();
    if((int)request.kind < 0 || (int)request.kind > (int)UiFileBrowserCreateKind::Jpeg)
    {
        error = "Unknown creation type.";
        return false;
    }
    String name = TrimBoth(request.name);
    if(!ValidLeaf(name))
    {
        error = "Enter a valid name, without a path.";
        return false;
    }
    bool image =
        request.kind == UiFileBrowserCreateKind::Png || request.kind == UiFileBrowserCreateKind::Jpeg;
    if(image &&
       (request.size.cx <= 0 || request.size.cy <= 0 || request.size.cx > 8192 || request.size.cy > 8192 ||
        (int64)request.size.cx * request.size.cy > 16 * 1024 * 1024 || IsNull(request.colour)))
    {
        error = "Use dimensions 1–8192, up to 16 million pixels, and an opaque fill colour.";
        return false;
    }
    String ext = ToLower(GetFileExt(name));
    if(request.kind == UiFileBrowserCreateKind::CppPair && (ext == ".h" || ext == ".cpp"))
        name = GetFileTitle(name);
    if(!ValidLeaf(name))
    {
        error = "Enter a valid base name.";
        return false;
    }
    const char *suffix = request.kind == UiFileBrowserCreateKind::Text      ? ".txt"
                         : request.kind == UiFileBrowserCreateKind::CppPair ? ".h"
                         : request.kind == UiFileBrowserCreateKind::Png     ? ".png"
                                                                            : ".jpg";
    if(request.kind != UiFileBrowserCreateKind::Folder && !ToLower(name).EndsWith(suffix))
    {
        if(!(request.kind == UiFileBrowserCreateKind::Jpeg && ToLower(name).EndsWith(".jpeg")))
            name << suffix;
    }
    String path = AppendFileName(folder, name);
    if(FileExists(path) || DirectoryExists(path))
    {
        error = "That name already exists.";
        return false;
    }
    if(request.kind == UiFileBrowserCreateKind::Folder)
    {
        if(!DirectoryCreate(path))
        {
            error = "Could not create the folder. Check permissions.";
            return false;
        }
    }
    else
    {
        NewFile first, second;
        if(!first.Open(path))
        {
            error = "Could not create the file. It may already exist or the folder is read-only.";
            return false;
        }
        String data;
        if(request.kind == UiFileBrowserCreateKind::CppPair)
        {
            String stem = GetFileTitle(name);
            if(!second.Open(AppendFileName(folder, stem + ".cpp")))
            {
                error = "Could not create the source file. It may already exist or the folder is read-only.";
                return false;
            }
            data = "// " + name + "\n#pragma once\n\n";
            if(!second.Write("#include \"" + name + "\"\n\n"))
            {
                error = "Could not write the source file.";
                return false;
            }
        }
        else if(image)
        {
#ifdef PLATFORM_WIN32
            if(!EncodeStarter(request, data))
            {
                error = "The system image encoder could not create this image.";
                return false;
            }
#else
            error = "Starter image creation requires a native PNG/JPEG encoder on this platform.";
            return false;
#endif
        }
        if(!first.Write(data))
        {
            error = "Could not write the file. Check available space.";
            return false;
        }
        first.Commit();
        second.Commit();
    }
    created_path = path;
    return true;
}
} // namespace Upp
