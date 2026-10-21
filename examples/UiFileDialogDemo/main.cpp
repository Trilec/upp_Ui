// One self-contained file-selection reference. Each backend retains its own
// inspector state; preview and generated C++ read that same configuration.
#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Ui/UiOsFileDialog/UiOsFileDialog.h>
#include <Ui/UiFileBrowser/UiFileBrowser.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>
#include <plugin/png/png.h>
#include "BrowserChecks.h"

using namespace Upp;

namespace {
class Demo : public TopWindow {
    enum Backend { Browser, Native, UppChooser };
    PropertyEditorFactory factory_;
    PropertyEditorModel models_[3];
    Backend backend_ = Browser;
    bool scan_browser_;
    UiTitleCard header_;
    UiBoxLayout actions_{UiDirection::H}, tools_{UiDirection::H}, selectors_{UiDirection::H};
    UiToolButton theme_, help_, exit_, inspector_mode_, code_mode_, copy_;
    UiPanel preview_, right_, inspector_page_, code_page_;
    UiStack pages_;
    PropertyEditor inspector_;
    UiMultiEdit code_, result_;
    UiButton backend_buttons_[3], launch_;
    UiLabel caption_;
    UiFileBrowser browser_;
    Color window_face_ = SColorFace();

    PropertyEditorModel& Model() { return models_[backend_]; }
    Value ValueOf(const String& id) const {
        const auto* item = models_[backend_].Find(id);
        return item ? item->value : Value();
    }
    String Text(const String& id) const { return AsString(ValueOf(id)); }
    String ModeName() const {
        String mode = Text("mode");
        return mode == "Open files" ? "OpenFiles" : mode == "Save file" ? "SaveFile" :
               mode == "Folder" ? "PickFolder" : "OpenFile";
    }
    bool DiscoverSequences() const { return (bool)ValueOf("advanced") && (bool)ValueOf("grouped"); }

    void BuildModels() {
        for(int i = 0; i < 3; ++i) {
            auto& model = models_[i];
            model.AddText("title", "Dialog title", "Choose a file", "Dialog");
            model.AddText("directory", "Initial directory", GetHomeDirectory(), "Dialog");
            if(i == Browser) {
                model.AddBoolean("advanced", "Advanced browser", true, "Browser");
                model.AddBoolean("grouped", "Group image sequences", true, "Browser");
                model.AddChoice("size", "View size", "Medium", "Browser")
                    .AddChoice("Small", "Small").AddChoice("Medium", "Medium").AddChoice("Large", "Large");
            }
            else {
                model.AddChoice("mode", "Mode", "Open file", "Dialog")
                    .AddChoice("Open file", "Open file").AddChoice("Open files", "Open files")
                    .AddChoice("Save file", "Save file").AddChoice("Folder", "Folder");
                model.AddText("filter_label", "Filter label", "Images", "Filters");
                model.AddText("patterns", "Patterns", "*.png;*.jpg;*.jpeg", "Filters");
                model.AddNumericInt("filter_index", "Initial filter", 0, 0, 1, 1, "Filters");
                model.AddText("name", "Suggested name", "untitled.txt", "Save");
                model.AddText("extension", "Default extension", "txt", "Save");
                model.AddBoolean("overwrite", "Confirm overwrite", true, "Save");
                if(i == Native) {
                    model.AddBoolean("hidden", "Show hidden files", false, "Dialog");
                    model.AddBoolean("aliases", "Follow aliases", true, "Dialog");
                    model.AddBoolean("create", "Create prompt", false, "Save");
                }
            }
            model.StructureChanged();
        }
    }

