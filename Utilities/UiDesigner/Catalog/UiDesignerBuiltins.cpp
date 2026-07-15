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
    AddUiDesignerCommonProperties(spec);

    spec.defaults.Set("visible", true);
    spec.defaults.Set("enabled", true);
    spec.defaults.Set("x", 20);
    spec.defaults.Set("y", 20);
    spec.defaults.Set("width", size.cx);
    spec.defaults.Set("height", size.cy);
    spec.defaults.Set("role", "Standard");
    return spec;
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

static void RegisterNative(UiDesignerCatalog& catalog)
{
    const char *controls_icon = "controls";
    const char *containers_icon = "containers";
    const char *layouts_icon = "layouts";
    const char *composites_icon = "composites";

    {
        auto s = MakeSpec("UiBoxLayout", "Box Layout", "Layouts",
                          "UiBoxLayout", "box", UiDesignerRuntimeKind::UiBoxLayout,
                          layouts_icon, UiDesignerNodeContainer | UiDesignerNodeLayout,
                          Size(320, 180));
        UiDesignerPropertySpec direction;
        direction.id = "direction";
        direction.label = "Direction";
        direction.group = "Layout";
        direction.kind = PropertyEditorKind::Choice;
        direction.domain = PropertyEditorDomain::Layout;
        direction.default_value = "V";
        direction.impact = PropertyImpactStructure |
                           PropertyImpactLocalLayout |
                           PropertyImpactCode;
        direction.Choice("H", "Horizontal").Choice("V", "Vertical");
        s.properties.Add(direction);
        s.defaults.Set("direction", "V");
        catalog.Register(pick(s));
    }
    {
        auto s = MakeSpec("UiGridLayout", "Grid Layout", "Layouts",
                          "UiGridLayout", "grid", UiDesignerRuntimeKind::UiGridLayout,
                          layouts_icon, UiDesignerNodeContainer | UiDesignerNodeLayout,
                          Size(320, 180));
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
    catalog.Register(MakeSpec("UiSplitter", "Splitter", "Layouts",
                              "UiSplitter", "splitter",
                              UiDesignerRuntimeKind::UiSplitter,
                              layouts_icon, UiDesignerNodeContainer | UiDesignerNodeLayout,
                              Size(360, 200)));
    catalog.Register(MakeSpec("UiQuadSplitter", "Quad Splitter", "Layouts",
                              "UiQuadSplitter", "quad_splitter",
                              UiDesignerRuntimeKind::UiQuadSplitter,
                              layouts_icon, UiDesignerNodeContainer | UiDesignerNodeLayout,
                              Size(420, 260)));

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
        if(String(c.type) == "UiGroupPanel" || String(c.type) == "UiTitleCard")
            AddTitle(s, c.display);
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
        }
        if(String(c.type) == "UiSlider" ||
           String(c.type) == "UiSliderEdit" ||
           String(c.type) == "UiProgressBar")
            AddValueRange(s, 50, 0, 100, 1,
                          String(c.type) == "UiProgressBar"
                              ? PropertyEditorKind::Integer
                              : PropertyEditorKind::SliderDouble);
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
    } stock[] = {
        {"UppLabel", "U++ Label", "Label", "upp_label", UiDesignerRuntimeKind::UppLabel, true},
        {"UppButton", "U++ Button", "Button", "upp_button", UiDesignerRuntimeKind::UppButton, true},
        {"UppOption", "U++ Option", "Option", "upp_option", UiDesignerRuntimeKind::UppOption, true},
        {"UppEditString", "U++ EditString", "EditString", "upp_edit", UiDesignerRuntimeKind::UppEditString, false},
        {"UppEditInt", "U++ EditInt", "EditInt", "upp_int", UiDesignerRuntimeKind::UppEditInt, false},
        {"UppEditDouble", "U++ EditDouble", "EditDouble", "upp_double", UiDesignerRuntimeKind::UppEditDouble, false},
        {"UppLineEdit", "U++ LineEdit", "LineEdit", "upp_line", UiDesignerRuntimeKind::UppLineEdit, false},
        {"UppDropList", "U++ DropList", "DropList", "upp_drop", UiDesignerRuntimeKind::UppDropList, false},
        {"UppArrayCtrl", "U++ ArrayCtrl", "ArrayCtrl", "upp_array", UiDesignerRuntimeKind::UppArrayCtrl, false},
        {"UppTreeCtrl", "U++ TreeCtrl", "TreeCtrl", "upp_tree", UiDesignerRuntimeKind::UppTreeCtrl, false},
        {"UppTabCtrl", "U++ TabCtrl", "TabCtrl", "upp_tab", UiDesignerRuntimeKind::UppTabCtrl, false},
        {"UppProgressIndicator", "U++ Progress", "ProgressIndicator", "upp_progress", UiDesignerRuntimeKind::UppProgressIndicator, false},
        {"UppSliderCtrl", "U++ Slider", "SliderCtrl", "upp_slider", UiDesignerRuntimeKind::UppSliderCtrl, false},
        {"UppColorPusher", "U++ Color Pusher", "ColorPusher", "upp_color", UiDesignerRuntimeKind::UppColorPusher, false},
        {"UppParentCtrl", "U++ ParentCtrl", "ParentCtrl", "upp_parent", UiDesignerRuntimeKind::UppParentCtrl, false},
        {"UppStaticRect", "U++ StaticRect", "StaticRect", "upp_rect", UiDesignerRuntimeKind::UppStaticRect, false},
        {"UppSplitter", "U++ Splitter", "Splitter", "upp_splitter", UiDesignerRuntimeKind::UppSplitter, false},
        {"UppHScrollBar", "U++ Horizontal ScrollBar", "HScrollBar", "upp_hscroll", UiDesignerRuntimeKind::UppHScrollBar, false},
        {"UppVScrollBar", "U++ Vertical ScrollBar", "VScrollBar", "upp_vscroll", UiDesignerRuntimeKind::UppVScrollBar, false},
    };

    for(const auto& c : stock) {
        auto s = MakeSpec(c.type, c.display, "U++ Controls",
                          c.cpp, c.base, c.kind, icon,
                          UiDesignerNodeStockUpp,
                          Size(190, (String(c.type) == "UppArrayCtrl" ||
                                     String(c.type) == "UppTreeCtrl" ||
                                     String(c.type) == "UppTabCtrl" ||
                                     String(c.type) == "UppParentCtrl") ? 110 : 32));
        s.stock_upp = true;
        s.theme = false;
        if(c.text)
            AddText(s, c.display);
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
    catalog.RegisterPreset({"settings", "Settings",
                            "A labelled settings form with grouped editors.",
                            "inspector"});
}

void RegisterUiDesignerBuiltins(UiDesignerCatalog& catalog)
{
    RegisterPresets(catalog);
    RegisterNative(catalog);
    RegisterStock(catalog);
}

}
