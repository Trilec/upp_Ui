#include "UiDesignerWindow.h"
#include "UiDesignerVersion.h"
#include <Ui/UiIcons.h>

#ifdef PLATFORM_WIN32
#include <windows.h>
#endif

namespace Upp {

static void Put(Ctrl& c, int x, int y, int cx, int cy)
{
    c.SetRect(x, y, max(0, cx), max(0, cy));
}

static UiPanel::Style UiDesignerReferencePillStyle()
{
    UiPanel::Style style = UiTheme::ResolvePanel(UiRole::Subtle);
    style.metrics.face_enabled = true;
    style.palette.face[ST_NORMAL] = UiFill::Solid(Color(243, 243, 243));
    style.metrics.frame_enabled = true;
    for(int i = 0; i < 4; i++)
        style.palette.frame[i] = Color(216, 216, 216);
    style.metrics.frame_width = DPI(1);
    style.metrics.radius = DPI(15);
    style.metrics.shadow.enabled = true;
    style.metrics.shadow.distance = DPI(9);
    style.metrics.shadow.offset_x = DPI(0);
    style.metrics.shadow.offset_y = DPI(0);
    style.metrics.shadow.alpha = 40;
    style.metrics.shadow.color = Black();
    style.metrics.shadow.mode = SHADOW_CURVE;
    style.metrics.shadow.curve = ShadowSoft();
    return style;
}

static UiPanel::Style UiDesignerFooterStyle(
    const UiDesignerThemeSnapshot& theme = UiDesignerThemeSnapshot())
{
    UiPanel::Style style = UiDesignerSurfaceStyle(UiRole::Subtle, theme);
    style.metrics.shadow.enabled = false;
    return style;
}

static UiScrollPanel::Style UiDesignerPreviewStyle()
{
    UiScrollPanel::Style style = UiTheme::ResolveScrollPanel(UiRole::Subtle);
    style.metrics.face_enabled = false;
    style.metrics.frame_enabled = true;
    style.metrics.frame_width = DPI(0);
    style.metrics.radius = DPI(0);
    style.metrics.shadow.enabled = false;
    return style;
}

static PropertyEditorStyle UiDesignerInspectorStyle()
{
    PropertyEditorStyle style = PropertyEditorStyle::System();
    style.filter_height = DPI(72);
    return style;
}

UiDesignerWindow::UiDesignerWindow() : interaction_overlay_(*this)
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

    footer_surface_.SetCustomStyle(UiDesignerFooterStyle());
    footer_.SetText("Ready").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    footer_surface_.Add(footer_.SizePos());
    Add(footer_surface_);

    ConnectServices();
    interaction_overlay_.SetDecorationsVisible(decorations_visible_);
    session_.AttachProjection(&preview_canvas_);
    ApplyThemeToShell();
    RefreshHierarchy();
    RefreshInspector();
    RefreshBehavior();
    RefreshThemeInspector();
    RefreshCode();
}

void UiDesignerWindow::UpdateDecorationsButton()
{
    decorations_.SetIcon(decorations_visible_
        ? ICON_ACTION_OUTLINED_VISIBILITY_48()
        : ICON_ACTION_OUTLINED_VISIBILITY_OFF_48());
}

