#include "UiDesignerThemeAdapter.h"
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>
#include <Utilities/UiDesigner/Core/UiDesignerOverlay.h>
#include <Utilities/UiDesigner/UiDesigner/UiDesignerButtonStyle.h>
#include <Ui/UiButton.h>
#include <Ui/UiTree.h>
#include <Ui/UiList.h>
#include <Ui/UiMenu.h>
#include <Ui/UiColorPicker.h>
#include <Ui/UiTitleCard.h>
#include <Ui/UiAccordion.h>
#include <Ui/UiTab.h>
#include <Ui/UiPanel.h>
#include <Ui/UiGroupPanel.h>
#include <Ui/UiScrollPanel.h>
#include <Ui/UiLabel.h>
#include <Ui/UiCheckBox.h>
#include <Ui/UiRadioButton.h>
#include <Ui/UiToggle.h>
#include <Ui/UiProgressBar.h>
#include <Ui/UiSlider.h>
#include <Ui/UiScrollBar.h>
#include <Ui/UiDropdown.h>
#include <Ui/UiBaseEdit.h>
#include <Ui/UiTheme.h>

namespace Upp {

UiDesignerSurfaceKind UiDesignerParseSurfaceKind(const Value& value)
{
    const String name = value;
    if(name == "None") return UiDesignerSurfaceKind::None;
    if(name == "Solid") return UiDesignerSurfaceKind::Solid;
    if(name == "Gradient") return UiDesignerSurfaceKind::Gradient;
    if(name == "Image") return UiDesignerSurfaceKind::Image;
    if(name == "NineSlice") return UiDesignerSurfaceKind::NineSlice;
    if(name == "Dashed") return UiDesignerSurfaceKind::Dashed;
    return UiDesignerSurfaceKind::UseTheme;
}

String UiDesignerSurfaceKindName(UiDesignerSurfaceKind kind)
{
    switch(kind) {
    case UiDesignerSurfaceKind::None: return "None";
    case UiDesignerSurfaceKind::Solid: return "Solid";
    case UiDesignerSurfaceKind::Gradient: return "Gradient";
    case UiDesignerSurfaceKind::Image: return "Image";
    case UiDesignerSurfaceKind::NineSlice: return "NineSlice";
    case UiDesignerSurfaceKind::Dashed: return "Dashed";
    default: return "UseTheme";
    }
}

void UiDesignerAddSurfaceChoices(UiDesignerThemeOverrideSpec& spec, bool include_dashed)
{
    spec.Choice("UseTheme", "Use theme")
        .Choice("None", "None")
        .Choice("Solid", "Solid")
        .Choice("Gradient", "Gradient")
        .Choice("Image", "Image")
        .Choice("NineSlice", "Nine-slice");
    if(include_dashed)
        spec.Choice("Dashed", "Dashed");
}

static UiRole ParseRole(const Value& value)
{
    const String role = value;
    if(role == "Subtle") return UiRole::Subtle;
    if(role == "Accent") return UiRole::Accent;
    if(role == "Alert") return UiRole::Alert;
    return UiRole::Standard;
}

static bool HasThemeValue(const UiDesignerNode& node,
                          const UiDesignerTransientOverlay* overlay,
                          const String& property)
{
    const bool authored = node.theme_overrides.Find(property) >= 0;
    return authored || (overlay && overlay->Has(node.id,
        UiDesignerTransientValueKind::ThemeOverride, property));
}

static Value ResolveThemeValue(const UiDesignerNode& node,
                               const UiDesignerTransientOverlay* overlay,
                               const String& property,
                               const Value& canonical)
{
    return overlay
        ? overlay->Resolve(node.id, UiDesignerTransientValueKind::ThemeOverride,
                           property, canonical)
        : canonical;
}

static String CppString(const String& text)
{
    String out = "\"";
    for(int i = 0; i < text.GetCount(); i++) {
        const byte c = text[i];
        if(c == '\\') out << "\\\\";
        else if(c == '"') out << "\\\"";
        else if(c == '\n') out << "\\n";
        else if(c == '\r') out << "\\r";
        else if(c == '\t') out << "\\t";
        else out.Cat(c);
    }
    out << "\"";
    return out;
}

static String EmitRoleExpr(const String& value)
{
    if(value == "Subtle") return "UiRole::Subtle";
    if(value == "Accent") return "UiRole::Accent";
    if(value == "Alert") return "UiRole::Alert";
    return "UiRole::Standard";
}

static String EmitAlign(const String& value)
{
    if(value == "Left") return "UiAlign::LEFT";
    if(value == "Right") return "UiAlign::RIGHT";
    if(value == "Top") return "UiAlign::TOP";
    if(value == "Bottom") return "UiAlign::BOTTOM";
    return "UiAlign::CENTER";
}

static String EmitValue(const Value& value)
{
    if(IsNull(value))
        return "Value()";
    if(value.Is<String>())
        return CppString(value);
    if(value.Is<bool>())
        return (bool)value ? "true" : "false";
    if(value.Is<int>() || value.Is<int64_t>())
        return AsString(value);
    if(value.Is<double>())
        return Format("%.12g", (double)value);
    if(value.Is<Color>()) {
        Color c = value;
        return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
    }
    return "ParseJSON(" + CppString(AsJSON(value, false)) + ")";
}

static Color UiDesignerFillColor(const UiFill& fill)
{
    return fill.IsSolid() ? fill.color : Null;
}

const char *UiDesignerButtonStyleFieldName(UiDesignerButtonStyleField field)
{
    switch(field) {
    case UiDesignerButtonStyleField::None: return "";
    case UiDesignerButtonStyleField::FontFace: return "font_face";
    case UiDesignerButtonStyleField::FontSize: return "font_size";
    case UiDesignerButtonStyleField::FontBold: return "font_bold";
    case UiDesignerButtonStyleField::FontItalic: return "font_italic";
    case UiDesignerButtonStyleField::FaceEnabled: return "face_enabled";
    case UiDesignerButtonStyleField::FaceNormal: return "face_normal";
    case UiDesignerButtonStyleField::FaceHot: return "face_hot";
    case UiDesignerButtonStyleField::FacePressed: return "face_pressed";
    case UiDesignerButtonStyleField::FaceDisabled: return "face_disabled";
    case UiDesignerButtonStyleField::Transparent: return "transparent";
    case UiDesignerButtonStyleField::FrameEnabled: return "frame_enabled";
    case UiDesignerButtonStyleField::FrameNormal: return "frame_normal";
    case UiDesignerButtonStyleField::FrameHot: return "frame_hot";
    case UiDesignerButtonStyleField::FramePressed: return "frame_pressed";
    case UiDesignerButtonStyleField::FrameDisabled: return "frame_disabled";
    case UiDesignerButtonStyleField::FrameWidth: return "frame_width";
    case UiDesignerButtonStyleField::Radius: return "radius";
    case UiDesignerButtonStyleField::FrameDashed: return "frame_dashed";
    case UiDesignerButtonStyleField::FrameDashPattern: return "frame_dash_pattern";
    case UiDesignerButtonStyleField::TextNormal: return "text_normal";
    case UiDesignerButtonStyleField::TextHot: return "text_hot";
    case UiDesignerButtonStyleField::TextPressed: return "text_pressed";
    case UiDesignerButtonStyleField::TextDisabled: return "text_disabled";
    case UiDesignerButtonStyleField::IconNormal: return "icon_normal";
    case UiDesignerButtonStyleField::IconHot: return "icon_hot";
    case UiDesignerButtonStyleField::IconPressed: return "icon_pressed";
    case UiDesignerButtonStyleField::IconDisabled: return "icon_disabled";
    case UiDesignerButtonStyleField::ShadowEnabled: return "shadow_enabled";
    case UiDesignerButtonStyleField::ShadowDistance: return "shadow_distance";
    case UiDesignerButtonStyleField::ShadowOffsetX: return "shadow_offset_x";
    case UiDesignerButtonStyleField::ShadowOffsetY: return "shadow_offset_y";
    case UiDesignerButtonStyleField::ShadowAlpha: return "shadow_alpha";
    case UiDesignerButtonStyleField::ShadowColor: return "shadow_color";
    case UiDesignerButtonStyleField::ShadowInset: return "shadow_inset";
    case UiDesignerButtonStyleField::ShadowMode: return "shadow_mode";
    case UiDesignerButtonStyleField::PressOffsetX: return "press_offset_x";
    case UiDesignerButtonStyleField::PressOffsetY: return "press_offset_y";
    case UiDesignerButtonStyleField::Overpaint: return "overpaint";
    case UiDesignerButtonStyleField::UnderlineEnabled: return "underline_enabled";
    case UiDesignerButtonStyleField::UnderlineWidth: return "underline_width";
    case UiDesignerButtonStyleField::UnderlineOffset: return "underline_offset";
    }
    return "";
}

bool UiDesignerParseButtonStyleField(const String& id,
                                    UiDesignerButtonStyleField& field)
{
    struct Item { const char *id; UiDesignerButtonStyleField field; };
    static const Item items[] = {
        {"font_face", UiDesignerButtonStyleField::FontFace},
        {"font_size", UiDesignerButtonStyleField::FontSize},
        {"font_bold", UiDesignerButtonStyleField::FontBold},
        {"font_italic", UiDesignerButtonStyleField::FontItalic},
        {"face_enabled", UiDesignerButtonStyleField::FaceEnabled},
        {"face_normal", UiDesignerButtonStyleField::FaceNormal},
        {"face_hot", UiDesignerButtonStyleField::FaceHot},
        {"face_pressed", UiDesignerButtonStyleField::FacePressed},
        {"face_disabled", UiDesignerButtonStyleField::FaceDisabled},
        {"transparent", UiDesignerButtonStyleField::Transparent},
        {"frame_enabled", UiDesignerButtonStyleField::FrameEnabled},
        {"frame_normal", UiDesignerButtonStyleField::FrameNormal},
        {"frame_hot", UiDesignerButtonStyleField::FrameHot},
        {"frame_pressed", UiDesignerButtonStyleField::FramePressed},
        {"frame_disabled", UiDesignerButtonStyleField::FrameDisabled},
        {"frame_width", UiDesignerButtonStyleField::FrameWidth},
        {"radius", UiDesignerButtonStyleField::Radius},
        {"frame_dashed", UiDesignerButtonStyleField::FrameDashed},
        {"frame_dash_pattern", UiDesignerButtonStyleField::FrameDashPattern},
        {"text_normal", UiDesignerButtonStyleField::TextNormal},
        {"text_hot", UiDesignerButtonStyleField::TextHot},
        {"text_pressed", UiDesignerButtonStyleField::TextPressed},
        {"text_disabled", UiDesignerButtonStyleField::TextDisabled},
        {"icon_normal", UiDesignerButtonStyleField::IconNormal},
        {"icon_hot", UiDesignerButtonStyleField::IconHot},
        {"icon_pressed", UiDesignerButtonStyleField::IconPressed},
        {"icon_disabled", UiDesignerButtonStyleField::IconDisabled},
        {"shadow_enabled", UiDesignerButtonStyleField::ShadowEnabled},
        {"shadow_distance", UiDesignerButtonStyleField::ShadowDistance},
        {"shadow_offset_x", UiDesignerButtonStyleField::ShadowOffsetX},
        {"shadow_offset_y", UiDesignerButtonStyleField::ShadowOffsetY},
        {"shadow_alpha", UiDesignerButtonStyleField::ShadowAlpha},
        {"shadow_color", UiDesignerButtonStyleField::ShadowColor},
        {"shadow_inset", UiDesignerButtonStyleField::ShadowInset},
        {"shadow_mode", UiDesignerButtonStyleField::ShadowMode},
        {"press_offset_x", UiDesignerButtonStyleField::PressOffsetX},
        {"press_offset_y", UiDesignerButtonStyleField::PressOffsetY},
        {"overpaint", UiDesignerButtonStyleField::Overpaint},
        {"underline_enabled", UiDesignerButtonStyleField::UnderlineEnabled},
        {"underline_width", UiDesignerButtonStyleField::UnderlineWidth},
        {"underline_offset", UiDesignerButtonStyleField::UnderlineOffset},
    };
    for(const Item& item : items)
        if(id == item.id) {
            field = item.field;
            return true;
        }
    field = UiDesignerButtonStyleField::None;
    return false;
}

bool UiDesignerButtonStyleFieldAffectsLayout(UiDesignerButtonStyleField field)
{
    switch(field) {
    case UiDesignerButtonStyleField::FontFace:
    case UiDesignerButtonStyleField::FontSize:
    case UiDesignerButtonStyleField::FontBold:
    case UiDesignerButtonStyleField::FontItalic:
    case UiDesignerButtonStyleField::FaceEnabled:
    case UiDesignerButtonStyleField::FaceNormal:
    case UiDesignerButtonStyleField::FaceHot:
    case UiDesignerButtonStyleField::FacePressed:
    case UiDesignerButtonStyleField::FaceDisabled:
    case UiDesignerButtonStyleField::FrameEnabled:
    case UiDesignerButtonStyleField::FrameNormal:
    case UiDesignerButtonStyleField::FrameHot:
    case UiDesignerButtonStyleField::FramePressed:
    case UiDesignerButtonStyleField::FrameDisabled:
    case UiDesignerButtonStyleField::FrameWidth:
    case UiDesignerButtonStyleField::Radius:
    case UiDesignerButtonStyleField::FrameDashed:
    case UiDesignerButtonStyleField::FrameDashPattern:
    case UiDesignerButtonStyleField::ShadowEnabled:
    case UiDesignerButtonStyleField::ShadowDistance:
    case UiDesignerButtonStyleField::ShadowOffsetX:
    case UiDesignerButtonStyleField::ShadowOffsetY:
    case UiDesignerButtonStyleField::ShadowAlpha:
    case UiDesignerButtonStyleField::ShadowColor:
    case UiDesignerButtonStyleField::ShadowInset:
    case UiDesignerButtonStyleField::ShadowMode:
    case UiDesignerButtonStyleField::Overpaint:
    case UiDesignerButtonStyleField::UnderlineEnabled:
    case UiDesignerButtonStyleField::UnderlineWidth:
    case UiDesignerButtonStyleField::UnderlineOffset:
        return true;
    default:
        return false;
    }
}

void UiDesignerApplyButtonStyleField(UiButton::Style& style,
                                     UiDesignerButtonStyleField field,
                                     const Value& value)
{
    switch(field) {
    case UiDesignerButtonStyleField::FontFace:
        style.font.FaceName(AsString(value));
        break;
    case UiDesignerButtonStyleField::FontSize:
        style.font.Height(max(1, (int)value));
        break;
    case UiDesignerButtonStyleField::FontBold:
        style.font.Bold((bool)value);
        break;
    case UiDesignerButtonStyleField::FontItalic:
        style.font.Italic((bool)value);
        break;
    case UiDesignerButtonStyleField::FaceEnabled:
        style.metrics.face_enabled = (bool)value;
        break;
    case UiDesignerButtonStyleField::FaceNormal:
        style.palette.face[ST_NORMAL] = UiFill::Solid((Color)value);
        break;
    case UiDesignerButtonStyleField::FaceHot:
        style.palette.face[ST_HOT] = UiFill::Solid((Color)value);
        break;
    case UiDesignerButtonStyleField::FacePressed:
        style.palette.face[ST_PRESSED] = UiFill::Solid((Color)value);
        break;
    case UiDesignerButtonStyleField::FaceDisabled:
        style.palette.face[ST_DISABLED] = UiFill::Solid((Color)value);
        break;
    case UiDesignerButtonStyleField::Transparent:
        style.transparent = (bool)value;
        break;
    case UiDesignerButtonStyleField::FrameEnabled:
        style.metrics.frame_enabled = (bool)value;
        break;
    case UiDesignerButtonStyleField::FrameNormal:
        style.palette.frame[ST_NORMAL] = (Color)value;
        break;
    case UiDesignerButtonStyleField::FrameHot:
        style.palette.frame[ST_HOT] = (Color)value;
        break;
    case UiDesignerButtonStyleField::FramePressed:
        style.palette.frame[ST_PRESSED] = (Color)value;
        break;
    case UiDesignerButtonStyleField::FrameDisabled:
        style.palette.frame[ST_DISABLED] = (Color)value;
        break;
    case UiDesignerButtonStyleField::FrameWidth:
        style.metrics.frame_width = max(0, (int)value);
        break;
    case UiDesignerButtonStyleField::Radius:
        style.metrics.radius = max(0, (int)value);
        break;
    case UiDesignerButtonStyleField::FrameDashed:
        style.metrics.dashed = (bool)value;
        break;
    case UiDesignerButtonStyleField::FrameDashPattern:
        style.metrics.dash_pattern = AsString(value);
        break;
    case UiDesignerButtonStyleField::TextNormal:
        style.palette.ink[ST_NORMAL] = (Color)value;
        break;
    case UiDesignerButtonStyleField::TextHot:
        style.palette.ink[ST_HOT] = (Color)value;
        break;
    case UiDesignerButtonStyleField::TextPressed:
        style.palette.ink[ST_PRESSED] = (Color)value;
        break;
    case UiDesignerButtonStyleField::TextDisabled:
        style.palette.ink[ST_DISABLED] = (Color)value;
        break;
    case UiDesignerButtonStyleField::IconNormal:
        style.palette.icon[ST_NORMAL] = (Color)value;
        break;
    case UiDesignerButtonStyleField::IconHot:
        style.palette.icon[ST_HOT] = (Color)value;
        break;
    case UiDesignerButtonStyleField::IconPressed:
        style.palette.icon[ST_PRESSED] = (Color)value;
        break;
    case UiDesignerButtonStyleField::IconDisabled:
        style.palette.icon[ST_DISABLED] = (Color)value;
        break;
    case UiDesignerButtonStyleField::ShadowEnabled:
        style.metrics.shadow.enabled = (bool)value;
        break;
    case UiDesignerButtonStyleField::ShadowDistance:
        style.metrics.shadow.distance = max(0, (int)value);
        break;
    case UiDesignerButtonStyleField::ShadowOffsetX:
        style.metrics.shadow.offset_x = (int)value;
        break;
    case UiDesignerButtonStyleField::ShadowOffsetY:
        style.metrics.shadow.offset_y = (int)value;
        break;
    case UiDesignerButtonStyleField::ShadowAlpha:
        style.metrics.shadow.alpha = minmax((int)value, 0, 255);
        break;
    case UiDesignerButtonStyleField::ShadowColor:
        style.metrics.shadow.color = (Color)value;
        break;
    case UiDesignerButtonStyleField::ShadowInset:
        style.metrics.shadow.inset = (bool)value;
        break;
    case UiDesignerButtonStyleField::ShadowMode:
        style.metrics.shadow.mode = value == "Hard" ? SHADOW_HARD : SHADOW_CURVE;
        break;
    case UiDesignerButtonStyleField::PressOffsetX:
        style.press_offset.x = (int)value;
        break;
    case UiDesignerButtonStyleField::PressOffsetY:
        style.press_offset.y = (int)value;
        break;
    case UiDesignerButtonStyleField::Overpaint:
        style.overpaint = max(0, (int)value);
        break;
    case UiDesignerButtonStyleField::UnderlineEnabled:
        style.underline = (bool)value;
        break;
    case UiDesignerButtonStyleField::UnderlineWidth:
        style.underline_width = max(0, (int)value);
        break;
    case UiDesignerButtonStyleField::UnderlineOffset:
        style.underline_offset = (int)value;
        break;
    default:
        break;
    }
}

Value UiDesignerButtonStyleFieldValue(const UiButton::Style& style,
                                     UiDesignerButtonStyleField field)
{
    switch(field) {
    case UiDesignerButtonStyleField::FontFace: return style.font.GetFaceName();
    case UiDesignerButtonStyleField::FontSize: return style.font.GetHeight();
    case UiDesignerButtonStyleField::FontBold: return style.font.IsBold();
    case UiDesignerButtonStyleField::FontItalic: return style.font.IsItalic();
    case UiDesignerButtonStyleField::FaceEnabled: return style.metrics.face_enabled;
    case UiDesignerButtonStyleField::FaceNormal: return UiDesignerFillColor(style.palette.face[ST_NORMAL]);
    case UiDesignerButtonStyleField::FaceHot: return UiDesignerFillColor(style.palette.face[ST_HOT]);
    case UiDesignerButtonStyleField::FacePressed: return UiDesignerFillColor(style.palette.face[ST_PRESSED]);
    case UiDesignerButtonStyleField::FaceDisabled: return UiDesignerFillColor(style.palette.face[ST_DISABLED]);
    case UiDesignerButtonStyleField::Transparent: return style.transparent;
    case UiDesignerButtonStyleField::FrameEnabled: return style.metrics.frame_enabled;
    case UiDesignerButtonStyleField::FrameNormal: return style.palette.frame[ST_NORMAL];
    case UiDesignerButtonStyleField::FrameHot: return style.palette.frame[ST_HOT];
    case UiDesignerButtonStyleField::FramePressed: return style.palette.frame[ST_PRESSED];
    case UiDesignerButtonStyleField::FrameDisabled: return style.palette.frame[ST_DISABLED];
    case UiDesignerButtonStyleField::FrameWidth: return style.metrics.frame_width;
    case UiDesignerButtonStyleField::Radius: return style.metrics.radius;
    case UiDesignerButtonStyleField::FrameDashed: return style.metrics.dashed;
    case UiDesignerButtonStyleField::FrameDashPattern: return style.metrics.dash_pattern;
    case UiDesignerButtonStyleField::TextNormal: return style.palette.ink[ST_NORMAL];
    case UiDesignerButtonStyleField::TextHot: return style.palette.ink[ST_HOT];
    case UiDesignerButtonStyleField::TextPressed: return style.palette.ink[ST_PRESSED];
    case UiDesignerButtonStyleField::TextDisabled: return style.palette.ink[ST_DISABLED];
    case UiDesignerButtonStyleField::IconNormal: return style.palette.icon[ST_NORMAL];
    case UiDesignerButtonStyleField::IconHot: return style.palette.icon[ST_HOT];
    case UiDesignerButtonStyleField::IconPressed: return style.palette.icon[ST_PRESSED];
    case UiDesignerButtonStyleField::IconDisabled: return style.palette.icon[ST_DISABLED];
    case UiDesignerButtonStyleField::ShadowEnabled: return style.metrics.shadow.enabled;
    case UiDesignerButtonStyleField::ShadowDistance: return style.metrics.shadow.distance;
    case UiDesignerButtonStyleField::ShadowOffsetX: return style.metrics.shadow.offset_x;
    case UiDesignerButtonStyleField::ShadowOffsetY: return style.metrics.shadow.offset_y;
    case UiDesignerButtonStyleField::ShadowAlpha: return style.metrics.shadow.alpha;
    case UiDesignerButtonStyleField::ShadowColor: return style.metrics.shadow.color;
    case UiDesignerButtonStyleField::ShadowInset: return style.metrics.shadow.inset;
    case UiDesignerButtonStyleField::ShadowMode: return style.metrics.shadow.mode == SHADOW_HARD ? "Hard" : "Curve";
    case UiDesignerButtonStyleField::PressOffsetX: return style.press_offset.x;
    case UiDesignerButtonStyleField::PressOffsetY: return style.press_offset.y;
    case UiDesignerButtonStyleField::Overpaint: return style.overpaint;
    case UiDesignerButtonStyleField::UnderlineEnabled: return style.underline;
    case UiDesignerButtonStyleField::UnderlineWidth: return style.underline_width;
    case UiDesignerButtonStyleField::UnderlineOffset: return style.underline_offset;
    default: break;
    }
    return Value();
}

static UiButton::Style ResolveButtonStyleBase(bool tool_button, const UiDesignerNode& node)
{
    const UiRole role = ParseRole(node.GetProperty("role", "Standard"));
    return tool_button ? UiTheme::ResolveToolButton(role)
                       : UiTheme::ResolveButton(role);
}

static void AddOverride(UiDesignerControlSpec& spec, const String& id,
                        const String& label, const String& group,
                        PropertyEditorKind kind, const Value& value,
                        PropertyEditorImpact impact, const String& adapter_field,
                        const String& help = String())
{
    UiDesignerThemeOverrideSpec item;
    item.id = id;
    item.label = label;
    item.group = group;
    item.kind = kind;
    item.domain = PropertyEditorDomain::Theme;
    item.default_value = value;
    item.impact = impact;
    item.adapter_field_id = adapter_field;
    item.help = help;
    spec.theme_overrides.Add(pick(item));
}

static void AddButtonThemeOverrides(UiDesignerControlSpec& spec, bool tool_button)
{
    const UiButton::Style base = tool_button ? UiTheme::ResolveToolButton(UiRole::Standard)
                                             : UiTheme::ResolveButton(UiRole::Standard);
    const auto add = [&](UiDesignerButtonStyleField field, const char *label,
                         const char *group, PropertyEditorKind kind,
                         const Value& value, PropertyEditorImpact impact,
                         const char *help = nullptr) {
        AddOverride(spec, UiDesignerButtonStyleFieldName(field), label, group, kind,
                    value, impact, UiDesignerButtonStyleFieldName(field),
                    help ? String(help) : String());
    };
    const auto add_bool = [&](UiDesignerButtonStyleField field, const char *label,
                              const char *group, bool value,
                              PropertyEditorImpact impact = PropertyImpactPaint | PropertyImpactCode) {
        add(field, label, group, PropertyEditorKind::Boolean, value, impact);
    };
    const auto add_int = [&](UiDesignerButtonStyleField field, const char *label,
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
        item.adapter_field_id = UiDesignerButtonStyleFieldName(field);
        spec.theme_overrides.Add(pick(item));
    };
    const auto add_color = [&](UiDesignerButtonStyleField field, const char *label,
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
              UiDesignerFillColor(base.palette.face[ST_NORMAL]));
    add_color(UiDesignerButtonStyleField::FaceHot, "Face hot", "Face",
              UiDesignerFillColor(base.palette.face[ST_HOT]));
    add_color(UiDesignerButtonStyleField::FacePressed, "Face pressed", "Face",
              UiDesignerFillColor(base.palette.face[ST_PRESSED]));
    add_color(UiDesignerButtonStyleField::FaceDisabled, "Face disabled", "Face",
              UiDesignerFillColor(base.palette.face[ST_DISABLED]));
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
        item.default_value = base.metrics.shadow.mode == SHADOW_HARD ? "Hard" : "Curve";
        item.impact = PropertyImpactPaint | PropertyImpactCode;
        item.adapter_field_id = UiDesignerButtonStyleFieldName(UiDesignerButtonStyleField::ShadowMode);
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

static String ButtonStyleExpr(bool tool_button, const String& role)
{
    const String role_expr = role == "Subtle" ? "UiRole::Subtle"
                           : role == "Accent" ? "UiRole::Accent"
                           : role == "Alert" ? "UiRole::Alert"
                                             : "UiRole::Standard";
    return tool_button
        ? "UiTheme::ResolveToolButton(" + role_expr + ")"
        : "UiTheme::ResolveButton(" + role_expr + ")";
}

static void EmitButtonStyleField(String& out, const String& style_var,
                                 UiDesignerButtonStyleField field,
                                 const Value& value)
{
    switch(field) {
    case UiDesignerButtonStyleField::FontFace:
        out << "\t" << style_var << ".font.FaceName(" << EmitValue(value) << ");\n";
        break;
    case UiDesignerButtonStyleField::FontSize:
        out << "\t" << style_var << ".font.Height(" << max(1, (int)value) << ");\n";
        break;
    case UiDesignerButtonStyleField::FontBold:
        out << "\t" << style_var << ".font.Bold(" << AsString((bool)value) << ");\n";
        break;
    case UiDesignerButtonStyleField::FontItalic:
        out << "\t" << style_var << ".font.Italic(" << AsString((bool)value) << ");\n";
        break;
    case UiDesignerButtonStyleField::FaceEnabled:
        out << "\t" << style_var << ".metrics.face_enabled = " << AsString((bool)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::FaceNormal:
        out << "\t" << style_var << ".palette.face[ST_NORMAL] = UiFill::Solid(" << EmitValue(value) << ");\n";
        break;
    case UiDesignerButtonStyleField::FaceHot:
        out << "\t" << style_var << ".palette.face[ST_HOT] = UiFill::Solid(" << EmitValue(value) << ");\n";
        break;
    case UiDesignerButtonStyleField::FacePressed:
        out << "\t" << style_var << ".palette.face[ST_PRESSED] = UiFill::Solid(" << EmitValue(value) << ");\n";
        break;
    case UiDesignerButtonStyleField::FaceDisabled:
        out << "\t" << style_var << ".palette.face[ST_DISABLED] = UiFill::Solid(" << EmitValue(value) << ");\n";
        break;
    case UiDesignerButtonStyleField::Transparent:
        out << "\t" << style_var << ".transparent = " << AsString((bool)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::FrameEnabled:
        out << "\t" << style_var << ".metrics.frame_enabled = " << AsString((bool)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::FrameNormal:
        out << "\t" << style_var << ".palette.frame[ST_NORMAL] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::FrameHot:
        out << "\t" << style_var << ".palette.frame[ST_HOT] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::FramePressed:
        out << "\t" << style_var << ".palette.frame[ST_PRESSED] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::FrameDisabled:
        out << "\t" << style_var << ".palette.frame[ST_DISABLED] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::FrameWidth:
        out << "\t" << style_var << ".metrics.frame_width = " << max(0, (int)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::Radius:
        out << "\t" << style_var << ".metrics.radius = " << max(0, (int)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::FrameDashed:
        out << "\t" << style_var << ".metrics.dashed = " << AsString((bool)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::FrameDashPattern:
        out << "\t" << style_var << ".metrics.dash_pattern = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::TextNormal:
        out << "\t" << style_var << ".palette.ink[ST_NORMAL] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::TextHot:
        out << "\t" << style_var << ".palette.ink[ST_HOT] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::TextPressed:
        out << "\t" << style_var << ".palette.ink[ST_PRESSED] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::TextDisabled:
        out << "\t" << style_var << ".palette.ink[ST_DISABLED] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::IconNormal:
        out << "\t" << style_var << ".palette.icon[ST_NORMAL] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::IconHot:
        out << "\t" << style_var << ".palette.icon[ST_HOT] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::IconPressed:
        out << "\t" << style_var << ".palette.icon[ST_PRESSED] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::IconDisabled:
        out << "\t" << style_var << ".palette.icon[ST_DISABLED] = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::ShadowEnabled:
        out << "\t" << style_var << ".metrics.shadow.enabled = " << AsString((bool)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::ShadowDistance:
        out << "\t" << style_var << ".metrics.shadow.distance = " << max(0, (int)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::ShadowOffsetX:
        out << "\t" << style_var << ".metrics.shadow.offset_x = " << (int)value << ";\n";
        break;
    case UiDesignerButtonStyleField::ShadowOffsetY:
        out << "\t" << style_var << ".metrics.shadow.offset_y = " << (int)value << ";\n";
        break;
    case UiDesignerButtonStyleField::ShadowAlpha:
        out << "\t" << style_var << ".metrics.shadow.alpha = " << minmax((int)value, 0, 255) << ";\n";
        break;
    case UiDesignerButtonStyleField::ShadowColor:
        out << "\t" << style_var << ".metrics.shadow.color = " << EmitValue(value) << ";\n";
        break;
    case UiDesignerButtonStyleField::ShadowInset:
        out << "\t" << style_var << ".metrics.shadow.inset = " << AsString((bool)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::ShadowMode:
        out << "\t" << style_var << ".metrics.shadow.mode = "
            << (AsString(value) == "Hard" ? "SHADOW_HARD" : "SHADOW_CURVE") << ";\n";
        break;
    case UiDesignerButtonStyleField::PressOffsetX:
        out << "\t" << style_var << ".press_offset.x = " << (int)value << ";\n";
        break;
    case UiDesignerButtonStyleField::PressOffsetY:
        out << "\t" << style_var << ".press_offset.y = " << (int)value << ";\n";
        break;
    case UiDesignerButtonStyleField::Overpaint:
        out << "\t" << style_var << ".overpaint = " << max(0, (int)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::UnderlineEnabled:
        out << "\t" << style_var << ".underline = " << AsString((bool)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::UnderlineWidth:
        out << "\t" << style_var << ".underline_width = " << max(0, (int)value) << ";\n";
        break;
    case UiDesignerButtonStyleField::UnderlineOffset:
        out << "\t" << style_var << ".underline_offset = " << (int)value << ";\n";
        break;
    default:
        break;
    }
}

static bool ButtonStyleHasAuthoredOverride(const UiDesignerNode& node,
                                           const UiDesignerControlSpec& spec)
{
    for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides)
        if(node.theme_overrides.Find(property.id) >= 0)
            return true;
    return false;
}

class ButtonThemeAdapter final : public UiDesignerThemeAdapter {
public:
    ButtonThemeAdapter(const char *id, bool tool_button)
        : id_(id), tool_button_(tool_button) {}

    const char *Id() const override { return id_; }

    bool Supports(UiDesignerRuntimeKind kind) const override
    {
        return tool_button_ ? kind == UiDesignerRuntimeKind::UiToolButton
                            : kind == UiDesignerRuntimeKind::UiButton ||
                              kind == UiDesignerRuntimeKind::UiSplitButton;
    }

    void AddThemeOverrides(UiDesignerControlSpec& spec) const override
    {
        AddButtonThemeOverrides(spec, tool_button_);
    }

    bool HasField(const String& field_id) const override
    {
        UiDesignerButtonStyleField field;
        return UiDesignerParseButtonStyleField(field_id, field);
    }

    bool FieldAffectsLayout(const String& field_id) const override
    {
        UiDesignerButtonStyleField field;
        if(!UiDesignerParseButtonStyleField(field_id, field))
            return false;
        return UiDesignerButtonStyleFieldAffectsLayout(field);
    }

    Value ResolveFieldValue(const UiDesignerNode& node,
                            const UiDesignerControlSpec& spec,
                            const String& field_id,
                            const UiDesignerTransientOverlay* overlay) const override
    {
        UiDesignerButtonStyleField field;
        if(!UiDesignerParseButtonStyleField(field_id, field))
            return Value();
        UiButton::Style style = ResolveButtonStyleBase(tool_button_, node);
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            UiDesignerButtonStyleField mapped;
            if(!UiDesignerParseButtonStyleField(property.adapter_field_id, mapped))
                continue;
            const int q = node.theme_overrides.Find(property.id);
            const bool active = q >= 0 || HasThemeValue(node, overlay, property.id);
            if(!active)
                continue;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q)
                                           : property.default_value;
            const Value effective = ResolveThemeValue(node, overlay, property.id, canonical);
            UiDesignerApplyButtonStyleField(style, mapped, effective);
        }
        return UiDesignerButtonStyleFieldValue(style, field);
    }

    void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node,
                           const UiDesignerControlSpec& spec,
                           const UiDesignerTransientOverlay* overlay) const override
    {
        UiButton *button = dynamic_cast<UiButton *>(&ctrl);
        if(!button)
            return;

        UiButton::Style style = ResolveButtonStyleBase(tool_button_, node);
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            UiDesignerButtonStyleField mapped;
            if(!UiDesignerParseButtonStyleField(property.adapter_field_id, mapped))
                continue;
            const int q = node.theme_overrides.Find(property.id);
            const bool active = q >= 0 || HasThemeValue(node, overlay, property.id);
            if(!active)
                continue;
            authored = true;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q)
                                           : property.default_value;
            const Value effective = ResolveThemeValue(node, overlay, property.id, canonical);
            UiDesignerApplyButtonStyleField(style, mapped, effective);
        }
        const UiRole role = ParseRole(node.GetProperty("role", "Standard"));
        if(authored)
            button->SetCustomStyle(style);
        else if(role != UiRole::Standard)
            button->SetCustomStyle(style);
        else
            button->ClearCustomStyle();
    }

    void EmitSetup(String& out, const String& member,
                   const UiDesignerNode& node,
                   const UiDesignerControlSpec& spec) const override
    {
        const String role = AsString(node.GetProperty("role", "Standard"));
        const bool authored = ButtonStyleHasAuthoredOverride(node, spec);
        if(!authored && role == "Standard")
            return;

        if(!authored) {
            out << "\t" << member << ".SetCustomStyle("
                << ButtonStyleExpr(tool_button_, role) << ");\n";
            return;
        }

        const String style_var = member + "_style";
        out << "\tUiButton::Style " << style_var << " = "
            << ButtonStyleExpr(tool_button_, role) << ";\n";
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            UiDesignerButtonStyleField field;
            if(!UiDesignerParseButtonStyleField(property.adapter_field_id, field))
                continue;
            const int q = node.theme_overrides.Find(property.id);
            if(q < 0)
                continue;
            EmitButtonStyleField(out, style_var, field,
                node.theme_overrides.GetValue(q));
        }
        out << "\t" << member << ".SetCustomStyle(" << style_var << ");\n";
    }

private:
    const char *id_;
    bool tool_button_;
};

static bool FieldMatches(const String& id, const char *const *fields, int count)
{
    for(int i = 0; i < count; i++)
        if(id == fields[i])
            return true;
    return false;
}

static UiTreeGlyphStyle ParseTreeGlyphStyle(const Value& value)
{
    const String text = value;
    if(text == "ThickChevron") return UITREEGLYPH_THICK_CHEVRON;
    if(text == "PlusMinus") return UITREEGLYPH_PLUSMINUS;
    if(text == "Custom") return UITREEGLYPH_CUSTOM;
    return UITREEGLYPH_CHEVRON;
}

static String TreeGlyphStyleName(UiTreeGlyphStyle style)
{
    switch(style) {
    case UITREEGLYPH_THICK_CHEVRON: return "ThickChevron";
    case UITREEGLYPH_PLUSMINUS: return "PlusMinus";
    case UITREEGLYPH_CUSTOM: return "Custom";
    default: return "Chevron";
    }
}

static UiIconRenderMode ParseIconRenderMode(const Value& value)
{
    const String mode = value;
    if(mode == "Auto") return UiIconRenderMode::Auto;
    if(mode == "PreserveColor") return UiIconRenderMode::PreserveColor;
    return UiIconRenderMode::MonoTint;
}

static String IconRenderModeName(UiIconRenderMode mode)
{
    if(mode == UiIconRenderMode::Auto) return "Auto";
    if(mode == UiIconRenderMode::PreserveColor) return "PreserveColor";
    return "MonoTint";
}

class TreeThemeAdapter final : public UiDesignerThemeAdapter {
public:
    const char *Id() const override { return "tree"; }
    bool Supports(UiDesignerRuntimeKind kind) const override
    {
        return kind == UiDesignerRuntimeKind::UiTree;
    }

    void AddThemeOverrides(UiDesignerControlSpec& spec) const override
    {
        const UiTree::Style base = UiTheme::ResolveTree();
        AddOverride(spec, "row_height", "Row height", "Layout", PropertyEditorKind::Integer,
                    base.row_height, PropertyImpactPaint | PropertyImpactCode,
                    "row_height");
        AddOverride(spec, "indent_px", "Indent", "Layout", PropertyEditorKind::Integer,
                    base.indent_px, PropertyImpactPaint | PropertyImpactCode,
                    "indent_px");
        AddOverride(spec, "glyph_size", "Glyph size", "Layout", PropertyEditorKind::Integer,
                    base.glyph_size, PropertyImpactPaint | PropertyImpactCode,
                    "glyph_size");
        AddOverride(spec, "icon_size", "Icon size", "Layout", PropertyEditorKind::Integer,
                    base.icon_size, PropertyImpactPaint | PropertyImpactCode,
                    "icon_size");
        AddOverride(spec, "content_gap", "Content gap", "Layout", PropertyEditorKind::Integer,
                    base.content_gap, PropertyImpactPaint | PropertyImpactCode,
                    "content_gap");
        AddOverride(spec, "item_spacing", "Item spacing", "Layout", PropertyEditorKind::Integer,
                    base.item_spacing, PropertyImpactPaint | PropertyImpactCode,
                    "item_spacing");
        AddOverride(spec, "h_padding", "Horizontal padding", "Layout", PropertyEditorKind::Integer,
                    base.h_padding, PropertyImpactPaint | PropertyImpactCode,
                    "h_padding");
        AddOverride(spec, "v_padding", "Vertical padding", "Layout", PropertyEditorKind::Integer,
                    base.v_padding, PropertyImpactPaint | PropertyImpactCode,
                    "v_padding");
        AddOverride(spec, "row_radius", "Row radius", "Layout", PropertyEditorKind::Integer,
                    base.row_radius, PropertyImpactPaint | PropertyImpactCode,
                    "row_radius");
        AddOverride(spec, "branch_hit_extra", "Branch hit extra", "Layout", PropertyEditorKind::Integer,
                    base.branch_hit_extra, PropertyImpactPaint | PropertyImpactCode,
                    "branch_hit_extra");
        AddOverride(spec, "metadata_size", "Metadata size", "Layout", PropertyEditorKind::Integer,
                    base.metadata_size, PropertyImpactPaint | PropertyImpactCode,
                    "metadata_size");
        AddOverride(spec, "metadata_gap", "Metadata gap", "Layout", PropertyEditorKind::Integer,
                    base.metadata_gap, PropertyImpactPaint | PropertyImpactCode,
                    "metadata_gap");
        AddOverride(spec, "accessory_gap", "Accessory gap", "Layout", PropertyEditorKind::Integer,
                    base.accessory_gap, PropertyImpactPaint | PropertyImpactCode,
                    "accessory_gap");
        AddOverride(spec, "show_icons", "Show icons", "Visibility", PropertyEditorKind::Boolean,
                    base.show_icons, PropertyImpactPaint | PropertyImpactCode,
                    "show_icons");
        AddOverride(spec, "show_connector_lines", "Show connector lines", "Visibility",
                    PropertyEditorKind::Boolean, base.show_connector_lines,
                    PropertyImpactPaint | PropertyImpactCode, "show_connector_lines");
        AddOverride(spec, "show_metadata_marker", "Show metadata marker", "Visibility",
                    PropertyEditorKind::Boolean, base.show_metadata_marker,
                    PropertyImpactPaint | PropertyImpactCode, "show_metadata_marker");
        AddOverride(spec, "glyph_style", "Glyph style", "Appearance", PropertyEditorKind::Choice,
                    TreeGlyphStyleName(base.glyph_style), PropertyImpactPaint | PropertyImpactCode,
                    "glyph_style");
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Chevron", "Chevron"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("ThickChevron", "Thick Chevron"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("PlusMinus", "Plus / Minus"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Custom", "Custom"));
        AddOverride(spec, "icon_render_mode", "Icon render mode", "Appearance",
                    PropertyEditorKind::Choice, IconRenderModeName(base.icon_render_mode),
                    PropertyImpactPaint | PropertyImpactCode, "icon_render_mode");
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Auto", "Auto"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("MonoTint", "Mono tint"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("PreserveColor", "Preserve color"));
        AddOverride(spec, "ink", "Ink", "Ink", PropertyEditorKind::Color,
                    base.ink, PropertyImpactPaint | PropertyImpactCode, "ink");
        AddOverride(spec, "disabled_ink", "Disabled ink", "Ink", PropertyEditorKind::Color,
                    base.disabled_ink, PropertyImpactPaint | PropertyImpactCode, "disabled_ink");
        AddOverride(spec, "hot_face", "Hot face", "Face", PropertyEditorKind::Color,
                    base.hot_face, PropertyImpactPaint | PropertyImpactCode, "hot_face");
        AddOverride(spec, "hot_frame", "Hot frame", "Face", PropertyEditorKind::Color,
                    base.hot_frame, PropertyImpactPaint | PropertyImpactCode, "hot_frame");
        AddOverride(spec, "hot_ink", "Hot ink", "Face", PropertyEditorKind::Color,
                    base.hot_ink, PropertyImpactPaint | PropertyImpactCode, "hot_ink");
        AddOverride(spec, "selected_face", "Selected face", "Face", PropertyEditorKind::Color,
                    base.selected_face, PropertyImpactPaint | PropertyImpactCode, "selected_face");
        AddOverride(spec, "selected_frame", "Selected frame", "Face", PropertyEditorKind::Color,
                    base.selected_frame, PropertyImpactPaint | PropertyImpactCode, "selected_frame");
        AddOverride(spec, "selected_ink", "Selected ink", "Face", PropertyEditorKind::Color,
                    base.selected_ink, PropertyImpactPaint | PropertyImpactCode, "selected_ink");
        AddOverride(spec, "line_color", "Line color", "Glyph", PropertyEditorKind::Color,
                    base.line_color, PropertyImpactPaint | PropertyImpactCode, "line_color");
        AddOverride(spec, "glyph_color", "Glyph color", "Glyph", PropertyEditorKind::Color,
                    base.glyph_color, PropertyImpactPaint | PropertyImpactCode, "glyph_color");
        AddOverride(spec, "glyph_hot_color", "Glyph hot color", "Glyph", PropertyEditorKind::Color,
                    base.glyph_hot_color, PropertyImpactPaint | PropertyImpactCode, "glyph_hot_color");
        AddOverride(spec, "glyph_selected_color", "Glyph selected color", "Glyph", PropertyEditorKind::Color,
                    base.glyph_selected_color, PropertyImpactPaint | PropertyImpactCode, "glyph_selected_color");
    }

    bool HasField(const String& field_id) const override
    {
        static const char *fields[] = {
            "row_height", "indent_px", "glyph_size", "icon_size", "content_gap",
            "item_spacing", "h_padding", "v_padding", "row_radius",
            "branch_hit_extra", "metadata_size", "metadata_gap", "accessory_gap",
            "show_icons", "show_connector_lines", "show_metadata_marker",
            "glyph_style", "icon_render_mode", "ink", "disabled_ink",
            "hot_face", "hot_frame", "hot_ink", "selected_face",
            "selected_frame", "selected_ink", "line_color", "glyph_color",
            "glyph_hot_color", "glyph_selected_color"
        };
        return FieldMatches(field_id, fields, __countof(fields));
    }

    bool FieldAffectsLayout(const String& field_id) const override
    {
        static const char *layout_fields[] = {
            "row_height", "indent_px", "glyph_size", "icon_size", "content_gap",
            "item_spacing", "h_padding", "v_padding", "row_radius",
            "branch_hit_extra", "metadata_size", "metadata_gap", "accessory_gap",
            "show_icons", "show_connector_lines", "show_metadata_marker"
        };
        return FieldMatches(field_id, layout_fields, __countof(layout_fields));
    }

    Value ResolveFieldValue(const UiDesignerNode& node,
                            const UiDesignerControlSpec& spec,
                            const String& field_id,
                            const UiDesignerTransientOverlay* overlay) const override
    {
        UiTree::Style style = UiTheme::ResolveTree();
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(property.id);
            const bool active = q >= 0 || HasThemeValue(node, overlay, property.id);
            if(!active)
                continue;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q)
                                           : property.default_value;
            const Value effective = ResolveThemeValue(node, overlay, property.id, canonical);
            if(property.id == "row_height") style.row_height = (int)effective;
            else if(property.id == "indent_px") style.indent_px = (int)effective;
            else if(property.id == "glyph_size") style.glyph_size = (int)effective;
            else if(property.id == "icon_size") style.icon_size = (int)effective;
            else if(property.id == "content_gap") style.content_gap = (int)effective;
            else if(property.id == "item_spacing") style.item_spacing = (int)effective;
            else if(property.id == "h_padding") style.h_padding = (int)effective;
            else if(property.id == "v_padding") style.v_padding = (int)effective;
            else if(property.id == "row_radius") style.row_radius = (int)effective;
            else if(property.id == "branch_hit_extra") style.branch_hit_extra = (int)effective;
            else if(property.id == "metadata_size") style.metadata_size = (int)effective;
            else if(property.id == "metadata_gap") style.metadata_gap = (int)effective;
            else if(property.id == "accessory_gap") style.accessory_gap = (int)effective;
            else if(property.id == "show_icons") style.show_icons = (bool)effective;
            else if(property.id == "show_connector_lines") style.show_connector_lines = (bool)effective;
            else if(property.id == "show_metadata_marker") style.show_metadata_marker = (bool)effective;
            else if(property.id == "glyph_style") style.glyph_style = ParseTreeGlyphStyle(effective);
            else if(property.id == "icon_render_mode") style.icon_render_mode = ParseIconRenderMode(effective);
            else if(property.id == "ink") style.ink = (Color)effective;
            else if(property.id == "disabled_ink") style.disabled_ink = (Color)effective;
            else if(property.id == "hot_face") style.hot_face = (Color)effective;
            else if(property.id == "hot_frame") style.hot_frame = (Color)effective;
            else if(property.id == "hot_ink") style.hot_ink = (Color)effective;
            else if(property.id == "selected_face") style.selected_face = (Color)effective;
            else if(property.id == "selected_frame") style.selected_frame = (Color)effective;
            else if(property.id == "selected_ink") style.selected_ink = (Color)effective;
            else if(property.id == "line_color") style.line_color = (Color)effective;
            else if(property.id == "glyph_color") style.glyph_color = (Color)effective;
            else if(property.id == "glyph_hot_color") style.glyph_hot_color = (Color)effective;
            else if(property.id == "glyph_selected_color") style.glyph_selected_color = (Color)effective;
        }
        if(field_id == "row_height") return style.row_height;
        if(field_id == "indent_px") return style.indent_px;
        if(field_id == "glyph_size") return style.glyph_size;
        if(field_id == "icon_size") return style.icon_size;
        if(field_id == "content_gap") return style.content_gap;
        if(field_id == "item_spacing") return style.item_spacing;
        if(field_id == "h_padding") return style.h_padding;
        if(field_id == "v_padding") return style.v_padding;
        if(field_id == "row_radius") return style.row_radius;
        if(field_id == "branch_hit_extra") return style.branch_hit_extra;
        if(field_id == "metadata_size") return style.metadata_size;
        if(field_id == "metadata_gap") return style.metadata_gap;
        if(field_id == "accessory_gap") return style.accessory_gap;
        if(field_id == "show_icons") return style.show_icons;
        if(field_id == "show_connector_lines") return style.show_connector_lines;
        if(field_id == "show_metadata_marker") return style.show_metadata_marker;
        if(field_id == "glyph_style") return TreeGlyphStyleName(style.glyph_style);
        if(field_id == "icon_render_mode") return IconRenderModeName(style.icon_render_mode);
        if(field_id == "ink") return style.ink;
        if(field_id == "disabled_ink") return style.disabled_ink;
        if(field_id == "hot_face") return style.hot_face;
        if(field_id == "hot_frame") return style.hot_frame;
        if(field_id == "hot_ink") return style.hot_ink;
        if(field_id == "selected_face") return style.selected_face;
        if(field_id == "selected_frame") return style.selected_frame;
        if(field_id == "selected_ink") return style.selected_ink;
        if(field_id == "line_color") return style.line_color;
        if(field_id == "glyph_color") return style.glyph_color;
        if(field_id == "glyph_hot_color") return style.glyph_hot_color;
        if(field_id == "glyph_selected_color") return style.glyph_selected_color;
        return Value();
    }

    void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node,
                           const UiDesignerControlSpec& spec,
                           const UiDesignerTransientOverlay* overlay) const override
    {
        UiTree *tree = dynamic_cast<UiTree *>(&ctrl);
        if(!tree)
            return;
        UiTree::Style style = UiTheme::ResolveTree();
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(property.id);
            const bool active = q >= 0 || HasThemeValue(node, overlay, property.id);
            if(!active)
                continue;
            authored = true;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q)
                                           : property.default_value;
            const Value effective = ResolveThemeValue(node, overlay, property.id, canonical);
            if(property.id == "row_height") style.row_height = (int)effective;
            else if(property.id == "indent_px") style.indent_px = (int)effective;
            else if(property.id == "glyph_size") style.glyph_size = (int)effective;
            else if(property.id == "icon_size") style.icon_size = (int)effective;
            else if(property.id == "content_gap") style.content_gap = (int)effective;
            else if(property.id == "item_spacing") style.item_spacing = (int)effective;
            else if(property.id == "h_padding") style.h_padding = (int)effective;
            else if(property.id == "v_padding") style.v_padding = (int)effective;
            else if(property.id == "row_radius") style.row_radius = (int)effective;
            else if(property.id == "branch_hit_extra") style.branch_hit_extra = (int)effective;
            else if(property.id == "metadata_size") style.metadata_size = (int)effective;
            else if(property.id == "metadata_gap") style.metadata_gap = (int)effective;
            else if(property.id == "accessory_gap") style.accessory_gap = (int)effective;
            else if(property.id == "show_icons") style.show_icons = (bool)effective;
            else if(property.id == "show_connector_lines") style.show_connector_lines = (bool)effective;
            else if(property.id == "show_metadata_marker") style.show_metadata_marker = (bool)effective;
            else if(property.id == "glyph_style") style.glyph_style = ParseTreeGlyphStyle(effective);
            else if(property.id == "icon_render_mode") style.icon_render_mode = ParseIconRenderMode(effective);
            else if(property.id == "ink") style.ink = (Color)effective;
            else if(property.id == "disabled_ink") style.disabled_ink = (Color)effective;
            else if(property.id == "hot_face") style.hot_face = (Color)effective;
            else if(property.id == "hot_frame") style.hot_frame = (Color)effective;
            else if(property.id == "hot_ink") style.hot_ink = (Color)effective;
            else if(property.id == "selected_face") style.selected_face = (Color)effective;
            else if(property.id == "selected_frame") style.selected_frame = (Color)effective;
            else if(property.id == "selected_ink") style.selected_ink = (Color)effective;
            else if(property.id == "line_color") style.line_color = (Color)effective;
            else if(property.id == "glyph_color") style.glyph_color = (Color)effective;
            else if(property.id == "glyph_hot_color") style.glyph_hot_color = (Color)effective;
            else if(property.id == "glyph_selected_color") style.glyph_selected_color = (Color)effective;
        }
        if(authored)
            tree->SetCustomStyle(style);
        else
            tree->ClearCustomStyle();
    }

    void EmitSetup(String& out, const String& member,
                   const UiDesignerNode& node,
                   const UiDesignerControlSpec& spec) const override
    {
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides)
            authored |= node.theme_overrides.Find(property.id) >= 0;
        if(!authored)
            return;

        const String style_var = member + "_style";
        out << "\tUiTree::Style " << style_var << " = UiTheme::ResolveTree();\n";
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(property.id);
            if(q < 0)
                continue;
            const Value value = node.theme_overrides.GetValue(q);
            if(property.id == "row_height") out << "\t" << style_var << ".row_height = " << (int)value << ";\n";
            else if(property.id == "indent_px") out << "\t" << style_var << ".indent_px = " << (int)value << ";\n";
            else if(property.id == "glyph_size") out << "\t" << style_var << ".glyph_size = " << (int)value << ";\n";
            else if(property.id == "icon_size") out << "\t" << style_var << ".icon_size = " << (int)value << ";\n";
            else if(property.id == "content_gap") out << "\t" << style_var << ".content_gap = " << (int)value << ";\n";
            else if(property.id == "item_spacing") out << "\t" << style_var << ".item_spacing = " << (int)value << ";\n";
            else if(property.id == "h_padding") out << "\t" << style_var << ".h_padding = " << (int)value << ";\n";
            else if(property.id == "v_padding") out << "\t" << style_var << ".v_padding = " << (int)value << ";\n";
            else if(property.id == "row_radius") out << "\t" << style_var << ".row_radius = " << (int)value << ";\n";
            else if(property.id == "branch_hit_extra") out << "\t" << style_var << ".branch_hit_extra = " << (int)value << ";\n";
            else if(property.id == "metadata_size") out << "\t" << style_var << ".metadata_size = " << (int)value << ";\n";
            else if(property.id == "metadata_gap") out << "\t" << style_var << ".metadata_gap = " << (int)value << ";\n";
            else if(property.id == "accessory_gap") out << "\t" << style_var << ".accessory_gap = " << (int)value << ";\n";
            else if(property.id == "show_icons") out << "\t" << style_var << ".show_icons = " << AsString((bool)value) << ";\n";
            else if(property.id == "show_connector_lines") out << "\t" << style_var << ".show_connector_lines = " << AsString((bool)value) << ";\n";
            else if(property.id == "show_metadata_marker") out << "\t" << style_var << ".show_metadata_marker = " << AsString((bool)value) << ";\n";
            else if(property.id == "glyph_style") out << "\t" << style_var << ".glyph_style = " << (value == "ThickChevron" ? "UITREEGLYPH_THICK_CHEVRON" : value == "PlusMinus" ? "UITREEGLYPH_PLUSMINUS" : value == "Custom" ? "UITREEGLYPH_CUSTOM" : "UITREEGLYPH_CHEVRON") << ";\n";
            else if(property.id == "icon_render_mode") out << "\t" << style_var << ".icon_render_mode = " << (value == "Auto" ? "UiIconRenderMode::Auto" : value == "PreserveColor" ? "UiIconRenderMode::PreserveColor" : "UiIconRenderMode::MonoTint") << ";\n";
            else if(property.id == "ink") out << "\t" << style_var << ".ink = " << EmitValue(value) << ";\n";
            else if(property.id == "disabled_ink") out << "\t" << style_var << ".disabled_ink = " << EmitValue(value) << ";\n";
            else if(property.id == "hot_face") out << "\t" << style_var << ".hot_face = " << EmitValue(value) << ";\n";
            else if(property.id == "hot_frame") out << "\t" << style_var << ".hot_frame = " << EmitValue(value) << ";\n";
            else if(property.id == "hot_ink") out << "\t" << style_var << ".hot_ink = " << EmitValue(value) << ";\n";
            else if(property.id == "selected_face") out << "\t" << style_var << ".selected_face = " << EmitValue(value) << ";\n";
            else if(property.id == "selected_frame") out << "\t" << style_var << ".selected_frame = " << EmitValue(value) << ";\n";
            else if(property.id == "selected_ink") out << "\t" << style_var << ".selected_ink = " << EmitValue(value) << ";\n";
            else if(property.id == "line_color") out << "\t" << style_var << ".line_color = " << EmitValue(value) << ";\n";
            else if(property.id == "glyph_color") out << "\t" << style_var << ".glyph_color = " << EmitValue(value) << ";\n";
            else if(property.id == "glyph_hot_color") out << "\t" << style_var << ".glyph_hot_color = " << EmitValue(value) << ";\n";
            else if(property.id == "glyph_selected_color") out << "\t" << style_var << ".glyph_selected_color = " << EmitValue(value) << ";\n";
        }
        out << "\t" << member << ".SetCustomStyle(" << style_var << ");\n";
    }
};

