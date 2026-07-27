#include "UiDesignerThemeAdapter.h"
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>
#include <Utilities/UiDesigner/Core/UiDesignerOverlay.h>
#include <Utilities/UiDesigner/UiDesigner/UiDesignerButtonStyle.h>
#include <Ui/UiButton.h>
#include <Ui/UiTree.h>
#include <Ui/UiList.h>
#include <Ui/UiMenu.h>
#include <Ui/UiColorPicker.h>
#include <Ui/UiTheme.h>

namespace Upp {

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
                            : kind == UiDesignerRuntimeKind::UiButton;
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

class ThemeAdapterRegistry {
public:
    ThemeAdapterRegistry()
    {
        adapters.Add(&button_adapter_);
        adapters.Add(&tool_button_adapter_);
        adapters.Add(&tree_adapter_);
        adapters.Add(&list_adapter_);
        adapters.Add(&menu_adapter_);
        adapters.Add(&color_picker_adapter_);
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
    TreeThemeAdapter tree_adapter_;
    ListThemeAdapter list_adapter_;
    MenuThemeAdapter menu_adapter_;
    ColorPickerThemeAdapter color_picker_adapter_;
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