void UiDesignerWindow::BuildHeader()
{
    header_surface_.SetCustomStyle(UiDesignerSurfaceStyle());
    Add(header_surface_);
    header_surface_.Add(header_layout_);
    header_layout_.SetDirection(UiDirection::H)
                  .SetGap(DPI(8))
                  .SetInset(UiDesignerStyleMetrics::HeaderInset())
                  .SetWrap(UiBoxWrap::Flow)
                  .SetWrapAutoResize(true)
                  .SetAlignItems(UiCrossAlign::Center);

    brand_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
    brand_.SetTitle("Designer").ShowTitleLine(false).ShowCardLine(false)
          .SetContentInset(DPI(4)).SetMediaGap(DPI(9))
          .SetMediaReserve(0).SetMediaMin(DPI(15)).SetMediaAutoFit(false);
    brand_.SetMedia(ICON_BRAND_NEWLOGO_V5_48(), Size(DPI(18), DPI(18)))
          .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER);

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
        if(action == "open") LoadDocument();
        else if(action == "blank") session_.NewDocument("blank");
        else if(action == "settings") session_.NewDocument("settings");
        else session_.NewDocument("three_pane");
    };

    export_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    export_.SetText("Export").SetSplitWidth(DPI(31));
    export_.Add("Complete C++ package", (int)UiDesignerExportProfile::CompleteCppPackage)
           .Add("C++ component / class", (int)UiDesignerExportProfile::ComponentOnly)
           .Add("UiDesigner project JSON", (int)UiDesignerExportProfile::ProjectJson)
           .Add("Document JSON", (int)UiDesignerExportProfile::DocumentJson)
           .Add("Theme JSON", (int)UiDesignerExportProfile::ThemeJson);
    export_.WhenAction = [=] { ExportProject(last_export_profile_); };
    export_.WhenSelect = [=](int, const Value& value) {
        last_export_profile_ = (UiDesignerExportProfile)(int)value;
        ExportProject(last_export_profile_);
    };

    version_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Accent));
    version_.SetText(UI_DESIGNER_VERSION)
            .SetIcon(ICON_DESIGN_ADJUST_48(), UiIconRenderMode::MonoTint)
            .SetIconSize(DPI(10), DPI(10));

    designer_mode_.SetText("Designer").SetCheckable().SetChecked(true);
    theme_mode_.SetText("Theme Studio").SetCheckable();
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
                 "Core, commands, catalog, semantic layout items, preview, "
                 "theme, behavior bindings, code generation, CLI and MCP "
                 "share one command/property pipeline.");
    };

    exit_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
    exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16));
    exit_.Tip("Close Ui Designer");
    exit_.WhenAction = [=] { Close(); };

    header_layout_.Add(brand_).Fixed(DPI(130)).MinCross(DPI(24));
    header_layout_.Add(save_).Fixed(DPI(92)).MinCross(DPI(24));
    header_layout_.Add(load_).Fixed(DPI(92)).MinCross(DPI(24));
    header_layout_.Add(export_).Fixed(DPI(100)).MinCross(DPI(24));
    header_layout_.Add(version_).Fixed(DPI(106)).MinCross(DPI(24));
    header_layout_.AddSpacer(1).Expand(1).MinMain(DPI(10));
    header_layout_.Add(designer_mode_).Fixed(DPI(92)).MinCross(DPI(24));
    header_layout_.Add(theme_mode_).Fixed(DPI(120)).MinCross(DPI(24));
    header_layout_.Add(theme_select_).Fixed(DPI(150)).MinCross(DPI(24));
    header_layout_.Add(dark_).Fixed(DPI(36)).MinCross(DPI(24));
    header_layout_.Add(help_).Fixed(DPI(36)).MinCross(DPI(24));
    header_layout_.Add(exit_).Fixed(DPI(36)).MinCross(DPI(24));
}