class ListThemeAdapter final : public UiDesignerThemeAdapter {
public:
    const char *Id() const override { return "list"; }
    bool Supports(UiDesignerRuntimeKind kind) const override
    {
        return kind == UiDesignerRuntimeKind::UiList;
    }

    void AddThemeOverrides(UiDesignerControlSpec& spec) const override
    {
        const UiList::Style base = UiTheme::ResolveList();
        AddOverride(spec, "row_height", "Row height", "Layout", PropertyEditorKind::Integer,
                    base.row_height, PropertyImpactPaint | PropertyImpactCode, "row_height");
        AddOverride(spec, "item_spacing", "Item spacing", "Layout", PropertyEditorKind::Integer,
                    base.item_spacing, PropertyImpactPaint | PropertyImpactCode, "item_spacing");
        AddOverride(spec, "icon_size", "Icon size", "Layout", PropertyEditorKind::Integer,
                    base.icon_size, PropertyImpactPaint | PropertyImpactCode, "icon_size");
        AddOverride(spec, "check_size", "Check size", "Layout", PropertyEditorKind::Integer,
                    base.check_size, PropertyImpactPaint | PropertyImpactCode, "check_size");
        AddOverride(spec, "content_gap", "Content gap", "Layout", PropertyEditorKind::Integer,
                    base.content_gap, PropertyImpactPaint | PropertyImpactCode, "content_gap");
        AddOverride(spec, "h_padding", "Horizontal padding", "Layout", PropertyEditorKind::Integer,
                    base.h_padding, PropertyImpactPaint | PropertyImpactCode, "h_padding");
        AddOverride(spec, "v_padding", "Vertical padding", "Layout", PropertyEditorKind::Integer,
                    base.v_padding, PropertyImpactPaint | PropertyImpactCode, "v_padding");
        AddOverride(spec, "row_radius", "Row radius", "Layout", PropertyEditorKind::Integer,
                    base.row_radius, PropertyImpactPaint | PropertyImpactCode, "row_radius");
        AddOverride(spec, "metadata_size", "Metadata size", "Layout", PropertyEditorKind::Integer,
                    base.metadata_size, PropertyImpactPaint | PropertyImpactCode, "metadata_size");
        AddOverride(spec, "metadata_gap", "Metadata gap", "Layout", PropertyEditorKind::Integer,
                    base.metadata_gap, PropertyImpactPaint | PropertyImpactCode, "metadata_gap");
        AddOverride(spec, "right_gap", "Right gap", "Layout", PropertyEditorKind::Integer,
                    base.right_gap, PropertyImpactPaint | PropertyImpactCode, "right_gap");
        AddOverride(spec, "drag_size", "Drag size", "Layout", PropertyEditorKind::Integer,
                    base.drag_size, PropertyImpactPaint | PropertyImpactCode, "drag_size");
        AddOverride(spec, "drag_gap", "Drag gap", "Layout", PropertyEditorKind::Integer,
                    base.drag_gap, PropertyImpactPaint | PropertyImpactCode, "drag_gap");
        AddOverride(spec, "show_icons", "Show icons", "Visibility", PropertyEditorKind::Boolean,
                    base.show_icons, PropertyImpactPaint | PropertyImpactCode, "show_icons");
        AddOverride(spec, "show_checks", "Show checks", "Visibility", PropertyEditorKind::Boolean,
                    base.show_checks, PropertyImpactPaint | PropertyImpactCode, "show_checks");
        AddOverride(spec, "show_metadata_marker", "Show metadata marker", "Visibility",
                    PropertyEditorKind::Boolean, base.show_metadata_marker,
                    PropertyImpactPaint | PropertyImpactCode, "show_metadata_marker");
        AddOverride(spec, "show_drag_handle", "Show drag handle", "Visibility",
                    PropertyEditorKind::Boolean, base.show_drag_handle,
                    PropertyImpactPaint | PropertyImpactCode, "show_drag_handle");
        AddOverride(spec, "drag_side", "Drag side", "Visibility", PropertyEditorKind::Choice,
                    base.drag_side == UiAlign::RIGHT ? "Right" : "Left",
                    PropertyImpactPaint | PropertyImpactCode, "drag_side");
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Left", "Left"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Right", "Right"));
        AddOverride(spec, "hot_as_underline", "Hot as underline", "State",
                    PropertyEditorKind::Boolean, base.hot_as_underline,
                    PropertyImpactPaint | PropertyImpactCode, "hot_as_underline");
        AddOverride(spec, "selected_as_underline", "Selected as underline", "State",
                    PropertyEditorKind::Boolean, base.selected_as_underline,
                    PropertyImpactPaint | PropertyImpactCode, "selected_as_underline");
        AddOverride(spec, "state_underline_thickness", "Underline thickness", "State",
                    PropertyEditorKind::Integer, base.state_underline_thickness,
                    PropertyImpactPaint | PropertyImpactCode, "state_underline_thickness");
        AddOverride(spec, "striped_rows", "Striped rows", "State", PropertyEditorKind::Boolean,
                    base.striped_rows, PropertyImpactPaint | PropertyImpactCode,
                    "striped_rows");
        AddOverride(spec, "ink", "Ink", "Ink", PropertyEditorKind::Color,
                    base.ink, PropertyImpactPaint | PropertyImpactCode, "ink");
        AddOverride(spec, "disabled_ink", "Disabled ink", "Ink", PropertyEditorKind::Color,
                    base.disabled_ink, PropertyImpactPaint | PropertyImpactCode, "disabled_ink");
        AddOverride(spec, "muted_ink", "Muted ink", "Ink", PropertyEditorKind::Color,
                    base.muted_ink, PropertyImpactPaint | PropertyImpactCode, "muted_ink");
        AddOverride(spec, "hot_face", "Hot face", "Face", PropertyEditorKind::Color,
                    base.hot_face, PropertyImpactPaint | PropertyImpactCode, "hot_face");
        AddOverride(spec, "hot_frame", "Hot frame", "Face", PropertyEditorKind::Color,
                    base.hot_frame, PropertyImpactPaint | PropertyImpactCode, "hot_frame");
        AddOverride(spec, "hot_ink", "Hot ink", "Face", PropertyEditorKind::Color,
                    base.hot_ink, PropertyImpactPaint | PropertyImpactCode, "hot_ink");
        AddOverride(spec, "selected_face", "Selected face", "Face", PropertyEditorKind::Color,
                    base.selected_face, PropertyImpactPaint | PropertyImpactCode, "selected_face");
        AddOverride(spec, "selected_frame", "Selected frame", "Face", PropertyEditorKind::Color,
                    base.selected_frame, PropertyImpactPaint | PropertyImpactCode, "selected_frame");
        AddOverride(spec, "selected_ink", "Selected ink", "Face", PropertyEditorKind::Color,
                    base.selected_ink, PropertyImpactPaint | PropertyImpactCode, "selected_ink");
        AddOverride(spec, "separator_color", "Separator color", "Face", PropertyEditorKind::Color,
                    base.separator_color, PropertyImpactPaint | PropertyImpactCode, "separator_color");
        AddOverride(spec, "row_even_face", "Row even face", "Face", PropertyEditorKind::Color,
                    base.row_even_face, PropertyImpactPaint | PropertyImpactCode,
                    "row_even_face");
        AddOverride(spec, "row_odd_face", "Row odd face", "Face", PropertyEditorKind::Color,
                    base.row_odd_face, PropertyImpactPaint | PropertyImpactCode,
                    "row_odd_face");
        AddOverride(spec, "show_row_separator", "Show row separator", "Visibility",
                    PropertyEditorKind::Boolean, base.show_row_separator,
                    PropertyImpactPaint | PropertyImpactCode, "show_row_separator");
        AddOverride(spec, "row_state_frame_enabled", "Row state frame enabled", "Face",
                    PropertyEditorKind::Boolean, base.row_state_frame_enabled,
                    PropertyImpactPaint | PropertyImpactCode, "row_state_frame_enabled");
        AddOverride(spec, "right_text_as_badge", "Right text as badge", "Content",
                    PropertyEditorKind::Boolean, base.right_text_as_badge,
                    PropertyImpactPaint | PropertyImpactCode, "right_text_as_badge");
        AddOverride(spec, "badge_face", "Badge face", "Badge", PropertyEditorKind::Color,
                    base.badge_face, PropertyImpactPaint | PropertyImpactCode, "badge_face");
        AddOverride(spec, "badge_frame", "Badge frame", "Badge", PropertyEditorKind::Color,
                    base.badge_frame, PropertyImpactPaint | PropertyImpactCode,
                    "badge_frame");
        AddOverride(spec, "badge_ink", "Badge ink", "Badge", PropertyEditorKind::Color,
                    base.badge_ink, PropertyImpactPaint | PropertyImpactCode, "badge_ink");
        AddOverride(spec, "badge_radius", "Badge radius", "Badge", PropertyEditorKind::Integer,
                    base.badge_radius, PropertyImpactPaint | PropertyImpactCode, "badge_radius");
        AddOverride(spec, "badge_h_padding", "Badge horizontal padding", "Badge",
                    PropertyEditorKind::Integer, base.badge_h_padding,
                    PropertyImpactPaint | PropertyImpactCode, "badge_h_padding");
        AddOverride(spec, "metadata_default", "Metadata default", "Badge",
                    PropertyEditorKind::Color, base.metadata_default,
                    PropertyImpactPaint | PropertyImpactCode, "metadata_default");
        AddOverride(spec, "check_frame", "Check frame", "Check", PropertyEditorKind::Color,
                    base.check_frame, PropertyImpactPaint | PropertyImpactCode, "check_frame");
        AddOverride(spec, "check_fill", "Check fill", "Check", PropertyEditorKind::Color,
                    base.check_fill, PropertyImpactPaint | PropertyImpactCode, "check_fill");
        AddOverride(spec, "drag_marker", "Drag marker", "Drag", PropertyEditorKind::Color,
                    base.drag_marker, PropertyImpactPaint | PropertyImpactCode, "drag_marker");
    }

    bool HasField(const String& field_id) const override
    {
        static const char *fields[] = {
            "row_height", "item_spacing", "icon_size", "check_size",
            "content_gap", "h_padding", "v_padding", "row_radius",
            "metadata_size", "metadata_gap", "right_gap", "drag_size",
            "drag_gap", "show_icons", "show_checks", "show_metadata_marker",
            "show_drag_handle", "drag_side", "hot_as_underline",
            "selected_as_underline", "state_underline_thickness", "striped_rows",
            "ink", "disabled_ink", "muted_ink", "hot_face", "hot_frame",
            "hot_ink", "selected_face", "selected_frame", "selected_ink",
            "separator_color", "row_even_face", "row_odd_face",
            "show_row_separator", "row_state_frame_enabled",
            "right_text_as_badge", "badge_face", "badge_frame", "badge_ink",
            "badge_radius", "badge_h_padding", "metadata_default",
            "check_frame", "check_fill", "drag_marker"
        };
        return FieldMatches(field_id, fields, __countof(fields));
    }

    bool FieldAffectsLayout(const String& field_id) const override
    {
        static const char *layout_fields[] = {
            "row_height", "item_spacing", "icon_size", "check_size",
            "content_gap", "h_padding", "v_padding", "row_radius",
            "metadata_size", "metadata_gap", "right_gap", "drag_size",
            "drag_gap", "show_icons", "show_checks", "show_metadata_marker",
            "show_drag_handle", "drag_side", "hot_as_underline",
            "selected_as_underline", "state_underline_thickness", "striped_rows"
        };
        return FieldMatches(field_id, layout_fields, __countof(layout_fields));
    }

    Value ResolveFieldValue(const UiDesignerNode& node,
                            const UiDesignerControlSpec& spec,
                            const String& field_id,
                            const UiDesignerTransientOverlay* overlay) const override
    {
        UiList::Style style = UiTheme::ResolveList();
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(property.id);
            const bool active = q >= 0 || HasThemeValue(node, overlay, property.id);
            if(!active)
                continue;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q)
                                           : property.default_value;
            const Value effective = ResolveThemeValue(node, overlay, property.id, canonical);
            if(property.id == "row_height") style.row_height = (int)effective;
            else if(property.id == "item_spacing") style.item_spacing = (int)effective;
            else if(property.id == "icon_size") style.icon_size = (int)effective;
            else if(property.id == "check_size") style.check_size = (int)effective;
            else if(property.id == "content_gap") style.content_gap = (int)effective;
            else if(property.id == "h_padding") style.h_padding = (int)effective;
            else if(property.id == "v_padding") style.v_padding = (int)effective;
            else if(property.id == "row_radius") style.row_radius = (int)effective;
            else if(property.id == "metadata_size") style.metadata_size = (int)effective;
            else if(property.id == "metadata_gap") style.metadata_gap = (int)effective;
            else if(property.id == "right_gap") style.right_gap = (int)effective;
            else if(property.id == "drag_size") style.drag_size = (int)effective;
            else if(property.id == "drag_gap") style.drag_gap = (int)effective;
            else if(property.id == "show_icons") style.show_icons = (bool)effective;
            else if(property.id == "show_checks") style.show_checks = (bool)effective;
            else if(property.id == "show_metadata_marker") style.show_metadata_marker = (bool)effective;
            else if(property.id == "show_drag_handle") style.show_drag_handle = (bool)effective;
            else if(property.id == "drag_side") style.drag_side = effective == "Right" ? UiAlign::RIGHT : UiAlign::LEFT;
            else if(property.id == "hot_as_underline") style.hot_as_underline = (bool)effective;
            else if(property.id == "selected_as_underline") style.selected_as_underline = (bool)effective;
            else if(property.id == "state_underline_thickness") style.state_underline_thickness = (int)effective;
            else if(property.id == "striped_rows") style.striped_rows = (bool)effective;
            else if(property.id == "ink") style.ink = (Color)effective;
            else if(property.id == "disabled_ink") style.disabled_ink = (Color)effective;
            else if(property.id == "muted_ink") style.muted_ink = (Color)effective;
            else if(property.id == "hot_face") style.hot_face = (Color)effective;
            else if(property.id == "hot_frame") style.hot_frame = (Color)effective;
            else if(property.id == "hot_ink") style.hot_ink = (Color)effective;
            else if(property.id == "selected_face") style.selected_face = (Color)effective;
            else if(property.id == "selected_frame") style.selected_frame = (Color)effective;
            else if(property.id == "selected_ink") style.selected_ink = (Color)effective;
            else if(property.id == "separator_color") style.separator_color = (Color)effective;
            else if(property.id == "row_even_face") style.row_even_face = (Color)effective;
            else if(property.id == "row_odd_face") style.row_odd_face = (Color)effective;
            else if(property.id == "show_row_separator") style.show_row_separator = (bool)effective;
            else if(property.id == "row_state_frame_enabled") style.row_state_frame_enabled = (bool)effective;
            else if(property.id == "right_text_as_badge") style.right_text_as_badge = (bool)effective;
            else if(property.id == "badge_face") style.badge_face = (Color)effective;
            else if(property.id == "badge_frame") style.badge_frame = (Color)effective;
            else if(property.id == "badge_ink") style.badge_ink = (Color)effective;
            else if(property.id == "badge_radius") style.badge_radius = (int)effective;
            else if(property.id == "badge_h_padding") style.badge_h_padding = (int)effective;
            else if(property.id == "metadata_default") style.metadata_default = (Color)effective;
            else if(property.id == "check_frame") style.check_frame = (Color)effective;
            else if(property.id == "check_fill") style.check_fill = (Color)effective;
            else if(property.id == "drag_marker") style.drag_marker = (Color)effective;
        }
        if(field_id == "row_height") return style.row_height;
        if(field_id == "item_spacing") return style.item_spacing;
        if(field_id == "icon_size") return style.icon_size;
        if(field_id == "check_size") return style.check_size;
        if(field_id == "content_gap") return style.content_gap;
        if(field_id == "h_padding") return style.h_padding;
        if(field_id == "v_padding") return style.v_padding;
        if(field_id == "row_radius") return style.row_radius;
        if(field_id == "metadata_size") return style.metadata_size;
        if(field_id == "metadata_gap") return style.metadata_gap;
        if(field_id == "right_gap") return style.right_gap;
        if(field_id == "drag_size") return style.drag_size;
        if(field_id == "drag_gap") return style.drag_gap;
        if(field_id == "show_icons") return style.show_icons;
        if(field_id == "show_checks") return style.show_checks;
        if(field_id == "show_metadata_marker") return style.show_metadata_marker;
        if(field_id == "show_drag_handle") return style.show_drag_handle;
        if(field_id == "drag_side") return style.drag_side == UiAlign::RIGHT ? "Right" : "Left";
        if(field_id == "hot_as_underline") return style.hot_as_underline;
        if(field_id == "selected_as_underline") return style.selected_as_underline;
        if(field_id == "state_underline_thickness") return style.state_underline_thickness;
        if(field_id == "striped_rows") return style.striped_rows;
        if(field_id == "ink") return style.ink;
        if(field_id == "disabled_ink") return style.disabled_ink;
        if(field_id == "muted_ink") return style.muted_ink;
        if(field_id == "hot_face") return style.hot_face;
        if(field_id == "hot_frame") return style.hot_frame;
        if(field_id == "hot_ink") return style.hot_ink;
        if(field_id == "selected_face") return style.selected_face;
        if(field_id == "selected_frame") return style.selected_frame;
        if(field_id == "selected_ink") return style.selected_ink;
        if(field_id == "separator_color") return style.separator_color;
        if(field_id == "row_even_face") return style.row_even_face;
        if(field_id == "row_odd_face") return style.row_odd_face;
        if(field_id == "show_row_separator") return style.show_row_separator;
        if(field_id == "row_state_frame_enabled") return style.row_state_frame_enabled;
        if(field_id == "right_text_as_badge") return style.right_text_as_badge;
        if(field_id == "badge_face") return style.badge_face;
        if(field_id == "badge_frame") return style.badge_frame;
        if(field_id == "badge_ink") return style.badge_ink;
        if(field_id == "badge_radius") return style.badge_radius;
        if(field_id == "badge_h_padding") return style.badge_h_padding;
        if(field_id == "metadata_default") return style.metadata_default;
        if(field_id == "check_frame") return style.check_frame;
        if(field_id == "check_fill") return style.check_fill;
        if(field_id == "drag_marker") return style.drag_marker;
        return Value();
    }

    void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node,
                           const UiDesignerControlSpec& spec,
                           const UiDesignerTransientOverlay* overlay) const override
    {
        UiList *list = dynamic_cast<UiList *>(&ctrl);
        if(!list)
            return;
        UiList::Style style = UiTheme::ResolveList();
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(property.id);
            const bool active = q >= 0 || HasThemeValue(node, overlay, property.id);
            if(!active)
                continue;
            authored = true;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q)
                                           : property.default_value;
            const Value effective = ResolveThemeValue(node, overlay, property.id, canonical);
            if(property.id == "row_height") style.row_height = (int)effective;
            else if(property.id == "item_spacing") style.item_spacing = (int)effective;
            else if(property.id == "icon_size") style.icon_size = (int)effective;
            else if(property.id == "check_size") style.check_size = (int)effective;
            else if(property.id == "content_gap") style.content_gap = (int)effective;
            else if(property.id == "h_padding") style.h_padding = (int)effective;
            else if(property.id == "v_padding") style.v_padding = (int)effective;
            else if(property.id == "row_radius") style.row_radius = (int)effective;
            else if(property.id == "metadata_size") style.metadata_size = (int)effective;
            else if(property.id == "metadata_gap") style.metadata_gap = (int)effective;
            else if(property.id == "right_gap") style.right_gap = (int)effective;
            else if(property.id == "drag_size") style.drag_size = (int)effective;
            else if(property.id == "drag_gap") style.drag_gap = (int)effective;
            else if(property.id == "show_icons") style.show_icons = (bool)effective;
            else if(property.id == "show_checks") style.show_checks = (bool)effective;
            else if(property.id == "show_metadata_marker") style.show_metadata_marker = (bool)effective;
            else if(property.id == "show_drag_handle") style.show_drag_handle = (bool)effective;
            else if(property.id == "drag_side") style.drag_side = effective == "Right" ? UiAlign::RIGHT : UiAlign::LEFT;
            else if(property.id == "hot_as_underline") style.hot_as_underline = (bool)effective;
            else if(property.id == "selected_as_underline") style.selected_as_underline = (bool)effective;
            else if(property.id == "state_underline_thickness") style.state_underline_thickness = (int)effective;
            else if(property.id == "striped_rows") style.striped_rows = (bool)effective;
            else if(property.id == "ink") style.ink = (Color)effective;
            else if(property.id == "disabled_ink") style.disabled_ink = (Color)effective;
            else if(property.id == "muted_ink") style.muted_ink = (Color)effective;
            else if(property.id == "hot_face") style.hot_face = (Color)effective;
            else if(property.id == "hot_frame") style.hot_frame = (Color)effective;
            else if(property.id == "hot_ink") style.hot_ink = (Color)effective;
            else if(property.id == "selected_face") style.selected_face = (Color)effective;
            else if(property.id == "selected_frame") style.selected_frame = (Color)effective;
            else if(property.id == "selected_ink") style.selected_ink = (Color)effective;
            else if(property.id == "separator_color") style.separator_color = (Color)effective;
            else if(property.id == "row_even_face") style.row_even_face = (Color)effective;
            else if(property.id == "row_odd_face") style.row_odd_face = (Color)effective;
            else if(property.id == "show_row_separator") style.show_row_separator = (bool)effective;
            else if(property.id == "row_state_frame_enabled") style.row_state_frame_enabled = (bool)effective;
            else if(property.id == "right_text_as_badge") style.right_text_as_badge = (bool)effective;
            else if(property.id == "badge_face") style.badge_face = (Color)effective;
            else if(property.id == "badge_frame") style.badge_frame = (Color)effective;
            else if(property.id == "badge_ink") style.badge_ink = (Color)effective;
            else if(property.id == "badge_radius") style.badge_radius = (int)effective;
            else if(property.id == "badge_h_padding") style.badge_h_padding = (int)effective;
            else if(property.id == "metadata_default") style.metadata_default = (Color)effective;
            else if(property.id == "check_frame") style.check_frame = (Color)effective;
            else if(property.id == "check_fill") style.check_fill = (Color)effective;
            else if(property.id == "drag_marker") style.drag_marker = (Color)effective;
        }
        if(authored)
            list->SetCustomStyle(style);
        else
            list->ClearCustomStyle();
    }

    void EmitSetup(String& out, const String& member,
                   const UiDesignerNode& node,
                   const UiDesignerControlSpec& spec) const override
    {
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides)
            authored |= node.theme_overrides.Find(property.id) >= 0;
        if(!authored)
            return;

        const String style_var = member + "_style";
        out << "\tUiList::Style " << style_var << " = UiTheme::ResolveList();\n";
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(property.id);
            if(q < 0)
                continue;
            const Value value = node.theme_overrides.GetValue(q);
            if(property.id == "row_height") out << "\t" << style_var << ".row_height = " << (int)value << ";\n";
            else if(property.id == "item_spacing") out << "\t" << style_var << ".item_spacing = " << (int)value << ";\n";
            else if(property.id == "icon_size") out << "\t" << style_var << ".icon_size = " << (int)value << ";\n";
            else if(property.id == "check_size") out << "\t" << style_var << ".check_size = " << (int)value << ";\n";
            else if(property.id == "content_gap") out << "\t" << style_var << ".content_gap = " << (int)value << ";\n";
            else if(property.id == "h_padding") out << "\t" << style_var << ".h_padding = " << (int)value << ";\n";
            else if(property.id == "v_padding") out << "\t" << style_var << ".v_padding = " << (int)value << ";\n";
            else if(property.id == "row_radius") out << "\t" << style_var << ".row_radius = " << (int)value << ";\n";
            else if(property.id == "metadata_size") out << "\t" << style_var << ".metadata_size = " << (int)value << ";\n";
            else if(property.id == "metadata_gap") out << "\t" << style_var << ".metadata_gap = " << (int)value << ";\n";
            else if(property.id == "right_gap") out << "\t" << style_var << ".right_gap = " << (int)value << ";\n";
            else if(property.id == "drag_size") out << "\t" << style_var << ".drag_size = " << (int)value << ";\n";
            else if(property.id == "drag_gap") out << "\t" << style_var << ".drag_gap = " << (int)value << ";\n";
            else if(property.id == "show_icons") out << "\t" << style_var << ".show_icons = " << AsString((bool)value) << ";\n";
            else if(property.id == "show_checks") out << "\t" << style_var << ".show_checks = " << AsString((bool)value) << ";\n";
            else if(property.id == "show_metadata_marker") out << "\t" << style_var << ".show_metadata_marker = " << AsString((bool)value) << ";\n";
            else if(property.id == "show_drag_handle") out << "\t" << style_var << ".show_drag_handle = " << AsString((bool)value) << ";\n";
            else if(property.id == "drag_side") out << "\t" << style_var << ".drag_side = " << (value == "Right" ? "UiAlign::RIGHT" : "UiAlign::LEFT") << ";\n";
            else if(property.id == "hot_as_underline") out << "\t" << style_var << ".hot_as_underline = " << AsString((bool)value) << ";\n";
            else if(property.id == "selected_as_underline") out << "\t" << style_var << ".selected_as_underline = " << AsString((bool)value) << ";\n";
            else if(property.id == "state_underline_thickness") out << "\t" << style_var << ".state_underline_thickness = " << (int)value << ";\n";
            else if(property.id == "striped_rows") out << "\t" << style_var << ".striped_rows = " << AsString((bool)value) << ";\n";
            else if(property.id == "ink") out << "\t" << style_var << ".ink = " << EmitValue(value) << ";\n";
            else if(property.id == "disabled_ink") out << "\t" << style_var << ".disabled_ink = " << EmitValue(value) << ";\n";
            else if(property.id == "muted_ink") out << "\t" << style_var << ".muted_ink = " << EmitValue(value) << ";\n";
            else if(property.id == "hot_face") out << "\t" << style_var << ".hot_face = " << EmitValue(value) << ";\n";
            else if(property.id == "hot_frame") out << "\t" << style_var << ".hot_frame = " << EmitValue(value) << ";\n";
            else if(property.id == "hot_ink") out << "\t" << style_var << ".hot_ink = " << EmitValue(value) << ";\n";
            else if(property.id == "selected_face") out << "\t" << style_var << ".selected_face = " << EmitValue(value) << ";\n";
            else if(property.id == "selected_frame") out << "\t" << style_var << ".selected_frame = " << EmitValue(value) << ";\n";
            else if(property.id == "selected_ink") out << "\t" << style_var << ".selected_ink = " << EmitValue(value) << ";\n";
            else if(property.id == "separator_color") out << "\t" << style_var << ".separator_color = " << EmitValue(value) << ";\n";
            else if(property.id == "row_even_face") out << "\t" << style_var << ".row_even_face = " << EmitValue(value) << ";\n";
            else if(property.id == "row_odd_face") out << "\t" << style_var << ".row_odd_face = " << EmitValue(value) << ";\n";
            else if(property.id == "show_row_separator") out << "\t" << style_var << ".show_row_separator = " << AsString((bool)value) << ";\n";
            else if(property.id == "row_state_frame_enabled") out << "\t" << style_var << ".row_state_frame_enabled = " << AsString((bool)value) << ";\n";
            else if(property.id == "right_text_as_badge") out << "\t" << style_var << ".right_text_as_badge = " << AsString((bool)value) << ";\n";
            else if(property.id == "badge_face") out << "\t" << style_var << ".badge_face = " << EmitValue(value) << ";\n";
            else if(property.id == "badge_frame") out << "\t" << style_var << ".badge_frame = " << EmitValue(value) << ";\n";
            else if(property.id == "badge_ink") out << "\t" << style_var << ".badge_ink = " << EmitValue(value) << ";\n";
            else if(property.id == "badge_radius") out << "\t" << style_var << ".badge_radius = " << (int)value << ";\n";
            else if(property.id == "badge_h_padding") out << "\t" << style_var << ".badge_h_padding = " << (int)value << ";\n";
            else if(property.id == "metadata_default") out << "\t" << style_var << ".metadata_default = " << EmitValue(value) << ";\n";
            else if(property.id == "check_frame") out << "\t" << style_var << ".check_frame = " << EmitValue(value) << ";\n";
            else if(property.id == "check_fill") out << "\t" << style_var << ".check_fill = " << EmitValue(value) << ";\n";
            else if(property.id == "drag_marker") out << "\t" << style_var << ".drag_marker = " << EmitValue(value) << ";\n";
        }
        out << "\t" << member << ".SetCustomStyle(" << style_var << ");\n";
    }
};

