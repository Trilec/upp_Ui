#include "UiDesignerWindow.h"
#include <Ui/UiIcons.h>

namespace Upp {

static void Put(Ctrl& c, int x, int y, int cx, int cy)
{
    c.SetRect(x, y, max(0, cx), max(0, cy));
}

UiDesignerWindow::UiDesignerWindow()
{
    Title("Ui Designer").Sizeable().Zoomable();
    SetRect(0, 0, DPI(1374), DPI(858));

    BuildHeader();
    BuildDesigner();
    BuildTheme();

    workspaces_.Add(designer_page_, "designer");
    workspaces_.Add(theme_page_, "theme");
    workspaces_.SetActiveKey("designer");
    Add(workspaces_);

    footer_surface_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Subtle));
    footer_.SetText("Ready").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    footer_surface_.Add(footer_.SizePos());
    Add(footer_surface_);

    ConnectServices();
    session_.AttachProjection(&preview_canvas_);
    ApplyThemeToShell();
    RefreshHierarchy();
    RefreshInspector();
    RefreshThemeInspector();
    RefreshCode();
}

void UiDesignerWindow::BuildHeader()
{
    header_surface_.SetCustomStyle(UiDesignerSurfaceStyle());
    Add(header_surface_);

    brand_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
    brand_.SetTitle("Designer").ShowTitleLine(false).ShowCardLine(false);
    brand_.SetMedia(ICON_BRAND_NEWLOGO_V5_48(),
                    Size(DPI(18), DPI(18)));

    save_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    save_.SetText("Save").SetSplitWidth(DPI(31));
    save_.Add("Save", "save").Add("Save As", "save_as");
    save_.WhenAction = [=] { SaveDocument(false); };
    save_.WhenSelect = [=](int, const Value& value) {
        SaveDocument((String)value == "save_as");
    };

    load_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    load_.SetText("Load").SetSplitWidth(DPI(30));
    load_.Add("Open", "open").Add("New blank", "blank")
         .Add("New three pane", "three_pane")
         .Add("New settings", "settings");
    load_.WhenAction = [=] { LoadDocument(); };
    load_.WhenSelect = [=](int, const Value& value) {
        const String action = value;
        if(action == "open")
            LoadDocument();
        else if(action == "blank")
            session_.NewDocument("blank");
        else if(action == "settings")
            session_.NewDocument("settings");
        else
            session_.NewDocument("three_pane");
    };

    export_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    export_.SetText("Export").SetSplitWidth(DPI(31));
    export_.Add("C++ project", "cpp").Add("JSON", "json");
    export_.WhenAction = [=] { ExportProject(); };
    export_.WhenSelect = [=](int, const Value&) { ExportProject(); };

    version_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Accent));
    version_.SetText("v1.0.0-rc1")
            .SetIcon(ICON_DESIGN_ADJUST_48(),
                     UiIconRenderMode::MonoTint)
            .SetIconSize(DPI(10), DPI(10));

    designer_mode_.SetText("Designer");
    theme_mode_.SetText("Theme Studio");
    designer_mode_.WhenAction = [=] { ShowDesigner(); };
    theme_mode_.WhenAction = [=] { ShowTheme(); };

    theme_select_.UseInternalModel().Clear()
                 .Add("Minimal", "Minimal")
                 .Add("Pill", "Pill")
                 .Add("Linear", "Linear")
                 .Add("Solid", "Solid")
                 .Add("Outline", "Outline")
                 .Add("Compact", "Compact")
                 .Add("Layered", "Layered");
    theme_select_.Select(0);
    theme_select_.WhenAction = [=] {
        String error;
        session_.Theme().Commit("preset", theme_select_.GetData(),
                                "Select theme preset", error);
        ApplyThemeToShell();
        RefreshThemeInspector();
    };

    dark_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
    dark_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16));
    dark_.WhenAction = [=] { ToggleDarkMode(); };

    help_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
    help_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16));
    help_.WhenAction = [=] {
        PromptOK("UiDesigner greenfield architecture\n"
                 "Core, commands, catalog, preview, theme, code generation "
                 "and MCP share one property pipeline.");
    };

    header_surface_.Add(brand_);
    header_surface_.Add(save_);
    header_surface_.Add(load_);
    header_surface_.Add(export_);
    header_surface_.Add(version_);
    header_surface_.Add(designer_mode_);
    header_surface_.Add(theme_mode_);
    header_surface_.Add(theme_select_);
    header_surface_.Add(dark_);
    header_surface_.Add(help_);
}