void UiDesignerWindow::BuildDesigner()
{
    designer_page_.Add(designer_left_);
    designer_page_.Add(designer_center_);
    designer_page_.Add(designer_right_);

    const UiDesignerCatalog& catalog = session_.Catalog();
    presets_list_.SetCatalog(&catalog); presets_list_.SetPresets();
    layouts_list_.SetCatalog(&catalog); layouts_list_.SetCategory("Layouts");
    containers_list_.SetCatalog(&catalog); containers_list_.SetCategory("Containers");
    controls_list_.SetCatalog(&catalog); controls_list_.SetCategory("Ui Controls");
    composites_list_.SetCatalog(&catalog); composites_list_.SetCategory("Composites");
    upp_controls_list_.SetCatalog(&catalog); upp_controls_list_.SetCategory("U++ Controls");

    designer_left_.AddSection("Layouts", ICON_DESIGN_LAYOUTS_CATEGORY_48(), layouts_list_)
                  .AddSection("Containers", ICON_DESIGN_TAB_GROUP_48(), containers_list_)
                  .AddSection("Controls", ICON_DESIGN_WIDGETS_48(), controls_list_)
                  .AddSection("Composites", ICON_DESIGN_DYNAMIC_FORM_48(), composites_list_)
                  .AddSection("Presets", ICON_DESIGN_DASHBOARD_EDIT_48(), presets_list_)
                  .AddSection("U++ Controls", ICON_EDITOR_CLARIFY_48(), upp_controls_list_);

    designer_right_.RightColumn()
                   .AddSection("Hierarchy", ICON_DESIGN_ACCOUNT_TREE_48(), hierarchy_)
                   .AddSection("Inspector", ICON_DESIGN_TUNE_48(), inspector_)
                   .AddSection("Data", ICON_EDITOR_FORMAT_LIST_BULLETED_48(), data_shell_)
                   .AddSection("Theme Overrides", ICON_DESIGN_FORMAT_PAINT_48(), overrides_)
                   .AddSection("Events & Actions", ICON_DESIGN_DYNAMIC_FORM_48(), behaviors_)
                   .AddSection("Code", ICON_DESIGN_CODE_BLOCKS_48(), code_)
                   .AddSection("Diagnostics", ICON_DESIGN_INFO_48(), diagnostics_shell_);
    designer_right_.SetActiveSection(0);
    inspector_.SetStyle(UiDesignerInspectorStyle());
    behaviors_.SetStyle(UiDesignerInspectorStyle());
    overrides_.SetStyle(UiDesignerInspectorStyle());
    data_shell_.SetReadOnly();
    diagnostics_shell_.SetReadOnly();

    designer_center_.SetCustomStyle(UiDesignerSurfaceStyle());
    aspect_pill_.SetCustomStyle(UiDesignerReferencePillStyle()).SetInset(DPI(5));
    portrait_.SetIcon(ICON_DESIGN_SPLITSCREEN_PORTRAIT_48())
             .SetIconSize(DPI(20), DPI(20)).Tip("Portrait");
    landscape_.SetIcon(ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48())
              .SetIconSize(DPI(20), DPI(20)).Tip("Landscape");
    aspect_preset_.SetText("16:9").SetSplitWidth(DPI(30));
    aspect_preset_.Add("Portrait 9:16", "9:16")
                  .Add("Portrait 2:3", "2:3")
                  .Add("Portrait 3:4", "3:4")
                  .Add("Square 1:1", "1:1")
                  .Add("Landscape 4:3", "4:3")
                  .Add("Landscape 3:2", "3:2")
                  .Add("Landscape 16:9", "16:9")
                  .Add("Landscape 2:1", "2:1");
    square_.SetIcon(ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48())
           .SetIconSize(DPI(17), DPI(17)).Tip("Square");
    UpdateDecorationsButton();
    decorations_.SetIconSize(DPI(16), DPI(16))
                .Tip("Toggle Designer decorations");
    decorations_.WhenAction = [=] {
        decorations_visible_ = !decorations_visible_;
        UpdateDecorationsButton();
        interaction_overlay_.SetDecorationsVisible(decorations_visible_);
    };

    portrait_.WhenAction = [=] { session_.SetVirtualSize(Size(576, 1024)); };
    landscape_.WhenAction = [=] { session_.SetVirtualSize(Size(1024, 576)); };
    square_.WhenAction = [=] { session_.SetVirtualSize(Size(720, 720)); };
    aspect_preset_.WhenSelect = [=](int, const Value& value) {
        const String ratio = value;
        if(ratio == "9:16") session_.SetVirtualSize(Size(576, 1024));
        else if(ratio == "2:3") session_.SetVirtualSize(Size(600, 900));
        else if(ratio == "3:4") session_.SetVirtualSize(Size(600, 800));
        else if(ratio == "1:1") session_.SetVirtualSize(Size(720, 720));
        else if(ratio == "4:3") session_.SetVirtualSize(Size(800, 600));
        else if(ratio == "3:2") session_.SetVirtualSize(Size(900, 600));
        else if(ratio == "2:1") session_.SetVirtualSize(Size(1000, 500));
        else session_.SetVirtualSize(Size(1024, 576));
    };
    aspect_pill_.AddControl(portrait_, DPI(32))
                .AddControl(landscape_, DPI(32))
                .AddControl(square_, DPI(32))
                .AddControl(aspect_preset_, DPI(92))
                .AddControl(decorations_, DPI(32));

    preview_scroll_.SetCustomStyle(UiDesignerPreviewStyle());
    preview_scroll_.SetInset(DPI(0));
    preview_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
    preview_workspace_.Add(preview_canvas_);
    preview_workspace_.Add(interaction_overlay_);
    preview_scroll_.Content().Add(preview_workspace_);
    designer_center_.Add(aspect_pill_);
    designer_center_.Add(preview_scroll_);

    auto wire_list = [=](UiDesignerCatalogList& list) {
        list.WhenActivate = [=](const String& id) { ActivateToolbox(id); };
        list.WhenFilter = [=](const String& query) {
            session_.State().toolbox_filter = query;
        };
        list.WhenToolDrag = [=](const String& type_id, Point screen) {
            TrackCatalogDrag(type_id, screen);
        };
        list.WhenToolDrop = [=](const String& type_id, Point screen) {
            FinishCatalogDrag(type_id, screen);
        };
        list.WhenToolCancel = [=] { CancelCatalogDrag(); };
    };
    wire_list(presets_list_); wire_list(layouts_list_);
    wire_list(containers_list_); wire_list(controls_list_);
    wire_list(composites_list_); wire_list(upp_controls_list_);
}