class MenuThemeAdapter final : public UiDesignerThemeAdapter {
public:
    const char *Id() const override { return "menu"; }
    bool Supports(UiDesignerRuntimeKind kind) const override
    {
        return kind == UiDesignerRuntimeKind::UiMenu;
    }

    void AddThemeOverrides(UiDesignerControlSpec& spec) const override
    {
        const UiMenu::Style base = UiTheme::ResolveMenu();
        AddOverride(spec, "row_height", "Row height", "Layout", PropertyEditorKind::Integer,
                    base.row_height, PropertyImpactPaint | PropertyImpactCode, "row_height");
        AddOverride(spec, "bar_height", "Bar height", "Layout", PropertyEditorKind::Integer,
                    base.bar_height, PropertyImpactPaint | PropertyImpactCode, "bar_height");
        AddOverride(spec, "icon_size", "Icon size", "Layout", PropertyEditorKind::Integer,
                    base.icon_size, PropertyImpactPaint | PropertyImpactCode, "icon_size");
        AddOverride(spec, "check_size", "Check size", "Layout", PropertyEditorKind::Integer,
                    base.check_size, PropertyImpactPaint | PropertyImpactCode, "check_size");
        AddOverride(spec, "arrow_size", "Arrow size", "Layout", PropertyEditorKind::Integer,
                    base.arrow_size, PropertyImpactPaint | PropertyImpactCode, "arrow_size");
        AddOverride(spec, "left_padding", "Left padding", "Layout", PropertyEditorKind::Integer,
                    base.left_padding, PropertyImpactPaint | PropertyImpactCode, "left_padding");
        AddOverride(spec, "right_padding", "Right padding", "Layout", PropertyEditorKind::Integer,
                    base.right_padding, PropertyImpactPaint | PropertyImpactCode, "right_padding");
        AddOverride(spec, "content_gap", "Content gap", "Layout", PropertyEditorKind::Integer,
                    base.content_gap, PropertyImpactPaint | PropertyImpactCode, "content_gap");
        AddOverride(spec, "item_spacing", "Item spacing", "Layout", PropertyEditorKind::Integer,
                    base.item_spacing, PropertyImpactPaint | PropertyImpactCode, "item_spacing");
        AddOverride(spec, "right_gap", "Right gap", "Layout", PropertyEditorKind::Integer,
                    base.right_gap, PropertyImpactPaint | PropertyImpactCode, "right_gap");
        AddOverride(spec, "popup_padding", "Popup padding", "Layout", PropertyEditorKind::Integer,
                    base.popup_padding, PropertyImpactPaint | PropertyImpactCode, "popup_padding");
        AddOverride(spec, "popup_min_width", "Popup min width", "Layout", PropertyEditorKind::Integer,
                    base.popup_min_width, PropertyImpactPaint | PropertyImpactCode, "popup_min_width");
        AddOverride(spec, "popup_max_height", "Popup max height", "Layout", PropertyEditorKind::Integer,
                    base.popup_max_height, PropertyImpactPaint | PropertyImpactCode, "popup_max_height");
        AddOverride(spec, "popup_shadow_margin", "Popup shadow margin", "Layout", PropertyEditorKind::Integer,
                    base.popup_shadow_margin, PropertyImpactPaint | PropertyImpactCode, "popup_shadow_margin");
        AddOverride(spec, "submenu_overlap", "Submenu overlap", "Layout", PropertyEditorKind::Integer,
                    base.submenu_overlap, PropertyImpactPaint | PropertyImpactCode, "submenu_overlap");
        AddOverride(spec, "show_icons", "Show icons", "Visibility", PropertyEditorKind::Boolean,
                    base.show_icons, PropertyImpactPaint | PropertyImpactCode, "show_icons");
        AddOverride(spec, "show_checks", "Show checks", "Visibility", PropertyEditorKind::Boolean,
                    base.show_checks, PropertyImpactPaint | PropertyImpactCode, "show_checks");
        AddOverride(spec, "show_descriptions", "Show descriptions", "Visibility", PropertyEditorKind::Boolean,
                    base.show_descriptions, PropertyImpactPaint | PropertyImpactCode, "show_descriptions");
        AddOverride(spec, "show_shortcuts", "Show shortcuts", "Visibility", PropertyEditorKind::Boolean,
                    base.show_shortcuts, PropertyImpactPaint | PropertyImpactCode, "show_shortcuts");
        AddOverride(spec, "show_separators", "Show separators", "Visibility", PropertyEditorKind::Boolean,
                    base.show_separators, PropertyImpactPaint | PropertyImpactCode, "show_separators");
        AddOverride(spec, "popup_bg", "Popup background", "Colours", PropertyEditorKind::Color,
                    base.popup_bg, PropertyImpactPaint | PropertyImpactCode, "popup_bg");
        AddOverride(spec, "bar_bg", "Bar background", "Colours", PropertyEditorKind::Color,
                    base.bar_bg, PropertyImpactPaint | PropertyImpactCode, "bar_bg");
        AddOverride(spec, "separator_color", "Separator color", "Colours", PropertyEditorKind::Color,
                    base.separator_color, PropertyImpactPaint | PropertyImpactCode, "separator_color");
        AddOverride(spec, "item_ink", "Item ink", "Colours", PropertyEditorKind::Color,
                    base.item_ink, PropertyImpactPaint | PropertyImpactCode, "item_ink");
        AddOverride(spec, "disabled_ink", "Disabled ink", "Colours", PropertyEditorKind::Color,
                    base.disabled_ink, PropertyImpactPaint | PropertyImpactCode, "disabled_ink");
        AddOverride(spec, "right_ink", "Right ink", "Colours", PropertyEditorKind::Color,
                    base.right_ink, PropertyImpactPaint | PropertyImpactCode, "right_ink");
        AddOverride(spec, "hot_bg", "Hot background", "Colours", PropertyEditorKind::Color,
                    base.hot_bg, PropertyImpactPaint | PropertyImpactCode, "hot_bg");
        AddOverride(spec, "hot_frame", "Hot frame", "Colours", PropertyEditorKind::Color,
                    base.hot_frame, PropertyImpactPaint | PropertyImpactCode, "hot_frame");
        AddOverride(spec, "pressed_bg", "Pressed background", "Colours", PropertyEditorKind::Color,
                    base.pressed_bg, PropertyImpactPaint | PropertyImpactCode, "pressed_bg");
        AddOverride(spec, "pressed_frame", "Pressed frame", "Colours", PropertyEditorKind::Color,
                    base.pressed_frame, PropertyImpactPaint | PropertyImpactCode, "pressed_frame");
        AddOverride(spec, "active_bar_bg", "Active bar background", "Colours", PropertyEditorKind::Color,
                    base.active_bar_bg, PropertyImpactPaint | PropertyImpactCode, "active_bar_bg");
        AddOverride(spec, "check_color", "Check color", "Colours", PropertyEditorKind::Color,
                    base.check_color, PropertyImpactPaint | PropertyImpactCode, "check_color");
        AddOverride(spec, "arrow_color", "Arrow color", "Colours", PropertyEditorKind::Color,
                    base.arrow_color, PropertyImpactPaint | PropertyImpactCode, "arrow_color");
        AddOverride(spec, "shadow_color", "Shadow color", "Colours", PropertyEditorKind::Color,
                    base.shadow_color, PropertyImpactPaint | PropertyImpactCode, "shadow_color");
    }