void UiDesignerWindow::BuildDesigner()
{
    designer_page_.Add(designer_left_);
    designer_page_.Add(designer_center_);
    designer_page_.Add(designer_right_);

    const UiDesignerCatalog& catalog = session_.Catalog();
    presets_list_.SetCatalog(&catalog);
    presets_list_.SetPresets();
    layouts_list_.SetCatalog(&catalog);
    layouts_list_.SetCategory("Layouts");
    containers_list_.SetCatalog(&catalog);
    containers_list_.SetCategory("Containers");
    controls_list_.SetCatalog(&catalog);
    controls_list_.SetCategory("Ui Controls");
    composites_list_.SetCatalog(&catalog);
    composites_list_.SetCategory("Composites");
    upp_controls_list_.SetCatalog(&catalog);
    upp_controls_list_.SetCategory("U++ Controls");

    designer_left_.AddSection("Layouts",
                              ICON_DESIGN_LAYOUTS_CATEGORY_48(), layouts_list_)
                  .AddSection("Containers",
                              ICON_DESIGN_TAB_GROUP_48(), containers_list_)
                  .AddSection("Controls",
                              ICON_DESIGN_WIDGETS_48(), controls_list_)
                  .AddSection("Composites",
                              ICON_DESIGN_DYNAMIC_FORM_48(), composites_list_)
                  .AddSection("Presets",
                              ICON_DESIGN_DASHBOARD_EDIT_48(), presets_list_)
                  .AddSection("U++ Controls",
                              ICON_DESIGN_WIDGETS_48(), upp_controls_list_);

    designer_right_.RightColumn()
                   .AddSection("Hierarchy",
                               ICON_DESIGN_ACCOUNT_TREE_48(), hierarchy_)
                   .AddSection("Inspector",
                               ICON_DESIGN_TUNE_48(), inspector_)
                   .AddSection("Theme Overrides",
                               ICON_DESIGN_FORMAT_PAINT_48(), overrides_)
                   .AddSection("Code",
                               ICON_DESIGN_CODE_BLOCKS_48(), code_);
    designer_right_.SetActiveSection(1);

    designer_center_.SetCustomStyle(UiDesignerSurfaceStyle());

    aspect_pill_.SetInset(UiDesignerStyleMetrics::RightPillInset());
    portrait_.SetIcon(ICON_DESIGN_SPLITSCREEN_PORTRAIT_48())
             .SetIconSize(DPI(20), DPI(20)).Tip("Portrait");
    landscape_.SetIcon(ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48())
              .SetIconSize(DPI(20), DPI(20)).Tip("Landscape");
    aspect_preset_.SetText("2:1").SetSplitWidth(DPI(30));
    aspect_preset_.Add("Portrait 1:2", "1:2")
                  .Add("Landscape 2:1", "2:1")
                  .Add("Square 1:1", "1:1");
    square_.SetIcon(ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48())
           .SetIconSize(DPI(17), DPI(17)).Tip("Square");

    portrait_.WhenAction = [=] {
        session_.SetVirtualSize(Size(668, 1020));
    };
    landscape_.WhenAction = [=] {
        session_.SetVirtualSize(Size(1020, 668));
    };
    square_.WhenAction = [=] {
        session_.SetVirtualSize(Size(800, 800));
    };
    aspect_preset_.WhenSelect = [=](int, const Value& value) {
        const String ratio = value;
        if(ratio == "1:2")
            session_.SetVirtualSize(Size(500, 1000));
        else if(ratio == "1:1")
            session_.SetVirtualSize(Size(800, 800));
        else
            session_.SetVirtualSize(Size(1000, 500));
    };

    aspect_pill_.AddControl(portrait_, DPI(32))
                .AddControl(landscape_, DPI(32))
                .AddControl(aspect_preset_, DPI(92))
                .AddControl(square_, DPI(32));

    preview_surface_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Standard));
    preview_surface_.Add(preview_canvas_.SizePos());
    preview_scroll_.SetCustomStyle(UiTheme::ResolveScrollPanel(UiRole::Subtle));
    preview_scroll_.SetInset(DPI(0));
    preview_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
    preview_scroll_.Add(preview_surface_.SizePos());

    designer_center_.Add(aspect_pill_);
    designer_center_.Add(preview_scroll_);

    auto wire_list = [=](UiDesignerCatalogList& list) {
        list.WhenActivate = [=](const String& id) { ActivateToolbox(id); };
    };
    wire_list(presets_list_);
    wire_list(layouts_list_);
    wire_list(containers_list_);
    wire_list(controls_list_);
    wire_list(composites_list_);
    wire_list(upp_controls_list_);
}

