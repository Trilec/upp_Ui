#include "UiOsFileDialog.h"

#ifdef PLATFORM_WIN32

#include <objbase.h>
#include <shobjidl.h>

namespace Upp {
namespace {

struct CoInitScope {
    HRESULT hr = E_FAIL;
    bool    owns_uninit = false;

    CoInitScope()
    {
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        owns_uninit = SUCCEEDED(hr);
    }

    ~CoInitScope()
    {
        if(owns_uninit)
            CoUninitialize();
    }

    bool Ok() const
    {
        // RPC_E_CHANGED_MODE still means COM is already initialized for the thread.
        return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
    }
};

template <class T>
struct ComPtr {
    T* p = nullptr;

    ~ComPtr()
    {
        if(p)
            p->Release();
    }

    T** operator~()       { return &p; }
    T* operator->() const { return p; }
    operator bool() const { return p != nullptr; }
};

static WString ToWideString(const String& s)
{
    return s.ToWString();
}

static String ToUppString(const wchar_t* ws)
{
    return ws ? String(WString(ws)) : String();
}

static DWORD GetOpenOptions(const UiOsFileDialog& dlg, UiOsFileDialog::Mode mode)
{
    DWORD opt = FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;

    if(mode == UiOsFileDialog::Mode::OpenFiles)
        opt |= FOS_ALLOWMULTISELECT;
    if(mode == UiOsFileDialog::Mode::PickFolder)
        opt |= FOS_PICKFOLDERS;
    if(dlg.IsShowHidden())
        opt |= FOS_FORCESHOWHIDDEN;
    if(dlg.IsCreatePrompt())
        opt |= FOS_CREATEPROMPT;
    if(!dlg.IsFollowAliases())
        opt |= FOS_NODEREFERENCELINKS;

    return opt;
}

static DWORD GetSaveOptions(const UiOsFileDialog& dlg)
{
    DWORD opt = FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;

    if(dlg.IsConfirmOverwrite())
        opt |= FOS_OVERWRITEPROMPT;
    if(dlg.IsShowHidden())
        opt |= FOS_FORCESHOWHIDDEN;
    if(dlg.IsCreatePrompt())
        opt |= FOS_CREATEPROMPT;
    if(!dlg.IsFollowAliases())
        opt |= FOS_NODEREFERENCELINKS;

    return opt;
}

struct FilterStorage {
    Vector<WString> names;
    Vector<WString> specs;
    Vector<COMDLG_FILTERSPEC> items;

    void Build(const Vector<UiOsFileDialog::Filter>& filters)
    {
        names.Clear();
        specs.Clear();
        items.Clear();

        for(const auto& f : filters) {
            names.Add(f.label);

            String joined;
            for(int i = 0; i < f.patterns.GetCount(); i++) {
                if(i)
                    joined << ';';
                joined << f.patterns[i];
            }
            specs.Add(joined.ToWString());
        }

        items.SetCount(filters.GetCount());
        for(int i = 0; i < filters.GetCount(); i++) {
            items[i].pszName = ~names[i];
            items[i].pszSpec = ~specs[i];
        }
    }
};

static bool SetDefaultFolder(IFileDialog* dlg, const String& path)
{
    if(path.IsEmpty())
        return true;

    ComPtr<IShellItem> folder;
    HRESULT hr = SHCreateItemFromParsingName(ToWideString(path), nullptr, IID_IShellItem, (void**)~folder);
    if(FAILED(hr) || !folder)
        return false;

    dlg->SetDefaultFolder(folder.p);
    dlg->SetFolder(folder.p);
    return true;
}

static void CaptureSingleResult(IFileDialog* dlg, UiOsFileDialog& out)
{
    ComPtr<IShellItem> item;
    if(FAILED(dlg->GetResult(~item)) || !item)
        return;

    PWSTR path = nullptr;
    if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path) {
        out.SetSingleResult(ToUppString(path));
        CoTaskMemFree(path);
    }
}

static void CaptureMultiResult(IFileOpenDialog* dlg, UiOsFileDialog& out)
{
    ComPtr<IShellItemArray> items;
    if(FAILED(dlg->GetResults(~items)) || !items)
        return;

    DWORD count = 0;
    if(FAILED(items->GetCount(&count)))
        return;

    for(DWORD i = 0; i < count; i++) {
        ComPtr<IShellItem> item;
        if(FAILED(items->GetItemAt(i, ~item)) || !item)
            continue;

        PWSTR path = nullptr;
        if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path) {
            out.AddResult(ToUppString(path));
            CoTaskMemFree(path);
        }
    }
}

}