    void Configure(UiOsFileDialog& dialog) const {
        const String mode = ModeName();
        dialog.SetMode(mode == "OpenFiles" ? UiOsFileDialog::Mode::OpenFiles :
                       mode == "SaveFile" ? UiOsFileDialog::Mode::SaveFile :
                       mode == "PickFolder" ? UiOsFileDialog::Mode::PickFolder : UiOsFileDialog::Mode::OpenFile)
              .SetTitle(Text("title")).SetInitialDirectory(Text("directory"))
              .SetShowHidden((bool)ValueOf("hidden")).SetFollowAliases((bool)ValueOf("aliases"));
        if(mode == "SaveFile")
            dialog.SetSuggestedName(Text("name")).SetDefaultExtension(Text("extension"))
                  .SetConfirmOverwrite((bool)ValueOf("overwrite")).SetCreatePrompt((bool)ValueOf("create"));
        if(mode != "PickFolder")
            dialog.AddFilter(Text("filter_label"), Text("patterns")).AddFilter("All files", "*.*")
                  .SetFilterIndex((int)ValueOf("filter_index"));
    }

    void Launch() {
        Vector<String> paths;
        if(backend_ == Browser) {
            UiFileBrowserDialog dialog;
            UiFileBrowserSelection selection;
            dialog.Title(Text("title"));
            if(dialog.Execute(Text("directory"), DiscoverSequences(), selection)) paths.Add(selection.path);
        }
        else if(backend_ == Native) {
            UiOsFileDialog dialog;
            Configure(dialog);
            if(dialog.Execute(this)) paths = clone(dialog.GetPaths());
        }
        else {
            FileSel dialog;
            const String mode = ModeName();
            dialog.ActiveDir(Text("directory")).Multi(mode == "OpenFiles");
            if(mode != "PickFolder")
                dialog.Type(Text("filter_label"), Text("patterns")).AllFilesType().ActiveType((int)ValueOf("filter_index"));
            if(mode == "SaveFile")
                dialog.Set(Text("name"));
            if(mode == "SaveFile")
                dialog.DefaultExt(Text("extension")).Asking((bool)ValueOf("overwrite"));
            bool accepted = mode == "SaveFile" ? dialog.ExecuteSaveAs(Text("title")) :
                            mode == "PickFolder" ? dialog.ExecuteSelectDir(Text("title")) : dialog.ExecuteOpen(Text("title"));
            if(accepted) for(int i = 0; i < dialog.GetCount(); ++i) paths.Add(dialog.GetFile(i));
        }
        // Cancelling never erases the previous accepted selection.
        if(!paths.IsEmpty()) result_.SetData(Join(paths, "\n"));
    }

    void SelectBackend(Backend backend) {
        backend_ = backend;
        inspector_.SetModel(&Model());
        for(int i = 0; i < 3; ++i) backend_buttons_[i].SetChecked(i == backend_);
        browser_.Show(backend_ == Browser);
        Apply();
    }
    void SelectPage(int page) {
        pages_.SetActivePage(page);
        inspector_mode_.SetChecked(page == 0); code_mode_.SetChecked(page == 1);
    }

    void Apply() {
        for(int i = 0; i < 3; ++i)
            backend_buttons_[i].SetCustomStyle(UiTheme::ResolveButton(i == backend_ ? UiRole::Accent : UiRole::Standard));
        launch_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
        launch_.SetText(backend_ == Browser ? "Open Ui browser dialog" : backend_ == Native ? "Open OS native dialog" : "Open U++ FileSel");
        caption_.SetText(backend_ == Browser ? "Browse below, or open the same control as a modal picker." :
                         backend_ == Native ? "Platform-native Open, Save and Folder dialogs. The OS owns their appearance." :
                         "The original U++ file selector, with Open, Save and Folder modes.");
        if(backend_ == Browser) {
            browser_.SetMode((bool)ValueOf("advanced") ? UiFileBrowser::Mode::Advanced : UiFileBrowser::Mode::Simple);
            browser_.SetSequenceGrouping((bool)ValueOf("grouped"));
            browser_.SetViewSize(Text("size") == "Small" ? UiFileBrowser::ViewSize::Small :
                                 Text("size") == "Large" ? UiFileBrowser::ViewSize::Large : UiFileBrowser::ViewSize::Medium);
            if(scan_browser_ && !Text("directory").IsEmpty() && browser_.GetFolder() != Text("directory"))
                browser_.SetFolder(Text("directory"));
        }
        code_.SetData(Generate());
        Layout();
    }