    bool HasField(const String& field_id) const override
    {
        static const char *fields[] = {
            "row_height", "bar_height", "icon_size", "check_size", "arrow_size",
            "left_padding", "right_padding", "content_gap", "item_spacing",
            "right_gap", "popup_padding", "popup_min_width", "popup_max_height",
            "popup_shadow_margin", "submenu_overlap", "show_icons",
            "show_checks", "show_descriptions", "show_shortcuts", "show_separators",
            "popup_bg", "bar_bg", "separator_color", "item_ink",
            "disabled_ink", "right_ink", "hot_bg", "hot_frame", "pressed_bg",
            "pressed_frame", "active_bar_bg", "check_color", "arrow_color",
            "shadow_color"
        };
        return FieldMatches(field_id, fields, __countof(fields));
    }

    bool FieldAffectsLayout(const String& field_id) const override
    {
        static const char *layout_fields[] = {
            "row_height", "bar_height", "icon_size", "check_size", "arrow_size",
            "left_padding", "right_padding", "content_gap", "item_spacing",
            "right_gap", "popup_padding", "popup_min_width", "popup_max_height",
            "popup_shadow_margin", "submenu_overlap", "show_icons",
            "show_checks", "show_descriptions", "show_shortcuts", "show_separators"
        };
        return FieldMatches(field_id, layout_fields, __countof(layout_fields));
    }

    Value ResolveFieldValue(const UiDesignerNode& node,
                            const UiDesignerControlSpec& spec,
                            const String& field_id,
                            const UiDesignerTransientOverlay* overlay) const override
    {
        UiMenu::Style style = UiTheme::ResolveMenu();
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(property.id);
            const bool active = q >= 0 || HasThemeValue(node, overlay, property.id);
            if(!active)
                continue;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q)
                                           : property.default_value;
            const Value effective = ResolveThemeValue(node, overlay, property.id, canonical);
            if(property.id == "row_height") style.row_height = (int)effective;
            else if(property.id == "bar_height") style.bar_height = (int)effective;
            else if(property.id == "icon_size") style.icon_size = (int)effective;
            else if(property.id == "check_size") style.check_size = (int)effective;
            else if(property.id == "arrow_size") style.arrow_size = (int)effective;
            else if(property.id == "left_padding") style.left_padding = (int)effective;
            else if(property.id == "right_padding") style.right_padding = (int)effective;
            else if(property.id == "content_gap") style.content_gap = (int)effective;
            else if(property.id == "item_spacing") style.item_spacing = (int)effective;
            else if(property.id == "right_gap") style.right_gap = (int)effective;
            else if(property.id == "popup_padding") style.popup_padding = (int)effective;
            else if(property.id == "popup_min_width") style.popup_min_width = (int)effective;
            else if(property.id == "popup_max_height") style.popup_max_height = (int)effective;
            else if(property.id == "popup_shadow_margin") style.popup_shadow_margin = (int)effective;
            else if(property.id == "submenu_overlap") style.submenu_overlap = (int)effective;
            else if(property.id == "show_icons") style.show_icons = (bool)effective;
            else if(property.id == "show_checks") style.show_checks = (bool)effective;
            else if(property.id == "show_descriptions") style.show_descriptions = (bool)effective;
            else if(property.id == "show_shortcuts") style.show_shortcuts = (bool)effective;
            else if(property.id == "show_separators") style.show_separators = (bool)effective;
            else if(property.id == "popup_bg") style.popup_bg = (Color)effective;
            else if(property.id == "bar_bg") style.bar_bg = (Color)effective;
            else if(property.id == "separator_color") style.separator_color = (Color)effective;
            else if(property.id == "item_ink") style.item_ink = (Color)effective;
            else if(property.id == "disabled_ink") style.disabled_ink = (Color)effective;
            else if(property.id == "right_ink") style.right_ink = (Color)effective;
            else if(property.id == "hot_bg") style.hot_bg = (Color)effective;
            else if(property.id == "hot_frame") style.hot_frame = (Color)effective;
            else if(property.id == "pressed_bg") style.pressed_bg = (Color)effective;
            else if(property.id == "pressed_frame") style.pressed_frame = (Color)effective;
            else if(property.id == "active_bar_bg") style.active_bar_bg = (Color)effective;
            else if(property.id == "check_color") style.check_color = (Color)effective;
            else if(property.id == "arrow_color") style.arrow_color = (Color)effective;
            else if(property.id == "shadow_color") style.shadow_color = (Color)effective;
        }
        if(field_id == "row_height") return style.row_height;
        if(field_id == "bar_height") return style.bar_height;
        if(field_id == "icon_size") return style.icon_size;
        if(field_id == "check_size") return style.check_size;
        if(field_id == "arrow_size") return style.arrow_size;
        if(field_id == "left_padding") return style.left_padding;
        if(field_id == "right_padding") return style.right_padding;
        if(field_id == "content_gap") return style.content_gap;
        if(field_id == "item_spacing") return style.item_spacing;
        if(field_id == "right_gap") return style.right_gap;
        if(field_id == "popup_padding") return style.popup_padding;
        if(field_id == "popup_min_width") return style.popup_min_width;
        if(field_id == "popup_max_height") return style.popup_max_height;
        if(field_id == "popup_shadow_margin") return style.popup_shadow_margin;
        if(field_id == "submenu_overlap") return style.submenu_overlap;
        if(field_id == "show_icons") return style.show_icons;
        if(field_id == "show_checks") return style.show_checks;
        if(field_id == "show_descriptions") return style.show_descriptions;
        if(field_id == "show_shortcuts") return style.show_shortcuts;
        if(field_id == "show_separators") return style.show_separators;
        if(field_id == "popup_bg") return style.popup_bg;
        if(field_id == "bar_bg") return style.bar_bg;
        if(field_id == "separator_color") return style.separator_color;
        if(field_id == "item_ink") return style.item_ink;
        if(field_id == "disabled_ink") return style.disabled_ink;
        if(field_id == "right_ink") return style.right_ink;
        if(field_id == "hot_bg") return style.hot_bg;
        if(field_id == "hot_frame") return style.hot_frame;
        if(field_id == "pressed_bg") return style.pressed_bg;
        if(field_id == "pressed_frame") return style.pressed_frame;
        if(field_id == "active_bar_bg") return style.active_bar_bg;
        if(field_id == "check_color") return style.check_color;
        if(field_id == "arrow_color") return style.arrow_color;
        if(field_id == "shadow_color") return style.shadow_color;
        return Value();
    }

    void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node,
                           const UiDesignerControlSpec& spec,
                           const UiDesignerTransientOverlay* overlay) const override
    {
        UiMenu *menu = dynamic_cast<UiMenu *>(&ctrl);
        if(!menu)
            return;
        UiMenu::Style style = UiTheme::ResolveMenu();
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(property.id);
            const bool active = q >= 0 || HasThemeValue(node, overlay, property.id);
            if(!active)
                continue;
            authored = true;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q)
                                           : property.default_value;
            const Value effective = ResolveThemeValue(node, overlay, property.id, canonical);
            if(property.id == "row_height") style.row_height = (int)effective;
            else if(property.id == "bar_height") style.bar_height = (int)effective;
            else if(property.id == "icon_size") style.icon_size = (int)effective;
            else if(property.id == "check_size") style.check_size = (int)effective;
            else if(property.id == "arrow_size") style.arrow_size = (int)effective;
            else if(property.id == "left_padding") style.left_padding = (int)effective;
            else if(property.id == "right_padding") style.right_padding = (int)effective;
            else if(property.id == "content_gap") style.content_gap = (int)effective;
            else if(property.id == "item_spacing") style.item_spacing = (int)effective;
            else if(property.id == "right_gap") style.right_gap = (int)effective;
            else if(property.id == "popup_padding") style.popup_padding = (int)effective;
            else if(property.id == "popup_min_width") style.popup_min_width = (int)effective;
            else if(property.id == "popup_max_height") style.popup_max_height = (int)effective;
            else if(property.id == "popup_shadow_margin") style.popup_shadow_margin = (int)effective;
            else if(property.id == "submenu_overlap") style.submenu_overlap = (int)effective;
            else if(property.id == "show_icons") style.show_icons = (bool)effective;
            else if(property.id == "show_checks") style.show_checks = (bool)effective;
            else if(property.id == "show_descriptions") style.show_descriptions = (bool)effective;
            else if(property.id == "show_shortcuts") style.show_shortcuts = (bool)effective;
            else if(property.id == "show_separators") style.show_separators = (bool)effective;
            else if(property.id == "popup_bg") style.popup_bg = (Color)effective;
            else if(property.id == "bar_bg") style.bar_bg = (Color)effective;
            else if(property.id == "separator_color") style.separator_color = (Color)effective;
            else if(property.id == "item_ink") style.item_ink = (Color)effective;
            else if(property.id == "disabled_ink") style.disabled_ink = (Color)effective;
            else if(property.id == "right_ink") style.right_ink = (Color)effective;
            else if(property.id == "hot_bg") style.hot_bg = (Color)effective;
            else if(property.id == "hot_frame") style.hot_frame = (Color)effective;
            else if(property.id == "pressed_bg") style.pressed_bg = (Color)effective;
            else if(property.id == "pressed_frame") style.pressed_frame = (Color)effective;
            else if(property.id == "active_bar_bg") style.active_bar_bg = (Color)effective;
            else if(property.id == "check_color") style.check_color = (Color)effective;
            else if(property.id == "arrow_color") style.arrow_color = (Color)effective;
            else if(property.id == "shadow_color") style.shadow_color = (Color)effective;
        }
        if(authored)
            menu->SetCustomStyle(style);
        else
            menu->ClearCustomStyle();
    }

    void EmitSetup(String& out, const String& member,
                   const UiDesignerNode& node,
                   const UiDesignerControlSpec& spec) const override
    {
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides)
            authored |= node.theme_overrides.Find(property.id) >= 0;
        if(!authored)
            return;

        const String style_var = member + "_style";
        out << "\tUiMenu::Style " << style_var << " = UiTheme::ResolveMenu();\n";
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(property.id);
            if(q < 0)
                continue;
            const Value value = node.theme_overrides.GetValue(q);
            if(property.id == "row_height") out << "\t" << style_var << ".row_height = " << (int)value << ";\n";
            else if(property.id == "bar_height") out << "\t" << style_var << ".bar_height = " << (int)value << ";\n";
            else if(property.id == "icon_size") out << "\t" << style_var << ".icon_size = " << (int)value << ";\n";
            else if(property.id == "check_size") out << "\t" << style_var << ".check_size = " << (int)value << ";\n";
            else if(property.id == "arrow_size") out << "\t" << style_var << ".arrow_size = " << (int)value << ";\n";
            else if(property.id == "left_padding") out << "\t" << style_var << ".left_padding = " << (int)value << ";\n";
            else if(property.id == "right_padding") out << "\t" << style_var << ".right_padding = " << (int)value << ";\n";
            else if(property.id == "content_gap") out << "\t" << style_var << ".content_gap = " << (int)value << ";\n";
            else if(property.id == "item_spacing") out << "\t" << style_var << ".item_spacing = " << (int)value << ";\n";
            else if(property.id == "right_gap") out << "\t" << style_var << ".right_gap = " << (int)value << ";\n";
            else if(property.id == "popup_padding") out << "\t" << style_var << ".popup_padding = " << (int)value << ";\n";
            else if(property.id == "popup_min_width") out << "\t" << style_var << ".popup_min_width = " << (int)value << ";\n";
            else if(property.id == "popup_max_height") out << "\t" << style_var << ".popup_max_height = " << (int)value << ";\n";
            else if(property.id == "popup_shadow_margin") out << "\t" << style_var << ".popup_shadow_margin = " << (int)value << ";\n";
            else if(property.id == "submenu_overlap") out << "\t" << style_var << ".submenu_overlap = " << (int)value << ";\n";
            else if(property.id == "show_icons") out << "\t" << style_var << ".show_icons = " << AsString((bool)value) << ";\n";
            else if(property.id == "show_checks") out << "\t" << style_var << ".show_checks = " << AsString((bool)value) << ";\n";
            else if(property.id == "show_descriptions") out << "\t" << style_var << ".show_descriptions = " << AsString((bool)value) << ";\n";
            else if(property.id == "show_shortcuts") out << "\t" << style_var << ".show_shortcuts = " << AsString((bool)value) << ";\n";
            else if(property.id == "show_separators") out << "\t" << style_var << ".show_separators = " << AsString((bool)value) << ";\n";
            else if(property.id == "popup_bg") out << "\t" << style_var << ".popup_bg = " << EmitValue(value) << ";\n";
            else if(property.id == "bar_bg") out << "\t" << style_var << ".bar_bg = " << EmitValue(value) << ";\n";
            else if(property.id == "separator_color") out << "\t" << style_var << ".separator_color = " << EmitValue(value) << ";\n";
            else if(property.id == "item_ink") out << "\t" << style_var << ".item_ink = " << EmitValue(value) << ";\n";
            else if(property.id == "disabled_ink") out << "\t" << style_var << ".disabled_ink = " << EmitValue(value) << ";\n";
            else if(property.id == "right_ink") out << "\t" << style_var << ".right_ink = " << EmitValue(value) << ";\n";
            else if(property.id == "hot_bg") out << "\t" << style_var << ".hot_bg = " << EmitValue(value) << ";\n";
            else if(property.id == "hot_frame") out << "\t" << style_var << ".hot_frame = " << EmitValue(value) << ";\n";
            else if(property.id == "pressed_bg") out << "\t" << style_var << ".pressed_bg = " << EmitValue(value) << ";\n";
            else if(property.id == "pressed_frame") out << "\t" << style_var << ".pressed_frame = " << EmitValue(value) << ";\n";
            else if(property.id == "active_bar_bg") out << "\t" << style_var << ".active_bar_bg = " << EmitValue(value) << ";\n";
            else if(property.id == "check_color") out << "\t" << style_var << ".check_color = " << EmitValue(value) << ";\n";
            else if(property.id == "arrow_color") out << "\t" << style_var << ".arrow_color = " << EmitValue(value) << ";\n";
            else if(property.id == "shadow_color") out << "\t" << style_var << ".shadow_color = " << EmitValue(value) << ";\n";
        }
        out << "\t" << member << ".SetCustomStyle(" << style_var << ");\n";
    }
};

class ColorPickerThemeAdapter final : public UiDesignerThemeAdapter {
public:
    const char *Id() const override { return "color_picker"; }
    bool Supports(UiDesignerRuntimeKind kind) const override
    {
        return kind == UiDesignerRuntimeKind::UiColorPicker;
    }

    void AddThemeOverrides(UiDesignerControlSpec& spec) const override
    {
        const UiColorPicker::Style& base = UiColorPicker::StyleDefault();
        AddOverride(spec, "face", "Face colour", "Surface", PropertyEditorKind::Color,
                    UiDesignerFillColor(base.palette.face[ST_NORMAL]),
                    PropertyImpactPaint | PropertyImpactCode, "face");
        AddOverride(spec, "frame", "Frame colour", "Surface", PropertyEditorKind::Color,
                    base.palette.frame[ST_NORMAL],
                    PropertyImpactPaint | PropertyImpactCode, "frame");
        AddOverride(spec, "radius", "Radius", "Surface", PropertyEditorKind::Integer,
                    base.metrics.radius, PropertyImpactPaint | PropertyImpactCode, "radius");
        AddOverride(spec, "navigation_height", "Navigation height", "Metrics",
                    PropertyEditorKind::Integer, base.navigation_height,
                    PropertyImpactPaint | PropertyImpactCode, "navigation_height");
        AddOverride(spec, "footer_height", "Footer height", "Metrics",
                    PropertyEditorKind::Integer, base.footer_height,
                    PropertyImpactPaint | PropertyImpactCode, "footer_height");
        AddOverride(spec, "slot_size", "Slot size", "Metrics", PropertyEditorKind::Integer,
                    base.slot_size, PropertyImpactPaint | PropertyImpactCode, "slot_size");
        AddOverride(spec, "slot_gap", "Slot gap", "Metrics", PropertyEditorKind::Integer,
                    base.slot_gap, PropertyImpactPaint | PropertyImpactCode, "slot_gap");
        AddOverride(spec, "page_gap", "Page gap", "Metrics", PropertyEditorKind::Integer,
                    base.page_gap, PropertyImpactPaint | PropertyImpactCode, "page_gap");
        AddOverride(spec, "right_panel_width", "Right panel width", "Metrics",
                    PropertyEditorKind::Integer, base.right_panel_width,
                    PropertyImpactPaint | PropertyImpactCode, "right_panel_width");
        AddOverride(spec, "section_gap", "Section gap", "Metrics", PropertyEditorKind::Integer,
                    base.section_gap, PropertyImpactPaint | PropertyImpactCode, "section_gap");
        AddOverride(spec, "readout_row_height", "Readout row height", "Metrics",
                    PropertyEditorKind::Integer, base.readout_row_height,
                    PropertyImpactPaint | PropertyImpactCode, "readout_row_height");
        AddOverride(spec, "channel_row_height", "Channel row height", "Metrics",
                    PropertyEditorKind::Integer, base.channel_row_height,
                    PropertyImpactPaint | PropertyImpactCode, "channel_row_height");
        AddOverride(spec, "button_height", "Button height", "Metrics",
                    PropertyEditorKind::Integer, base.button_height,
                    PropertyImpactPaint | PropertyImpactCode, "button_height");
    }

    bool HasField(const String& field_id) const override
    {
        static const char *fields[] = {"face", "frame", "radius", "navigation_height",
            "footer_height", "slot_size", "slot_gap", "page_gap", "right_panel_width",
            "section_gap", "readout_row_height", "channel_row_height", "button_height"};
        return FieldMatches(field_id, fields, (int)(sizeof(fields) / sizeof(fields[0])));
    }

    bool FieldAffectsLayout(const String& field_id) const override
    {
        return field_id != "face" && field_id != "frame" && field_id != "radius";
    }

