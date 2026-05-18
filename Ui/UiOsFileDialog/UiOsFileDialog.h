#ifndef _Ui_UiOsFileDialog_h_
#define _Ui_UiOsFileDialog_h_

/*
    UiOsFileDialog
    ==============

    Purpose
    - Small platform-native file dialog wrapper for U++ applications.

    Intent
    - Provide a simple, familiar, U++-friendly API for native open/save/folder
      dialogs while keeping platform code isolated in backend source files.
    - Stay intentionally small: this class wraps native OS dialogs; it does not
      try to become a full custom browser.

    Scope
    - Windows: native Common Item Dialog backend.
    - macOS: native NSOpenPanel / NSSavePanel backend (.mm file).
    - Linux: GTK native chooser backend.
    - Other platforms: returns false.

    Design notes
    - Public API is platform-neutral and value-based.
    - Each Execute() call creates a fresh native dialog object.
    - Owner is accepted for future parent-window wiring; the initial version
      keeps ownership optional and does not depend on HWND/NSWindow/GtkWindow.
    - Filters use familiar label + glob patterns, e.g.:
          AddFilter("Images", "*.png;*.jpg;*.jpeg");
      or:
          AddFilter("Images", Vector<String>{"*.png", "*.jpg"});

    Thread context
    - GUI thread only. Native dialogs are modal UI.

    Changelog
    - 2026-04: initial cross-platform wrapper draft for Ui library integration.
*/

#include <Core/Core.h>
#include <CtrlCore/CtrlCore.h>

namespace Upp {

class UiOsFileDialog {
public:
    enum class Mode {
        OpenFile,
        OpenFiles,
        SaveFile,
        PickFolder
    };

    struct Filter : Moveable<Filter> {
        String         label;
        Vector<String> patterns;

        Filter() {}
        Filter(const String& label, const Vector<String>& patterns)
            : label(label)
        {
            this->patterns <<= clone(patterns);
        }
    };

    UiOsFileDialog();

    UiOsFileDialog& SetMode(Mode m);
    UiOsFileDialog& SetTitle(const String& s);
    UiOsFileDialog& SetInitialDirectory(const String& path);
    UiOsFileDialog& SetSuggestedName(const String& name);
    UiOsFileDialog& SetDefaultExtension(const String& ext);

    UiOsFileDialog& SetConfirmOverwrite(bool on = true);
    UiOsFileDialog& SetShowHidden(bool on = true);
    UiOsFileDialog& SetCreatePrompt(bool on = true);
    UiOsFileDialog& SetFollowAliases(bool on = true);

    UiOsFileDialog& ClearFilters();
    UiOsFileDialog& AddFilter(const String& label, const String& patterns);
    UiOsFileDialog& AddFilter(const String& label, const Vector<String>& patterns);
    UiOsFileDialog& SetFilterIndex(int i);

    bool Execute(Ctrl* owner = nullptr);

    String GetPath() const;
    const Vector<String>& GetPaths() const { return result_paths_; }
    int GetFilterIndex() const             { return filter_index_; }

    bool IsConfirmOverwrite() const { return confirm_overwrite_; }
    bool IsShowHidden() const       { return show_hidden_; }
    bool IsCreatePrompt() const     { return create_prompt_; }
    bool IsFollowAliases() const    { return follow_aliases_; }

    void ClearResult();

    static String SelectOpenFile(const String& title = String(),
                                 const String& initial_directory = String(),
                                 Ctrl* owner = nullptr);

    static Vector<String> SelectOpenFiles(const String& title = String(),
                                          const String& initial_directory = String(),
                                          Ctrl* owner = nullptr);

    static String SelectSaveFile(const String& title = String(),
                                 const String& suggested_name = String(),
                                 const String& initial_directory = String(),
                                 Ctrl* owner = nullptr);

    static String SelectFolder(const String& title = String(),
                               const String& initial_directory = String(),
                               Ctrl* owner = nullptr);

private:
    Mode           mode_ = Mode::OpenFile;
    String         title_;
    String         initial_directory_;
    String         suggested_name_;
    String         default_extension_;
    Vector<Filter> filters_;
    int            filter_index_ = 0;

    bool           confirm_overwrite_ = true;
    bool           show_hidden_ = false;
    bool           create_prompt_ = false;
    bool           follow_aliases_ = true;

    Vector<String> result_paths_;

    static Vector<String> SplitPatterns(const String& patterns);
    static String NormalizeExtension(const String& ext);

    bool ExecuteWin(Ctrl* owner);
    bool ExecuteMac(Ctrl* owner);
    bool ExecuteLinux(Ctrl* owner);

    void SetSingleResult(const String& path);
    void AddResult(const String& path);
};

}

#endif
