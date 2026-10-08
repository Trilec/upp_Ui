// Bounded native image/text decoding and generation-based worker publication.
// No Ctrl access occurs here; cancellation suppresses obsolete output rather
// than pretending a blocked OS/codec call is interruptible.
#include "UiFileBrowserPreview.h"
#include "UiFileBrowserNative.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace Upp
{
namespace
{
bool IsText(const String &path)
{
    String ext = ToLower(GetFileExt(path));
    for(const char *type : {".txt", ".ini",  ".h",   ".hpp", ".c",   ".cpp", ".cc",  ".json", ".xml", ".yaml",
                            ".yml", ".toml", ".md",  ".log", ".csv", ".py",  ".js",  ".ts",   ".css", ".html",
                            ".sh",  ".bat",  ".ps1", ".upp", ".var", ".cfg", ".inc", ".inl",  ".url"})
        if(ext == type)
            return true;
    String name = ToLower(GetFileName(path));
    return name == "readme" || name == "license" || name == "makefile" || name == ".gitignore" ||
           name == ".env";
}
#ifdef PLATFORM_WIN32
// Windows' built-in codecs avoid bundling a second, ABI-incompatible libjpeg
// into hosts such as CineView that already use Imaging's libjpeg-turbo.
using UiFileBrowserNative::ComOwner;
Image NativePreview(const UiFileBrowserPreviewRequest &request, Size &original)
{
    UiFileBrowserNative::ComApartment apartment(COINIT_MULTITHREADED);
    if(!apartment.IsReady())
        return Image();
    ComOwner<IWICImagingFactory> factory;
    ComOwner<IWICBitmapDecoder> decoder;
    ComOwner<IWICBitmapFrameDecode> frame;
    ComOwner<IWICBitmapScaler> scaler;
    ComOwner<IWICFormatConverter> converter;
    Vector<char16> path = ToUtf16(request.path);
    path.Add(0);
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                               IID_PPV_ARGS(factory.Out()))) ||
       FAILED(factory->CreateDecoderFromFilename(reinterpret_cast<const wchar_t *>(path.Begin()), nullptr,
                                                 GENERIC_READ, WICDecodeMetadataCacheOnDemand,
                                                 decoder.Out())) ||
       FAILED(decoder->GetFrame(0, frame.Out())))
        return Image();
    UINT width = 0, height = 0;
    if(FAILED(frame->GetSize(&width, &height)) || !width || !height || width > 16384 || height > 16384 ||
       (uint64)width * height > (uint64)request.image_pixels)
        return Image();
    original = Size((int)width, (int)height);
    double scale = min(1., min((double)request.max_size.cx / width, (double)request.max_size.cy / height));
    Size size(max(1, (int)(width * scale)), max(1, (int)(height * scale)));
    if(FAILED(factory->CreateBitmapScaler(scaler.Out())) ||
       FAILED(scaler->Initialize(frame.value, size.cx, size.cy, WICBitmapInterpolationModeFant)) ||
       FAILED(factory->CreateFormatConverter(converter.Out())) ||
       FAILED(converter->Initialize(scaler.value, GUID_WICPixelFormat32bppPRGBA, WICBitmapDitherTypeNone,
                                    nullptr, 0, WICBitmapPaletteTypeCustom)))
        return Image();
    std::vector<byte> bytes((size_t)size.cx * size.cy * 4);
    if(FAILED(converter->CopyPixels(nullptr, size.cx * 4, (UINT)bytes.size(), bytes.data())))
        return Image();
    ImageBuffer image(size);
    for(int i = 0; i < size.cx * size.cy; ++i)
    {
        RGBA &p = image.Begin()[i];
        p.r = bytes[i * 4];
        p.g = bytes[i * 4 + 1];
        p.b = bytes[i * 4 + 2];
        p.a = bytes[i * 4 + 3];
    }
    return Image(image);
}
#endif
} // namespace
UiFileBrowserPreviewData LoadFileBrowserPreview(const UiFileBrowserPreviewRequest &request,
                                                const UiFileBrowserPreviewCancel &cancelled)
{
    UiFileBrowserPreviewData result;
    // Public callers may omit cancellation. Bound thumbnail allocations even
    // when a host provides unreasonable limits; providers have their own policy.
    auto stopped = [&] { return cancelled && cancelled(); };
    if(stopped())
        return result;
    if(request.max_size.cx <= 0 || request.max_size.cy <= 0 || request.max_size.cx > 4096 ||
       request.max_size.cy > 4096 || request.image_pixels <= 0 || request.image_file_bytes <= 0)
    {
        result.info = "Invalid preview limits";
        return result;
    }
    String ext = ToLower(GetFileExt(request.path));
    if(ext == ".jpg" || ext == ".jpeg" || ext == ".png")
    {
        FileIn file(request.path);
        if(!file.IsOpen() || file.GetSize() > request.image_file_bytes)
        {
            result.info = "Image unavailable or exceeds preview file limit";
            return result;
        }
        Size original;
#ifdef PLATFORM_WIN32
        result.image = NativePreview(request, original);
#else
        // Portable fallback uses codecs registered by the native U++ host.
        One<StreamRaster> raster = StreamRaster::OpenAny(file);
        if(raster)
        {
            original = raster->GetSize();
            if(original.cx > 0 && original.cy > 0 && original.cx <= 16384 && original.cy <= 16384 &&
               (int64)original.cx * original.cy <= request.image_pixels)
            {
                Image image = raster->GetImage([&](int, int) { return stopped(); });
                double scale = min(1., min((double)request.max_size.cx / original.cx,
                                           (double)request.max_size.cy / original.cy));
                if(!IsNull(image))
                    result.image = Rescale(
                        image, Size(max(1, (int)(original.cx * scale)), max(1, (int)(original.cy * scale))));
            }
        }
#endif
        if(stopped())
            return UiFileBrowserPreviewData();
        result.type = IsNull(result.image) ? UiFileBrowserPreviewData::Type::Message
                                           : UiFileBrowserPreviewData::Type::Image;
        result.image_size = original;
        if(IsNull(result.image))
            result.info = "Image decode failed or exceeds preview pixel limit";
        return result;
    }
    if(IsText(request.path))
    {
        FileIn file(request.path);
        if(!file.IsOpen())
        {
            result.info = "Text file unavailable";
            return result;
        }
        String data = file.Get(64 * 1024); // Bounded prefix even for huge logs/single lines.
        if(data.StartsWith("\xef\xbb\xbf"))
            data = data.Mid(3);
        bool little = data.GetCount() >= 2 && (byte)data[0] == 255 && (byte)data[1] == 254;
        bool big = data.GetCount() >= 2 && (byte)data[0] == 254 && (byte)data[1] == 255;
        if(little || big)
        {
            WString decoded;
            for(int i = 2; i + 1 < data.GetCount(); i += 2)
            {
                int word = little ? (byte)data[i] + ((byte)data[i + 1] << 8)
                                  : ((byte)data[i] << 8) + (byte)data[i + 1];
                if(word >= 0xd800 && word <= 0xdbff && i + 3 < data.GetCount())
                {
                    int next = little ? (byte)data[i + 2] + ((byte)data[i + 3] << 8)
                                      : ((byte)data[i + 2] << 8) + (byte)data[i + 3];
                    if(next >= 0xdc00 && next <= 0xdfff)
                    {
                        word = 0x10000 + ((word - 0xd800) << 10) + next - 0xdc00;
                        i += 2;
                    }
                }
                decoded.Cat(word);
            }
            data = ToUtf8(decoded);
        }
        if(data.Find(char(0)) >= 0)
        {
            result.info = "Binary content; text preview unavailable";
            return result;
        }
        int offset = 0, lines = 0;
        while(lines < 10 && offset < data.GetCount())
        {
            int end = data.Find('\n', offset);
            if(end < 0)
                end = data.GetCount();
            String line = data.Mid(offset, end - offset);
            if(line.EndsWith("\r"))
                line.Trim(line.GetCount() - 1);
            if(lines++)
                result.text << '\n';
            for(char ch : line)
            {
                if(ch == '\t')
                    result.text << "    ";
                else if((byte)ch >= 32)
                    result.text.Cat(ch);
            }
            offset = end + 1;
        }
        if(stopped())
            return UiFileBrowserPreviewData();
        result.type = UiFileBrowserPreviewData::Type::Text;
        if(ext == ".url")
        {
            // InternetShortcut is an INI document. Show its target as plain text;
            // do not resolve, fetch, or launch it. Fall back to its bounded text.
            bool shortcut = false;
            for(String line : Split(data, '\n'))
            {
                line = TrimBoth(line);
                if(line.StartsWith("["))
                    shortcut = ToLower(line) == "[internetshortcut]";
                else if(shortcut && ToLower(line.Left(4)) == "url=")
                {
                    result.text = TrimBoth(line.Mid(4));
                    break;
                }
            }
        }
        if(!lines)
            result.info = "Empty text file";
        return result;
    }
    result.info = "No preview provider for this file type";
    return result;
}
struct UiFileBrowserPreviewWorker::State
{
    std::mutex mutex;
    std::condition_variable wake;
    std::atomic<uint64> generation{0};
    bool stop = false, pending = false, has_ready = false;
    UiFileBrowserPreviewRequest request;
    UiFileBrowserPreviewProvider provider;
    Result ready;
};
UiFileBrowserPreviewWorker::UiFileBrowserPreviewWorker() : state_(std::make_shared<State>())
{
    Thread::Start(
        [state = state_]
        {
            for(;;)
            {
                UiFileBrowserPreviewRequest request;
                UiFileBrowserPreviewProvider provider;
                uint64 generation;
                {
                    std::unique_lock<std::mutex> lock(state->mutex);
                    while(!state->stop && !state->pending && !Thread::IsShutdownThreads())
                        state->wake.wait_for(lock, std::chrono::milliseconds(100));
                    if(state->stop || Thread::IsShutdownThreads())
                        return;
                    request = state->request;
                    provider = state->provider;
                    generation = state->generation;
                    state->pending = false;
                }
                auto cancelled = [state, generation]
                { return state->generation.load() != generation || Thread::IsShutdownThreads(); };
                Result result;
                result.generation = generation;
                try
                {
                    if(!cancelled() && (!provider || !provider(request, result.data, cancelled)))
                        result.data = LoadFileBrowserPreview(request, cancelled);
                }
                catch(const std::exception &e)
                {
                    result.data = UiFileBrowserPreviewData();
                    result.data.info = String("Preview failed: ") + e.what();
                }
                catch(...)
                {
                    result.data = UiFileBrowserPreviewData();
                    result.data.info = "Preview failed";
                }
                std::lock_guard<std::mutex> lock(state->mutex);
                if(state->stop)
                    return;
                if(!cancelled())
                {
                    state->ready = std::move(result);
                    state->has_ready = true;
                }
            }
        });
}
UiFileBrowserPreviewWorker::~UiFileBrowserPreviewWorker()
{
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->stop = true;
        ++state_->generation;
    }
    state_->wake.notify_one();
}
uint64 UiFileBrowserPreviewWorker::Request(UiFileBrowserPreviewRequest request,
                                           UiFileBrowserPreviewProvider provider)
{
    std::lock_guard<std::mutex> lock(state_->mutex);
    uint64 generation = ++state_->generation;
    state_->request = std::move(request);
    state_->provider = std::move(provider);
    state_->pending = true;
    state_->has_ready = false;
    state_->ready = Result();
    state_->wake.notify_one();
    return generation;
}
void UiFileBrowserPreviewWorker::Cancel()
{
    std::lock_guard<std::mutex> lock(state_->mutex);
    ++state_->generation;
    state_->pending = state_->has_ready = false;
    state_->ready = Result();
}
bool UiFileBrowserPreviewWorker::Poll(Result &result)
{
    std::lock_guard<std::mutex> lock(state_->mutex);
    if(!state_->has_ready)
        return false;
    result = std::move(state_->ready);
    state_->has_ready = false;
    return true;
}
} // namespace Upp
