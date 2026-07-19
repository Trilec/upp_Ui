#include "UiDesignerCatalog.h"

namespace Upp {

static UiDesignerControlSpec MakeSpec(
    const char *type, const char *display, const char *category,
    const char *cpp_type, const char *base_name,
    UiDesignerRuntimeKind runtime_kind, const char *icon_key,
    dword flags = UiDesignerNodeNone, Size size = Size(160, 32))
{
    UiDesignerControlSpec spec;
    spec.type_id = type;
    spec.display_name = display;
    spec.category = category;
    spec.runtime_cpp_type = cpp_type;
    spec.default_base_name = base_name;
    spec.runtime_kind = runtime_kind;
    spec.icon_key = icon_key;
    spec.node_flags = flags;
    spec.default_size = size;
    spec.capabilities = UiDesignerCapabilityRuntimeCtrl;
    if(flags & UiDesignerNodeContainer)
        spec.capabilities |= UiDesignerCapabilityContainer |
                             UiDesignerCapabilityOrdered;
    spec.preview_adapter_id = "runtime:" + String(type);
    spec.codegen_adapter_id = "control";
    spec.child_adapter_id = (flags & UiDesignerNodeContainer) ? "add" : "none";
    AddUiDesignerCommonProperties(spec);

    spec.defaults.Set("visible", true);
    spec.defaults.Set("enabled", true);
    spec.defaults.Set("role", "Standard");
    if(flags & (UiDesignerNodeContainer | UiDesignerNodeLayout)) {
        UiDesignerPropertySpec inset = UiDesignerNumberProperty(
            "inset", "Inset", 0, 0, 1000, 1, PropertyEditorKind::Integer);
        inset.group = "Layout";
        inset.domain = PropertyEditorDomain::Layout;
        inset.impact = PropertyImpactLocalLayout |
                       PropertyImpactAncestorLayout | PropertyImpactCode;
        spec.properties.Add(inset);
        spec.defaults.Set("inset", 0);
    }
    return spec;
}

static void AddEvent(UiDesignerControlSpec& spec, const char *id,
                     const char *label, const char *help)
{
    UiDesignerEventSpec& event = spec.events.Add();
    event.id = id;
    event.label = label;
    event.help = help;
    spec.capabilities |= UiDesignerCapabilityAcceptActions;
}

static void AddText(UiDesignerControlSpec& spec, const String& value)
{
    UiDesignerPropertySpec property = UiDesignerTextProperty();
    property.default_value = value;
    spec.properties.Add(property);
    spec.defaults.Set("text", value);
}

static void AddTitle(UiDesignerControlSpec& spec, const String& value)
{
    UiDesignerPropertySpec property = UiDesignerTextProperty("title", "Title");
    property.default_value = value;
    spec.properties.Add(property);
    spec.defaults.Set("title", value);
}

static void AddValueRange(UiDesignerControlSpec& spec,
                          double value, double minimum, double maximum,
                          double step, PropertyEditorKind kind)
{
    UiDesignerPropertySpec value_property = UiDesignerNumberProperty(
        "value", "Value", value, minimum, maximum, step, kind);
    spec.properties.Add(value_property);
    spec.defaults.Set("value", value);

    UiDesignerPropertySpec min_property = UiDesignerNumberProperty(
        "minimum", "Minimum", minimum, -100000, 100000, step,
        PropertyEditorKind::Double);
    min_property.group = "Value";
    spec.properties.Add(min_property);
    spec.defaults.Set("minimum", minimum);

    UiDesignerPropertySpec max_property = UiDesignerNumberProperty(
        "maximum", "Maximum", maximum, -100000, 100000, step,
        PropertyEditorKind::Double);
    max_property.group = "Value";
    spec.properties.Add(max_property);
    spec.defaults.Set("maximum", maximum);
}

static UiDesignerPropertySpec ChoiceProperty(
    const char *id, const char *label, const char *group,
    const char *default_value,
    std::initializer_list<std::pair<const char *, const char *>> choices,
    PropertyEditorImpact impact = PropertyImpactStructure |
                                  PropertyImpactAncestorLayout |
                                  PropertyImpactCode)
{
    UiDesignerPropertySpec property;
    property.id = id;
    property.label = label;
    property.group = group;
    property.kind = PropertyEditorKind::Choice;
    property.domain = PropertyEditorDomain::Layout;
    property.default_value = default_value;
    property.impact = impact;
    for(const auto& choice : choices)
        property.Choice(choice.first, choice.second);
    return property;
}

static UiDesignerControlSpec MakeSpacer()
{
    UiDesignerControlSpec spec;
    spec.type_id = "Spacer";
    spec.display_name = "Spacer / Separator";
    spec.category = "Layouts";
    spec.default_base_name = "spacer";
    spec.help = "Semantic layout space, break or separator. It does not create a runtime Ctrl.";
    spec.icon_key = "spacer";
    spec.runtime_kind = UiDesignerRuntimeKind::SemanticSpacer;
    spec.node_flags = UiDesignerNodeStructural | UiDesignerNodeSemanticItem;
    spec.capabilities = UiDesignerCapabilitySemanticItem;
    spec.default_size = Size(80, 24);
    spec.preview_adapter_id = "spacer";
    spec.codegen_adapter_id = "spacer";
    spec.child_adapter_id = "none";
    spec.preview = true;
    spec.inspector = true;
    spec.codegen = true;
    spec.theme = false;

    UiDesignerPropertySpec name;
    name.id = "name";
    name.label = "Name";
    name.group = "Identity";
    name.kind = PropertyEditorKind::Text;
    name.domain = PropertyEditorDomain::DesignerOnly;
    name.default_value = "spacer";
    name.impact = PropertyImpactCode | PropertyImpactSelection;
    name.designer_only = true;
    spec.properties.Add(name);

    auto AddNumber = [&](const char *id, const char *label, const char *group,
                         double value, double minimum, double maximum,
                         PropertyEditorKind kind = PropertyEditorKind::Integer) {
        UiDesignerPropertySpec p = UiDesignerNumberProperty(
            id, label, value, minimum, maximum, 1, kind);
        p.group = group;
        p.domain = PropertyEditorDomain::Layout;
        p.impact = PropertyImpactAncestorLayout | PropertyImpactCode;
        spec.properties.Add(p);
        spec.defaults.Set(id, value);
    };
    auto AddBool = [&](const char *id, const char *label,
                       const char *group, bool value,
                       PropertyEditorImpact impact = PropertyImpactAncestorLayout |
                                                     PropertyImpactCode) {
        UiDesignerPropertySpec p = UiDesignerBoolProperty(id, label, value);
        p.group = group;
        p.domain = PropertyEditorDomain::Layout;
        p.impact = impact;
        spec.properties.Add(p);
        spec.defaults.Set(id, value);
    };

    AddNumber("weight", "Weight", "Layout", 1.0, 0.0, 1000.0,
              PropertyEditorKind::Double);
    AddBool("layout_break", "Layout break", "Layout", false,
            PropertyImpactStructure | PropertyImpactAncestorLayout |
            PropertyImpactCode);

    spec.properties.Add(ChoiceProperty(
        "h_sizing", "Horizontal sizing", "Sizing", "Auto",
        {{"Auto", "Auto"}, {"Fixed", "Fixed"}, {"Fill", "Fill"},
         {"MinMax", "Min / Max"}}));
    spec.defaults.Set("h_sizing", "Auto");
    spec.properties.Add(ChoiceProperty(
        "v_sizing", "Vertical sizing", "Sizing", "Auto",
        {{"Auto", "Auto"}, {"Fixed", "Fixed"}, {"Fill", "Fill"},
         {"MinMax", "Min / Max"}}));
    spec.defaults.Set("v_sizing", "Auto");

    AddNumber("fixed_width", "Fixed width", "Sizing", 0, 0, 10000);
    AddNumber("fixed_height", "Fixed height", "Sizing", 0, 0, 10000);
    AddNumber("min_width", "Minimum width", "Sizing", 0, 0, 10000);
    AddNumber("min_height", "Minimum height", "Sizing", 0, 0, 10000);
    AddNumber("max_width", "Maximum width", "Sizing", 0, 0, 10000);
    AddNumber("max_height", "Maximum height", "Sizing", 0, 0, 10000);

    AddNumber("grid_row", "Grid row", "Grid", 0, 0, 1024);
    AddNumber("grid_column", "Grid column", "Grid", 0, 0, 1024);

    AddBool("line_enabled", "Separator line", "Separator", false,
            PropertyImpactPaint | PropertyImpactAncestorLayout |
            PropertyImpactCode);
    spec.properties.Add(ChoiceProperty(
        "line_orientation", "Orientation", "Separator", "Horizontal",
        {{"Horizontal", "Horizontal"}, {"Vertical", "Vertical"}},
        PropertyImpactPaint | PropertyImpactAncestorLayout |
        PropertyImpactCode));
    spec.defaults.Set("line_orientation", "Horizontal");
    spec.properties.Add(ChoiceProperty(
        "line_align", "Alignment", "Separator", "Center",
        {{"Start", "Start"}, {"Center", "Center"}, {"End", "End"}},
        PropertyImpactPaint | PropertyImpactCode));
    spec.defaults.Set("line_align", "Center");
    AddNumber("line_thickness", "Thickness", "Separator", 1, 1, 20);
    spec.properties.Add(ChoiceProperty(
        "line_dash", "Dash", "Separator", "Solid",
        {{"Solid", "Solid"}, {"Dash", "Dash"}, {"Dot", "Dot"}},
        PropertyImpactPaint | PropertyImpactCode));
    spec.defaults.Set("line_dash", "Solid");
    AddNumber("line_inset", "Inset", "Separator", 0, 0, 1000);
    AddBool("line_color_enabled", "Custom colour", "Separator", false,
            PropertyImpactPaint | PropertyImpactCode);

    UiDesignerPropertySpec color;
    color.id = "line_color";
    color.label = "Line colour";
    color.group = "Separator";
    color.kind = PropertyEditorKind::Color;
    color.domain = PropertyEditorDomain::Appearance;
    color.default_value = Color(128, 128, 128);
    color.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(color);
    spec.defaults.Set("line_color", color.default_value);
    return spec;
}

static void RegisterNative(UiDesignerCatalog& catalog)
{
    const char *controls_icon = "controls";
    const char *containers_icon = "containers";
    const char *layouts_icon = "layouts";
    const char *composites_icon = "composites";

    catalog.Register(MakeSpacer());

    {
        auto s = MakeSpec("UiBoxLayout", "Box Layout", "Layouts",
                          "UiBoxLayout", "box", UiDesignerRuntimeKind::UiBoxLayout,
                          layouts_icon, UiDesignerNodeContainer | UiDesignerNodeLayout,
                          Size(320, 180));
        s.capabilities |= UiDesignerCapabilityOrdered |
                          UiDesignerCapabilityAcceptSpacer;
        s.child_adapter_id = "box";
        UiDesignerPropertySpec debug = UiDesignerBoolProperty(
            "debug_layout", "Debug geometry", false);
        debug.group = "Designer";
        debug.domain = PropertyEditorDomain::DesignerOnly;
        debug.designer_only = true;
        debug.impact = PropertyImpactPaint;
        s.properties.Add(debug);
        s.defaults.Set("debug_layout", false);
        UiDesignerPropertySpec direction = ChoiceProperty(
            "direction", "Direction", "Layout", "V",
            {{"H", "Horizontal"}, {"V", "Vertical"}});
        s.properties.Add(direction);
        s.defaults.Set("direction", "V");
        catalog.Register(pick(s));
    }
    {
        auto s = MakeSpec("UiGridLayout", "Grid Layout", "Layouts",
                          "UiGridLayout", "grid", UiDesignerRuntimeKind::UiGridLayout,
                          layouts_icon, UiDesignerNodeContainer | UiDesignerNodeLayout,
                          Size(320, 180));
        s.capabilities |= UiDesignerCapabilityGrid |
                          UiDesignerCapabilityAcceptSpacer;
        s.child_adapter_id = "grid";
        UiDesignerPropertySpec debug = UiDesignerBoolProperty(
            "debug_layout", "Debug geometry", false);
        debug.group = "Designer";
        debug.domain = PropertyEditorDomain::DesignerOnly;
        debug.designer_only = true;
        debug.impact = PropertyImpactPaint;
        s.properties.Add(debug);
        s.defaults.Set("debug_layout", false);
        auto rows = UiDesignerNumberProperty("rows", "Rows", 2, 1, 64, 1,
                                             PropertyEditorKind::Integer);
        rows.group = "Structure";
        rows.impact = PropertyImpactStructure | PropertyImpactSubtree |
                      PropertyImpactCode;
        auto columns = UiDesignerNumberProperty("columns", "Columns", 2, 1, 64, 1,
                                                PropertyEditorKind::Integer);
        columns.group = "Structure";
        columns.impact = rows.impact;
        s.properties.Add(rows);
        s.properties.Add(columns);
        s.defaults.Set("rows", 2);
        s.defaults.Set("columns", 2);
        catalog.Register(pick(s));
    }
    {
        auto s = MakeSpec("UiAbsoluteLayout", "Absolute Layout", "Layouts",
                          "UiAbsoluteLayout", "absolute",
                          UiDesignerRuntimeKind::UiAbsoluteLayout,
                          layouts_icon,
                          UiDesignerNodeContainer | UiDesignerNodeLayout,
                          Size(320, 180));
        s.capabilities |= UiDesignerCapabilityFreeform;
        s.child_adapter_id = "absolute";
        for(const auto& field : {std::pair<const char *, const char *>("x", "X"),
                                 {"y", "Y"}, {"width", "Width"}, {"height", "Height"}}) {
            UiDesignerPropertySpec p = UiDesignerNumberProperty(
                field.first, field.second,
                field.first == String("width") ? s.default_size.cx :
                field.first == String("height") ? s.default_size.cy : 20,
                0, 10000, 1, PropertyEditorKind::Integer);
            p.group = "Absolute position";
            p.domain = PropertyEditorDomain::Layout;
            p.impact = PropertyImpactLocalLayout |
                       PropertyImpactAncestorLayout | PropertyImpactCode;
            s.properties.Add(p);
            s.defaults.Set(field.first, p.default_value);
        }
        s.help = "Places each child at an exact local X, Y, width and height. "
                 "Children may overlap and paint in insertion order.";
        catalog.Register(pick(s));
    }
    {
        auto s = MakeSpec("UiSplitter", "Splitter", "Layouts",
                          "UiSplitter", "splitter",
                          UiDesignerRuntimeKind::UiSplitter,
                          layouts_icon, UiDesignerNodeContainer | UiDesignerNodeLayout,
                          Size(360, 200));
        s.child_adapter_id = "splitter";
        catalog.Register(pick(s));
    }
    {
        auto s = MakeSpec("UiQuadSplitter", "Quad Splitter", "Layouts",
                          "UiQuadSplitter", "quad_splitter",
                          UiDesignerRuntimeKind::UiQuadSplitter,
                          layouts_icon, UiDesignerNodeContainer | UiDesignerNodeLayout,
                          Size(420, 260));
        s.child_adapter_id = "quad";
        catalog.Register(pick(s));
    }

    const struct NativeContainer {
        const char *type;
        const char *display;
        const char *cpp;
        const char *base;
        UiDesignerRuntimeKind kind;
    } containers[] = {
        {"UiPanel", "Panel", "UiPanel", "panel", UiDesignerRuntimeKind::UiPanel},
        {"UiDirectContentHost", "Direct Content Host", "UiDirectContentHost", "host", UiDesignerRuntimeKind::UiDirectContentHost},
        {"UiGroupPanel", "Group Panel", "UiGroupPanel", "group", UiDesignerRuntimeKind::UiGroupPanel},
        {"UiStack", "Stack", "UiStack", "stack", UiDesignerRuntimeKind::UiStack},
        {"UiAccordion", "Accordion", "UiAccordion", "accordion", UiDesignerRuntimeKind::UiAccordion},
        {"UiScrollPanel", "Scroll Panel", "UiScrollPanel", "scroll", UiDesignerRuntimeKind::UiScrollPanel},
        {"UiTab", "Tab", "UiTab", "tab", UiDesignerRuntimeKind::UiTab},
        {"UiTitleCard", "Title Card", "UiTitleCard", "title_card", UiDesignerRuntimeKind::UiTitleCard},
    };
    for(const auto& c : containers) {
        auto s = MakeSpec(c.type, c.display, "Containers", c.cpp, c.base, c.kind,
                          containers_icon, UiDesignerNodeContainer, Size(280, 160));
        s.capabilities |= UiDesignerCapabilityFreeform;
        if(String(c.type) == "UiStack" || String(c.type) == "UiTab" ||
           String(c.type) == "UiAccordion") {
            s.capabilities |= UiDesignerCapabilityPages;
            s.capabilities &= ~(dword)UiDesignerCapabilityFreeform;
            s.child_adapter_id = String(c.type) == "UiTab" ? "tab" :
                                 String(c.type) == "UiStack" ? "stack" :
                                 "accordion";
            AddEvent(s, "WhenAction", "Page changed",
                     "Runs after the active page changes.");
        }
        if(String(c.type) == "UiScrollPanel" ||
           String(c.type) == "UiDirectContentHost")
            s.child_adapter_id = "single";
        if(String(c.type) == "UiGroupPanel" || String(c.type) == "UiTitleCard")
            AddTitle(s, c.display);
        if(String(c.type) == "UiTitleCard") {
            UiDesignerPropertySpec icon = ChoiceProperty(
                "icon", "Icon", "Content", "None",
                {{"None", "None"},
                 {"ICON_DESIGN_DESCRIPTION_48", "Description"}},
                PropertyImpactControlState | PropertyImpactLocalLayout |
                PropertyImpactCode);
            icon.domain = PropertyEditorDomain::Content;
            s.properties.Add(icon);
            s.defaults.Set("icon", "None");
        }
        catalog.Register(pick(s));
    }

    const struct NativeControl {
        const char *type;
        const char *display;
        const char *cpp;
        const char *base;
        UiDesignerRuntimeKind kind;
        bool text;
    } controls[] = {
        {"UiLabel", "Label", "UiLabel", "label", UiDesignerRuntimeKind::UiLabel, true},
        {"UiCheckBox", "Check Box", "UiCheckBox", "check", UiDesignerRuntimeKind::UiCheckBox, true},
        {"UiRadioButton", "Radio Button", "UiRadioButton", "radio", UiDesignerRuntimeKind::UiRadioButton, true},
        {"UiToggle", "Toggle", "UiToggle", "toggle", UiDesignerRuntimeKind::UiToggle, false},
        {"UiButton", "Button", "UiButton", "button", UiDesignerRuntimeKind::UiButton, true},
        {"UiToolButton", "Tool Button", "UiToolButton", "tool_button", UiDesignerRuntimeKind::UiToolButton, false},
        {"UiSplitButton", "Split Button", "UiSplitButton", "split_button", UiDesignerRuntimeKind::UiSplitButton, true},
        {"UiLineEdit", "Line Edit", "UiLineEdit", "line_edit", UiDesignerRuntimeKind::UiLineEdit, false},
        {"UiIntEdit", "Integer Edit", "UiIntEdit", "int_edit", UiDesignerRuntimeKind::UiIntEdit, false},
        {"UiFloatEdit", "Float Edit", "UiFloatEdit", "float_edit", UiDesignerRuntimeKind::UiFloatEdit, false},
        {"UiPasswordEdit", "Password Edit", "UiPasswordEdit", "password_edit", UiDesignerRuntimeKind::UiPasswordEdit, false},
        {"UiMultiEdit", "Multi Edit", "UiMultiEdit", "multi_edit", UiDesignerRuntimeKind::UiMultiEdit, false},
        {"UiMaskEdit", "Mask Edit", "UiMaskEdit", "mask_edit", UiDesignerRuntimeKind::UiMaskEdit, false},
        {"UiProgressBar", "Progress Bar", "UiProgressBar", "progress", UiDesignerRuntimeKind::UiProgressBar, false},
        {"UiSlider", "Slider", "UiSlider", "slider", UiDesignerRuntimeKind::UiSlider, false},
        {"UiBreadcrumbs", "Breadcrumbs", "UiBreadcrumbs", "breadcrumbs", UiDesignerRuntimeKind::UiBreadcrumbs, false},
        {"UiSliderEdit", "Slider Edit", "UiSliderEdit", "slider_edit", UiDesignerRuntimeKind::UiSliderEdit, false},
        {"UiScrollBar", "Scroll Bar", "UiScrollBar", "scroll_bar", UiDesignerRuntimeKind::UiScrollBar, false},
        {"UiTable", "Table", "UiTable", "table", UiDesignerRuntimeKind::UiTable, false},
        {"UiDoc", "Document", "UiDoc", "document", UiDesignerRuntimeKind::UiDoc, false},
        {"UiTree", "Tree", "UiTree", "tree", UiDesignerRuntimeKind::UiTree, false},
        {"UiList", "List", "UiList", "list", UiDesignerRuntimeKind::UiList, false},
        {"UiBezierCurveEditor", "Bezier Curve Editor", "UiBezierCurveEditor", "curve_editor", UiDesignerRuntimeKind::UiBezierCurveEditor, false},
        {"UiBezierCurveField", "Bezier Curve Field", "UiBezierCurveField", "curve", UiDesignerRuntimeKind::UiBezierCurveField, false},
        {"UiDropdown", "Dropdown", "UiDropdown", "dropdown", UiDesignerRuntimeKind::UiDropdown, false},
        {"UiMenu", "Menu", "UiMenu", "menu", UiDesignerRuntimeKind::UiMenu, false},
        {"UiColorPicker", "Color Picker", "UiColorPicker", "color_picker", UiDesignerRuntimeKind::UiColorPicker, false},
    };
    for(const auto& c : controls) {
        auto s = MakeSpec(c.type, c.display, "Ui Controls", c.cpp, c.base,
                          c.kind, controls_icon, UiDesignerNodeNone,
                          Size(190, (String(c.type) == "UiMultiEdit" ||
                                     String(c.type) == "UiDoc" ||
                                     String(c.type) == "UiTable" ||
                                     String(c.type) == "UiTree" ||
                                     String(c.type) == "UiList") ? 110 : 34));
        if(c.text)
            AddText(s, c.display);
        if(String(c.type) == "UiToggle" ||
           String(c.type) == "UiCheckBox" ||
           String(c.type) == "UiRadioButton") {
            auto checked = UiDesignerBoolProperty("checked", "Checked", false);
            s.properties.Add(checked);
            s.defaults.Set("checked", false);
            AddEvent(s, "WhenAction", "Changed", "Runs after the checked state changes.");
        }
        if(String(c.type) == "UiButton" || String(c.type) == "UiToolButton")
            AddEvent(s, "WhenAction", "Clicked", "Runs when the button is activated.");
        if(String(c.type) == "UiSplitButton") {
            AddEvent(s, "WhenAction", "Primary action", "Runs when the main button is activated.");
            AddEvent(s, "WhenSelect", "Menu selection", "Runs after a split-menu item is selected.");
        }
        if(String(c.type) == "UiLineEdit" || String(c.type) == "UiIntEdit" ||
           String(c.type) == "UiFloatEdit" || String(c.type) == "UiPasswordEdit" ||
           String(c.type) == "UiMultiEdit" || String(c.type) == "UiMaskEdit") {
            AddEvent(s, "WhenChanging", "Changing", "Runs during interactive editing.");
            AddEvent(s, "WhenAction", "Committed", "Runs when editing is committed.");
        }
        if(String(c.type) == "UiSlider" ||
           String(c.type) == "UiSliderEdit" ||
           String(c.type) == "UiProgressBar") {
            AddValueRange(s, 50, 0, 100, 1,
                          String(c.type) == "UiProgressBar"
                              ? PropertyEditorKind::Integer
                              : PropertyEditorKind::SliderDouble);
            if(String(c.type) != "UiProgressBar") {
                AddEvent(s, "WhenChanging", "Changing", "Runs during slider movement.");
                AddEvent(s, "WhenAction", "Committed", "Runs after the slider gesture.");
            }
        }
        if(String(c.type) == "UiDropdown")
            AddEvent(s, "WhenAction", "Selection changed", "Runs after selection changes.");
        if(String(c.type) == "UiColorPicker") {
            UiDesignerPropertySpec color;
            color.id = "color";
            color.label = "Color";
            color.group = "Value";
            color.kind = PropertyEditorKind::Color;
            color.domain = PropertyEditorDomain::Appearance;
            color.default_value = Color(58, 132, 255);
            color.impact = PropertyImpactControlState |
                           PropertyImpactPaint |
                           PropertyImpactCode;
            s.properties.Add(color);
            s.defaults.Set("color", color.default_value);
            AddEvent(s, "WhenChanging", "Colour changing", "Runs during colour preview.");
            AddEvent(s, "WhenAccept", "Colour accepted", "Runs after colour acceptance.");
            AddEvent(s, "WhenCancel", "Colour cancelled", "Runs when colour editing is cancelled.");
        }
        catalog.Register(pick(s));
    }

    const struct CompositeControl {
        const char *type;
        const char *display;
        const char *base;
        UiDesignerRuntimeKind kind;
    } composites[] = {
        {"UiCompositeSlider", "Composite Slider", "composite_slider", UiDesignerRuntimeKind::UiCompositeSlider},
        {"UiCompositeToggle", "Composite Toggle", "composite_toggle", UiDesignerRuntimeKind::UiCompositeToggle},
        {"UiCompositeColor", "Composite Color", "composite_color", UiDesignerRuntimeKind::UiCompositeColor},
        {"UiCompositeDropdown", "Composite Dropdown", "composite_dropdown", UiDesignerRuntimeKind::UiCompositeDropdown},
        {"UiCompositeLabel", "Composite Label", "composite_label", UiDesignerRuntimeKind::UiCompositeLabel},
        {"UiCompositeEdit", "Composite Edit", "composite_edit", UiDesignerRuntimeKind::UiCompositeEdit},
    };
    for(const auto& c : composites) {
        auto s = MakeSpec(c.type, c.display, "Composites", c.type,
                          c.base, c.kind, composites_icon,
                          UiDesignerNodeContainer, Size(260, 72));
        AddTitle(s, c.display);
        AddEvent(s, "WhenAction", "Action", "Runs when the composite commits its value.");
        catalog.Register(pick(s));
    }
}

static void RegisterStock(UiDesignerCatalog& catalog)
{
    const char *icon = "controls";
    const struct Stock {
        const char *type;
        const char *display;
        const char *cpp;
        const char *base;
        UiDesignerRuntimeKind kind;
        bool text;
        bool container;
    } stock[] = {
        {"UppLabel", "U++ Label", "Label", "upp_label", UiDesignerRuntimeKind::UppLabel, true, false},
        {"UppButton", "U++ Button", "Button", "upp_button", UiDesignerRuntimeKind::UppButton, true, false},
        {"UppOption", "U++ Option", "Option", "upp_option", UiDesignerRuntimeKind::UppOption, true, false},
        {"UppEditString", "U++ EditString", "EditString", "upp_edit", UiDesignerRuntimeKind::UppEditString, false, false},
        {"UppEditInt", "U++ EditInt", "EditInt", "upp_int", UiDesignerRuntimeKind::UppEditInt, false, false},
        {"UppEditDouble", "U++ EditDouble", "EditDouble", "upp_double", UiDesignerRuntimeKind::UppEditDouble, false, false},
        {"UppLineEdit", "U++ LineEdit", "LineEdit", "upp_line", UiDesignerRuntimeKind::UppLineEdit, false, false},
        {"UppDropList", "U++ DropList", "DropList", "upp_drop", UiDesignerRuntimeKind::UppDropList, false, false},
        {"UppArrayCtrl", "U++ ArrayCtrl", "ArrayCtrl", "upp_array", UiDesignerRuntimeKind::UppArrayCtrl, false, false},
        {"UppTreeCtrl", "U++ TreeCtrl", "TreeCtrl", "upp_tree", UiDesignerRuntimeKind::UppTreeCtrl, false, false},
        {"UppTabCtrl", "U++ TabCtrl", "TabCtrl", "upp_tab", UiDesignerRuntimeKind::UppTabCtrl, false, true},
        {"UppProgressIndicator", "U++ Progress", "ProgressIndicator", "upp_progress", UiDesignerRuntimeKind::UppProgressIndicator, false, false},
        {"UppSliderCtrl", "U++ Slider", "SliderCtrl", "upp_slider", UiDesignerRuntimeKind::UppSliderCtrl, false, false},
        {"UppColorPusher", "U++ Color Pusher", "ColorPusher", "upp_color", UiDesignerRuntimeKind::UppColorPusher, false, false},
        {"UppParentCtrl", "U++ ParentCtrl", "ParentCtrl", "upp_parent", UiDesignerRuntimeKind::UppParentCtrl, false, true},
        {"UppStaticRect", "U++ StaticRect", "StaticRect", "upp_rect", UiDesignerRuntimeKind::UppStaticRect, false, true},
        {"UppSplitter", "U++ Splitter", "Splitter", "upp_splitter", UiDesignerRuntimeKind::UppSplitter, false, true},
        {"UppHScrollBar", "U++ Horizontal ScrollBar", "HScrollBar", "upp_hscroll", UiDesignerRuntimeKind::UppHScrollBar, false, false},
        {"UppVScrollBar", "U++ Vertical ScrollBar", "VScrollBar", "upp_vscroll", UiDesignerRuntimeKind::UppVScrollBar, false, false},
    };

    for(const auto& c : stock) {
        const dword flags = UiDesignerNodeStockUpp |
                            (c.container ? UiDesignerNodeContainer : 0);
        auto s = MakeSpec(c.type, c.display, "U++ Controls",
                          c.cpp, c.base, c.kind, icon, flags,
                          Size(190, (String(c.type) == "UppArrayCtrl" ||
                                     String(c.type) == "UppTreeCtrl" ||
                                     String(c.type) == "UppTabCtrl" ||
                                     String(c.type) == "UppParentCtrl") ? 110 : 32));
        s.stock_upp = true;
        s.theme = false;
        if(c.container)
            s.capabilities |= UiDesignerCapabilityFreeform;
        if(String(c.type) == "UppTabCtrl")
            s.child_adapter_id = "upp_tab";
        if(String(c.type) == "UppSplitter")
            s.child_adapter_id = "upp_splitter";
        if(c.text)
            AddText(s, c.display);
        if(String(c.type) == "UppButton" || String(c.type) == "UppOption" ||
           String(c.type) == "UppEditString" || String(c.type) == "UppEditInt" ||
           String(c.type) == "UppEditDouble" || String(c.type) == "UppDropList" ||
           String(c.type) == "UppSliderCtrl" || String(c.type) == "UppColorPusher")
            AddEvent(s, "WhenAction", "Action", "Runs when the control commits its value.");
        catalog.Register(pick(s));
    }
}

static void RegisterPresets(UiDesignerCatalog& catalog)
{
    catalog.RegisterPreset({"blank", "Blank Form",
                            "A clean window with one root layout.",
                            "presets"});
    catalog.RegisterPreset({"three_pane", "Three Pane",
                            "Toolbox, canvas and Inspector columns.",
                            "layouts"});
    catalog.RegisterPreset({"dialog", "Dialog",
                            "A compact dialog with heading, content and actions.",
                            "inspector"});
}

void RegisterUiDesignerBuiltins(UiDesignerCatalog& catalog)
{
    RegisterPresets(catalog);
    RegisterNative(catalog);
    RegisterStock(catalog);
}

}
