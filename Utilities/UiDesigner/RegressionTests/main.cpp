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
    PropertyEditorModel metadata_model;
    metadata_model.AddText("name", "Name", "button", "Identity");
    metadata_model.StructureChanged();
    const int metadata_structure_revision = metadata_model.GetStructureRevision();
    int metadata_events = 0;
    String metadata_group;
    metadata_model.WhenGroupMetadataChanged = [&](String group) {
        metadata_events++;
        metadata_group = group;
    };
    metadata_model.SetGroupSubtitle("Identity", "UiButton");
    Check(metadata_events == 1 && metadata_group == "Identity" &&
              metadata_model.GetStructureRevision() == metadata_structure_revision,
          "Setting a subtitle emits one non-structural metadata event");
    metadata_model.SetGroupSubtitle("Identity", "UiButton");
    Check(metadata_events == 1,
          "Setting an identical subtitle is a true no-op");
    metadata_model.SetGroupSubtitle("Missing", String());
    Check(metadata_events == 1,
          "Clearing an absent subtitle is a true no-op");
    metadata_model.SetGroupSubtitle("Identity", "UiPanel");
    Check(metadata_events == 2 && metadata_group == "Identity",
          "Changing a subtitle emits one metadata event");
    metadata_model.ClearGroupSubtitle("Identity");
    Check(metadata_events == 3 &&
              metadata_model.GetGroupSubtitle("Identity").IsEmpty(),
          "Clearing a present subtitle emits one metadata event");

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

    UiDesignerSession semantic_session;
    const UiDesignerNodeId root = semantic_session.Document().GetRootId();
    const UiDesignerNodeId accordion = semantic_session.AddControl("UiAccordion", root);
    const UiDesignerNodeId section = semantic_session.Commands().AddAccordionSection(
        accordion, "Section", "Subtitle", "Copy", true);
    semantic_session.Select(section);
    Check(section != 0 && semantic_session.ResolveThemeOverrideOwner() == accordion,
          "Accordion section resolves its real Accordion theme owner");
    Check(semantic_session.ThemeOverrideModel().Find("theme.status") == nullptr &&
              semantic_session.ThemeOverrideModel().GetCount() > 0,
          "Accordion section selection exposes owner Theme Overrides");
    const UiDesignerControlSpec *accordion_spec =
        semantic_session.Catalog().Find("UiAccordion");
    const UiDesignerThemeOverrideSpec *editable_override = nullptr;
    if(accordion_spec)
        for(const UiDesignerThemeOverrideSpec& candidate : accordion_spec->theme_overrides)
            if(!candidate.read_only) {
                editable_override = &candidate;
                break;
            }
    Check(editable_override != nullptr,
          "Accordion has an editable Theme Override contract");
    if(editable_override) {
        Check(semantic_session.SetThemeOverrideActive(
                  editable_override->id, true, error),
              "Semantic selection activates an owner override: " + error);
        const UiDesignerNode *accordion_node =
            semantic_session.Document().Find(accordion);
        const UiDesignerNode *section_node =
            semantic_session.Document().Find(section);
        Check(accordion_node &&
                  accordion_node->IsThemeOverrideActive(editable_override->id) &&
                  section_node && section_node->theme_overrides.IsEmpty(),
              "Semantic Theme Override is authored on the owner only");
    }

    const UiDesignerNodeId tab = semantic_session.AddControl("UiTab", root);
    const UiDesignerNodeId page = semantic_session.Commands().AddTabPage(tab, "Page");
    semantic_session.Select(page);
    Check(page != 0 && semantic_session.ResolveThemeOverrideOwner() == tab,
          "Tab page resolves its real Tab theme owner");
    Check(semantic_session.ThemeOverrideModel().Find("theme.status") == nullptr &&
              semantic_session.ThemeOverrideModel().GetCount() > 0,
          "Tab page selection exposes owner Theme Overrides without fake per-page style");

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