    void ApplyTheme();

public:
    Demo(bool scan_browser = true) : scan_browser_(scan_browser) {
        Title("UiFileDialog Demo").Sizeable().Zoomable();
        SetRect(0, 0, DPI(1520), DPI(880));
        RegisterPropertyEditorEditors(factory_);
        BuildModels();
        Add(header_); Add(preview_); Add(right_);
        header_.SetTitle("UiFileDialog").SetSubTitle("File and folder selection: Ui browser, OS native and original U++")
               .ShowTitleLine(false).SetContentCell(actions_);
        actions_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
        actions_.AddSpacer(1).Expand(1);
        theme_.Tip("Light / Dark"); help_.SetIcon(ICON_DESIGN_HELP_48()).Tip("Usage help");
        exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).Tip("Close demo");
        for(auto* b : {&theme_, &help_, &exit_}) {
            b->SetIconSize(DPI(16), DPI(16)); actions_.Add(*b).Fixed(DPI(34));
        }
        preview_.Add(selectors_);
        selectors_.SetGap(DPI(6)).SetAlignItems(UiCrossAlign::Center);
        const char* names[] = {"Ui browser", "OS native", "U++ FileSel"};
        for(int i = 0; i < 3; ++i) {
            backend_buttons_[i].SetText(names[i]).SetCheckable();
            backend_buttons_[i].WhenAction = [=] { SelectBackend(Backend(i)); };
            selectors_.Add(backend_buttons_[i]).Fixed(DPI(150));
        }
        selectors_.AddSpacer(1).Expand(1);
        preview_.Add(launch_); preview_.Add(caption_); preview_.Add(browser_); preview_.Add(result_);
        caption_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        result_.SetReadOnly(); result_.SetData("No selection");
        browser_.ShowThemeButton(false);
        browser_.WhenAccept = [=](const UiFileBrowserSelection& selection) { result_.SetData(selection.path); };
        browser_.WhenCancel = [=] { caption_.SetText("Selection cancelled. Previous accepted paths are retained."); };
        browser_.WhenError = [=](const String& error) { caption_.SetText(error); };
        browser_.WhenFolderChanged = [=](const String& folder) {
            models_[Browser].SetValue("directory", folder);
            if(backend_ == Browser) code_.SetData(Generate());
        };
        launch_.WhenAction = [=] { Launch(); };
        right_.Add(tools_); right_.Add(pages_);
        tools_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
        inspector_mode_.SetIcon(ICON_DESIGN_TUNE_48()).SetCheckable().Tip("Inspector");
        code_mode_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetCheckable().Tip("Generated C++");
        for(auto* b : {&inspector_mode_, &code_mode_}) {
            b->SetIconSize(DPI(17), DPI(17)); tools_.Add(*b).Fixed(DPI(38));
        }
        tools_.AddSpacer(1).Expand(1);
        pages_.Add(inspector_page_, "inspector"); pages_.Add(code_page_, "code");
        inspector_page_.Add(inspector_.SizePos());
        code_page_.Add(copy_.RightPos(DPI(8), DPI(32)).TopPos(DPI(6), DPI(30)));
        copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16), DPI(16)).Tip("Copy C++");
        code_page_.Add(code_.HSizePos(DPI(6), DPI(6)).VSizePos(DPI(42), DPI(6))); code_.SetReadOnly();
        inspector_.SetFactory(&factory_);
        auto edit = [=](String, Value) { Apply(); };
        inspector_.WhenPreview = edit; inspector_.WhenCommit = edit; inspector_.WhenCancel = edit;
        inspector_mode_.WhenAction = [=] { SelectPage(0); }; code_mode_.WhenAction = [=] { SelectPage(1); };
        copy_.WhenAction = [=] { WriteClipboardText(Generate()); };
        theme_.WhenAction = [=] {
            auto ctx = UiTheme::GetContext();
            ctx.mode = ctx.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
            UiTheme::Set(ctx); Ctrl::SwapDarkLight(); ApplyTheme();
        };
        help_.WhenAction = [=] { PromptOK("Select Ui browser, OS native or U++ FileSel. The inspector and C++ page follow that selection and retain each backend's settings. The Ui browser can be embedded or opened modally; its toolbar exposes filters, previews and sequences. OS native and U++ support Open, multiple files, Save and Folder modes. Picking a save path does not write a file. Cancel retains the previous result."); };
        exit_.WhenAction = [=] { Close(); };
        SelectBackend(Browser); SelectPage(0); ApplyTheme();
    }

    void ConfigureExport(const String& mode) { if(Model().Find("mode")) Model().SetValue("mode", mode); Apply(); }
    void SetBackend(const String& name) { SelectBackend(name == "native" ? Native : name == "upp" ? UppChooser : Browser); }
    void SetFolder(const String& folder) { Model().SetValue("directory", folder); Apply(); }
    void SetTitleText(const String& title) { Model().SetValue("title", title); Apply(); }
    void Paint(Draw& w) override { w.DrawRect(GetSize(), window_face_); }
    void Layout() override {
        const Size s = GetSize();
        const int pad = DPI(12), gap = DPI(10), top = DPI(94);
        const int rail = min(DPI(420), max(DPI(300), s.cx * 26 / 100));
        const int pw = max(0, s.cx - pad * 2 - gap - rail), h = max(0, s.cy - top - pad);
        header_.SetRect(pad, pad, max(0, s.cx - pad * 2), DPI(72));
        preview_.SetRect(pad, top, pw, h); right_.SetRect(pad + pw + gap, top, rail, h);
        tools_.SetRect(DPI(6), DPI(4), max(0, rail - DPI(12)), DPI(38));
        pages_.SetRect(DPI(6), DPI(48), max(0, rail - DPI(12)), max(0, h - DPI(54)));
        selectors_.SetRect(DPI(12), DPI(12), max(0, pw - DPI(24)), DPI(36));
        launch_.SetRect(DPI(16), DPI(60), min(DPI(220), max(0, pw - DPI(32))), DPI(36));
        caption_.SetRect(DPI(252), DPI(54), max(0, pw - DPI(268)), DPI(48));
        browser_.SetRect(DPI(12), DPI(110), max(0, pw - DPI(24)), max(0, h - DPI(192)));
        result_.SetRect(DPI(16), max(DPI(110), h - DPI(70)), max(0, pw - DPI(32)), DPI(54));
    }
    String Generate() const;
    bool CheckShell();
};
} // namespace