void UiDesignerWindow::BuildTheme()
{
    theme_page_.Add(theme_gallery_column_);
    theme_page_.Add(theme_right_);
    theme_gallery_column_.SetCustomStyle(UiDesignerSurfaceStyle());
    theme_gallery_pill_.SetInset(UiDesignerStyleMetrics::LeftPillInset());

    theme_all_.SetIcon(ICON_DESIGN_WIDGETS_48()).SetIconSize(DPI(16), DPI(16)).Tip("All controls");
    theme_inputs_.SetIcon(ICON_DESIGN_DYNAMIC_FORM_48()).SetIconSize(DPI(16), DPI(16)).Tip("Inputs");
    theme_containers_.SetIcon(ICON_DESIGN_TAB_GROUP_48()).SetIconSize(DPI(16), DPI(16)).Tip("Containers");
    theme_all_.WhenAction = [=] { theme_gallery_.SetFilter("all"); };
    theme_inputs_.WhenAction = [=] { theme_gallery_.SetFilter("inputs"); };
    theme_containers_.WhenAction = [=] { theme_gallery_.SetFilter("containers"); };
    theme_gallery_pill_.AddControl(theme_all_, DPI(32))
                       .AddControl(theme_inputs_, DPI(32))
                       .AddControl(theme_containers_, DPI(32));

    gallery_surface_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Standard));
    gallery_surface_.Add(theme_gallery_);
    gallery_scroll_.SetCustomStyle(UiTheme::ResolveScrollPanel(UiRole::Subtle));
    gallery_scroll_.SetInset(DPI(0));
    gallery_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
    gallery_scroll_.Add(gallery_surface_.SizePos());
    theme_gallery_column_.Add(theme_gallery_pill_);
    theme_gallery_column_.Add(gallery_scroll_);

    theme_right_.RightColumn()
                .AddSection("Inspector", ICON_DESIGN_TUNE_48(), theme_inspector_)
                .AddSection("Code", ICON_DESIGN_CODE_BLOCKS_48(), theme_code_);
    theme_inspector_.SetStyle(UiDesignerInspectorStyle());
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
    hierarchy_.WhenDelete = [=] {
        if(session_.RemoveSelection())
            RefreshStatus("Selection deleted");
        else
            RefreshStatus(session_.Commands().GetLastError());
    };
    hierarchy_.PlanDrop = [=](const Vector<UiDesignerNodeId>& nodes,
                              UiDesignerNodeId parent, int index) {
        return session_.Drops().PlanMove(nodes, parent, Point(), false, index);
    };
    hierarchy_.ExecuteDrop = [=](const UiDesignerDropPlan& plan, String& error) {
        return session_.ExecuteDrop(plan, nullptr, error);
    };
    hierarchy_.WhenDropStatus = [=](const String& status) { RefreshStatus(status); };

    inspector_.SetModel(&session_.InspectorModel());
    inspector_.WhenPreview = [=](const String& id, const Value& value) {
        String error;
        if(!session_.PreviewProperty(id, value, error)) RefreshStatus(error);
    };
    inspector_.WhenCommit = [=](const String& id, const Value& value) {
        String error;
        if(!session_.CommitProperty(id, value, error)) RefreshStatus(error);
    };
    inspector_.WhenReset = [=](const String& id) {
        String error;
        if(!session_.ResetProperty(id, error)) RefreshStatus(error);
    };

    behaviors_.SetModel(&session_.BehaviorModel());
    behaviors_.WhenCommit = [=](const String& id, const Value& value) {
        String error;
        if(!session_.CommitBehaviorField(id, value, error)) RefreshStatus(error);
        else RefreshBehavior();
    };
    behaviors_.WhenReset = [=](const String&) {
        String error;
        if(!session_.RemoveActiveBehavior(error)) RefreshStatus(error);
        else RefreshBehavior();
    };

    overrides_.SetModel(&session_.ThemeOverrideModel());
    overrides_.WhenPreview = [=](const String& id, const Value& value) {
        String error;
        if(!session_.PreviewThemeOverride(id, value, error))
            RefreshStatus(error);
    };
    overrides_.WhenCommit = [=](const String& id, const Value& value) {
        String error;
        if(!session_.CommitThemeOverride(id, value, error))
            RefreshStatus(error);
    };
    overrides_.WhenReset = [=](const String& id) {
        String error;
        if(!session_.ResetThemeOverride(id, error))
            RefreshStatus(error);
    };
    theme_inspector_.SetModel(&session_.ThemeModel());
    theme_inspector_.WhenPreview = [=](const String& id, const Value& value) {
        String error;
        if(!session_.Theme().Preview(id, value, error)) RefreshStatus(error);
        ApplyThemeToShell();
    };
    theme_inspector_.WhenCommit = [=](const String& id, const Value& value) {
        String error;
        if(!session_.Theme().Commit(id, value, "Set theme " + id, error)) RefreshStatus(error);
        ApplyThemeToShell();
        RefreshThemeInspector();
    };
    theme_inspector_.WhenReset = [=](const String& id) {
        String error;
        if(!session_.Theme().Reset(id, error)) RefreshStatus(error);
        ApplyThemeToShell();
        RefreshThemeInspector();
    };

    session_.WhenSelectionChanged = [=] {
        // Selection feedback is visible immediately. Code and behavior panes do
        // not affect the selected control, so coalesce their heavier rebuild.
        RefreshHierarchy();
        RefreshInspector();
        overrides_.Refresh();
        preview_canvas_.Refresh();
        interaction_overlay_.Refresh();
        PostSelectionDetailsRefresh();
    };
    session_.WhenInspectorChanged = [=] { RefreshInspector(); };
    session_.WhenBehaviorChanged = [=] { RefreshBehavior(); };
    session_.WhenCodeChanged = [=] { RefreshCode(); };
    session_.WhenStatus = [=](const String& text) { RefreshStatus(text); };

    session_.Document().WhenChanged = [=](const UiDesignerChangeSet& changes) {
        interaction_overlay_.InvalidateCatalogDrag();
        preview_canvas_.ApplyChangeSet(changes);
        if(changes.virtual_size_changed)
            RefreshLayout();
        interaction_overlay_.Refresh();
        RefreshHierarchy(); RefreshCode(); RefreshBehavior();
        overrides_.Refresh();
    };
    session_.Theme().WhenChanged = [=] {
        ApplyThemeToShell(); RefreshThemeInspector(); RefreshCode();
    };
    session_.Theme().WhenPreviewChanged = [=] { ApplyThemeToShell(); };

    designer_left_.WhenWidthChanged = [=] { Layout(); };
    designer_right_.WhenWidthChanged = [=] { Layout(); };
    theme_right_.WhenWidthChanged = [=] { Layout(); };
}

