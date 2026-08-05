#include "UiDesignerWindow.h"
#include "UiDesignerVersion.h"
#include <Ui/UiIcons.h>
#include <Utilities/UiDesigner/Services/UiDesignerTreeDataAdapter.h>
#include <Utilities/UiDesigner/Services/UiDesignerListDataAdapter.h>

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

static UiDesignerNodeId SelectedModelNodeId(const Value& value)
{
    return IsNull(value) ? 0 : (UiDesignerNodeId)(int64)value;
}

static const UiDesignerNode* ResolveAccordionOwner(const UiDesignerDocument& document,
                                                   const UiDesignerNode* node)
{
    if(!node)
        return nullptr;
    if(node->type == "UiAccordion")
        return node;
    if(node->type == "UiAccordionSection")
        return document.Find(node->parent);
    return nullptr;
}

static const UiDesignerNode* ResolveAccordionSection(const UiDesignerDocument& document,
                                                     UiDesignerNodeId section_id)
{
    const UiDesignerNode* section = document.Find(section_id);
    if(!section || section->type != "UiAccordionSection")
        return nullptr;
    const UiDesignerNode* owner = document.Find(section->parent);
    return owner && owner->type == "UiAccordion" ? section : nullptr;
}

static int FindChildIndex(const UiDesignerNode& parent, UiDesignerNodeId child)
{
    for(int i = 0; i < parent.children.GetCount(); i++)
        if(parent.children[i] == child)
            return i;
    return -1;
}

static ValueMap DesignerTreeRoot(const UiDesignerNode& node)
{
    return UiDesignerTreeDataAdapter::Root(node);
}

static ValueArray DesignerTreeChildren(const ValueMap& root)
{
    return UiDesignerTreeDataAdapter::Children(root);
}

static ValueArray DesignerTreePath(const Value& value)
{
    return UiDesignerTreeDataAdapter::Path(value);
}

static ValueMap DesignerTreeItemAt(const ValueMap& root, const ValueArray& path)
{
    return UiDesignerTreeDataAdapter::ItemAt(root, path);
}

static bool DesignerTreeSetItemAt(ValueMap& root, const ValueArray& path,
                                  const ValueMap& replacement, int depth = 0)
{
    (void)depth;
    return UiDesignerTreeDataAdapter::SetItem(root, path, replacement);
}

static bool DesignerTreeAppendChild(ValueMap& root, const ValueArray& parent_path,
                                    const ValueMap& child)
{
    return UiDesignerTreeDataAdapter::AppendChild(root, parent_path, child);
}

static bool DesignerTreeRemoveItem(ValueMap& root, const ValueArray& path)
{
    return UiDesignerTreeDataAdapter::RemoveItem(root, path);
}

static bool DesignerTreeMoveItem(ValueMap& root, const ValueArray& path, int delta)
{
    return UiDesignerTreeDataAdapter::MoveItem(root, path, delta);
}

static ValueArray ParentTreePath(const ValueArray& path)
{
    ValueArray parent;
    for(int i = 0; i + 1 < path.GetCount(); i++)
        parent.Add(path[i]);
    return parent;
}

static ValueMap DesignerListRoot(const UiDesignerNode& node)
{
    return UiDesignerListDataAdapter::Root(node);
}

static Value CurrentDataListToken(const UiList& list, const UiListModel& model)
{
    const Vector<int> selection = list.GetSelection();
    if(!selection.IsEmpty() && selection[0] >= 0 &&
       selection[0] < model.GetCount())
        return model.Get(selection[0]).data;
    return list.GetData();
}

static Value SelectedDataToken(const Value& snapshot, const UiList& list,
                               const UiListModel& model)
{
    if(!IsNull(snapshot))
        return snapshot;
    return CurrentDataListToken(list, model);
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
    style.row_odd = Color(250, 249, 247);
    style.row_even = Color(255, 247, 238);
    style.row_hover = Color(255, 238, 214);
    style.row_selected = Color(220, 235, 248);
    style.group_background = Color(233, 226, 217);
    style.group_ink = Color(70, 60, 52);
    style.label_ratio = 38;
    style.show_group_summaries = true;
    return style;
}