void UiDesignerWindow::BuildTheme()
{
    theme_page_.Add(theme_gallery_column_);
    theme_page_.Add(theme_right_);

    theme_gallery_column_.SetCustomStyle(UiDesignerSurfaceStyle());
    theme_gallery_pill_.SetInset(UiDesignerStyleMetrics::LeftPillInset());

    theme_all_.SetIcon(ICON_DESIGN_WIDGETS_48())
              .SetIconSize(DPI(16), DPI(16)).Tip("All controls");
    theme_inputs_.SetIcon(ICON_DESIGN_DYNAMIC_FORM_48())
                 .SetIconSize(DPI(16), DPI(16)).Tip("Inputs");
    theme_containers_.SetIcon(ICON_DESIGN_TAB_GROUP_48())
                     .SetIconSize(DPI(16), DPI(16)).Tip("Containers");

    theme_all_.WhenAction = [=] { theme_gallery_.SetFilter("all"); };
    theme_inputs_.WhenAction = [=] { theme_gallery_.SetFilter("inputs"); };
    theme_containers_.WhenAction = [=] {
        theme_gallery_.SetFilter("containers");
    };

    theme_gallery_pill_.AddControl(theme_all_, DPI(32))
                       .AddControl(theme_inputs_, DPI(32))
                       .AddControl(theme_containers_, DPI(32));

    gallery_surface_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Standard));
    gallery_surface_.Add(theme_gallery_);
    gallery_scroll_.SetCustomStyle(
        UiTheme::ResolveScrollPanel(UiRole::Subtle));
    gallery_scroll_.SetInset(DPI(0));
    gallery_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
    gallery_scroll_.Add(gallery_surface_.SizePos());

    theme_gallery_column_.Add(theme_gallery_pill_);
    theme_gallery_column_.Add(gallery_scroll_);

    theme_right_.RightColumn()
                .AddSection("Inspector",
                            ICON_DESIGN_TUNE_48(), theme_inspector_)
                .AddSection("Code",
                            ICON_DESIGN_CODE_BLOCKS_48(), theme_code_);

    theme_gallery_.SetCatalog(&session_.Catalog());
    theme_gallery_.SetThemeDocument(&session_.Theme());
}

