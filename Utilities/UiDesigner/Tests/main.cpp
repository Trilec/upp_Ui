#include <Utilities/UiDesigner/Services/UiDesignerServices.h>
#include <Utilities/UiDesigner/Services/UiDesignerTreeDataAdapter.h>
#include <Utilities/UiDesigner/Services/UiDesignerListDataAdapter.h>
#include <Utilities/UiDesigner/Preview/UiDesignerPreview.h>
#include <Utilities/UiDesigner/Theme/UiDesignerThemeAdapter.h>
#include <Ui/UiAbsoluteLayout.h>
#include <Ui/UiGridLayout.h>
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

static bool SameButtonStyle(const UiButton::Style& a, const UiButton::Style& b)
{
    auto SameFill = [](const UiFill& x, const UiFill& y) {
        if(x.IsNone() || y.IsNone())
            return x.IsNone() && y.IsNone();
        if(x.IsSolid() || y.IsSolid())
            return x.IsSolid() && y.IsSolid() && x.color == y.color;
        return x.IsImage() && y.IsImage();
    };
    for(int i = 0; i < 4; i++) {
        if(!SameFill(a.palette.face[i], b.palette.face[i]))
            return false;
        if(a.palette.frame[i] != b.palette.frame[i])
            return false;
        if(a.palette.ink[i] != b.palette.ink[i])
            return false;
        if(a.palette.icon[i] != b.palette.icon[i])
            return false;
    }
    return a.metrics.face_enabled == b.metrics.face_enabled &&
           a.metrics.frame_enabled == b.metrics.frame_enabled &&
           a.metrics.content_margin == b.metrics.content_margin &&
           a.metrics.shadow.enabled == b.metrics.shadow.enabled &&
           a.metrics.shadow.distance == b.metrics.shadow.distance &&
           a.metrics.shadow.offset_x == b.metrics.shadow.offset_x &&
           a.metrics.shadow.offset_y == b.metrics.shadow.offset_y &&
           a.metrics.shadow.alpha == b.metrics.shadow.alpha &&
           a.metrics.shadow.color == b.metrics.shadow.color &&
           a.metrics.shadow.inset == b.metrics.shadow.inset &&
           a.metrics.shadow.mode == b.metrics.shadow.mode &&
           a.press_offset == b.press_offset &&
           a.overpaint == b.overpaint &&
           a.font.GetHeight() == b.font.GetHeight() &&
           a.font.IsBold() == b.font.IsBold() &&
           a.font.IsItalic() == b.font.IsItalic() &&
           a.transparent == b.transparent &&
           a.align_h == b.align_h &&
           a.align_v == b.align_v &&
           a.icon_side == b.icon_side &&
           a.content_gap == b.content_gap &&
           a.underline == b.underline &&
           a.underline_width == b.underline_width &&
           a.underline_offset == b.underline_offset &&
           a.skin.enabled == b.skin.enabled &&
           a.skin.content_inset == b.skin.content_inset;
}