    Value ResolveFieldValue(const UiDesignerNode& node, const UiDesignerControlSpec& spec,
                            const String& field_id,
                            const UiDesignerTransientOverlay* overlay) const override
    {
        const UiColorPicker::Style& base = UiColorPicker::StyleDefault();
        Value value;
        if(field_id == "face") value = UiDesignerFillColor(base.palette.face[ST_NORMAL]);
        else if(field_id == "frame") value = base.palette.frame[ST_NORMAL];
        else if(field_id == "radius") value = base.metrics.radius;
        else if(field_id == "navigation_height") value = base.navigation_height;
        else if(field_id == "footer_height") value = base.footer_height;
        else if(field_id == "slot_size") value = base.slot_size;
        else if(field_id == "slot_gap") value = base.slot_gap;
        else if(field_id == "page_gap") value = base.page_gap;
        else if(field_id == "right_panel_width") value = base.right_panel_width;
        else if(field_id == "section_gap") value = base.section_gap;
        else if(field_id == "readout_row_height") value = base.readout_row_height;
        else if(field_id == "channel_row_height") value = base.channel_row_height;
        else if(field_id == "button_height") value = base.button_height;
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides)
            if(p.adapter_field_id == field_id && (node.theme_overrides.Find(p.id) >= 0 || HasThemeValue(node, overlay, p.id))) {
                const int q = node.theme_overrides.Find(p.id);
                value = ResolveThemeValue(node, overlay, p.id,
                                          q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value);
            }
        return value;
    }

    void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node,
                           const UiDesignerControlSpec& spec,
                           const UiDesignerTransientOverlay* overlay) const override
    {
        UiColorPicker *picker = dynamic_cast<UiColorPicker *>(&ctrl);
        if(!picker) return;
        UiColorPicker::Style style = UiColorPicker::StyleDefault();
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            if(node.theme_overrides.Find(p.id) < 0 && !HasThemeValue(node, overlay, p.id)) continue;
            authored = true;
            Value v = ResolveFieldValue(node, spec, p.adapter_field_id, overlay);
            if(p.adapter_field_id == "face") style.palette.face[ST_NORMAL] = UiFill::Solid((Color)v);
            else if(p.adapter_field_id == "frame") style.palette.frame[ST_NORMAL] = (Color)v;
            else if(p.adapter_field_id == "radius") style.metrics.radius = max(0, (int)v);
            else if(p.adapter_field_id == "navigation_height") style.navigation_height = max(1, (int)v);
            else if(p.adapter_field_id == "footer_height") style.footer_height = max(1, (int)v);
            else if(p.adapter_field_id == "slot_size") style.slot_size = max(1, (int)v);
            else if(p.adapter_field_id == "slot_gap") style.slot_gap = max(0, (int)v);
            else if(p.adapter_field_id == "page_gap") style.page_gap = max(0, (int)v);
            else if(p.adapter_field_id == "right_panel_width") style.right_panel_width = max(1, (int)v);
            else if(p.adapter_field_id == "section_gap") style.section_gap = max(0, (int)v);
            else if(p.adapter_field_id == "readout_row_height") style.readout_row_height = max(1, (int)v);
            else if(p.adapter_field_id == "channel_row_height") style.channel_row_height = max(1, (int)v);
            else if(p.adapter_field_id == "button_height") style.button_height = max(1, (int)v);
        }
        if(authored) picker->SetCustomStyle(style);
        else picker->ClearCustomStyle();
    }

    void EmitSetup(String& out, const String& member, const UiDesignerNode& node,
                   const UiDesignerControlSpec& spec) const override
    {
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides)
            authored |= node.theme_overrides.Find(p.id) >= 0;
        if(!authored) return;
        String var = member + "_style";
        out << "\tUiColorPicker::Style " << var << " = UiColorPicker::StyleDefault();\n";
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            int q = node.theme_overrides.Find(p.id);
            if(q < 0) continue;
            Value v = node.theme_overrides.GetValue(q);
            if(p.adapter_field_id == "face") out << "\t" << var << ".palette.face[ST_NORMAL] = UiFill::Solid(" << EmitValue(v) << ");\n";
            else if(p.adapter_field_id == "frame") out << "\t" << var << ".palette.frame[ST_NORMAL] = " << EmitValue(v) << ";\n";
            else if(p.adapter_field_id == "radius") out << "\t" << var << ".metrics.radius = " << (int)v << ";\n";
            else out << "\t" << var << "." << p.adapter_field_id << " = " << (int)v << ";\n";
        }
        out << "\t" << member << ".SetCustomStyle(" << var << ");\n";
    }
};

static UiTitleCard::Style ResolveTitleCardThemeBase(UiRole role)
{
    return UiTheme::ResolveTitleCard(role);
}

static const char *const edit_fields[] = {
    "font_face", "font_size", "font_bold", "font_italic", "text_align",
    "face_enabled", "face_normal", "face_hot", "face_pressed", "face_disabled",
    "frame_enabled", "frame_normal", "frame_hot", "frame_pressed", "frame_disabled",
    "frame_width", "radius", "text_normal", "text_hot", "text_pressed", "text_disabled",
    "caret_color", "caret_width", "block_caret", "selection_color", "selection_ink",
    "placeholder_ink", "underline_enabled", "underline_width",
    "underline_normal", "underline_hot", "underline_pressed", "underline_disabled"
};

static UiBaseEdit::Style ResolveEditStyleBase(const UiDesignerNode& node)
{
    return UiTheme::ResolveEdit(ParseRole(node.GetProperty("role", "Standard")));
}

static Value EditFieldValue(const UiBaseEdit::Style& s, const String& id)
{
    if(id == "font_face") return s.font.GetFaceName();
    if(id == "font_size") return s.font.GetHeight();
    if(id == "font_bold") return s.font.IsBold();
    if(id == "font_italic") return s.font.IsItalic();
    if(id == "text_align") return s.text_align == UiAlign::RIGHT ? "Right" :
        s.text_align == UiAlign::CENTER ? "Center" : "Left";
    if(id == "face_enabled") return s.metrics.face_enabled;
    if(id == "face_normal") return UiDesignerFillColor(s.palette.face[ST_NORMAL]);
    if(id == "face_hot") return UiDesignerFillColor(s.palette.face[ST_HOT]);
    if(id == "face_pressed") return UiDesignerFillColor(s.palette.face[ST_PRESSED]);
    if(id == "face_disabled") return UiDesignerFillColor(s.palette.face[ST_DISABLED]);
    if(id == "frame_enabled") return s.metrics.frame_enabled;
    if(id == "frame_normal") return s.palette.frame[ST_NORMAL];
    if(id == "frame_hot") return s.palette.frame[ST_HOT];
    if(id == "frame_pressed") return s.palette.frame[ST_PRESSED];
    if(id == "frame_disabled") return s.palette.frame[ST_DISABLED];
    if(id == "frame_width") return s.metrics.frame_width;
    if(id == "radius") return s.metrics.radius;
    if(id == "text_normal") return s.palette.ink[ST_NORMAL];
    if(id == "text_hot") return s.palette.ink[ST_HOT];
    if(id == "text_pressed") return s.palette.ink[ST_PRESSED];
    if(id == "text_disabled") return s.palette.ink[ST_DISABLED];
    if(id == "caret_color") return s.caret_color;
    if(id == "caret_width") return s.caret_width;
    if(id == "block_caret") return s.block_caret;
    if(id == "selection_color") return s.selection_color;
    if(id == "selection_ink") return s.selection_ink;
    if(id == "placeholder_ink") return s.placeholder_ink;
    if(id == "underline_enabled") return s.underline_enabled;
    if(id == "underline_width") return s.underline_width;
    if(id == "underline_normal") return s.underline[ST_NORMAL];
    if(id == "underline_hot") return s.underline[ST_HOT];
    if(id == "underline_pressed") return s.underline[ST_PRESSED];
    if(id == "underline_disabled") return s.underline[ST_DISABLED];
    return Value();
}

static void ApplyEditField(UiBaseEdit::Style& s, const String& id, const Value& v)
{
    if(id == "font_face") s.font.FaceName(AsString(v));
    else if(id == "font_size") s.font.Height(max(1, (int)v));
    else if(id == "font_bold") s.font.Bold((bool)v);
    else if(id == "font_italic") s.font.Italic((bool)v);
    else if(id == "text_align") s.text_align = AsString(v) == "Right" ? UiAlign::RIGHT :
        AsString(v) == "Center" ? UiAlign::CENTER : UiAlign::LEFT;
    else if(id == "face_enabled") s.metrics.face_enabled = (bool)v;
    else if(id == "face_normal") s.palette.face[ST_NORMAL] = UiFill::Solid(Color(v));
    else if(id == "face_hot") s.palette.face[ST_HOT] = UiFill::Solid(Color(v));
    else if(id == "face_pressed") s.palette.face[ST_PRESSED] = UiFill::Solid(Color(v));
    else if(id == "face_disabled") s.palette.face[ST_DISABLED] = UiFill::Solid(Color(v));
    else if(id == "frame_enabled") s.metrics.frame_enabled = (bool)v;
    else if(id == "frame_normal") s.palette.frame[ST_NORMAL] = Color(v);
    else if(id == "frame_hot") s.palette.frame[ST_HOT] = Color(v);
    else if(id == "frame_pressed") s.palette.frame[ST_PRESSED] = Color(v);
    else if(id == "frame_disabled") s.palette.frame[ST_DISABLED] = Color(v);
    else if(id == "frame_width") s.metrics.frame_width = max(0, (int)v);
    else if(id == "radius") s.metrics.radius = max(0, (int)v);
    else if(id == "text_normal") s.palette.ink[ST_NORMAL] = Color(v);
    else if(id == "text_hot") s.palette.ink[ST_HOT] = Color(v);
    else if(id == "text_pressed") s.palette.ink[ST_PRESSED] = Color(v);
    else if(id == "text_disabled") s.palette.ink[ST_DISABLED] = Color(v);
    else if(id == "caret_color") s.caret_color = Color(v);
    else if(id == "caret_width") s.caret_width = max(1, (int)v);
    else if(id == "block_caret") s.block_caret = (bool)v;
    else if(id == "selection_color") s.selection_color = Color(v);
    else if(id == "selection_ink") s.selection_ink = Color(v);
    else if(id == "placeholder_ink") s.placeholder_ink = Color(v);
    else if(id == "underline_enabled") s.underline_enabled = (bool)v;
    else if(id == "underline_width") s.underline_width = max(1, (int)v);
    else if(id == "underline_normal") s.underline[ST_NORMAL] = Color(v);
    else if(id == "underline_hot") s.underline[ST_HOT] = Color(v);
    else if(id == "underline_pressed") s.underline[ST_PRESSED] = Color(v);
    else if(id == "underline_disabled") s.underline[ST_DISABLED] = Color(v);
}

static void AddEditThemeOverrides(UiDesignerControlSpec& spec)
{
    const UiBaseEdit::Style base = UiTheme::ResolveEdit(UiRole::Standard);
    auto add = [&](const char *id, const char *label, const char *group,
                   PropertyEditorKind kind, const Value& value,
                   PropertyEditorImpact impact = PropertyImpactPaint | PropertyImpactCode) {
        UiDesignerThemeOverrideSpec item;
        item.id = id; item.label = label; item.group = group; item.kind = kind;
        item.domain = PropertyEditorDomain::Theme; item.default_value = value;
        item.impact = impact; item.adapter_field_id = id;
        spec.theme_overrides.Add(pick(item));
    };
    add("font_face", "Font face", "Typography", PropertyEditorKind::Text, base.font.GetFaceName(), PropertyImpactPaint | PropertyImpactCode | PropertyImpactLocalLayout);
    add("font_size", "Font size", "Typography", PropertyEditorKind::Integer, base.font.GetHeight(), PropertyImpactPaint | PropertyImpactCode | PropertyImpactLocalLayout);
    add("font_bold", "Font bold", "Typography", PropertyEditorKind::Boolean, base.font.IsBold());
    add("font_italic", "Font italic", "Typography", PropertyEditorKind::Boolean, base.font.IsItalic());
    UiDesignerThemeOverrideSpec align; align.id = "text_align"; align.label = "Text align"; align.group = "Typography"; align.kind = PropertyEditorKind::Choice; align.domain = PropertyEditorDomain::Theme; align.default_value = "Left"; align.impact = PropertyImpactPaint | PropertyImpactCode; align.adapter_field_id = "text_align"; align.Choice("Left", "Left").Choice("Center", "Center").Choice("Right", "Right"); spec.theme_overrides.Add(pick(align));
    add("face_enabled", "Face enabled", "Face", PropertyEditorKind::Boolean, base.metrics.face_enabled);
    add("face_normal", "Face normal", "Face", PropertyEditorKind::Color, UiDesignerFillColor(base.palette.face[ST_NORMAL]));
    add("face_hot", "Face hot", "Face", PropertyEditorKind::Color, UiDesignerFillColor(base.palette.face[ST_HOT]));
    add("face_pressed", "Face pressed", "Face", PropertyEditorKind::Color, UiDesignerFillColor(base.palette.face[ST_PRESSED]));
    add("face_disabled", "Face disabled", "Face", PropertyEditorKind::Color, UiDesignerFillColor(base.palette.face[ST_DISABLED]));
    add("frame_enabled", "Frame enabled", "Frame", PropertyEditorKind::Boolean, base.metrics.frame_enabled);
    add("frame_normal", "Frame normal", "Frame", PropertyEditorKind::Color, base.palette.frame[ST_NORMAL]);
    add("frame_hot", "Frame hot", "Frame", PropertyEditorKind::Color, base.palette.frame[ST_HOT]);
    add("frame_pressed", "Frame pressed", "Frame", PropertyEditorKind::Color, base.palette.frame[ST_PRESSED]);
    add("frame_disabled", "Frame disabled", "Frame", PropertyEditorKind::Color, base.palette.frame[ST_DISABLED]);
    add("frame_width", "Frame width", "Frame", PropertyEditorKind::Integer, base.metrics.frame_width);
    add("radius", "Radius", "Frame", PropertyEditorKind::Integer, base.metrics.radius);
    add("text_normal", "Text normal", "Text", PropertyEditorKind::Color, base.palette.ink[ST_NORMAL]);
    add("text_hot", "Text hot", "Text", PropertyEditorKind::Color, base.palette.ink[ST_HOT]);
    add("text_pressed", "Text pressed", "Text", PropertyEditorKind::Color, base.palette.ink[ST_PRESSED]);
    add("text_disabled", "Text disabled", "Text", PropertyEditorKind::Color, base.palette.ink[ST_DISABLED]);
    add("caret_color", "Caret color", "Editing", PropertyEditorKind::Color, base.caret_color);
    add("caret_width", "Caret width", "Editing", PropertyEditorKind::Integer, base.caret_width);
    add("block_caret", "Block caret", "Editing", PropertyEditorKind::Boolean, base.block_caret);
    add("selection_color", "Selection color", "Editing", PropertyEditorKind::Color, base.selection_color);
    add("selection_ink", "Selection ink", "Editing", PropertyEditorKind::Color, base.selection_ink);
    add("placeholder_ink", "Placeholder ink", "Editing", PropertyEditorKind::Color, base.placeholder_ink);
    add("underline_enabled", "Underline enabled", "Underline", PropertyEditorKind::Boolean, base.underline_enabled);
    add("underline_width", "Underline width", "Underline", PropertyEditorKind::Integer, base.underline_width);
    add("underline_normal", "Underline normal", "Underline", PropertyEditorKind::Color, base.underline[ST_NORMAL]);
    add("underline_hot", "Underline hot", "Underline", PropertyEditorKind::Color, base.underline[ST_HOT]);
    add("underline_pressed", "Underline pressed", "Underline", PropertyEditorKind::Color, base.underline[ST_PRESSED]);
    add("underline_disabled", "Underline disabled", "Underline", PropertyEditorKind::Color, base.underline[ST_DISABLED]);
}

static void EmitEditField(String& out, const String& style, const String& id,
                          const Value& value)
{
    if(id == "font_face") out << style << ".font.FaceName(" << CppString(AsString(value)) << ");\n";
    else if(id == "font_size") out << style << ".font.Height(" << max(1, (int)value) << ");\n";
    else if(id == "font_bold") out << style << ".font.Bold(" << ((bool)value ? "true" : "false") << ");\n";
    else if(id == "font_italic") out << style << ".font.Italic(" << ((bool)value ? "true" : "false") << ");\n";
    else if(id == "text_align") out << style << ".text_align = " << EmitAlign(AsString(value)) << ";\n";
    else if(id == "face_enabled") out << style << ".metrics.face_enabled = " << ((bool)value ? "true" : "false") << ";\n";
    else if(id.StartsWith("face_")) out << style << ".palette.face[ST_" << ToUpper(id.Mid(5)) << "] = UiFill::Solid(" << EmitValue(value) << ");\n";
    else if(id == "frame_enabled") out << style << ".metrics.frame_enabled = " << ((bool)value ? "true" : "false") << ";\n";
    else if(id.StartsWith("frame_")) out << style << ".palette.frame[ST_" << ToUpper(id.Mid(6)) << "] = " << EmitValue(value) << ";\n";
    else if(id == "frame_width") out << style << ".metrics.frame_width = " << (int)value << ";\n";
    else if(id == "radius") out << style << ".metrics.radius = " << (int)value << ";\n";
    else if(id.StartsWith("text_")) out << style << ".palette.ink[ST_" << ToUpper(id.Mid(5)) << "] = " << EmitValue(value) << ";\n";
    else if(id == "caret_color") out << style << ".caret_color = " << EmitValue(value) << ";\n";
    else if(id == "caret_width") out << style << ".caret_width = " << (int)value << ";\n";
    else if(id == "block_caret") out << style << ".block_caret = " << ((bool)value ? "true" : "false") << ";\n";
    else if(id == "selection_color") out << style << ".selection_color = " << EmitValue(value) << ";\n";
    else if(id == "selection_ink") out << style << ".selection_ink = " << EmitValue(value) << ";\n";
    else if(id == "placeholder_ink") out << style << ".placeholder_ink = " << EmitValue(value) << ";\n";
    else if(id == "underline_enabled") out << style << ".underline_enabled = " << ((bool)value ? "true" : "false") << ";\n";
    else if(id == "underline_width") out << style << ".underline_width = " << (int)value << ";\n";
    else if(id.StartsWith("underline_")) out << style << ".underline[ST_" << ToUpper(id.Mid(10)) << "] = " << EmitValue(value) << ";\n";
}

class EditThemeAdapter final : public UiDesignerThemeAdapter {
public:
    const char *Id() const override { return "edit"; }
    bool Supports(UiDesignerRuntimeKind kind) const override
    {
        return kind == UiDesignerRuntimeKind::UiLineEdit ||
               kind == UiDesignerRuntimeKind::UiIntEdit ||
               kind == UiDesignerRuntimeKind::UiFloatEdit ||
               kind == UiDesignerRuntimeKind::UiPasswordEdit ||
               kind == UiDesignerRuntimeKind::UiMultiEdit ||
               kind == UiDesignerRuntimeKind::UiMaskEdit;
    }
    void AddThemeOverrides(UiDesignerControlSpec& spec) const override { AddEditThemeOverrides(spec); }
    bool HasField(const String& field_id) const override { return FieldMatches(field_id, edit_fields, sizeof(edit_fields) / sizeof(edit_fields[0])); }
    bool FieldAffectsLayout(const String& field_id) const override
    {
        return field_id == "font_face" || field_id == "font_size" || field_id == "frame_width" || field_id == "radius";
    }
    Value ResolveFieldValue(const UiDesignerNode& node, const UiDesignerControlSpec& spec,
                            const String& field_id, const UiDesignerTransientOverlay* overlay) const override
    {
        UiBaseEdit::Style style = ResolveEditStyleBase(node);
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0 && !HasThemeValue(node, overlay, p.id)) continue;
            ApplyEditField(style, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value));
        }
        return EditFieldValue(style, field_id);
    }
    void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node,
                           const UiDesignerControlSpec& spec, const UiDesignerTransientOverlay* overlay) const override
    {
        UiBaseEdit* edit = dynamic_cast<UiBaseEdit *>(&ctrl);
        if(!edit) return;
        UiBaseEdit::Style style = ResolveEditStyleBase(node);
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0 && !HasThemeValue(node, overlay, p.id)) continue;
            authored = true;
            ApplyEditField(style, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value));
        }
        if(authored || ParseRole(node.GetProperty("role", "Standard")) != UiRole::Standard)
            edit->SetCustomStyle(style);
        else
            edit->ClearCustomStyle();
    }
    void EmitSetup(String& out, const String& member, const UiDesignerNode& node,
                   const UiDesignerControlSpec& spec) const override
    {
        const bool authored = [&] { for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) if(node.theme_overrides.Find(p.id) >= 0) return true; return false; }();
        const String role = AsString(node.GetProperty("role", "Standard"));
        if(!authored && role == "Standard") return;
        const String role_expr = role == "Subtle" ? "UiEditRole::Subtle" : role == "Accent" ? "UiEditRole::Strong" : role == "Alert" ? "UiEditRole::Strong" : "UiEditRole::Field";
        const String var = member + "_style";
        out << "\tUiBaseEdit::Style " << var << " = UiTheme::ResolveEdit(" << role_expr << ");\n";
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0) continue;
            const Value v = node.theme_overrides.GetValue(q);
            if(p.adapter_field_id == "font_face") out << "\t" << var << ".font.FaceName(" << CppString(AsString(v)) << ");\n";
            else if(p.adapter_field_id == "font_size") out << "\t" << var << ".font.Height(" << max(1, (int)v) << ");\n";
            else {
                String field;
                EmitEditField(field, var, p.adapter_field_id, v);
                out << "\t" << field;
            }
        }
        out << "\t" << member << ".SetCustomStyle(" << var << ");\n";
    }
};

static void ApplyTitleCardThemeField(UiTitleCard::Style& style,
                                     const String& field_id,
                                     const Value& value)
{
    if(field_id == "face_enabled") style.metrics.face_enabled = (bool)value;
    else if(field_id == "face_normal") style.palette.face[ST_NORMAL] = UiFill::Solid((Color)value);
    else if(field_id == "face_hot") style.palette.face[ST_HOT] = UiFill::Solid((Color)value);
    else if(field_id == "face_pressed") style.palette.face[ST_PRESSED] = UiFill::Solid((Color)value);
    else if(field_id == "face_disabled") style.palette.face[ST_DISABLED] = UiFill::Solid((Color)value);
    else if(field_id == "frame_enabled") style.metrics.frame_enabled = (bool)value;
    else if(field_id == "frame_normal") style.palette.frame[ST_NORMAL] = (Color)value;
    else if(field_id == "frame_hot") style.palette.frame[ST_HOT] = (Color)value;
    else if(field_id == "frame_pressed") style.palette.frame[ST_PRESSED] = (Color)value;
    else if(field_id == "frame_disabled") style.palette.frame[ST_DISABLED] = (Color)value;
    else if(field_id == "frame_width") style.metrics.frame_width = max(0, (int)value);
    else if(field_id == "radius") style.metrics.radius = max(0, (int)value);
    else if(field_id == "text_normal") style.palette.ink[ST_NORMAL] = (Color)value;
    else if(field_id == "text_disabled") style.palette.ink[ST_DISABLED] = (Color)value;
    else if(field_id == "title_color") style.title_color = (Color)value;
    else if(field_id == "subtitle_color") style.subtitle_color = (Color)value;
    else if(field_id == "copy_color") style.copy_color = (Color)value;
    else if(field_id == "title_font_face") style.title_font.FaceName(AsString(value));
    else if(field_id == "title_font_size") style.title_font.Height(max(1, (int)value));
    else if(field_id == "title_font_bold") style.title_font.Bold((bool)value);
    else if(field_id == "subtitle_font_face") style.subtitle_font.FaceName(AsString(value));
    else if(field_id == "subtitle_font_size") style.subtitle_font.Height(max(1, (int)value));
    else if(field_id == "subtitle_font_bold") style.subtitle_font.Bold((bool)value);
    else if(field_id == "copy_font_face") style.copy_font.FaceName(AsString(value));
    else if(field_id == "copy_font_size") style.copy_font.Height(max(1, (int)value));
    else if(field_id == "content_margin") style.metrics.content_margin = Rect(DPI(max(0, (int)value)), DPI(max(0, (int)value)), DPI(max(0, (int)value)), DPI(max(0, (int)value)));
    else if(field_id == "theme_title_subtitle_gap") style.title_subtitle_gap = DPI(max(0, (int)value));
    else if(field_id == "theme_subtitle_copy_gap") style.subtitle_copy_gap = DPI(max(0, (int)value));
    else if(field_id == "theme_media_reserve") style.media_reserve = DPI(max(0, (int)value));
    else if(field_id == "theme_media_min") style.media_min = DPI(max(0, (int)value));
    else if(field_id == "theme_media_gap") style.media_gap = DPI(max(0, (int)value));
    else if(field_id == "theme_media_auto_fit") style.media_auto_fit = (bool)value;
    else if(field_id == "theme_title_line") style.title_line = (bool)value;
    else if(field_id == "theme_title_line_thickness") style.title_line_thickness = DPI(max(0, (int)value));
    else if(field_id == "theme_title_line_style") style.title_line_style = AsString(value) == "Dashed" ? DASHED : AsString(value) == "Dotted" ? DOTTED : SOLID;
    else if(field_id == "theme_card_line") style.card_line = (bool)value;
    else if(field_id == "theme_card_line_side") style.card_line_side = AsString(value) == "Top" ? UiAlign::TOP : AsString(value) == "Bottom" ? UiAlign::BOTTOM : AsString(value) == "Right" ? UiAlign::RIGHT : UiAlign::LEFT;
    else if(field_id == "theme_card_line_thickness") style.card_line_thickness = DPI(max(0, (int)value));
    else if(field_id == "theme_card_line_gap") style.card_line_gap = DPI(max(0, (int)value));
    else if(field_id == "theme_transparent") style.transparent = (bool)value;
    else if(field_id == "theme_hover_enabled") style.hover_enabled = (bool)value;
}

