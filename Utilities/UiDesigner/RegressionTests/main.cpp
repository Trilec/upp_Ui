#include <Utilities/UiDesigner/Services/UiDesignerServices.h>
#include <Utilities/UiDesigner/UiDesigner/UiDesignerWidgets.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>

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

static String LegacySizingJson()
{
    return R"JSON({
      "format":"upp-ui-designer-next",
      "schema":3,
      "document_id":"sizing-regression",
      "virtual_size":{"cx":512,"cy":250},
      "nodes":[
        {"id":1,"parent":0,"type":"Window","name":"Window","flags":3,
         "children":[2],"properties":{},"actions":[]},
        {"id":2,"parent":1,"type":"UiButton","name":"button","flags":0,
         "children":[],
         "properties":{"h_sizing":"Expand","v_sizing":"Fill"},
         "actions":[]}
      ],
      "resources":[]
    })JSON";
}

CONSOLE_APP_MAIN
{
    UiDesignerSession session;
    session.NewDocument("blank");
    const UiDesignerNodeId button = session.AddControl("UiButton");
    Check(button != 0, "Button fixture is created");

    UiDesignerHierarchyView hierarchy;
    hierarchy.SetRect(0, 0, 404, 220);
    hierarchy.SetCatalog(&session.Catalog());
    hierarchy.SetDocument(&session.Document());
    hierarchy.SetSelection(&session.State().selection);

    int selections = 0;
    UiDesignerNodeId selected = 0;
    hierarchy.WhenSelectNode = [&](UiDesignerNodeId id, bool) {
        selections++;
        selected = id;
    };

    hierarchy.LeftDown(Point(DPI(16), DPI(12)), 0);
    hierarchy.LeftUp(Point(DPI(16), DPI(12)), 0);
    Check(selections == 0,
          "Hierarchy header does not alias the first document row");

    hierarchy.LeftDown(Point(DPI(16), DPI(24 + 30 + 15)), 0);
    hierarchy.LeftUp(Point(DPI(16), DPI(24 + 30 + 15)), 0);
    Check(selections == 1 && selected == button,
          "Hierarchy name click selects the real row below the header");

    int sizing_requests = 0;
    bool requested_height = false;
    UiDesignerNodeId sizing_node = 0;
    hierarchy.CycleSizingMode = [&](UiDesignerNodeId id, bool height) {
        sizing_requests++;
        sizing_node = id;
        requested_height = height;
        return true;
    };

    const Rect width_mode = hierarchy.GetWidthModeRect(1);
    hierarchy.LeftDown(width_mode.CenterPoint(), 0);
    hierarchy.LeftUp(width_mode.CenterPoint(), 0);
    Check(sizing_requests == 1 && sizing_node == button && !requested_height,
          "Hierarchy W icon routes one width-mode request");

    const Rect height_mode = hierarchy.GetHeightModeRect(1);
    hierarchy.LeftDown(height_mode.CenterPoint(), 0);
    hierarchy.LeftUp(height_mode.CenterPoint(), 0);
    Check(sizing_requests == 2 && sizing_node == button && requested_height,
          "Hierarchy H icon routes one height-mode request");

    PropertyEditorModel override_model;
    PropertyEditorItem& radius = override_model.AddNumericInt(
        "radius", "Radius", 8, 0, 100, 1, "General");
    radius.overrideable = true;
    radius.override_active = false;
    radius.inherited = true;
    radius.enabled = true;
    radius.value_editable = false;
    radius.read_only = true;
    override_model.StructureChanged();

    PropertyEditor override_editor;
    override_editor.SetRect(0, 0, 404, 180);
    override_editor.SetModel(&override_model);
    int override_requests = 0;
    override_editor.WhenOverride = [&](String id, bool active) {
        override_requests++;
        Check(id == "radius" && active,
              "Radius circle requests inherited activation");
    };
    override_editor.LeftDown(Point(394, 75), 0);
    Check(override_requests == 1,
          "Radius override circle remains active while its value editor is locked");

    UiDesignerSession radius_session;
    const UiDesignerNodeId panel = radius_session.AddControl("UiPanel");
    radius_session.Select(panel);
    PropertyEditorItem *radius_item =
        radius_session.ThemeOverrideModel().Find("radius");
    Check(radius_item && radius_item->inherited && !radius_item->override_active,
          "Panel Radius starts inherited");
    String error;
    Check(radius_session.SetThemeOverrideActive("radius", true, error),
          "Panel Radius activates through the command path: " + error);
    radius_item = radius_session.ThemeOverrideModel().Find("radius");
    Check(radius_item && !radius_item->inherited && radius_item->override_active &&
              radius_item->value_editable,
          "Panel Radius becomes an editable local value after activation");

    UiDesignerDocument migrated;
    Check(UiDesignerDeserialize(LegacySizingJson(), migrated, error),
          "Legacy sizing fixture loads: " + error);
    const UiDesignerNode *migrated_button = migrated.Find(2);
    Check(migrated_button &&
              migrated_button->GetProperty("width_mode", "") == "Expand" &&
              migrated_button->GetProperty("height_mode", "") == "Expand",
          "Legacy Expand and Fill both migrate to canonical Expand");
    Check(migrated_button &&
              migrated_button->properties.Find("h_sizing") < 0 &&
              migrated_button->properties.Find("v_sizing") < 0,
          "Legacy common-control sizing aliases are removed after migration");

    UiDesignerCodeView code_view;
    code_view.SetRect(0, 0, 404, 260);
    const String code = "line 1\nline 2\nline 3\n";
    code_view.SetCode(code);
    code_view.Layout();
    Check(code_view.GetCode() == code,
          "Code viewer preserves selectable generated source text");

    Cout() << "UiDesignerRegressionTests: Checks: " << checks
           << " Fails: " << fails << "\n";
    SetExitCode(fails ? 1 : 0);
}