void UiDesignerWindow::ConnectServices()
{
    hierarchy_.SetDocument(&session_.Document());
    hierarchy_.SetSelection(&session_.State().selection);
    hierarchy_.WhenSelectNode = [=](UiDesignerNodeId id, bool toggle) {
        session_.Select(id, toggle);
    };

    inspector_.SetModel(&session_.InspectorModel());
    inspector_.WhenPreview = [=](const String& id, const Value& value) {
        String error;
        if(!session_.PreviewProperty(id, value, error))
            RefreshStatus(error);
    };
    inspector_.WhenCommit = [=](const String& id, const Value& value) {
        String error;
        if(!session_.CommitProperty(id, value, error))
            RefreshStatus(error);
    };
    inspector_.WhenReset = [=](const String& id) {
        String error;
        if(!session_.ResetProperty(id, error))
            RefreshStatus(error);
    };

    theme_inspector_.SetModel(&theme_model_);
    theme_inspector_.WhenPreview = [=](const String& id, const Value& value) {
        String error;
        if(!session_.Theme().Preview(id, value, error))
            RefreshStatus(error);
        ApplyThemeToShell();
        theme_gallery_.SetThemeDocument(&session_.Theme());
    };
    theme_inspector_.WhenCommit = [=](const String& id, const Value& value) {
        String error;
        if(!session_.Theme().Commit(id, value, "Set theme " + id, error))
            RefreshStatus(error);
        ApplyThemeToShell();
        RefreshThemeInspector();
        theme_gallery_.SetThemeDocument(&session_.Theme());
    };
    theme_inspector_.WhenReset = [=](const String& id) {
        String error;
        if(!session_.Theme().Reset(id, error))
            RefreshStatus(error);
        ApplyThemeToShell();
        RefreshThemeInspector();
        theme_gallery_.SetThemeDocument(&session_.Theme());
    };

    session_.WhenSelectionChanged = [=] {
        RefreshHierarchy();
        RefreshInspector();
    };
    session_.WhenInspectorChanged = [=] {
        RefreshInspector();
        RefreshThemeInspector();
    };
    session_.WhenCodeChanged = [=] { RefreshCode(); };
    session_.WhenStatus = [=](const String& text) { RefreshStatus(text); };

    designer_left_.WhenWidthChanged = [=] { Layout(); };
    designer_right_.WhenWidthChanged = [=] { Layout(); };
    theme_right_.WhenWidthChanged = [=] { Layout(); };
}

void UiDesignerWindow::ShowDesigner()
{
    session_.Theme().CancelPreview();
    workspaces_.SetActiveKey("designer");
    session_.State().active_workspace = "designer";
    brand_.SetTitle("Designer");
    Layout();
}

void UiDesignerWindow::ShowTheme()
{
    session_.CancelPreview();
    workspaces_.SetActiveKey("theme");
    session_.State().active_workspace = "theme";
    brand_.SetTitle("Theme Studio");
    RefreshThemeInspector();
    Layout();
}

void UiDesignerWindow::RefreshHierarchy()
{
    hierarchy_.SetSelection(&session_.State().selection);
    hierarchy_.Rebuild();
}

void UiDesignerWindow::RefreshInspector()
{
    inspector_.SetModel(&session_.InspectorModel());
    inspector_.RefreshModel();

    overrides_model_.Clear();
    if(session_.State().selection.primary) {
        overrides_model_.AddReadOnly(
            "theme.note", "Theme overrides",
            "Control-specific overrides use the same property descriptors.",
            "Theme");
    }
    overrides_.SetModel(&overrides_model_);
    overrides_.RefreshModel();
}

void UiDesignerWindow::RefreshThemeInspector()
{
    session_.Theme().BuildPropertyModel(theme_model_);
    theme_inspector_.SetModel(&theme_model_);
    theme_inspector_.RefreshModel();
    theme_gallery_.SetThemeDocument(&session_.Theme());
    theme_code_.SetCode(session_.Theme().Serialize(true));
    ApplyThemeToShell();
}

void UiDesignerWindow::RefreshCode()
{
    code_.SetCode(session_.GenerateCode("GeneratedUiWindow"));
    theme_code_.SetCode(session_.Theme().Serialize(true));
}

void UiDesignerWindow::RefreshStatus(const String& text)
{
    String status = text;
    if(status.IsEmpty())
        status = (session_.Commands().IsDirty() || session_.Theme().IsDirty())
                     ? "Modified" : "Ready";
    {
        const UiDesignerPreviewStats& stats = preview_canvas_.GetStats();
        status << "  |  live " << stats.live_applies
               << "  layout " << stats.local_layouts + stats.ancestor_layouts
               << "  subtree " << stats.subtree_rebuilds
               << "  full " << stats.full_rebuilds;
    }
    footer_.SetText(status);
}

void UiDesignerWindow::ActivateToolbox(const String& id)
{
    if(id.StartsWith("preset:")) {
        session_.NewDocument(id.Mid(7));
        return;
    }
    if(!session_.AddControl(id))
        RefreshStatus("Unable to add " + id);
}