// Styling is deliberately local to this demo; no other example is a dependency.

String Demo::Generate() const
{
    const String title = AsCString(Text("title")), directory = AsCString(Text("directory"));
    if(backend_ == Browser) {
        String out = "#include <Ui/UiFileBrowser/UiFileBrowser.h>\n\nusing namespace Upp;\n\n";
        out << "// Embed this control in a host that outlives it; handle WhenAccept/WhenCancel.\n"
            << "void ConfigureBrowser(UiFileBrowser& browser)\n{\n"
            << "    browser.SetMode(UiFileBrowser::Mode::" << ((bool)ValueOf("advanced") ? "Advanced" : "Simple") << ")\n"
            << "           .SetViewSize(UiFileBrowser::ViewSize::" << Text("size") << ")\n"
            << "           .SetSequenceGrouping(" << ((bool)ValueOf("grouped") ? "true" : "false") << ")\n"
            << "           .SetFolder(" << directory << ");\n}\n\n"
            << "// The modal wrapper exposes Open/Cancel and sequence discovery.\n"
            << "// Use the embedded control above for full view-size/mode configuration.\n"
            << "bool ChooseFile(UiFileBrowserSelection& selection)\n{\n"
            << "    UiFileBrowserDialog dialog;\n    dialog.Title(" << title << ");\n"
            << "    return dialog.Execute(" << directory << ", " << (DiscoverSequences() ? "true" : "false")
            << ", selection);\n}\n";
        return out;
    }
    const String mode = ModeName();
    if(backend_ == Native) {
        String out = "#include <Ui/UiOsFileDialog/UiOsFileDialog.h>\n\nusing namespace Upp;\n\nVector<String> ChoosePaths(Ctrl* owner)\n{\n    UiOsFileDialog dialog;\n";
        out << "    dialog.SetMode(UiOsFileDialog::Mode::" << mode << ")\n          .SetTitle(" << title << ")";
        if(mode == "SaveFile")
            out << "\n          .SetSuggestedName(" << AsCString(Text("name")) << ")\n          .SetDefaultExtension(" << AsCString(Text("extension")) << ")";
        if(!Text("directory").IsEmpty()) out << "\n          .SetInitialDirectory(" << directory << ")";
        if((bool)ValueOf("hidden")) out << "\n          .SetShowHidden()";
        if(!(bool)ValueOf("aliases")) out << "\n          .SetFollowAliases(false)";
        if(mode == "SaveFile" && !(bool)ValueOf("overwrite")) out << "\n          .SetConfirmOverwrite(false)";
        if(mode == "SaveFile" && (bool)ValueOf("create")) out << "\n          .SetCreatePrompt()";
        if(mode != "PickFolder")
            out << "\n          .AddFilter(" << AsCString(Text("filter_label")) << ", " << AsCString(Text("patterns"))
                << ")\n          .AddFilter(\"All files\", \"*.*\")\n          .SetFilterIndex(" << (int)ValueOf("filter_index") << ")";
        return out << ";\n    if(dialog.Execute(owner)) return clone(dialog.GetPaths());\n    return Vector<String>();\n}\n";
    }
    String out = "#include <CtrlLib/CtrlLib.h>\n\nusing namespace Upp;\n\nVector<String> ChoosePaths()\n{\n    FileSel dialog;\n";
    out << "    dialog.ActiveDir(" << directory << ").Multi(" << (mode == "OpenFiles" ? "true" : "false") << ");\n";
    if(mode != "PickFolder")
        out << "    dialog.Type(" << AsCString(Text("filter_label")) << ", " << AsCString(Text("patterns"))
            << ").AllFilesType().ActiveType(" << (int)ValueOf("filter_index") << ");\n";
    if(mode == "SaveFile")
        out << "    dialog.Set(" << AsCString(Text("name")) << ");\n    dialog.DefaultExt(" << AsCString(Text("extension"))
            << ").Asking(" << ((bool)ValueOf("overwrite") ? "true" : "false") << ");\n";
    out << "    Vector<String> paths;\n    if(dialog." << (mode == "SaveFile" ? "ExecuteSaveAs" : mode == "PickFolder" ? "ExecuteSelectDir" : "ExecuteOpen")
        << "(" << title << "))\n        for(int i = 0; i < dialog.GetCount(); ++i) paths.Add(dialog.GetFile(i));\n    return paths;\n}\n";
    return out;
}