static void AddTitleCardThemeOverrides(UiDesignerControlSpec& spec)
{
    const UiTitleCard::Style base = ResolveTitleCardThemeBase(UiRole::Standard);
    AddOverride(spec, "face_enabled", "Face enabled", "Surface", PropertyEditorKind::Boolean,
                base.metrics.face_enabled, PropertyImpactPaint | PropertyImpactCode, "face_enabled");
    AddOverride(spec, "face_normal", "Face normal", "Surface", PropertyEditorKind::Color,
                base.palette.face[ST_NORMAL].IsSolid() ? base.palette.face[ST_NORMAL].color : White(),
                PropertyImpactPaint | PropertyImpactCode, "face_normal");
    AddOverride(spec, "frame_enabled", "Frame enabled", "Surface", PropertyEditorKind::Boolean,
                base.metrics.frame_enabled, PropertyImpactPaint | PropertyImpactCode, "frame_enabled");
    AddOverride(spec, "frame_normal", "Frame normal", "Surface", PropertyEditorKind::Color,
                base.palette.frame[ST_NORMAL], PropertyImpactPaint | PropertyImpactCode, "frame_normal");
    AddOverride(spec, "frame_width", "Frame width", "Surface", PropertyEditorKind::Integer,
                base.metrics.frame_width, PropertyImpactPaint | PropertyImpactCode, "frame_width");
    AddOverride(spec, "radius", "Radius", "Surface", PropertyEditorKind::Integer,
                base.metrics.radius, PropertyImpactPaint | PropertyImpactCode, "radius");
    AddOverride(spec, "text_normal", "Text normal", "Ink", PropertyEditorKind::Color,
                base.palette.ink[ST_NORMAL], PropertyImpactPaint | PropertyImpactCode, "text_normal");
    AddOverride(spec, "text_disabled", "Text disabled", "Ink", PropertyEditorKind::Color,
                base.palette.ink[ST_DISABLED], PropertyImpactPaint | PropertyImpactCode, "text_disabled");
    AddOverride(spec, "title_color", "Title color", "Ink", PropertyEditorKind::Color,
                base.title_color, PropertyImpactPaint | PropertyImpactCode, "title_color");
    AddOverride(spec, "subtitle_color", "Subtitle color", "Ink", PropertyEditorKind::Color,
                base.subtitle_color, PropertyImpactPaint | PropertyImpactCode, "subtitle_color");
    AddOverride(spec, "copy_color", "Copy color", "Ink", PropertyEditorKind::Color,
                base.copy_color, PropertyImpactPaint | PropertyImpactCode, "copy_color");
    AddOverride(spec, "title_font_face", "Title font face", "Typography", PropertyEditorKind::Text,
                base.title_font.GetFaceName(), PropertyImpactPaint | PropertyImpactCode, "title_font_face");
    AddOverride(spec, "title_font_size", "Title font size", "Typography", PropertyEditorKind::Integer,
                base.title_font.GetHeight(), PropertyImpactPaint | PropertyImpactCode, "title_font_size");
    AddOverride(spec, "title_font_bold", "Title bold", "Typography", PropertyEditorKind::Boolean,
                base.title_font.IsBold(), PropertyImpactPaint | PropertyImpactCode, "title_font_bold");
    AddOverride(spec, "subtitle_font_face", "Subtitle font face", "Typography", PropertyEditorKind::Text,
                base.subtitle_font.GetFaceName(), PropertyImpactPaint | PropertyImpactCode, "subtitle_font_face");
    AddOverride(spec, "subtitle_font_size", "Subtitle font size", "Typography", PropertyEditorKind::Integer,
                base.subtitle_font.GetHeight(), PropertyImpactPaint | PropertyImpactCode, "subtitle_font_size");
    AddOverride(spec, "subtitle_font_bold", "Subtitle bold", "Typography", PropertyEditorKind::Boolean,
                base.subtitle_font.IsBold(), PropertyImpactPaint | PropertyImpactCode, "subtitle_font_bold");
    AddOverride(spec, "copy_font_face", "Copy font face", "Typography", PropertyEditorKind::Text,
                base.copy_font.GetFaceName(), PropertyImpactPaint | PropertyImpactCode, "copy_font_face");
    AddOverride(spec, "copy_font_size", "Copy font size", "Typography", PropertyEditorKind::Integer,
                base.copy_font.GetHeight(), PropertyImpactPaint | PropertyImpactCode, "copy_font_size");
    AddOverride(spec, "content_margin", "Content inset", "Layout", PropertyEditorKind::Integer,
                base.metrics.content_margin.left, PropertyImpactPaint | PropertyImpactCode, "content_margin");
    AddOverride(spec, "theme_title_subtitle_gap", "Title/subtitle gap", "Layout", PropertyEditorKind::Integer,
                base.title_subtitle_gap / DPI(1), PropertyImpactPaint | PropertyImpactCode, "theme_title_subtitle_gap");
    AddOverride(spec, "theme_subtitle_copy_gap", "Subtitle/copy gap", "Layout", PropertyEditorKind::Integer,
                base.subtitle_copy_gap / DPI(1), PropertyImpactPaint | PropertyImpactCode, "theme_subtitle_copy_gap");
    AddOverride(spec, "theme_media_reserve", "Media reserve", "Layout", PropertyEditorKind::Integer,
                base.media_reserve / DPI(1), PropertyImpactPaint | PropertyImpactCode, "theme_media_reserve");
    AddOverride(spec, "theme_media_min", "Media minimum", "Layout", PropertyEditorKind::Integer,
                base.media_min / DPI(1), PropertyImpactPaint | PropertyImpactCode, "theme_media_min");
    AddOverride(spec, "theme_media_gap", "Media gap", "Layout", PropertyEditorKind::Integer,
                base.media_gap / DPI(1), PropertyImpactPaint | PropertyImpactCode, "theme_media_gap");
    AddOverride(spec, "theme_media_auto_fit", "Media auto-fit", "Layout", PropertyEditorKind::Boolean,
                base.media_auto_fit, PropertyImpactPaint | PropertyImpactCode, "theme_media_auto_fit");
    AddOverride(spec, "theme_title_line", "Title line", "Layout", PropertyEditorKind::Boolean,
                base.title_line, PropertyImpactPaint | PropertyImpactCode, "theme_title_line");
    AddOverride(spec, "theme_title_line_thickness", "Title line thickness", "Layout", PropertyEditorKind::Integer,
                base.title_line_thickness / DPI(1), PropertyImpactPaint | PropertyImpactCode, "theme_title_line_thickness");
    AddOverride(spec, "theme_title_line_style", "Title line style", "Layout", PropertyEditorKind::Choice,
                String("Solid"), PropertyImpactPaint | PropertyImpactCode, "theme_title_line_style");
    spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Solid", "Solid"));
    spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Dashed", "Dashed"));
    spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Dotted", "Dotted"));
    AddOverride(spec, "theme_card_line", "Card line", "Layout", PropertyEditorKind::Boolean,
                base.card_line, PropertyImpactPaint | PropertyImpactCode, "theme_card_line");
    AddOverride(spec, "theme_card_line_side", "Card line side", "Layout", PropertyEditorKind::Choice,
                String("Bottom"), PropertyImpactPaint | PropertyImpactCode, "theme_card_line_side");
    spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Left", "Left"));
    spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Right", "Right"));
    spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Top", "Top"));
    spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Bottom", "Bottom"));
    AddOverride(spec, "theme_card_line_thickness", "Card line thickness", "Layout", PropertyEditorKind::Integer,
                base.card_line_thickness / DPI(1), PropertyImpactPaint | PropertyImpactCode, "theme_card_line_thickness");
    AddOverride(spec, "theme_card_line_gap", "Card line gap", "Layout", PropertyEditorKind::Integer,
                base.card_line_gap / DPI(1), PropertyImpactPaint | PropertyImpactCode, "theme_card_line_gap");
    AddOverride(spec, "theme_transparent", "Transparent", "Surface", PropertyEditorKind::Boolean,
                base.transparent, PropertyImpactPaint | PropertyImpactCode, "theme_transparent");
    AddOverride(spec, "theme_hover_enabled", "Hover enabled", "Surface", PropertyEditorKind::Boolean,
                base.hover_enabled, PropertyImpactPaint | PropertyImpactCode, "theme_hover_enabled");
}

class TitleCardThemeAdapter final : public UiDesignerThemeAdapter {
public:
    const char *Id() const override { return "title_card"; }
    bool Supports(UiDesignerRuntimeKind kind) const override { return kind == UiDesignerRuntimeKind::UiTitleCard; }
    void AddThemeOverrides(UiDesignerControlSpec& spec) const override { AddTitleCardThemeOverrides(spec); }
    bool HasField(const String& field_id) const override
    {
        static const char *fields[] = {
            "face_enabled", "face_normal", "frame_enabled", "frame_normal", "frame_width", "radius",
            "text_normal", "text_disabled", "title_color", "subtitle_color", "copy_color",
            "title_font_face", "title_font_size", "title_font_bold",
            "subtitle_font_face", "subtitle_font_size", "subtitle_font_bold",
            "copy_font_face", "copy_font_size", "content_margin", "theme_title_subtitle_gap",
            "theme_subtitle_copy_gap", "theme_media_reserve", "theme_media_min", "theme_media_gap", "theme_media_auto_fit",
            "theme_title_line", "theme_title_line_thickness", "theme_title_line_style", "theme_card_line", "theme_card_line_side",
            "theme_card_line_thickness", "theme_card_line_gap", "theme_transparent", "theme_hover_enabled"
        };
        for(const char *field : fields) if(field_id == field) return true;
        return false;
    }
    bool FieldAffectsLayout(const String& field_id) const override
    {
        return field_id != "face_normal" && field_id != "frame_normal" &&
               field_id != "text_normal" && field_id != "text_disabled" &&
               field_id != "title_color" && field_id != "subtitle_color" &&
               field_id != "copy_color";
    }
    Value ResolveFieldValue(const UiDesignerNode& node, const UiDesignerControlSpec& spec,
                            const String& field_id, const UiDesignerTransientOverlay* overlay) const override
    {
        UiTitleCard::Style style = ResolveTitleCardThemeBase(ParseRole(node.GetProperty("role", "Standard")));
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0 && !HasThemeValue(node, overlay, p.id))
                continue;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value;
            ApplyTitleCardThemeField(style, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, canonical));
        }
        if(field_id == "face_enabled") return style.metrics.face_enabled;
        if(field_id == "face_normal") return UiDesignerFillColor(style.palette.face[ST_NORMAL]);
        if(field_id == "frame_enabled") return style.metrics.frame_enabled;
        if(field_id == "frame_normal") return style.palette.frame[ST_NORMAL];
        if(field_id == "frame_width") return style.metrics.frame_width;
        if(field_id == "radius") return style.metrics.radius;
        if(field_id == "text_normal") return style.palette.ink[ST_NORMAL];
        if(field_id == "text_disabled") return style.palette.ink[ST_DISABLED];
        if(field_id == "title_color") return style.title_color;
        if(field_id == "subtitle_color") return style.subtitle_color;
        if(field_id == "copy_color") return style.copy_color;
        if(field_id == "title_font_face") return style.title_font.GetFaceName();
        if(field_id == "title_font_size") return style.title_font.GetHeight();
        if(field_id == "title_font_bold") return style.title_font.IsBold();
        if(field_id == "subtitle_font_face") return style.subtitle_font.GetFaceName();
        if(field_id == "subtitle_font_size") return style.subtitle_font.GetHeight();
        if(field_id == "subtitle_font_bold") return style.subtitle_font.IsBold();
        if(field_id == "copy_font_face") return style.copy_font.GetFaceName();
        if(field_id == "copy_font_size") return style.copy_font.GetHeight();
        if(field_id == "content_margin") return style.metrics.content_margin.left;
        if(field_id == "theme_title_subtitle_gap") return style.title_subtitle_gap / DPI(1);
        if(field_id == "theme_subtitle_copy_gap") return style.subtitle_copy_gap / DPI(1);
        if(field_id == "theme_media_reserve") return style.media_reserve / DPI(1);
        if(field_id == "theme_media_min") return style.media_min / DPI(1);
        if(field_id == "theme_media_gap") return style.media_gap / DPI(1);
        if(field_id == "theme_media_auto_fit") return style.media_auto_fit;
        if(field_id == "theme_title_line") return style.title_line;
        if(field_id == "theme_title_line_thickness") return style.title_line_thickness / DPI(1);
        if(field_id == "theme_title_line_style") return style.title_line_style == DASHED ? "Dashed" : style.title_line_style == DOTTED ? "Dotted" : "Solid";
        if(field_id == "theme_card_line") return style.card_line;
        if(field_id == "theme_card_line_side") return style.card_line_side == UiAlign::TOP ? "Top" : style.card_line_side == UiAlign::BOTTOM ? "Bottom" : style.card_line_side == UiAlign::RIGHT ? "Right" : "Left";
        if(field_id == "theme_card_line_thickness") return style.card_line_thickness / DPI(1);
        if(field_id == "theme_card_line_gap") return style.card_line_gap / DPI(1);
        if(field_id == "theme_transparent") return style.transparent;
        if(field_id == "theme_hover_enabled") return style.hover_enabled;
        return Value();
    }
    void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node, const UiDesignerControlSpec& spec, const UiDesignerTransientOverlay* overlay) const override
    {
        UiTitleCard *card = dynamic_cast<UiTitleCard *>(&ctrl);
        if(!card)
            return;
        UiTitleCard::Style style = ResolveTitleCardThemeBase(ParseRole(node.GetProperty("role", "Standard")));
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0 && !HasThemeValue(node, overlay, p.id))
                continue;
            authored = true;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value;
            ApplyTitleCardThemeField(style, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, canonical));
        }
        if(authored) card->SetCustomStyle(style); else card->ClearCustomStyle();
    }
    void EmitSetup(String& out, const String& member, const UiDesignerNode& node, const UiDesignerControlSpec& spec) const override
    {
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides)
            authored |= node.theme_overrides.Find(p.id) >= 0;
        if(!authored)
            return;
        out << "\tUiTitleCard::Style " << member << "_style = UiTheme::ResolveTitleCard("
            << EmitRoleExpr(AsString(node.GetProperty("role", "Standard"))) << ");\n";
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0)
                continue;
            const Value v = node.theme_overrides.GetValue(q);
            if(p.adapter_field_id == "face_enabled") out << "\t" << member << "_style.metrics.face_enabled = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "face_normal") out << "\t" << member << "_style.palette.face[ST_NORMAL] = UiFill::Solid(" << EmitValue(v) << ");\n";
            else if(p.adapter_field_id == "frame_enabled") out << "\t" << member << "_style.metrics.frame_enabled = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "frame_normal") out << "\t" << member << "_style.palette.frame[ST_NORMAL] = " << EmitValue(v) << ";\n";
            else if(p.adapter_field_id == "frame_width") out << "\t" << member << "_style.metrics.frame_width = " << (int)v << ";\n";
            else if(p.adapter_field_id == "radius") out << "\t" << member << "_style.metrics.radius = " << (int)v << ";\n";
            else if(p.adapter_field_id == "text_normal") out << "\t" << member << "_style.palette.ink[ST_NORMAL] = " << EmitValue(v) << ";\n";
            else if(p.adapter_field_id == "text_disabled") out << "\t" << member << "_style.palette.ink[ST_DISABLED] = " << EmitValue(v) << ";\n";
            else if(p.adapter_field_id == "title_color") out << "\t" << member << "_style.title_color = " << EmitValue(v) << ";\n";
            else if(p.adapter_field_id == "subtitle_color") out << "\t" << member << "_style.subtitle_color = " << EmitValue(v) << ";\n";
            else if(p.adapter_field_id == "copy_color") out << "\t" << member << "_style.copy_color = " << EmitValue(v) << ";\n";
            else if(p.adapter_field_id == "title_font_face") out << "\t" << member << "_style.title_font.FaceName(" << EmitValue(v) << ");\n";
            else if(p.adapter_field_id == "title_font_size") out << "\t" << member << "_style.title_font.Height(" << (int)v << ");\n";
            else if(p.adapter_field_id == "title_font_bold") out << "\t" << member << "_style.title_font.Bold(" << AsString((bool)v) << ");\n";
            else if(p.adapter_field_id == "subtitle_font_face") out << "\t" << member << "_style.subtitle_font.FaceName(" << EmitValue(v) << ");\n";
            else if(p.adapter_field_id == "subtitle_font_size") out << "\t" << member << "_style.subtitle_font.Height(" << (int)v << ");\n";
            else if(p.adapter_field_id == "subtitle_font_bold") out << "\t" << member << "_style.subtitle_font.Bold(" << AsString((bool)v) << ");\n";
            else if(p.adapter_field_id == "copy_font_face") out << "\t" << member << "_style.copy_font.FaceName(" << EmitValue(v) << ");\n";
            else if(p.adapter_field_id == "copy_font_size") out << "\t" << member << "_style.copy_font.Height(" << (int)v << ");\n";
            else if(p.adapter_field_id == "content_margin") out << "\t" << member << "_style.metrics.content_margin = Rect(DPI(" << (int)v << "), DPI(" << (int)v << "), DPI(" << (int)v << "), DPI(" << (int)v << "));\n";
            else if(p.adapter_field_id == "theme_title_subtitle_gap") out << "\t" << member << "_style.title_subtitle_gap = DPI(" << (int)v << ");\n";
            else if(p.adapter_field_id == "theme_subtitle_copy_gap") out << "\t" << member << "_style.subtitle_copy_gap = DPI(" << (int)v << ");\n";
            else if(p.adapter_field_id == "theme_media_reserve") out << "\t" << member << "_style.media_reserve = DPI(" << (int)v << ");\n";
            else if(p.adapter_field_id == "theme_media_min") out << "\t" << member << "_style.media_min = DPI(" << (int)v << ");\n";
            else if(p.adapter_field_id == "theme_media_gap") out << "\t" << member << "_style.media_gap = DPI(" << (int)v << ");\n";
            else if(p.adapter_field_id == "theme_media_auto_fit") out << "\t" << member << "_style.media_auto_fit = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "theme_title_line") out << "\t" << member << "_style.title_line = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "theme_title_line_thickness") out << "\t" << member << "_style.title_line_thickness = DPI(" << (int)v << ");\n";
            else if(p.adapter_field_id == "theme_title_line_style") out << "\t" << member << "_style.title_line_style = " << (AsString(v) == "Dashed" ? "DASHED" : AsString(v) == "Dotted" ? "DOTTED" : "SOLID") << ";\n";
            else if(p.adapter_field_id == "theme_card_line") out << "\t" << member << "_style.card_line = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "theme_card_line_side") out << "\t" << member << "_style.card_line_side = " << EmitAlign(AsString(v)) << ";\n";
            else if(p.adapter_field_id == "theme_card_line_thickness") out << "\t" << member << "_style.card_line_thickness = DPI(" << (int)v << ");\n";
            else if(p.adapter_field_id == "theme_card_line_gap") out << "\t" << member << "_style.card_line_gap = DPI(" << (int)v << ");\n";
            else if(p.adapter_field_id == "theme_transparent") out << "\t" << member << "_style.transparent = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "theme_hover_enabled") out << "\t" << member << "_style.hover_enabled = " << AsString((bool)v) << ";\n";
        }
        out << "\t" << member << ".SetCustomStyle(" << member << "_style);\n";
    }
};

struct PanelThemeValues {
    UiDesignerSurfaceKind face_surface = UiDesignerSurfaceKind::UseTheme;
    Color face[4];
    UiDesignerSurfaceKind frame_surface = UiDesignerSurfaceKind::UseTheme;
    Color frame[4];
    int frame_width;
    int radius;
    bool transparent;
};

static PanelThemeValues ResolvePanelThemeValues(UiDesignerRuntimeKind kind, UiRole role)
{
    PanelThemeValues v;
    if(kind == UiDesignerRuntimeKind::UiGroupPanel) {
        UiGroupPanel::Style s = UiTheme::ResolveGroupPanel(role);
        for(int i = 0; i < 4; i++) { v.face[i] = UiDesignerFillColor(s.palette.face[i]); v.frame[i] = s.palette.frame[i]; }
        v.frame_width = s.metrics.frame_width; v.radius = s.metrics.radius; v.transparent = s.transparent;
    }
    else if(kind == UiDesignerRuntimeKind::UiScrollPanel) {
        UiScrollPanel::Style s = UiTheme::ResolveScrollPanel(role);
        for(int i = 0; i < 4; i++) { v.face[i] = UiDesignerFillColor(s.palette.face[i]); v.frame[i] = s.palette.frame[i]; }
        v.frame_width = s.metrics.frame_width; v.radius = s.metrics.radius; v.transparent = s.transparent;
    }
    else {
        UiPanel::Style s = UiTheme::ResolvePanel(role);
        for(int i = 0; i < 4; i++) { v.face[i] = UiDesignerFillColor(s.palette.face[i]); v.frame[i] = s.palette.frame[i]; }
        v.frame_width = s.metrics.frame_width; v.radius = s.metrics.radius; v.transparent = s.transparent;
    }
    return v;
}

static void ApplyPanelThemeField(UiPanel::Style& s, const String& id, const Value& v)
{
    if(id == "face_surface") {
        const UiDesignerSurfaceKind kind = UiDesignerParseSurfaceKind(v);
        if(kind == UiDesignerSurfaceKind::None) s.metrics.face_enabled = false;
        else if(kind == UiDesignerSurfaceKind::Solid) s.metrics.face_enabled = true;
    }
    else if(id == "frame_surface") {
        const UiDesignerSurfaceKind kind = UiDesignerParseSurfaceKind(v);
        if(kind == UiDesignerSurfaceKind::None) s.metrics.frame_enabled = false;
        else if(kind == UiDesignerSurfaceKind::Solid || kind == UiDesignerSurfaceKind::Dashed) s.metrics.frame_enabled = true;
    }
    else if(id == "face_colors" || id == "frame_colors") {
        if(!v.Is<ValueArray>()) return;
        ValueArray colors = v;
        for(int i = 0; i < min(4, colors.GetCount()); i++)
            if(colors[i].Is<Color>()) {
                if(id == "face_colors") s.palette.face[i] = UiFill::Solid((Color)colors[i]);
                else s.palette.frame[i] = (Color)colors[i];
            }
    }
    else if(id == "frame_width") s.metrics.frame_width = max(0, (int)v);
    else if(id == "radius") s.metrics.radius = max(0, (int)v);
    else if(id == "transparent") s.transparent = (bool)v;
}

static void ApplyPanelThemeField(UiGroupPanel::Style& s, const String& id, const Value& v)
{
    if(id == "face_surface") {
        const UiDesignerSurfaceKind kind = UiDesignerParseSurfaceKind(v);
        if(kind == UiDesignerSurfaceKind::None) s.metrics.face_enabled = false;
        else if(kind == UiDesignerSurfaceKind::Solid) s.metrics.face_enabled = true;
    }
    else if(id == "frame_surface") {
        const UiDesignerSurfaceKind kind = UiDesignerParseSurfaceKind(v);
        if(kind == UiDesignerSurfaceKind::None) s.metrics.frame_enabled = false;
        else if(kind == UiDesignerSurfaceKind::Solid || kind == UiDesignerSurfaceKind::Dashed) s.metrics.frame_enabled = true;
    }
    else if(id == "face_colors" || id == "frame_colors") {
        if(!v.Is<ValueArray>()) return;
        ValueArray colors = v;
        for(int i = 0; i < min(4, colors.GetCount()); i++)
            if(colors[i].Is<Color>()) {
                if(id == "face_colors") s.palette.face[i] = UiFill::Solid((Color)colors[i]);
                else s.palette.frame[i] = (Color)colors[i];
            }
    }
    else if(id == "frame_width") s.metrics.frame_width = max(0, (int)v);
    else if(id == "radius") s.metrics.radius = max(0, (int)v);
    else if(id == "transparent") s.transparent = (bool)v;
}

static void ApplyPanelThemeField(UiScrollPanel::Style& s, const String& id, const Value& v)
{
    if(id == "face_surface") {
        const UiDesignerSurfaceKind kind = UiDesignerParseSurfaceKind(v);
        if(kind == UiDesignerSurfaceKind::None) s.metrics.face_enabled = false;
        else if(kind == UiDesignerSurfaceKind::Solid) s.metrics.face_enabled = true;
    }
    else if(id == "frame_surface") {
        const UiDesignerSurfaceKind kind = UiDesignerParseSurfaceKind(v);
        if(kind == UiDesignerSurfaceKind::None) s.metrics.frame_enabled = false;
        else if(kind == UiDesignerSurfaceKind::Solid || kind == UiDesignerSurfaceKind::Dashed) s.metrics.frame_enabled = true;
    }
    else if(id == "face_colors" || id == "frame_colors") {
        if(!v.Is<ValueArray>()) return;
        ValueArray colors = v;
        for(int i = 0; i < min(4, colors.GetCount()); i++)
            if(colors[i].Is<Color>()) {
                if(id == "face_colors") s.palette.face[i] = UiFill::Solid((Color)colors[i]);
                else s.palette.frame[i] = (Color)colors[i];
            }
    }
    else if(id == "frame_width") s.metrics.frame_width = max(0, (int)v);
    else if(id == "radius") s.metrics.radius = max(0, (int)v);
    else if(id == "transparent") s.transparent = (bool)v;
}

class PanelThemeAdapter final : public UiDesignerThemeAdapter {
public:
    explicit PanelThemeAdapter(const char *id, UiDesignerRuntimeKind kind) : id_(id), kind_(kind) {}
    const char *Id() const override { return id_; }
    bool Supports(UiDesignerRuntimeKind kind) const override { return kind == kind_; }
    void AddThemeOverrides(UiDesignerControlSpec& spec) const override
    {
        PanelThemeValues b = ResolvePanelThemeValues(kind_, UiRole::Standard);
        AddOverride(spec, "face_surface", "Face", "Surface", PropertyEditorKind::Choice, String("UseTheme"), PropertyImpactPaint | PropertyImpactCode, "face_surface", "Use theme inherits the active theme; None explicitly removes the face.");
        UiDesignerAddSurfaceChoices(spec.theme_overrides.Top());
        ValueArray face_colors;
        for(int i = 0; i < 4; i++) face_colors.Add(b.face[i]);
        AddOverride(spec, "face_colors", "Face colours", "Face", PropertyEditorKind::ColorPalette,
                    face_colors, PropertyImpactPaint | PropertyImpactCode, "face_colors");
        spec.theme_overrides.Top().ColorCount(4).VisibleWhen("face_surface", "Solid");
        AddOverride(spec, "frame_surface", "Frame", "Surface", PropertyEditorKind::Choice, String("UseTheme"), PropertyImpactPaint | PropertyImpactCode, "frame_surface", "Use theme inherits the active theme; None explicitly removes the frame.");
        UiDesignerAddSurfaceChoices(spec.theme_overrides.Top(), true);
        ValueArray frame_colors;
        for(int i = 0; i < 4; i++) frame_colors.Add(b.frame[i]);
        AddOverride(spec, "frame_colors", "Frame colours", "Frame", PropertyEditorKind::ColorPalette,
                    frame_colors, PropertyImpactPaint | PropertyImpactCode, "frame_colors");
        spec.theme_overrides.Top().ColorCount(4).VisibleWhen("frame_surface", "Solid");
        AddOverride(spec, "frame_width", "Frame width", "Surface", PropertyEditorKind::Integer, b.frame_width, PropertyImpactPaint | PropertyImpactCode, "frame_width");
        AddOverride(spec, "radius", "Radius", "Surface", PropertyEditorKind::Integer, b.radius, PropertyImpactPaint | PropertyImpactCode, "radius");
        AddOverride(spec, "transparent", "Transparent", "Surface", PropertyEditorKind::Boolean, b.transparent, PropertyImpactPaint | PropertyImpactCode, "transparent");
    }
    bool HasField(const String& id) const override { return id == "face_surface" || id == "frame_surface" || id == "face_colors" || id == "frame_colors" || id == "frame_width" || id == "radius" || id == "transparent"; }
    bool FieldAffectsLayout(const String& id) const override { return id == "face_surface" || id == "frame_surface" || id == "frame_width" || id == "radius"; }
    Value ResolveFieldValue(const UiDesignerNode& node, const UiDesignerControlSpec& spec, const String& id, const UiDesignerTransientOverlay* overlay) const override
    {
        PanelThemeValues v = ResolvePanelThemeValues(kind_, ParseRole(node.GetProperty("role", "Standard")));
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0 && !HasThemeValue(node, overlay, p.id)) continue;
            Value x = ResolveThemeValue(node, overlay, p.id, q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value);
            if(p.adapter_field_id == "face_surface") v.face_surface = UiDesignerParseSurfaceKind(x);
            else if(p.adapter_field_id == "frame_surface") v.frame_surface = UiDesignerParseSurfaceKind(x);
            else if(p.adapter_field_id == "face_colors" && x.Is<ValueArray>()) {
                ValueArray colors = x;
                for(int i = 0; i < min(4, colors.GetCount()); i++)
                    if(colors[i].Is<Color>()) v.face[i] = (Color)colors[i];
            }
            else if(p.adapter_field_id == "frame_colors" && x.Is<ValueArray>()) {
                ValueArray colors = x;
                for(int i = 0; i < min(4, colors.GetCount()); i++)
                    if(colors[i].Is<Color>()) v.frame[i] = (Color)colors[i];
            }
            else if(p.adapter_field_id == "frame_width") v.frame_width = (int)x;
            else if(p.adapter_field_id == "radius") v.radius = (int)x;
            else if(p.adapter_field_id == "transparent") v.transparent = (bool)x;
        }
        if(id == "face_surface") return UiDesignerSurfaceKindName(v.face_surface);
        if(id == "frame_surface") return UiDesignerSurfaceKindName(v.frame_surface);
        if(id == "face_colors") {
            ValueArray colors;
            for(int i = 0; i < 4; i++) colors.Add(v.face[i]);
            return colors;
        }
        if(id == "frame_colors") {
            ValueArray colors;
            for(int i = 0; i < 4; i++) colors.Add(v.frame[i]);
            return colors;
        }
        if(id == "frame_width") return v.frame_width; if(id == "radius") return v.radius;
        if(id == "transparent") return v.transparent; return Value();
    }
    void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node, const UiDesignerControlSpec& spec, const UiDesignerTransientOverlay* overlay) const override
    {
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) authored |= HasThemeValue(node, overlay, p.id);
        const UiRole role = ParseRole(node.GetProperty("role", "Standard"));
        if(kind_ == UiDesignerRuntimeKind::UiGroupPanel) {
            UiGroupPanel *c = dynamic_cast<UiGroupPanel *>(&ctrl); if(!c) return;
            UiGroupPanel::Style s = UiTheme::ResolveGroupPanel(role);
            for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) { const int q = node.theme_overrides.Find(p.id); if(q >= 0 || HasThemeValue(node, overlay, p.id)) ApplyPanelThemeField(s, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value)); }
            if(authored || role != UiRole::Standard) c->SetCustomStyle(s); else c->ClearCustomStyle();
        }
        else if(kind_ == UiDesignerRuntimeKind::UiScrollPanel) {
            UiScrollPanel *c = dynamic_cast<UiScrollPanel *>(&ctrl); if(!c) return;
            UiScrollPanel::Style s = UiTheme::ResolveScrollPanel(role);
            for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) { const int q = node.theme_overrides.Find(p.id); if(q >= 0 || HasThemeValue(node, overlay, p.id)) ApplyPanelThemeField(s, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value)); }
            if(authored || role != UiRole::Standard) c->SetCustomStyle(s); else c->ClearCustomStyle();
        }
        else {
            UiPanel *c = dynamic_cast<UiPanel *>(&ctrl); if(!c) return;
            UiPanel::Style s = UiTheme::ResolvePanel(role);
            for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) { const int q = node.theme_overrides.Find(p.id); if(q >= 0 || HasThemeValue(node, overlay, p.id)) ApplyPanelThemeField(s, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value)); }
            if(authored || role != UiRole::Standard) c->SetCustomStyle(s); else c->ClearCustomStyle();
        }
    }
    void EmitSetup(String& out, const String& member, const UiDesignerNode& node, const UiDesignerControlSpec& spec) const override
    {
        bool authored = false; for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) authored |= node.theme_overrides.Find(p.id) >= 0;
        if(!authored) return;
        const char *resolver = kind_ == UiDesignerRuntimeKind::UiGroupPanel ? "UiTheme::ResolveGroupPanel(UiRole::Standard)" : kind_ == UiDesignerRuntimeKind::UiScrollPanel ? "UiTheme::ResolveScrollPanel(UiRole::Standard)" : "UiTheme::ResolvePanel(UiRole::Standard)";
        out << "\t" << node.type << "::Style " << member << "_style = " << resolver << ";\n";
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) { const int q = node.theme_overrides.Find(p.id); if(q < 0) continue; const Value v = node.theme_overrides.GetValue(q); const String f = p.adapter_field_id;
            if(f == "face_surface" && AsString(v) == "None") out << "\t" << member << "_style.metrics.face_enabled = false;\n";
            else if(f == "face_surface" && AsString(v) == "Solid") out << "\t" << member << "_style.metrics.face_enabled = true;\n";
            else if(f == "frame_surface" && AsString(v) == "None") out << "\t" << member << "_style.metrics.frame_enabled = false;\n";
            else if(f == "frame_surface" && (AsString(v) == "Solid" || AsString(v) == "Dashed")) out << "\t" << member << "_style.metrics.frame_enabled = true;\n";
            else if(f == "face_colors" && v.Is<ValueArray>()) {
                ValueArray colors = v;
                static const char *states[] = {"ST_NORMAL", "ST_HOT", "ST_PRESSED", "ST_DISABLED"};
                for(int i = 0; i < min(4, colors.GetCount()); i++)
                    if(colors[i].Is<Color>())
                        out << "\t" << member << "_style.palette.face[" << states[i] << "] = UiFill::Solid(" << EmitValue(colors[i]) << ");\n";
            }
            else if(f == "frame_colors" && v.Is<ValueArray>()) {
                ValueArray colors = v;
                static const char *states[] = {"ST_NORMAL", "ST_HOT", "ST_PRESSED", "ST_DISABLED"};
                for(int i = 0; i < min(4, colors.GetCount()); i++)
                    if(colors[i].Is<Color>())
                        out << "\t" << member << "_style.palette.frame[" << states[i] << "] = " << EmitValue(colors[i]) << ";\n";
            }
            else if(f == "frame_width") out << "\t" << member << "_style.metrics.frame_width = DPI(" << (int)v << ");\n";
            else if(f == "radius") out << "\t" << member << "_style.metrics.radius = DPI(" << (int)v << ");\n";
            else if(f == "transparent") out << "\t" << member << "_style.transparent = " << AsString((bool)v) << ";\n";
        }
        out << "\t" << member << ".SetCustomStyle(" << member << "_style);\n";
    }