CONSOLE_APP_MAIN
{
    UiDesignerCatalog catalog;
    Check(PropertyEditorKindName(PropertyEditorKind::ColorPalette) == "ColorPalette" &&
              PropertyEditorKindName(PropertyEditorKind::FilePath) == "FilePath" &&
              PropertyEditorKindName(PropertyEditorKind::NumericInt) == "NumericInt" &&
              PropertyEditorKindName(PropertyEditorKind::NumericDouble) == "NumericDouble",
          "PropertyEditor exposes palette, file-path and dual numeric editor kinds");
    const PropertyEditorStyle property_style = PropertyEditorStyle::System();
    Check(property_style.row_odd != property_style.row_even &&
              property_style.row_hover != property_style.row_even &&
              property_style.row_selected != property_style.row_odd,
          "PropertyEditor row backgrounds remain visually distinguishable");
    RegisterUiDesignerBuiltins(catalog);

    String error;
    Check(catalog.Validate(error), "catalog validates: " + error);
    const UiDesignerControlSpec *metadata_tree = catalog.Find("UiTree");
    const UiDesignerControlSpec *metadata_list = catalog.Find("UiList");
    const UiDesignerControlSpec *metadata_tab = catalog.Find("UiTab");
    const UiDesignerControlSpec *metadata_accordion = catalog.Find("UiAccordion");
    Check(metadata_tree && metadata_tree->data_capability == UiDesignerDataCapability::Tree &&
              metadata_tree->data_adapter_id == "tree" && !metadata_tree->data_defaults.IsEmpty(),
          "UiTree declares its canonical tree data adapter");
    Check(metadata_list && metadata_list->data_capability == UiDesignerDataCapability::List &&
              metadata_list->data_adapter_id == "list" && !metadata_list->data_defaults.IsEmpty(),
          "UiList declares its canonical list data adapter");
    Check(metadata_tab && metadata_tab->data_capability == UiDesignerDataCapability::Pages &&
              metadata_tab->data_adapter_id == "tab",
          "UiTab declares its semantic page data adapter");
    Check(metadata_accordion &&
              metadata_accordion->data_capability == UiDesignerDataCapability::AccordionSections &&
              metadata_accordion->data_adapter_id == "accordion",
          "UiAccordion declares its semantic section data adapter");
    for(const UiDesignerControlSpec& spec : catalog.GetControls())
        if(!spec.data_defaults.IsEmpty())
            Check(spec.data_capability != UiDesignerDataCapability::None &&
                      !spec.data_adapter_id.IsEmpty(),
                  spec.type_id + " data defaults have an explicit adapter");
    Check(catalog.GetCount() >= 50, "complete native and U++ catalog");
    Check(catalog.FindCategory("Layouts").GetCount() >= 5, "layout catalog");
    Check(catalog.FindCategory("Containers").GetCount() >= 8, "container catalog");
    Check(catalog.FindCategory("Ui Controls").GetCount() >= 20, "Ui control catalog");
    Check(catalog.FindCategory("Composites").GetCount() >= 6, "composite catalog");
    Check(catalog.FindCategory("U++ Controls").GetCount() >= 18, "stock U++ catalog");
    Check(catalog.GetPresets().GetCount() >= 3, "preset catalog");

    auto DefaultSizeValue = [](const UiDesignerControlSpec& spec,
                               const char *field) {
        const int q = spec.defaults.Find(field);
        return q >= 0 ? (int)spec.defaults.GetValue(q) : -1;
    };
    for(const UiDesignerControlSpec& spec : catalog.GetControls()) {
        if(spec.IsSemanticItem())
            continue;
        Check(spec.default_size.cx > 0 && spec.default_size.cy > 0,
              spec.type_id + " has a visible natural size");
        Check(spec.minimum_size.cx > 0 && spec.minimum_size.cy > 0 &&
                  spec.minimum_size.cx <= spec.default_size.cx &&
                  spec.minimum_size.cy <= spec.default_size.cy,
              spec.type_id + " has a usable minimum size");
        Check(DefaultSizeValue(spec, "fixed_width") == spec.default_size.cx &&
                  DefaultSizeValue(spec, "fixed_height") == spec.default_size.cy,
              spec.type_id + " Fixed mode starts from its natural size");
        Check(DefaultSizeValue(spec, "min_width") == spec.minimum_size.cx &&
                  DefaultSizeValue(spec, "min_height") == spec.minimum_size.cy,
              spec.type_id + " minimum fields use its visible minimum");
        Check(DefaultSizeValue(spec, "max_width") == 0 &&
                  DefaultSizeValue(spec, "max_height") == 0,
              spec.type_id + " keeps zero only for unbounded maximums");
        const UiDesignerPropertySpec* fixed_width =
            spec.FindProperty("fixed_width");
        const UiDesignerPropertySpec* fixed_height =
            spec.FindProperty("fixed_height");
        const UiDesignerPropertySpec* min_width =
            spec.FindProperty("min_width");
        const UiDesignerPropertySpec* min_height =
            spec.FindProperty("min_height");
        Check(fixed_width && fixed_height && min_width && min_height &&
                  (int)fixed_width->minimum == 1 &&
                  (int)fixed_height->minimum == 1 &&
                  (int)min_width->minimum == 1 &&
                  (int)min_height->minimum == 1,
              spec.type_id + " cannot author zero fixed/minimum dimensions");
    }

    auto CheckSizeProfile = [&](const char *type, Size natural,
                                Size minimum) {
        const UiDesignerControlSpec* spec = catalog.Find(type);
        Check(spec && spec->default_size == natural &&
                  spec->minimum_size == minimum,
              String(type) + " has its expected sizing profile");
    };
    CheckSizeProfile("UiButton", Size(80, 25), Size(50, 25));
    CheckSizeProfile("UiToolButton", Size(25, 25), Size(25, 25));
    CheckSizeProfile("UiLabel", Size(100, 25), Size(40, 20));
    CheckSizeProfile("UiSlider", Size(160, 25), Size(80, 20));
    CheckSizeProfile("UiMultiEdit", Size(190, 90), Size(100, 50));
    CheckSizeProfile("UiColorPicker", Size(480, 360), Size(320, 240));
    CheckSizeProfile("UiGroupPanel", Size(280, 160), Size(100, 70));
    CheckSizeProfile("UppVScrollBar", Size(18, 160), Size(16, 80));

    UiDesignerSession initial_sizing_session;
    const UiDesignerNodeId sizing_button =
        initial_sizing_session.AddControl("UiButton");
    const UiDesignerNode* sizing_button_node =
        initial_sizing_session.Document().Find(sizing_button);
    Check(sizing_button_node &&
              (int)sizing_button_node->GetProperty("fixed_width", 0) == 80 &&
              (int)sizing_button_node->GetProperty("fixed_height", 0) == 25 &&
              (int)sizing_button_node->GetProperty("min_width", 0) == 50 &&
              (int)sizing_button_node->GetProperty("min_height", 0) == 25,
          "new Button stores its natural and minimum sizing profile");
    Check(initial_sizing_session.InspectorModel().Find("fixed_width") &&
              (int)initial_sizing_session.InspectorModel().Find("fixed_width")->value == 80 &&
              initial_sizing_session.InspectorModel().Find("min_width") &&
              (int)initial_sizing_session.InspectorModel().Find("min_width")->value == 50,
          "Button Inspector presents non-zero sizing values immediately");
    String sizing_error;
    Check(initial_sizing_session.CommitProperty("width_mode", "Fixed", sizing_error),
          "Button width mode switches to Fixed: " + sizing_error);
    Check(initial_sizing_session.CommitProperty("fixed_width", 120, sizing_error),
          "Button Fixed width can be edited: " + sizing_error);
    Check(initial_sizing_session.ResetProperty("fixed_width", sizing_error) &&
              (int)initial_sizing_session.Document().Find(sizing_button)
                  ->GetProperty("fixed_width", 0) == 80,
          "Button Fixed width reset returns to natural width: " + sizing_error);

    UiDesignerDocument legacy_sizing_document;
    legacy_sizing_document.NewDocument();
    UiDesignerCommandService legacy_sizing_commands(legacy_sizing_document);
    const UiDesignerControlSpec* legacy_button_spec =
        catalog.Find("UiButton");
    Check(legacy_button_spec != nullptr,
          "legacy sizing fixture resolves the Button specification");
    ValueMap legacy_button_defaults;
    dword legacy_button_flags = UiDesignerNodeNone;
    if(legacy_button_spec) {
        legacy_button_defaults = legacy_button_spec->defaults;
        legacy_button_flags = legacy_button_spec->node_flags;
    }
    const UiDesignerNodeId legacy_button = legacy_sizing_commands.AddNode(
        "UiButton", "legacy_button", legacy_sizing_document.GetRootId(),
        legacy_button_flags, legacy_button_defaults, "Add legacy button");
    UiDesignerNode* legacy_button_node =
        legacy_sizing_document.Find(legacy_button);
    if(legacy_button_node) {
        legacy_button_node->SetProperty("fixed_width", 0);
        legacy_button_node->SetProperty("fixed_height", 0);
        legacy_button_node->SetProperty("min_width", 0);
        legacy_button_node->SetProperty("min_height", 0);
    }
    Check(catalog.ApplySizingDefaults(legacy_sizing_document),
          "legacy zero sizing values are upgraded");
    legacy_button_node = legacy_sizing_document.Find(legacy_button);
    Check(legacy_button_node &&
              (int)legacy_button_node->GetProperty("fixed_width", 0) == 80 &&
              (int)legacy_button_node->GetProperty("fixed_height", 0) == 25 &&
              (int)legacy_button_node->GetProperty("min_width", 0) == 50 &&
              (int)legacy_button_node->GetProperty("min_height", 0) == 25 &&
              (int)legacy_button_node->GetProperty("max_width", 0) == 0 &&
              (int)legacy_button_node->GetProperty("max_height", 0) == 0,
          "legacy Button receives its profile without bounding maximums");
    Check(!catalog.ApplySizingDefaults(legacy_sizing_document),
          "legacy sizing upgrade is idempotent");

    ImageBuffer background_buffer(4, 2);
    Fill(~background_buffer, RGBA(Color(64, 96, 128)), background_buffer.GetLength());
    UiPanel background_panel;
    background_panel.SetBackgroundImage(Image(background_buffer),
                                        UiBackgroundImageMode::Fit);
    Check(background_panel.GetStyle().skin.enabled &&
              background_panel.GetStyle().skin.image_mode ==
                  UiBackgroundImageMode::Fit,
          "styled controls expose a fit background image contract");
    background_panel.SetBackgroundImageMode(UiBackgroundImageMode::Fill);
    Check(background_panel.GetStyle().skin.image_mode ==
              UiBackgroundImageMode::Fill,
          "styled controls expose a fill background image contract");
    background_panel.ClearBackgroundImage();
    Check(!background_panel.GetStyle().skin.enabled,
          "styled controls can clear a background image");

    UiDesignerDocument resource_document;
    ValueMap resource_metadata;
    resource_metadata.Set("purpose", "designer fixture");
    const String png_bytes = "embedded-png-fixture-bytes";
    const String resource_key = resource_document.AddResource(
        "image", png_bytes, "image/png", "fixture.png", 4, 2,
        resource_metadata);
    UiDesignerResource resource;
    Check(!resource_key.IsEmpty() && resource_document.GetResource(resource_key, resource) &&
              resource.bytes == png_bytes && resource.mime == "image/png" &&
              resource.metadata.GetValue(resource_metadata.Find("purpose")) ==
                  "designer fixture",
          "Designer resource table stores embedded image bytes and metadata");
    UiDesignerDocument resource_roundtrip;
    String resource_error;
    Check(UiDesignerDocumentFromValue(
              UiDesignerDocumentToValue(resource_document), resource_roundtrip,
              resource_error) && resource_roundtrip.GetResources().GetCount() == 1 &&
              resource_roundtrip.GetResource(resource_key, resource) &&
              resource.bytes == png_bytes,
          "embedded image resource survives document round trip: " + resource_error);

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

    static const char *edit_types[] = {
        "UiLineEdit", "UiIntEdit", "UiFloatEdit", "UiPasswordEdit",
        "UiMultiEdit", "UiMaskEdit"
    };
    for(int i = 0; i < __countof(edit_types); i++) {
        const UiDesignerControlSpec* edit = catalog.Find(edit_types[i]);
        Check(edit && edit->theme && edit->theme_adapter_id == "edit" &&
              edit->theme_overrides.GetCount() > 0,
              String("edit has shared theme overrides: ") + edit_types[i]);
        Check(edit && UiDesignerThemeAdapterSupports(*edit),
              String("edit theme adapter supports: ") + edit_types[i]);
    }

    const UiDesignerControlSpec* absolute = catalog.Find("UiAbsoluteLayout");
    Check(absolute && absolute->child_adapter_id == "absolute",
          "absolute layout has an exact-rect child adapter");
    Check(absolute && HasUiDesignerCapability(
              absolute->capabilities, UiDesignerCapabilityFreeform),
          "absolute layout accepts freeform Designer placement");
    Check(absolute && absolute->FindProperty("x") &&
              absolute->FindProperty("y") &&
              absolute->FindProperty("width") &&
              absolute->FindProperty("height") &&
              absolute->FindProperty("width")->kind == PropertyEditorKind::NumericInt &&
              absolute->FindProperty("height")->kind == PropertyEditorKind::NumericInt,
          "absolute layout exposes Inspector geometry bindings with numeric toggles");
    const UiDesignerControlSpec* tool_button = catalog.Find("UiToolButton");
    Check(tool_button &&
              UiDesignerMapValue(tool_button->defaults, "icon", Value()) ==
                  "ICON_DESIGN_TUNE_48",
          "UiToolButton default icon is the inspector glyph");
    Check(tool_button && tool_button->theme_adapter_id == "tool_button",
          "UiToolButton is wired to the tool-button theme adapter");
    const UiDesignerControlSpec* button = catalog.Find("UiButton");
    Check(button && button->theme_adapter_id == "button",
          "UiButton is wired to the button theme adapter");
    const UiDesignerControlSpec* tree = catalog.Find("UiTree");
    const UiDesignerControlSpec* list = catalog.Find("UiList");
    const UiDesignerControlSpec* menu = catalog.Find("UiMenu");
    Check(tree && tree->theme_adapter_id == "tree",
          "UiTree is wired to the tree theme adapter");
    Check(list && list->theme_adapter_id == "list",
          "UiList is wired to the list theme adapter");
    Check(menu && menu->theme_adapter_id == "menu",
          "UiMenu is wired to the menu theme adapter");
    const UiDesignerControlSpec* tab_spec = catalog.Find("UiTab");
    Check(tab_spec && tab_spec->theme_adapter_id == "tab" && !tab_spec->theme_overrides.IsEmpty(),
          "UiTab is wired to the typed theme adapter with overrides");
    const UiDesignerControlSpec* panel_theme_spec = catalog.Find("UiPanel");
    const UiDesignerControlSpec* group_panel_theme_spec = catalog.Find("UiGroupPanel");
    const UiDesignerControlSpec* scroll_panel_theme_spec = catalog.Find("UiScrollPanel");
    Check(panel_theme_spec && panel_theme_spec->theme_adapter_id == "panel" && !panel_theme_spec->theme_overrides.IsEmpty(),
          "UiPanel is wired to the typed theme adapter with overrides");
    Check(group_panel_theme_spec && group_panel_theme_spec->theme_adapter_id == "group_panel" && !group_panel_theme_spec->theme_overrides.IsEmpty(),
          "UiGroupPanel is wired to the typed theme adapter with overrides");
    Check(scroll_panel_theme_spec && scroll_panel_theme_spec->theme_adapter_id == "scroll_panel" && !scroll_panel_theme_spec->theme_overrides.IsEmpty(),
          "UiScrollPanel is wired to the typed theme adapter with overrides");
    const char *face_ids[] = {"face.normal", "face.hot", "face.pressed", "face.disabled"};
    bool panel_face_rows = panel_theme_spec != nullptr;
    for(const char *id : face_ids) {
        const UiDesignerThemeOverrideSpec *row = panel_theme_spec
            ? panel_theme_spec->FindThemeOverride(id) : nullptr;
        panel_face_rows &= row && row->kind == PropertyEditorKind::FillRecipe &&
                           row->default_value.Is<ValueMap>();
    }
    Check(panel_face_rows && panel_theme_spec &&
              panel_theme_spec->FindThemeOverride("face_surface") == nullptr &&
              panel_theme_spec->FindThemeOverride("face_colors") == nullptr,
          "UiPanel exposes four independent FillRecipe face states without legacy rows");
    Check(panel_theme_spec && panel_theme_spec->FindThemeOverride("frame.style") &&
              panel_theme_spec->FindThemeOverride("frame.width") &&
              panel_theme_spec->FindThemeOverride("skin") &&
              panel_theme_spec->FindThemeOverride("frame.normal"),
          "UiPanel exposes shared frame style, width, per-state colors and separate skin");
    UiDesignerSession panel_override_session;
    const UiDesignerNodeId panel_override_node =
        panel_override_session.AddControl("UiPanel");
    panel_override_session.Select(panel_override_node);
    Check(panel_override_session.ThemeOverrideModel().Find("face.normal") &&
              panel_override_session.ThemeOverrideModel().Find("face.normal")->kind ==
                  PropertyEditorKind::FillRecipe &&
              !panel_override_session.ThemeOverrideModel().Find("face.normal")->override_active,
          "Panel Inspector exposes inherited FillRecipe face state");
    String panel_override_error;
    Check(panel_theme_spec->FindThemeOverride("radius") &&
              panel_theme_spec->FindThemeOverride("radius")->label == "Radius",
          "Panel radius uses the concise Radius label");
    Check(panel_override_session.SetThemeOverrideActive("radius", true,
                                                        panel_override_error),
          "Panel Radius override activates: " + panel_override_error);
    Check(panel_override_session.ThemeOverrideModel().Find("radius") &&
              panel_override_session.ThemeOverrideModel().Find("radius")->override_active &&
              panel_override_session.ThemeOverrideModel().Find("radius")->value_editable &&
              panel_override_session.Document().GetThemeOverride(
                  panel_override_node, "radius").Is<int>(),
          "Panel Radius becomes an editable local numeric value");
    Check(panel_override_session.SetThemeOverrideActive("radius", false,
                                                        panel_override_error) &&
              !panel_override_session.Document().Find(panel_override_node)
                   ->IsThemeOverrideActive("radius"),
          "Panel Radius can return to inherited state: " + panel_override_error);
    Check(panel_override_session.SetThemeOverrideActive("radius", true,
                                                        panel_override_error) &&
              panel_override_session.Document().Find(panel_override_node)
                   ->IsThemeOverrideActive("radius"),
          "Panel Radius restores saved state after first activation: " +
              panel_override_error);
    Check(panel_override_session.ThemeOverrideModel().GetCount() > 0 &&
              panel_override_session.ThemeOverrideModel()[0].group == "General",
          "Panel Theme Overrides present General first");
    ValueMap solid_recipe = panel_override_session.ThemeOverrideModel().Find("face.normal")->value;
    solid_recipe.Set("mode", "Solid");
    solid_recipe.Set("solid", Color(12, 34, 56));
    Check(panel_override_session.CommitThemeOverride("face.normal", solid_recipe,
                                                     panel_override_error),
          "Panel Normal FillRecipe override commits: " + panel_override_error);
    UiDesignerNode *panel_override_document_node =
        panel_override_session.Document().Find(panel_override_node);
    Check(panel_override_document_node &&
              panel_override_document_node->IsThemeOverrideActive("face.normal"),
          "Theme override is active after commit");
    Check(panel_override_session.SetThemeOverrideActive("face.normal", false,
                                                        panel_override_error),
          "Theme override can be deactivated without deleting its value: " +
              panel_override_error);
    panel_override_document_node =
        panel_override_session.Document().Find(panel_override_node);
    Check(panel_override_document_node &&
              !panel_override_document_node->IsThemeOverrideActive("face.normal") &&
              panel_override_document_node->theme_override_saved.Find("face.normal") >= 0,
          "Inactive theme override retains its saved authored value");
    UiDesignerDocument inactive_roundtrip;
    String inactive_json = UiDesignerSerialize(
        panel_override_session.Document(), false);
    Check(UiDesignerDeserialize(inactive_json, inactive_roundtrip,
                                panel_override_error),
          "Theme override state JSON round-trip succeeds");
    Check(inactive_roundtrip.Find(panel_override_node) &&
              !inactive_roundtrip.Find(panel_override_node)->IsThemeOverrideActive(
                  "face.normal") &&
              inactive_roundtrip.Find(panel_override_node)->theme_override_saved
                  .GetValue(inactive_roundtrip.Find(panel_override_node)
                      ->theme_override_saved.Find("face.normal")).Is<ValueMap>(),
          "Inactive theme override state and value survive JSON round-trip");
    Check(panel_override_session.SetThemeOverrideActive("face.normal", true,
                                                        panel_override_error) &&
              panel_override_session.Document().GetThemeOverride(
                  panel_override_node, "face.normal").Is<ValueMap>(),
          "Reactivating restores the saved authored value: " +
              panel_override_error + " [" +
              AsString(panel_override_session.Document().GetThemeOverride(
                  panel_override_node, "face.normal")) + "]");
    const char *basic_theme_types[] = {"UiLabel", "UiCheckBox", "UiRadioButton", "UiToggle", "UiProgressBar", "UiSlider", "UiScrollBar", "UiDropdown"};
    for(const char *type : basic_theme_types) {
        const UiDesignerControlSpec* basic = catalog.Find(type);
        Check(basic && basic->theme && !basic->theme_adapter_id.IsEmpty() && !basic->theme_overrides.IsEmpty(),
              String(type) + " is wired to a typed theme adapter with overrides");
    }
    const UiDesignerControlSpec* split_button = catalog.Find("UiSplitButton");
    Check(split_button && split_button->theme_adapter_id == "button" && !split_button->theme_overrides.IsEmpty(),
          "UiSplitButton reuses the complete button theme contract");
    const UiDesignerControlSpec* tree_spec = catalog.Find("UiTree");
    Check(tree_spec && tree_spec->data_defaults.Find("root") >= 0,
          "UiTree catalog declares canonical root data defaults");
    const UiDesignerControlSpec* list_data_spec = catalog.Find("UiList");
    Check(list_data_spec && list_data_spec->data_defaults.Find("root") >= 0,
          "UiList catalog declares canonical item data defaults");
    Check(UiBreadcrumbs::ResolveThemeStyle().font.GetHeight() > 0,
          "UiBreadcrumbs exposes a theme-derived resolver");
    Check(UiTheme::ResolveTable().row_height > 0,
          "UiTable exposes a theme-derived resolver");
    Check(UiTheme::ResolveDoc().font.GetHeight() > 0,
          "UiDoc exposes a theme-derived resolver");
    Check(UiTheme::ResolveBezierCurveEditor().stroke > 0,
          "UiBezierCurveEditor exposes a theme-derived resolver");
    UiDesignerDocument data_document;
    UiDesignerCommandService data_commands(data_document);
    const UiDesignerNodeId data_tree = data_commands.AddNode(
        "UiTree", "data_tree", data_document.GetRootId(),
        UiDesignerNodeContainer, ValueMap(), "Add data tree");
    ValueMap tree_root;
    tree_root.Set("text", "Root");
    ValueArray tree_children;
    ValueMap child_one; child_one.Set("text", "First"); child_one.Set("key", "first");
    ValueMap child_two; child_two.Set("text", "Second"); child_two.Set("key", "second");
    tree_children.Add(child_one); tree_children.Add(child_two);
    tree_root.Set("children", tree_children);
    Check(data_commands.SetData(data_tree, "root", tree_root,
                               UiDesignerImpactStructure | UiDesignerImpactCode,
                               "Set tree data"),
          "Designer data command stores canonical tree data");

    UiDesignerDocument list_data_document;
    UiDesignerCommandService list_data_commands(list_data_document);
    const UiDesignerNodeId data_list_node = list_data_commands.AddNode(
        "UiList", "data_list", list_data_document.GetRootId(),
        UiDesignerNodeContainer, ValueMap(), "Add data list");
    ValueMap list_root;
    ValueArray list_items;
    ValueMap list_first; list_first.Set("text", "First"); list_first.Set("key", "first");
    ValueMap list_second; list_second.Set("text", "Second"); list_second.Set("key", "second");
    list_items.Add(list_first); list_items.Add(list_second);
    list_root.Set("items", list_items);
    Check(list_data_commands.SetData(data_list_node, "root", list_root,
                                     UiDesignerImpactStructure,
                                     "Set list data"),
          "Designer list data command stores canonical items");
    Check(UiDesignerListDataAdapter::Index(
              UiDesignerListDataAdapter::Token(1)) == 1,
          "UiList adapter preserves scalar item selection token");
    ValueMap list_added = list_root;
    ValueMap list_third; list_third.Set("text", "Third"); list_third.Set("enabled", true);
    Check(UiDesignerListDataAdapter::AppendItem(list_added, list_third),
          "UiList adapter appends item");
    Check(UiDesignerListDataAdapter::MoveItem(list_added, 2, -1),
          "UiList adapter reorders item");
    Check(UiDesignerListDataAdapter::RemoveItem(list_added, 1),
          "UiList adapter removes item");
    ValueMap list_changed = UiDesignerListDataAdapter::Item(list_added, 0);
    list_changed.Set("text", "Edited");
    list_changed.Set("checked", true);
    Check(UiDesignerListDataAdapter::SetItem(list_added, 0, list_changed),
          "UiList adapter edits item state");
    Check(list_data_commands.SetData(data_list_node, "root", list_added,
                                     UiDesignerImpactStructure,
                                     "Edit list data"),
          "Designer list edit commits through command service");
    Check(list_data_commands.Undo() && list_data_commands.Redo() &&
              list_data_document.GetData(data_list_node, "root") == list_added,
          "UiList data supports undo and redo");
    UiDesignerDocument list_data_roundtrip;
    String list_data_error;
    Check(UiDesignerDocumentFromValue(UiDesignerDocumentToValue(list_data_document),
                                      list_data_roundtrip, list_data_error) &&
              list_data_roundtrip.GetData(data_list_node, "root") == list_added,
          "UiList data survives JSON round trip");
    Check(data_document.GetData(data_tree, "root", ValueMap()) == tree_root,
          "Designer data command reads canonical tree data");
    UiDesignerDocument data_roundtrip;
    String data_error;
    Check(UiDesignerDocumentFromValue(UiDesignerDocumentToValue(data_document),
                                      data_roundtrip, data_error) &&
              data_roundtrip.GetData(data_tree, "root", ValueMap()) == tree_root,
          "Designer canonical data survives JSON round trip");

    ValueMap nested_root = tree_root;
    ValueArray nested_children = (ValueArray)nested_root.GetValue(
        nested_root.Find("children"));
    ValueMap nested_first = (ValueMap)nested_children[0];
    ValueArray nested_first_children;
    ValueMap nested_leaf; nested_leaf.Set("text", "Leaf"); nested_leaf.Set("key", "leaf");
    nested_first_children.Add(nested_leaf);
    nested_first.Set("children", nested_first_children);
    ValueArray nested_updated;
    nested_updated.Add(nested_first); nested_updated.Add(nested_children[1]);
    nested_root.Set("children", nested_updated);
    Check(data_commands.SetData(data_tree, "root", nested_root,
                                UiDesignerImpactStructure, "Set nested tree data"),
          "nested tree data command commits");
    const Vector<UiDesignerTreeDataRow> nested_rows =
        UiDesignerTreeDataAdapter::Rows(nested_root);
    Check(nested_rows.GetCount() == 4,
          "tree data adapter flattens nested rows without invalidating paths");
    Check(nested_rows.GetCount() > 2 && nested_rows[2].path.GetCount() == 2 &&
              nested_rows[2].path[0] == 0 && nested_rows[2].path[1] == 0,
          "tree data adapter preserves nested index paths");
    UiList tree_data_list;
    UiListModel tree_data_model;
    const Value tree_root_token = UiDesignerTreeDataAdapter::Token(ValueArray());
    ValueArray first_path; first_path.Add(0);
    const Value tree_first_token = UiDesignerTreeDataAdapter::Token(first_path);
    tree_data_model.Add("Root", tree_root_token);
    tree_data_model.Add("First", tree_first_token);
    tree_data_list.SetModel(tree_data_model).SetSelectionMode(UILISTSEL_SINGLE);
    tree_data_list.SetData(tree_first_token);
    Check(tree_data_list.GetData() == tree_first_token &&
              tree_data_list.GetSelectionCount() == 1,
          "tree path token selects one Data Inspector row");
    Check(data_commands.GetHistoryPosition() > 0 &&
              UiDesignerMapValue((ValueMap)UiDesignerMapValue(nested_root, "children",
                                                               ValueArray())[0],
                                 "children", ValueArray()).Is<ValueArray>(),
          "nested tree path is represented by child arrays");

    ValueMap edited_root = nested_root;
    ValueArray edited_children = (ValueArray)edited_root.GetValue(
        edited_root.Find("children"));
    ValueMap edited_first = (ValueMap)edited_children[0];
    ValueArray edited_leafs = (ValueArray)edited_first.GetValue(
        edited_first.Find("children"));
    ValueMap edited_leaf = (ValueMap)edited_leafs[0];
    edited_leaf.Set("text", "Edited leaf");
    edited_leaf.Set("enabled", false);
    ValueArray edited_leafs_updated;
    edited_leafs_updated.Add(edited_leaf);
    edited_leafs = edited_leafs_updated;
    edited_first.Set("children", edited_leafs);
    ValueArray edited_children_updated;
    edited_children_updated.Add(edited_first);
    edited_children_updated.Add(edited_children[1]);
    edited_children = edited_children_updated;
    edited_root.Set("children", edited_children);
    Check(data_commands.SetData(data_tree, "root", edited_root,
                                UiDesignerImpactStructure, "Edit nested tree item"),
          "nested tree edit commits");
    Check(data_commands.Undo() && data_document.GetData(data_tree, "root") == nested_root,
          "nested tree edit undo restores previous map");
    Check(data_commands.Redo() && data_document.GetData(data_tree, "root") == edited_root,
          "nested tree edit redo restores edited map");

    ValueMap add_root = edited_root;
    ValueMap add_parent = edited_first;
    ValueArray add_children = edited_leafs;
    ValueMap added_leaf; added_leaf.Set("text", "Added leaf"); added_leaf.Set("key", "added");
    add_children.Add(added_leaf);
    add_parent.Set("children", add_children);
    add_root.Set("children", edited_children);
    ValueArray add_root_children = (ValueArray)add_root.GetValue(add_root.Find("children"));
    ValueArray add_root_children_updated;
    add_root_children_updated.Add(add_parent);
    add_root_children_updated.Add(add_root_children[1]);
    add_root_children = add_root_children_updated;
    add_root.Set("children", add_root_children);
    Check(data_commands.SetData(data_tree, "root", add_root,
                                UiDesignerImpactStructure, "Add nested tree item"),
          "nested tree add commits");
    Check(((ValueArray)((ValueMap)((ValueArray)add_root.GetValue(add_root.Find("children")))[0])
               .GetValue(((ValueMap)((ValueArray)add_root.GetValue(add_root.Find("children")))[0]).Find("children"))).GetCount() == 2,
          "nested tree add increases child count");
    Check(data_commands.Undo() && data_commands.Redo(),
          "nested tree add supports undo and redo");

    ValueMap removed_root = add_root;
    ValueArray removed_root_children = (ValueArray)removed_root.GetValue(
        removed_root.Find("children"));
    ValueMap removed_parent = (ValueMap)removed_root_children[0];
    ValueArray removed_items = (ValueArray)removed_parent.GetValue(
        removed_parent.Find("children"));
    ValueArray removed_items_updated;
    removed_items_updated.Add(removed_items[1]);
    removed_parent.Set("children", removed_items_updated);
    ValueArray removed_root_children_updated;
    removed_root_children_updated.Add(removed_parent);
    removed_root_children_updated.Add(removed_root_children[1]);
    removed_root.Set("children", removed_root_children_updated);
    Check(data_commands.SetData(data_tree, "root", removed_root,
                                UiDesignerImpactStructure, "Remove nested tree item"),
          "nested tree remove commits");
    Check(((ValueArray)((ValueMap)((ValueArray)removed_root.GetValue(
        removed_root.Find("children")))[0]).GetValue(
            ((ValueMap)((ValueArray)removed_root.GetValue(
                removed_root.Find("children")))[0]).Find("children"))).GetCount() == 1,
          "nested tree remove decreases child count");

    ValueMap reordered_root = removed_root;
    ValueArray reordered_children = (ValueArray)reordered_root.GetValue(
        reordered_root.Find("children"));
    ValueArray reordered_children_updated;
    reordered_children_updated.Add(reordered_children[1]);
    reordered_children_updated.Add(reordered_children[0]);
    reordered_root.Set("children", reordered_children_updated);
    Check(data_commands.SetData(data_tree, "root", reordered_root,
                                UiDesignerImpactStructure, "Reorder tree items"),
          "tree sibling reorder commits");
    Check((String)UiDesignerMapValue((ValueMap)reordered_children_updated[0],
                                     "text", "") == "Second",
          "tree sibling reorder changes authored order");

    ValueMap disabled_root = reordered_root;
    ValueArray disabled_children = (ValueArray)disabled_root.GetValue(
        disabled_root.Find("children"));
    ValueMap disabled_item = (ValueMap)disabled_children[0];
    disabled_item.Set("enabled", false);
    ValueArray disabled_children_updated;
    disabled_children_updated.Add(disabled_item);
    disabled_children_updated.Add(disabled_children[1]);
    disabled_root.Set("children", disabled_children_updated);
    Check(data_commands.SetData(data_tree, "root", disabled_root,
                                UiDesignerImpactStructure, "Disable tree item"),
          "tree enabled state commits");
    Check(!(bool)UiDesignerMapValue((ValueMap)disabled_children_updated[0],
                                     "enabled", true),
          "tree enabled state is authored in data");

    UiDesignerDocument nested_roundtrip;
    Check(UiDesignerDocumentFromValue(UiDesignerDocumentToValue(data_document),
                                      nested_roundtrip, data_error) &&
              nested_roundtrip.GetData(data_tree, "root") == disabled_root,
          "nested tree data survives JSON reload");
    const UiDesignerControlSpec* title_card = catalog.Find("UiTitleCard");
    Check(title_card &&
              UiDesignerMapValue(title_card->defaults, "icon", Value()) ==
                  "ICON_DESIGN_DESCRIPTION_48",
          "UiTitleCard default icon is the description glyph");
    Check(title_card && title_card->FindProperty("icon") &&
              title_card->FindProperty("icon")->default_value ==
                  "ICON_DESIGN_DESCRIPTION_48",
          "UiTitleCard icon property and defaults agree");
    Check(title_card && title_card->FindProperty("media_reserve") &&
              title_card->FindProperty("media_reserve")->default_value == 10 &&
              title_card->FindProperty("media_reserve")->kind == PropertyEditorKind::NumericInt &&
              UiDesignerMapValue(title_card->defaults, "media_reserve", Value()) == 10,
          "UiTitleCard media reserve defaults to 10");
    Check(title_card &&
              !TrimBoth(AsString(UiDesignerMapValue(title_card->defaults,
                                                     "subtitle", Value()))).IsEmpty(),
          "UiTitleCard default subtitle is non-empty");
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

    UiDesignerSession grid_allocation_session;
    grid_allocation_session.NewDocument("blank");
    const UiDesignerNodeId allocation_grid =
        grid_allocation_session.AddControl("UiGridLayout");
    Check(allocation_grid != 0, "Grid allocation fixture creates its Grid");
    UiDesignerDropPlan allocation_panel_plan =
        grid_allocation_session.PlanAddControl("UiPanel", allocation_grid);
    UiDesignerNodeId allocation_panel = 0;
    Check(allocation_panel_plan.valid &&
              grid_allocation_session.ExecuteDrop(allocation_panel_plan,
                                                   &allocation_panel, drop_error),
          "non-positioned Grid drop allocates a Panel cell");
    UiDesignerDropPlan allocation_card_plan =
        grid_allocation_session.PlanAddControl("UiTitleCard", allocation_grid);
    UiDesignerNodeId allocation_card = 0;
    Check(allocation_card_plan.valid &&
              grid_allocation_session.ExecuteDrop(allocation_card_plan,
                                                   &allocation_card, drop_error),
          "non-positioned Grid drop allocates a Title Card cell");
    Check(allocation_panel && allocation_card &&
              grid_allocation_session.Document().GetProperty(allocation_panel,
                                                             "grid_row", -1) == 0 &&
              grid_allocation_session.Document().GetProperty(allocation_panel,
                                                             "grid_column", -1) == 0 &&
              grid_allocation_session.Document().GetProperty(allocation_card,
                                                             "grid_row", -1) == 0 &&
              grid_allocation_session.Document().GetProperty(allocation_card,
                                                             "grid_column", -1) == 1,
          "non-positioned Grid children receive distinct canonical cells");

    UiDesignerSession nested_host_session;
    nested_host_session.NewDocument("blank");
    const UiDesignerNodeId nested_grid = nested_host_session.AddControl("UiGridLayout");
    UiDesignerNodeId nested_panel_left = 0;
    UiDesignerNodeId nested_panel_right = 0;
    Check(nested_host_session.ExecuteDrop(
              nested_host_session.PlanAddControl("UiPanel", nested_grid,
                                                  Point(), false, -1, 0, 0),
              &nested_panel_left, drop_error),
          "nested geometry fixture adds left Panel");
    Check(nested_host_session.ExecuteDrop(
              nested_host_session.PlanAddControl("UiPanel", nested_grid,
                                                  Point(), false, -1, 0, 1),
              &nested_panel_right, drop_error),
          "nested geometry fixture adds right Panel");
    UiDesignerNodeId nested_test_accordion = 0;
    UiDesignerNodeId nested_test_tab = 0;
    Check(nested_host_session.ExecuteDrop(
              nested_host_session.PlanAddControl("UiAccordion", nested_panel_left,
                                                  Point(), true),
              &nested_test_accordion, drop_error),
          "nested geometry fixture adds Accordion to Panel");
    Check(nested_host_session.ExecuteDrop(
              nested_host_session.PlanAddControl("UiTab", nested_panel_right,
                                                  Point(), true),
              &nested_test_tab, drop_error),
          "nested geometry fixture adds Tab to Panel");
    UiDesignerSelection nested_selection;
    UiDesignerPreviewCanvas nested_preview;
    nested_preview.SetRect(0, 0, 512, 320);
    nested_preview.Bind(&nested_host_session.Document(), &catalog, nullptr,
                        &nested_selection);
    nested_preview.RebuildDocument();
    const Rect left_panel_rect = nested_preview.GetNodeRect(nested_panel_left);
    const Rect right_panel_rect = nested_preview.GetNodeRect(nested_panel_right);
    const Rect accordion_rect = nested_preview.GetNodeRect(nested_test_accordion);
    const Rect tab_rect = nested_preview.GetNodeRect(nested_test_tab);
    Check(left_panel_rect.Contains(accordion_rect) &&
              right_panel_rect.Contains(tab_rect),
          "nested container children remain inside their Panel bodies");
    const bool accordion_crosses_right = accordion_rect.left < right_panel_rect.right &&
        accordion_rect.right > right_panel_rect.left &&
        accordion_rect.top < right_panel_rect.bottom &&
        accordion_rect.bottom > right_panel_rect.top;
    const bool tab_crosses_left = tab_rect.left < left_panel_rect.right &&
        tab_rect.right > left_panel_rect.left &&
        tab_rect.top < left_panel_rect.bottom &&
        tab_rect.bottom > left_panel_rect.top;
    Check(!accordion_crosses_right && !tab_crosses_left,
          "nested container children do not cross into the sibling Panel cell");

    UiDesignerSession move_session;
    move_session.NewDocument("blank");
    UiDesignerCommandService& move_commands = move_session.Commands();
    const UiDesignerControlSpec* move_grid_spec = catalog.Find("UiGridLayout");
    const UiDesignerControlSpec* move_button_spec = catalog.Find("UiButton");
    UiDesignerNodeId move_grid = move_commands.AddNode(
        "UiGridLayout", "move_grid", move_session.Document().GetRootId(),
        move_grid_spec ? move_grid_spec->node_flags : 0,
        move_grid_spec ? move_grid_spec->defaults : ValueMap(), "Add move Grid");
    Check(move_grid != 0, "move Grid created");
    UiDesignerNodeId move_button = move_commands.AddNode(
        "UiButton", "move_button", move_grid,
        move_button_spec ? move_button_spec->node_flags : 0,
        move_button_spec ? move_button_spec->defaults : ValueMap(),
        "Add move Button");
    Check(move_button != 0, "move Button created");
    Check(move_commands.SetProperty(
              move_button, "width_mode", "Expand",
              UiDesignerImpactLocalLayout | UiDesignerImpactCode,
              "Set move button width mode"),
          "move Button width mode command");
    Check(move_commands.SetProperty(
              move_button, "height_mode", "Fixed",
              UiDesignerImpactLocalLayout | UiDesignerImpactCode,
              "Set move button height mode"),
          "move Button height mode command");
    Check(move_commands.SetProperty(
              move_button, "fixed_height", 44,
              UiDesignerImpactLocalLayout | UiDesignerImpactCode,
              "Set move button fixed height"),
          "move Button fixed height command");
    Check(move_commands.SetProperty(
              move_button, "cell_align_x", "End",
              UiDesignerImpactLocalLayout | UiDesignerImpactCode,
              "Set move button cell align x"),
          "move Button cell align x command");
    Check(move_commands.SetProperty(
              move_button, "cell_align_y", "Center",
              UiDesignerImpactLocalLayout | UiDesignerImpactCode,
              "Set move button cell align y"),
          "move Button cell align y command");
    move_session.Select(move_button);
    UiDesignerDropPlan move_plan = move_session.PlanMoveSelection(
        move_grid, Point(144, 96), true, -1, 1, 1);
    Check(move_plan.valid && move_plan.property_updates.GetCount() == 1,
          "move plan is valid and targets one node");
    if(move_plan.property_updates.GetCount() == 1) {
        const ValueMap& updates = move_plan.property_updates[0];
        Check(updates.Find("grid_row") >= 0 && updates.Find("grid_column") >= 0,
              "move plan updates grid coordinates");
        Check(updates.Find("width_mode") < 0 && updates.Find("height_mode") < 0 &&
                  updates.Find("fixed_height") < 0 &&
                  updates.Find("cell_align_x") < 0 &&
                  updates.Find("cell_align_y") < 0,
              "move plan preserves authored sizing and alignment");
    }
    String move_error;
    Check(move_session.ExecuteDrop(move_plan, nullptr, move_error),
          "move executes without disturbing authored sizing: " + move_error);
    Check(move_session.Document().GetProperty(move_button, "width_mode") == "Expand",
          "moved Button keeps width mode");
    Check(move_session.Document().GetProperty(move_button, "height_mode") == "Fixed",
          "moved Button keeps height mode");
    Check((int)move_session.Document().GetProperty(move_button, "fixed_height") == 44,
          "moved Button keeps fixed height");
    Check(move_session.Document().GetProperty(move_button, "cell_align_x") == "End",
          "moved Button keeps cell align x");
    Check(move_session.Document().GetProperty(move_button, "cell_align_y") == "Center",
          "moved Button keeps cell align y");

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
    const UiDesignerControlSpec* color_picker_spec = catalog.Find("UiColorPicker");
    Check(color_picker_spec && color_picker_spec->theme_adapter_id == "color_picker" &&
              color_picker_spec->theme_overrides.GetCount() >= 10,
          "Color Picker exposes a typed theme override surface");
    Check(color_picker_spec && color_picker_spec->FindProperty("color") &&
              color_picker_spec->FindProperty("alpha") &&
              color_picker_spec->FindProperty("page_mode") &&
              color_picker_spec->FindProperty("channel_mode") &&
              color_picker_spec->FindProperty("spectrum_mode") &&
              color_picker_spec->FindProperty("harmony_mode") &&
              color_picker_spec->FindProperty("slot_count") &&
              color_picker_spec->FindProperty("active_slot"),
          "Color Picker ordinary properties remain separate from theme overrides");
    Check(panel_spec && panel_spec->FindProperty("inset") &&
              panel_spec->FindProperty("inset")->default_value == 8 &&
              panel_spec->defaults.GetValue(panel_spec->defaults.Find("inset")) == 8,
          "Panel inset metadata and defaults are 8");
    Check(grid_spec && grid_spec->FindProperty("inset") &&
              grid_spec->FindProperty("inset")->default_value == 8 &&
              grid_spec->defaults.GetValue(grid_spec->defaults.Find("inset")) == 8,
          "Grid inset metadata and defaults are 8");
    Check(grid_spec && grid_spec->FindProperty("min_cell_width") &&
              grid_spec->FindProperty("min_cell_height") &&
              grid_spec->defaults.GetValue(grid_spec->defaults.Find("min_cell_width")) == 10 &&
              grid_spec->defaults.GetValue(grid_spec->defaults.Find("min_cell_height")) == 10,
          "Grid exposes 10x10 minimum cell defaults");
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
    Check(!preview.GetNodeRect(preview_box).IsEmpty(),
          "preview assigns the root Box a visible rectangle");
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

    UiDesignerDocument grid_document;
    UiDesignerCommandService grid_commands(grid_document);
    UiDesignerNodeId grid_node = grid_commands.AddNode(
        "UiGridLayout", "grid_node", grid_document.GetRootId(),
        grid_spec ? grid_spec->node_flags : 0,
        grid_spec ? grid_spec->defaults : ValueMap(), "Add test Grid");
    Check(grid_node != 0, "grid node created");
    UiDesignerSelection grid_selection;
    UiDesignerPreviewCanvas grid_preview;
    grid_preview.SetRect(0, 0, 512, 250);
    grid_preview.Bind(&grid_document, &catalog, nullptr, &grid_selection);
    grid_preview.RebuildDocument();
    const UiDesignerGeometrySnapshot& grid_geometry_snapshot = grid_preview.GetGeometrySnapshot();
    const UiDesignerGeometryRecord* grid_geometry = grid_geometry_snapshot.Find(grid_node);
    Check(grid_geometry && grid_geometry->cell_rects.GetCount() == 4,
          Format("grid snapshot publishes explicit 2x2 cell rectangles (%d)",
                 grid_geometry ? grid_geometry->cell_rects.GetCount() : -1));
    Check(grid_geometry && !grid_geometry->cell_rects.IsEmpty() &&
              grid_geometry->cell_rects[0].Size().cx > 0 &&
              grid_geometry->cell_rects[0].Size().cy > 0,
          "grid cell rectangles are non-empty");
    const UiDesignerDropRegion* grid_cell_drop =
        grid_geometry_snapshot.HitDropRegion(grid_geometry->cell_rects[0].CenterPoint());
    Check(grid_cell_drop && grid_cell_drop->owner == grid_node &&
              grid_cell_drop->kind == UiDesignerDropRegionKind::GridCell,
          Format("grid cell hit testing resolves the grid cell (owner=%d kind=%d point=%s rect=%s)",
                 grid_cell_drop ? (int)grid_cell_drop->owner : 0,
                 grid_cell_drop ? (int)grid_cell_drop->kind : -1,
                 AsString(grid_geometry->cell_rects[0].CenterPoint()),
                 AsString(grid_geometry->cell_rects[0])));
    if(!grid_cell_drop || grid_cell_drop->owner != grid_node ||
       grid_cell_drop->kind != UiDesignerDropRegionKind::GridCell) {
        Cout() << Format("grid drop region count=%d\n",
                         grid_geometry_snapshot.GetDropRegionCount());
        for(const UiDesignerDropRegion& region : grid_geometry_snapshot.GetDropRegions())
            Cout() << Format("drop owner=%d kind=%d row=%d col=%d depth=%d order=%d rect=%s visual=%s occupied=%d label=%s\n",
                             (int)region.owner, (int)region.kind, region.grid_row,
                             region.grid_column, region.depth, region.paint_order,
                             AsString(region.rect), AsString(region.visual_rect),
                             (int)region.occupied, region.label);
    }

    UiGridLayout empty_grid_probe;
    empty_grid_probe.SetGridSize(3, 2);
    empty_grid_probe.SetRect(0, 0, 420, 280);
    Vector<Rect> empty_cells;
    empty_grid_probe.GetCellRects(empty_cells);
    Check(empty_cells.GetCount() == 6,
          Format("empty non-square grid publishes row-major cell count (%d)",
                 empty_cells.GetCount()));
    Check(empty_grid_probe.GetCellRect(1, 2) == empty_cells[5],
          "GetCellRect matches the cached row-major collection");
    Check(empty_grid_probe.GetCellRect(-1, 0).IsEmpty() &&
              empty_grid_probe.GetCellRect(9, 9).IsEmpty(),
          "invalid grid coordinates return an empty rectangle");
    Button grid_item_probe;
    const int grid_item = empty_grid_probe.Add(grid_item_probe, 0, 0, false);
    empty_grid_probe.SetItem(grid_item, 0, 0, false, false);
    Vector<Rect> after_item_cells;
    empty_grid_probe.GetCellRects(after_item_cells);
    Check(after_item_cells.GetCount() == 6,
          "SetItem preserves configured 3x2 dimensions after insertion");
    const int empty_builds = empty_grid_probe.GetResolvedCellGeometryBuildCount();
    Vector<Rect> repeated_cells;
    empty_grid_probe.GetCellRects(repeated_cells);
    (void)empty_grid_probe.GetCellRect(0, 0);
    Check(empty_grid_probe.GetResolvedCellGeometryBuildCount() == empty_builds,
          "grid cell accessors reuse cached geometry without rebuilding");

    UiDesignerDocument populated_grid_document;
    UiDesignerCommandService populated_grid_commands(populated_grid_document);
    UiDesignerNodeId populated_grid = populated_grid_commands.AddNode(
        "UiGridLayout", "populated_grid", populated_grid_document.GetRootId(),
        grid_spec ? grid_spec->node_flags : 0,
        grid_spec ? grid_spec->defaults : ValueMap(), "Add populated Grid");
    Check(populated_grid != 0, "populated Grid created");
    Check(populated_grid_commands.SetProperty(
              populated_grid, "columns", 3,
              UiDesignerImpactStructure | UiDesignerImpactCode,
              "Set populated grid columns"),
          "populated grid columns command");
    Check(populated_grid_commands.SetProperty(
              populated_grid, "rows", 2,
              UiDesignerImpactStructure | UiDesignerImpactCode,
              "Set populated grid rows"),
          "populated grid rows command");
    UiDesignerNodeId populated_button = populated_grid_commands.AddNode(
        "UiButton", "populated_button", populated_grid,
        button ? button->node_flags : 0,
        button ? button->defaults : ValueMap(), "Add populated Button");
    Check(populated_button != 0, "populated Button created");
    populated_grid_commands.SetProperty(
        populated_button, "grid_row", 1,
        UiDesignerImpactAncestorLayout | UiDesignerImpactCode, "Set populated row");
    populated_grid_commands.SetProperty(
        populated_button, "grid_column", 2,
        UiDesignerImpactAncestorLayout | UiDesignerImpactCode, "Set populated column");
    UiDesignerSelection populated_selection;
    UiDesignerPreviewCanvas populated_preview;
    populated_preview.SetRect(0, 0, 420, 280);
    populated_preview.Bind(&populated_grid_document, &catalog, nullptr, &populated_selection);
    populated_preview.RebuildDocument();
    const UiDesignerGeometrySnapshot& populated_geometry =
        populated_preview.GetGeometrySnapshot();
    const UiDesignerGeometryRecord* populated_grid_geometry =
        populated_geometry.Find(populated_grid);
    Check(populated_grid_geometry && populated_grid_geometry->cell_rects.GetCount() == 6,
          Format("populated non-square grid publishes row-major cell count (%d)",
                 populated_grid_geometry ? populated_grid_geometry->cell_rects.GetCount() : -1));
    for(const UiDesignerDropRegion& region : populated_geometry.GetDropRegions()) {
        if(region.owner != populated_grid || region.kind != UiDesignerDropRegionKind::GridCell)
            continue;
        const int index = region.grid_row * 3 + region.grid_column;
        Check(index >= 0 && index < populated_grid_geometry->cell_rects.GetCount(),
              "grid cell metadata stays in row-major range");
        if(index >= 0 && index < populated_grid_geometry->cell_rects.GetCount()) {
            Check(region.rect == populated_grid_geometry->cell_rects[index],
                  "grid drop region rectangle equals the published cell rectangle");
            Check(region.grid_row * 3 + region.grid_column == index,
                  "grid cell row-major metadata is stable");
        }
    }
    UiGridLayout* populated_runtime_grid =
        dynamic_cast<UiGridLayout *>(populated_preview.FindRuntime(populated_grid));
    Check(populated_runtime_grid != nullptr, "preview creates a runtime grid instance");
    if(populated_runtime_grid) {
        const int populated_query_count = populated_runtime_grid->GetResolvedCellGeometryQueryCount();
        Check(populated_query_count == 1,
              "preview snapshot queries the cached grid geometry once rather than once per cell");
        Vector<Rect> populated_runtime_cells;
        populated_runtime_grid->GetCellRects(populated_runtime_cells);
        Check(populated_runtime_cells == populated_grid_geometry->cell_rects,
              "populated grid uses the same resolved cell geometry contract as the runtime grid");
        Check(populated_runtime_grid->GetResolvedCellGeometryQueryCount() == populated_query_count + 1,
              "runtime grid geometry is still served from cache for the comparison read");
    }

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
              Format("UiButton content inset applies (got %d,%d,%d,%d)",
                     button->GetContentInset().left,
                     button->GetContentInset().top,
                     button->GetContentInset().right,
                     button->GetContentInset().bottom));
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
    Check(commands.SetProperty(node, "accent_color", Color(12, 34, 56),
                               UiDesignerImpactControlState,
                               "Set serializable color"),
          "color property committed");
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
    Check(roundtrip.GetProperty(node, "accent_color") == Color(12, 34, 56),
          "color property survives JSON round trip");

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
    overlay.Set(node, UiDesignerTransientValueKind::NormalProperty,
                "text", "Transient");
    Check(overlay.Resolve(node, UiDesignerTransientValueKind::NormalProperty,
                          "text", "Hello") == "Transient",
          "transient overlay");
    overlay.Remove(node, UiDesignerTransientValueKind::NormalProperty,
                   "text");
    Check(overlay.Resolve(node, UiDesignerTransientValueKind::NormalProperty,
                          "text", "Hello") == "Hello",
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
    Check(session.State().selection.nodes.IsEmpty(),
          "blank session starts without a selected node");
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

    UiDesignerSession override_session;
    override_session.NewDocument("blank");
    UiDesignerNodeId override_button = override_session.AddControl("UiButton");
    override_session.Select(override_button, false);
    Check(!override_session.ThemeOverrideModel().Find("role"),
          "ordinary properties stay out of theme overrides");
    Check(override_session.ThemeOverrideModel().Find("icon_normal") != nullptr,
          "button theme overrides populate for the selected control");
    const int override_structure_before =
        override_session.ThemeOverrideModel().GetStructureRevision();
    Check(override_session.PreviewThemeOverride(
              "icon_normal", Color(12, 34, 56), error),
          "theme override preview succeeds: " + error);
    Check(override_session.ThemeOverrideModel().Find("icon_normal") &&
              override_session.ThemeOverrideModel().Find("icon_normal")->value ==
                  Color(12, 34, 56),
          "theme override model receives the preview value");
    Check(override_session.CommitThemeOverride(
              "icon_normal", Color(12, 34, 56), error),
          "theme override commit succeeds: " + error);
    Check(override_session.Document().GetThemeOverride(
              override_button, "icon_normal") == Color(12, 34, 56),
          "theme override persists to the document");
    Check(override_session.ThemeOverrideModel().GetStructureRevision() ==
              override_structure_before,
          "theme override commit keeps the model structure stable");
    Check(override_session.ResetThemeOverride("icon_normal", error),
          "theme override reset succeeds: " + error);
    Check(IsNull(override_session.Document().GetThemeOverride(
              override_button, "icon_normal")),
          "theme override reset clears the authored override");

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
    Check(preview_session.PreviewOverlay().Has(
              transient_box, UiDesignerTransientValueKind::NormalProperty,
              "inset"),
          "transient inset is tracked by node/property");
    preview_session.CancelPreview();
    Check(!preview_session.PreviewOverlay().Has(
              transient_box, UiDesignerTransientValueKind::NormalProperty,
              "inset"),
          "cancel clears only tracked transient properties");
    Check(preview_session.Document().GetProperty(transient_box, "inset") == 8,
          "cancel leaves canonical inset unchanged");
    Check(preview_session.Commands().GetHistoryPosition() == history_before_preview_cancel,
          "cancel preview creates no undo command");

    UiDesignerResizeHistory resize_history;
    for(int i = 0; i < UiDesignerResizeHistory::CAPACITY + 5; i++) {
        UiDesignerResizeSample sample;
        sample.sequence = (uint64)i + 1;
        sample.total_ms = (double)(i + 1);
        resize_history.Add(sample);
    }
    Check(resize_history.GetCount() == UiDesignerResizeHistory::CAPACITY,
          "resize history keeps fixed capacity");
    Check(resize_history.GetLatest().sequence == (uint64)UiDesignerResizeHistory::CAPACITY + 5,
          "resize history preserves overwrite order");
    Check(resize_history.GetLatestDuration() ==
              (double)(UiDesignerResizeHistory::CAPACITY + 5),
          "resize history latest duration tracks the newest sample");
    resize_history.Clear();
    Check(resize_history.IsEmpty(), "resize history clears to empty");

    UiBoxLayout resize_box(UiDirection::H);
    resize_box.SetRect(0, 0, 240, 80);
    auto *resize_box_button = new UiButton;
    resize_box_button->SetText("One");
    resize_box.Add(*resize_box_button);
    const int box_layout_count_before = resize_box.GetLayoutCallCount();
    resize_box.Layout();
    resize_box.Layout();
    Check(resize_box.GetLayoutCallCount() == box_layout_count_before + 2,
          "UiBoxLayout layout counter counts actual layout calls");
    Check(resize_box.GetLastLayoutDurationMs() < 0,
          "UiBoxLayout does not collect unconditional timing");

    UiGridLayout resize_grid;
    resize_grid.SetRect(0, 0, 240, 120);
    auto *resize_grid_button = new UiButton;
    resize_grid_button->SetText("Cell");
    resize_grid.Add(*resize_grid_button, 0, 0, false, false);
    const int grid_layout_count_before = resize_grid.GetLayoutCallCount();
    resize_grid.Layout();
    resize_grid.Layout();
    Check(resize_grid.GetLayoutCallCount() == grid_layout_count_before + 2,
          "UiGridLayout layout counter counts actual layout calls");
    Check(resize_grid.GetLastLayoutDurationMs() < 0,
          "UiGridLayout does not collect unconditional timing");

    const UiDesignerControlSpec* resize_box_spec = catalog.Find("UiBoxLayout");
    UiDesignerDocument resize_document;
    UiDesignerCommandService resize_commands(resize_document);
    UiDesignerNodeId resize_root_box = resize_commands.AddNode(
        "UiBoxLayout", "resize_root_box", resize_document.GetRootId(),
        resize_box_spec ? resize_box_spec->node_flags : 0,
        resize_box_spec ? resize_box_spec->defaults : ValueMap(), "Add resize box");
    UiDesignerNodeId resize_child = resize_commands.AddNode(
        "UiButton", "resize_child", resize_root_box,
        button ? button->node_flags : 0,
        button ? button->defaults : ValueMap(), "Add resize child");
    UiDesignerSelection resize_selection;
    UiDesignerPreviewCanvas resize_preview;
    resize_preview.SetRect(0, 0, 512, 250);
    resize_preview.Bind(&resize_document, &catalog, nullptr, &resize_selection);
    resize_preview.RebuildDocument();
    const UiDesignerNodeId resize_root = resize_document.GetRootId();
    const uint64 child_generation_before = resize_preview.GetInstanceGeneration(resize_child);
    const int live_instances_before = resize_preview.GetLiveInstanceCount();
    const Size canonical_size_before = resize_document.GetVirtualSize();
    Check(resize_preview.GetNodeRect(resize_root).Size() == canonical_size_before,
          "preview starts from canonical document size");
    resize_preview.SetTransientVirtualSize(Size(640, 360));
    resize_preview.Layout();
    const UiDesignerGeometrySnapshot& resize_geometry = resize_preview.GetGeometrySnapshot();
    Check(resize_document.GetVirtualSize() == canonical_size_before,
          "transient resize leaves canonical document size unchanged");
    Check(resize_preview.GetInstanceGeneration(resize_child) == child_generation_before,
          "transient resize keeps existing instance generations stable");
    Check(resize_preview.GetLiveInstanceCount() == live_instances_before,
          "transient resize does not reconstruct live instances");
    Check(resize_preview.GetNodeRect(resize_root).Size() == Size(640, 360),
          "transient resize updates the preview root rectangle");
    const UiDesignerDropRegion* transient_root_region =
        resize_geometry.HitDropRegion(Point(10, 10));
    Check(transient_root_region != nullptr,
          "transient resize keeps drop hit testing alive");
    resize_preview.ClearTransientVirtualSize();
    resize_preview.Layout();
    Check(resize_preview.GetNodeRect(resize_root).Size() == canonical_size_before,
          "clearing transient size restores canonical preview geometry");

    {
    auto build_preview = [&](UiDesignerDocument& document,
                             UiDesignerPreviewCanvas& preview,
                             UiDesignerSelection& selection) {
        preview.SetRect(0, 0, 512, 250);
        preview.Bind(&document, &catalog, nullptr, &selection);
        preview.RebuildDocument();
    };

    UiDesignerDocument standard_button_document;
    UiDesignerCommandService standard_button_commands(standard_button_document);
    UiDesignerNodeId standard_button = standard_button_commands.AddNode(
        "UiButton", "standard_button", standard_button_document.GetRootId(),
        button ? button->node_flags : 0,
        button ? button->defaults : ValueMap(), "Add standard button");
    UiDesignerSelection standard_button_selection;
    UiDesignerPreviewCanvas standard_button_preview;
    build_preview(standard_button_document, standard_button_preview, standard_button_selection);
    if(auto *runtime_button = dynamic_cast<UiButton *>(
            standard_button_preview.FindRuntime(standard_button))) {
    }

    UiDesignerDocument accent_button_document;
    UiDesignerCommandService accent_button_commands(accent_button_document);
    UiDesignerNodeId accent_button = accent_button_commands.AddNode(
        "UiButton", "accent_button", accent_button_document.GetRootId(),
        button ? button->node_flags : 0,
        button ? button->defaults : ValueMap(), "Add accent button");
    Check(accent_button_commands.SetProperty(
        accent_button, "role", "Accent",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set accent role"),
          "accent button role command");
    UiDesignerSelection accent_button_selection;
    UiDesignerPreviewCanvas accent_button_preview;
    build_preview(accent_button_document, accent_button_preview, accent_button_selection);
    if(auto *runtime_button = dynamic_cast<UiButton *>(
            accent_button_preview.FindRuntime(accent_button))) {
        Check(runtime_button->HasCustomStyle(),
              "accent Button without overrides uses a custom style");
        Check(runtime_button->GetStyle().palette.icon[ST_NORMAL] ==
                  UiTheme::ResolveButton(UiRole::Accent).palette.icon[ST_NORMAL],
              "accent Button uses the Accent icon ink");
    }

    UiDesignerDocument override_button_document;
    UiDesignerCommandService override_button_commands(override_button_document);
    UiDesignerNodeId override_button = override_button_commands.AddNode(
        "UiButton", "override_button", override_button_document.GetRootId(),
        button ? button->node_flags : 0,
        button ? button->defaults : ValueMap(), "Add override button");
    Check(override_button_commands.SetProperty(
        override_button, "role", "Accent",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set accent role"),
          "override button role command");
    Check(override_button_commands.SetThemeOverride(
        override_button, "font_size", 24,
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set button font size"),
          "override button font-size command");
    Check(override_button_commands.SetProperty(
        override_button, "align_h", "Right",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set button align h"),
          "override button align_h command");
    Check(override_button_commands.SetProperty(
        override_button, "align_v", "Top",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set button align v"),
          "override button align_v command");
    Check(override_button_commands.SetProperty(
        override_button, "icon_side", "Right",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set button icon side"),
          "override button icon_side command");
    Check(override_button_commands.SetProperty(
        override_button, "content_gap", 9,
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set button content gap"),
          "override button content_gap command");
    Check(override_button_commands.SetProperty(
        override_button, "content_inset_left", 1,
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set button inset left"),
          "override button inset left command");
    Check(override_button_commands.SetProperty(
        override_button, "content_inset_top", 2,
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set button inset top"),
          "override button inset top command");
    Check(override_button_commands.SetProperty(
        override_button, "content_inset_right", 3,
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set button inset right"),
          "override button inset right command");
    Check(override_button_commands.SetProperty(
        override_button, "content_inset_bottom", 4,
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set button inset bottom"),
          "override button inset bottom command");
    UiDesignerSelection override_button_selection;
    UiDesignerPreviewCanvas override_button_preview;
    build_preview(override_button_document, override_button_preview, override_button_selection);
    if(auto *runtime_button = dynamic_cast<UiButton *>(
            override_button_preview.FindRuntime(override_button))) {
        Check(runtime_button->HasCustomStyle(),
              "accent Button with overrides keeps custom style");
        Check(runtime_button->GetStyle().font.GetHeight() == 24,
              "accent Button keeps authored font size");
        Check(runtime_button->GetStyle().align_h == UiAlign::RIGHT &&
              runtime_button->GetStyle().align_v == UiAlign::TOP,
              "accent Button keeps authored alignment");
        Check(runtime_button->GetStyle().icon_side == UiAlign::RIGHT,
              "accent Button keeps authored icon side");
        Check(runtime_button->GetStyle().content_gap == 9,
              "accent Button keeps authored content gap");
        Check(runtime_button->GetStyle().metrics.content_margin ==
                  Rect(DPI(1), DPI(2), DPI(3), DPI(4)),
              "accent Button keeps authored content inset");
    }
    Check(override_button_commands.RemoveThemeOverride(
        override_button, "font_size",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Remove button font size"),
          "remove final button override command");
    override_button_preview.RebuildDocument();
    if(auto *runtime_button = dynamic_cast<UiButton *>(
            override_button_preview.FindRuntime(override_button))) {
        Check(runtime_button->HasCustomStyle(),
              "accent Button retains custom style after the last override is removed");
        Check(runtime_button->GetStyle().font.GetHeight() != 24,
              "accent Button no longer keeps the removed font override");
    }

    UiDesignerDocument subtle_tool_document;
    UiDesignerCommandService subtle_tool_commands(subtle_tool_document);
    const UiDesignerControlSpec* tool_button_spec = catalog.Find("UiToolButton");
    UiDesignerNodeId subtle_tool = subtle_tool_commands.AddNode(
        "UiToolButton", "subtle_tool", subtle_tool_document.GetRootId(),
        tool_button_spec ? tool_button_spec->node_flags : 0,
        tool_button_spec ? tool_button_spec->defaults : ValueMap(),
        "Add subtle tool button");
    Check(subtle_tool_commands.SetProperty(
        subtle_tool, "role", "Subtle",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set subtle role"),
          "subtle tool button role command");
    UiDesignerSelection subtle_tool_selection;
    UiDesignerPreviewCanvas subtle_tool_preview;
    build_preview(subtle_tool_document, subtle_tool_preview, subtle_tool_selection);
    if(auto *runtime_tool = dynamic_cast<UiToolButton *>(
            subtle_tool_preview.FindRuntime(subtle_tool))) {
        Check(runtime_tool->HasCustomStyle(),
              "subtle ToolButton without overrides uses a custom style");
        Check(runtime_tool->GetStyle().palette.icon[ST_NORMAL] ==
                  UiTheme::ResolveToolButton(UiRole::Subtle).palette.icon[ST_NORMAL],
              "subtle ToolButton uses the resolved tool-button icon ink");
    }

    UiDesignerDocument shadow_button_document;
    UiDesignerCommandService shadow_button_commands(shadow_button_document);
    UiDesignerNodeId shadow_button = shadow_button_commands.AddNode(
        "UiButton", "shadow_button", shadow_button_document.GetRootId(),
        button ? button->node_flags : 0,
        button ? button->defaults : ValueMap(), "Add shadow button");
    Check(shadow_button_commands.SetProperty(
        shadow_button, "role", "Accent",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set accent role"),
          "shadow button role command");
    Check(shadow_button_commands.SetThemeOverride(
        shadow_button, "shadow_distance", 12,
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set shadow distance"),
          "shadow distance command");
    Check(shadow_button_commands.SetThemeOverride(
        shadow_button, "shadow_color", Color(12, 34, 56),
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set shadow color"),
          "shadow color command");
    UiDesignerSelection shadow_button_selection;
    UiDesignerPreviewCanvas shadow_button_preview;
    build_preview(shadow_button_document, shadow_button_preview, shadow_button_selection);
    if(auto *runtime_button = dynamic_cast<UiButton *>(
            shadow_button_preview.FindRuntime(shadow_button))) {
        Check(!runtime_button->GetStyle().metrics.shadow.enabled,
              "shadow subfields do not enable the master shadow flag");
        Check(runtime_button->GetStyle().metrics.shadow.distance == 12 &&
              runtime_button->GetStyle().metrics.shadow.color == Color(12, 34, 56),
              "shadow subfields still author their values");
    }

    UiDesignerDocument tree_document;
    UiDesignerCommandService tree_commands(tree_document);
    UiDesignerNodeId tree_node = tree_commands.AddNode(
        "UiTree", "tree_node", tree_document.GetRootId(),
        tree ? tree->node_flags : 0, tree ? tree->defaults : ValueMap(),
        "Add tree");
    Check(tree_commands.SetData(tree_node, "root", disabled_root,
                                UiDesignerImpactStructure, "Populate preview tree"),
          "preview tree data command commits");
    UiDesignerSelection tree_selection;
    UiDesignerPreviewCanvas tree_preview;
    build_preview(tree_document, tree_preview, tree_selection);
    if(auto *runtime_tree = dynamic_cast<UiTree *>(tree_preview.FindRuntime(tree_node))) {
        Check(!runtime_tree->HasCustomStyle(), "UiTree no overrides clears stale custom style");
        Check(runtime_tree->GetInternalModel().GetChildCount(
                  runtime_tree->GetInternalModel().Root()) == 2,
              "UiTree preview projects authored root children");
        UiTreeNodeRef nested = runtime_tree->GetInternalModel().GetChild(
            runtime_tree->GetInternalModel().Root(), 1);
        Check(runtime_tree->GetInternalModel().GetChildCount(nested) == 1,
              "UiTree preview projects nested children");
        UiTreeNodeRef disabled = runtime_tree->GetInternalModel().GetChild(
            runtime_tree->GetInternalModel().Root(), 0);
        Check(!runtime_tree->GetInternalModel().Get(disabled).enabled,
              "UiTree preview projects disabled item state");
    }
    tree_document.WhenChanged = [&](const UiDesignerChangeSet& changes) {
        tree_preview.ApplyChangeSet(changes);
    };
    ValueMap updated_tree_root = tree_root;
    ValueArray updated_tree_children = (ValueArray)updated_tree_root.GetValue(
        updated_tree_root.Find("children"));
    ValueMap updated_tree_item = (ValueMap)updated_tree_children[0];
    updated_tree_item.Set("text", "Updated first");
    ValueArray rebuilt_tree_children;
    for(int i = 0; i < updated_tree_children.GetCount(); i++)
        rebuilt_tree_children.Add(i == 0 ? Value(updated_tree_item)
                                         : updated_tree_children[i]);
    updated_tree_children = rebuilt_tree_children;
    updated_tree_root.Set("children", updated_tree_children);
    Check(tree_commands.SetData(tree_node, "root", updated_tree_root,
                                UiDesignerImpactStructure, "Update tree preview"),
          "UiTree data change commits through preview projection");
    if(auto *runtime_tree = dynamic_cast<UiTree *>(tree_preview.FindRuntime(tree_node))) {
        UiTreeNodeRef updated = runtime_tree->GetInternalModel().GetChild(
            runtime_tree->GetInternalModel().Root(), 0);
        Check(runtime_tree->GetInternalModel().Get(updated).text == "Updated first",
              "UiTree preview reflects committed data edit");
    }

    UiDesignerDocument list_document;
    UiDesignerCommandService list_commands(list_document);
    UiDesignerNodeId list_node = list_commands.AddNode(
        "UiList", "list_node", list_document.GetRootId(),
        list ? list->node_flags : 0, list ? list->defaults : ValueMap(),
        "Add list");
    ValueMap preview_list_root;
    ValueArray preview_list_items;
    ValueMap preview_list_item;
    preview_list_item.Set("text", "Preview item");
    preview_list_item.Set("checked", true);
    preview_list_items.Add(preview_list_item);
    preview_list_root.Set("items", preview_list_items);
    Check(list_commands.SetData(list_node, "root", preview_list_root,
                                UiDesignerImpactStructure, "Populate list preview"),
          "list preview data command commits");
    UiDesignerSelection list_selection;
    UiDesignerPreviewCanvas list_preview;
    build_preview(list_document, list_preview, list_selection);
    if(auto *runtime_list = dynamic_cast<UiList *>(list_preview.FindRuntime(list_node))) {
        Check(!runtime_list->HasCustomStyle(), "UiList no overrides clears stale custom style");
        Check(runtime_list->GetInternalModel().GetCount() == 1 &&
              runtime_list->GetInternalModel().Get(0).text == "Preview item" &&
              runtime_list->GetInternalModel().Get(0).checked,
              "UiList preview projects canonical item data");
    }
    list_document.WhenChanged = [&](const UiDesignerChangeSet& changes) {
        list_preview.ApplyChangeSet(changes);
    };
    ValueMap updated_list_root = preview_list_root;
    ValueArray updated_list_items = (ValueArray)updated_list_root.GetValue(
        updated_list_root.Find("items"));
    ValueMap updated_list_item = (ValueMap)updated_list_items[0];
    updated_list_item.Set("text", "Updated preview item");
    ValueArray rebuilt_list_items;
    for(int i = 0; i < updated_list_items.GetCount(); i++)
        rebuilt_list_items.Add(i == 0 ? Value(updated_list_item)
                                      : updated_list_items[i]);
    updated_list_items = rebuilt_list_items;
    updated_list_root.Set("items", updated_list_items);
    Check(list_commands.SetData(list_node, "root", updated_list_root,
                                UiDesignerImpactStructure, "Update list preview"),
          "UiList data change commits through preview projection");
    if(auto *runtime_list = dynamic_cast<UiList *>(list_preview.FindRuntime(list_node)))
        Check(runtime_list->GetInternalModel().GetCount() == 1 &&
              runtime_list->GetInternalModel().Get(0).text == "Updated preview item",
              "UiList preview reflects committed data edit");

    UiDesignerDocument menu_document;
    UiDesignerCommandService menu_commands(menu_document);
    UiDesignerNodeId menu_node = menu_commands.AddNode(
        "UiMenu", "menu_node", menu_document.GetRootId(),
        menu ? menu->node_flags : 0, menu ? menu->defaults : ValueMap(),
        "Add menu");
    UiDesignerSelection menu_selection;
    UiDesignerPreviewCanvas menu_preview;
    build_preview(menu_document, menu_preview, menu_selection);
    if(auto *runtime_menu = dynamic_cast<UiMenu *>(menu_preview.FindRuntime(menu_node)))
        Check(!runtime_menu->HasCustomStyle(), "UiMenu no overrides clears stale custom style");
    }

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

    ValueMap list_request;
    list_request.Set("method", "list_controls");
    Value list_response = automation.Handle(list_request);
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

    UiDesignerDocument standard_button_document;
    UiDesignerCommandService standard_button_commands(standard_button_document);
    UiDesignerNodeId standard_button = standard_button_commands.AddNode(
        "UiButton", "standard_button", standard_button_document.GetRootId(),
        button ? button->node_flags : 0,
        button ? button->defaults : ValueMap(), "Add standard button");
    Check(standard_button != 0, "standard button created");
    UiDesignerGeneratedProject standard_button_generated =
        generator.Generate(standard_button_document, "StandardButtonWindow");
    Check(standard_button_generated.source.Find("SetCustomStyle(") < 0 &&
              standard_button_generated.source.Find("UiButton::Style") < 0,
          "standard Button emits no redundant style block");

    UiDesignerDocument role_button_document;
    UiDesignerCommandService role_button_commands(role_button_document);
    UiDesignerNodeId role_button = role_button_commands.AddNode(
        "UiButton", "role_button", role_button_document.GetRootId(),
        button ? button->node_flags : 0,
        button ? button->defaults : ValueMap(), "Add role button");
    Check(role_button_commands.SetProperty(
        role_button, "role", "Accent",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set accent role"),
          "accent role command");
    UiDesignerGeneratedProject role_button_generated =
        generator.Generate(role_button_document, "RoleButtonWindow");
    Check(role_button_generated.source.Find(
              "SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent))") >= 0,
          "Accent Button emits role-only custom style");
    Check(role_button_generated.source.Find("UiButton::Style") < 0,
          "Accent Button with no overrides emits no patched style block");

    UiDesignerDocument override_button_document;
    UiDesignerCommandService override_button_commands(override_button_document);
    UiDesignerNodeId override_theme_button = override_button_commands.AddNode(
        "UiButton", "override_button", override_button_document.GetRootId(),
        button ? button->node_flags : 0,
        button ? button->defaults : ValueMap(), "Add override button");
    Check(override_button_commands.SetProperty(
        override_theme_button, "role", "Accent",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set accent role"),
          "accent role command for override button");
    Check(override_button_commands.SetThemeOverride(
        override_theme_button, "font_size", 24,
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set button font size"),
          "accent button font-size override command");
    UiDesignerGeneratedProject override_button_generated =
        generator.Generate(override_button_document, "OverrideButtonWindow");
    Check(override_button_generated.source.Find(
              "UiTheme::ResolveButton(UiRole::Accent)") >= 0,
          "Accent Button override resolves the selected role");
    Check(override_button_generated.source.Find(".font.Height(24)") >= 0,
          "Accent Button override emits the authored font size");
    Check(override_button_generated.source.Find(".SetCustomStyle(") >= 0,
          "Accent Button with overrides emits a style patch");

    UiDesignerDocument subtle_tool_document;
    UiDesignerCommandService subtle_tool_commands(subtle_tool_document);
    const UiDesignerControlSpec* tool_button_spec = catalog.Find("UiToolButton");
    UiDesignerNodeId subtle_tool = subtle_tool_commands.AddNode(
        "UiToolButton", "subtle_tool", subtle_tool_document.GetRootId(),
        tool_button_spec ? tool_button_spec->node_flags : 0,
        tool_button_spec ? tool_button_spec->defaults : ValueMap(),
        "Add subtle tool button");
    Check(subtle_tool_commands.SetProperty(
        subtle_tool, "role", "Subtle",
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set subtle role"),
          "subtle tool-button role command");
    Check(subtle_tool_commands.SetThemeOverride(
        subtle_tool, "icon_normal", Color(12, 34, 56),
        UiDesignerImpactPaint | UiDesignerImpactCode, "Set tool icon ink"),
          "subtle tool-button icon-ink override command");
    UiDesignerGeneratedProject subtle_tool_generated =
        generator.Generate(subtle_tool_document, "SubtleToolButtonWindow");
    Check(subtle_tool_generated.source.Find(
              "UiTheme::ResolveToolButton(UiRole::Subtle)") >= 0,
          "ToolButton Subtle emits role-only custom style");
    Check(subtle_tool_generated.source.Find(".palette.icon[ST_NORMAL] = Color(12, 34, 56)") >= 0,
          "ToolButton icon ink override is emitted");

    // UiTab Data contract: semantic pages are owned by one Tab, activation is
    // authored state, and the preview follows it without a document rebuild.
    UiDesignerDocument tab_document;
    UiDesignerCommandService tab_commands(tab_document);
    const UiDesignerControlSpec *tab_catalog_spec = catalog.Find("UiTab");
    UiDesignerNodeId tab_a = tab_commands.AddNode(
        "UiTab", "tab_a", tab_document.GetRootId(),
        tab_catalog_spec ? tab_catalog_spec->node_flags : 0,
        tab_catalog_spec ? tab_catalog_spec->defaults : ValueMap(), "Add first Tab");
    UiDesignerNodeId tab_b = tab_commands.AddNode(
        "UiTab", "tab_b", tab_document.GetRootId(),
        tab_spec ? tab_spec->node_flags : 0,
        tab_spec ? tab_spec->defaults : ValueMap(), "Add second Tab");
    String tab_error;
    Check(tab_a && tab_b && catalog.ValidateDocument(tab_document, tab_error),
          "two Tabs validate with globally safe semantic names");
    Check(tab_document.Find(tab_a)->children.GetCount() == 2,
          "new Tab gets two canonical pages");
    const UiDesignerNodeId tab_page = tab_document.Find(tab_a)->children[1];
    Check(tab_commands.SetActiveTabPage(tab_a, tab_page),
          "Set Active updates canonical active_page");
    UiDesignerSelection tab_selection;
    UiDesignerPreviewCanvas tab_preview;
    tab_preview.Bind(&tab_document, &catalog, nullptr, &tab_selection);
    tab_preview.RebuildDocument();
    if(auto *runtime_tab = dynamic_cast<UiTab *>(tab_preview.FindRuntime(tab_a)))
        Check(runtime_tab->GetActiveTab() == 1,
              "preview follows the second active Tab page");
    else
        Check(false, "runtime UiTab exists for active-page projection");
    Check(!tab_commands.RenameTabPage(tab_page, "   "),
          "blank Tab page title is rejected");
    Check(tab_commands.RemoveTabPage(tab_page),
          "active middle/last page removal succeeds atomically");
    Check(tab_document.Find(tab_a)->GetProperty("active_page", (UiDesignerNodeId)0) ==
              tab_document.Find(tab_a)->children[0],
          "page removal selects the deterministic replacement");
    Check(tab_commands.Undo(), "Tab page removal undo succeeds");
    Check(tab_document.Find(tab_page) != nullptr &&
              tab_document.Find(tab_a)->children.GetCount() == 2,
          "Tab page removal undo restores exact page identity");
    Check(tab_commands.Redo(), "Tab page removal redo succeeds");
    Check(!tab_commands.AddTabPage(tab_b, "   "),
          "empty additional page title is rejected");
    UiDesignerNodeId extra = tab_commands.AddTabPage(tab_b, "Extra");
    Check(extra != 0, "additional page is created");
    Check(!tab_commands.MoveTabPage(extra, 99),
          "out of range page move is rejected");
    Check(tab_commands.SetTabPageEnabled(extra, false),
          "page disable command succeeds");
    Check(tab_commands.SetTabPageEnabled(extra, true),
          "page enable command succeeds");
    Check(!tab_commands.SetActiveTabPage(tab_a, extra),
          "activation rejects a page owned by another Tab");
    String tab_json = UiDesignerSerialize(tab_document);
    UiDesignerDocument tab_round_trip;
    String tab_load_error;
    Check(UiDesignerDeserialize(tab_json, tab_round_trip, tab_load_error),
          "Tab JSON round trip succeeds");
    Check(catalog.ValidateDocument(tab_round_trip, tab_load_error),
          "round-tripped Tabs remain valid");
    Check(tab_round_trip.Find(tab_b) && tab_round_trip.Find(tab_b)->children.GetCount() == 3,
          "round trip preserves additional Tab page");

    UiDesignerDocument fit_document;
    UiDesignerCommandService fit_commands(fit_document);
    const UiDesignerControlSpec *fit_grid = catalog.Find("UiGridLayout");
    UiDesignerNodeId fit_grid_id = fit_commands.AddNode(
        "UiGridLayout", "fit_grid", fit_document.GetRootId(),
        fit_grid ? fit_grid->node_flags : 0,
        fit_grid ? fit_grid->defaults : ValueMap(), "Add Fit Grid");
    fit_commands.SetProperty(fit_grid_id, "rows", 2, UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_grid_id, "columns", 2, UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_grid_id, "width_mode", "Fixed", UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_grid_id, "height_mode", "Fixed", UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_grid_id, "fixed_width", 260, UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_grid_id, "fixed_height", 180, UiDesignerImpactLocalLayout);
    const UiDesignerControlSpec *card_spec = catalog.Find("UiTitleCard");
    const UiDesignerControlSpec *button_spec = catalog.Find("UiButton");
    UiDesignerNodeId fit_card = fit_commands.AddNode(
        "UiTitleCard", "fit_card", fit_grid_id,
        card_spec ? card_spec->node_flags : 0,
        card_spec ? card_spec->defaults : ValueMap(), "Add Fit Card");
    UiDesignerNodeId fit_tab = fit_commands.AddNode(
        "UiTab", "fit_tab", fit_grid_id,
        tab_spec ? tab_spec->node_flags : 0,
        tab_spec ? tab_spec->defaults : ValueMap(), "Add Fit Tab");
    UiDesignerNodeId fit_button = fit_commands.AddNode(
        "UiButton", "fit_button", fit_grid_id,
        button ? button->node_flags : 0,
        button ? button->defaults : ValueMap(), "Add Fit Button");
    fit_commands.SetProperty(fit_card, "grid_row", 0, UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_card, "grid_column", 0, UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_card, "width_mode", "Fit", UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_card, "height_mode", "Fit", UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_card, "cell_align_x", "Center", UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_card, "cell_align_y", "Center", UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_tab, "grid_row", 0, UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_tab, "grid_column", 1, UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_button, "grid_row", 1, UiDesignerImpactLocalLayout);
    fit_commands.SetProperty(fit_button, "grid_column", 0, UiDesignerImpactLocalLayout);
    UiDesignerSelection fit_selection;
    UiDesignerPreviewCanvas fit_preview;
    fit_preview.SetRect(0, 0, 512, 250);
    fit_preview.Bind(&fit_document, &catalog, nullptr, &fit_selection);
    fit_preview.RebuildDocument();
    const UiDesignerGeometryRecord *fit_geometry =
        fit_preview.GetGeometrySnapshot().Find(fit_grid_id);
    Check(fit_geometry && fit_geometry->cell_rects.GetCount() == 4,
          "Fit contract preserves a 2x2 Grid");
    for(UiDesignerNodeId child : {fit_card, fit_tab, fit_button}) {
        const UiDesignerNode *child_node = fit_document.Find(child);
        const Rect child_rect = fit_preview.GetNodeRect(child);
        const int cell_index = child_node
            ? (int)child_node->GetProperty("grid_row", 0) * 2 +
              (int)child_node->GetProperty("grid_column", 0) : -1;
        Check(fit_geometry && cell_index >= 0 && cell_index < fit_geometry->cell_rects.GetCount() &&
              fit_geometry->cell_rects[cell_index].Contains(child_rect),
              "Fit child remains inside its assigned Grid cell");
    }
    Check(fit_geometry && fit_geometry->rect == fit_preview.GetNodeRect(fit_grid_id),
          "Grid snapshot keeps the runtime Grid rectangle");
    const Rect grid_client = fit_geometry ? fit_geometry->body : Rect();
    Check(fit_geometry && grid_client.GetWidth() >= 0 && grid_client.GetHeight() >= 0,
          "Grid client rectangle is non-negative");
    if(fit_geometry) {
        for(int i = 0; i < fit_geometry->cell_rects.GetCount(); i++) {
            Check(grid_client.Contains(fit_geometry->cell_rects[i]),
                  "Every resolved Grid cell stays inside the client rectangle");
        }
    }
    Check(fit_preview.GetNodeRect(fit_card).GetWidth() >= 0 &&
          fit_preview.GetNodeRect(fit_card).GetHeight() >= 0,
          "Title Card resolved rectangle is non-negative");
    Check(fit_preview.GetNodeRect(fit_tab).GetWidth() >= 0 &&
          fit_preview.GetNodeRect(fit_tab).GetHeight() >= 0,
          "UiTab resolved rectangle is non-negative");
    Check(fit_preview.GetNodeRect(fit_button).GetWidth() >= 0 &&
          fit_preview.GetNodeRect(fit_button).GetHeight() >= 0,
          "UiButton resolved rectangle is non-negative");
    Check(fit_geometry && fit_geometry->cell_rects.GetCount() == 2 * 2,
          "Grid remains exactly two rows by two columns");
    Check(fit_geometry && fit_geometry->cell_rects[0].left >= grid_client.left,
          "Grid first cell honours the left client edge");
    Check(fit_geometry && fit_geometry->cell_rects[0].top >= grid_client.top,
          "Grid first cell honours the top client edge");
    Check(fit_geometry && fit_geometry->cell_rects.Top().right <= grid_client.right,
          "Grid first row does not cross the right client edge");
    Check(fit_geometry && fit_geometry->cell_rects.Top().bottom <= grid_client.bottom,
          "Grid first row does not cross the bottom client edge");
    Check(fit_geometry && fit_geometry->cell_rects.GetCount() == 4,
          "Bounded allocation publishes all four cells");
    Check(fit_geometry && fit_geometry->cell_rects[3].right <= grid_client.right,
          "Grid final column remains bounded after rounding");
    Check(fit_geometry && fit_geometry->cell_rects[3].bottom <= grid_client.bottom,
          "Grid final row remains bounded after rounding");

    // Focused reproduction of the reported Title Card insertion defects.
    UiDesignerSession card_session;
    card_session.NewDocument("blank");
    UiDesignerCommandService& card_commands = card_session.Commands();
    const UiDesignerNodeId card_grid = card_commands.AddNode(
        "UiGridLayout", "card_grid", card_session.Document().GetRootId(),
        grid_spec ? grid_spec->node_flags : 0,
        grid_spec ? grid_spec->defaults : ValueMap(), "Add Title Card Grid");
    card_commands.SetProperty(card_grid, "rows", 2, UiDesignerImpactLocalLayout);
    card_commands.SetProperty(card_grid, "columns", 2, UiDesignerImpactLocalLayout);
    card_commands.SetProperty(card_grid, "width_mode", "Fixed", UiDesignerImpactLocalLayout);
    card_commands.SetProperty(card_grid, "height_mode", "Fixed", UiDesignerImpactLocalLayout);
    card_commands.SetProperty(card_grid, "fixed_width", 320, UiDesignerImpactLocalLayout);
    card_commands.SetProperty(card_grid, "fixed_height", 220, UiDesignerImpactLocalLayout);
    UiDesignerDropPlan card_plan = card_session.PlanAddControl(
        "UiTitleCard", card_grid, Point(240, 120), true, -1, 0, 1);
    UiDesignerNodeId inserted_card = 0;
    String card_drop_error;
    Check(card_session.ExecuteDrop(card_plan, &inserted_card, card_drop_error),
          "Title Card insertion reproduction executes: " + card_drop_error);
    const UiDesignerNode* inserted_card_node = card_session.Document().Find(inserted_card);
    Check(inserted_card_node &&
              inserted_card_node->GetProperty("icon", "") == "ICON_DESIGN_DESCRIPTION_48",
          "Reproduction canonical Title Card icon is the description glyph");
    Check(inserted_card_node && inserted_card_node->GetProperty("width_mode", "") == "Expand" &&
              inserted_card_node->GetProperty("height_mode", "") == "Expand" &&
              inserted_card_node->GetProperty("cell_align_x", "") == "Stretch" &&
              inserted_card_node->GetProperty("cell_align_y", "") == "Stretch",
          "Reproduction Title Card uses container placement defaults");
    UiDesignerSelection card_selection;
    UiDesignerPreviewCanvas card_preview;
    card_preview.SetRect(0, 0, 512, 320);
    card_preview.Bind(&card_session.Document(), &catalog, nullptr, &card_selection);
    card_preview.RebuildDocument();
    const UiDesignerGeometryRecord* card_grid_geometry =
        card_preview.GetGeometrySnapshot().Find(card_grid);
    const Rect card_cell = card_grid_geometry && card_grid_geometry->cell_rects.GetCount() > 1
        ? card_grid_geometry->cell_rects[1] : Rect();
    const Rect card_rect = card_preview.GetNodeRect(inserted_card);
    Check(card_grid_geometry && card_cell == card_rect,
          "Reproduction Title Card fills its assigned Grid cell");
    if(UiTitleCard* runtime_card = dynamic_cast<UiTitleCard *>(card_preview.FindRuntime(inserted_card)))
        Check(!runtime_card->GetContentCellRect().IsEmpty(),
              "Reproduction empty Title Card publishes prospective content geometry");
    Check(card_preview.GetGeometrySnapshot().GetDropRegionCount() > 0,
          "Reproduction publishes a Title Card content drop region");
    const UiDesignerGeometryRecord* card_record =
        card_preview.GetGeometrySnapshot().Find(inserted_card);
    Check(card_record && card_record->drop_target,
          "Selected Title Card is a content drop target");
    UiDesignerDropPlan contract_panel_plan = card_session.PlanAddControl(
        "UiPanel", card_grid, Point(40, 40), true, -1, 0, 0);
    UiDesignerNodeId panel_node = 0;
    String panel_drop_error;
    Check(card_session.ExecuteDrop(contract_panel_plan, &panel_node, panel_drop_error),
          "Panel insertion into the Grid succeeds: " + panel_drop_error);
    card_preview.RebuildDocument();
    const UiDesignerGeometryRecord* panel_record =
        card_preview.GetGeometrySnapshot().Find(panel_node);
    const UiDesignerDropRegion* panel_region = nullptr;
    for(const UiDesignerDropRegion& region : card_preview.GetGeometrySnapshot().GetDropRegions())
        if(region.owner == panel_node && region.kind == UiDesignerDropRegionKind::PanelBody)
            panel_region = &region;
    Check(panel_record && panel_record->drop_target && panel_region,
          "Selected Panel publishes its inner body region");
    if(panel_region) {
        const UiDesignerDropRegion* winning = card_preview.GetGeometrySnapshot().HitDropRegion(
            panel_region->rect.CenterPoint());
        Check(winning && winning->owner == panel_node &&
                  winning->kind == UiDesignerDropRegionKind::PanelBody,
              "Panel body wins hit testing over its surrounding Grid cell");
        UiDesignerDropPlan nested_card_plan = card_session.PlanAddControl(
            "UiTitleCard", panel_node, panel_region->rect.CenterPoint(), true);
        Check(nested_card_plan.valid && nested_card_plan.parent == panel_node,
              "Title Card drop over Panel resolves to the Panel owner");
    }
    if(UiTitleCard* runtime_card = dynamic_cast<UiTitleCard *>(card_preview.FindRuntime(inserted_card))) {
        Check(runtime_card->HasMedia(), "Inserted Title Card runtime media is present");
        Check(card_preview.RebuildSubtree(inserted_card),
              "Title Card subtree rebuild succeeds");
        Check(dynamic_cast<UiTitleCard *>(card_preview.FindRuntime(inserted_card))->HasMedia(),
              "Title Card icon survives subtree rebuild");
    }
    Check(card_commands.SetProperty(inserted_card, "icon", "None",
                                    UiDesignerImpactControlState | UiDesignerImpactLocalLayout),
          "Title Card icon can be cleared");
    card_preview.RebuildDocument();
    Check(!dynamic_cast<UiTitleCard *>(card_preview.FindRuntime(inserted_card))->HasMedia(),
          "Title Card None clears runtime media");
    Check(card_commands.Undo(), "Title Card icon undo succeeds");
    card_preview.RebuildDocument();
    Check(dynamic_cast<UiTitleCard *>(card_preview.FindRuntime(inserted_card))->HasMedia(),
          "Title Card icon undo restores media");
    Check(card_commands.Redo(), "Title Card icon redo succeeds");
    card_preview.RebuildDocument();
    Check(!dynamic_cast<UiTitleCard *>(card_preview.FindRuntime(inserted_card))->HasMedia(),
          "Title Card icon redo clears media");

    const UiDesignerControlSpec* card_box_spec = catalog.Find("UiBoxLayout");
    const UiDesignerNodeId card_box = card_commands.AddNode(
        "UiBoxLayout", "card_content", inserted_card,
        card_box_spec ? card_box_spec->node_flags : 0,
        card_box_spec ? card_box_spec->defaults : ValueMap(),
        "Add Title Card content");
    const UiDesignerControlSpec* card_button_spec = catalog.Find("UiButton");
    const UiDesignerNodeId card_button = card_commands.AddNode(
        "UiButton", "card_button", card_box,
        card_button_spec ? card_button_spec->node_flags : 0,
        card_button_spec ? card_button_spec->defaults : ValueMap(),
        "Add Title Card Button");
    card_preview.RebuildDocument();
    UiTitleCard* occupied_card = dynamic_cast<UiTitleCard *>(card_preview.FindRuntime(inserted_card));
    Check(occupied_card && occupied_card->GetContentCell() == card_preview.FindRuntime(card_box),
          "Title Card child attaches through SetContentCell");
    bool found_occupied_region = false;
    for(const UiDesignerDropRegion& region : card_preview.GetGeometrySnapshot().GetDropRegions())
        if(region.owner == inserted_card && region.kind == UiDesignerDropRegionKind::TitleCardContent) {
            found_occupied_region = true;
            Check(region.occupied, "Occupied Title Card region is marked occupied");
        }
    Check(found_occupied_region, "Occupied Title Card publishes its content region");
    UiDesignerDropPlan second_card_child = card_session.PlanAddControl(
        "UiPanel", inserted_card, Point(0, 0), true);
    Check(!second_card_child.valid &&
              second_card_child.reason.Find("one direct content child") >= 0,
          "Title Card rejects a second direct child with guidance");
    Check(card_commands.SetProperty(inserted_card, "icon", "ICON_DESIGN_DESCRIPTION_48",
                                    UiDesignerImpactControlState | UiDesignerImpactLocalLayout),
          "Title Card authored icon is restored for code generation");
    String card_json = UiDesignerSerialize(card_session.Document());
    UiDesignerDocument card_round_trip;
    String card_json_error;
    Check(UiDesignerDeserialize(card_json, card_round_trip, card_json_error),
          "Title Card JSON round trip succeeds");
    Check(card_round_trip.Find(inserted_card) &&
              card_round_trip.Find(inserted_card)->GetProperty("icon", "") ==
                  "ICON_DESIGN_DESCRIPTION_48" &&
              card_round_trip.Find(card_box),
          "Title Card JSON preserves icon and content child");
    const UiDesignerControlSpec* nested_group_spec = catalog.Find("UiGroupPanel");
    const UiDesignerNodeId nested_group = card_commands.AddNode(
        "UiGroupPanel", "generated_group", card_session.Document().GetRootId(),
        nested_group_spec ? nested_group_spec->node_flags : 0,
        nested_group_spec ? nested_group_spec->defaults : ValueMap(),
        "Add generated GroupPanel");
    const UiDesignerControlSpec* nested_button_spec = catalog.Find("UiButton");
    card_commands.AddNode("UiButton", "generated_group_button", nested_group,
                          nested_button_spec ? nested_button_spec->node_flags : 0,
                          nested_button_spec ? nested_button_spec->defaults : ValueMap(),
                          "Add generated GroupPanel Button");
    const UiDesignerControlSpec* nested_tab_spec = catalog.Find("UiTab");
    const UiDesignerNodeId nested_tab = card_commands.AddNode(
        "UiTab", "generated_tab", card_session.Document().GetRootId(),
        nested_tab_spec ? nested_tab_spec->node_flags : 0,
        nested_tab_spec ? nested_tab_spec->defaults : ValueMap(), "Add generated Tab");
    const UiDesignerNode* nested_tab_node = card_session.Document().Find(nested_tab);
    const UiDesignerNodeId nested_page = nested_tab_node && !nested_tab_node->children.IsEmpty()
        ? nested_tab_node->children[0] : 0;
    card_commands.AddNode("UiButton", "generated_tab_button", nested_page,
                          nested_button_spec ? nested_button_spec->node_flags : 0,
                          nested_button_spec ? nested_button_spec->defaults : ValueMap(),
                          "Add generated Tab Button");
    const UiDesignerControlSpec* nested_accordion_spec = catalog.Find("UiAccordion");
    const UiDesignerNodeId nested_accordion = card_commands.AddNode(
        "UiAccordion", "generated_accordion", card_session.Document().GetRootId(),
        nested_accordion_spec ? nested_accordion_spec->node_flags : 0,
        nested_accordion_spec ? nested_accordion_spec->defaults : ValueMap(),
        "Add generated Accordion");
    const UiDesignerNode* nested_accordion_node = card_session.Document().Find(nested_accordion);
    const UiDesignerNodeId nested_section = nested_accordion_node &&
        !nested_accordion_node->children.IsEmpty() ? nested_accordion_node->children[0] : 0;
    card_commands.AddNode("UiButton", "generated_accordion_button", nested_section,
                          nested_button_spec ? nested_button_spec->node_flags : 0,
                          nested_button_spec ? nested_button_spec->defaults : ValueMap(),
                          "Add generated Accordion Button");
    UiDesignerGeneratedProject card_generated = generator.Generate(
        card_session.Document(), "GeneratedTitleCardWindow");
    Check(card_generated.source.Find(".SetContentCell(") >= 0,
          "Generated Title Card uses SetContentCell");
    Check(card_generated.source.Find(".SetMedia(ICON_DESIGN_DESCRIPTION_48()") >= 0,
          "Generated Title Card emits its description media");
    Check(card_generated.source.Find(".SetContent(") >= 0,
          "Generated nested GroupPanel uses SetContent");
    Check(card_generated.source.Find(".GetSectionContent(") >= 0,
          "Generated nested Accordion uses section content attachment");
    const UiDesignerNode* generated_page_node = card_session.Document().Find(nested_page);
    const String generated_page_member = generated_page_node
        ? generated_page_node->name + "_n" + AsString(nested_page) : String();
    Check(!generated_page_member.IsEmpty() &&
              card_generated.source.Find(generated_page_member + ".Add(") >= 0,
          "Generated nested Tab includes its page host");
    UiDesignerExportWriteOptions card_write_options;
    Vector<String> card_written_files;
    String card_write_error;
    Check(UiDesignerWriteGeneratedProject(
              "E:/apps/github/upp_Ui/build/TitleCardGeneratedFixture",
              card_generated, card_write_options, card_written_files, card_write_error),
          "Title Card generated package writes: " + card_write_error);
    Check(card_commands.RemoveNode(card_box, "Remove Title Card content"),
          "Title Card content removal succeeds");
    card_preview.RebuildDocument();
    Check(dynamic_cast<UiTitleCard *>(card_preview.FindRuntime(inserted_card))->GetContentCell() == nullptr,
          "Removing Title Card content clears the runtime relationship");
    bool found_empty_region = false;
    for(const UiDesignerDropRegion& region : card_preview.GetGeometrySnapshot().GetDropRegions())
        if(region.owner == inserted_card && region.kind == UiDesignerDropRegionKind::TitleCardContent) {
            found_empty_region = true;
            Check(!region.occupied, "Empty Title Card region is marked available");
            const UiDesignerDropRegion* hit = card_preview.GetGeometrySnapshot().HitDropRegion(
                region.rect.CenterPoint());
            Check(hit && hit->owner == inserted_card &&
                      hit->kind == UiDesignerDropRegionKind::TitleCardContent,
                  "Title Card drag hit testing uses the published content region");
        }
    Check(found_empty_region, "Removing content restores the empty Title Card drop area");

    const String catalog_drag_payload = UiDesignerCatalogDragText("UiButton");
    String parsed_catalog_type;
    Check(UiDesignerParseCatalogDragText(catalog_drag_payload, parsed_catalog_type) &&
              parsed_catalog_type == "UiButton",
          "Catalog drag payload round trips through the shared parser");
    Check(!UiDesignerParseCatalogDragText(UiDesignerNodesDragText(Vector<UiDesignerNodeId>()),
                                          parsed_catalog_type),
          "Catalog parser rejects node drag payloads");

    UiDesignerSession host_session;
    host_session.NewDocument("blank");
    UiDesignerCommandService& host_commands = host_session.Commands();
    const UiDesignerControlSpec* group_spec = catalog.Find("UiGroupPanel");
    const UiDesignerNodeId group = host_commands.AddNode(
        "UiGroupPanel", "group_host", host_session.Document().GetRootId(),
        group_spec ? group_spec->node_flags : 0,
        group_spec ? group_spec->defaults : ValueMap(), "Add GroupPanel");
    UiDesignerDropPlan group_child_plan = host_session.PlanAddControl(
        "UiBoxLayout", group, Point(), false);
    UiDesignerNodeId group_child = 0;
    String group_error;
    Check(group_child_plan.valid && host_session.ExecuteDrop(group_child_plan,
          &group_child, group_error),
          "GroupPanel accepts one content child: " + group_error);
    UiDesignerPreviewCanvas host_preview;
    host_preview.SetRect(0, 0, 320, 220);
    host_preview.Bind(&host_session.Document(), &catalog, nullptr, nullptr);
    host_preview.RebuildDocument();
    UiGroupPanel* runtime_group = dynamic_cast<UiGroupPanel *>(host_preview.FindRuntime(group));
    Check(runtime_group && runtime_group->GetContent() == host_preview.FindRuntime(group_child),
          "GroupPanel child attaches through SetContent");
    bool group_region = false;
    for(const UiDesignerDropRegion& region : host_preview.GetGeometrySnapshot().GetDropRegions())
        if(region.owner == group && region.kind == UiDesignerDropRegionKind::GroupPanelBody)
            group_region = true;
    Check(group_region, "GroupPanel publishes its body drop region");
    Check(!host_session.PlanAddControl("UiPanel", group, Point(), false).valid,
          "GroupPanel rejects a second direct child");
    Check(host_preview.FindGeometry(group) &&
              host_preview.FindGeometry(group)->drop_target,
          "GroupPanel drop-target state comes from content-host metadata");
    UiDesignerGeneratedProject group_generated = generator.Generate(
        host_session.Document(), "GeneratedGroupHost");
    Check(group_generated.source.Find(".SetContent(") >= 0,
          "Generated GroupPanel uses SetContent");

    UiDesignerSession direct_group_session;
    direct_group_session.NewDocument("blank");
    const UiDesignerNodeId direct_group = direct_group_session.Commands().AddNode(
        "UiGroupPanel", "direct_group", direct_group_session.Document().GetRootId(),
        group_spec ? group_spec->node_flags : 0,
        group_spec ? group_spec->defaults : ValueMap(), "Add direct GroupPanel");
    UiDesignerNodeId direct_button = 0;
    String direct_group_error;
    Check(direct_group_session.ExecuteDrop(direct_group_session.PlanAddControl(
              "UiButton", direct_group, Point(), false), &direct_button,
          direct_group_error),
          "GroupPanel accepts a direct Button: " + direct_group_error);

    UiDesignerSession tab_session;
    tab_session.NewDocument("blank");
    const UiDesignerControlSpec* tab_host_spec = catalog.Find("UiTab");
    const UiDesignerNodeId tab = tab_session.Commands().AddNode(
        "UiTab", "tab_host", tab_session.Document().GetRootId(),
        tab_host_spec ? tab_host_spec->node_flags : 0,
        tab_host_spec ? tab_host_spec->defaults : ValueMap(), "Add Tab");
    const UiDesignerNode* tab_node = tab_session.Document().Find(tab);
    const UiDesignerNodeId page = tab_node && tab_node->children.GetCount()
        ? tab_node->children[0] : 0;
    UiDesignerDropPlan page_child_plan = tab_session.PlanAddControl(
        "UiPanel", page, Point(), false);
    UiDesignerNodeId page_child = 0;
    String page_error;
    Check(page_child_plan.valid && tab_session.ExecuteDrop(page_child_plan,
          &page_child, page_error),
          "Tab Page accepts one direct content child: " + page_error);
    Check(!tab_session.PlanAddControl("UiPanel", page, Point(), false).valid,
          "Tab Page rejects a second direct child");
    UiDesignerPreviewCanvas tab_host_preview;
    tab_host_preview.SetRect(0, 0, 320, 220);
    tab_host_preview.Bind(&tab_session.Document(), &catalog, nullptr, nullptr);
    tab_host_preview.RebuildDocument();
    Check(tab_host_preview.FindGeometry(page) && tab_host_preview.FindGeometry(page)->drop_target,
          "Tab Page drop-target state comes from content-host metadata");
    bool tab_page_region = false;
    for(const UiDesignerDropRegion& region : tab_host_preview.GetGeometrySnapshot().GetDropRegions())
        if(region.owner == page && region.kind == UiDesignerDropRegionKind::TabPageContent)
            tab_page_region = true;
    Check(tab_page_region, "Active Tab Page publishes its content region");
    UiDesignerGeneratedProject tab_generated = generator.Generate(
        tab_session.Document(), "GeneratedTabHost");
    const String page_member = tab_session.Document().Find(page)
        ? tab_session.Document().Find(page)->name + "_n" + AsString(page) : String();
    Check(!page_member.IsEmpty() && tab_generated.source.Find(page_member + ".Add(") >= 0,
          "Generated Tab Page attaches content to the page host");

    UiDesignerSession accordion_drop_session;
    accordion_drop_session.NewDocument("blank");
    const UiDesignerControlSpec* accordion_drop_spec = catalog.Find("UiAccordion");
    const UiDesignerNodeId accordion_drop = accordion_drop_session.Commands().AddNode(
        "UiAccordion", "accordion_drop", accordion_drop_session.Document().GetRootId(),
        accordion_drop_spec ? accordion_drop_spec->node_flags : 0,
        accordion_drop_spec ? accordion_drop_spec->defaults : ValueMap(), "Add Accordion");
    const UiDesignerNode* accordion_drop_node = accordion_drop_session.Document().Find(accordion_drop);
    const UiDesignerNodeId closed_section = accordion_drop_node &&
        accordion_drop_node->children.GetCount() ? accordion_drop_node->children[0] : 0;
    UiDesignerDropPlan section_drop_plan = accordion_drop_session.PlanAddControl(
        "UiPanel", closed_section, Point(), false);
    UiDesignerNodeId section_child = 0;
    String section_error;
    Check(section_drop_plan.valid && accordion_drop_session.ExecuteDrop(
          section_drop_plan, &section_child, section_error),
          "Closed Accordion section accepts content: " + section_error);
    Check(accordion_drop_session.Document().Find(closed_section)->GetProperty("open", false),
          "Accordion section opens with content drop");
    Check(accordion_drop_session.Commands().Undo() &&
          !accordion_drop_session.Document().Find(closed_section)->GetProperty("open", false) &&
          !accordion_drop_session.Document().Find(section_child),
          "Accordion content drop undo is atomic");

    UiDesignerSession move_section_session;
    move_section_session.NewDocument("blank");
    const UiDesignerNodeId move_accordion = move_section_session.AddControl("UiAccordion");
    const UiDesignerNode* move_accordion_node = move_section_session.Document().Find(move_accordion);
    const UiDesignerNodeId move_section = move_accordion_node->children[0];
    const UiDesignerNodeId moved_section_button = move_section_session.AddControl("UiButton");
    const UiDesignerNodeId moved_button_original_parent =
        move_section_session.Document().Find(moved_section_button)->parent;
    UiDesignerDropPlan move_section_plan = move_section_session.Drops().PlanMove(
        Vector<UiDesignerNodeId>{moved_section_button}, move_section);
    String move_section_error;
    Check(move_section_session.ExecuteDrop(move_section_plan, nullptr, move_section_error),
          "Existing Button moves into a closed Accordion section: " + move_section_error);
    Check(move_section_session.Document().Find(move_section)->GetProperty("open", false) &&
              move_section_session.Document().Find(moved_section_button)->parent == move_section,
          "Accordion move opens the destination section atomically");
    const bool move_undo = move_section_session.Commands().Undo();
    Check(move_undo, "Accordion move undo executes");
    Check(move_section_session.Document().Find(moved_section_button)->parent ==
              moved_button_original_parent,
          "Accordion move undo restores the original parent");
    Check(!move_section_session.Document().Find(move_section)->GetProperty("open", false),
          "Accordion move undo restores closed state");
    Check(move_section_session.Commands().Redo() &&
              move_section_session.Document().Find(moved_section_button)->parent == move_section &&
              move_section_session.Document().Find(move_section)->GetProperty("open", false),
          "Accordion move redo restores parent and open state");
    Check(move_section_session.Commands().SetAccordionSectionLock(move_section, "Closed"),
          "Accordion section can be locked closed");
    Check(!move_section_session.Drops().PlanMove(
              Vector<UiDesignerNodeId>{moved_section_button}, move_section).valid,
          "Locked closed Accordion section rejects existing-node moves");

    const UiDesignerControlSpec* accordion_spec = catalog.Find("UiAccordion");
    const UiDesignerControlSpec* label_spec = catalog.Find("UiLabel");
    Check(accordion_spec && accordion_spec->sizing_class == UiDesignerSizingClass::Container &&
              label_spec && label_spec->sizing_class == UiDesignerSizingClass::Leaf,
          "Catalog classifies containers and leaves for fresh sizing");
    UiDesignerSession sizing_session;
    sizing_session.NewDocument("blank");
    UiDesignerNodeId sizing_grid = sizing_session.Commands().AddNode(
        "UiGridLayout", "sizing_grid", sizing_session.Document().GetRootId(),
        grid_spec ? grid_spec->node_flags : 0,
        grid_spec ? grid_spec->defaults : ValueMap(), "Add sizing Grid");
    UiDesignerDropPlan sizing_panel = sizing_session.PlanAddControl(
        "UiPanel", sizing_grid, Point(0, 0), true, -1, 0, 0);
    UiDesignerDropPlan sizing_label = sizing_session.PlanAddControl(
        "UiLabel", sizing_grid, Point(0, 0), true, -1, 0, 1);
    Check(sizing_panel.valid && sizing_panel.add_defaults.GetValue(
              sizing_panel.add_defaults.Find("width_mode")) == "Expand" &&
              sizing_panel.add_defaults.GetValue(
                  sizing_panel.add_defaults.Find("cell_align_x")) == "Stretch",
          "Fresh container Grid drops use Expand and Stretch");
    Check(sizing_label.valid && sizing_label.add_defaults.GetValue(
              sizing_label.add_defaults.Find("width_mode")) == "Fit",
          "Fresh leaf Grid drops remain Fit");

    UiDesignerDocument accordion_document;
    UiDesignerCommandService accordion_commands(accordion_document);
    UiDesignerNodeId accordion_node = accordion_commands.AddNode(
        "UiAccordion", "contract_accordion", accordion_document.GetRootId(),
        accordion_spec ? accordion_spec->node_flags : 0,
        accordion_spec ? accordion_spec->defaults : ValueMap(),
        "Add contract Accordion");
    const UiDesignerNode* accordion_authored = accordion_document.Find(accordion_node);
    Check(accordion_authored && accordion_authored->children.GetCount() == 3,
          "New Accordion creates Overview, Details and Notes");
    bool all_closed = true;
    if(accordion_authored)
        for(UiDesignerNodeId section_id : accordion_authored->children)
            all_closed &= !(bool)accordion_document.Find(section_id)->GetProperty("open", true);
    Check(all_closed, "New Accordion sections default closed");
    const UiDesignerNodeId overview_node = accordion_authored
        ? accordion_authored->children[0] : 0;
    const UiDesignerControlSpec* accordion_box_spec = catalog.Find("UiBoxLayout");
    const UiDesignerNodeId overview_box = accordion_commands.AddNode(
        "UiBoxLayout", "overview_content", overview_node,
        accordion_box_spec ? accordion_box_spec->node_flags : 0,
        accordion_box_spec ? accordion_box_spec->defaults : ValueMap(),
        "Add Accordion section content");
    UiDesignerPreviewCanvas accordion_preview;
    UiDesignerSelection accordion_selection;
    accordion_preview.SetRect(0, 0, 320, 260);
    accordion_preview.Bind(&accordion_document, &catalog, nullptr, &accordion_selection);
    accordion_preview.RebuildDocument();
    UiAccordion* runtime_accordion = dynamic_cast<UiAccordion *>(
        accordion_preview.FindRuntime(accordion_node));
    Check(runtime_accordion && runtime_accordion->GetCount() == 3,
          "Preview creates all canonical Accordion sections");
    Check(runtime_accordion && runtime_accordion->GetSectionContent(0).GetFirstChild() ==
              accordion_preview.FindRuntime(overview_box),
          "Accordion section content attaches through its runtime section host");
    const UiDesignerGeometryRecord* overview_record =
        accordion_preview.GetGeometrySnapshot().Find(overview_node);
    Check(overview_record && !overview_record->rect.IsEmpty(),
          "Accordion section has a stable published runtime rectangle");
    bool section_region = false;
    if(runtime_accordion)
        for(const UiDesignerDropRegion& region : accordion_preview.GetGeometrySnapshot().GetDropRegions())
            if(region.owner == overview_node &&
               region.kind == UiDesignerDropRegionKind::AccordionSectionContent) {
                section_region = true;
                Check(region.occupied, "Occupied Accordion section region is marked occupied");
            }
    Check(section_region, "Accordion section content region is published");
    accordion_selection.nodes.Add(overview_node);
    accordion_selection.primary = overview_node;
    const UiDesignerGeometryRecord* selected_overview =
        accordion_preview.GetGeometrySnapshot().Find(overview_node);
    Check(selected_overview && selected_overview->drop_target,
          "Selected Accordion section exposes its content target");
    bool selected_section_region = false;
    for(const UiDesignerDropRegion& region : accordion_preview.GetGeometrySnapshot().GetDropRegions())
        if(region.owner == overview_node &&
           region.kind == UiDesignerDropRegionKind::AccordionSectionContent)
            selected_section_region = true;
    Check(selected_section_region, "Selected Accordion section retains its orange region");
    accordion_selection.nodes.Clear();
    accordion_selection.primary = accordion_node;
    UiDesignerGeometryRecord const* selected_accordion =
        accordion_preview.GetGeometrySnapshot().Find(accordion_node);
    Check(selected_accordion && selected_accordion->drop_target,
          "Selected Accordion exposes section content targets");
    String second_section_child_error;
    Check(!catalog.CanInsert(accordion_document, "UiPanel", overview_node, -1,
                             second_section_child_error) &&
              second_section_child_error.Find("one direct content child") >= 0,
          "Accordion section rejects a second direct child with guidance");
    UiAccordion contract_runtime;
    contract_runtime.SetRect(0, 0, 260, 120);
    const int overview = contract_runtime.AddSection("Overview", false);
    const int details = contract_runtime.AddSection("Details", false);
    const int notes = contract_runtime.AddSection("Notes", false);
    contract_runtime.Layout();
    Check(!contract_runtime.IsOpen(overview) && !contract_runtime.IsOpen(details) &&
              !contract_runtime.IsOpen(notes),
          "Runtime Accordion preserves closed section state");
    const Rect accordion_bounds = RectC(0, 0, 260, 120);
    Check(accordion_bounds.Contains(contract_runtime.GetSectionHeaderRect(overview)) &&
              accordion_bounds.Contains(contract_runtime.GetSectionHeaderRect(details)) &&
              accordion_bounds.Contains(contract_runtime.GetSectionHeaderRect(notes)),
          "Closed Accordion headers remain inside the boundary");
    Check(contract_runtime.GetSectionBodyHeight(overview) == 0,
          "Empty closed Accordion body has zero rendered height");
    contract_runtime.Open(overview);
    contract_runtime.Layout();
    Check(contract_runtime.GetSectionBodyHeight(overview) == 0,
          "Opening an empty Accordion section does not reserve a fake body");
    UiBoxLayout contract_box;
    contract_runtime.GetSectionContent(overview).Add(contract_box.SizePos());
    contract_runtime.Open(overview);
    contract_runtime.Layout();
    Check(accordion_bounds.Contains(contract_runtime.GetSectionHeaderRect(overview)) &&
              accordion_bounds.Contains(contract_runtime.GetSectionBodyRect(overview)) &&
              contract_runtime.GetSectionBodyHeight(overview) >= 0,
          "Populated Accordion body remains bounded");
    contract_runtime.Open(details);
    contract_runtime.Open(notes);
    contract_runtime.Layout();
    const Rect notes_body = contract_runtime.GetSectionBodyRect(notes);
    Check(accordion_bounds.Contains(contract_runtime.GetSectionHeaderRect(notes)) &&
              (notes_body.IsEmpty() || accordion_bounds.Contains(notes_body)),
          "Multiple open Accordion sections remain bounded");

    UiDesignerGeneratedProject accordion_generated = generator.Generate(
        accordion_document, "GeneratedAccordionWindow");
    Check(accordion_generated.source.Find("GetSectionContent(") >= 0,
          "Generated Accordion uses typed section content attachment");
    UiDesignerSession accordion_theme_session;
    const UiDesignerNodeId accordion_theme_node =
        accordion_theme_session.AddControl("UiAccordion");
    accordion_theme_session.Select(accordion_theme_node);
    Check(accordion_theme_session.Catalog().Find("UiAccordion") &&
              accordion_theme_session.Catalog().Find("UiAccordion")->theme_overrides.GetCount() > 0 &&
              accordion_theme_session.ThemeOverrideModel().GetCount() > 0,
          "Accordion theme override model is populated");
    UiDesignerSession title_card_model_session;
    const UiDesignerNodeId title_card_model_node =
        title_card_model_session.AddControl("UiTitleCard");
    title_card_model_session.Select(title_card_model_node);
    title_card_model_session.RebuildBehaviorModel();
    Check(title_card_model_session.ThemeOverrideModel().GetCount() > 0,
          "Title Card theme override model is populated");
    Check(title_card_model_session.BehaviorModel().GetCount() > 0,
          "Title Card behavior model is populated");

    // Regression fixture for managed-parent detachment and incremental
    // Accordion section projection.
    UiDesignerSession lifecycle_session;
    UiDesignerPreviewCanvas lifecycle_preview;
    lifecycle_preview.SetRect(0, 0, 640, 320);
    lifecycle_session.AttachProjection(&lifecycle_preview);
    UiDesignerNodeId lifecycle_grid = lifecycle_session.AddControl("UiGridLayout");
    lifecycle_session.Commands().SetProperty(lifecycle_grid, "rows", 2,
                                              UiDesignerImpactLocalLayout);
    lifecycle_session.Commands().SetProperty(lifecycle_grid, "columns", 2,
                                              UiDesignerImpactLocalLayout);
    lifecycle_session.Commands().SetProperty(lifecycle_grid, "width_mode", "Fixed",
                                              UiDesignerImpactLocalLayout);
    lifecycle_session.Commands().SetProperty(lifecycle_grid, "height_mode", "Fixed",
                                              UiDesignerImpactLocalLayout);
    lifecycle_session.Commands().SetProperty(lifecycle_grid, "fixed_width", 640,
                                              UiDesignerImpactLocalLayout);
    lifecycle_session.Commands().SetProperty(lifecycle_grid, "fixed_height", 320,
                                              UiDesignerImpactLocalLayout);
    String lifecycle_error;
    UiDesignerNodeId lifecycle_card = 0;
    UiDesignerDropPlan lifecycle_card_plan = lifecycle_session.PlanAddControl(
        "UiTitleCard", lifecycle_grid, Point(480, 40), true, -1, 0, 1);
    Check(lifecycle_session.ExecuteDrop(lifecycle_card_plan, &lifecycle_card,
                                        lifecycle_error),
          "Lifecycle fixture adds Title Card: " + lifecycle_error);
    UiDesignerNodeId lifecycle_accordion = 0;
    UiDesignerDropPlan lifecycle_accordion_plan = lifecycle_session.PlanAddControl(
        "UiAccordion", lifecycle_grid, Point(40, 40), true, -1, 0, 0);
    Check(lifecycle_session.ExecuteDrop(lifecycle_accordion_plan,
                                        &lifecycle_accordion, lifecycle_error),
          "Lifecycle fixture adds Accordion: " + lifecycle_error);
    lifecycle_preview.RebuildDocument();
    UiGridLayout* lifecycle_grid_runtime = dynamic_cast<UiGridLayout *>(
        lifecycle_preview.FindRuntime(lifecycle_grid));
    UiAccordion* lifecycle_accordion_runtime = dynamic_cast<UiAccordion *>(
        lifecycle_preview.FindRuntime(lifecycle_accordion));
    Check(lifecycle_grid_runtime && lifecycle_grid_runtime->GetItemCount() == 2 &&
              lifecycle_accordion_runtime && lifecycle_accordion_runtime->GetCount() == 3,
          "Lifecycle fixture starts with two Grid items and three closed sections");
    const int lifecycle_grid_items_before = lifecycle_grid_runtime
        ? lifecycle_grid_runtime->GetItemCount() : -1;
    const uint64 lifecycle_card_generation =
        lifecycle_preview.GetInstanceGeneration(lifecycle_card);
    const UiDesignerNode* lifecycle_accordion_node =
        lifecycle_session.Document().Find(lifecycle_accordion);
    UiDesignerNodeId lifecycle_removed = lifecycle_accordion_node
        ? lifecycle_accordion_node->children[0] : 0;
    lifecycle_session.Select(lifecycle_removed);
    Check(lifecycle_session.RemoveSelection(),
          "Lifecycle fixture removes the selected first Accordion section");
    Check(lifecycle_session.State().selection.primary != lifecycle_removed &&
              lifecycle_session.Document().Find(lifecycle_session.State().selection.primary),
          "Deleted Accordion section selects a remaining section");
    lifecycle_grid_runtime = dynamic_cast<UiGridLayout *>(
        lifecycle_preview.FindRuntime(lifecycle_grid));
    Check(lifecycle_grid_runtime && lifecycle_grid_runtime->GetItemCount() ==
              lifecycle_grid_items_before,
          "Deleting a section preserves the parent Grid item count");
    const UiDesignerNodeId lifecycle_selected =
        lifecycle_session.State().selection.primary;
    Check(lifecycle_selected != 0, "Lifecycle fixture retains a selected section");
    const uint64 lifecycle_generation_before_property =
        lifecycle_preview.GetInstanceGeneration(lifecycle_accordion);
    Check(lifecycle_session.PreviewProperty("open", true, lifecycle_error),
          "Accordion open transient preview succeeds: " + lifecycle_error);
    Check(lifecycle_preview.GetInstanceGeneration(lifecycle_accordion) ==
              lifecycle_generation_before_property &&
              lifecycle_preview.GetInstanceGeneration(lifecycle_card) ==
                  lifecycle_card_generation,
          "Transient Accordion open does not rebuild Accordion or Title Card");
    lifecycle_session.CancelPreview();
    lifecycle_accordion_runtime = dynamic_cast<UiAccordion *>(
        lifecycle_preview.FindRuntime(lifecycle_accordion));
    Check(!lifecycle_session.Document().GetProperty(lifecycle_selected, "open", false) &&
              lifecycle_accordion_runtime && !lifecycle_accordion_runtime->IsOpen(0),
          "Cancelling Accordion open restores canonical state");
    Check(lifecycle_session.PreviewProperty("open", true, lifecycle_error),
          "Repeated Accordion open preview succeeds");
    lifecycle_session.CancelPreview();
    Check(lifecycle_session.CommitProperty("open", true, lifecycle_error),
          "Committed Accordion open succeeds without subtree rebuild");
    Check(lifecycle_session.Document().GetProperty(lifecycle_selected, "open", false),
          "Committed Accordion open changes canonical state");
    Check(lifecycle_session.Undo(), "Accordion open undo succeeds");
    Check(!lifecycle_session.Document().GetProperty(lifecycle_selected, "open", true),
          "Accordion open undo restores closed state");
    Check(lifecycle_session.Redo(), "Accordion open redo succeeds");
    Check(lifecycle_session.Document().GetProperty(lifecycle_selected, "open", false),
          "Accordion open redo restores open state");
    lifecycle_session.Select(lifecycle_selected);
    for(int cycle = 0; cycle < 100; cycle++) {
        const bool open = (cycle & 1) != 0;
        const bool preview_ok = lifecycle_session.PreviewProperty("open", open, lifecycle_error);
        Check(preview_ok, "Accordion repeated transient open/close preview succeeds");
        lifecycle_session.CancelPreview();
        lifecycle_preview.RebuildSubtree(lifecycle_accordion);
        lifecycle_grid_runtime = dynamic_cast<UiGridLayout *>(
            lifecycle_preview.FindRuntime(lifecycle_grid));
        Check(lifecycle_grid_runtime && lifecycle_grid_runtime->GetItemCount() ==
                  lifecycle_grid_items_before,
              "Accordion rebuild cycle preserves Grid item count");
    }
    Check(lifecycle_preview.GetInstanceGeneration(lifecycle_card) != 0,
          "Unrelated Title Card remains live after Accordion stress cycles");
    Check(lifecycle_grid_runtime && lifecycle_grid_runtime->ValidateItems(),
          "Grid item invariants remain valid after Accordion stress cycles");
    for(int cycle = 0; cycle < 25; cycle++) {
        const UiDesignerNodeId added = lifecycle_session.Commands().AddAccordionSection(
            lifecycle_accordion, "Cycle " + AsString(cycle), "", "", false, "None");
        Check(added != 0, "Repeated Accordion section addition succeeds");
        lifecycle_session.Select(added);
        Check(lifecycle_session.RemoveSelection(),
              "Repeated Accordion section deletion succeeds");
        lifecycle_grid_runtime = dynamic_cast<UiGridLayout *>(
            lifecycle_preview.FindRuntime(lifecycle_grid));
        Check(lifecycle_grid_runtime && lifecycle_grid_runtime->GetItemCount() ==
                  lifecycle_grid_items_before && lifecycle_grid_runtime->ValidateItems(),
              "Repeated section add/remove preserves Grid invariants");
    }

    UiDesignerSession drag_session;
    drag_session.NewDocument("blank");
    UiDesignerHierarchyView drag_hierarchy;
    drag_hierarchy.SetRect(0, 0, 320, 200);
    drag_hierarchy.SetDocument(&drag_session.Document());
    drag_hierarchy.SetSelection(&drag_session.State().selection);
    drag_hierarchy.PlanCatalogDrop = [&](const String& type,
                                         UiDesignerNodeId parent, int index) {
        return drag_session.Drops().PlanAdd(type, parent, Point(), false, index);
    };
    drag_hierarchy.ExecuteDrop = [&](const UiDesignerDropPlan& plan, String& drop_error) {
        return drag_session.ExecuteDrop(plan, nullptr, drop_error);
    };
    drag_hierarchy.Rebuild();
    drag_hierarchy.CancelMode();
    drag_hierarchy.CancelMode();
    Check(!drag_hierarchy.IsNodeDragPollArmed() &&
              drag_hierarchy.GetNodeDragPollArmCount() == 0,
          "Hierarchy repeated cancellation leaves no poll armed");
    drag_hierarchy.TrackCatalogDrop("UiPanel", Point(20, 15));
    Check(drag_hierarchy.HasDropTarget(),
          "Hierarchy catalog tracking publishes a target");
    Check(drag_hierarchy.FinishCatalogDrop("UiPanel", Point(20, 15)) &&
              !drag_hierarchy.HasDropTarget() &&
              drag_session.Document().GetCount() == 2,
          "Hierarchy catalog completion clears its target after one execution");


    Cout() << "Checks: " << checks << " Fails: " << fails << "\n";
    SetExitCode(fails ? 1 : 0);
}
