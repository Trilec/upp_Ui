#include <Utilities/UiDesigner/Services/UiDesignerServices.h>
#include <Utilities/UiDesigner/UiDesigner/UiDesignerWidgets.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Ui/UiIcons.h>

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

static String LegacySiblingOrderJson()
{
    return R"JSON({
      "format":"upp-ui-designer",
      "virtual_size":{"cx":512,"cy":250},
      "nodes":[
        {"id":1,"parent":0,"type":"Window","name":"Window","properties":{}},
        {"id":2,"parent":1,"type":"BoxLayout","name":"column","properties":{}},
        {"id":3,"parent":2,"type":"Label","name":"heading","properties":{}},
        {"id":4,"parent":2,"type":"Panel","name":"content","properties":{}},
        {"id":5,"parent":2,"type":"Label","name":"footer","properties":{}}
      ]
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

    UiDesignerSession dialog_session;
    dialog_session.NewDocument("dialog");
    const UiDesignerDocument& dialog_document = dialog_session.Document();
    const UiDesignerNode *dialog_actions = nullptr;
    for(const UiDesignerNode& candidate : dialog_document.GetNodes())
        if(candidate.name == "dialog_actions") {
            dialog_actions = &candidate;
            break;
        }
    Check(dialog_actions &&
              AsString(dialog_actions->GetProperty("direction", "")) == "H" &&
              AsString(dialog_actions->GetProperty("wrap", "")) == "Flow",
          "Dialog actions author horizontal Flow layout before any Inspector toggle");
    if(dialog_actions) {
        Check(dialog_actions->children.GetCount() == 3,
              "Dialog actions retain Spacer, Cancel and OK source order");
        if(dialog_actions->children.GetCount() == 3) {
            const UiDesignerNode *spacer_node =
                dialog_document.Find(dialog_actions->children[0]);
            const UiDesignerNode *cancel_node =
                dialog_document.Find(dialog_actions->children[1]);
            const UiDesignerNode *ok_node =
                dialog_document.Find(dialog_actions->children[2]);
            Check(spacer_node && spacer_node->type == "Spacer" &&
                      AsString(spacer_node->GetProperty("width_mode", "")) == "Expand",
                  "Dialog action spacer expands on the horizontal main axis");
            Check(cancel_node && ok_node &&
                      AsString(cancel_node->GetProperty("width_mode", "")) == "Fixed" &&
                      AsString(ok_node->GetProperty("width_mode", "")) == "Fixed" &&
                      (int)cancel_node->GetProperty("fixed_width", 0) == 88 &&
                      (int)ok_node->GetProperty("fixed_width", 0) == 88,
                  "Dialog buttons retain fixed widths");
        }
    }
    UiDesignerGeneratedProject dialog_generated =
        UiDesignerCodeGenerator(dialog_session.Catalog()).Generate(
            dialog_document, "DialogParityWindow");
    Check(dialog_generated.source.Find("dialog_column_n2.Add(dialog_content_n4).Expand(1)") >= 0,
          "Generated dialog content expands instead of defaulting to Fit");
    Check(dialog_generated.source.Find("dialog_column_n2.Add(dialog_actions_n5).Fixed(DPI(40))") >= 0,
          "Generated dialog action row uses its fixed main-axis height");
    Check(dialog_generated.source.Find("dialog_actions_n5.AddSpacer().Expand(1)") >= 0,
          "Generated dialog preserves the expanding semantic spacer");
    Check(dialog_generated.source.Find("dialog_actions_n5.Add(cancel_button_n7).Fixed(DPI(88))") >= 0 &&
              dialog_generated.source.Find("dialog_actions_n5.Add(ok_button_n8).Fixed(DPI(88))") >= 0,
          "Generated dialog buttons use their authored fixed widths");

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

    UiDesignerSession sizing_session;
    sizing_session.NewDocument("blank");
    const UiDesignerNodeId sizing_label = sizing_session.AddControl("UiLabel");
    const UiDesignerNodeId sizing_box = sizing_session.AddControl("UiBoxLayout");
    sizing_session.Select(sizing_box);
    const uint64 sizing_selection_revision = sizing_session.State().selection.revision;
    String sizing_error;
    Check(sizing_session.CycleSizingMode(sizing_label, false, sizing_error),
          "Unselected Label width mode cycles by exact node identity: " + sizing_error);
    Check(sizing_session.Document().Find(sizing_label)->GetProperty("width_mode", "Fit") == "Fixed" &&
              sizing_session.Document().Find(sizing_box)->GetProperty("width_mode", "Fit") == "Fit",
          "Hierarchy sizing mutates the clicked Label and not the selected BoxLayout");
    Check(sizing_session.State().selection.primary == sizing_box &&
              sizing_session.State().selection.revision == sizing_selection_revision,
          "Exact-node sizing preserves designer selection");
    Check(sizing_session.CycleSizingMode(sizing_label, false, sizing_error) &&
              sizing_session.Document().Find(sizing_label)->GetProperty("width_mode", "Fit") == "Expand" &&
              sizing_session.CycleSizingMode(sizing_label, false, sizing_error) &&
              sizing_session.Document().Find(sizing_label)->GetProperty("width_mode", "Fit") == "Fit",
          "Repeated sizing cycles Fit to Fixed to Expand to Fit");
    Check(sizing_session.Undo() &&
              sizing_session.Document().Find(sizing_label)->GetProperty("width_mode", "Fit") == "Expand" &&
              sizing_session.Redo() &&
              sizing_session.Document().Find(sizing_label)->GetProperty("width_mode", "Fit") == "Fit",
          "Exact-node sizing remains undoable and redoable");

    const UiDesignerNodeId sizing_accordion = sizing_session.AddControl(
        "UiAccordion", sizing_session.Document().GetRootId());
    const UiDesignerNodeId sizing_section = sizing_session.Commands().AddAccordionSection(
        sizing_accordion, "Section");
    const String section_before = sizing_session.Document().Find(sizing_section)->GetProperty(
        "width_mode", "Fit");
    Check(!sizing_session.CycleSizingMode(sizing_section, false, sizing_error) &&
              sizing_session.Document().Find(sizing_section)->GetProperty(
                  "width_mode", "Fit") == section_before,
          "Unsupported semantic hierarchy rows remain inert");

    UiDesignerSession dialog_session;
    dialog_session.NewDocument("dialog");
    const UiDesignerDocument& dialog_document = dialog_session.Document();
    const UiDesignerNode *dialog_actions = nullptr;
    for(const UiDesignerNode& candidate : dialog_document.GetNodes())
        if(candidate.name == "dialog_actions")
            dialog_actions = &candidate;
    Check(dialog_actions && dialog_actions->children.GetCount() == 3,
          "Fresh Dialog preset creates one three-item action row");
    if(dialog_actions && dialog_actions->children.GetCount() == 3) {
        const UiDesignerNode *dialog_spacer = dialog_document.Find(dialog_actions->children[0]);
        const UiDesignerNode *dialog_cancel = dialog_document.Find(dialog_actions->children[1]);
        const UiDesignerNode *dialog_ok = dialog_document.Find(dialog_actions->children[2]);
        Check(dialog_spacer && dialog_cancel && dialog_ok &&
                  dialog_spacer->name == "dialog_action_spacer" &&
                  dialog_cancel->name == "cancel_button" &&
                  dialog_ok->name == "ok_button",
              "Dialog action source order is Spacer, Cancel, OK");
        Check(dialog_actions->GetProperty("direction", "") == "H" &&
                  dialog_actions->GetProperty("wrap", "") == "None" &&
                  (int)dialog_actions->GetProperty("gap", -1) == 8,
              "Dialog action row starts horizontal, non-wrapping, and deterministic");
        Check(dialog_spacer->GetProperty("width_mode", "") == "Expand" &&
                  dialog_spacer->GetProperty("height_mode", "") == "Fit" &&
                  dialog_cancel->GetProperty("width_mode", "") == "Fixed" &&
                  (int)dialog_cancel->GetProperty("fixed_width", 0) == 88 &&
                  dialog_ok->GetProperty("width_mode", "") == "Fixed" &&
                  (int)dialog_ok->GetProperty("fixed_width", 0) == 88,
              "Dialog spacer expands while both buttons retain fixed width");
        Check(dialog_cancel->GetProperty("height_mode", "") == "Fixed" &&
                  (int)dialog_cancel->GetProperty("fixed_height", 0) == 32 &&
                  dialog_ok->GetProperty("height_mode", "") == "Fixed" &&
                  (int)dialog_ok->GetProperty("fixed_height", 0) == 32,
              "Dialog button heights are deterministic");
    }

    UiDesignerPreviewCanvas preview;
    dialog_session.AttachProjection(&preview);
    preview.ResetStats();
    const UiDesignerNodeId actions_id = dialog_actions ? dialog_actions->id : 0;
    if(actions_id) {
        Check(dialog_session.Commands().SetProperty(
                  actions_id, "direction", "V",
                  UiDesignerImpactLocalLayout | UiDesignerImpactAncestorLayout,
                  "Test direction rebuild"),
              "Dialog action direction change commits");
        Check(preview.GetStats().subtree_rebuilds == 1,
              "Direction change rebuilds the affected BoxLayout subtree exactly once");
        preview.ResetStats();
        Check(dialog_session.Commands().SetProperty(
                  actions_id, "wrap", "Flow",
                  UiDesignerImpactLocalLayout | UiDesignerImpactAncestorLayout,
                  "Test wrap rebuild"),
              "Dialog action wrap change commits");
        Check(preview.GetStats().subtree_rebuilds == 1,
              "Wrap change rebuilds the affected BoxLayout subtree exactly once");
    }

    hierarchy.PlanCatalogDrop = [&](const String& type,
                                    UiDesignerNodeId parent, int index) {
        return session.PlanAddControl(type, parent, Point(0, 0), false, index);
    };
    hierarchy.ExecuteDrop = [&](const UiDesignerDropPlan& plan, String& error) {
        UiDesignerNodeId created = 0;
        return session.ExecuteDrop(plan, &created, error);
    };
    const Point header_screen = hierarchy.GetScreenRect().TopLeft() +
                                Point(DPI(20), DPI(15));
    const int count_before_header_drop = session.Document().GetCount();
    hierarchy.TrackCatalogDrop("UiPanel", header_screen);
    Check(hierarchy.HasDropTarget(),
          "Catalog drag over the non-selectable heading targets the document root");
    Check(hierarchy.FinishCatalogDrop("UiPanel", header_screen) &&
              session.Document().GetCount() == count_before_header_drop + 1,
          "Hierarchy heading performs one explicit root catalog drop");

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
    Check(section != 0 && semantic_session.ResolveThemeOverrideOwner() == section,
          "Accordion section is its own selectable Theme Override owner");
    Check(semantic_session.ThemeOverrideModel().Find("theme.status") == nullptr &&
              semantic_session.ThemeOverrideModel().GetCount() > 0,
          "Accordion section selection exposes its UiTitleCard Theme Overrides");
    const UiDesignerControlSpec *accordion_spec =
        semantic_session.Catalog().Find("UiAccordionSection");
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
        Check(section_node &&
                  section_node->IsThemeOverrideActive(editable_override->id) &&
                  accordion_node &&
                  !accordion_node->IsThemeOverrideActive(editable_override->id),
              "Semantic Theme Override is authored on the selected section only");
    }

    const UiDesignerNodeId tab = semantic_session.AddControl("UiTab", root);
    const UiDesignerNodeId page = semantic_session.Commands().AddTabPage(tab, "Page");
    semantic_session.Select(page);
    Check(page != 0 && semantic_session.ResolveThemeOverrideOwner() == 0,
          "Tab page does not invent a per-page or redirected Theme Override contract");
    Check(semantic_session.ThemeOverrideModel().GetCount() == 0,
          "Tab page selection exposes only API-backed content and behaviour");

    const UiDesignerControlSpec *section_spec =
        semantic_session.Catalog().Find("UiAccordionSection");
    const UiDesignerControlSpec *page_spec =
        semantic_session.Catalog().Find("UiTabPage");
    const UiDesignerControlSpec *label_spec =
        semantic_session.Catalog().Find("UiLabel");
    Check(section_spec && section_spec->FindProperty("name") &&
              section_spec->FindProperty("icon") &&
              section_spec->FindProperty("open") &&
              section_spec->FindProperty("media_side"),
          "Accordion section exposes Identity, Content, Behaviour and Appearance");
    Check(page_spec && page_spec->FindProperty("name") &&
              page_spec->FindProperty("icon") &&
              page_spec->FindProperty("tooltip") &&
              page_spec->FindProperty("closable") &&
              page_spec->FindProperty("draggable"),
          "Tab page exposes API-backed identity, content and behaviour");
    Check(label_spec && label_spec->FindProperty("icon") &&
              label_spec->FindProperty("icon_side") &&
              label_spec->FindProperty("content_gap"),
          "UiLabel exposes its icon content API");
    const UiDesignerPropertySpec *section_icon =
        section_spec ? section_spec->FindProperty("icon") : nullptr;
    Check(section_icon && section_icon->choices.GetCount() ==
              UiIconCatalog().GetCount() + 1,
          "Icon properties enumerate the authoritative UiIcons catalogue");

    const UiDesignerControlSpec *accordion_owner_spec =
        semantic_session.Catalog().Find("UiAccordion");
    const UiDesignerPropertySpec *open_glyph = accordion_owner_spec
        ? accordion_owner_spec->FindProperty("chevron_open_icon") : nullptr;
    Check(open_glyph && open_glyph->default_value == "Default" &&
              open_glyph->choices.GetCount() == UiIconCatalog().GetCount() + 2,
          "Accordion glyph choices preserve the control default and expose every catalogue icon");

    const UiDesignerControlSpec *tab_spec =
        semantic_session.Catalog().Find("UiTab");
    Check(tab_spec && tab_spec->FindProperty("tab_font_face") &&
              tab_spec->FindProperty("tab_extent") &&
              tab_spec->FindProperty("tab_padding_left") &&
              tab_spec->FindProperty("strip_inset_left") &&
              tab_spec->FindProperty("content_gap"),
          "UiTab exposes API-backed font, extent, padding, inset and spacing appearance");

    int content_runs = 0;
    bool in_content = false;
    if(section_spec)
        for(const UiDesignerPropertySpec& property : section_spec->properties) {
            if(property.group == "Content") {
                if(!in_content)
                    content_runs++;
                in_content = true;
            }
            else
                in_content = false;
        }
    Check(content_runs == 1,
          "Accordion section has one contiguous Content group");

    UiDesignerSession default_glyph_session;
    const UiDesignerNodeId default_accordion =
        default_glyph_session.AddControl("UiAccordion");
    Check(default_accordion != 0,
          "Default Accordion code-generation fixture is created");
    const String default_glyph_code =
        default_glyph_session.GenerateCode("DefaultGlyphFixture");
    Check(default_glyph_code.Find(".SetChevronGlyphs(") < 0 &&
              default_glyph_code.Find(".SetDragGlyph(") < 0,
          "Generated default Accordion preserves native chevron and drag glyphs");

    const String semantic_code =
        semantic_session.GenerateCode("SemanticInspectorFixture");
    Check(semantic_code.Find(".SetTabIcon(") >= 0 &&
              semantic_code.Find(".SetTabTip(") >= 0 &&
              semantic_code.Find(".SetTabClosable(") >= 0 &&
              semantic_code.Find(".SetTabDraggable(") >= 0,
          "Generated Tab pages preserve API-backed content and behaviour");
    Check(semantic_code.Find(".GetSectionHeader(") >= 0 &&
              semantic_code.Find(".SetMedia(") >= 0,
          "Generated Accordion sections author the exposed UiTitleCard header");
    Check(semantic_code.Find("_tab_layout_style.content_gap = DPI(") >= 0 &&
              semantic_code.Find(".SetContentGap(DPI(") < 0,
          "Generated UiTab routes content gap through UiTab::Style only");

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

    UiDesignerDocument ordered_legacy;
    Check(UiDesignerDeserialize(LegacySiblingOrderJson(), ordered_legacy, error),
          "Legacy sibling-order fixture loads: " + error);
    const UiDesignerNode *ordered_column = ordered_legacy.Find(2);
    Check(ordered_column && ordered_column->children.GetCount() == 3 &&
              ordered_legacy.Find(ordered_column->children[0])->name == "heading" &&
              ordered_legacy.Find(ordered_column->children[1])->name == "content" &&
              ordered_legacy.Find(ordered_column->children[2])->name == "footer",
          "Legacy import preserves source sibling order from heading to footer");

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