void UiDesignerWindow::ApplyThemeToShell()
{
    const UiDesignerThemeSnapshot& theme = session_.Theme().GetEffective();
    UiDesignerApplyGlobalTheme(theme);
    header_surface_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Standard, theme));
    designer_mode_.SetCustomStyle(UiTheme::ResolveButton(
        session_.State().active_workspace == "designer" ? UiRole::Accent : UiRole::Subtle));
    theme_mode_.SetCustomStyle(UiTheme::ResolveButton(
        session_.State().active_workspace == "theme" ? UiRole::Accent : UiRole::Subtle));
    footer_surface_.SetCustomStyle(UiDesignerFooterStyle(theme));
    designer_left_.ApplyTheme(theme);
    designer_right_.ApplyTheme(theme);
    theme_right_.ApplyTheme(theme);
    designer_center_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Standard, theme));
    theme_gallery_column_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Standard, theme));
    gallery_surface_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Standard, theme));
    // This is an intentional instance override from DesignerExportGrid, not
    // a generic theme pill. Reapply it after global theme changes.
    aspect_pill_.SetCustomStyle(UiDesignerReferencePillStyle());
    preview_scroll_.SetCustomStyle(UiDesignerPreviewStyle());
    theme_gallery_pill_.ApplyTheme(theme);
    preview_canvas_.SetAccent(theme.accent);
    theme_gallery_.SetThemeDocument(&session_.Theme());
    RefreshLayout(); Refresh();
}

void UiDesignerWindow::ShowDesigner()
{
    workspaces_.SetActiveKey("designer");
    session_.State().active_workspace = "designer";
    designer_mode_.SetChecked(true); theme_mode_.SetChecked(false);
    ApplyThemeToShell();
    RefreshStatus("Designer workspace");
}

void UiDesignerWindow::ShowTheme()
{
    workspaces_.SetActiveKey("theme");
    session_.State().active_workspace = "theme";
    designer_mode_.SetChecked(false); theme_mode_.SetChecked(true);
    ApplyThemeToShell();
    RefreshThemeInspector();
    RefreshStatus("Theme Studio workspace");
}

