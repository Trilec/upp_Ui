#include <Utilities/UiDesigner/Services/UiDesignerServices.h>
#include <Utilities/UiDesigner/Preview/UiDesignerPreview.h>
#include <Ui/UiAbsoluteLayout.h>

using namespace Upp;

static int checks = 0;
static int fails = 0;

static void Check(bool condition, const String& message)
{
    checks++;
    if(!condition) {
        fails++;
        Cout() << "FAIL: " << message << "\n";
    }
}

CONSOLE_APP_MAIN
{
    UiDesignerCatalog catalog;
    RegisterUiDesignerBuiltins(catalog);

    String error;
    Check(catalog.Validate(error), "catalog validates: " + error);
    Check(catalog.GetCount() >= 50, "complete native and U++ catalog");
    Check(catalog.FindCategory("Layouts").GetCount() >= 5, "layout catalog");
    Check(catalog.FindCategory("Containers").GetCount() >= 8, "container catalog");
    Check(catalog.FindCategory("Ui Controls").GetCount() >= 20, "Ui control catalog");
    Check(catalog.FindCategory("Composites").GetCount() >= 6, "composite catalog");
    Check(catalog.FindCategory("U++ Controls").GetCount() >= 18, "stock U++ catalog");
    Check(catalog.GetPresets().GetCount() >= 3, "preset catalog");

    UiDesignerDocument blank_preview_document;
    UiDesignerPreviewCanvas blank_preview;
    blank_preview.SetRect(0, 0, 512, 250);
    UiDesignerSelection blank_selection;
    blank_preview.Bind(&blank_preview_document, &catalog, nullptr, &blank_selection);
    blank_preview.RebuildDocument();
    const UiDesignerGeometrySnapshot& blank_geometry = blank_preview.GetGeometrySnapshot();
    const UiDesignerNodeId blank_root = blank_preview_document.GetRootId();
    Check(blank_selection.nodes.IsEmpty() && blank_selection.primary == 0,
          "blank preview starts without a selected child");
    const UiDesignerGeometryRecord* blank_root_geometry = blank_geometry.Find(blank_root);
    Check(blank_root_geometry && blank_root_geometry->cue_kind == UiDesignerCueKind::ContainerBounds,
          "blank root publishes a container cue");
    Check(blank_geometry.GetDropRegionCount() == 1,
          Format("blank document publishes one root drop region (%d)",
                 blank_geometry.GetDropRegionCount()));
    const UiDesignerDropRegion* blank_root_drop =
        blank_geometry.HitDropRegion(blank_preview.GetNodeRect(blank_root).CenterPoint());
    Check(blank_root_drop && blank_root_drop->owner == blank_root &&
              blank_root_drop->kind == UiDesignerDropRegionKind::WindowContent,
          "blank root hit testing resolves the Window region");

    static const char *required_ui[] = {
        "UiLabel", "UiCheckBox", "UiRadioButton", "UiToggle", "UiPanel",
        "UiDirectContentHost", "UiGroupPanel", "UiStack", "UiAccordion",
        "UiScrollPanel", "UiTab", "UiTitleCard", "UiGridLayout", "UiBoxLayout",
        "UiAbsoluteLayout",
        "UiButton", "UiToolButton", "UiSplitButton", "UiLineEdit", "UiIntEdit",
        "UiFloatEdit", "UiPasswordEdit", "UiMultiEdit", "UiMaskEdit",
        "UiProgressBar", "UiSlider", "UiBreadcrumbs", "UiSliderEdit",
        "UiScrollBar", "UiSplitter", "UiQuadSplitter", "UiTable", "UiDoc",
        "UiTree", "UiList", "UiBezierCurveEditor", "UiBezierCurveField",
        "UiDropdown", "UiMenu", "UiColorPicker", "UiCompositeSlider",
        "UiCompositeToggle", "UiCompositeColor", "UiCompositeDropdown",
        "UiCompositeLabel", "UiCompositeEdit"
    };
    for(int i = 0; i < __countof(required_ui); i++)
        Check(catalog.Find(required_ui[i]) != nullptr,
              String("catalog includes ") + required_ui[i]);

    const UiDesignerControlSpec* absolute = catalog.Find("UiAbsoluteLayout");
    Check(absolute && absolute->child_adapter_id == "absolute",
          "absolute layout has an exact-rect child adapter");
    Check(absolute && HasUiDesignerCapability(
              absolute->capabilities, UiDesignerCapabilityFreeform),
          "absolute layout accepts freeform Designer placement");
    Check(absolute && absolute->FindProperty("x") &&
              absolute->FindProperty("y") &&
              absolute->FindProperty("width") &&
              absolute->FindProperty("height"),
          "absolute layout exposes Inspector geometry bindings");
    One<Ctrl> absolute_preview;
    if(absolute)
        absolute_preview = UiDesignerPreviewFactory::Create(*absolute);
    Check(absolute_preview &&
              dynamic_cast<UiAbsoluteLayout *>(absolute_preview.Get()),
          "absolute layout preview creates the runtime control");

    UiDesignerSession drop_session;
    drop_session.NewDocument("blank");
    const UiDesignerNodeId root = drop_session.Document().GetRootId();
    UiDesignerDropPlan panel_plan =
        drop_session.PlanAddControl("UiPanel", root, Point(64, 48), true);
    Check(panel_plan.valid && panel_plan.parent == root,
          "root window accepts Panel drops");
    Check(panel_plan.has_canvas_position &&
              panel_plan.add_defaults.Find("x") < 0 &&
              panel_plan.add_defaults.Find("y") < 0 &&
              panel_plan.add_defaults.GetValue(panel_plan.add_defaults.Find("width_mode")) == "Expand" &&
              panel_plan.add_defaults.GetValue(panel_plan.add_defaults.Find("height_mode")) == "Expand" &&
              panel_plan.add_defaults.GetValue(panel_plan.add_defaults.Find("cell_align_x")) == "Center" &&
              panel_plan.add_defaults.GetValue(panel_plan.add_defaults.Find("cell_align_y")) == "Center",
          "root window drop uses centered expand placement without x/y");
    UiDesignerDropPlan layout_plan =
        drop_session.PlanAddControl("UiBoxLayout", root, Point(64, 48), true);
    Check(layout_plan.valid && layout_plan.parent == root,
          "root window accepts BoxLayout drops");
    Check(layout_plan.has_canvas_position && layout_plan.add_defaults.Find("x") < 0,
          "root window drop ignores canvas coordinates for layouts");
    UiDesignerNodeId first_root = 0;
    String drop_error;
    Check(drop_session.ExecuteDrop(panel_plan, &first_root, drop_error),
          "root drop executes: " + drop_error);
    Check(!drop_session.PlanAddControl("UiLabel", root, Point(10, 10), true).valid,
          "root window rejects a second direct child");

    const UiDesignerControlSpec* box_spec = catalog.Find("UiBoxLayout");
    Check(box_spec && box_spec->defaults.GetValue(box_spec->defaults.Find("inset")) == 8,
          "Box default inset is 8");
    Check(box_spec && box_spec->defaults.GetValue(box_spec->defaults.Find("gap")) == 8,
          "Box default gap is 8");
    Check(box_spec && box_spec->FindProperty("debug_layout"),
          "Box exposes Designer debug geometry");
    Check(box_spec && box_spec->FindProperty("cell_align_x") &&
              box_spec->FindProperty("cell_align_x")->choices.GetCount() == 3,
          "Box alignment has no Auto choice");

    UiDesignerDocument preview_document;
    UiDesignerCommandService preview_commands(preview_document);
    UiDesignerNodeId preview_box = preview_commands.AddNode(
        "UiBoxLayout", "preview_box", preview_document.GetRootId(),
        box_spec ? box_spec->node_flags : 0,
        box_spec ? box_spec->defaults : ValueMap(), "Add preview Box");
    const UiDesignerControlSpec* panel_spec = catalog.Find("UiPanel");
    const UiDesignerControlSpec* grid_spec = catalog.Find("UiGridLayout");
    Check(panel_spec && panel_spec->FindProperty("inset") &&
              panel_spec->FindProperty("inset")->default_value == 8 &&
              panel_spec->defaults.GetValue(panel_spec->defaults.Find("inset")) == 8,
          "Panel inset metadata and defaults are 8");
    Check(grid_spec && grid_spec->FindProperty("inset") &&
              grid_spec->FindProperty("inset")->default_value == 8 &&
              grid_spec->defaults.GetValue(grid_spec->defaults.Find("inset")) == 8,
          "Grid inset metadata and defaults are 8");
    UiDesignerNodeId preview_panel_a = preview_commands.AddNode(
        "UiPanel", "preview_panel_a", preview_box,
        panel_spec ? panel_spec->node_flags : 0,
        panel_spec ? panel_spec->defaults : ValueMap(), "Add preview Panel A");
    UiDesignerNodeId preview_panel_b = preview_commands.AddNode(
        "UiPanel", "preview_panel_b", preview_box,
        panel_spec ? panel_spec->node_flags : 0,
        panel_spec ? panel_spec->defaults : ValueMap(), "Add preview Panel B");
    UiDesignerSelection preview_selection;
    UiDesignerPreviewCanvas preview;
    preview.SetRect(0, 0, 512, 250);
    preview.Bind(&preview_document, &catalog, nullptr, &preview_selection);
    preview.RebuildDocument();
    const Rect preview_a = preview.GetNodeRect(preview_panel_a);
    const Rect preview_b = preview.GetNodeRect(preview_panel_b);
    Check(preview.GetNodeRect(preview_box).Size() == Size(512, 250),
          "preview assigns the root Box its final rectangle first");
    Check(!preview_a.IsEmpty() && !preview_b.IsEmpty() && preview_a != preview_b,
          Format("Box children have distinct non-empty preview rectangles: %s / %s",
                 AsString(preview_a), AsString(preview_b)));
    Check(!preview_a.IsEmpty() && preview.HitNode(preview_a.CenterPoint()) == preview_panel_a,
          Format("preview hit testing resolves the Panel over its Box: %s",
                 AsString(preview_a)));
    const UiDesignerGeometrySnapshot& geometry = preview.GetGeometrySnapshot();
    const UiDesignerGeometryRecord* box_geometry = geometry.Find(preview_box);
    const UiDesignerGeometryRecord* panel_geometry = geometry.Find(preview_panel_a);
    Check(box_geometry && panel_geometry && box_geometry->rect == preview.GetNodeRect(preview_box),
          "geometry snapshot matches final Box rectangle");
    Check(box_geometry && box_geometry->cue_kind == UiDesignerCueKind::LayoutBounds,
          "Box publishes a layout cue");
    Check(panel_geometry && panel_geometry->cue_kind == UiDesignerCueKind::ContainerBounds,
          "Panel publishes a container cue");
    Check(panel_geometry && panel_geometry->parent == preview_box &&
              panel_geometry->depth > (box_geometry ? box_geometry->depth : -1),
          "Panel geometry is ordered ahead of its Box parent");
    Check(box_geometry && box_geometry->item_rects.GetCount() >= 2,
          "Box snapshot keeps runtime item rectangles");
    Check(box_geometry && box_geometry->gap == 8,
          "Box snapshot keeps authoritative gap geometry");
    Check(box_geometry && box_geometry->gap_rects.GetCount() > 0,
          "Box snapshot exposes explicit gap regions");
    Check(box_geometry && box_geometry->item_rects.GetCount() > 0 &&
              box_geometry->item_rects[0].TopLeft() !=
              preview.GetNodeRect(preview_box).TopLeft(),
          "Box item rectangles use document coordinates");
    Check(box_geometry && geometry.Hit(Point(1, 1)) == preview_box,
          "exposed Box region resolves to the Box");
    Check(panel_geometry && geometry.HitDropTarget(preview_a.CenterPoint()) == preview_panel_a,
          Format("drop resolver starts with the foremost supported target: %d / %d",
                 (int)geometry.HitDropTarget(preview_a.CenterPoint()), (int)preview_panel_a));
    Check(geometry.Hit(preview_a.CenterPoint()) == preview_panel_a,
          "snapshot hit testing agrees with the painted Panel target");
    Check(geometry.GetDropRegionCount() >= 3,
          Format("geometry snapshot publishes drop regions (%d)",
                 geometry.GetDropRegionCount()));
    if(geometry.GetDropRegionCount() < 3) {
        for(const UiDesignerDropRegion& region : geometry.GetDropRegions())
            Cout() << Format("region owner=%d kind=%d depth=%d order=%d label=%s\n",
                             (int)region.owner, (int)region.kind,
                             region.depth, region.paint_order, region.label);
    }
    const UiDesignerDropRegion* panel_drop = geometry.HitDropRegion(preview_a.CenterPoint());
    Check(panel_drop && panel_drop->owner == preview_panel_a &&
              panel_drop->kind == UiDesignerDropRegionKind::PanelBody,
          Format("panel drop region wins over its Box parent (%d, kind=%d)",
                 panel_drop ? (int)panel_drop->owner : 0,
                 panel_drop ? (int)panel_drop->kind : -1));
    const UiDesignerDropRegion* box_inset_drop =
        geometry.HitDropRegion(preview.GetNodeRect(preview_box).TopLeft() + Point(1, 1));
    Check(box_inset_drop && box_inset_drop->owner == preview_box &&
              (box_inset_drop->kind == UiDesignerDropRegionKind::BoxFrame ||
               box_inset_drop->kind == UiDesignerDropRegionKind::BoxEmptyBody ||
               box_inset_drop->kind == UiDesignerDropRegionKind::BoxBody),
          Format("Box inset resolves to the Box itself (%d, kind=%d)",
                 box_inset_drop ? (int)box_inset_drop->owner : 0,
                 box_inset_drop ? (int)box_inset_drop->kind : -1));
    Check(panel_drop && geometry.FindDropRegion(panel_drop->paint_order) == panel_drop,
          "drop region lookup is stable");

    UiDesignerDocument sample_document;
    UiDesignerCommandService sample_commands(sample_document);
    UiDesignerNodeId sample_box = sample_commands.AddNode(
        "UiBoxLayout", "sample_box", sample_document.GetRootId(),
        box_spec ? box_spec->node_flags : 0,
        box_spec ? box_spec->defaults : ValueMap(), "Add sample Box");
    Check(sample_box != 0, "sample Box created");
    auto sample_add = [&](const char *type, const char *name) -> UiDesignerNodeId {
        const UiDesignerControlSpec* spec = catalog.Find(type);
        return sample_commands.AddNode(
            type, name, sample_box,
            spec ? spec->node_flags : 0,
            spec ? spec->defaults : ValueMap(),
            Format("Add sample %s", type));
    };
    const UiDesignerNodeId sample_line = sample_add("UiLineEdit", "sample_line");
    const UiDesignerNodeId sample_int = sample_add("UiIntEdit", "sample_int");
    const UiDesignerNodeId sample_float = sample_add("UiFloatEdit", "sample_float");
    const UiDesignerNodeId sample_password = sample_add("UiPasswordEdit", "sample_password");
    const UiDesignerNodeId sample_multi = sample_add("UiMultiEdit", "sample_multi");
    const UiDesignerNodeId sample_mask = sample_add("UiMaskEdit", "sample_mask");
    const UiDesignerNodeId sample_slider_edit = sample_add("UiSliderEdit", "sample_slider_edit");
    const UiDesignerNodeId sample_progress = sample_add("UiProgressBar", "sample_progress");
    const UiDesignerNodeId sample_edit_string = sample_add("UppEditString", "sample_edit_string");
    const UiDesignerNodeId sample_edit_int = sample_add("UppEditInt", "sample_edit_int");
    const UiDesignerNodeId sample_edit_double = sample_add("UppEditDouble", "sample_edit_double");
    const UiDesignerNodeId sample_line_edit = sample_add("UppLineEdit", "sample_line_edit");
    const UiDesignerNodeId sample_drop = sample_add("UppDropList", "sample_drop");
    const UiDesignerNodeId sample_tab = sample_add("UppTabCtrl", "sample_tab");
    const UiDesignerNodeId sample_doc = sample_add("UiDoc", "sample_doc");
    const UiDesignerNodeId sample_slider = sample_add("UiSlider", "sample_slider");
    const UiDesignerNodeId sample_slider_ctrl = sample_add("UppSliderCtrl", "sample_slider_ctrl");
    const UiDesignerNodeId sample_dropdown = sample_add("UiCompositeDropdown", "sample_dropdown");
    const UiDesignerNodeId sample_comp_slider = sample_add("UiCompositeSlider", "sample_comp_slider");
    const UiDesignerNodeId sample_comp_toggle = sample_add("UiCompositeToggle", "sample_comp_toggle");
    const UiDesignerNodeId sample_comp_color = sample_add("UiCompositeColor", "sample_comp_color");
    const UiDesignerNodeId sample_comp_label = sample_add("UiCompositeLabel", "sample_comp_label");
    const UiDesignerNodeId sample_comp_edit = sample_add("UiCompositeEdit", "sample_comp_edit");
    const UiDesignerNodeId sample_button = sample_add("UiButton", "sample_button");
    const UiDesignerNodeId sample_title_card = sample_add("UiTitleCard", "sample_title_card");
    Check(sample_commands.SetProperty(
        sample_button, "icon_render_mode", "PreserveColor",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set button icon render mode"),
          "button icon render mode command");
    Check(sample_commands.SetProperty(
        sample_button, "icon_width", 24,
        UiDesignerImpactLocalLayout | UiDesignerImpactCode, "Set button icon width"),
          "button icon width command");
    Check(sample_commands.SetProperty(
        sample_button, "icon_height", 20,
        UiDesignerImpactLocalLayout | UiDesignerImpactCode, "Set button icon height"),
          "button icon height command");
    Check(sample_commands.SetProperty(
        sample_button, "content_inset_left", 7,
        UiDesignerImpactLocalLayout | UiDesignerImpactCode, "Set button content inset left"),
          "button content inset left command");
    Check(sample_commands.SetProperty(
        sample_button, "content_inset_top", 6,
        UiDesignerImpactLocalLayout | UiDesignerImpactCode, "Set button content inset top"),
          "button content inset top command");
    Check(sample_commands.SetProperty(
        sample_button, "content_inset_right", 5,
        UiDesignerImpactLocalLayout | UiDesignerImpactCode, "Set button content inset right"),
          "button content inset right command");
    Check(sample_commands.SetProperty(
        sample_button, "content_inset_bottom", 4,
        UiDesignerImpactLocalLayout | UiDesignerImpactCode, "Set button content inset bottom"),
          "button content inset bottom command");
    Check(sample_commands.SetProperty(
        sample_button, "checkable", true,
        UiDesignerImpactControlState | UiDesignerImpactPaint | UiDesignerImpactCode,
        "Set button checkable"), "button checkable command");
    Check(sample_commands.SetProperty(
        sample_button, "checked", true,
        UiDesignerImpactControlState | UiDesignerImpactPaint | UiDesignerImpactCode,
        "Set button checked"), "button checked command");
    Check(sample_commands.SetProperty(
        sample_title_card, "subtitle", "Supporting information",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set title card subtitle"),
          "title card subtitle command");
    Check(sample_commands.SetProperty(
        sample_title_card, "copy",
        "Add a short description or place content in the card.",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set title card copy"),
          "title card copy command");
    Check(sample_commands.SetProperty(
        sample_title_card, "text_align_h", "Center",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set title card text align"),
          "title card text align command");
    Check(sample_commands.SetProperty(
        sample_title_card, "media_side", "Right",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set title card media side"),
          "title card media side command");
    Check(sample_commands.SetProperty(
        sample_title_card, "media_reserve", 80,
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set title card media reserve"),
          "title card media reserve command");
    Check(sample_commands.SetProperty(
        sample_title_card, "media_share_percent", 25,
        UiDesignerImpactPaint | UiDesignerImpactCode,
        "Set title card media share percent"),
          "title card media share percent command");
    Check(sample_commands.SetProperty(
        sample_title_card, "show_title_line", false,
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set title card title line"),
          "title card title line command");
    Check(sample_commands.SetProperty(
        sample_title_card, "show_card_line", true,
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set title card card line"),
          "title card card line command");
    UiDesignerSelection sample_selection;
    UiDesignerPreviewCanvas sample_preview;
    sample_preview.SetRect(0, 0, 512, 250);
    sample_preview.Bind(&sample_document, &catalog, nullptr, &sample_selection);
    sample_preview.RebuildDocument();
    const auto CheckRuntime = [&](UiDesignerNodeId id, const char *type) -> Ctrl* {
        Ctrl *runtime = sample_preview.FindRuntime(id);
        Check(runtime != nullptr, String(type) + " preview instance exists");
        return runtime;
    };
    if(auto *edit = dynamic_cast<UiLineEdit *>(CheckRuntime(sample_line, "UiLineEdit")))
        Check(edit->GetTextUtf8() == "Line edit", "UiLineEdit representative text");
    if(auto *edit = dynamic_cast<UiIntEdit *>(CheckRuntime(sample_int, "UiIntEdit")))
        Check(edit->GetValue() == 0, "UiIntEdit representative value");
    if(auto *edit = dynamic_cast<UiFloatEdit *>(CheckRuntime(sample_float, "UiFloatEdit")))
        Check(edit->GetValue() == 0.0, "UiFloatEdit representative value");
    if(auto *edit = dynamic_cast<UiPasswordEdit *>(CheckRuntime(sample_password, "UiPasswordEdit")))
        Check(edit->GetTextUtf8() == "password" && !edit->IsPlainTextVisible() &&
              edit->GetPasswordChar() == 0x2022, "UiPasswordEdit representative masked text");
    if(auto *edit = dynamic_cast<UiMultiEdit *>(CheckRuntime(sample_multi, "UiMultiEdit")))
        Check(edit->GetTextUtf8() == "Multi-line\nfollowed by text on a second line",
              "UiMultiEdit representative multiline text");
    if(auto *edit = dynamic_cast<UiMaskEdit *>(CheckRuntime(sample_mask, "UiMaskEdit")))
        Check(edit->GetMask() == "##/##/####" && edit->GetTextUtf8() == "01/02/2026",
              "UiMaskEdit representative masked text");
    if(auto *edit = dynamic_cast<UiSliderEdit *>(CheckRuntime(sample_slider_edit, "UiSliderEdit")))
        Check(edit->GetValue() == 50, "UiSliderEdit representative value");
    if(auto *bar = dynamic_cast<UiProgressBar *>(CheckRuntime(sample_progress, "UiProgressBar")))
        Check(bar->GetText() == "Loading assets" && bar->GetPercent() == 50,
              "UiProgressBar representative value");
    if(auto *edit = dynamic_cast<EditString *>(CheckRuntime(sample_edit_string, "EditString")))
        Check(edit->GetData().ToString() == "Edit string", "EditString representative text");
    if(auto *edit = dynamic_cast<EditInt *>(CheckRuntime(sample_edit_int, "EditInt")))
        Check(edit->GetData() == 0, "EditInt representative value");
    if(auto *edit = dynamic_cast<EditDouble *>(CheckRuntime(sample_edit_double, "EditDouble")))
        Check(edit->GetData() == 0.0, "EditDouble representative value");
    if(auto *edit = dynamic_cast<LineEdit *>(CheckRuntime(sample_line_edit, "LineEdit")))
        Check(edit->GetData().ToString() == "Line edit", "LineEdit representative text");
    if(auto *drop = dynamic_cast<DropList *>(CheckRuntime(sample_drop, "DropList")))
        Check(drop->GetData() == 1 && drop->GetCount() == 2,
              "DropList representative selection");
    if(auto *tab = dynamic_cast<TabCtrl *>(CheckRuntime(sample_tab, "TabCtrl")))
        Check(tab->GetData() == 0, "TabCtrl representative selection");
    if(auto *doc = dynamic_cast<UiDoc *>(CheckRuntime(sample_doc, "UiDoc")))
        Check(doc->GetText() == "UiDoc sample", "UiDoc representative text");
    if(auto *slider = dynamic_cast<UiSlider *>(CheckRuntime(sample_slider, "UiSlider")))
        Check(slider->GetValue() == 50, "UiSlider representative value");
    if(auto *slider = dynamic_cast<SliderCtrl *>(CheckRuntime(sample_slider_ctrl, "SliderCtrl")))
        Check(slider->GetData() == 50, "SliderCtrl representative value");
    if(auto *dropdown = dynamic_cast<UiCompositeDropdown *>(CheckRuntime(sample_dropdown, "UiCompositeDropdown")))
        Check(dropdown->GetData() == 1, "UiCompositeDropdown representative selection");
    if(auto *composite = dynamic_cast<UiCompositeSlider *>(CheckRuntime(sample_comp_slider, "UiCompositeSlider")))
        Check(composite->GetData() == 50, "UiCompositeSlider representative value");
    if(auto *composite = dynamic_cast<UiCompositeToggle *>(CheckRuntime(sample_comp_toggle, "UiCompositeToggle")))
        Check(composite->GetData() == true, "UiCompositeToggle representative value");
    if(auto *composite = dynamic_cast<UiCompositeColor *>(CheckRuntime(sample_comp_color, "UiCompositeColor")))
        Check(composite->GetColors().GetCount() == 1 &&
              composite->GetColors()[0] == Color(58, 132, 255),
              "UiCompositeColor representative swatch");
    if(auto *composite = dynamic_cast<UiCompositeLabel *>(CheckRuntime(sample_comp_label, "UiCompositeLabel")))
        Check(composite->GetData().ToString() == "Value", "UiCompositeLabel representative text");
    if(auto *composite = dynamic_cast<UiCompositeEdit *>(CheckRuntime(sample_comp_edit, "UiCompositeEdit")))
        Check(composite->GetData().ToString() == "Editable value",
              "UiCompositeEdit representative text");
    if(auto *button = dynamic_cast<UiButton *>(CheckRuntime(sample_button, "UiButton"))) {
        Check(button->IsCheckable() && button->IsChecked(),
              "UiButton representative checked state");
        Check(button->GetIconRenderMode() == UiIconRenderMode::PreserveColor,
              "UiButton icon render mode applies");
        Check(button->GetIconSize() == Size(DPI(24), DPI(20)),
              Format("UiButton icon size applies (got %d x %d)",
                     button->GetIconSize().cx, button->GetIconSize().cy));
        Check(button->GetContentInset() == Rect(DPI(7), DPI(6), DPI(5), DPI(4)),
              "UiButton content inset applies");
        Check(button->GetContentGap() == 4,
              "UiButton content gap remains default");
        Check(button->GetStyle().align_h == UiAlign::CENTER &&
              button->GetStyle().align_v == UiAlign::CENTER,
              "UiButton content alignment remains centered");
    }
    if(auto *card = dynamic_cast<UiTitleCard *>(CheckRuntime(sample_title_card, "UiTitleCard"))) {
        Check(card->GetStyle().text_align_h == UiAlign::CENTER &&
              card->GetStyle().text_align_v == UiAlign::CENTER,
              "UiTitleCard text alignment applies");
        Check(card->GetStyle().media_side == UiAlign::RIGHT,
              "UiTitleCard media side applies");
        Check(card->GetStyle().media_reserve == 80,
              "UiTitleCard media reserve applies");
        Check(card->GetStyle().media_share_percent == 25,
              Format("UiTitleCard media share percent applies (got %d)",
                     card->GetStyle().media_share_percent));
        Check(!card->GetStyle().title_line,
              "UiTitleCard title line visibility applies");
        Check(card->GetStyle().card_line,
              "UiTitleCard card line visibility applies");
    }

    UiDesignerDocument document;
    UiDesignerCommandService commands(document);

    const UiDesignerControlSpec* label = catalog.Find("UiLabel");
    Check(label != nullptr, "UiLabel spec exists");

    UiDesignerNodeId node = commands.AddNode(
        "UiLabel", "label", document.GetRootId(),
        label ? label->node_flags : 0,
        label ? label->defaults : ValueMap(), "Add label");
    Check(node != 0, "add node command");

    Check(commands.SetProperty(
        node, "text", "Hello",
        UiDesignerImpactControlState |
        UiDesignerImpactLocalLayout |
        UiDesignerImpactCode, "Set text"), "set property command");
    Check(document.GetProperty(node, "text") == "Hello", "property committed");
    Check(commands.CanUndo(), "undo available");
    Check(commands.Undo(), "undo succeeds");
    Check(document.GetProperty(node, "text") == "Label",
          "undo restores property default");
    Check(commands.Redo(), "redo succeeds");
    Check(document.GetProperty(node, "text") == "Hello", "redo restores property");
    const int history_before_invalid = commands.GetHistoryPosition();
    Check(!commands.MoveNode(node, node, -1, "Invalid self move"),
          "invalid command rejected");
    Check(commands.GetHistoryPosition() == history_before_invalid,
          "invalid command creates no history entry");

    String json = UiDesignerSerialize(document, true);
    UiDesignerDocument roundtrip;
    Check(UiDesignerDeserialize(json, roundtrip, error),
          "document round trip: " + error);
    Check(roundtrip.GetCount() == document.GetCount(), "round-trip node count");
    Check(roundtrip.GetVirtualSize() == document.GetVirtualSize(),
          "round-trip virtual size");

    const String legacy_json =
        "{\"format\":\"upp-ui-designer\",\"schema\":1,"
        "\"virtual_size\":{\"cx\":640,\"cy\":480},"
        "\"selection\":[2],\"nodes\":["
        "{\"id\":1,\"parent\":0,\"type\":\"Window\","
        "\"name\":\"Window\",\"properties\":{}},"
        "{\"id\":2,\"parent\":1,\"type\":\"Label\","
        "\"name\":\"legacy_label\",\"last_rect\":{"
        "\"left\":10,\"top\":20,\"right\":180,\"bottom\":54},"
        "\"properties\":{\"text\":{\"type\":\"string\","
        "\"value\":\"Legacy\"}}}]}";
    UiDesignerDocument legacy;
    Check(UiDesignerDeserialize(legacy_json, legacy, error),
          "legacy document import: " + error);
    Check(legacy.GetCount() == 2, "legacy node count");
    Check(legacy.GetNodes()[1].type == "UiLabel", "legacy type mapping");
    Check(legacy.GetNodes()[1].GetProperty("text") == "Legacy",
          "legacy property unwrapping");

    UiDesignerTransientOverlay overlay;
    overlay.Set(node, "text", "Transient");
    Check(overlay.Resolve(node, "text", "Hello") == "Transient",
          "transient overlay");
    overlay.Remove(node, "text");
    Check(overlay.Resolve(node, "text", "Hello") == "Hello",
          "overlay cancellation");

    UiDesignerThemeDocument theme;
    PropertyEditorModel theme_model;
    theme.BuildPropertyModel(theme_model);
    Check(theme_model.GetCount() >= 10, "theme property model");
    Check(theme.Preview("pill_radius", 30, error), "theme preview");
    Check(theme.GetEffective().pill_radius == 30, "theme effective preview");
    theme.CancelPreview();
    Check(theme.GetEffective().pill_radius == 25, "theme cancel");
    Check(theme.Commit("pill_radius", 28, "Set pill radius", error),
          "theme commit");
    Check(theme.CanUndo(), "theme undo available");
    Check(theme.Undo(), "theme undo");
    Check(theme.Get().pill_radius == 25, "theme undo value");
    Check(theme.Redo(), "theme redo");
    Check(theme.Get().pill_radius == 28, "theme redo value");

    UiDesignerSession session;
    session.NewDocument("blank");
    UiDesignerNodeId a = session.AddControl("UiLabel");
    UiDesignerNodeId b = session.AddControl("UiLabel");
    session.Select(a, false);
    session.Select(b, true);
    session.RebuildInspector();

    PropertyEditorItem* text = session.InspectorModel().Find("text");
    Check(text != nullptr, "multi-selection common property");
    const int inspector_structure_before = session.InspectorModel().GetStructureRevision();
    Check(session.CommitProperty("text", "Shared", error),
          "multi-selection commit: " + error);
    Check(session.Document().GetProperty(a, "text") == "Shared",
          "first target updated");
    Check(session.Document().GetProperty(b, "text") == "Shared",
          "second target updated");
    Check(session.InspectorModel().GetStructureRevision() == inspector_structure_before,
          "ordinary commit keeps inspector structure stable");
    Check(session.InspectorModel().Find("text") &&
              session.InspectorModel().Find("text")->value == "Shared",
          "inspector model receives committed value");
    Check(session.CommitProperty("visible", false, error),
          "boolean commit succeeds: " + error);
    Check(session.CommitProperty("fixed_width", 320, error),
          "integer commit succeeds: " + error);
    Check(session.Commands().CanUndo(), "bulk edit is one history entry");
    Check(session.Undo(), "bulk edit undo");

    UiDesignerSession preview_session;
    UiDesignerPreviewCanvas preview_projection;
    preview_projection.SetRect(0, 0, 512, 250);
    preview_session.AttachProjection(&preview_projection);
    UiDesignerNodeId transient_box = preview_session.AddControl("UiBoxLayout");
    preview_session.Select(transient_box, false);
    const int history_before_preview_cancel =
        preview_session.Commands().GetHistoryPosition();
    Check(preview_session.PreviewProperty("inset", 20, error),
          "transient inset preview succeeds");
    Check(preview_session.PreviewOverlay().Has(transient_box, "inset"),
          "transient inset is tracked by node/property");
    preview_session.CancelPreview();
    Check(!preview_session.PreviewOverlay().Has(transient_box, "inset"),
          "cancel clears only tracked transient properties");
    Check(preview_session.Document().GetProperty(transient_box, "inset") == 8,
          "cancel leaves canonical inset unchanged");
    Check(preview_session.Commands().GetHistoryPosition() == history_before_preview_cancel,
          "cancel preview creates no undo command");

    UiDesignerNodeId c = session.AddControl("UiLabel");
    session.Select(c, false);
    Check(session.RemoveSelection(), "single delete command succeeds");
    Check(!session.Document().Find(c), "single delete removes node");
    Check(session.Undo(), "single delete undo restores node");
    Check(session.Document().Find(c) != nullptr, "single delete undo restores selection target");

    UiDesignerNodeId d = session.AddControl("UiLabel");
    UiDesignerNodeId e = session.AddControl("UiLabel");
    session.Select(d, false);
    session.Select(e, true);
    Check(session.RemoveSelection(), "multi delete command succeeds");
    Check(!session.Document().Find(d) && !session.Document().Find(e),
          "multi delete removes both nodes");
    Check(session.Undo(), "multi delete undo restores nodes");
    Check(session.Document().Find(d) != nullptr && session.Document().Find(e) != nullptr,
          "multi delete undo restores both targets");

    session.ClearSelection();
    session.Select(session.Document().GetRootId(), false);
    Check(!session.RemoveSelection(), "root delete is rejected");

    UiDesignerAutomationService automation(session);
    ValueMap initialize;
    initialize.Set("method", "initialize");
    Value init_response = automation.Handle(initialize);
    Check((bool)UiDesignerMapValue(ValueMap(init_response), "ok", false), "automation initialize");

    ValueMap list;
    list.Set("method", "list_controls");
    Value list_response = automation.Handle(list);
    Check((bool)UiDesignerMapValue(ValueMap(list_response), "ok", false), "automation list controls");
    Check((bool)UiDesignerMapValue(ValueMap(automation.ValidateDocument()), "ok", false),
          "automation validation");

    ValueMap theme_preview;
    theme_preview.Set("property", "pill_radius");
    theme_preview.Set("value", 31);
    Check((bool)UiDesignerMapValue(ValueMap(automation.PreviewThemeProperty(theme_preview)), "ok", false),
          "automation theme preview");
    Check(session.Theme().GetEffective().pill_radius == 31,
          "automation theme effective value");
    automation.CancelThemePreview();
    Check(session.Theme().GetEffective().pill_radius == 25,
          "automation theme cancel");

    UiDesignerMcpEndpoint endpoint(automation);
    String mcp_initialize = endpoint.HandleJsonLine(
        "{\"jsonrpc\":\"2.0\",\"id\":1,"
        "\"method\":\"initialize\",\"params\":{"
        "\"protocolVersion\":\"2025-03-26\"}}");
    Check(mcp_initialize.Find("serverInfo") >= 0, "MCP initialize");
    String mcp_tools = endpoint.HandleJsonLine(
        "{\"jsonrpc\":\"2.0\",\"id\":2,"
        "\"method\":\"tools/list\",\"params\":{}}");
    Check(mcp_tools.Find("uidesigner_commit_theme_property") >= 0,
          "MCP theme tools listed");

    UiDesignerCodeGenerator generator(catalog);
    UiDesignerGeneratedProject generated =
        generator.Generate(document, "GeneratedUiWindow");
    Check(generated.header.Find("class GeneratedUiWindow") >= 0,
          "generated header");
    Check(generated.source.Find("SetText(\"Hello\")") >= 0,
          "generated property");
    Check(generated.json.Find("upp-ui-designer-next") >= 0,
          "generated JSON");
    Check(json.Find("geometry") < 0 && generated.source.Find("GeometrySnapshot") < 0,
          "geometry snapshot remains outside serialization and codegen");

    UiDesignerGeneratedProject sample_generated =
        generator.Generate(sample_document, "SampleUiWindow");
    Check(sample_generated.source.Find(".SetIconRenderMode(UiIconRenderMode::PreserveColor)") >= 0,
          "generated UiButton icon render mode");
    Check(sample_generated.source.Find(".SetIconSize(DPI(24), DPI(20))") >= 0,
          "generated UiButton icon size");
    Check(sample_generated.source.Find(".SetContentInset(Rect(DPI(7), DPI(6), DPI(5), DPI(4)))") >= 0,
          "generated UiButton content inset");
    Check(sample_generated.source.Find(".SetSubTitle(\"Supporting information\")") >= 0,
          "generated UiTitleCard subtitle");
    Check(sample_generated.source.Find(".SetCopyText(\"Add a short description or place content in the card.\")") >= 0,
          "generated UiTitleCard copy");
    Check(sample_generated.source.Find(".SetTextAlign(UiAlign::CENTER, UiAlign::CENTER)") >= 0,
          "generated UiTitleCard text alignment");
    Check(sample_generated.source.Find(".SetMediaSide(UiAlign::RIGHT)") >= 0,
          "generated UiTitleCard media side");
    Check(sample_generated.source.Find(".ShowTitleLine(false)") >= 0 &&
              sample_generated.source.Find(".ShowCardLine(true)") >= 0,
          "generated UiTitleCard line visibility");

    Cout() << "Checks: " << checks << " Fails: " << fails << "\n";
    SetExitCode(fails ? 1 : 0);
}