private:
    const char *id_; UiDesignerRuntimeKind kind_;
};

struct BasicThemeValues {
    Color face;
    Color frame;
    Color ink;
    bool face_enabled;
    bool frame_enabled;
    int frame_width;
};

static BasicThemeValues ResolveBasicThemeValues(UiDesignerRuntimeKind kind, UiRole role)
{
    BasicThemeValues v;
    if(kind == UiDesignerRuntimeKind::UiLabel) {
        UiLabel::Style s = UiTheme::ResolveLabel(role); v.face = UiDesignerFillColor(s.palette.face[ST_NORMAL]); v.frame = s.palette.frame[ST_NORMAL]; v.ink = s.palette.ink[ST_NORMAL]; v.face_enabled = s.metrics.face_enabled; v.frame_enabled = s.metrics.frame_enabled; v.frame_width = s.metrics.frame_width;
    }
    else if(kind == UiDesignerRuntimeKind::UiCheckBox) {
        UiCheckBox::Style s = UiTheme::ResolveCheckBox(role, UICHECKVIS_CLASSIC); v.face = UiDesignerFillColor(s.palette.face[ST_NORMAL]); v.frame = s.palette.frame[ST_NORMAL]; v.ink = s.palette.ink[ST_NORMAL]; v.face_enabled = s.metrics.face_enabled; v.frame_enabled = s.metrics.frame_enabled; v.frame_width = s.metrics.frame_width;
    }
    else if(kind == UiDesignerRuntimeKind::UiRadioButton) {
        UiRadioButton::Style s = UiTheme::ResolveRadioButton(role, UIRADIOVIS_CLASSIC); v.face = UiDesignerFillColor(s.palette.face[ST_NORMAL]); v.frame = s.palette.frame[ST_NORMAL]; v.ink = s.palette.ink[ST_NORMAL]; v.face_enabled = s.metrics.face_enabled; v.frame_enabled = s.metrics.frame_enabled; v.frame_width = s.metrics.frame_width;
    }
    else if(kind == UiDesignerRuntimeKind::UiToggle) {
        UiToggle::Style s = UiTheme::ResolveToggle(); v.face = UiDesignerFillColor(s.palette.face[ST_NORMAL]); v.frame = s.palette.frame[ST_NORMAL]; v.ink = s.palette.ink[ST_NORMAL]; v.face_enabled = s.metrics.face_enabled; v.frame_enabled = s.metrics.frame_enabled; v.frame_width = s.metrics.frame_width;
    }
    else if(kind == UiDesignerRuntimeKind::UiProgressBar) {
        UiProgressBar::Style s = UiTheme::ResolveProgressBar(role); v.face = UiDesignerFillColor(s.track_palette.face[ST_NORMAL]); v.frame = s.track_palette.frame[ST_NORMAL]; v.ink = s.fill_palette.ink[ST_NORMAL]; v.face_enabled = s.track_metrics.face_enabled; v.frame_enabled = s.track_metrics.frame_enabled; v.frame_width = s.track_metrics.frame_width;
    }
    else if(kind == UiDesignerRuntimeKind::UiSlider) {
        UiSlider::Style s = UiTheme::ResolveSlider(); v.face = UiDesignerFillColor(s.track_palette.face[ST_NORMAL]); v.frame = s.track_palette.frame[ST_NORMAL]; v.ink = UiDesignerFillColor(s.thumb_palette.face[ST_NORMAL]); v.face_enabled = s.track_metrics.face_enabled; v.frame_enabled = s.track_metrics.frame_enabled; v.frame_width = s.track_metrics.frame_width;
    }
    else if(kind == UiDesignerRuntimeKind::UiScrollBar) {
        UiScrollBar::Style s = UiTheme::ResolveScrollBar(); v.face = UiDesignerFillColor(s.track_palette.face[ST_NORMAL]); v.frame = s.track_palette.frame[ST_NORMAL]; v.ink = UiDesignerFillColor(s.thumb_palette.face[ST_NORMAL]); v.face_enabled = s.track_metrics.face_enabled; v.frame_enabled = s.track_metrics.frame_enabled; v.frame_width = s.track_metrics.frame_width;
    }
    else {
        UiDropdown::Style s = UiTheme::ResolveDropdown(role); v.face = UiDesignerFillColor(s.palette.face[ST_NORMAL]); v.frame = s.palette.frame[ST_NORMAL]; v.ink = s.palette.ink[ST_NORMAL]; v.face_enabled = s.metrics.face_enabled; v.frame_enabled = s.metrics.frame_enabled; v.frame_width = s.metrics.frame_width;
    }
    return v;
}

static void ApplyBasicThemeField(BasicThemeValues& v, const String& id, const Value& x)
{
    if(id == "face_normal") v.face = (Color)x;
    else if(id == "frame_normal") v.frame = (Color)x;
    else if(id == "ink_normal") v.ink = (Color)x;
    else if(id == "face_enabled") v.face_enabled = (bool)x;
    else if(id == "frame_enabled") v.frame_enabled = (bool)x;
    else if(id == "frame_width") v.frame_width = max(0, (int)x);
}

static void ApplyBasicStyle(Ctrl& ctrl, UiDesignerRuntimeKind kind, const BasicThemeValues& v, bool custom)
{
    if(kind == UiDesignerRuntimeKind::UiLabel) { auto *c = dynamic_cast<UiLabel*>(&ctrl); if(!c) return; auto s = c->GetStyle(); s.palette.face[ST_NORMAL] = UiFill::Solid(v.face); s.palette.frame[ST_NORMAL] = v.frame; s.palette.ink[ST_NORMAL] = v.ink; s.metrics.face_enabled = v.face_enabled; s.metrics.frame_enabled = v.frame_enabled; s.metrics.frame_width = v.frame_width; if(custom) c->SetCustomStyle(s); else c->ClearCustomStyle(); }
    else if(kind == UiDesignerRuntimeKind::UiCheckBox) { auto *c = dynamic_cast<UiCheckBox*>(&ctrl); if(!c) return; auto s = c->GetStyle(); s.palette.face[ST_NORMAL] = UiFill::Solid(v.face); s.palette.frame[ST_NORMAL] = v.frame; s.palette.ink[ST_NORMAL] = v.ink; s.metrics.face_enabled = v.face_enabled; s.metrics.frame_enabled = v.frame_enabled; s.metrics.frame_width = v.frame_width; if(custom) c->SetCustomStyle(s); else c->ClearCustomStyle(); }
    else if(kind == UiDesignerRuntimeKind::UiRadioButton) { auto *c = dynamic_cast<UiRadioButton*>(&ctrl); if(!c) return; auto s = c->GetStyle(); s.palette.face[ST_NORMAL] = UiFill::Solid(v.face); s.palette.frame[ST_NORMAL] = v.frame; s.palette.ink[ST_NORMAL] = v.ink; s.metrics.face_enabled = v.face_enabled; s.metrics.frame_enabled = v.frame_enabled; s.metrics.frame_width = v.frame_width; if(custom) c->SetCustomStyle(s); else c->ClearCustomStyle(); }
    else if(kind == UiDesignerRuntimeKind::UiToggle) { auto *c = dynamic_cast<UiToggle*>(&ctrl); if(!c) return; auto s = c->GetStyle(); s.palette.face[ST_NORMAL] = UiFill::Solid(v.face); s.palette.frame[ST_NORMAL] = v.frame; s.palette.ink[ST_NORMAL] = v.ink; s.metrics.face_enabled = v.face_enabled; s.metrics.frame_enabled = v.frame_enabled; s.metrics.frame_width = v.frame_width; if(custom) c->SetCustomStyle(s); else c->ClearCustomStyle(); }
    else if(kind == UiDesignerRuntimeKind::UiProgressBar) { auto *c = dynamic_cast<UiProgressBar*>(&ctrl); if(!c) return; auto s = c->GetStyle(); s.track_palette.face[ST_NORMAL] = UiFill::Solid(v.face); s.track_palette.frame[ST_NORMAL] = v.frame; s.fill_palette.ink[ST_NORMAL] = v.ink; s.track_metrics.face_enabled = v.face_enabled; s.track_metrics.frame_enabled = v.frame_enabled; s.track_metrics.frame_width = v.frame_width; if(custom) c->SetCustomStyle(s); else c->ClearCustomStyle(); }
    else if(kind == UiDesignerRuntimeKind::UiSlider) { auto *c = dynamic_cast<UiSlider*>(&ctrl); if(!c) return; auto s = c->GetStyle(); s.track_palette.face[ST_NORMAL] = UiFill::Solid(v.face); s.track_palette.frame[ST_NORMAL] = v.frame; s.thumb_palette.face[ST_NORMAL] = UiFill::Solid(v.ink); s.track_metrics.face_enabled = v.face_enabled; s.track_metrics.frame_enabled = v.frame_enabled; s.track_metrics.frame_width = v.frame_width; if(custom) c->SetCustomStyle(s); else c->ClearCustomStyle(); }
    else if(kind == UiDesignerRuntimeKind::UiScrollBar) { auto *c = dynamic_cast<UiScrollBar*>(&ctrl); if(!c) return; auto s = c->GetStyle(); s.track_palette.face[ST_NORMAL] = UiFill::Solid(v.face); s.track_palette.frame[ST_NORMAL] = v.frame; s.thumb_palette.face[ST_NORMAL] = UiFill::Solid(v.ink); s.track_metrics.face_enabled = v.face_enabled; s.track_metrics.frame_enabled = v.frame_enabled; s.track_metrics.frame_width = v.frame_width; if(custom) c->SetCustomStyle(s); else c->ClearCustomStyle(); }
    else { auto *c = dynamic_cast<UiDropdown*>(&ctrl); if(!c) return; auto s = c->GetStyle(); s.palette.face[ST_NORMAL] = UiFill::Solid(v.face); s.palette.frame[ST_NORMAL] = v.frame; s.palette.ink[ST_NORMAL] = v.ink; s.metrics.face_enabled = v.face_enabled; s.metrics.frame_enabled = v.frame_enabled; s.metrics.frame_width = v.frame_width; if(custom) c->SetCustomStyle(s); else c->ClearCustomStyle(); }
}

class BasicThemeAdapter final : public UiDesignerThemeAdapter {
public:
    BasicThemeAdapter(const char *id, UiDesignerRuntimeKind kind) : id_(id), kind_(kind) {}
    const char *Id() const override { return id_; }
    bool Supports(UiDesignerRuntimeKind kind) const override { return kind == kind_; }
    void AddThemeOverrides(UiDesignerControlSpec& spec) const override { BasicThemeValues b = ResolveBasicThemeValues(kind_, UiRole::Standard); AddOverride(spec, "face_normal", "Face normal", "Surface", PropertyEditorKind::Color, b.face, PropertyImpactPaint | PropertyImpactCode, "face_normal"); AddOverride(spec, "frame_normal", "Frame normal", "Surface", PropertyEditorKind::Color, b.frame, PropertyImpactPaint | PropertyImpactCode, "frame_normal"); AddOverride(spec, "ink_normal", "Ink normal", "Ink", PropertyEditorKind::Color, b.ink, PropertyImpactPaint | PropertyImpactCode, "ink_normal"); AddOverride(spec, "face_enabled", "Face enabled", "Surface", PropertyEditorKind::Boolean, b.face_enabled, PropertyImpactPaint | PropertyImpactCode, "face_enabled"); AddOverride(spec, "frame_enabled", "Frame enabled", "Surface", PropertyEditorKind::Boolean, b.frame_enabled, PropertyImpactPaint | PropertyImpactCode, "frame_enabled"); AddOverride(spec, "frame_width", "Frame width", "Surface", PropertyEditorKind::Integer, b.frame_width, PropertyImpactPaint | PropertyImpactCode, "frame_width"); }
    bool HasField(const String& id) const override { return id == "face_normal" || id == "frame_normal" || id == "ink_normal" || id == "face_enabled" || id == "frame_enabled" || id == "frame_width"; }
    bool FieldAffectsLayout(const String& id) const override { return id == "face_enabled" || id == "frame_enabled" || id == "frame_width"; }
    Value ResolveFieldValue(const UiDesignerNode& node, const UiDesignerControlSpec& spec, const String& id, const UiDesignerTransientOverlay* overlay) const override { BasicThemeValues v = ResolveBasicThemeValues(kind_, ParseRole(node.GetProperty("role", "Standard"))); for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) { int q = node.theme_overrides.Find(p.id); if(q < 0 && !HasThemeValue(node, overlay, p.id)) continue; ApplyBasicThemeField(v, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value)); } if(id == "face_normal") return v.face; if(id == "frame_normal") return v.frame; if(id == "ink_normal") return v.ink; if(id == "face_enabled") return v.face_enabled; if(id == "frame_enabled") return v.frame_enabled; if(id == "frame_width") return v.frame_width; return Value(); }
    void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node, const UiDesignerControlSpec& spec, const UiDesignerTransientOverlay* overlay) const override { BasicThemeValues v = ResolveBasicThemeValues(kind_, ParseRole(node.GetProperty("role", "Standard"))); bool authored = false; for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) { int q = node.theme_overrides.Find(p.id); if(q < 0 && !HasThemeValue(node, overlay, p.id)) continue; authored = true; ApplyBasicThemeField(v, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value)); } ApplyBasicStyle(ctrl, kind_, v, authored || ParseRole(node.GetProperty("role", "Standard")) != UiRole::Standard); }
    void EmitSetup(String& out, const String& member, const UiDesignerNode& node, const UiDesignerControlSpec& spec) const override { bool authored = false; for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) authored |= node.theme_overrides.Find(p.id) >= 0; if(!authored) return; const char *type = node.type; const char *palette = (kind_ == UiDesignerRuntimeKind::UiProgressBar || kind_ == UiDesignerRuntimeKind::UiSlider || kind_ == UiDesignerRuntimeKind::UiScrollBar) ? "track_palette" : "palette"; const char *metrics = (kind_ == UiDesignerRuntimeKind::UiProgressBar || kind_ == UiDesignerRuntimeKind::UiSlider || kind_ == UiDesignerRuntimeKind::UiScrollBar) ? "track_metrics" : "metrics"; out << "\t" << type << "::Style " << member << "_style = UiTheme::Resolve" << (kind_ == UiDesignerRuntimeKind::UiLabel ? "Label(UiRole::Standard)" : kind_ == UiDesignerRuntimeKind::UiCheckBox ? "CheckBox(UiRole::Standard, UICHECKVIS_CLASSIC)" : kind_ == UiDesignerRuntimeKind::UiRadioButton ? "RadioButton(UiRole::Standard, UIRADIOVIS_CLASSIC)" : kind_ == UiDesignerRuntimeKind::UiToggle ? "Toggle()" : kind_ == UiDesignerRuntimeKind::UiProgressBar ? "ProgressBar(UiRole::Standard)" : kind_ == UiDesignerRuntimeKind::UiSlider ? "Slider()" : kind_ == UiDesignerRuntimeKind::UiScrollBar ? "ScrollBar()" : "Dropdown(UiRole::Standard)") << ";\n"; for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) { int q = node.theme_overrides.Find(p.id); if(q < 0) continue; String f = p.adapter_field_id; Value v = node.theme_overrides.GetValue(q); if(f == "face_normal") out << "\t" << member << "_style." << palette << ".face[ST_NORMAL] = UiFill::Solid(" << EmitValue(v) << ");\n"; else if(f == "frame_normal") out << "\t" << member << "_style." << palette << ".frame[ST_NORMAL] = " << EmitValue(v) << ";\n"; else if(f == "ink_normal") out << "\t" << member << "_style." << palette << ".ink[ST_NORMAL] = " << EmitValue(v) << ";\n"; else if(f == "face_enabled") out << "\t" << member << "_style." << metrics << ".face_enabled = " << AsString((bool)v) << ";\n"; else if(f == "frame_enabled") out << "\t" << member << "_style." << metrics << ".frame_enabled = " << AsString((bool)v) << ";\n"; else if(f == "frame_width") out << "\t" << member << "_style." << metrics << ".frame_width = DPI(" << (int)v << ");\n"; } out << "\t" << member << ".SetCustomStyle(" << member << "_style);\n"; }
private: const char *id_; UiDesignerRuntimeKind kind_;
};

static UiTab::Style ResolveTabThemeBase(UiRole role)
{
    return UiTheme::ResolveTab(role, UITAB_CLASSIC);
}

static void ApplyTabThemeField(UiTab::Style& style, const String& field_id, const Value& value)
{
    if(field_id == "face_enabled") style.metrics.face_enabled = (bool)value;
    else if(field_id == "face_normal") style.palette.face[ST_NORMAL] = UiFill::Solid((Color)value);
    else if(field_id == "frame_enabled") style.metrics.frame_enabled = (bool)value;
    else if(field_id == "frame_normal") style.palette.frame[ST_NORMAL] = (Color)value;
    else if(field_id == "frame_width") style.metrics.frame_width = max(0, (int)value);
    else if(field_id == "radius") style.metrics.radius = max(0, (int)value);
    else if(field_id == "tab_face_normal") style.tab_palette.face[ST_NORMAL] = UiFill::Solid((Color)value);
    else if(field_id == "tab_frame_normal") style.tab_palette.frame[ST_NORMAL] = (Color)value;
    else if(field_id == "tab_ink_normal") style.tab_palette.ink[ST_NORMAL] = (Color)value;
    else if(field_id == "tab_ink_hot") style.tab_palette.ink[ST_HOT] = (Color)value;
    else if(field_id == "tab_ink_pressed") style.tab_palette.ink[ST_PRESSED] = (Color)value;
    else if(field_id == "indicator_color") style.active_frame_color = (Color)value;
    else if(field_id == "indicator_thickness") style.indicator_thickness = max(0, (int)value);
    else if(field_id == "active_frame_width") style.active_frame_width = max(0, (int)value);
    else if(field_id == "tab_extent") style.tab_extent = max(1, (int)value);
    else if(field_id == "item_spacing") style.item_spacing = max(0, (int)value);
    else if(field_id == "body_gap") style.body_gap = max(0, (int)value);
    else if(field_id == "content_gap") style.content_gap = max(0, (int)value);
    else if(field_id == "expand_tabs") style.expand_tabs = (bool)value, style.fill_tabs = style.expand_tabs;
    else if(field_id == "active_tab_uses_body_face") style.active_tab_uses_body_face = (bool)value;
}

class TabThemeAdapter final : public UiDesignerThemeAdapter {
public:
    const char *Id() const override { return "tab"; }
    bool Supports(UiDesignerRuntimeKind kind) const override { return kind == UiDesignerRuntimeKind::UiTab; }
    void AddThemeOverrides(UiDesignerControlSpec& spec) const override
    {
        const UiTab::Style base = ResolveTabThemeBase(UiRole::Standard);
        AddOverride(spec, "face_enabled", "Face enabled", "Surface", PropertyEditorKind::Boolean, base.metrics.face_enabled, PropertyImpactPaint | PropertyImpactCode, "face_enabled");
        AddOverride(spec, "face_normal", "Face normal", "Surface", PropertyEditorKind::Color, UiDesignerFillColor(base.palette.face[ST_NORMAL]), PropertyImpactPaint | PropertyImpactCode, "face_normal");
        AddOverride(spec, "frame_enabled", "Frame enabled", "Surface", PropertyEditorKind::Boolean, base.metrics.frame_enabled, PropertyImpactPaint | PropertyImpactCode, "frame_enabled");
        AddOverride(spec, "frame_normal", "Frame normal", "Surface", PropertyEditorKind::Color, base.palette.frame[ST_NORMAL], PropertyImpactPaint | PropertyImpactCode, "frame_normal");
        AddOverride(spec, "frame_width", "Frame width", "Surface", PropertyEditorKind::Integer, base.metrics.frame_width, PropertyImpactPaint | PropertyImpactCode, "frame_width");
        AddOverride(spec, "radius", "Radius", "Surface", PropertyEditorKind::Integer, base.metrics.radius, PropertyImpactPaint | PropertyImpactCode, "radius");
        AddOverride(spec, "tab_face_normal", "Tab face", "Tabs", PropertyEditorKind::Color, UiDesignerFillColor(base.tab_palette.face[ST_NORMAL]), PropertyImpactPaint | PropertyImpactCode, "tab_face_normal");
        AddOverride(spec, "tab_frame_normal", "Tab frame", "Tabs", PropertyEditorKind::Color, base.tab_palette.frame[ST_NORMAL], PropertyImpactPaint | PropertyImpactCode, "tab_frame_normal");
        AddOverride(spec, "tab_ink_normal", "Tab ink", "Tabs", PropertyEditorKind::Color, base.tab_palette.ink[ST_NORMAL], PropertyImpactPaint | PropertyImpactCode, "tab_ink_normal");
        AddOverride(spec, "tab_ink_hot", "Tab hot ink", "Tabs", PropertyEditorKind::Color, base.tab_palette.ink[ST_HOT], PropertyImpactPaint | PropertyImpactCode, "tab_ink_hot");
        AddOverride(spec, "tab_ink_pressed", "Tab active ink", "Tabs", PropertyEditorKind::Color, base.tab_palette.ink[ST_PRESSED], PropertyImpactPaint | PropertyImpactCode, "tab_ink_pressed");
        AddOverride(spec, "indicator_color", "Active indicator", "Tabs", PropertyEditorKind::Color, base.active_frame_color, PropertyImpactPaint | PropertyImpactCode, "indicator_color");
        AddOverride(spec, "indicator_thickness", "Indicator thickness", "Tabs", PropertyEditorKind::Integer, base.indicator_thickness, PropertyImpactPaint | PropertyImpactCode, "indicator_thickness");
        AddOverride(spec, "active_frame_width", "Active frame width", "Tabs", PropertyEditorKind::Integer, base.active_frame_width, PropertyImpactPaint | PropertyImpactCode, "active_frame_width");
        AddOverride(spec, "tab_extent", "Tab extent", "Layout", PropertyEditorKind::Integer, base.tab_extent, PropertyImpactPaint | PropertyImpactCode, "tab_extent");
        AddOverride(spec, "item_spacing", "Item spacing", "Layout", PropertyEditorKind::Integer, base.item_spacing, PropertyImpactPaint | PropertyImpactCode, "item_spacing");
        AddOverride(spec, "body_gap", "Body gap", "Layout", PropertyEditorKind::Integer, base.body_gap, PropertyImpactPaint | PropertyImpactCode, "body_gap");
        AddOverride(spec, "content_gap", "Content gap", "Layout", PropertyEditorKind::Integer, base.content_gap, PropertyImpactPaint | PropertyImpactCode, "content_gap");
        AddOverride(spec, "expand_tabs", "Expand tabs", "Layout", PropertyEditorKind::Boolean, base.expand_tabs || base.fill_tabs, PropertyImpactPaint | PropertyImpactCode, "expand_tabs");
        AddOverride(spec, "active_tab_uses_body_face", "Active tab uses body face", "Tabs", PropertyEditorKind::Boolean, base.active_tab_uses_body_face, PropertyImpactPaint | PropertyImpactCode, "active_tab_uses_body_face");
    }
    bool HasField(const String& field_id) const override
    {
        static const char *fields[] = {"face_enabled", "face_normal", "frame_enabled", "frame_normal", "frame_width", "radius", "tab_face_normal", "tab_frame_normal", "tab_ink_normal", "tab_ink_hot", "tab_ink_pressed", "indicator_color", "indicator_thickness", "active_frame_width", "tab_extent", "item_spacing", "body_gap", "content_gap", "expand_tabs", "active_tab_uses_body_face"};
        for(const char *field : fields) if(field_id == field) return true;
        return false;
    }
    bool FieldAffectsLayout(const String& field_id) const override
    {
        return field_id == "face_enabled" || field_id == "frame_enabled" || field_id == "frame_width" || field_id == "radius" || field_id == "indicator_thickness" || field_id == "active_frame_width" || field_id == "tab_extent" || field_id == "item_spacing" || field_id == "body_gap" || field_id == "content_gap" || field_id == "expand_tabs";
    }
    Value ResolveFieldValue(const UiDesignerNode& node, const UiDesignerControlSpec& spec, const String& field_id, const UiDesignerTransientOverlay* overlay) const override
    {
        UiTab::Style style = ResolveTabThemeBase(ParseRole(node.GetProperty("role", "Standard")));
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0 && !HasThemeValue(node, overlay, p.id)) continue;
            ApplyTabThemeField(style, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value));
        }
        if(field_id == "face_enabled") return style.metrics.face_enabled;
        if(field_id == "face_normal") return UiDesignerFillColor(style.palette.face[ST_NORMAL]);
        if(field_id == "frame_enabled") return style.metrics.frame_enabled;
        if(field_id == "frame_normal") return style.palette.frame[ST_NORMAL];
        if(field_id == "frame_width") return style.metrics.frame_width;
        if(field_id == "radius") return style.metrics.radius;
        if(field_id == "tab_face_normal") return UiDesignerFillColor(style.tab_palette.face[ST_NORMAL]);
        if(field_id == "tab_frame_normal") return style.tab_palette.frame[ST_NORMAL];
        if(field_id == "tab_ink_normal") return style.tab_palette.ink[ST_NORMAL];
        if(field_id == "tab_ink_hot") return style.tab_palette.ink[ST_HOT];
        if(field_id == "tab_ink_pressed") return style.tab_palette.ink[ST_PRESSED];
        if(field_id == "indicator_color") return style.active_frame_color;
        if(field_id == "indicator_thickness") return style.indicator_thickness;
        if(field_id == "active_frame_width") return style.active_frame_width;
        if(field_id == "tab_extent") return style.tab_extent;
        if(field_id == "item_spacing") return style.item_spacing;
        if(field_id == "body_gap") return style.body_gap;
        if(field_id == "content_gap") return style.content_gap;
        if(field_id == "expand_tabs") return style.expand_tabs || style.fill_tabs;
        if(field_id == "active_tab_uses_body_face") return style.active_tab_uses_body_face;
        return Value();
    }
    void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node, const UiDesignerControlSpec& spec, const UiDesignerTransientOverlay* overlay) const override
    {
        UiTab *tab = dynamic_cast<UiTab *>(&ctrl);
        if(!tab) return;
        UiTab::Style style = ResolveTabThemeBase(ParseRole(node.GetProperty("role", "Standard")));
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0 && !HasThemeValue(node, overlay, p.id)) continue;
            authored = true;
            ApplyTabThemeField(style, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value));
        }
        if(authored || ParseRole(node.GetProperty("role", "Standard")) != UiRole::Standard) tab->SetCustomStyle(style);
        else tab->ClearCustomStyle();
    }
    void EmitSetup(String& out, const String& member, const UiDesignerNode& node, const UiDesignerControlSpec& spec) const override
    {
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) authored |= node.theme_overrides.Find(p.id) >= 0;
        if(!authored) return;
        out << "\tUiTab::Style " << member << "_style = UiTheme::ResolveTab(UiRole::Standard, UITAB_CLASSIC);\n";
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0) continue;
            const Value v = node.theme_overrides.GetValue(q);
            const String f = p.adapter_field_id;
            if(f == "face_enabled") out << "\t" << member << "_style.metrics.face_enabled = " << AsString((bool)v) << ";\n";
            else if(f == "face_normal") out << "\t" << member << "_style.palette.face[ST_NORMAL] = UiFill::Solid(" << EmitValue(v) << ");\n";
            else if(f == "frame_enabled") out << "\t" << member << "_style.metrics.frame_enabled = " << AsString((bool)v) << ";\n";
            else if(f == "frame_normal") out << "\t" << member << "_style.palette.frame[ST_NORMAL] = " << EmitValue(v) << ";\n";
            else if(f == "frame_width") out << "\t" << member << "_style.metrics.frame_width = DPI(" << (int)v << ");\n";
            else if(f == "radius") out << "\t" << member << "_style.metrics.radius = DPI(" << (int)v << ");\n";
            else if(f == "tab_face_normal") out << "\t" << member << "_style.tab_palette.face[ST_NORMAL] = UiFill::Solid(" << EmitValue(v) << ");\n";
            else if(f == "tab_frame_normal") out << "\t" << member << "_style.tab_palette.frame[ST_NORMAL] = " << EmitValue(v) << ";\n";
            else if(f == "tab_ink_normal") out << "\t" << member << "_style.tab_palette.ink[ST_NORMAL] = " << EmitValue(v) << ";\n";
            else if(f == "tab_ink_hot") out << "\t" << member << "_style.tab_palette.ink[ST_HOT] = " << EmitValue(v) << ";\n";
            else if(f == "tab_ink_pressed") out << "\t" << member << "_style.tab_palette.ink[ST_PRESSED] = " << EmitValue(v) << ";\n";
            else if(f == "indicator_color") out << "\t" << member << "_style.active_frame_color = " << EmitValue(v) << ";\n";
            else if(f == "indicator_thickness") out << "\t" << member << "_style.indicator_thickness = DPI(" << (int)v << ");\n";
            else if(f == "active_frame_width") out << "\t" << member << "_style.active_frame_width = DPI(" << (int)v << ");\n";
            else if(f == "tab_extent") out << "\t" << member << "_style.tab_extent = DPI(" << (int)v << ");\n";
            else if(f == "item_spacing") out << "\t" << member << "_style.item_spacing = DPI(" << (int)v << ");\n";
            else if(f == "body_gap") out << "\t" << member << "_style.body_gap = DPI(" << (int)v << ");\n";
            else if(f == "content_gap") out << "\t" << member << "_style.content_gap = DPI(" << (int)v << ");\n";
            else if(f == "expand_tabs") out << "\t" << member << "_style.expand_tabs = " << AsString((bool)v) << "; " << member << "_style.fill_tabs = " << AsString((bool)v) << ";\n";
            else if(f == "active_tab_uses_body_face") out << "\t" << member << "_style.active_tab_uses_body_face = " << AsString((bool)v) << ";\n";
        }
        out << "\t" << member << ".SetCustomStyle(" << member << "_style);\n";
    }
};