void UiDesignerWindow::SaveDocument(bool save_as)
{
    String path = current_path_;
    if(save_as || path.IsEmpty()) {
        FileSel file;
        file.Type("UiDesigner document", "*.uidesign.json");
        if(!file.ExecuteSaveAs("Save UiDesigner document"))
            return;
        path = ~file;
    }
    String error;
    if(session_.Save(path, error))
        current_path_ = path;
    else
        Exclamation(error);
}

void UiDesignerWindow::LoadDocument()
{
    FileSel file;
    file.Type("UiDesigner document", "*.uidesign.json");
    if(!file.ExecuteOpen("Open UiDesigner document"))
        return;
    String error;
    if(session_.Load(~file, error))
        current_path_ = ~file;
    else
        Exclamation(error);
}

void UiDesignerWindow::ExportProject()
{
    FileSel folder;
    if(!folder.ExecuteSelectDir("Export generated project"))
        return;
    String error;
    if(!session_.Export(~folder, "GeneratedUiWindow", error))
        Exclamation(error);
}

void UiDesignerWindow::ToggleDarkMode()
{
    String error;
    const String next =
        session_.Theme().Get().mode == "Dark" ? "Light" : "Dark";
    session_.Theme().Commit("mode", next, "Toggle dark mode", error);
    RefreshThemeInspector();
    theme_gallery_.Refresh();
    RefreshStatus("Theme mode: " + next);
}

void UiDesignerWindow::ApplyThemeToShell()
{
    const UiDesignerThemeSnapshot theme = session_.Theme().GetEffective();
    UiDesignerApplyGlobalTheme(theme);

    header_surface_.SetCustomStyle(
        UiDesignerSurfaceStyle(UiRole::Subtle, theme));
    footer_surface_.SetCustomStyle(
        UiDesignerSurfaceStyle(UiRole::Subtle, theme));
    designer_center_.SetCustomStyle(
        UiDesignerSurfaceStyle(UiRole::Subtle, theme));
    preview_surface_.SetCustomStyle(
        UiDesignerSurfaceStyle(UiRole::Standard, theme));
    theme_gallery_column_.SetCustomStyle(
        UiDesignerSurfaceStyle(UiRole::Subtle, theme));
    gallery_surface_.SetCustomStyle(
        UiDesignerSurfaceStyle(UiRole::Standard, theme));

    aspect_pill_.ApplyTheme(theme);
    theme_gallery_pill_.ApplyTheme(theme);
    designer_left_.ApplyTheme(theme);
    designer_right_.ApplyTheme(theme);
    theme_right_.ApplyTheme(theme);

    preview_scroll_.SetCustomStyle(
        UiTheme::ResolveScrollPanel(UiRole::Subtle));
    gallery_scroll_.SetCustomStyle(
        UiTheme::ResolveScrollPanel(UiRole::Subtle));
    preview_canvas_.SetAccent(theme.accent);

    brand_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
    save_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    load_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    export_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    version_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Accent));
    dark_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
    help_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));

    theme_gallery_.SetThemeDocument(&session_.Theme());
    RefreshLayout();
    Refresh();
}

void UiDesignerWindow::Close()
{
    session_.CancelPreview();
    session_.Theme().CancelPreview();
    if((session_.Commands().IsDirty() || session_.Theme().IsDirty()) &&
       !PromptYesNo("Discard unsaved UiDesigner changes?"))
        return;
    TopWindow::Close();
}

bool UiDesignerWindow::Key(dword key, int count)
{
    if(key == K_CTRL_Z) {
        if(workspaces_.GetActiveKey() == "theme")
            session_.Theme().Undo();
        else
            session_.Undo();
        RefreshThemeInspector();
        ApplyThemeToShell();
        return true;
    }
    if(key == K_CTRL_Y) {
        if(workspaces_.GetActiveKey() == "theme")
            session_.Theme().Redo();
        else
            session_.Redo();
        RefreshThemeInspector();
        ApplyThemeToShell();
        return true;
    }
    if(key == K_DELETE) {
        session_.RemoveSelection();
        return true;
    }
    if(key == K_CTRL_S) {
        SaveDocument(false);
        return true;
    }
    return TopWindow::Key(key, count);
}