void UiDesignerWindow::ToggleDarkMode()
{
    const String next = session_.Theme().GetEffective().mode == "Dark" ? "Light" : "Dark";
    String error;
    session_.Theme().Commit("mode", next, "Toggle dark mode", error);
    ApplyThemeToShell();
    RefreshThemeInspector();
}

void UiDesignerWindow::ActivateToolbox(const String& id)
{
    if(id.StartsWith("preset:")) {
        const String preset_id = id.Mid(7);
        const UiDesignerPreset* preset = session_.Catalog().FindPreset(preset_id);
        const bool has_document_content = session_.Document().GetCount() > 1;
        if((session_.Commands().IsDirty() || has_document_content) &&
           !PromptYesNo("Replace the current design with " +
                        (preset ? preset->display_name : preset_id) + "?\n\n"
                        "The current document will be replaced."))
            return;
        session_.NewDocument(preset_id);
        return;
    }
    RefreshStatus("Selected " + id + ". Drag it onto the Window to add it.");
}

void UiDesignerWindow::TrackCatalogDrag(const String& type_id, Point screen)
{
    active_catalog_drag_type_ = type_id;
    catalog_drag_active_ = !type_id.IsEmpty();
    if(!catalog_drag_active_) {
        interaction_overlay_.CancelCatalogDrag();
        return;
    }
    interaction_overlay_.TrackCatalogDrag(type_id, screen);
}

void UiDesignerWindow::FinishCatalogDrag(const String& type_id, Point screen)
{
    const String drag_type = active_catalog_drag_type_.IsEmpty() ? type_id
                                                                 : active_catalog_drag_type_;
    catalog_drag_active_ = false;
    active_catalog_drag_type_.Clear();
    interaction_overlay_.FinishCatalogDrag(drag_type, screen);
}

void UiDesignerWindow::CancelCatalogDrag()
{
    catalog_drag_active_ = false;
    active_catalog_drag_type_.Clear();
    interaction_overlay_.CancelCatalogDrag();
}

void UiDesignerWindow::SaveDocument(bool save_as)
{
    if(current_file_.IsEmpty() || save_as) {
        FileSel fs;
        fs.Type("UiDesigner project", "*.uidesign.json");
        if(!fs.ExecuteSaveAs("Save UiDesigner project")) return;
        current_file_ = ~fs;
    }
    String error;
    if(!session_.Save(current_file_, error)) Exclamation(error);
    else RefreshStatus("Saved " + current_file_);
}

void UiDesignerWindow::LoadDocument()
{
    FileSel fs;
    fs.Type("UiDesigner project", "*.uidesign.json");
    fs.Type("Legacy Designer JSON", "*.json");
    if(!fs.ExecuteOpen("Load UiDesigner project")) return;
    String error;
    if(!session_.Load(~fs, error)) { Exclamation(error); return; }
    current_file_ = ~fs;
    RefreshHierarchy(); RefreshInspector(); RefreshBehavior(); RefreshCode();
    RefreshStatus("Loaded " + current_file_);
}

void UiDesignerWindow::ExportProject(UiDesignerExportProfile profile)
{
    UiDesignerExportDialog dialog(session_, profile);
    if(dialog.Execute()) {
        last_export_profile_ = profile;
        RefreshStatus(dialog.GetResult().diagnostic);
    }
}

void UiDesignerWindow::RefreshHierarchy()
{
    hierarchy_.SetSelection(&session_.State().selection);
    hierarchy_.Rebuild();
}

void UiDesignerWindow::RefreshInspector()
{
    inspector_.SetModel(&session_.InspectorModel());
    inspector_.Refresh();
    RefreshData();
    RefreshDiagnostics();
}

void UiDesignerWindow::RefreshData()
{
    const UiDesignerNodeId selected = session_.State().selection.primary;
    const UiDesignerNode* node = selected ? session_.Document().Find(selected) : nullptr;
    const UiDesignerControlSpec* spec = node ? session_.Catalog().Find(node->type) : nullptr;
    String out;
    if(!node) {
        out = "This control has no authored data model.\n\nNo node is selected.";
    }
    else {
        out << "Selected control\n";
        out << "  name: " << node->name << "\n";
        out << "  type: " << node->type << "\n";
        out << "  adapter: " << (spec ? spec->preview_adapter_id : String()) << "\n";
        out << "  storage: " << (spec ? spec->child_adapter_id : String()) << "\n";
        out << "  count: " << node->children.GetCount() << "\n\n";
        if(spec)
            out << "Data editing will be available through this adapter.";
        else
            out << "This control has no authored data model.";
    }
    data_shell_.SetData(out);
}