bool UiOsFileDialog::ExecuteWin(Ctrl*)
{
    CoInitScope com;
    if(!com.Ok())
        return false;

    if(mode_ == Mode::SaveFile) {
        ComPtr<IFileSaveDialog> save;
        HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog,
                                      nullptr,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IFileSaveDialog,
                                      (void**)~save);
        if(FAILED(hr) || !save)
            return false;

        DWORD opt = 0;
        if(FAILED(save->GetOptions(&opt)))
            return false;

        opt |= GetSaveOptions(*this);
        if(FAILED(save->SetOptions(opt)))
            return false;

        if(!title_.IsEmpty())
            save->SetTitle(ToWideString(title_));
        if(!suggested_name_.IsEmpty())
            save->SetFileName(ToWideString(suggested_name_));
        if(!default_extension_.IsEmpty())
            save->SetDefaultExtension(ToWideString(default_extension_));

        SetDefaultFolder(save.p, initial_directory_);

        FilterStorage fs;
        fs.Build(filters_);
        if(!fs.items.IsEmpty()) {
            save->SetFileTypes((UINT)fs.items.GetCount(), fs.items.Begin());

            UINT idx = (UINT)minmax(filter_index_ + 1, 1, fs.items.GetCount());
            save->SetFileTypeIndex(idx);
        }

        hr = save->Show(nullptr);
        if(hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            return false;
        if(FAILED(hr))
            return false;

        CaptureSingleResult(save.p, *this);

        if(!fs.items.IsEmpty()) {
            UINT idx = 0;
            if(SUCCEEDED(save->GetFileTypeIndex(&idx)) && idx > 0)
                filter_index_ = (int)idx - 1;
        }

        return !result_paths_.IsEmpty();
    }

    ComPtr<IFileOpenDialog> open;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IFileOpenDialog,
                                  (void**)~open);
    if(FAILED(hr) || !open)
        return false;

    DWORD opt = 0;
    if(FAILED(open->GetOptions(&opt)))
        return false;

    opt |= GetOpenOptions(*this, mode_);
    if(FAILED(open->SetOptions(opt)))
        return false;

    if(!title_.IsEmpty())
        open->SetTitle(ToWideString(title_));

    SetDefaultFolder(open.p, initial_directory_);

    FilterStorage fs;
    fs.Build(filters_);
    if(!fs.items.IsEmpty() && mode_ != Mode::PickFolder) {
        open->SetFileTypes((UINT)fs.items.GetCount(), fs.items.Begin());

        UINT idx = (UINT)minmax(filter_index_ + 1, 1, fs.items.GetCount());
        open->SetFileTypeIndex(idx);
    }

    hr = open->Show(nullptr);
    if(hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        return false;
    if(FAILED(hr))
        return false;

    if(mode_ == Mode::OpenFiles)
        CaptureMultiResult(open.p, *this);
    else
        CaptureSingleResult(open.p, *this);

    if(!fs.items.IsEmpty() && mode_ != Mode::PickFolder) {
        UINT idx = 0;
        if(SUCCEEDED(open->GetFileTypeIndex(&idx)) && idx > 0)
            filter_index_ = (int)idx - 1;
    }

    return !result_paths_.IsEmpty();
}

}
#endif