static UiAccordion::Style ResolveAccordionThemeBase()
{
    UiAccordion::Style style = UiAccordion::StyleDefault();
    UiPanel::Style panel = UiTheme::ResolvePanel(UiPanelRole::Surface);
    style.palette = panel.palette;
    style.metrics.radius = max(DPI(8), panel.metrics.radius);
    style.transparent = true;
    style.metrics.frame_width = 0;
    style.metrics.frame_enabled = false;
    style.metrics.face_enabled = false;
    style.metrics.shadow.enabled = false;
    style.body_style = UiTheme::ResolvePanel(UiPanelRole::Surface);
    style.body_style.transparent = true;
    style.body_style.metrics.face_enabled = false;
    style.body_style.metrics.frame_enabled = false;
    style.body_style.metrics.frame_width = 0;
    style.body_style.metrics.radius = 0;
    style.body_style.metrics.focus_enabled = false;
    style.body_style.metrics.content_margin = Rect(0, 0, 0, 0);
    style.body_style.metrics.shadow.enabled = false;
    style.header_style = UiTheme::ResolveTitleCard(UiRole::Accent);
    style.header_style.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    style.header_style.hover_enabled = false;
    style.header_style.metrics.focus_enabled = false;
    style.header_style.title_line = false;
    style.header_style.card_line = true;
    style.header_style.media_tint_mono = true;
    style.header_style.title_font = SansSerifZ(11).Bold();
    style.header_style.subtitle_font = SansSerifZ(8);
    return style;
}

static void ApplyAccordionThemeField(UiAccordion::Style& style, const String& field_id, const Value& value)
{
    if(field_id == "face_enabled") style.metrics.face_enabled = (bool)value;
    else if(field_id == "face_normal") style.palette.face[ST_NORMAL] = UiFill::Solid((Color)value);
    else if(field_id == "frame_enabled") style.metrics.frame_enabled = (bool)value;
    else if(field_id == "frame_normal") style.palette.frame[ST_NORMAL] = (Color)value;
    else if(field_id == "frame_width") style.metrics.frame_width = max(0, (int)value);
    else if(field_id == "radius") style.metrics.radius = max(0, (int)value);
    else if(field_id == "ink_normal") style.palette.ink[ST_NORMAL] = (Color)value;
    else if(field_id == "ink_disabled") style.palette.ink[ST_DISABLED] = (Color)value;
    else if(field_id == "header_height") style.header_height = max(1, (int)value);
    else if(field_id == "item_spacing") style.item_spacing = max(0, (int)value);
    else if(field_id == "header_body_gap") style.header_body_gap = max(0, (int)value);
    else if(field_id == "body_min_height") style.body_min_height = max(0, (int)value);
    else if(field_id == "single_open") style.single_open = (bool)value;
    else if(field_id == "enforce_one") style.enforce_one = (bool)value;
    else if(field_id == "show_chevron") style.show_chevron = (bool)value;
    else if(field_id == "chevron_side") style.chevron_side = AsString(value) == "Top" ? UiAlign::TOP : AsString(value) == "Bottom" ? UiAlign::BOTTOM : AsString(value) == "Left" ? UiAlign::LEFT : UiAlign::RIGHT;
    else if(field_id == "show_drag_handle") style.show_drag_handle = (bool)value;
    else if(field_id == "drag_side") style.drag_side = AsString(value) == "Left" ? UiAlign::LEFT : UiAlign::RIGHT;
    else if(field_id == "unified_section_frame") style.unified_section_frame = (bool)value;
    else if(field_id == "unified_section_radius") style.unified_section_radius = max(0, (int)value);
    else if(field_id == "unified_section_frame_width") style.unified_section_frame_width = max(0, (int)value);
    else if(field_id == "body_line_extent") style.body_line_extent = AsString(value) == "Small" ? SMALL : AsString(value) == "Medium" ? MEDIUM : AsString(value) == "Large" ? LARGE : NONE;
    else if(field_id == "body_line_thickness") style.body_line_thickness = max(0, (int)value);
    else if(field_id == "transparent") style.transparent = (bool)value;
    else if(field_id == "animation_enabled") style.animation_enabled = (bool)value;
    else if(field_id == "anim_open_ms") style.anim_open_ms = max(0, (int)value);
    else if(field_id == "anim_close_ms") style.anim_close_ms = max(0, (int)value);
}

class AccordionThemeAdapter final : public UiDesignerThemeAdapter {
public:
    const char *Id() const override { return "accordion"; }
    bool Supports(UiDesignerRuntimeKind kind) const override { return kind == UiDesignerRuntimeKind::UiAccordion; }
    void AddThemeOverrides(UiDesignerControlSpec& spec) const override
    {
        const UiAccordion::Style base = ResolveAccordionThemeBase();
        AddOverride(spec, "face_enabled", "Face enabled", "Surface", PropertyEditorKind::Boolean, base.metrics.face_enabled, PropertyImpactPaint | PropertyImpactCode, "face_enabled");
        AddOverride(spec, "face_normal", "Face normal", "Surface", PropertyEditorKind::Color, base.palette.face[ST_NORMAL].IsSolid() ? base.palette.face[ST_NORMAL].color : White(), PropertyImpactPaint | PropertyImpactCode, "face_normal");
        AddOverride(spec, "frame_enabled", "Frame enabled", "Surface", PropertyEditorKind::Boolean, base.metrics.frame_enabled, PropertyImpactPaint | PropertyImpactCode, "frame_enabled");
        AddOverride(spec, "frame_normal", "Frame normal", "Surface", PropertyEditorKind::Color, base.palette.frame[ST_NORMAL], PropertyImpactPaint | PropertyImpactCode, "frame_normal");
        AddOverride(spec, "frame_width", "Frame width", "Surface", PropertyEditorKind::Integer, base.metrics.frame_width, PropertyImpactPaint | PropertyImpactCode, "frame_width");
        AddOverride(spec, "radius", "Radius", "Surface", PropertyEditorKind::Integer, base.metrics.radius, PropertyImpactPaint | PropertyImpactCode, "radius");
        AddOverride(spec, "ink_normal", "Ink normal", "Ink", PropertyEditorKind::Color, base.palette.ink[ST_NORMAL], PropertyImpactPaint | PropertyImpactCode, "ink_normal");
        AddOverride(spec, "ink_disabled", "Ink disabled", "Ink", PropertyEditorKind::Color, base.palette.ink[ST_DISABLED], PropertyImpactPaint | PropertyImpactCode, "ink_disabled");
        AddOverride(spec, "header_height", "Header height", "Layout", PropertyEditorKind::Integer, base.header_height, PropertyImpactPaint | PropertyImpactCode, "header_height");
        AddOverride(spec, "item_spacing", "Item spacing", "Layout", PropertyEditorKind::Integer, base.item_spacing, PropertyImpactPaint | PropertyImpactCode, "item_spacing");
        AddOverride(spec, "header_body_gap", "Header/body gap", "Layout", PropertyEditorKind::Integer, base.header_body_gap, PropertyImpactPaint | PropertyImpactCode, "header_body_gap");
        AddOverride(spec, "body_min_height", "Body min height", "Layout", PropertyEditorKind::Integer, base.body_min_height, PropertyImpactPaint | PropertyImpactCode, "body_min_height");
        AddOverride(spec, "single_open", "Single open", "Behavior", PropertyEditorKind::Boolean, base.single_open, PropertyImpactPaint | PropertyImpactCode, "single_open");
        AddOverride(spec, "enforce_one", "Enforce one open", "Behavior", PropertyEditorKind::Boolean, base.enforce_one, PropertyImpactPaint | PropertyImpactCode, "enforce_one");
        AddOverride(spec, "show_chevron", "Show chevron", "Header", PropertyEditorKind::Boolean, base.show_chevron, PropertyImpactPaint | PropertyImpactCode, "show_chevron");
        AddOverride(spec, "chevron_side", "Chevron side", "Header", PropertyEditorKind::Choice, String("Right"), PropertyImpactPaint | PropertyImpactCode, "chevron_side");
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Left", "Left"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Right", "Right"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Top", "Top"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Bottom", "Bottom"));
        AddOverride(spec, "show_drag_handle", "Show drag handle", "Header", PropertyEditorKind::Boolean, base.show_drag_handle, PropertyImpactPaint | PropertyImpactCode, "show_drag_handle");
        AddOverride(spec, "drag_side", "Drag side", "Header", PropertyEditorKind::Choice, String("Right"), PropertyImpactPaint | PropertyImpactCode, "drag_side");
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Left", "Left"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Right", "Right"));
        AddOverride(spec, "unified_section_frame", "Unified section frame", "Body", PropertyEditorKind::Boolean, base.unified_section_frame, PropertyImpactPaint | PropertyImpactCode, "unified_section_frame");
        AddOverride(spec, "unified_section_radius", "Unified section radius", "Body", PropertyEditorKind::Integer, base.unified_section_radius, PropertyImpactPaint | PropertyImpactCode, "unified_section_radius");
        AddOverride(spec, "unified_section_frame_width", "Unified section frame width", "Body", PropertyEditorKind::Integer, base.unified_section_frame_width, PropertyImpactPaint | PropertyImpactCode, "unified_section_frame_width");
        AddOverride(spec, "body_line_extent", "Body line extent", "Body", PropertyEditorKind::Choice, String("None"), PropertyImpactPaint | PropertyImpactCode, "body_line_extent");
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("None", "None"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Small", "Small"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Medium", "Medium"));
        spec.theme_overrides.Top().choices.Add(PropertyEditorChoice("Large", "Large"));
        AddOverride(spec, "body_line_thickness", "Body line thickness", "Body", PropertyEditorKind::Integer, base.body_line_thickness, PropertyImpactPaint | PropertyImpactCode, "body_line_thickness");
        AddOverride(spec, "transparent", "Transparent", "Surface", PropertyEditorKind::Boolean, base.transparent, PropertyImpactPaint | PropertyImpactCode, "transparent");
        AddOverride(spec, "animation_enabled", "Animation enabled", "Behavior", PropertyEditorKind::Boolean, base.animation_enabled, PropertyImpactPaint | PropertyImpactCode, "animation_enabled");
        AddOverride(spec, "anim_open_ms", "Open animation ms", "Behavior", PropertyEditorKind::Integer, base.anim_open_ms, PropertyImpactPaint | PropertyImpactCode, "anim_open_ms");
        AddOverride(spec, "anim_close_ms", "Close animation ms", "Behavior", PropertyEditorKind::Integer, base.anim_close_ms, PropertyImpactPaint | PropertyImpactCode, "anim_close_ms");
    }
    bool HasField(const String& field_id) const override
    {
        static const char *fields[] = {
            "face_enabled","face_normal","frame_enabled","frame_normal","frame_width","radius",
            "ink_normal","ink_disabled","header_height","item_spacing","header_body_gap",
            "body_min_height","single_open","enforce_one","show_chevron","chevron_side",
            "show_drag_handle","drag_side","unified_section_frame","unified_section_radius",
            "unified_section_frame_width","body_line_extent","body_line_thickness","transparent",
            "animation_enabled","anim_open_ms","anim_close_ms"
        };
        for(const char *field : fields) if(field_id == field) return true;
        return false;
    }
    bool FieldAffectsLayout(const String& field_id) const override
    {
        return field_id == "face_enabled" || field_id == "frame_enabled" || field_id == "frame_width" ||
               field_id == "radius" || field_id == "header_height" || field_id == "item_spacing" ||
               field_id == "header_body_gap" || field_id == "body_min_height" || field_id == "show_chevron" ||
               field_id == "chevron_side" || field_id == "show_drag_handle" || field_id == "drag_side" ||
               field_id == "unified_section_frame" || field_id == "unified_section_radius" ||
               field_id == "unified_section_frame_width" || field_id == "body_line_extent" ||
               field_id == "body_line_thickness" || field_id == "animation_enabled" ||
               field_id == "anim_open_ms" || field_id == "anim_close_ms";
    }
    Value ResolveFieldValue(const UiDesignerNode& node, const UiDesignerControlSpec& spec,
                            const String& field_id, const UiDesignerTransientOverlay* overlay) const override
    {
        UiAccordion::Style style = ResolveAccordionThemeBase();
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0 && !HasThemeValue(node, overlay, p.id))
                continue;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value;
            ApplyAccordionThemeField(style, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, canonical));
        }
        if(field_id == "face_enabled") return style.metrics.face_enabled;
        if(field_id == "face_normal") return UiDesignerFillColor(style.palette.face[ST_NORMAL]);
        if(field_id == "frame_enabled") return style.metrics.frame_enabled;
        if(field_id == "frame_normal") return style.palette.frame[ST_NORMAL];
        if(field_id == "frame_width") return style.metrics.frame_width;
        if(field_id == "radius") return style.metrics.radius;
        if(field_id == "ink_normal") return style.palette.ink[ST_NORMAL];
        if(field_id == "ink_disabled") return style.palette.ink[ST_DISABLED];
        if(field_id == "header_height") return style.header_height;
        if(field_id == "item_spacing") return style.item_spacing;
        if(field_id == "header_body_gap") return style.header_body_gap;
        if(field_id == "body_min_height") return style.body_min_height;
        if(field_id == "single_open") return style.single_open;
        if(field_id == "enforce_one") return style.enforce_one;
        if(field_id == "show_chevron") return style.show_chevron;
        if(field_id == "chevron_side") return style.chevron_side == UiAlign::TOP ? "Top" : style.chevron_side == UiAlign::BOTTOM ? "Bottom" : style.chevron_side == UiAlign::LEFT ? "Left" : "Right";
        if(field_id == "show_drag_handle") return style.show_drag_handle;
        if(field_id == "drag_side") return style.drag_side == UiAlign::LEFT ? "Left" : "Right";
        if(field_id == "unified_section_frame") return style.unified_section_frame;
        if(field_id == "unified_section_radius") return style.unified_section_radius;
        if(field_id == "unified_section_frame_width") return style.unified_section_frame_width;
        if(field_id == "body_line_extent") return style.body_line_extent == SMALL ? "Small" : style.body_line_extent == MEDIUM ? "Medium" : style.body_line_extent == LARGE ? "Large" : "None";
        if(field_id == "body_line_thickness") return style.body_line_thickness;
        if(field_id == "transparent") return style.transparent;
        if(field_id == "animation_enabled") return style.animation_enabled;
        if(field_id == "anim_open_ms") return style.anim_open_ms;
        if(field_id == "anim_close_ms") return style.anim_close_ms;
        return Value();
    }
    void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node, const UiDesignerControlSpec& spec, const UiDesignerTransientOverlay* overlay) const override
    {
        UiAccordion *accordion = dynamic_cast<UiAccordion *>(&ctrl);
        if(!accordion)
            return;
        UiAccordion::Style style = ResolveAccordionThemeBase();
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0 && !HasThemeValue(node, overlay, p.id))
                continue;
            authored = true;
            const Value canonical = q >= 0 ? node.theme_overrides.GetValue(q) : p.default_value;
            ApplyAccordionThemeField(style, p.adapter_field_id, ResolveThemeValue(node, overlay, p.id, canonical));
        }
        if(authored) accordion->SetCustomStyle(style); else accordion->ClearCustomStyle();
    }
    void EmitSetup(String& out, const String& member, const UiDesignerNode& node, const UiDesignerControlSpec& spec) const override
    {
        bool authored = false;
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides)
            authored |= node.theme_overrides.Find(p.id) >= 0;
        if(!authored)
            return;
        out << "\tUiAccordion::Style " << member << "_style = UiAccordion::StyleDefault();\n";
        out << "\tUiPanel::Style " << member << "_panel = UiTheme::ResolvePanel(UiPanelRole::Surface);\n";
        out << "\t" << member << "_style.palette = " << member << "_panel.palette;\n";
        out << "\t" << member << "_style.header_style = UiTheme::ResolveTitleCard(UiRole::Accent);\n";
        for(const UiDesignerThemeOverrideSpec& p : spec.theme_overrides) {
            const int q = node.theme_overrides.Find(p.id);
            if(q < 0)
                continue;
            const Value v = node.theme_overrides.GetValue(q);
            if(p.adapter_field_id == "face_enabled") out << "\t" << member << "_style.metrics.face_enabled = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "face_normal") out << "\t" << member << "_style.palette.face[ST_NORMAL] = UiFill::Solid(" << EmitValue(v) << ");\n";
            else if(p.adapter_field_id == "frame_enabled") out << "\t" << member << "_style.metrics.frame_enabled = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "frame_normal") out << "\t" << member << "_style.palette.frame[ST_NORMAL] = " << EmitValue(v) << ";\n";
            else if(p.adapter_field_id == "frame_width") out << "\t" << member << "_style.metrics.frame_width = " << (int)v << ";\n";
            else if(p.adapter_field_id == "radius") out << "\t" << member << "_style.metrics.radius = " << (int)v << ";\n";
            else if(p.adapter_field_id == "ink_normal") out << "\t" << member << "_style.palette.ink[ST_NORMAL] = " << EmitValue(v) << ";\n";
            else if(p.adapter_field_id == "ink_disabled") out << "\t" << member << "_style.palette.ink[ST_DISABLED] = " << EmitValue(v) << ";\n";
            else if(p.adapter_field_id == "header_height") out << "\t" << member << "_style.header_height = " << (int)v << ";\n";
            else if(p.adapter_field_id == "item_spacing") out << "\t" << member << "_style.item_spacing = " << (int)v << ";\n";
            else if(p.adapter_field_id == "header_body_gap") out << "\t" << member << "_style.header_body_gap = " << (int)v << ";\n";
            else if(p.adapter_field_id == "body_min_height") out << "\t" << member << "_style.body_min_height = " << (int)v << ";\n";
            else if(p.adapter_field_id == "single_open") out << "\t" << member << "_style.single_open = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "enforce_one") out << "\t" << member << "_style.enforce_one = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "show_chevron") out << "\t" << member << "_style.show_chevron = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "chevron_side") out << "\t" << member << "_style.chevron_side = " << EmitAlign(AsString(v)) << ";\n";
            else if(p.adapter_field_id == "show_drag_handle") out << "\t" << member << "_style.show_drag_handle = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "drag_side") out << "\t" << member << "_style.drag_side = " << EmitAlign(AsString(v)) << ";\n";
            else if(p.adapter_field_id == "unified_section_frame") out << "\t" << member << "_style.unified_section_frame = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "unified_section_radius") out << "\t" << member << "_style.unified_section_radius = " << (int)v << ";\n";
            else if(p.adapter_field_id == "unified_section_frame_width") out << "\t" << member << "_style.unified_section_frame_width = " << (int)v << ";\n";
            else if(p.adapter_field_id == "body_line_extent") out << "\t" << member << "_style.body_line_extent = " << (AsString(v) == "Small" ? "SMALL" : AsString(v) == "Medium" ? "MEDIUM" : AsString(v) == "Large" ? "LARGE" : "NONE") << ";\n";
            else if(p.adapter_field_id == "body_line_thickness") out << "\t" << member << "_style.body_line_thickness = " << (int)v << ";\n";
            else if(p.adapter_field_id == "transparent") out << "\t" << member << "_style.transparent = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "animation_enabled") out << "\t" << member << "_style.animation_enabled = " << AsString((bool)v) << ";\n";
            else if(p.adapter_field_id == "anim_open_ms") out << "\t" << member << "_style.anim_open_ms = " << (int)v << ";\n";
            else if(p.adapter_field_id == "anim_close_ms") out << "\t" << member << "_style.anim_close_ms = " << (int)v << ";\n";
        }
        out << "\t" << member << ".SetCustomStyle(" << member << "_style);\n";
    }
};

class ThemeAdapterRegistry {
public:
    ThemeAdapterRegistry()
    {
        adapters.Add(&button_adapter_);
        adapters.Add(&tool_button_adapter_);
        adapters.Add(&edit_adapter_);
        adapters.Add(&tree_adapter_);
        adapters.Add(&list_adapter_);
        adapters.Add(&menu_adapter_);
        adapters.Add(&color_picker_adapter_);
        adapters.Add(&panel_adapter_);
        adapters.Add(&group_panel_adapter_);
        adapters.Add(&scroll_panel_adapter_);
        adapters.Add(&label_adapter_);
        adapters.Add(&check_adapter_);
        adapters.Add(&radio_adapter_);
        adapters.Add(&toggle_adapter_);
        adapters.Add(&progress_adapter_);
        adapters.Add(&slider_adapter_);
        adapters.Add(&scroll_bar_adapter_);
        adapters.Add(&dropdown_adapter_);
        adapters.Add(&tab_adapter_);
        adapters.Add(&title_card_adapter_);
        adapters.Add(&accordion_adapter_);
    }

    const UiDesignerThemeAdapter* Find(const String& id) const
    {
        for(const UiDesignerThemeAdapter* adapter : adapters)
            if(id == adapter->Id())
                return adapter;
        return nullptr;
    }

    const UiDesignerThemeAdapter* Find(UiDesignerRuntimeKind kind) const
    {
        for(const UiDesignerThemeAdapter* adapter : adapters)
            if(adapter->Supports(kind))
                return adapter;
        return nullptr;
    }

private:
    ButtonThemeAdapter button_adapter_{"button", false};
    ButtonThemeAdapter tool_button_adapter_{"tool_button", true};
    EditThemeAdapter edit_adapter_;
    TreeThemeAdapter tree_adapter_;
    ListThemeAdapter list_adapter_;
    MenuThemeAdapter menu_adapter_;
    ColorPickerThemeAdapter color_picker_adapter_;
    PanelThemeAdapter panel_adapter_{"panel", UiDesignerRuntimeKind::UiPanel};
    PanelThemeAdapter group_panel_adapter_{"group_panel", UiDesignerRuntimeKind::UiGroupPanel};
    PanelThemeAdapter scroll_panel_adapter_{"scroll_panel", UiDesignerRuntimeKind::UiScrollPanel};
    BasicThemeAdapter label_adapter_{"label", UiDesignerRuntimeKind::UiLabel};
    BasicThemeAdapter check_adapter_{"check", UiDesignerRuntimeKind::UiCheckBox};
    BasicThemeAdapter radio_adapter_{"radio", UiDesignerRuntimeKind::UiRadioButton};
    BasicThemeAdapter toggle_adapter_{"toggle", UiDesignerRuntimeKind::UiToggle};
    BasicThemeAdapter progress_adapter_{"progress", UiDesignerRuntimeKind::UiProgressBar};
    BasicThemeAdapter slider_adapter_{"slider", UiDesignerRuntimeKind::UiSlider};
    BasicThemeAdapter scroll_bar_adapter_{"scroll_bar", UiDesignerRuntimeKind::UiScrollBar};
    BasicThemeAdapter dropdown_adapter_{"dropdown", UiDesignerRuntimeKind::UiDropdown};
    TabThemeAdapter tab_adapter_;
    TitleCardThemeAdapter title_card_adapter_;
    AccordionThemeAdapter accordion_adapter_;
    Array<const UiDesignerThemeAdapter*> adapters;
};

static const ThemeAdapterRegistry& Registry()
{
    static ThemeAdapterRegistry registry;
    return registry;
}

const UiDesignerThemeAdapter* UiDesignerFindThemeAdapter(const String& id)
{
    return Registry().Find(id);
}

const UiDesignerThemeAdapter* UiDesignerFindThemeAdapter(UiDesignerRuntimeKind kind)
{
    return Registry().Find(kind);
}

const UiDesignerThemeAdapter* UiDesignerGetThemeAdapter(const UiDesignerControlSpec& spec)
{
    if(!spec.theme_adapter_id.IsEmpty())
        return UiDesignerFindThemeAdapter(spec.theme_adapter_id);
    return UiDesignerFindThemeAdapter(spec.runtime_kind);
}

bool UiDesignerThemeAdapterSupports(const UiDesignerControlSpec& spec)
{
    const UiDesignerThemeAdapter* adapter = UiDesignerGetThemeAdapter(spec);
    return adapter && adapter->Supports(spec.runtime_kind);
}

}
