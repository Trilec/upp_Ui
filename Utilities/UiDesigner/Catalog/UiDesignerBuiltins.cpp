#include "UiDesignerCatalog.h"
#include <Utilities/UiDesigner/Theme/UiDesignerThemeAdapter.h>
#include <Utilities/UiDesigner/UiDesigner/UiDesignerButtonStyle.h>

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
    spec.theme = false;
    if((flags & (UiDesignerNodeContainer | UiDesignerNodeLayout)) &&
       strcmp(type, "UiAbsoluteLayout") != 0) {
        const int inset_default = !strcmp(type, "UiPanel") ||
                                  !strcmp(type, "UiBoxLayout") ||
                                  !strcmp(type, "UiGridLayout") ? 8 : 0;
        UiDesignerPropertySpec inset = UiDesignerNumberProperty(
            "inset", "Inset", inset_default, 0, 1000, 1, PropertyEditorKind::Integer);
        inset.group = "Layout";
        inset.domain = PropertyEditorDomain::Layout;
        inset.impact = PropertyImpactLocalLayout |
                       PropertyImpactAncestorLayout | PropertyImpactCode;
        spec.properties.Add(inset);
        spec.defaults.Set("inset", inset_default);
    }
    if(flags & (UiDesignerNodeContainer | UiDesignerNodeLayout)) {
        spec.defaults.Set("width_mode", "Expand");
        spec.defaults.Set("height_mode", "Expand");
    }
    else {
        spec.defaults.Set("width_mode", "Fit");
        spec.defaults.Set("height_mode", "Fit");
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

static Color UiDesignerBuiltinsFillColor(const UiFill& fill)
{
    return fill.IsSolid() ? fill.color : Null;
}

static String UiDesignerBuiltinsShadowModeName(ShadowMode mode)
{
    return mode == SHADOW_HARD ? "Hard" : "Curve";
}

static void AddText(UiDesignerControlSpec& spec, const String& value)
{
    UiDesignerPropertySpec property = UiDesignerTextProperty();
    property.default_value = value;
    spec.properties.Add(property);
    spec.defaults.Set("text", value);
}

static void AddButtonProperties(UiDesignerControlSpec& spec)
{
    UiDesignerPropertySpec tooltip = UiDesignerTextProperty("tooltip", "Tooltip");
    tooltip.group = "Content";
    tooltip.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(tooltip);

    UiDesignerPropertySpec icon;
    icon.id = "icon";
    icon.label = "Icon";
    icon.group = "Content";
    icon.kind = PropertyEditorKind::Choice;
    icon.domain = PropertyEditorDomain::Content;
    icon.default_value = "None";
    icon.impact = PropertyImpactPaint | PropertyImpactCode;
    icon.choices.Add(PropertyEditorChoice("None", "None"));
    icon.choices.Add(PropertyEditorChoice("ICON_DESIGN_DESCRIPTION_48", "Description"));
    icon.choices.Add(PropertyEditorChoice("ICON_DESIGN_WIDGETS_48", "Widgets"));
    icon.choices.Add(PropertyEditorChoice("ICON_DESIGN_ACCOUNT_TREE_48", "Hierarchy"));
    icon.choices.Add(PropertyEditorChoice("ICON_DESIGN_TUNE_48", "Inspector"));
    spec.properties.Add(icon);
    spec.defaults.Set("icon", "None");

    UiDesignerPropertySpec icon_side;
    icon_side.id = "icon_side";
    icon_side.label = "Icon side";
    icon_side.group = "Appearance";
    icon_side.kind = PropertyEditorKind::Choice;
    icon_side.domain = PropertyEditorDomain::Appearance;
    icon_side.default_value = "Left";
    icon_side.impact = PropertyImpactPaint | PropertyImpactCode;
    icon_side.choices.Add(PropertyEditorChoice("Left", "Left"));
    icon_side.choices.Add(PropertyEditorChoice("Right", "Right"));
    icon_side.choices.Add(PropertyEditorChoice("Top", "Top"));
    icon_side.choices.Add(PropertyEditorChoice("Bottom", "Bottom"));
    spec.properties.Add(icon_side);
    spec.defaults.Set("icon_side", "Left");

    UiDesignerPropertySpec icon_width = UiDesignerNumberProperty(
        "icon_width", "Icon width", 18, 0, 256, 1, PropertyEditorKind::Integer);
    icon_width.group = "Appearance";
    icon_width.domain = PropertyEditorDomain::Appearance;
    icon_width.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(icon_width);
    spec.defaults.Set("icon_width", 18);

    UiDesignerPropertySpec icon_height = UiDesignerNumberProperty(
        "icon_height", "Icon height", 18, 0, 256, 1, PropertyEditorKind::Integer);
    icon_height.group = "Appearance";
    icon_height.domain = PropertyEditorDomain::Appearance;
    icon_height.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(icon_height);
    spec.defaults.Set("icon_height", 18);

    UiDesignerPropertySpec icon_render_mode;
    icon_render_mode.id = "icon_render_mode";
    icon_render_mode.label = "Icon render mode";
    icon_render_mode.group = "Appearance";
    icon_render_mode.kind = PropertyEditorKind::Choice;
    icon_render_mode.domain = PropertyEditorDomain::Appearance;
    icon_render_mode.default_value = "MonoTint";
    icon_render_mode.impact = PropertyImpactPaint | PropertyImpactCode;
    icon_render_mode.choices.Add(PropertyEditorChoice("Auto", "Auto"));
    icon_render_mode.choices.Add(PropertyEditorChoice("MonoTint", "Mono tint"));
    icon_render_mode.choices.Add(PropertyEditorChoice("PreserveColor", "Preserve color"));
    spec.properties.Add(icon_render_mode);
    spec.defaults.Set("icon_render_mode", "MonoTint");

    UiDesignerPropertySpec scale_icon = UiDesignerBoolProperty(
        "scale_icon_to_content", "Scale icon to content", false);
    scale_icon.group = "Appearance";
    scale_icon.domain = PropertyEditorDomain::Appearance;
    scale_icon.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(scale_icon);
    spec.defaults.Set("scale_icon_to_content", false);

    UiDesignerPropertySpec align_h;
    align_h.id = "align_h";
    align_h.label = "Horizontal align";
    align_h.group = "Appearance";
    align_h.kind = PropertyEditorKind::Choice;
    align_h.domain = PropertyEditorDomain::Appearance;
    align_h.default_value = "Center";
    align_h.impact = PropertyImpactPaint | PropertyImpactCode;
    align_h.choices.Add(PropertyEditorChoice("Left", "Left"));
    align_h.choices.Add(PropertyEditorChoice("Center", "Center"));
    align_h.choices.Add(PropertyEditorChoice("Right", "Right"));
    spec.properties.Add(align_h);
    spec.defaults.Set("align_h", "Center");

    UiDesignerPropertySpec align_v;
    align_v.id = "align_v";
    align_v.label = "Vertical align";
    align_v.group = "Appearance";
    align_v.kind = PropertyEditorKind::Choice;
    align_v.domain = PropertyEditorDomain::Appearance;
    align_v.default_value = "Center";
    align_v.impact = PropertyImpactPaint | PropertyImpactCode;
    align_v.choices.Add(PropertyEditorChoice("Top", "Top"));
    align_v.choices.Add(PropertyEditorChoice("Center", "Center"));
    align_v.choices.Add(PropertyEditorChoice("Bottom", "Bottom"));
    spec.properties.Add(align_v);
    spec.defaults.Set("align_v", "Center");

    UiDesignerPropertySpec content_gap = UiDesignerNumberProperty(
        "content_gap", "Content gap", 4, 0, 100, 1, PropertyEditorKind::Integer);
    content_gap.group = "Appearance";
    content_gap.domain = PropertyEditorDomain::Appearance;
    content_gap.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(content_gap);

    UiDesignerPropertySpec content_inset_left = UiDesignerNumberProperty(
        "content_inset_left", "Content inset left", 4, 0, 100, 1, PropertyEditorKind::Integer);
    content_inset_left.group = "Appearance";
    content_inset_left.domain = PropertyEditorDomain::Appearance;
    content_inset_left.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(content_inset_left);
    spec.defaults.Set("content_inset_left", 4);

    UiDesignerPropertySpec content_inset_top = UiDesignerNumberProperty(
        "content_inset_top", "Content inset top", 4, 0, 100, 1, PropertyEditorKind::Integer);
    content_inset_top.group = "Appearance";
    content_inset_top.domain = PropertyEditorDomain::Appearance;
    content_inset_top.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(content_inset_top);
    spec.defaults.Set("content_inset_top", 4);

    UiDesignerPropertySpec content_inset_right = UiDesignerNumberProperty(
        "content_inset_right", "Content inset right", 4, 0, 100, 1, PropertyEditorKind::Integer);
    content_inset_right.group = "Appearance";
    content_inset_right.domain = PropertyEditorDomain::Appearance;
    content_inset_right.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(content_inset_right);
    spec.defaults.Set("content_inset_right", 4);

    UiDesignerPropertySpec content_inset_bottom = UiDesignerNumberProperty(
        "content_inset_bottom", "Content inset bottom", 4, 0, 100, 1, PropertyEditorKind::Integer);
    content_inset_bottom.group = "Appearance";
    content_inset_bottom.domain = PropertyEditorDomain::Appearance;
    content_inset_bottom.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(content_inset_bottom);
    spec.defaults.Set("content_inset_bottom", 4);

    UiDesignerPropertySpec click_focus = UiDesignerBoolProperty("click_focus", "Click focus", true);
    click_focus.group = "State";
    click_focus.domain = PropertyEditorDomain::Behaviour;
    click_focus.impact = PropertyImpactControlState | PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(click_focus);
    spec.defaults.Set("click_focus", true);

    UiDesignerPropertySpec checkable = UiDesignerBoolProperty("checkable", "Checkable", false);
    checkable.group = "State";
    checkable.domain = PropertyEditorDomain::Behaviour;
    checkable.impact = PropertyImpactControlState | PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(checkable);
    spec.defaults.Set("checkable", false);

    UiDesignerPropertySpec checked = UiDesignerBoolProperty("checked", "Checked", false);
    checked.group = "State";
    checked.domain = PropertyEditorDomain::Behaviour;
    checked.impact = PropertyImpactControlState | PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(checked);
    spec.defaults.Set("checked", false);
}

static void AddButtonThemeOverrides(UiDesignerControlSpec& spec)
{
    const UiButton::Style base = UiTheme::ResolveButton(UiRole::Standard);
    auto add = [&](UiDesignerButtonStyleField field, const char *label,
                   const char *group, PropertyEditorKind kind,
                   const Value& value, PropertyEditorImpact impact,
                   const char *help = nullptr) {
        UiDesignerThemeOverrideSpec item;
        item.id = UiDesignerButtonStyleFieldName(field);
        item.label = label;
        item.group = group;
        item.kind = kind;
        item.domain = PropertyEditorDomain::Theme;
        item.default_value = value;
        item.impact = impact;
        item.AdapterField(UiDesignerButtonStyleFieldName(field));
        if(help)
            item.help = help;
        spec.theme_overrides.Add(pick(item));
    };
    auto add_bool = [&](UiDesignerButtonStyleField field, const char *label,
                        const char *group, bool value,
                        PropertyEditorImpact impact = PropertyImpactPaint | PropertyImpactCode) {
        add(field, label, group, PropertyEditorKind::Boolean, value, impact);
    };
    auto add_int = [&](UiDesignerButtonStyleField field, const char *label,
                       const char *group, int value, int min_value, int max_value,
                       int step = 1,
                       PropertyEditorImpact impact = PropertyImpactPaint | PropertyImpactCode) {
        UiDesignerThemeOverrideSpec item;
        item.id = UiDesignerButtonStyleFieldName(field);
        item.label = label;
        item.group = group;
        item.kind = PropertyEditorKind::Integer;
        item.domain = PropertyEditorDomain::Theme;
        item.default_value = value;
        item.minimum = min_value;
        item.maximum = max_value;
        item.step = step;
        item.impact = impact;
        item.AdapterField(UiDesignerButtonStyleFieldName(field));
        spec.theme_overrides.Add(pick(item));
    };
    auto add_color = [&](UiDesignerButtonStyleField field, const char *label,
                         const char *group, Color value,
                         PropertyEditorImpact impact = PropertyImpactPaint | PropertyImpactCode) {
        add(field, label, group, PropertyEditorKind::Color, value, impact);
    };

    add(UiDesignerButtonStyleField::FontFace, "Font face", "Typography",
        PropertyEditorKind::Text, base.font.GetFaceName(),
        PropertyImpactPaint | PropertyImpactCode);
    add_int(UiDesignerButtonStyleField::FontSize, "Font size", "Typography",
            base.font.GetHeight(), 1, 256, 1);
    add_bool(UiDesignerButtonStyleField::FontBold, "Font bold", "Typography",
             base.font.IsBold());
    add_bool(UiDesignerButtonStyleField::FontItalic, "Font italic", "Typography",
             base.font.IsItalic());

    add_bool(UiDesignerButtonStyleField::FaceEnabled, "Face enabled", "Face",
             base.metrics.face_enabled);
    add_color(UiDesignerButtonStyleField::FaceNormal, "Face normal", "Face",
              UiDesignerBuiltinsFillColor(base.palette.face[ST_NORMAL]));
    add_color(UiDesignerButtonStyleField::FaceHot, "Face hot", "Face",
              UiDesignerBuiltinsFillColor(base.palette.face[ST_HOT]));
    add_color(UiDesignerButtonStyleField::FacePressed, "Face pressed", "Face",
              UiDesignerBuiltinsFillColor(base.palette.face[ST_PRESSED]));
    add_color(UiDesignerButtonStyleField::FaceDisabled, "Face disabled", "Face",
              UiDesignerBuiltinsFillColor(base.palette.face[ST_DISABLED]));
    add_bool(UiDesignerButtonStyleField::Transparent, "Transparent", "Face",
             base.transparent);

    add_bool(UiDesignerButtonStyleField::FrameEnabled, "Frame enabled", "Frame",
             base.metrics.frame_enabled);
    add_color(UiDesignerButtonStyleField::FrameNormal, "Frame normal", "Frame",
              base.palette.frame[ST_NORMAL]);
    add_color(UiDesignerButtonStyleField::FrameHot, "Frame hot", "Frame",
              base.palette.frame[ST_HOT]);
    add_color(UiDesignerButtonStyleField::FramePressed, "Frame pressed", "Frame",
              base.palette.frame[ST_PRESSED]);
    add_color(UiDesignerButtonStyleField::FrameDisabled, "Frame disabled", "Frame",
              base.palette.frame[ST_DISABLED]);
    add_int(UiDesignerButtonStyleField::FrameWidth, "Frame width", "Frame",
            base.metrics.frame_width, 0, 20, 1);
    add_int(UiDesignerButtonStyleField::Radius, "Radius", "Frame",
            base.metrics.radius, 0, 40, 1);
    add_bool(UiDesignerButtonStyleField::FrameDashed, "Frame dashed", "Frame",
             base.metrics.dashed);
    add(UiDesignerButtonStyleField::FrameDashPattern, "Frame dash pattern",
        "Frame", PropertyEditorKind::Text, base.metrics.dash_pattern,
        PropertyImpactPaint | PropertyImpactCode);

    add_color(UiDesignerButtonStyleField::TextNormal, "Text normal", "Text ink",
              base.palette.ink[ST_NORMAL]);
    add_color(UiDesignerButtonStyleField::TextHot, "Text hot", "Text ink",
              base.palette.ink[ST_HOT]);
    add_color(UiDesignerButtonStyleField::TextPressed, "Text pressed", "Text ink",
              base.palette.ink[ST_PRESSED]);
    add_color(UiDesignerButtonStyleField::TextDisabled, "Text disabled", "Text ink",
              base.palette.ink[ST_DISABLED]);

    add_color(UiDesignerButtonStyleField::IconNormal, "Icon normal", "Icon ink",
              base.palette.icon[ST_NORMAL]);
    add_color(UiDesignerButtonStyleField::IconHot, "Icon hot", "Icon ink",
              base.palette.icon[ST_HOT]);
    add_color(UiDesignerButtonStyleField::IconPressed, "Icon pressed", "Icon ink",
              base.palette.icon[ST_PRESSED]);
    add_color(UiDesignerButtonStyleField::IconDisabled, "Icon disabled", "Icon ink",
              base.palette.icon[ST_DISABLED]);

    add_bool(UiDesignerButtonStyleField::ShadowEnabled, "Shadow enabled", "Shadow",
             base.metrics.shadow.enabled);
    add_int(UiDesignerButtonStyleField::ShadowDistance, "Shadow distance", "Shadow",
            base.metrics.shadow.distance, 0, 64, 1);
    add_int(UiDesignerButtonStyleField::ShadowOffsetX, "Shadow offset X", "Shadow",
            base.metrics.shadow.offset_x, -64, 64, 1);
    add_int(UiDesignerButtonStyleField::ShadowOffsetY, "Shadow offset Y", "Shadow",
            base.metrics.shadow.offset_y, -64, 64, 1);
    add_int(UiDesignerButtonStyleField::ShadowAlpha, "Shadow alpha", "Shadow",
            base.metrics.shadow.alpha, 0, 255, 1);
    add_color(UiDesignerButtonStyleField::ShadowColor, "Shadow color", "Shadow",
              base.metrics.shadow.color);
    add_bool(UiDesignerButtonStyleField::ShadowInset, "Shadow inset", "Shadow",
             base.metrics.shadow.inset);
    {
        UiDesignerThemeOverrideSpec item;
        item.id = UiDesignerButtonStyleFieldName(UiDesignerButtonStyleField::ShadowMode);
        item.label = "Shadow mode";
        item.group = "Shadow";
        item.kind = PropertyEditorKind::Choice;
        item.domain = PropertyEditorDomain::Theme;
        item.default_value = UiDesignerBuiltinsShadowModeName(base.metrics.shadow.mode);
        item.impact = PropertyImpactPaint | PropertyImpactCode;
        item.AdapterField(UiDesignerButtonStyleFieldName(UiDesignerButtonStyleField::ShadowMode));
        item.Choice("Hard", "Hard");
        item.Choice("Curve", "Curve");
        spec.theme_overrides.Add(pick(item));
    }

    add_int(UiDesignerButtonStyleField::PressOffsetX, "Press offset X", "Additional",
            base.press_offset.x, -64, 64, 1);
    add_int(UiDesignerButtonStyleField::PressOffsetY, "Press offset Y", "Additional",
            base.press_offset.y, -64, 64, 1);
    add_int(UiDesignerButtonStyleField::Overpaint, "Overpaint", "Additional",
            base.overpaint, 0, 8, 1);
    add_bool(UiDesignerButtonStyleField::UnderlineEnabled, "Underline enabled",
             "Additional", base.underline);
    add_int(UiDesignerButtonStyleField::UnderlineWidth, "Underline width",
            "Additional", base.underline_width, 0, 10, 1);
    add_int(UiDesignerButtonStyleField::UnderlineOffset, "Underline offset",
            "Additional", base.underline_offset, -32, 32, 1);
}

static void AddTitle(UiDesignerControlSpec& spec, const String& value)
{
    UiDesignerPropertySpec property = UiDesignerTextProperty("title", "Title");
    property.default_value = value;
    spec.properties.Add(property);
    spec.defaults.Set("title", value);
}

static void AddTitleCardProperties(UiDesignerControlSpec& spec)
{
    UiDesignerPropertySpec subtitle = UiDesignerTextProperty("subtitle", "Subtitle");
    subtitle.group = "Content";
    subtitle.impact = PropertyImpactPaint | PropertyImpactCode;
    subtitle.default_value = "Supporting information";
    spec.properties.Add(subtitle);
    spec.defaults.Set("subtitle", "Supporting information");

    UiDesignerPropertySpec copy = UiDesignerTextProperty("copy", "Copy");
    copy.group = "Content";
    copy.impact = PropertyImpactPaint | PropertyImpactCode;
    copy.default_value = "Add a short description or place content in the card.";
    spec.properties.Add(copy);
    spec.defaults.Set("copy", "Add a short description or place content in the card.");

    UiDesignerPropertySpec text_align_h;
    text_align_h.id = "text_align_h";
    text_align_h.label = "Text align horizontal";
    text_align_h.group = "Appearance";
    text_align_h.kind = PropertyEditorKind::Choice;
    text_align_h.domain = PropertyEditorDomain::Appearance;
    text_align_h.default_value = "Left";
    text_align_h.impact = PropertyImpactPaint | PropertyImpactCode;
    text_align_h.choices.Add(PropertyEditorChoice("Left", "Left"));
    text_align_h.choices.Add(PropertyEditorChoice("Center", "Center"));
    text_align_h.choices.Add(PropertyEditorChoice("Right", "Right"));
    spec.properties.Add(text_align_h);
    spec.defaults.Set("text_align_h", "Left");

    UiDesignerPropertySpec text_align_v;
    text_align_v.id = "text_align_v";
    text_align_v.label = "Text align vertical";
    text_align_v.group = "Appearance";
    text_align_v.kind = PropertyEditorKind::Choice;
    text_align_v.domain = PropertyEditorDomain::Appearance;
    text_align_v.default_value = "Center";
    text_align_v.impact = PropertyImpactPaint | PropertyImpactCode;
    text_align_v.choices.Add(PropertyEditorChoice("Top", "Top"));
    text_align_v.choices.Add(PropertyEditorChoice("Center", "Center"));
    text_align_v.choices.Add(PropertyEditorChoice("Bottom", "Bottom"));
    spec.properties.Add(text_align_v);
    spec.defaults.Set("text_align_v", "Center");

    UiDesignerPropertySpec media_side;
    media_side.id = "media_side";
    media_side.label = "Media side";
    media_side.group = "Appearance";
    media_side.kind = PropertyEditorKind::Choice;
    media_side.domain = PropertyEditorDomain::Appearance;
    media_side.default_value = "Left";
    media_side.impact = PropertyImpactPaint | PropertyImpactCode;
    media_side.choices.Add(PropertyEditorChoice("Left", "Left"));
    media_side.choices.Add(PropertyEditorChoice("Right", "Right"));
    media_side.choices.Add(PropertyEditorChoice("Top", "Top"));
    media_side.choices.Add(PropertyEditorChoice("Bottom", "Bottom"));
    spec.properties.Add(media_side);
    spec.defaults.Set("media_side", "Left");

    UiDesignerPropertySpec media_align_h;
    media_align_h.id = "media_align_h";
    media_align_h.label = "Media align horizontal";
    media_align_h.group = "Appearance";
    media_align_h.kind = PropertyEditorKind::Choice;
    media_align_h.domain = PropertyEditorDomain::Appearance;
    media_align_h.default_value = "Center";
    media_align_h.impact = PropertyImpactPaint | PropertyImpactCode;
    media_align_h.choices.Add(PropertyEditorChoice("Left", "Left"));
    media_align_h.choices.Add(PropertyEditorChoice("Center", "Center"));
    media_align_h.choices.Add(PropertyEditorChoice("Right", "Right"));
    spec.properties.Add(media_align_h);
    spec.defaults.Set("media_align_h", "Center");

    UiDesignerPropertySpec media_align_v;
    media_align_v.id = "media_align_v";
    media_align_v.label = "Media align vertical";
    media_align_v.group = "Appearance";
    media_align_v.kind = PropertyEditorKind::Choice;
    media_align_v.domain = PropertyEditorDomain::Appearance;
    media_align_v.default_value = "Center";
    media_align_v.impact = PropertyImpactPaint | PropertyImpactCode;
    media_align_v.choices.Add(PropertyEditorChoice("Top", "Top"));
    media_align_v.choices.Add(PropertyEditorChoice("Center", "Center"));
    media_align_v.choices.Add(PropertyEditorChoice("Bottom", "Bottom"));
    spec.properties.Add(media_align_v);
    spec.defaults.Set("media_align_v", "Center");

    UiDesignerPropertySpec media_reserve = UiDesignerNumberProperty(
        "media_reserve", "Media reserve", 72, 0, 1000, 1, PropertyEditorKind::Integer);
    media_reserve.group = "Appearance";
    media_reserve.domain = PropertyEditorDomain::Appearance;
    media_reserve.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(media_reserve);
    spec.defaults.Set("media_reserve", 72);

    UiDesignerPropertySpec media_min = UiDesignerNumberProperty(
        "media_min", "Media min", 24, 0, 1000, 1, PropertyEditorKind::Integer);
    media_min.group = "Appearance";
    media_min.domain = PropertyEditorDomain::Appearance;
    media_min.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(media_min);
    spec.defaults.Set("media_min", 24);

    UiDesignerPropertySpec media_gap = UiDesignerNumberProperty(
        "media_gap", "Media gap", 10, 0, 100, 1, PropertyEditorKind::Integer);
    media_gap.group = "Appearance";
    media_gap.domain = PropertyEditorDomain::Appearance;
    media_gap.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(media_gap);
    spec.defaults.Set("media_gap", 10);

    UiDesignerPropertySpec media_auto_fit = UiDesignerBoolProperty("media_auto_fit", "Media auto fit", true);
    media_auto_fit.group = "Appearance";
    media_auto_fit.domain = PropertyEditorDomain::Appearance;
    media_auto_fit.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(media_auto_fit);
    spec.defaults.Set("media_auto_fit", true);

    UiDesignerPropertySpec media_share_percent = UiDesignerNumberProperty(
        "media_share_percent", "Media share percent", 0, 0, 100, 1, PropertyEditorKind::Integer);
    media_share_percent.group = "Appearance";
    media_share_percent.domain = PropertyEditorDomain::Appearance;
    media_share_percent.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(media_share_percent);
    spec.defaults.Set("media_share_percent", 0);

    UiDesignerPropertySpec content_inset = UiDesignerNumberProperty(
        "content_inset", "Content inset", 8, 0, 1000, 1, PropertyEditorKind::Integer);
    content_inset.group = "Appearance";
    content_inset.domain = PropertyEditorDomain::Appearance;
    content_inset.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(content_inset);
    spec.defaults.Set("content_inset", 8);

    UiDesignerPropertySpec content_cell_gap = UiDesignerNumberProperty(
        "content_cell_gap", "Content-cell gap", 8, 0, 1000, 1, PropertyEditorKind::Integer);
    content_cell_gap.group = "Appearance";
    content_cell_gap.domain = PropertyEditorDomain::Appearance;
    content_cell_gap.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(content_cell_gap);
    spec.defaults.Set("content_cell_gap", 8);

    UiDesignerPropertySpec show_title_line = UiDesignerBoolProperty("show_title_line", "Show title line", true);
    show_title_line.group = "Appearance";
    show_title_line.domain = PropertyEditorDomain::Appearance;
    show_title_line.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(show_title_line);
    spec.defaults.Set("show_title_line", true);

    UiDesignerPropertySpec title_line_length;
    title_line_length.id = "title_line_length";
    title_line_length.label = "Title line length";
    title_line_length.group = "Appearance";
    title_line_length.kind = PropertyEditorKind::Choice;
    title_line_length.domain = PropertyEditorDomain::Appearance;
    title_line_length.default_value = "Large";
    title_line_length.impact = PropertyImpactPaint | PropertyImpactCode;
    title_line_length.choices.Add(PropertyEditorChoice("None", "None"));
    title_line_length.choices.Add(PropertyEditorChoice("Small", "Small"));
    title_line_length.choices.Add(PropertyEditorChoice("Large", "Large"));
    spec.properties.Add(title_line_length);
    spec.defaults.Set("title_line_length", "Large");

    UiDesignerPropertySpec title_line_thickness = UiDesignerNumberProperty(
        "title_line_thickness", "Title line thickness", 1, 0, 20, 1, PropertyEditorKind::Integer);
    title_line_thickness.group = "Appearance";
    title_line_thickness.domain = PropertyEditorDomain::Appearance;
    title_line_thickness.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(title_line_thickness);
    spec.defaults.Set("title_line_thickness", 1);

    UiDesignerPropertySpec title_line_style;
    title_line_style.id = "title_line_style";
    title_line_style.label = "Title line style";
    title_line_style.group = "Appearance";
    title_line_style.kind = PropertyEditorKind::Choice;
    title_line_style.domain = PropertyEditorDomain::Appearance;
    title_line_style.default_value = "Solid";
    title_line_style.impact = PropertyImpactPaint | PropertyImpactCode;
    title_line_style.choices.Add(PropertyEditorChoice("Solid", "Solid"));
    title_line_style.choices.Add(PropertyEditorChoice("Dashed", "Dashed"));
    title_line_style.choices.Add(PropertyEditorChoice("Dotted", "Dotted"));
    spec.properties.Add(title_line_style);
    spec.defaults.Set("title_line_style", "Solid");

    UiDesignerPropertySpec show_card_line = UiDesignerBoolProperty("show_card_line", "Show card line", false);
    show_card_line.group = "Appearance";
    show_card_line.domain = PropertyEditorDomain::Appearance;
    show_card_line.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(show_card_line);
    spec.defaults.Set("show_card_line", false);

    UiDesignerPropertySpec card_line_side;
    card_line_side.id = "card_line_side";
    card_line_side.label = "Card line side";
    card_line_side.group = "Appearance";
    card_line_side.kind = PropertyEditorKind::Choice;
    card_line_side.domain = PropertyEditorDomain::Appearance;
    card_line_side.default_value = "Bottom";
    card_line_side.impact = PropertyImpactPaint | PropertyImpactCode;
    card_line_side.choices.Add(PropertyEditorChoice("Left", "Left"));
    card_line_side.choices.Add(PropertyEditorChoice("Right", "Right"));
    card_line_side.choices.Add(PropertyEditorChoice("Top", "Top"));
    card_line_side.choices.Add(PropertyEditorChoice("Bottom", "Bottom"));
    spec.properties.Add(card_line_side);
    spec.defaults.Set("card_line_side", "Bottom");

    UiDesignerPropertySpec card_line_length;
    card_line_length.id = "card_line_length";
    card_line_length.label = "Card line length";
    card_line_length.group = "Appearance";
    card_line_length.kind = PropertyEditorKind::Choice;
    card_line_length.domain = PropertyEditorDomain::Appearance;
    card_line_length.default_value = "Large";
    card_line_length.impact = PropertyImpactPaint | PropertyImpactCode;
    card_line_length.choices.Add(PropertyEditorChoice("None", "None"));
    card_line_length.choices.Add(PropertyEditorChoice("Small", "Small"));
    card_line_length.choices.Add(PropertyEditorChoice("Large", "Large"));
    spec.properties.Add(card_line_length);
    spec.defaults.Set("card_line_length", "Large");

    UiDesignerPropertySpec card_line_thickness = UiDesignerNumberProperty(
        "card_line_thickness", "Card line thickness", 1, 0, 20, 1, PropertyEditorKind::Integer);
    card_line_thickness.group = "Appearance";
    card_line_thickness.domain = PropertyEditorDomain::Appearance;
    card_line_thickness.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(card_line_thickness);
    spec.defaults.Set("card_line_thickness", 1);

    UiDesignerPropertySpec card_line_gap = UiDesignerNumberProperty(
        "card_line_gap", "Card line gap", 0, 0, 1000, 1, PropertyEditorKind::Integer);
    card_line_gap.group = "Appearance";
    card_line_gap.domain = PropertyEditorDomain::Appearance;
    card_line_gap.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(card_line_gap);
    spec.defaults.Set("card_line_gap", 0);

    UiDesignerPropertySpec hover_enabled = UiDesignerBoolProperty("hover_enabled", "Hover enabled", false);
    hover_enabled.group = "Appearance";
    hover_enabled.domain = PropertyEditorDomain::Appearance;
    hover_enabled.impact = PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(hover_enabled);
    spec.defaults.Set("hover_enabled", false);

    UiDesignerPropertySpec selectable = UiDesignerBoolProperty("selectable", "Selectable", true);
    selectable.group = "State";
    selectable.domain = PropertyEditorDomain::Behaviour;
    selectable.impact = PropertyImpactControlState | PropertyImpactPaint | PropertyImpactCode;
    spec.properties.Add(selectable);
    spec.defaults.Set("selectable", true);
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
        UiDesignerPropertySpec gap = UiDesignerNumberProperty(
            "gap", "Gap", 8, 0, 1000, 1, PropertyEditorKind::Integer);
        gap.group = "Layout";
        gap.domain = PropertyEditorDomain::Layout;
        gap.impact = PropertyImpactLocalLayout | PropertyImpactAncestorLayout |
                     PropertyImpactCode;
        s.properties.Add(gap);
        s.defaults.Set("inset", 8);
        s.defaults.Set("gap", 8);
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
        UiDesignerPropertySpec gap = UiDesignerNumberProperty(
            "gap", "Gap", 8, 0, 1000, 1, PropertyEditorKind::Integer);
        gap.group = "Layout";
        gap.domain = PropertyEditorDomain::Layout;
        gap.impact = PropertyImpactLocalLayout | PropertyImpactAncestorLayout |
                     PropertyImpactCode;
        s.properties.Add(gap);
        s.defaults.Set("inset", 8);
        s.defaults.Set("gap", 8);
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
        auto min_cell_width = UiDesignerNumberProperty(
            "min_cell_width", "Minimum cell width", 10, 0, 10000, 1,
            PropertyEditorKind::Integer);
        min_cell_width.group = "Structure";
        min_cell_width.impact = PropertyImpactLocalLayout |
                                 PropertyImpactAncestorLayout | PropertyImpactCode;
        auto min_cell_height = UiDesignerNumberProperty(
            "min_cell_height", "Minimum cell height", 10, 0, 10000, 1,
            PropertyEditorKind::Integer);
        min_cell_height.group = "Structure";
        min_cell_height.impact = min_cell_width.impact;
        s.properties.Add(min_cell_width);
        s.properties.Add(min_cell_height);
        s.defaults.Set("min_cell_width", 10);
        s.defaults.Set("min_cell_height", 10);
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
            if(String(c.type) == "UiAccordion") {
                AddEvent(s, "WhenSectionToggled", "Section toggled", "Runs after a section changes open state.");
                AddEvent(s, "WhenReordered", "Sections reordered", "Runs after sections are reordered.");
                AddEvent(s, "WhenRemoved", "Section removed", "Runs after a section is removed.");
                AddEvent(s, "WhenAdded", "Section added", "Runs after a section is added.");
            }
            else
                AddEvent(s, "WhenAction", "Page changed", "Runs after the active page changes.");
        }
        if(String(c.type) == "UiAccordion") {
            s.theme = true;
            s.theme_adapter_id = "accordion";
            if(const UiDesignerThemeAdapter* adapter = UiDesignerFindThemeAdapter(s.theme_adapter_id))
                adapter->AddThemeOverrides(s);
        }
        if(String(c.type) == "UiScrollPanel" ||
           String(c.type) == "UiDirectContentHost")
            s.child_adapter_id = "single";
        if(String(c.type) == "UiGroupPanel" || String(c.type) == "UiTitleCard")
            AddTitle(s, c.display);
        if(String(c.type) == "UiTitleCard") {
            s.child_adapter_id = "title_card";
            s.theme = true;
            s.theme_adapter_id = "title_card";
            if(const UiDesignerThemeAdapter* adapter = UiDesignerFindThemeAdapter(s.theme_adapter_id))
                adapter->AddThemeOverrides(s);
            AddEvent(s, "WhenAction", "Action", "Runs when the card is activated.");
            AddTitleCardProperties(s);
            s.defaults.Set("width_mode", "Expand");
            s.defaults.Set("height_mode", "Expand");
            s.defaults.Set("cell_align_x", "Stretch");
            s.defaults.Set("cell_align_y", "Stretch");
            UiDesignerPropertySpec icon = ChoiceProperty(
                "icon", "Icon", "Content", "ICON_DESIGN_DESCRIPTION_48",
                {{"None", "None"},
                 {"ICON_DESIGN_DESCRIPTION_48", "Description"}},
                PropertyImpactControlState | PropertyImpactLocalLayout |
                PropertyImpactCode);
            icon.domain = PropertyEditorDomain::Content;
            s.properties.Add(icon);
            s.defaults.Set("icon", "ICON_DESIGN_DESCRIPTION_48");
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
        if(String(c.type) == "UiButton" || String(c.type) == "UiToolButton") {
            AddButtonProperties(s);
            s.theme_adapter_id = String(c.type) == "UiToolButton" ? "tool_button" : "button";
            s.theme = true;
            if(const UiDesignerThemeAdapter* adapter = UiDesignerFindThemeAdapter(s.theme_adapter_id))
                adapter->AddThemeOverrides(s);
            AddEvent(s, "WhenAction", "Clicked", "Runs when the button is activated.");
            if(String(c.type) == "UiToolButton")
                s.defaults.Set("icon", "ICON_DESIGN_TUNE_48");
        }
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
            auto alpha = UiDesignerNumberProperty("alpha", "Alpha", 255, 0, 255, 1,
                                                  PropertyEditorKind::Integer);
            alpha.group = "Value";
            alpha.impact = PropertyImpactControlState | PropertyImpactPaint | PropertyImpactCode;
            s.properties.Add(alpha);
            s.defaults.Set("alpha", 255);
            auto alpha_enabled = UiDesignerBoolProperty("alpha_enabled", "Alpha enabled", true);
            alpha_enabled.group = "Value";
            alpha_enabled.impact = PropertyImpactControlState | PropertyImpactPaint | PropertyImpactCode;
            s.properties.Add(alpha_enabled);
            s.defaults.Set("alpha_enabled", true);
            s.properties.Add(ChoiceProperty("page_mode", "Page mode", "Configuration", "color",
                {{"color", "Color"}, {"palettes", "Palettes"}, {"generator", "Generator"}}));
            s.defaults.Set("page_mode", "color");
            s.properties.Add(ChoiceProperty("channel_mode", "Channel mode", "Configuration", "rgb_float",
                {{"rgb_float", "RGB float"}, {"rgb_integer", "RGB integer"}, {"hsv", "HSV"},
                 {"hsl", "HSL"}, {"tmi", "TMI"}, {"cmyk", "CMYK"}, {"lab", "Lab"}},
                PropertyImpactControlState | PropertyImpactPaint | PropertyImpactCode));
            s.defaults.Set("channel_mode", "rgb_float");
            s.properties.Add(ChoiceProperty("spectrum_mode", "Spectrum mode", "Configuration", "hue_strip",
                {{"hsv_rectangle", "HSV rectangle"}, {"hue_strip", "Hue strip"},
                 {"rgb_spectrum", "RGB spectrum"}, {"hsv_wheel", "HSV wheel"}},
                PropertyImpactControlState | PropertyImpactPaint | PropertyImpactCode));
            s.defaults.Set("spectrum_mode", "hue_strip");
            s.properties.Add(ChoiceProperty("harmony_mode", "Harmony mode", "Configuration", "triad",
                {{"custom", "Custom"}, {"analogous", "Analogous"}, {"complementary", "Complementary"},
                 {"split_complementary", "Split complementary"}, {"triad", "Triad"}, {"square", "Square"},
                 {"compound", "Compound"}, {"shades", "Shades"}, {"monochromatic", "Monochromatic"},
                 {"image_extract", "Image extract"}},
                PropertyImpactControlState | PropertyImpactPaint | PropertyImpactCode));
            s.defaults.Set("harmony_mode", "triad");
            auto slots = UiDesignerNumberProperty("slot_count", "Slot count", 4, 1, 4, 1,
                                                  PropertyEditorKind::Integer);
            slots.group = "Configuration";
            slots.impact = PropertyImpactControlState | PropertyImpactPaint | PropertyImpactCode;
            s.properties.Add(slots);
            s.defaults.Set("slot_count", 4);
            auto active_slot = UiDesignerNumberProperty("active_slot", "Active slot", 0, 0, 3, 1,
                                                        PropertyEditorKind::Integer);
            active_slot.group = "Configuration";
            active_slot.impact = PropertyImpactControlState | PropertyImpactPaint | PropertyImpactCode;
            s.properties.Add(active_slot);
            s.defaults.Set("active_slot", 0);
            s.theme_adapter_id = "color_picker";
            s.theme = true;
            if(const UiDesignerThemeAdapter* adapter = UiDesignerFindThemeAdapter(s.theme_adapter_id))
                adapter->AddThemeOverrides(s);
            AddEvent(s, "WhenChanging", "Colour changing", "Runs during colour preview.");
            AddEvent(s, "WhenAccept", "Colour accepted", "Runs after colour acceptance.");
            AddEvent(s, "WhenCancel", "Colour cancelled", "Runs when colour editing is cancelled.");
        }
        if(String(c.type) == "UiTree" || String(c.type) == "UiList" || String(c.type) == "UiMenu") {
            s.theme_adapter_id = String(c.type) == "UiTree" ? "tree" :
                                 String(c.type) == "UiList" ? "list" : "menu";
            s.theme = true;
            if(const UiDesignerThemeAdapter* adapter = UiDesignerFindThemeAdapter(s.theme_adapter_id))
                adapter->AddThemeOverrides(s);
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

static void RegisterInternalSemanticItems(UiDesignerCatalog& catalog)
{
    UiDesignerControlSpec page = MakeSpec("UiTabPage", "Tab Page", "Internal",
        "", "tab_page", UiDesignerRuntimeKind::SemanticTabPage,
        "none", UiDesignerNodeStructural | UiDesignerNodeSemanticItem);
    page.capabilities = UiDesignerCapabilitySemanticItem;
    page.preview_adapter_id = "semantic:tab_page";
    page.codegen_adapter_id = "tab_page_deferred";
    page.semantic_owner_type = "UiTab";
    page.properties.Clear();
    page.defaults.Clear();
    UiDesignerPropertySpec key;
    key.id = "key"; key.label = "Key"; key.group = "Page";
    key.kind = PropertyEditorKind::Text; key.default_value = "page_1";
    key.impact = PropertyImpactStructure | PropertyImpactCode;
    page.properties.Add(key); page.defaults.Set("key", "page_1");
    UiDesignerPropertySpec title = key;
    title.id = "title"; title.label = "Title"; title.default_value = "Page 1";
    page.properties.Add(title); page.defaults.Set("title", "Page 1");
    UiDesignerPropertySpec enabled = UiDesignerBoolProperty("enabled", "Enabled", true);
    enabled.group = "Page"; enabled.impact = PropertyImpactStructure | PropertyImpactPaint;
    page.properties.Add(enabled); page.defaults.Set("enabled", true);
    catalog.Register(pick(page));

    UiDesignerControlSpec section = MakeSpec("UiAccordionSection", "Accordion Section", "Internal",
        "", "accordion_section", UiDesignerRuntimeKind::SemanticAccordionSection,
        "none", UiDesignerNodeStructural | UiDesignerNodeSemanticItem);
    section.capabilities = UiDesignerCapabilitySemanticItem;
    section.preview_adapter_id = "semantic:accordion_section";
    section.codegen_adapter_id = "accordion_section";
    section.semantic_owner_type = "UiAccordion";
    section.properties.Clear();
    section.defaults.Clear();
    UiDesignerPropertySpec skey = key;
    skey.default_value = "overview";
    section.properties.Add(skey); section.defaults.Set("key", "overview");
    UiDesignerPropertySpec stitle = title;
    stitle.default_value = "Overview";
    section.properties.Add(stitle); section.defaults.Set("title", "Overview");
    UiDesignerPropertySpec subtitle = UiDesignerTextProperty("subtitle", "Subtitle");
    subtitle.group = "Section"; subtitle.impact = PropertyImpactStructure | PropertyImpactCode;
    section.properties.Add(subtitle); section.defaults.Set("subtitle", "Summary");
    UiDesignerPropertySpec copy = UiDesignerTextProperty("copy", "Copy");
    copy.group = "Section"; copy.impact = PropertyImpactStructure | PropertyImpactCode;
    section.properties.Add(copy); section.defaults.Set("copy", "Overview content");
    UiDesignerPropertySpec open = UiDesignerBoolProperty("open", "Open", true);
    open.group = "Section"; open.impact = PropertyImpactControlState | PropertyImpactCode;
    section.properties.Add(open); section.defaults.Set("open", true);
    UiDesignerPropertySpec lock = ChoiceProperty("lock", "Lock", "Section", "None",
        {{"None", "None"}, {"Open", "Open"}, {"Closed", "Closed"}},
        PropertyImpactControlState | PropertyImpactCode);
    section.properties.Add(lock); section.defaults.Set("lock", "None");
    catalog.Register(pick(section));
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
    RegisterInternalSemanticItems(catalog);
}

}