void UiDesignerWindow::RefreshBehavior()
{
    session_.RebuildBehaviorModel();
    behaviors_.SetModel(&session_.BehaviorModel());
    behaviors_.Refresh();
}

void UiDesignerWindow::RefreshThemeInspector()
{
    theme_inspector_.SetModel(&session_.ThemeModel());
    theme_inspector_.Refresh();
    theme_code_.SetCode(session_.Theme().Serialize(true));
}

void UiDesignerWindow::RefreshCode()
{
    code_.SetCode(session_.GenerateHeader("GeneratedUiWindow") + "\n" +
                  session_.GenerateCode("GeneratedUiWindow"));
    RefreshDiagnostics();
}

void UiDesignerWindow::RefreshDiagnostics()
{
    const UiDesignerPreviewStats& stats = preview_canvas_.GetStats();
    const UiDesignerDocument& document = session_.Document();
    const UiDesignerNodeId selected = session_.State().selection.primary;
    const UiDesignerNode* node = selected ? document.Find(selected) : nullptr;
    const UiDesignerNode* parent = node && node->parent ? document.Find(node->parent) : nullptr;
    const UiDesignerControlSpec* spec = node ? session_.Catalog().Find(node->type) : nullptr;
    String out;
    out << "LIVE PREVIEW\n";
    out << "  controls: " << document.GetNodes().GetCount() << "\n";
    out << "  instances: " << stats.layout_count << "\n";
    out << "  full rebuilds: " << stats.full_rebuilds << "\n";
    out << "  subtree rebuilds: " << stats.subtree_rebuilds << "\n";
    out << "  direct applies: " << stats.live_applies << "\n";
    out << "  layout-item updates: " << stats.layout_item_updates << "\n";
    out << "  deferred batches: 0\n\n";
    out << "LATEST RESIZE\n";
    out << "  total event: 0 ms\n";
    out << "  live mutation: 0 ms\n";
    out << "  grid/box layout: 0 ms\n";
    out << "  geometry snapshot: 0 ms\n";
    out << "  overlay repaint: 0 ms\n";
    out << "  canvas paint: 0 ms\n";
    out << "  inspector: 0 ms\n";
    out << "  code: 0 ms\n\n";
    out << "SELECTED CONTROL\n";
    if(node) {
        out << "  node id: " << (int64)node->id << "\n";
        out << "  type: " << node->type << "\n";
        out << "  runtime: " << (spec ? spec->runtime_cpp_type : String()) << "\n";
        out << "  runtime generation: " << (int64)preview_canvas_.GetInstanceGeneration(node->id) << "\n";
        out << "  parent runtime type: " << (parent ? parent->type : String()) << "\n";
        out << "  rect: " << AsString(preview_canvas_.GetNodeRect(node->id)) << "\n";
        out << "  sizing: "
            << AsString(node->GetProperty("width_mode", "Expand")) << " / "
            << AsString(node->GetProperty("height_mode", "Expand")) << "\n";
    }
    else
        out << "  none\n";
    diagnostics_shell_.SetData(out);
}

void UiDesignerWindow::PostSelectionDetailsRefresh()
{
    if(selection_details_refresh_posted_)
        return;
    selection_details_refresh_posted_ = true;
    Ptr<UiDesignerWindow> self = this;
    PostCallback([self] {
        if(!self)
            return;
        self->selection_details_refresh_posted_ = false;
        self->RefreshBehavior();
        self->RefreshData();
        self->RefreshCode();
        self->RefreshDiagnostics();
    });
}

void UiDesignerWindow::RefreshStatus(const String& status)
{
    footer_.SetText(status.IsEmpty() ? "Ready" : status);
}

void UiDesignerWindow::WriteLaunchDiagnostic()
{
    ValueMap diagnostic;
    diagnostic.Set("title", GetTitle());
    const Size virtual_size = session_.Document().GetVirtualSize();
    diagnostic.Set("document_width", virtual_size.cx);
    diagnostic.Set("document_height", virtual_size.cy);
    Rect r = GetRect();
    ValueMap rect;
    rect.Set("left", r.left); rect.Set("top", r.top);
    rect.Set("right", r.right); rect.Set("bottom", r.bottom);
    diagnostic.Set("rect", rect);
    diagnostic.Set("open", IsOpen());
#ifdef PLATFORM_WIN32
    diagnostic.Set("process_id", (int64)::GetCurrentProcessId());
    diagnostic.Set("native_handle", (int64)(uintptr_t)GetHWND());
#else
    diagnostic.Set("native_handle", (int64)0);
#endif
    SaveFile(AppendFileName(GetTempPath(), "uidesigner-launch.json"),
             AsJSON(diagnostic, true));
}