bool Demo::CheckShell()
{
    int checks = 0, failed = 0;
    auto check = [&](bool value, const char* label) { ++checks; if(!value) { ++failed; Cout() << "FAIL: " << label << '\n'; } };
    const UiThemeContext saved = UiTheme::GetContext();
    for(UiThemeMode mode : {UiThemeMode::Light, UiThemeMode::Dark}) {
        auto ctx = saved; ctx.mode = mode; UiTheme::Set(ctx);
        if((SColorPaper().GetR() < 128) != (mode == UiThemeMode::Dark)) Ctrl::SwapDarkLight();
        ApplyTheme();
        for(int i = 0; i < 3; ++i) {
            SelectBackend(Backend(i));
            check(browser_.IsShown() == (i == Browser), "one selected backend preview");
            check((Model().Find("advanced") != nullptr) == (i == Browser), "browser properties belong to browser");
            check((Model().Find("hidden") != nullptr) == (i == Native), "OS properties belong to OS dialog");
            Model().SetValue("title", "A \"quoted\" title\nsecond line");
            check(Generate().Find(AsCString(Text("title"))) >= 0, "generated title is escaped");
            if(i != Browser) for(const char* m : {"Open file", "Open files", "Save file", "Folder"}) {
                ConfigureExport(m);
                check(Generate().Find(i == Native ? "UiOsFileDialog dialog" : "FileSel dialog") >= 0,
                      "generated code selects the concrete backend");
            }
            SelectPage(1); check(code_mode_.IsChecked(), "code page selection");
            SelectPage(0); check(inspector_mode_.IsChecked(), "inspector page selection");
        }
    }
    UiTheme::Set(saved);
    if((SColorPaper().GetR() < 128) != (saved.mode == UiThemeMode::Dark)) Ctrl::SwapDarkLight();
    ApplyTheme();
    Cout() << "UI_FILE_DIALOG_DEMO_SUMMARY checks=" << checks << " failed=" << failed << '\n';
    return failed == 0;
}

