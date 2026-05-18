#include "UiOsFileDialog.h"

namespace Upp {

UiOsFileDialog::UiOsFileDialog()
{
}

UiOsFileDialog& UiOsFileDialog::SetMode(Mode m)
{
    mode_ = m;
    return *this;
}

UiOsFileDialog& UiOsFileDialog::SetTitle(const String& s)
{
    title_ = s;
    return *this;
}

UiOsFileDialog& UiOsFileDialog::SetInitialDirectory(const String& path)
{
    initial_directory_ = path;
    return *this;
}

UiOsFileDialog& UiOsFileDialog::SetSuggestedName(const String& name)
{
    suggested_name_ = name;
    return *this;
}

UiOsFileDialog& UiOsFileDialog::SetDefaultExtension(const String& ext)
{
    default_extension_ = NormalizeExtension(ext);
    return *this;
}

UiOsFileDialog& UiOsFileDialog::SetConfirmOverwrite(bool on)
{
    confirm_overwrite_ = on;
    return *this;
}

UiOsFileDialog& UiOsFileDialog::SetShowHidden(bool on)
{
    show_hidden_ = on;
    return *this;
}

UiOsFileDialog& UiOsFileDialog::SetCreatePrompt(bool on)
{
    create_prompt_ = on;
    return *this;
}

UiOsFileDialog& UiOsFileDialog::SetFollowAliases(bool on)
{
    follow_aliases_ = on;
    return *this;
}

UiOsFileDialog& UiOsFileDialog::ClearFilters()
{
    filters_.Clear();
    filter_index_ = 0;
    return *this;
}

UiOsFileDialog& UiOsFileDialog::AddFilter(const String& label, const String& patterns)
{
    return AddFilter(label, SplitPatterns(patterns));
}

UiOsFileDialog& UiOsFileDialog::AddFilter(const String& label, const Vector<String>& patterns)
{
    Filter& f = filters_.Add();
    f.label = label;
    f.patterns <<= clone(patterns);
    return *this;
}

UiOsFileDialog& UiOsFileDialog::SetFilterIndex(int i)
{
    filter_index_ = max(0, i);
    return *this;
}

void UiOsFileDialog::ClearResult()
{
    result_paths_.Clear();
}

String UiOsFileDialog::GetPath() const
{
    return result_paths_.IsEmpty() ? String() : result_paths_[0];
}

void UiOsFileDialog::SetSingleResult(const String& path)
{
    result_paths_.Clear();
    AddResult(path);
}

void UiOsFileDialog::AddResult(const String& path)
{
    if(IsNull(path) || path.IsEmpty())
        return;
    result_paths_.Add(path);
}

Vector<String> UiOsFileDialog::SplitPatterns(const String& patterns)
{
    Vector<String> out;
    for(String part : Split(patterns, ';')) {
        part = TrimBoth(part);
        if(!part.IsEmpty())
            out.Add(part);
    }
    return out;
}

String UiOsFileDialog::NormalizeExtension(const String& ext)
{
    String s = TrimBoth(ext);
    if(s.IsEmpty())
        return s;
    if(s[0] == '.')
        s.Remove(0, 1);
    return s;
}

bool UiOsFileDialog::Execute(Ctrl* owner)
{
    ClearResult();

#if defined(PLATFORM_WIN32)
    return ExecuteWin(owner);
#elif defined(PLATFORM_COCOA)
    return ExecuteMac(owner);
#elif defined(PLATFORM_POSIX)
    return ExecuteLinux(owner);
#else
    return false;
#endif
}

String UiOsFileDialog::SelectOpenFile(const String& title,
                                      const String& initial_directory,
                                      Ctrl* owner)
{
    UiOsFileDialog dlg;
    dlg.SetMode(Mode::OpenFile)
       .SetTitle(title)
       .SetInitialDirectory(initial_directory);

    return dlg.Execute(owner) ? dlg.GetPath() : String();
}

Vector<String> UiOsFileDialog::SelectOpenFiles(const String& title,
                                               const String& initial_directory,
                                               Ctrl* owner)
{
    UiOsFileDialog dlg;
    dlg.SetMode(Mode::OpenFiles)
       .SetTitle(title)
       .SetInitialDirectory(initial_directory);

    if(!dlg.Execute(owner))
        return Vector<String>();
    Vector<String> out;
    out <<= clone(dlg.GetPaths());
    return out;
}

String UiOsFileDialog::SelectSaveFile(const String& title,
                                      const String& suggested_name,
                                      const String& initial_directory,
                                      Ctrl* owner)
{
    UiOsFileDialog dlg;
    dlg.SetMode(Mode::SaveFile)
       .SetTitle(title)
       .SetSuggestedName(suggested_name)
       .SetInitialDirectory(initial_directory);

    return dlg.Execute(owner) ? dlg.GetPath() : String();
}

String UiOsFileDialog::SelectFolder(const String& title,
                                    const String& initial_directory,
                                    Ctrl* owner)
{
    UiOsFileDialog dlg;
    dlg.SetMode(Mode::PickFolder)
       .SetTitle(title)
       .SetInitialDirectory(initial_directory);

    return dlg.Execute(owner) ? dlg.GetPath() : String();
}

}
