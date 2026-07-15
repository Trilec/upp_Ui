#include <Utilities/UiDesigner/Services/UiDesignerServices.h>

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
    Check(catalog.FindCategory("Layouts").GetCount() >= 4, "layout catalog");
    Check(catalog.FindCategory("Containers").GetCount() >= 8, "container catalog");
    Check(catalog.FindCategory("Ui Controls").GetCount() >= 20, "Ui control catalog");
    Check(catalog.FindCategory("Composites").GetCount() >= 6, "composite catalog");
    Check(catalog.FindCategory("U++ Controls").GetCount() >= 18, "stock U++ catalog");
    Check(catalog.GetPresets().GetCount() >= 3, "preset catalog");

    static const char *required_ui[] = {
        "UiLabel", "UiCheckBox", "UiRadioButton", "UiToggle", "UiPanel",
        "UiDirectContentHost", "UiGroupPanel", "UiStack", "UiAccordion",
        "UiScrollPanel", "UiTab", "UiTitleCard", "UiGridLayout", "UiBoxLayout",
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
    Check(session.CommitProperty("text", "Shared", error),
          "multi-selection commit: " + error);
    Check(session.Document().GetProperty(a, "text") == "Shared",
          "first target updated");
    Check(session.Document().GetProperty(b, "text") == "Shared",
          "second target updated");
    Check(session.Commands().CanUndo(), "bulk edit is one history entry");
    Check(session.Undo(), "bulk edit undo");

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