GUI_APP_MAIN
{
    SetStdFont(UiFileBrowserAppearance::ModernFont());
    const auto& args = CommandLine();
    if(args.GetCount() && args[0] == "--self-test") {
        String report = args.GetCount() >= 2 ? args[1] : AppendFileName(GetCurrentDirectory(), "build/file-dialog-model-test.txt");
        SetExitCode(RunBrowserModelChecks(report) ? 0 : 1); return;
    }
    if(args.GetCount() >= 2 && args[0] == "--ui-test") {
        SetExitCode(RunBrowserUiTests(BuildBrowserFixture(), args[1]) ? 0 : 1); return;
    }
    bool export_code = false, shell_test = false;
    String output, render;
    bool dark = false;
    for(const String& arg : args) {
        export_code |= arg.StartsWith("--export-code=");
        shell_test |= arg == "--shell-test";
        dark |= arg == "--dark" || arg == "--render-dark";
    }
    if((SColorPaper().GetR() < 128) != dark) Ctrl::SwapDarkLight();
    UiTheme::Set(dark ? UiThemeMode::Dark : UiThemeMode::Light);
    Demo demo(!export_code && !shell_test);
    for(const String& arg : args) if(arg.StartsWith("--backend=")) demo.SetBackend(arg.Mid(10));
    for(const String& arg : args) {
        if(arg.StartsWith("--mode=")) demo.ConfigureExport(arg.Mid(7));
        if(arg.StartsWith("--directory=")) demo.SetFolder(arg.Mid(12));
        if(arg.StartsWith("--title=")) demo.SetTitleText(arg.Mid(8));
        if(arg == "--fixture") demo.SetFolder(BuildBrowserFixture());
        if(arg.StartsWith("--export-code=")) output = arg.Mid(14);
        if(!arg.StartsWith("--") && DirectoryExists(arg)) demo.SetFolder(arg);
    }
    if(export_code) { SetExitCode(SaveFile(output, demo.Generate()) ? 0 : 1); return; }
    if(shell_test) { SetExitCode(demo.CheckShell() ? 0 : 1); return; }
    for(int i = 0; i + 1 < args.GetCount(); ++i)
        if(args[i] == "--render-dark" || args[i] == "--render-light") render = args[i+1];
    if(!render.IsEmpty()) {
        demo.SetFolder(BuildBrowserFixture());
        demo.SetTimeCallback(700, [&] {
            Ctrl::ProcessEvents();
            ImageDraw image(demo.GetSize()); demo.DrawCtrl(image);
            SetExitCode(PNGEncoder().SaveFile(render, image) ? 0 : 1); demo.Close();
        });
    }
    else if(FindIndex(args, String("--smoke")) >= 0) demo.SetTimeCallback(700, [&] { demo.Close(); });
    demo.Run();
}