static Size UiDesignerScaleVirtualSize(Size current, int width, int height)
{
    width = max(1, width);
    height = max(1, height);
    const int base = max(1, max(current.cx, current.cy));
    if(width >= height) {
        const int scaled_height = max(1, int(base * (double)height / width + 0.5));
        return Size(base, scaled_height);
    }
    const int scaled_width = max(1, int(base * (double)width / height + 0.5));
    return Size(scaled_width, base);
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
    RefreshData();
    RefreshThemeInspector();
    RefreshCode();
    RefreshDiagnostics();
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
                   .AddSection("Theme Overrides", ICON_DESIGN_FORMAT_PAINT_48(), overrides_shell_)
                   .AddSection("Data", ICON_EDITOR_FORMAT_LIST_BULLETED_48(), data_panel_,
                               "Edit the selected control’s data")
                   .AddSection("Events & Actions", ICON_DESIGN_DYNAMIC_FORM_48(), behaviors_)
                   .AddSection("Code", ICON_DESIGN_CODE_BLOCKS_48(), code_)
                   .AddSection("Diagnostics", ICON_DESIGN_INFO_48(), diagnostics_panel_,
                               "Inspect live preview performance and projection activity");
    designer_right_.SetPaneWidth(PANE_MEDIUM);
    designer_right_.SetActiveSection(0);
    inspector_.SetStyle(UiDesignerInspectorStyle());
    behaviors_.SetStyle(UiDesignerInspectorStyle());
    overrides_.SetStyle(UiDesignerInspectorStyle());
    overrides_shell_.Add(overrides_layout_.SizePos());
    overrides_layout_.SetDirection(UiDirection::V)
                    .SetGap(DPI(4), DPI(4))
                    .SetInset(DPI(4));
    overrides_visibility_.SetIcon(ICON_ACTION_OUTLINED_VISIBILITY_48())
                         .SetIconSize(DPI(16), DPI(16))
                         .Tip("Theme overrides visible in Preview");
    overrides_layout_.Add(overrides_visibility_).Fixed(DPI(28)).MinCross(DPI(24));
    overrides_layout_.Add(overrides_).Expand(1);
    overrides_visibility_.WhenAction = [=] {
        const bool suppress = !preview_canvas_.AreThemeOverridesSuppressed();
        preview_canvas_.SetThemeOverridesSuppressed(suppress);
        overrides_visibility_.SetIcon(suppress
            ? ICON_ACTION_OUTLINED_VISIBILITY_OFF_48()
            : ICON_ACTION_OUTLINED_VISIBILITY_48());
        overrides_visibility_.Tip(suppress
            ? "Theme overrides suppressed in Preview"
            : "Theme overrides visible in Preview");
        RefreshStatus(suppress ? "Theme overrides suppressed in Preview"
                               : "Theme overrides restored in Preview");
    };
    data_list_.SetModel(data_model_).SetSelectionMode(UILISTSEL_SINGLE);
    data_editor_.SetStyle(UiDesignerInspectorStyle());
    data_editor_.SetModel(&data_editor_model_);
    data_panel_.Add(data_layout_.SizePos());
    data_layout_.SetDirection(UiDirection::V).SetGap(DPI(4), DPI(4)).SetInset(DPI(6));
    data_layout_.Add(data_list_.SizePos());
    data_layout_.Add(data_editor_).Fixed(DPI(330)).MinCross(DPI(24));
    data_layout_.Add(data_actions_);
    data_actions_.SetDirection(UiDirection::H).SetWrap(UiBoxWrap::Flow).SetGap(DPI(3), DPI(3));
    data_actions_.Add(data_add_); data_actions_.Add(data_remove_); data_actions_.Add(data_rename_);
    data_actions_.Add(data_up_); data_actions_.Add(data_down_); data_actions_.Add(data_enable_); data_actions_.Add(data_active_);
    data_actions_.Add(data_select_content_); data_actions_.Add(data_remove_content_);
    data_add_.SetText("Add"); data_remove_.SetText("Remove"); data_rename_.SetText("Rename");
    data_up_.SetText("Up"); data_down_.SetText("Down"); data_enable_.SetText("Enable"); data_active_.SetText("Set Active");
    data_select_content_.SetText("Select Content"); data_remove_content_.SetText("Remove Content");
    diagnostics_shell_.SetReadOnly();
    diagnostics_panel_.SetCustomStyle(UiDesignerSurfaceStyle());
    diagnostics_panel_.Add(diagnostics_layout_);
    diagnostics_layout_.SetDirection(UiDirection::V)
                       .SetGap(DPI(6), DPI(6))
                       .SetInset(DPI(6));
    diagnostics_toolbar_.SetDirection(UiDirection::H)
                        .SetGap(DPI(4), DPI(4))
                        .SetInset(DPI(0))
                        .SetWrap(UiBoxWrap::None);
    diagnostics_reset_.SetText("Reset").SetIcon(ICON_DESIGN_CLEAR_ALL_48())
                      .SetIconSize(DPI(16), DPI(16));
    diagnostics_pause_.SetText("Pause").SetCheckable();
    diagnostics_copy_.SetText("Copy").SetIcon(ICON_CONTENT_CONTENT_COPY_48())
                     .SetIconSize(DPI(16), DPI(16));
    diagnostics_log_.SetText("Write log");
    diagnostics_timing_.SetText("Timing").SetCheckable();
    diagnostics_toolbar_.Add(diagnostics_reset_).Fixed(DPI(84)).MinCross(DPI(24));
    diagnostics_toolbar_.Add(diagnostics_pause_).Fixed(DPI(84)).MinCross(DPI(24));
    diagnostics_toolbar_.Add(diagnostics_copy_).Fixed(DPI(74)).MinCross(DPI(24));
    diagnostics_toolbar_.Add(diagnostics_log_).Fixed(DPI(90)).MinCross(DPI(24));
    diagnostics_toolbar_.Add(diagnostics_timing_).Fixed(DPI(82)).MinCross(DPI(24));
    diagnostics_layout_.Add(diagnostics_toolbar_).Fixed(DPI(28)).MinCross(DPI(24));
    diagnostics_layout_.Add(diagnostics_shell_).Expand(1);
    diagnostics_reset_.WhenAction = [=] {
        preview_canvas_.ResetPerformance();
        RefreshDiagnostics();
        RefreshStatus("Diagnostics counters reset");
    };
    diagnostics_pause_.WhenAction = [=] {
        diagnostics_capture_paused_ = diagnostics_pause_.IsChecked();
        preview_canvas_.SetCapturePaused(diagnostics_capture_paused_);
        RefreshStatus(diagnostics_capture_paused_ ? "Diagnostics capture paused"
                                                  : "Diagnostics capture resumed");
    };
    diagnostics_copy_.WhenAction = [=] {
        WriteClipboardText(AsString(diagnostics_shell_.GetData()));
        RefreshStatus("Diagnostics copied");
    };
    diagnostics_log_.WhenAction = [=] {
        const String path = AppendFileName(GetTempPath(), "uidesigner-performance.log");
        if(SaveFile(path, AsString(diagnostics_shell_.GetData())))
            RefreshStatus("Diagnostics written to " + path);
        else
            RefreshStatus("Unable to write diagnostics log");
    };
    diagnostics_timing_.WhenAction = [=] {
        preview_canvas_.SetDetailedTiming(diagnostics_timing_.IsChecked());
        preview_canvas_.SetCapturePaused(diagnostics_capture_paused_);
        RefreshDiagnostics();
        RefreshStatus(preview_canvas_.IsDetailedTimingEnabled()
            ? "Detailed timing enabled" : "Detailed timing disabled");
    };

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

    portrait_.WhenAction = [=] {
        session_.SetVirtualSize(UiDesignerScaleVirtualSize(session_.Document().GetVirtualSize(), 9, 16));
    };
    landscape_.WhenAction = [=] {
        session_.SetVirtualSize(UiDesignerScaleVirtualSize(session_.Document().GetVirtualSize(), 16, 9));
    };
    square_.WhenAction = [=] {
        session_.SetVirtualSize(UiDesignerScaleVirtualSize(session_.Document().GetVirtualSize(), 1, 1));
    };
    aspect_preset_.WhenSelect = [=](int, const Value& value) {
        const String ratio = value;
        if(ratio == "9:16") session_.SetVirtualSize(UiDesignerScaleVirtualSize(session_.Document().GetVirtualSize(), 9, 16));
        else if(ratio == "2:3") session_.SetVirtualSize(UiDesignerScaleVirtualSize(session_.Document().GetVirtualSize(), 2, 3));
        else if(ratio == "3:4") session_.SetVirtualSize(UiDesignerScaleVirtualSize(session_.Document().GetVirtualSize(), 3, 4));
        else if(ratio == "1:1") session_.SetVirtualSize(UiDesignerScaleVirtualSize(session_.Document().GetVirtualSize(), 1, 1));
        else if(ratio == "4:3") session_.SetVirtualSize(UiDesignerScaleVirtualSize(session_.Document().GetVirtualSize(), 4, 3));
        else if(ratio == "3:2") session_.SetVirtualSize(UiDesignerScaleVirtualSize(session_.Document().GetVirtualSize(), 3, 2));
        else if(ratio == "2:1") session_.SetVirtualSize(UiDesignerScaleVirtualSize(session_.Document().GetVirtualSize(), 2, 1));
        else session_.SetVirtualSize(UiDesignerScaleVirtualSize(session_.Document().GetVirtualSize(), 16, 9));
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
    hierarchy_.SetCatalog(&session_.Catalog());
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
    hierarchy_.PlanCatalogDrop = [=](const String& type,
                                     UiDesignerNodeId parent, int index) {
        return session_.Drops().PlanAdd(type, parent, Point(), false, index);
    };
    hierarchy_.IsContentHost = [=](UiDesignerNodeId node) {
        return session_.Drops().IsContentHost(node);
    };
    hierarchy_.ExecuteDrop = [=](const UiDesignerDropPlan& plan, String& error) {
        UiDesignerNodeId created = 0;
        return session_.ExecuteDrop(plan, &created, error);
    };
    hierarchy_.CycleSizingMode = [=](UiDesignerNodeId id, bool height) {
        const UiDesignerNode *node = session_.Document().Find(id);
        if(!node)
            return false;
        const String property = height ? "height_mode" : "width_mode";
        const String current = node->GetProperty(property, "Fit");
        const String next = current == "Fit" ? "Fixed"
                            : current == "Fixed" ? "Expand" : "Fit";
        String error;
        if(!session_.CommitProperty(property, next, error)) {
            RefreshStatus(error);
            return false;
        }
        return true;
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
    overrides_.WhenOverride = [=](const String& id, bool active) {
        String error;
        if(!session_.SetThemeOverrideActive(id, active, error))
            RefreshStatus(error);
        else {
            // Activation changes row editability and may create an inline
            // numeric editor; refresh this palette immediately after the
            // command-model rebuild rather than relying on a later selection
            // refresh.
            overrides_.RefreshModel();
            preview_canvas_.Refresh();
        }
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
        RequestDiagnosticsRefresh();
    };
    data_editor_.WhenCommit = [=](const String& id, const Value& value) {
        const UiDesignerNode* node = session_.Document().Find(session_.State().selection.primary);
        if(node && node->type == "UiList") {
            ValueMap root = DesignerListRoot(*node);
            const int index = UiDesignerListDataAdapter::Index(
                SelectedDataToken(data_selected_token_, data_list_, data_model_));
            ValueMap item = UiDesignerListDataAdapter::Item(root, index);
            if(item.IsEmpty())
                return;
            if(id == "text") item.Set("text", AsString(value));
            else if(id == "key") item.Set("key", AsString(value));
            else if(id == "value") item.Set("data", AsString(value));
            else if(id == "description") item.Set("description", AsString(value));
            else if(id == "right_text") item.Set("right_text", AsString(value));
            else if(id == "enabled") item.Set("enabled", (bool)value);
            else if(id == "checked") item.Set("checked", (bool)value);
            if(!UiDesignerListDataAdapter::SetItem(root, index, item))
                return;
            if(!session_.Commands().SetData(node->id, "root", root,
                                            UiDesignerImpactStructure,
                                            "Edit UiList data"))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            return;
        }
        if(node && node->type == "UiTree") {
            ValueMap root = DesignerTreeRoot(*node);
            const ValueArray path = DesignerTreePath(
                SelectedDataToken(data_selected_token_, data_list_, data_model_));
            ValueMap item = DesignerTreeItemAt(root, path);
            if(item.IsEmpty())
                return;
            if(id == "text") item.Set("text", AsString(value));
            else if(id == "key") item.Set("key", AsString(value));
            else if(id == "value") item.Set("data", AsString(value));
            else if(id == "enabled") item.Set("enabled", (bool)value);
            else if(id == "editable") item.Set("editable", (bool)value);
            if(!DesignerTreeSetItemAt(root, path, item))
                return;
            if(!session_.Commands().SetData(node->id, "root", root,
                                            UiDesignerImpactStructure,
                                            "Edit UiTree data"))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            return;
        }
        const UiDesignerNode* owner = ResolveAccordionOwner(session_.Document(), node);
        if(!owner || owner->type != "UiAccordion")
            return;
        const UiDesignerNodeId section = SelectedModelNodeId(data_list_.GetData());
        if(!section)
            return;
        String error;
        bool ok = false;
        if(id == "title")
            ok = session_.Commands().SetAccordionSectionTitle(section, AsString(value));
        else if(id == "subtitle")
            ok = session_.Commands().SetAccordionSectionSubtitle(section, AsString(value));
        else if(id == "copy")
            ok = session_.Commands().SetAccordionSectionCopy(section, AsString(value));
        else if(id == "open")
            ok = session_.Commands().SetAccordionSectionOpen(section, (bool)value);
        else if(id == "lock")
            ok = session_.Commands().SetAccordionSectionLock(section, AsString(value));
        if(!ok)
            RefreshStatus(session_.Commands().GetLastError());
        RefreshData();
    };
    data_editor_.WhenReset = [=](const String& id) {
        const UiDesignerNode* node = session_.Document().Find(session_.State().selection.primary);
        if(node && node->type == "UiTree") {
            RefreshData();
            return;
        }
        const UiDesignerNode* owner = ResolveAccordionOwner(session_.Document(), node);
        if(!owner || owner->type != "UiAccordion")
            return;
        const UiDesignerNodeId section = SelectedModelNodeId(data_list_.GetData());
        if(!section)
            return;
        const UiDesignerNode* current = ResolveAccordionSection(session_.Document(), section);
        if(!current)
            return;
        bool ok = false;
        if(id == "title")
            ok = session_.Commands().SetAccordionSectionTitle(section, current->GetProperty("title", String()));
        else if(id == "subtitle")
            ok = session_.Commands().SetAccordionSectionSubtitle(section, current->GetProperty("subtitle", String()));
        else if(id == "copy")
            ok = session_.Commands().SetAccordionSectionCopy(section, current->GetProperty("copy", String()));
        else if(id == "open")
            ok = session_.Commands().SetAccordionSectionOpen(section, current->GetProperty("open", false));
        else if(id == "lock")
            ok = session_.Commands().SetAccordionSectionLock(section, current->GetProperty("lock", "None"));
        if(!ok)
            RefreshStatus(session_.Commands().GetLastError());
        RefreshData();
    };
    data_list_.EnableRenameOnDblClick(true);
    data_list_.WhenSelection = [=] {
        if(!data_projection_refreshing_) {
            const Vector<int> selection = data_list_.GetSelection();
            if(!selection.IsEmpty() && selection[0] >= 0 &&
               selection[0] < data_model_.GetCount())
                data_selected_token_ = data_model_.Get(selection[0]).data;
            data_selection_refreshing_ = true;
            RefreshData();
            data_selection_refreshing_ = false;
        }
    };
    data_list_.WhenRename = [=](int index, const String& title) {
        if(index < 0 || index >= data_model_.GetCount()) return;
        const UiDesignerNodeId item_id = SelectedModelNodeId(data_model_.Get(index).data);
        const UiDesignerNode* node = session_.Document().Find(session_.State().selection.primary);
        if(node && node->type == "UiList") {
            ValueMap root = DesignerListRoot(*node);
            const int item_index = UiDesignerListDataAdapter::Index(data_model_.Get(index).data);
            ValueMap item = UiDesignerListDataAdapter::Item(root, item_index);
            if(!item.IsEmpty()) {
                item.Set("text", title);
                UiDesignerListDataAdapter::SetItem(root, item_index, item);
                if(!session_.Commands().SetData(node->id, "root", root,
                                                UiDesignerImpactStructure,
                                                "Rename UiList item"))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            RefreshData();
            return;
        }
        if(node && node->type == "UiTree") {
            ValueMap root = DesignerTreeRoot(*node);
            const ValueArray path = DesignerTreePath(data_model_.Get(index).data);
            ValueMap item = DesignerTreeItemAt(root, path);
            if(!item.IsEmpty()) {
                item.Set("text", title);
                DesignerTreeSetItemAt(root, path, item);
                if(!session_.Commands().SetData(node->id, "root", root,
                                                UiDesignerImpactStructure,
                                                "Rename UiTree item"))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            RefreshData();
            return;
        }
        const UiDesignerNode* owner = ResolveAccordionOwner(session_.Document(), node);
        if(owner && owner->type == "UiAccordion") {
            if(!session_.Commands().RenameAccordionSection(item_id, title))
                RefreshStatus(session_.Commands().GetLastError());
        }
        else if(node && node->type == "UiTab") {
            if(!session_.Commands().RenameTabPage(item_id, title))
                RefreshStatus(session_.Commands().GetLastError());
        }
        RefreshData();
    };
    auto SelectedDataNode = [=]() -> const UiDesignerNode* {
        return ResolveAccordionSection(session_.Document(), SelectedModelNodeId(data_list_.GetData()));
    };
    auto SelectedTabPage = [=]() -> UiDesignerNodeId {
        const Value v = data_list_.GetData();
        return IsNull(v) ? 0 : (UiDesignerNodeId)(int64)v;
    };
    auto PageIndex = [=](UiDesignerNodeId page) {
        const UiDesignerNode *p = session_.Document().Find(page);
        if(!p) return -1;
        const UiDesignerNode *parent = session_.Document().Find(p->parent);
        if(!parent) return -1;
        return FindChildIndex(*parent, page);
    };
    data_rename_.WhenAction = [=] {
        const UiDesignerNode *node = session_.Document().Find(session_.State().selection.primary);
        const UiDesignerNode *owner = ResolveAccordionOwner(session_.Document(), node);
        if(owner && owner->type == "UiAccordion") {
            const UiDesignerNode *section = SelectedDataNode();
            if(!section)
                return;
            String title = section->GetProperty("title", section->name);
            if(EditText(title, "Rename Accordion section", "Title") &&
               !session_.Commands().RenameAccordionSection(section->id, title))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            return;
        }
        const UiDesignerNodeId page = SelectedTabPage();
        const UiDesignerNode *page_node = session_.Document().Find(page);
        if(!page_node) return;
        String title = page_node->GetProperty("title", page_node->name);
        if(EditText(title, "Rename Tab page", "Title") &&
           !session_.Commands().RenameTabPage(page, title))
            RefreshStatus(session_.Commands().GetLastError());
        RefreshData();
    };
    data_add_.WhenAction = [=] {
        const UiDesignerNode *node = session_.Document().Find(session_.State().selection.primary);
        if(node && node->type == "UiList") {
            ValueMap root = DesignerListRoot(*node);
            ValueMap item;
            item.Set("text", "New item");
            item.Set("key", "item_" + AsString(UiDesignerListDataAdapter::Items(root).GetCount() + 1));
            item.Set("enabled", true);
            const bool appended = UiDesignerListDataAdapter::AppendItem(root, item);
            const int new_index = UiDesignerListDataAdapter::Items(root).GetCount() - 1;
            if(!appended || !session_.Commands().SetData(node->id, "root", root,
                                                         UiDesignerImpactStructure,
                                                         "Add UiList item"))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            if(appended)
                data_list_.SetData(UiDesignerListDataAdapter::Token(new_index));
            return;
        }
        if(node && node->type == "UiTree") {
            ValueMap root = DesignerTreeRoot(*node);
            ValueMap item;
            item.Set("text", "New item");
            item.Set("key", "item_1");
            item.Set("enabled", true);
            item.Set("editable", true);
            const ValueArray path = DesignerTreePath(
                SelectedDataToken(data_selected_token_, data_list_, data_model_));
            const bool appended = DesignerTreeAppendChild(root, path, item);
            const int new_index = DesignerTreeChildren(
                DesignerTreeItemAt(root, path)).GetCount() - 1;
            if(!appended)
                RefreshStatus("Selected UiTree item cannot contain children");
            if(appended && !session_.Commands().SetData(node->id, "root", root,
                                                        UiDesignerImpactStructure,
                                                        "Add UiTree item"))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            if(appended) {
                ValueArray new_path = path;
                new_path.Add(new_index);
                data_list_.SetData(UiDesignerTreeDataAdapter::Token(new_path));
            }
            return;
        }
        const UiDesignerNode *owner = ResolveAccordionOwner(session_.Document(), node);
        if(owner && owner->type == "UiAccordion") {
            if(!session_.Commands().AddAccordionSection(owner->id, "New Section"))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            return;
        }
        const UiDesignerNode *tab = session_.Document().Find(session_.State().selection.primary);
        if(tab && tab->type == "UiTab" && !session_.Commands().AddTabPage(tab->id, "New Page"))
            RefreshStatus(session_.Commands().GetLastError());
        RefreshData();
    };
    data_remove_.WhenAction = [=] {
        const UiDesignerNode *node = session_.Document().Find(session_.State().selection.primary);
        if(node && node->type == "UiList") {
            ValueMap root = DesignerListRoot(*node);
            const int index = UiDesignerListDataAdapter::Index(
                SelectedDataToken(data_selected_token_, data_list_, data_model_));
            if(index >= 0 && UiDesignerListDataAdapter::RemoveItem(root, index) &&
               !session_.Commands().SetData(node->id, "root", root,
                                            UiDesignerImpactStructure,
                                            "Remove UiList item"))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            return;
        }
        if(node && node->type == "UiTree") {
            ValueMap root = DesignerTreeRoot(*node);
            const ValueArray path = DesignerTreePath(
                SelectedDataToken(data_selected_token_, data_list_, data_model_));
            if(!path.IsEmpty()) {
                DesignerTreeRemoveItem(root, path);
                if(!session_.Commands().SetData(node->id, "root", root,
                                                UiDesignerImpactStructure,
                                                "Remove UiTree item"))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            RefreshData();
            return;
        }
        const UiDesignerNode *owner = ResolveAccordionOwner(session_.Document(), node);
        if(owner && owner->type == "UiAccordion") {
            const UiDesignerNode *section = SelectedDataNode();
            if(section && !session_.Commands().RemoveAccordionSection(section->id))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            return;
        }
        if(!session_.Commands().RemoveTabPage(SelectedTabPage()))
            RefreshStatus(session_.Commands().GetLastError());
        RefreshData();
    };
    data_select_content_.WhenAction = [=] {
        const UiDesignerNode* section = SelectedDataNode();
        if(section && !section->children.IsEmpty())
            session_.Select(section->children[0]);
        else
            RefreshStatus("Accordion section has no content");
    };
    data_remove_content_.WhenAction = [=] {
        const UiDesignerNode* section = SelectedDataNode();
        if(!section || section->children.IsEmpty())
            return;
        if(!session_.Commands().RemoveNode(section->children[0],
                                           "Remove Accordion section content"))
            RefreshStatus(session_.Commands().GetLastError());
        RefreshData();
    };
    data_up_.WhenAction = [=] {
        const UiDesignerNode *node = session_.Document().Find(session_.State().selection.primary);
        if(node && node->type == "UiList") {
            ValueMap root = DesignerListRoot(*node);
            const int index = UiDesignerListDataAdapter::Index(
                SelectedDataToken(data_selected_token_, data_list_, data_model_));
            if(UiDesignerListDataAdapter::MoveItem(root, index, -1) &&
               !session_.Commands().SetData(node->id, "root", root,
                                            UiDesignerImpactStructure,
                                            "Move UiList item up"))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            return;
        }
        if(node && node->type == "UiTree") {
            ValueMap root = DesignerTreeRoot(*node);
            const ValueArray path = DesignerTreePath(
                SelectedDataToken(data_selected_token_, data_list_, data_model_));
            if(!path.IsEmpty() && DesignerTreeMoveItem(root, path, -1)) {
                if(!session_.Commands().SetData(node->id, "root", root,
                                                UiDesignerImpactStructure,
                                                "Move UiTree item up"))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            RefreshData();
            return;
        }
        const UiDesignerNode *owner = ResolveAccordionOwner(session_.Document(), node);
        if(owner && owner->type == "UiAccordion") {
            const UiDesignerNode *section = SelectedDataNode();
            const int index = section ? FindChildIndex(*owner, section->id) : -1;
            if(section && index > 0 &&
               !session_.Commands().MoveAccordionSection(section->id, index - 1))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            return;
        }
        const UiDesignerNode *p = session_.Document().Find(SelectedTabPage());
        if(p && !session_.Commands().MoveTabPage(p->id, max(0, PageIndex(p->id)-1)))
            RefreshStatus(session_.Commands().GetLastError());
        RefreshData();
    };
    data_down_.WhenAction = [=] {
        const UiDesignerNode *node = session_.Document().Find(session_.State().selection.primary);
        if(node && node->type == "UiList") {
            ValueMap root = DesignerListRoot(*node);
            const int index = UiDesignerListDataAdapter::Index(
                SelectedDataToken(data_selected_token_, data_list_, data_model_));
            if(UiDesignerListDataAdapter::MoveItem(root, index, 1) &&
               !session_.Commands().SetData(node->id, "root", root,
                                            UiDesignerImpactStructure,
                                            "Move UiList item down"))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            return;
        }
        if(node && node->type == "UiTree") {
            ValueMap root = DesignerTreeRoot(*node);
            const ValueArray path = DesignerTreePath(
                SelectedDataToken(data_selected_token_, data_list_, data_model_));
            if(!path.IsEmpty() && DesignerTreeMoveItem(root, path, 1)) {
                if(!session_.Commands().SetData(node->id, "root", root,
                                                UiDesignerImpactStructure,
                                                "Move UiTree item down"))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            RefreshData();
            return;
        }
        const UiDesignerNode *owner = ResolveAccordionOwner(session_.Document(), node);
        if(owner && owner->type == "UiAccordion") {
            const UiDesignerNode *section = SelectedDataNode();
            const int index = section ? FindChildIndex(*owner, section->id) : -1;
            if(section && index >= 0 && index + 1 < owner->children.GetCount() &&
               !session_.Commands().MoveAccordionSection(section->id, index + 1))
                RefreshStatus(session_.Commands().GetLastError());
            RefreshData();
            return;
        }
        const UiDesignerNode *p = session_.Document().Find(SelectedTabPage());
        if(p && !session_.Commands().MoveTabPage(p->id, PageIndex(p->id)+1))
            RefreshStatus(session_.Commands().GetLastError());
        RefreshData();
    };
    data_enable_.WhenAction = [=] {
        const UiDesignerNode *node = session_.Document().Find(session_.State().selection.primary);
        if(node && node->type == "UiList") {
            ValueMap root = DesignerListRoot(*node);
            const int index = UiDesignerListDataAdapter::Index(
                SelectedDataToken(data_selected_token_, data_list_, data_model_));
            ValueMap item = UiDesignerListDataAdapter::Item(root, index);
            if(!item.IsEmpty()) {
                item.Set("enabled", !(bool)UiDesignerMapValue(item, "enabled", true));
                UiDesignerListDataAdapter::SetItem(root, index, item);
                if(!session_.Commands().SetData(node->id, "root", root,
                                                UiDesignerImpactStructure,
                                                "Toggle UiList item"))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            RefreshData();
            return;
        }
        if(node && node->type == "UiTree") {
            ValueMap root = DesignerTreeRoot(*node);
            const ValueArray path = DesignerTreePath(
                SelectedDataToken(data_selected_token_, data_list_, data_model_));
            ValueMap item = DesignerTreeItemAt(root, path);
            if(!item.IsEmpty()) {
                item.Set("enabled", !(bool)UiDesignerMapValue(item, "enabled", true));
                DesignerTreeSetItemAt(root, path, item);
                if(!session_.Commands().SetData(node->id, "root", root,
                                                UiDesignerImpactStructure,
                                                "Toggle UiTree item"))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            RefreshData();
            return;
        }
        const UiDesignerNode *owner = ResolveAccordionOwner(session_.Document(), node);
        if(owner && owner->type == "UiAccordion") {
            const UiDesignerNode *section = SelectedDataNode();
            if(!section)
                return;
            const String lock = section->GetProperty("lock", "None");
            if(lock == "Open") {
                if(!session_.Commands().SetAccordionSectionOpen(section->id, false))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            else if(lock == "Closed") {
                if(!session_.Commands().SetAccordionSectionOpen(section->id, true))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            else {
                if(!session_.Commands().SetAccordionSectionOpen(section->id, !section->GetProperty("open", false)))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            RefreshData();
            return;
        }
        const UiDesignerNode *p = session_.Document().Find(SelectedTabPage());
        if(p && !session_.Commands().SetTabPageEnabled(p->id, !p->GetProperty("enabled", true)))
            RefreshStatus(session_.Commands().GetLastError());
        RefreshData();
    };
    data_active_.WhenAction = [=] {
        const UiDesignerNode *node = session_.Document().Find(session_.State().selection.primary);
        const UiDesignerNode *owner = ResolveAccordionOwner(session_.Document(), node);
        if(owner && owner->type == "UiAccordion") {
            const UiDesignerNode *section = SelectedDataNode();
            if(!section)
                return;
            const String lock = section->GetProperty("lock", "None");
            const bool open = section->GetProperty("open", false);
            if(lock == "None") {
                if(!session_.Commands().SetAccordionSectionLock(section->id, open ? "Open" : "Closed"))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            else {
                if(!session_.Commands().SetAccordionSectionLock(section->id, "None"))
                    RefreshStatus(session_.Commands().GetLastError());
            }
            RefreshData();
            return;
        }
        const UiDesignerNodeId tab = session_.State().selection.primary;
        const UiDesignerNodeId page = SelectedTabPage();
        if(page && !session_.Commands().SetActiveTabPage(tab, page))
            RefreshStatus(session_.Commands().GetLastError());
        RefreshData();
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
        RefreshHierarchy(); RefreshInspector(); RefreshData();
        RefreshCode(); RefreshBehavior();
        overrides_.Refresh();
        RequestDiagnosticsRefresh();
    };
    session_.Theme().WhenChanged = [=] {
        ApplyThemeToShell(); RefreshThemeInspector(); RefreshCode();
        RequestDiagnosticsRefresh();
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
        CancelCatalogDrag();
        return;
    }
    const CatalogDragDestination destination = ResolveCatalogDragDestination(screen);
    SetCatalogDragDestination(destination);
    if(destination == CatalogDragDestination::Hierarchy)
        hierarchy_.TrackCatalogDrop(type_id, screen);
    else if(destination == CatalogDragDestination::Canvas)
        interaction_overlay_.TrackCatalogDrag(type_id, screen);
}

void UiDesignerWindow::FinishCatalogDrag(const String& type_id, Point screen)
{
    const String drag_type = active_catalog_drag_type_.IsEmpty() ? type_id
                                                                 : active_catalog_drag_type_;
    const CatalogDragDestination destination = ResolveCatalogDragDestination(screen);
    SetCatalogDragDestination(destination);
    catalog_drag_active_ = false;
    active_catalog_drag_type_.Clear();
    catalog_drag_destination_ = CatalogDragDestination::None;
    if(destination == CatalogDragDestination::Hierarchy)
        hierarchy_.FinishCatalogDrop(drag_type, screen);
    else if(destination == CatalogDragDestination::Canvas)
        interaction_overlay_.FinishCatalogDrag(drag_type, screen);
}

void UiDesignerWindow::CancelCatalogDrag()
{
    catalog_drag_active_ = false;
    active_catalog_drag_type_.Clear();
    SetCatalogDragDestination(CatalogDragDestination::None);
}

UiDesignerWindow::CatalogDragDestination UiDesignerWindow::ResolveCatalogDragDestination(
    Point screen) const
{
    if(hierarchy_.GetScreenRect().Contains(screen))
        return CatalogDragDestination::Hierarchy;
    if(interaction_overlay_.GetScreenRect().Contains(screen))
        return CatalogDragDestination::Canvas;
    return CatalogDragDestination::None;
}

void UiDesignerWindow::SetCatalogDragDestination(CatalogDragDestination destination)
{
    if(catalog_drag_destination_ == destination)
        return;
    if(catalog_drag_destination_ == CatalogDragDestination::Hierarchy)
        hierarchy_.CancelCatalogDrop();
    else if(catalog_drag_destination_ == CatalogDragDestination::Canvas)
        interaction_overlay_.CancelCatalogDrag();
    catalog_drag_destination_ = destination;
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
    RefreshHierarchy(); RefreshInspector(); RefreshData(); RefreshBehavior(); RefreshCode();
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
    preview_canvas_.BumpHierarchyRefresh();
    hierarchy_.SetSelection(&session_.State().selection);
    hierarchy_.Rebuild();
}

void UiDesignerWindow::RefreshInspector()
{
    preview_canvas_.BumpPropertyEditorRefresh();
    PropertyEditorModel& model = session_.InspectorModel();
    model.ClearGroupSubtitles();
    const UiDesignerSelection& selection = session_.State().selection;
    const UiDesignerNode* primary = session_.Document().Find(selection.primary);
    if(primary) {
        String identity;
        if(selection.nodes.GetCount() > 1) {
            bool same_type = true;
            for(UiDesignerNodeId id : selection.nodes) {
                const UiDesignerNode* node = session_.Document().Find(id);
                if(!node || node->type != primary->type) {
                    same_type = false;
                    break;
                }
            }
            if(same_type) {
                const UiDesignerControlSpec* spec = session_.Catalog().Find(primary->type);
                identity = (spec ? spec->display_name : primary->type) +
                           Format(" - %d selected", selection.nodes.GetCount());
            }
            else
                identity = Format("%d mixed controls", selection.nodes.GetCount());
        }
        else if(primary->id != session_.Document().GetRootId()) {
            const UiDesignerControlSpec* spec = session_.Catalog().Find(primary->type);
            identity = spec ? spec->display_name : primary->type;
        }

        if(!identity.IsEmpty())
            model.SetGroupSubtitle("Identity", identity);

        Rect resolved = preview_canvas_.GetNodeRect(primary->id);
        Size size = resolved.IsEmpty() ? Size(0, 0) : resolved.GetSize();
        if(primary->id == session_.Document().GetRootId())
            size = preview_canvas_.GetEffectiveVirtualSize();
        const String layout = size.cx > 0 && size.cy > 0
            ? Format("%d x %d px", size.cx, size.cy)
            : "Size unavailable";
        model.SetGroupSubtitle("Layout", layout);
    }
    inspector_.SetModel(&model);
    inspector_.Refresh();
}

void UiDesignerWindow::RefreshData()
{
    if(data_projection_refreshing_)
        return;
    data_projection_refreshing_ = true;
    const UiDesignerNodeId selected = session_.State().selection.primary;
    const UiDesignerNode* node = selected ? session_.Document().Find(selected) : nullptr;
    const UiDesignerControlSpec* node_spec = node ? session_.Catalog().Find(node->type) : nullptr;
    const UiDesignerNode* accordion_owner = ResolveAccordionOwner(session_.Document(), node);
    const UiDesignerControlSpec* accordion_spec = accordion_owner
        ? session_.Catalog().Find(accordion_owner->type) : nullptr;
    const Value data_selection = SelectedDataToken(data_selected_token_,
                                                   data_list_, data_model_);
    const Vector<int> data_rows = data_list_.GetSelection();
    Value selected_data = data_selection;
    if(!data_rows.IsEmpty() && data_rows[0] >= 0 &&
       data_rows[0] < data_model_.GetCount())
        selected_data = data_model_.Get(data_rows[0]).data;
    const auto Finish = [&] {
        data_projection_refreshing_ = false;
    };
    data_model_.Clear();
    data_editor_model_.Clear();
    data_select_content_.Enable(false);
    data_remove_content_.Enable(false);
    if(node_spec && node_spec->data_capability == UiDesignerDataCapability::List) {
        const ValueMap root = DesignerListRoot(*node);
        const Vector<UiDesignerListDataRow> rows =
            UiDesignerListDataAdapter::Rows(root);
        for(const UiDesignerListDataRow& row : rows)
            data_model_.Add(row.text, UiDesignerListDataAdapter::Token(row.index),
                            row.enabled);
        const Value prior = selected_data;
        bool keep = false;
        for(int i = 0; i < data_model_.GetCount(); i++)
            keep |= data_model_.Get(i).data == prior;
        if(!data_selection_refreshing_)
            data_list_.SetData(keep ? prior : data_model_.GetCount()
                ? data_model_.Get(0).data : Value());
        const Value selected_token = data_selection_refreshing_
            ? prior : (keep ? prior : data_model_.GetCount()
                ? data_model_.Get(0).data : Value());
        const int index = UiDesignerListDataAdapter::Index(selected_token);
        data_selected_token_ = selected_token;
        const ValueMap item = UiDesignerListDataAdapter::Item(root, index);
        data_editor_model_.Add("text", "Text", PropertyEditorKind::Text,
                               UiDesignerMapValue(item, "text", ""), "Item");
        data_editor_model_.Add("key", "Key", PropertyEditorKind::Text,
                               UiDesignerMapValue(item, "key", ""), "Item");
        data_editor_model_.Add("value", "Value", PropertyEditorKind::Text,
                               UiDesignerMapValue(item, "data", ""), "Item");
        data_editor_model_.Add("description", "Description", PropertyEditorKind::Text,
                               UiDesignerMapValue(item, "description", ""), "Item");
        data_editor_model_.Add("right_text", "Right text", PropertyEditorKind::Text,
                               UiDesignerMapValue(item, "right_text", ""), "Item");
        data_editor_model_.Add("enabled", "Enabled", PropertyEditorKind::Boolean,
                               UiDesignerMapValue(item, "enabled", true), "Item");
        data_editor_model_.Add("checked", "Checked", PropertyEditorKind::Boolean,
                               UiDesignerMapValue(item, "checked", false), "Item");
        data_add_.Enable(true);
        data_remove_.Enable(index >= 0);
        data_rename_.Enable(index >= 0);
        data_up_.Enable(index > 0);
        data_down_.Enable(index >= 0 && index + 1 < data_model_.GetCount());
        data_enable_.Enable(index >= 0);
        data_active_.Enable(false);
        data_enable_.SetText(index >= 0 &&
            (bool)UiDesignerMapValue(item, "enabled", true) ? "Disable" : "Enable");
        data_editor_.SetModel(&data_editor_model_);
        data_editor_model_.StructureChanged();
        Finish();
        return;
    }
    if(node_spec && node_spec->data_capability == UiDesignerDataCapability::Tree) {
        const Value prior = selected_data;
        const ValueMap root = DesignerTreeRoot(*node);
        const Vector<UiDesignerTreeDataRow> rows =
            UiDesignerTreeDataAdapter::Rows(root);
        for(const UiDesignerTreeDataRow& row : rows)
            data_model_.Add(String(' ', row.depth * 2) + row.text,
                            UiDesignerTreeDataAdapter::Token(row.path), row.enabled);
        const ValueArray root_path;
        bool keep = false;
        for(int i = 0; i < data_model_.GetCount(); i++)
            keep |= data_model_.Get(i).data == prior;
        if(!data_selection_refreshing_)
            data_list_.SetData(keep ? prior : UiDesignerTreeDataAdapter::Token(root_path));
        const Value tree_selection = data_selection_refreshing_
            ? prior : (keep ? prior : UiDesignerTreeDataAdapter::Token(root_path));
        data_selected_token_ = tree_selection;
        const ValueArray path = DesignerTreePath(tree_selection);
        const ValueMap selected_item = DesignerTreeItemAt(root, path);
        const ValueArray parent_path = ParentTreePath(path);
        const ValueMap parent = DesignerTreeItemAt(root, parent_path);
        const ValueArray siblings = DesignerTreeChildren(parent);
        const int index = path.IsEmpty() ? -1 :
            (int)(int64)path[path.GetCount() - 1];
        data_editor_model_.Add("text", "Text", PropertyEditorKind::Text,
                               UiDesignerMapValue(selected_item, "text", ""), "Item");
        if(!path.IsEmpty()) {
            data_editor_model_.Add("key", "Key", PropertyEditorKind::Text,
                                   UiDesignerMapValue(selected_item, "key", ""), "Item");
            data_editor_model_.Add("value", "Value", PropertyEditorKind::Text,
                                   UiDesignerMapValue(selected_item, "data", ""), "Item");
            data_editor_model_.Add("enabled", "Enabled", PropertyEditorKind::Boolean,
                                   UiDesignerMapValue(selected_item, "enabled", true), "Item");
            data_editor_model_.Add("editable", "Editable", PropertyEditorKind::Boolean,
                                   UiDesignerMapValue(selected_item, "editable", false), "Item");
        }
        data_add_.Enable(true);
        data_remove_.Enable(!path.IsEmpty());
        data_rename_.Enable(!path.IsEmpty());
        data_up_.Enable(index > 0);
        data_down_.Enable(index >= 0 && index + 1 < siblings.GetCount());
        data_enable_.Enable(!path.IsEmpty());
        data_active_.Enable(false);
        data_select_content_.Enable(false);
        data_remove_content_.Enable(false);
        data_enable_.SetText(!path.IsEmpty() &&
            (bool)UiDesignerMapValue(selected_item, "enabled", true) ? "Disable" : "Enable");
        data_editor_.SetModel(&data_editor_model_);
        data_editor_model_.StructureChanged();
        Finish();
        return;
    }
    if(node_spec && node_spec->data_capability == UiDesignerDataCapability::Pages) {
        const Value prior = data_list_.GetData();
        for(UiDesignerNodeId id : node->children) {
            const UiDesignerNode *page = session_.Document().Find(id);
            if(page && page->type == "UiTabPage")
                data_model_.Add(AsString(page->GetProperty("title", page->name)),
                                page->id, page->GetProperty("enabled", true));
        }
        bool keep = false;
        for(int i = 0; i < data_model_.GetCount(); i++)
            keep |= data_model_.Get(i).data == prior;
        data_list_.SetData(keep ? prior : data_model_.GetCount() ? data_model_.Get(0).data : Value());
        data_add_.Enable(true);
        data_remove_.Enable(data_model_.GetCount() > 1 && !IsNull(data_list_.GetData()));
        data_rename_.Enable(!IsNull(data_list_.GetData()));
        int selected = -1;
        for(int i = 0; i < data_model_.GetCount(); i++) if(data_model_.Get(i).data == data_list_.GetData()) selected = i;
        data_up_.Enable(selected > 0);
        data_down_.Enable(selected >= 0 && selected + 1 < data_model_.GetCount());
        data_enable_.Enable(selected >= 0);
        data_active_.Enable(!IsNull(data_list_.GetData()) && data_list_.GetData() != node->GetProperty("active_page", Value()));
        data_enable_.SetText(selected >= 0 && data_model_.Get(selected).enabled ? "Disable" : "Enable");
        data_editor_.SetModel(&data_editor_model_);
        data_editor_model_.StructureChanged();
        Finish();
        return;
    }
    if(accordion_owner && accordion_spec &&
       accordion_spec->data_capability == UiDesignerDataCapability::AccordionSections) {
        const Value prior = data_list_.GetData();
        for(UiDesignerNodeId id : accordion_owner->children) {
            const UiDesignerNode *section = session_.Document().Find(id);
            if(section && section->type == "UiAccordionSection")
                data_model_.Add(AsString(section->GetProperty("title", section->name)),
                                section->id, section->GetProperty("open", false));
        }
        bool keep = false;
        for(int i = 0; i < data_model_.GetCount(); i++)
            keep |= data_model_.Get(i).data == prior;
        if(!keep && data_model_.GetCount() > 0)
            data_list_.SetData(data_model_.Get(0).data);
        else
            data_list_.SetData(prior);

        const UiDesignerNode* section = ResolveAccordionSection(session_.Document(), SelectedModelNodeId(data_list_.GetData()));
        const int index = section ? FindChildIndex(*accordion_owner, section->id) : -1;
        const String lock = section ? AsString(section->GetProperty("lock", "None")) : "None";
        const bool open = section ? (bool)section->GetProperty("open", false) : false;
        data_add_.Enable(true);
        data_remove_.Enable(section && accordion_owner->children.GetCount() > 1);
        data_rename_.Enable(section != nullptr);
        data_up_.Enable(section && index > 0);
        data_down_.Enable(section && index >= 0 && index + 1 < accordion_owner->children.GetCount());
        data_enable_.Enable(section && lock != "Open" && lock != "Closed");
        data_active_.Enable(section != nullptr);
        data_enable_.SetText(open ? "Close" : "Open");
        data_active_.SetText(lock == "None"
            ? (open ? "Lock Open" : "Lock Closed")
            : "Unlock");

        if(section) {
            data_editor_model_.Add("title", "Title", PropertyEditorKind::Text,
                                   section->GetProperty("title", section->name),
                                   "Section");
            data_editor_model_.Add("subtitle", "Subtitle", PropertyEditorKind::Text,
                                   section->GetProperty("subtitle", String()),
                                   "Section");
            data_editor_model_.Add("copy", "Copy", PropertyEditorKind::Text,
                                   section->GetProperty("copy", String()),
                                   "Section");
            data_editor_model_.Add("open", "Open", PropertyEditorKind::Boolean,
                                   section->GetProperty("open", false),
                                   "Section");
            PropertyEditorItem& lock_item = data_editor_model_.Add(
                "lock", "Lock", PropertyEditorKind::Choice,
                section->GetProperty("lock", "None"), "Section");
            lock_item.choices.Add(PropertyEditorChoice("None", "None"));
            lock_item.choices.Add(PropertyEditorChoice("Open", "Open"));
            lock_item.choices.Add(PropertyEditorChoice("Closed", "Closed"));
            const UiDesignerNode* content = section->children.IsEmpty()
                ? nullptr : session_.Document().Find(section->children[0]);
            data_model_.Add(content
                ? "Content: " + content->name + " [" + content->type + "]"
                : "Content: Empty", Value(), false);
            data_select_content_.Enable(content != nullptr);
            data_remove_content_.Enable(content != nullptr);
        }
        data_editor_model_.StructureChanged();
        data_editor_.SetModel(&data_editor_model_);
        Finish();
        return;
    }
    String data_status = "Select a control to view data";
    if(node) {
        if(node_spec && node_spec->data_capability == UiDesignerDataCapability::Pages)
            data_status = "Select a page or semantic owner to edit page data.";
        else if(node_spec && node_spec->data_capability == UiDesignerDataCapability::AccordionSections)
            data_status = "Select an Accordion section to edit section data.";
        else if(node_spec && node_spec->data_capability == UiDesignerDataCapability::Unsupported)
            data_status = "This control has no stable serializable Designer data contract.";
        else data_status = "Data is not supported for this control yet.";
    }
    data_model_.Add(data_status,
                    Value(), true);
    data_add_.Enable(false); data_remove_.Enable(false); data_rename_.Enable(false);
    data_up_.Enable(false); data_down_.Enable(false); data_enable_.Enable(false); data_active_.Enable(false);
    data_enable_.SetText("Enable");
    data_editor_.SetModel(&data_editor_model_);
    data_editor_model_.StructureChanged();
    Finish();
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
    preview_canvas_.BumpCodeRefresh();
    const String header = session_.GenerateHeader("GeneratedUiWindow");
    const String source = session_.GenerateCode("GeneratedUiWindow");
    if(header.IsEmpty() && source.IsEmpty()) {
        const UiDesignerNodeId selected = session_.State().selection.primary;
        const UiDesignerNode* node = selected ? session_.Document().Find(selected) : nullptr;
        code_.SetCode(Format("Code generation unavailable for selected control: %s",
                             node ? node->type : String("none")));
        return;
    }
    code_.SetCode(header + "\n" + source);
}

void UiDesignerWindow::RefreshDiagnostics()
{
    const UiDesignerPreviewStats& stats = preview_canvas_.GetStats();
    const UiDesignerDocument& document = session_.Document();
    const UiDesignerNodeId selected = session_.State().selection.primary;
    const UiDesignerNode* node = selected ? document.Find(selected) : nullptr;
    const UiDesignerNode* parent = node && node->parent ? document.Find(node->parent) : nullptr;
    const UiDesignerControlSpec* spec = node ? session_.Catalog().Find(node->type) : nullptr;
    const auto FmtMs = [](double ms) {
        return ms >= 0 ? Format("%.3f ms", ms) : String("unavailable");
    };
    String out;
    out << "LIVE PREVIEW\n";
    out << "  document nodes: " << document.GetNodes().GetCount() << "\n";
    out << "  live instances: " << preview_canvas_.GetLiveInstanceCount() << "\n";
    out << "  preview layout calls: " << stats.layout_count << "\n";
    out << "  grid layout passes: " << stats.grid_layout_passes << "\n";
    out << "  box layout passes: " << stats.box_layout_passes << "\n";
    out << "  absolute updates: " << stats.absolute_layout_updates << "\n";
    out << "  live instance creations: " << stats.live_instance_creations << "\n";
    out << "  live instance destructions: " << stats.live_instance_destructions << "\n";
    out << "  cached grid geometry publications: " << stats.cached_grid_geometry_publications << "\n";
    out << "  cached grid geometry reads: " << stats.cached_grid_geometry_reads << "\n";
    out << "  full rebuilds: " << stats.full_rebuilds << "\n";
    out << "  subtree rebuilds: " << stats.subtree_rebuilds << "\n";
    out << "  direct applies: " << stats.live_applies << "\n";
    out << "  layout-item updates: " << stats.layout_item_updates << "\n";
    out << "  deferred batches: " << stats.deferred_batches << "\n\n";
    out << "  timing: " << (preview_canvas_.IsDetailedTimingEnabled() ? "enabled" : "disabled") << "\n";
    out << "  capture: " << (preview_canvas_.IsCapturePaused() ? "paused" : "running") << "\n\n";
    out << "LATEST RESIZE\n";
    const UiDesignerResizeHistory& history = preview_canvas_.GetResizeHistory();
    if(!history.IsEmpty()) {
        const UiDesignerResizeSample& sample = history.GetLatest();
        if(!sample.timing_enabled)
            out << "  state: timing disabled\n";
        else if(sample.paint_complete)
            out << "  state: complete sample\n";
        else
            out << "  state: incomplete asynchronous paint data\n";
        out << "  sequence: " << (int64)sample.sequence << "\n";
        out << "  total event: " << FmtMs(sample.total_ms) << "\n";
        out << "  window resize: " << FmtMs(sample.window_resize_ms) << "\n";
        out << "  preview update: " << FmtMs(sample.immediate_preview_ms) << "\n";
        out << "  grid layout: " << FmtMs(sample.grid_layout_ms) << "\n";
        out << "  box layout: " << FmtMs(sample.box_layout_ms) << "\n";
        out << "  geometry walk: " << FmtMs(sample.geometry_walk_ms) << "\n";
        out << "  geometry snapshot: " << FmtMs(sample.snapshot_ms) << "\n";
        out << "  overlay paint: " << FmtMs(sample.overlay_paint_ms) << "\n";
        out << "  canvas paint: " << FmtMs(sample.canvas_paint_ms) << "\n";
        out << "  inspector: " << FmtMs(sample.inspector_ms) << "\n";
        out << "  code: " << FmtMs(sample.code_ms) << "\n";
        out << "  counters: grid=" << sample.grid_layout_passes
            << " box=" << sample.box_layout_passes
            << " layout=" << sample.preview_layout_calls
            << " snapshot=" << sample.geometry_snapshot_publications
            << " repaint=" << sample.overlay_only_repaints
            << "/" << sample.full_canvas_repaints << "\n";
    }
    else {
        out << "  state: no sample\n";
        out << "  total event: unavailable\n";
        out << "  window resize: unavailable\n";
        out << "  preview update: unavailable\n";
        out << "  grid layout: unavailable\n";
        out << "  box layout: unavailable\n";
        out << "  geometry walk: unavailable\n";
        out << "  geometry snapshot: unavailable\n";
        out << "  overlay paint: unavailable\n";
        out << "  canvas paint: unavailable\n";
        out << "  inspector: unavailable\n";
        out << "  code: unavailable\n";
    }
    out << "\n";
    out << "RECENT PERFORMANCE\n";
    if(history.IsEmpty()) {
        out << "  resize fps: not available\n";
        out << "  avg frame: not available\n";
        out << "  max frame: not available\n";
        out << "  >16.7 ms: not available\n";
        out << "  >33.3 ms: not available\n";
        out << "  >66.7 ms: not available\n";
    }
    else {
        out << "  resize fps: " << Format("%.1f", history.GetEstimatedFps()) << "\n";
        out << "  avg frame: " << FmtMs(history.GetRecentAverageDuration()) << "\n";
        out << "  max frame: " << FmtMs(history.GetRecentMaximumDuration()) << "\n";
        out << "  >16.7 ms: " << history.GetFramesAbove(17) << "\n";
        out << "  >33.3 ms: " << history.GetFramesAbove(34) << "\n";
        out << "  >66.7 ms: " << history.GetFramesAbove(67) << "\n";
    }
    out << "\n";
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
        out << "  latest direct-apply result: not available\n";
    }
    else
        out << "  none\n";
    diagnostics_shell_.SetData(out);
}

void UiDesignerWindow::RequestDiagnosticsRefresh()
{
    if(diagnostics_refresh_posted_)
        return;
    diagnostics_refresh_posted_ = true;
    Ptr<UiDesignerWindow> self = this;
    PostCallback([self] {
        if(!self)
            return;
        self->diagnostics_refresh_posted_ = false;
        if(self->diagnostics_capture_paused_)
            return;
        self->RefreshDiagnostics();
    });
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
        self->preview_canvas_.BumpDeferredBatch();
        self->RefreshBehavior();
        self->RefreshData();
        self->RefreshCode();
        self->RequestDiagnosticsRefresh();
    });
}

void UiDesignerWindow::RefreshStatus(const String& status)
{
    footer_.SetText(status.IsEmpty() ? "Ready" : status);
}

bool UiDesignerWindow::Key(dword key, int count)
{
    if(key == K_CTRL_Z) {
        if(session_.Undo()) {
            RefreshStatus("Undo");
            return true;
        }
        return false;
    }
    if(key == K_CTRL_Y) {
        if(session_.Redo()) {
            RefreshStatus("Redo");
            return true;
        }
        return false;
    }
    return TopWindow::Key(key, count);
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

void UiDesignerWindow::ApplyPreviewVirtualSize(const Size& virtual_size)
{
    const Size size(max(1, virtual_size.cx), max(1, virtual_size.cy));
    const Size workspace_size = preview_scroll_.GetSize();
    const int preview_margin = DPI(40);
    const Size extent(max(workspace_size.cx, size.cx + preview_margin * 2),
                      max(workspace_size.cy, size.cy + preview_margin * 2));
    preview_workspace_.SetRect(0, 0, extent.cx, extent.cy);
    preview_canvas_.SetRect((extent.cx - size.cx) / 2,
                            (extent.cy - size.cy) / 2, size.cx, size.cy);
    interaction_overlay_.SetRect(0, 0, extent.cx, extent.cy);
    preview_canvas_.Refresh();
    interaction_overlay_.Refresh();
}

void UiDesignerWindow::QueuePreviewVirtualSize(const Size& virtual_size,
                                                Function<void()> applied)
{
    pending_preview_virtual_size_ = virtual_size;
    pending_preview_resize_callback_ = pick(applied);
    preview_resize_pending_ = true;
    if(preview_resize_posted_)
        return;
    preview_resize_posted_ = true;
    Ptr<UiDesignerWindow> self = this;
    PostCallback([self] {
        if(!self)
            return;
        self->preview_resize_posted_ = false;
        self->FlushPreviewVirtualSize();
    });
}

void UiDesignerWindow::FlushPreviewVirtualSize()
{
    if(!preview_resize_pending_)
        return;
    preview_resize_pending_ = false;
    const Size size = pending_preview_virtual_size_;
    Function<void()> callback = pick(pending_preview_resize_callback_);
    preview_canvas_.SetTransientVirtualSize(size);
    ApplyPreviewVirtualSize(size);
    preview_canvas_.Layout();
    if(callback)
        callback();
    preview_canvas_.BumpDeferredBatch();
    RequestDiagnosticsRefresh();
}

void UiDesignerWindow::CancelQueuedPreviewVirtualSize()
{
    preview_resize_pending_ = false;
    pending_preview_resize_callback_.Clear();
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
    const Size virtual_size = preview_canvas_.GetEffectiveVirtualSize();
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
