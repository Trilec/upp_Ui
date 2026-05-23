/*
    UiOsFileDialogDemo
    ==================

    Purpose
    - Shared-shell smoke demo for UiOsFileDialog, the native OS dialog wrapper.

    Intent
    - Keep the native dialog API visible beside the same version/theme/exit,
      usage, state, and property shell used by the current Ui demos.

    Notes
    - The native dialogs themselves are owned by the operating system; this
      demo validates the Ui launch controls and wrapper result contract.

    Changelog
    - 2026-05: moved from standalone sample window to BuilderDemoSupport shell.
*/

#include "../BuilderDemoSupport.h"
#include <Ui/UiOsFileDialog/UiOsFileDialog.h>

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

class UiOsFileDialogDemoWindow : public BuilderWindowBase {
public:
    typedef UiOsFileDialogDemoWindow CLASSNAME;

    UiOsFileDialogDemoWindow()
        : BuilderWindowBase("UiOsFileDialog Demo",
                            "UiOsFileDialog",
                            "Native OS file dialog wrapper with Ui-styled launch controls.",
                            1120, 720)
    {
        open_file_.SetText("Open file").SetIcon(ICON_DESIGN_FOLDER_48()).SetIconSize(DPI(15), DPI(15));
        open_files_.SetText("Open files").SetIcon(ICON_DESIGN_FOLDER_48()).SetIconSize(DPI(15), DPI(15));
        save_file_.SetText("Save file").SetIcon(ICON_DESIGN_CHECK_SMALL_48()).SetIconSize(DPI(15), DPI(15));
        pick_folder_.SetText("Pick folder").SetIcon(ICON_DESIGN_FOLDER_48()).SetIconSize(DPI(15), DPI(15));

        result_.SetText("No selection").SetAlign(UiAlign::LEFT, UiAlign::TOP).SetSelectable(true);

        Add(open_file_);
        Add(open_files_);
        Add(save_file_);
        Add(pick_folder_);
        Add(result_);

        AddStateRow(StateBox(), state_mode_row_, state_mode_label_, state_mode_value_, "Mode");
        AddStateRow(StateBox(), state_result_row_, state_result_label_, state_result_value_, "Result");

        open_file_.WhenAction = [=] { SelectOpenFile(); };
        open_files_.WhenAction = [=] { SelectOpenFiles(); };
        save_file_.WhenAction = [=] { SelectSaveFile(); };
        pick_folder_.WhenAction = [=] { SelectFolder(); };

        FinishInit();
        RefreshState("Idle", "No selection");
        SetUsageCode(BuildUsageCode());
    }

private:
    void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect().Deflated(DPI(28));
        int bw = DPI(150);
        int bh = DPI(34);
        int gap = DPI(10);
        int total = bw * 2 + gap;
        int x = canvas.left + max(0, (canvas.GetWidth() - total) / 2);
        int block_h = (bh + gap) * 2 + DPI(16) + DPI(120);
        int y = canvas.top + max(0, (canvas.GetHeight() - block_h) / 2);

        open_file_.SetRect(x, y, bw, bh);
        open_files_.SetRect(x + bw + gap, y, bw, bh);
        save_file_.SetRect(x, y + bh + gap, bw, bh);
        pick_folder_.SetRect(x + bw + gap, y + bh + gap, bw, bh);

        Rect rr(x, y + (bh + gap) * 2 + DPI(16), total, DPI(120));
        result_.SetRect(rr);
    }

    void ApplyDemoTheme() override
    {
        UiButton::Style standard = UiTheme::ResolveButton(UiRole::Standard);
        UiButton::Style accent = UiTheme::ResolveButton(UiRole::Accent);
        open_file_.SetCustomStyle(accent);
        open_files_.SetCustomStyle(standard);
        save_file_.SetCustomStyle(standard);
        pick_folder_.SetCustomStyle(standard);
        result_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Standard));
    }

    void SelectOpenFile()
    {
        UiOsFileDialog dlg;
        dlg.SetMode(UiOsFileDialog::Mode::OpenFile)
           .SetTitle("Open file")
           .AddFilter("Images", "*.png;*.jpg;*.jpeg")
           .AddFilter("All files", "*.*");
        if(dlg.Execute(this))
            RefreshState("Open file", dlg.GetPath());
    }

    void SelectOpenFiles()
    {
        UiOsFileDialog dlg;
        dlg.SetMode(UiOsFileDialog::Mode::OpenFiles)
           .SetTitle("Open files")
           .AddFilter("Documents", "*.txt;*.md;*.cpp;*.h")
           .AddFilter("All files", "*.*");
        if(dlg.Execute(this))
            RefreshState("Open files", Join(dlg.GetPaths(), "\n"));
    }

    void SelectSaveFile()
    {
        UiOsFileDialog dlg;
        dlg.SetMode(UiOsFileDialog::Mode::SaveFile)
           .SetTitle("Save file")
           .SetSuggestedName("untitled.txt")
           .SetDefaultExtension("txt")
           .AddFilter("Text", "*.txt")
           .AddFilter("All files", "*.*");
        if(dlg.Execute(this))
            RefreshState("Save file", dlg.GetPath());
    }

    void SelectFolder()
    {
        UiOsFileDialog dlg;
        dlg.SetMode(UiOsFileDialog::Mode::PickFolder)
           .SetTitle("Pick folder");
        if(dlg.Execute(this))
            RefreshState("Pick folder", dlg.GetPath());
    }

    void RefreshState(const String& mode, const String& path)
    {
        String shown = path.IsEmpty() ? "No selection" : path;
        result_.SetText(shown);
        state_mode_value_.SetText(mode);
        state_result_value_.SetText(shown);
    }

    String BuildUsageCode() const
    {
        return R"(#include <Ui/Ui.h>
#include <Ui/UiOsFileDialog/UiOsFileDialog.h>

UiOsFileDialog dlg;
dlg.SetMode(UiOsFileDialog::Mode::OpenFile)
   .SetTitle("Open file")
   .AddFilter("Images", "*.png;*.jpg;*.jpeg")
   .AddFilter("All files", "*.*");

if(dlg.Execute(this)) {
    String selected = dlg.GetPath();
}
)";
    }

    UiButton open_file_;
    UiButton open_files_;
    UiButton save_file_;
    UiButton pick_folder_;
    UiLabel result_;

    UiBoxLayout state_mode_row_ { UiDirection::H };
    UiBoxLayout state_result_row_ { UiDirection::H };
    UiLabel state_mode_label_;
    UiLabel state_mode_value_;
    UiLabel state_result_label_;
    UiLabel state_result_value_;
};

}

GUI_APP_MAIN
{
    UiOsFileDialogDemoWindow().Run();
}
