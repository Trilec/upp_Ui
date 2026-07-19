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
    Check(panel_geometry && panel_geometry->parent == preview_box &&
              panel_geometry->depth > (box_geometry ? box_geometry->depth : -1),
          "Panel geometry is ordered ahead of its Box parent");
    Check(box_geometry && box_geometry->item_rects.GetCount() >= 2,
          "Box snapshot keeps runtime item rectangles");
    Check(box_geometry && box_geometry->gap == 8,
          "Box snapshot keeps authoritative gap geometry");
    Check(box_geometry && geometry.Hit(Point(1, 1)) == preview_box,
          "exposed Box region resolves to the Box");
    Check(geometry.Hit(preview_a.CenterPoint()) == preview_panel_a,
          "snapshot hit testing agrees with the painted Panel target");

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

    Cout() << "Checks: " << checks << " Fails: " << fails << "\n";
    SetExitCode(fails ? 1 : 0);
}
