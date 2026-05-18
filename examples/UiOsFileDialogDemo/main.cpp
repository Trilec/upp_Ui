#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Ui/UiOsFileDialog/UiOsFileDialog.h>

using namespace Upp;

namespace {

struct UiOsFileDialogDemoWindow : TopWindow {
    typedef UiOsFileDialogDemoWindow CLASSNAME;

    UiBoxLayout root { UiDirection::V };
    UiTitleCard header;
    UiButton open_file;
    UiButton open_files;
    UiButton save_file;
    UiButton pick_folder;
    UiLabel result;

    UiOsFileDialogDemoWindow()
    {
        Title("UiOsFileDialog Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(620), DPI(360));

        UiTheme::Set(UiThemePreset::Minimal, UiThemeMode::Light);

        Add(root.SizePos());
        root.SetInset(DPI(18)).SetGap(DPI(10));

        header.SetTitle("UiOsFileDialog")
              .SetSubTitle("Native OS file dialog wrapper with Ui-styled launch controls.");
        root.Add(header).Fixed(DPI(86));

        open_file.SetText("Open file");
        open_files.SetText("Open files");
        save_file.SetText("Save file");
        pick_folder.SetText("Pick folder");

        root.Add(open_file).Fixed(DPI(36));
        root.Add(open_files).Fixed(DPI(36));
        root.Add(save_file).Fixed(DPI(36));
        root.Add(pick_folder).Fixed(DPI(36));
        root.Add(result).Fixed(DPI(72));

        result.SetText("No selection").SetAlign(UiAlign::LEFT, UiAlign::TOP);

        open_file.WhenAction = [=] { SelectOpenFile(); };
        open_files.WhenAction = [=] { SelectOpenFiles(); };
        save_file.WhenAction = [=] { SelectSaveFile(); };
        pick_folder.WhenAction = [=] { SelectFolder(); };
    }

    void SelectOpenFile()
    {
        UiOsFileDialog dlg;
        dlg.SetMode(UiOsFileDialog::Mode::OpenFile)
           .SetTitle("Open file")
           .AddFilter("Images", "*.png;*.jpg;*.jpeg")
           .AddFilter("All files", "*.*");
        if(dlg.Execute(this))
            result.SetText(dlg.GetPath());
    }

    void SelectOpenFiles()
    {
        UiOsFileDialog dlg;
        dlg.SetMode(UiOsFileDialog::Mode::OpenFiles)
           .SetTitle("Open files")
           .AddFilter("Documents", "*.txt;*.md;*.cpp;*.h")
           .AddFilter("All files", "*.*");
        if(dlg.Execute(this))
            result.SetText(Join(dlg.GetPaths(), "\n"));
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
            result.SetText(dlg.GetPath());
    }

    void SelectFolder()
    {
        UiOsFileDialog dlg;
        dlg.SetMode(UiOsFileDialog::Mode::PickFolder)
           .SetTitle("Pick folder");
        if(dlg.Execute(this))
            result.SetText(dlg.GetPath());
    }
};

}

GUI_APP_MAIN
{
    UiOsFileDialogDemoWindow().Run();
}