void UiDesignerWindow::Layout()
{
    const Size sz = GetSize();
    const int gap = UiDesignerStyleMetrics::Gap();
    const int header_h = UiDesignerStyleMetrics::HeaderHeight();
    const int footer_h = UiDesignerStyleMetrics::FooterHeight();

    Put(header_surface_, 0, 0, sz.cx, header_h);
    int x = UiDesignerStyleMetrics::HeaderInset();
    const int y = UiDesignerStyleMetrics::HeaderInset();
    const int h = header_h - y * 2;

    Put(brand_, x, y, DPI(145), h); x += DPI(153);
    Put(save_, x, y, DPI(74), h); x += DPI(82);
    Put(load_, x, y, DPI(74), h); x += DPI(82);
    Put(export_, x, y, DPI(74), h); x += DPI(82);
    Put(version_, x, y, DPI(90), h); x += DPI(98);
    Put(designer_mode_, x, y, DPI(78), h); x += DPI(86);
    Put(theme_mode_, x, y, DPI(98), h);

    Put(help_, sz.cx - y - DPI(34), y, DPI(34), h);
    Put(dark_, sz.cx - y - DPI(76), y, DPI(34), h);
    Put(theme_select_, sz.cx - y - DPI(186), y, DPI(102), h);

    const int body_h = max(0, sz.cy - header_h - footer_h);
    Put(workspaces_, 0, header_h, sz.cx, body_h);
    Put(footer_surface_, 0, sz.cy - footer_h, sz.cx, footer_h);

    const bool theme = workspaces_.GetActiveKey() == "theme";
    if(!theme) {
        const int left_w = min(designer_left_.GetDesiredWidth(),
                               max(DPI(56), sz.cx / 3));
        const int right_w = min(designer_right_.GetDesiredWidth(),
                                max(DPI(56), sz.cx / 3));

        Put(designer_page_, 0, 0, sz.cx, body_h);
        Put(designer_left_, 0, 0, left_w, body_h);
        Put(designer_right_, max(0, sz.cx - right_w), 0,
            right_w, body_h);
        Put(designer_center_, left_w + gap, 0,
            max(0, sz.cx - left_w - right_w - gap * 2), body_h);

        const int pill_h =
            UiDesignerStyleMetrics::DesignerToolbarHeight();
        Put(aspect_pill_, 0, 0,
            designer_center_.GetSize().cx, pill_h);
        Put(preview_scroll_, 0, pill_h + gap,
            designer_center_.GetSize().cx,
            max(0, body_h - pill_h - gap));

        preview_surface_.SetRect(
            0, 0,
            max(preview_scroll_.GetSize().cx,
                session_.Document().GetVirtualSize().cx + DPI(40)),
            max(preview_scroll_.GetSize().cy,
                session_.Document().GetVirtualSize().cy + DPI(40)));
        preview_canvas_.SetRect(
            DPI(20), DPI(20),
            session_.Document().GetVirtualSize().cx,
            session_.Document().GetVirtualSize().cy);
    }
    else {
        const int right_w = min(theme_right_.GetDesiredWidth(),
                                max(DPI(56), sz.cx / 3));

        Put(theme_page_, 0, 0, sz.cx, body_h);
        Put(theme_right_, max(0, sz.cx - right_w), 0,
            right_w, body_h);
        Put(theme_gallery_column_, 0, 0,
            max(0, sz.cx - right_w - gap), body_h);

        const int pill_h =
            UiDesignerStyleMetrics::DesignerToolbarHeight();
        Put(theme_gallery_pill_, 0, 0,
            theme_gallery_column_.GetSize().cx, pill_h);
        Put(gallery_scroll_, 0, pill_h + gap,
            theme_gallery_column_.GetSize().cx,
            max(0, body_h - pill_h - gap));

        const int gallery_w =
            max(DPI(820), gallery_scroll_.GetSize().cx);
        theme_gallery_.SetRect(0, 0, gallery_w,
                               max(DPI(1200),
                                   theme_gallery_.GetContentHeight()));
        gallery_surface_.SetRect(
            0, 0, gallery_w,
            max(gallery_scroll_.GetSize().cy,
                theme_gallery_.GetContentHeight()));
    }
}

}