void Demo::ApplyTheme()
{
    const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    window_face_ = UiTheme::ResolvePanel(UiPanelRole::Surface).palette.face[ST_NORMAL].color;
    header_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
    UiPanel::Style surface = UiTheme::ResolvePanel(UiPanelRole::Surface);
    const Color panel_face = dark ? Color(18, 18, 18) : Color(245, 245, 245);
    surface.transparent = false;
    surface.metrics.face_enabled = true;
    surface.metrics.frame_enabled = true;
    surface.metrics.frame_width = DPI(1);
    surface.metrics.radius = DPI(8);
    surface.metrics.shadow.enabled = false;
    surface.metrics.focus_enabled = false;
    for(int state = 0; state < 4; state++) {
        surface.palette.face[state] = UiFill::Solid(panel_face);
        surface.palette.frame[state] = dark ? Color(48, 48, 48) : Color(220, 220, 220);
    }
    preview_.SetCustomStyle(surface);
    right_.SetCustomStyle(surface);
    UiPanel::Style page_style = surface;
    page_style.transparent = true;
    page_style.metrics.face_enabled = page_style.metrics.frame_enabled = false;
    for(UiPanel* panel : { &inspector_page_, &code_page_ })
        panel->SetCustomStyle(page_style);
    for(UiLabel* label : { &caption_ })
        label->SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
    const auto mode = dark
                    ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
    inspector_.SetPaletteMode(mode);
    for(PropertyEditor* editor : { &inspector_ }) {
        PropertyEditorStyle editor_style = editor->GetStyle();
        editor_style.show_frame = false;
        editor_style.background = panel_face;
        editor_style.show_group_summaries = true;
        editor->SetStyle(editor_style);
    }
    for(UiToolButton* button : { &theme_, &help_, &exit_, &inspector_mode_, &code_mode_, &copy_ }) {
        UiToolButton::Style style = UiTheme::ResolveToolButton(UiRole::Standard);
        style.transparent = true;
        style.metrics.face_enabled = style.metrics.frame_enabled = false;
        style.metrics.focus_enabled = false;
        style.metrics.shadow.enabled = false;
        style.underline = false;
        for(int state = 0; state < 4; state++) {
            style.palette.face[state] = UiFill::None();
            style.palette.frame[state] = Null;
        }
        const Color neutral = dark ? Color(180, 180, 180) : Color(110, 110, 110);
        style.palette.icon[ST_NORMAL] = neutral;
        style.palette.icon[ST_HOT] = dark ? White() : Color(32, 32, 32);
        style.palette.icon[ST_PRESSED] = Color(0, 120, 212);
        style.palette.icon[ST_DISABLED] = Blend(neutral, panel_face, 150);
        button->SetCustomStyle(style);
    }
    UiToolButton::Style exit_style = exit_.GetStyle();
    exit_style.palette.icon[ST_NORMAL] = Color(200, 60, 60);
    exit_style.palette.icon[ST_HOT] = Color(240, 85, 85);
    exit_style.palette.icon[ST_PRESSED] = Color(180, 45, 45);
    exit_.SetCustomStyle(exit_style);
    theme_.SetIcon(dark ? ICON_ACTION_LIGHT_MODE_48() : ICON_ACTION_DARK_MODE_48());
    code_.SetCustomStyle(UiTheme::ResolveEdit(UiRole::Standard));
    result_.SetCustomStyle(UiTheme::ResolveEdit(UiRole::Subtle));
    browser_.ApplyTheme();
    Apply();
    Refresh();
}
