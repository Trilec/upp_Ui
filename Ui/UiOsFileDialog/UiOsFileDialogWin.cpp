#include "UiOsFileDialog.h"

#ifdef PLATFORM_WIN32

#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>

namespace Upp {
namespace {

static std::wstring ToWinWide(const String& s)
{
    return s.ToWString().ToStd();
}

static String ToUppString(const wchar_t* ws)
{
    return ws ? WString(std::wstring(ws)).ToString() : String();
}

static std::wstring BuildFilterString(const Vector<UiOsFileDialog::Filter>& filters)
{
    std::wstring out;
    if(filters.IsEmpty()) {
        out += L"All files";
        out.push_back(L'\0');
        out += L"*.*";
        out.push_back(L'\0');
        out.push_back(L'\0');
        return out;
    }

    for(const auto& f : filters) {
        out += ToWinWide(f.label);
        out.push_back(L'\0');

        String patterns;
        for(int i = 0; i < f.patterns.GetCount(); i++) {
            if(i)
                patterns << ';';
            patterns << f.patterns[i];
        }
        out += ToWinWide(patterns);
        out.push_back(L'\0');
    }
    out.push_back(L'\0');
    return out;
}

static Vector<String> ParseMultiSelectBuffer(const Vector<wchar_t>& buffer)
{
    Vector<String> out;
    const wchar_t* first = buffer.Begin();
    if(!first || !*first)
        return out;

    const wchar_t* p = first + wcslen(first) + 1;
    if(!*p) {
        out.Add(ToUppString(first));
        return out;
    }

    String dir = ToUppString(first);
    while(*p) {
        String name = ToUppString(p);
        out.Add(AppendFileName(dir, name));
        p += wcslen(p) + 1;
    }
    return out;
}

}

bool UiOsFileDialog::ExecuteWin(Ctrl*)
{
    Vector<wchar_t> file_buffer;
    file_buffer.SetCount(mode_ == Mode::OpenFiles ? 32768 : 4096, 0);

    std::wstring title = ToWinWide(title_);
    std::wstring initial_dir = ToWinWide(initial_directory_);
    std::wstring suggested = ToWinWide(suggested_name_);
    std::wstring defext = ToWinWide(default_extension_);
    std::wstring filter = BuildFilterString(filters_);

    if(mode_ == Mode::PickFolder) {
        BROWSEINFOW bi;
        Zero(bi);
        bi.lpszTitle = title.empty() ? nullptr : title.c_str();
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

        PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
        if(!pidl)
            return false;

        wchar_t path[MAX_PATH];
        bool ok = SHGetPathFromIDListW(pidl, path);
        CoTaskMemFree(pidl);
        if(!ok)
            return false;

        SetSingleResult(ToUppString(path));
        return !result_paths_.IsEmpty();
    }

    if(!suggested.empty())
        wcsncpy(file_buffer.Begin(), suggested.c_str(), file_buffer.GetCount() - 1);

    OPENFILENAMEW ofn;
    Zero(ofn);
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = file_buffer.Begin();
    ofn.nMaxFile = file_buffer.GetCount();
    ofn.lpstrTitle = title.empty() ? nullptr : title.c_str();
    ofn.lpstrInitialDir = initial_dir.empty() ? nullptr : initial_dir.c_str();
    ofn.lpstrFilter = filter.c_str();
    ofn.nFilterIndex = (DWORD)max(1, filter_index_ + 1);
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if(mode_ == Mode::OpenFile || mode_ == Mode::OpenFiles) {
        ofn.Flags |= OFN_FILEMUSTEXIST;
        if(mode_ == Mode::OpenFiles)
            ofn.Flags |= OFN_ALLOWMULTISELECT;

        if(!GetOpenFileNameW(&ofn))
            return false;

        Vector<String> paths = ParseMultiSelectBuffer(file_buffer);
        for(const String& path : paths)
            AddResult(path);
    }
    else {
        if(confirm_overwrite_)
            ofn.Flags |= OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = defext.empty() ? nullptr : defext.c_str();

        if(!GetSaveFileNameW(&ofn))
            return false;

        SetSingleResult(ToUppString(file_buffer.Begin()));
    }

    filter_index_ = max(0, (int)ofn.nFilterIndex - 1);
    return !result_paths_.IsEmpty();
}

}

#endif