void UiDesignerWindow::Layout()
{
    const int margin = UiDesignerStyleMetrics::Gap();
    const int gap = UiDesignerStyleMetrics::Gap();
    const Size size = GetSize();
    const int header_w = max(0, size.cx - margin * 2);
    const int header_h = max(UiDesignerStyleMetrics::HeaderHeight(),
                             header_layout_.MeasureHeightForWidth(header_w));
    const int footer_h = UiDesignerStyleMetrics::FooterHeight();

    Put(header_surface_, margin, margin, header_w, header_h);
    const int header_content_h = header_layout_.MeasureHeightForWidth(header_w);
    header_layout_.SetRect(0, max(0, (header_h - header_content_h) / 2),
                           header_w, header_content_h);
    const int content_y = margin + header_h + gap;
    const int content_h = max(0, size.cy - content_y - footer_h - gap - margin);
    Put(workspaces_, margin, content_y, max(0, size.cx - margin * 2), content_h);
    Put(footer_surface_, margin, content_y + content_h + gap,
        max(0, size.cx - margin * 2), footer_h);

    const int left_w = designer_left_.GetDesiredWidth();
    const int right_w = designer_right_.GetDesiredWidth();
    const int inner_h = designer_page_.GetSize().cy;
    // Side columns own their fixed width. The center must use only the
    // remaining page rectangle; inventing a minimum here previously pushed
    // the right column beyond the page and let preview chrome overlap it.
    const int center_w = max(0, designer_page_.GetSize().cx - left_w - right_w);
    Put(designer_left_, 0, 0, left_w, inner_h);
    Put(designer_center_, left_w, 0, center_w, inner_h);
    Put(designer_right_, left_w + center_w, 0, right_w, inner_h);

    const int pill_h = UiDesignerStyleMetrics::DesignerToolbarHeight();
    const int pill_w = min(designer_center_.GetSize().cx, DPI(340));
    Put(aspect_pill_, max(0, (designer_center_.GetSize().cx - pill_w) / 2),
        0, pill_w, pill_h);
    Put(preview_scroll_, 0, pill_h, designer_center_.GetSize().cx,
        max(0, designer_center_.GetSize().cy - pill_h));
    const int theme_right_w = theme_right_.GetDesiredWidth();
    const int theme_gallery_w = max(0, theme_page_.GetSize().cx - theme_right_w - gap);
    Put(theme_gallery_column_, 0, 0, theme_gallery_w, theme_page_.GetSize().cy);
    Put(theme_right_, theme_gallery_w + gap, 0, theme_right_w, theme_page_.GetSize().cy);
    Put(theme_gallery_pill_, 0, 0, theme_gallery_column_.GetSize().cx, pill_h);
    Put(gallery_scroll_, 0, pill_h + gap, theme_gallery_column_.GetSize().cx,
        max(0, theme_gallery_column_.GetSize().cy - pill_h - gap));
    const Size virtual_size = session_.Document().GetVirtualSize();
    const int preview_margin = DPI(40);
    const Size preview_size(max(preview_scroll_.GetSize().cx,
                                virtual_size.cx + preview_margin * 2),
                            max(preview_scroll_.GetSize().cy,
                                virtual_size.cy + preview_margin * 2));
    preview_workspace_.SetRect(0, 0, preview_size.cx, preview_size.cy);
    preview_canvas_.SetRect((preview_size.cx - virtual_size.cx) / 2,
                            (preview_size.cy - virtual_size.cy) / 2,
                            virtual_size.cx, virtual_size.cy);
    interaction_overlay_.SetRect(0, 0, preview_size.cx, preview_size.cy);

    const Size gallery_size(gallery_scroll_.GetSize().cx,
                            theme_gallery_.GetContentHeight());
    gallery_surface_.SetRect(0, 0, max(gallery_scroll_.GetSize().cx, gallery_size.cx),
                            max(gallery_scroll_.GetSize().cy, gallery_size.cy));
    theme_gallery_.SetRect(DPI(8), DPI(8),
                           max(0, gallery_surface_.GetSize().cx - DPI(16)),
                           max(0, gallery_surface_.GetSize().cy - DPI(16)));
}

void UiDesignerWindow::Close()
{
    if(session_.Commands().IsDirty() &&
       !PromptYesNo("The UiDesigner document has unsaved changes. Close anyway?"))
        return;
    TopWindow::Close();
}

}
