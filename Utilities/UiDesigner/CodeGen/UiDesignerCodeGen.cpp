#include "UiDesignerCodeGen.h"
#include <Utilities/UiDesigner/Catalog/UiDesignerColorPickerContract.h>
#include <Utilities/UiDesigner/UiDesigner/UiDesignerButtonStyle.h>
#include <Utilities/UiDesigner/Theme/UiDesignerThemeAdapter.h>

namespace Upp {

static String SanitizeIdentifier(String value)
{
    value = TrimBoth(value);
    if(value.IsEmpty())
        value = "control";
    String out;
    for(int i = 0; i < value.GetCount(); i++) {
        const int c = value[i];
        if(IsAlNum(c) || c == '_')
            out.Cat(c);
        else if(out.IsEmpty() || out[out.GetCount() - 1] != '_')
            out.Cat('_');
    }
    while(out.GetCount() && out[out.GetCount() - 1] == '_')
        out.Trim(out.GetCount() - 1);
    if(out.IsEmpty())
        out = "control";
    if(IsDigit(out[0]))
        out = "_" + out;
    static const char *reserved[] = {
        "class", "private", "public", "protected", "template", "typename",
        "operator", "int", "double", "float", "bool", "char", "void",
        "auto", "return", "new", "delete", "namespace"
    };
    for(const char *word : reserved)
        if(out == word)
            return "_" + out;
    return out;
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

static String NamespaceOpen(const String& ns)
{
    return ns.IsEmpty() ? String() : "namespace " + ns + " {\n\n";
}

static String NamespaceClose(const String& ns)
{
    return ns.IsEmpty() ? String() : "\n} // namespace " + ns + "\n";
}

static String BoxAlignExpr(const String& value)
{
    if(value == "Stretch" || value == "Fill") return "UiCrossAlign::Stretch";
    if(value == "Start") return "UiCrossAlign::Start";
    if(value == "End") return "UiCrossAlign::End";
    if(value == "Center") return "UiCrossAlign::Center";
    return "UiCrossAlign::Auto";
}

static String GridAlignExpr(const String& value)
{
    if(value == "Stretch" || value == "Fill") return "UiGridLayout::Align::Stretch";
    if(value == "Start") return "UiGridLayout::Align::Start";
    if(value == "End") return "UiGridLayout::Align::End";
    if(value == "Center") return "UiGridLayout::Align::Center";
    return "UiGridLayout::Align::Auto";
}

static String LineOrientationExpr(const String& value)
{
    if(value == "Vertical") return "UiSpacerLineOrientation::Vertical";
    if(value == "Horizontal") return "UiSpacerLineOrientation::Horizontal";
    return "UiSpacerLineOrientation::Auto";
}

static String LineDashExpr(const String& value)
{
    if(value == "Dash") return "DASHED";
    if(value == "Dot") return "DOTTED";
    return "SOLID";
}

const UiDesignerGeneratedFile* UiDesignerGeneratedProject::FindFile(
    const String& path) const
{
    for(const UiDesignerGeneratedFile& file : files)
        if(file.relative_path == path)
            return &file;
    return nullptr;
}

bool UiDesignerValidateCppIdentifier(const String& value, String& error)
{
    if(value.IsEmpty()) {
        error = "C++ identifier is empty";
        return false;
    }
    if(!IsAlpha(value[0]) && value[0] != '_') {
        error = "C++ identifier must start with a letter or underscore: " + value;
        return false;
    }
    for(int i = 1; i < value.GetCount(); i++)
        if(!IsAlNum(value[i]) && value[i] != '_') {
            error = "C++ identifier contains an invalid character: " + value;
            return false;
        }
    if(SanitizeIdentifier(value) != value) {
        error = "C++ identifier is reserved or not canonical: " + value;
        return false;
    }
    error.Clear();
    return true;
}

bool UiDesignerValidateGenerationOptions(
    const UiDesignerCodeGenerationOptions& options, String& error)
{
    if(!UiDesignerValidateCppIdentifier(options.package_name, error) ||
       !UiDesignerValidateCppIdentifier(options.class_name, error))
        return false;
    if(!options.namespace_name.IsEmpty()) {
        Vector<String> parts = Split(options.namespace_name, "::");
        for(const String& part : parts)
            if(!UiDesignerValidateCppIdentifier(part, error))
                return false;
    }
    error.Clear();
    return true;
}

String UiDesignerCodeGenerator::MemberName(const UiDesignerNode& node) const
{
    String base = SanitizeIdentifier(node.name.IsEmpty()
                                     ? ToLower(node.type)
                                     : node.name);
    return base + "_n" + AsString(node.id);
}

String UiDesignerCodeGenerator::EmitColor(Color c) const
{
    return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

String UiDesignerCodeGenerator::EmitValue(const Value& value) const
{
    if(IsNull(value))
        return "Value()";
    if(value.Is<String>())
        return CppString(value);
    if(value.Is<bool>())
        return (bool)value ? "true" : "false";
    if(value.Is<int>() || value.Is<int64>())
        return AsString(value);
    if(value.Is<double>())
        return Format("%.12g", (double)value);
    if(value.Is<Color>())
        return EmitColor((Color)value);
    return "ParseJSON(" + CppString(AsJSON(value, false)) + ")";
}

String UiDesignerCodeGenerator::QualifiedClass(
    const UiDesignerCodeGenerationOptions& options,
    const String& suffix) const
{
    return options.namespace_name.IsEmpty()
        ? options.class_name + suffix
        : options.namespace_name + "::" + options.class_name + suffix;
}

static bool UsesLabelSetter(UiDesignerRuntimeKind kind)
{
    return kind == UiDesignerRuntimeKind::UppLabel ||
           kind == UiDesignerRuntimeKind::UppButton ||
           kind == UiDesignerRuntimeKind::UppOption;
}

static bool IsButtonFamily(UiDesignerRuntimeKind kind)
{
    return kind == UiDesignerRuntimeKind::UiButton ||
           kind == UiDesignerRuntimeKind::UiToolButton ||
           kind == UiDesignerRuntimeKind::UiSplitButton;
}

static String EmitRoleExpr(const String& value)
{
    if(value == "Subtle") return "UiRole::Subtle";
    if(value == "Accent") return "UiRole::Accent";
    if(value == "Alert") return "UiRole::Alert";
    return "UiRole::Standard";
}

static String ButtonStyleResolverExpr(UiDesignerRuntimeKind kind,
                                      const String& role)
{
    if(kind == UiDesignerRuntimeKind::UiToolButton)
        return "UiTheme::ResolveToolButton(" + EmitRoleExpr(role) + ")";
    return "UiTheme::ResolveButton(" + EmitRoleExpr(role) + ")";
}

static String EmitButtonIcon(const String& name)
{
    if(name.IsEmpty() || name == "None")
        return "Image()";
    return name + "()";
}

static String EmitAlign(const String& value)
{
    if(value == "Left") return "UiAlign::LEFT";
    if(value == "Right") return "UiAlign::RIGHT";
    if(value == "Top") return "UiAlign::TOP";
    if(value == "Bottom") return "UiAlign::BOTTOM";
    return "UiAlign::CENTER";
}

static String EmitIconRenderMode(const String& value)
{
    if(value == "Auto")
        return "UiIconRenderMode::Auto";
    if(value == "PreserveColor")
        return "UiIconRenderMode::PreserveColor";
    return "UiIconRenderMode::MonoTint";
}

static String EmitUiSpan(const String& value)
{
    if(value == "None") return "NONE";
    if(value == "Small") return "SMALL";
    return "LARGE";
}

static String EmitUiLineStyle(const String& value)
{
    if(value == "Dashed") return "DASHED";
    if(value == "Dotted") return "DOTTED";
    return "SOLID";
}

void UiDesignerCodeGenerator::EmitSetup(
    String& out, const UiDesignerNode& node,
    const UiDesignerControlSpec& spec) const
{
    if(spec.IsSemanticItem())
        return;
    const String member = MemberName(node);
    const auto Property = [&](const String& property, const Value& fallback) -> Value {
        return node.GetProperty(property, fallback);
    };
    const auto Effective = Property;

    const auto EmitButtonStyleField = [&](const String& style_var,
                                          UiDesignerButtonStyleField field,
                                          const Value& value) {
        switch(field) {
        case UiDesignerButtonStyleField::FontFace:
            out << "\t" << style_var << ".font.FaceName(" << EmitValue(value) << ");\n";
            break;
        case UiDesignerButtonStyleField::FontSize:
            out << "\t" << style_var << ".font.Height("
                << max(1, (int)value) << ");\n";
            break;
        case UiDesignerButtonStyleField::FontBold:
            out << "\t" << style_var << ".font.Bold("
                << EmitValue(value) << ");\n";
            break;
        case UiDesignerButtonStyleField::FontItalic:
            out << "\t" << style_var << ".font.Italic("
                << EmitValue(value) << ");\n";
            break;
        case UiDesignerButtonStyleField::FaceEnabled:
            out << "\t" << style_var << ".metrics.face_enabled = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::FaceNormal:
            out << "\t" << style_var << ".palette.face[ST_NORMAL] = UiFill::Solid("
                << EmitValue(value) << ");\n";
            break;
        case UiDesignerButtonStyleField::FaceHot:
            out << "\t" << style_var << ".palette.face[ST_HOT] = UiFill::Solid("
                << EmitValue(value) << ");\n";
            break;
        case UiDesignerButtonStyleField::FacePressed:
            out << "\t" << style_var << ".palette.face[ST_PRESSED] = UiFill::Solid("
                << EmitValue(value) << ");\n";
            break;
        case UiDesignerButtonStyleField::FaceDisabled:
            out << "\t" << style_var << ".palette.face[ST_DISABLED] = UiFill::Solid("
                << EmitValue(value) << ");\n";
            break;
        case UiDesignerButtonStyleField::Transparent:
            out << "\t" << style_var << ".transparent = " << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::FrameEnabled:
            out << "\t" << style_var << ".metrics.frame_enabled = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::FrameNormal:
            out << "\t" << style_var << ".palette.frame[ST_NORMAL] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::FrameHot:
            out << "\t" << style_var << ".palette.frame[ST_HOT] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::FramePressed:
            out << "\t" << style_var << ".palette.frame[ST_PRESSED] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::FrameDisabled:
            out << "\t" << style_var << ".palette.frame[ST_DISABLED] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::FrameWidth:
            out << "\t" << style_var << ".metrics.frame_width = "
                << max(0, (int)value) << ";\n";
            break;
        case UiDesignerButtonStyleField::Radius:
            out << "\t" << style_var << ".metrics.radius = "
                << max(0, (int)value) << ";\n";
            break;
        case UiDesignerButtonStyleField::FrameDashed:
            out << "\t" << style_var << ".metrics.dashed = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::FrameDashPattern:
            out << "\t" << style_var << ".metrics.dash_pattern = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::TextNormal:
            out << "\t" << style_var << ".palette.ink[ST_NORMAL] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::TextHot:
            out << "\t" << style_var << ".palette.ink[ST_HOT] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::TextPressed:
            out << "\t" << style_var << ".palette.ink[ST_PRESSED] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::TextDisabled:
            out << "\t" << style_var << ".palette.ink[ST_DISABLED] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::IconNormal:
            out << "\t" << style_var << ".palette.icon[ST_NORMAL] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::IconHot:
            out << "\t" << style_var << ".palette.icon[ST_HOT] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::IconPressed:
            out << "\t" << style_var << ".palette.icon[ST_PRESSED] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::IconDisabled:
            out << "\t" << style_var << ".palette.icon[ST_DISABLED] = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::ShadowEnabled:
            out << "\t" << style_var << ".metrics.shadow.enabled = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::ShadowDistance:
            out << "\t" << style_var << ".metrics.shadow.distance = "
                << max(0, (int)value) << ";\n";
            break;
        case UiDesignerButtonStyleField::ShadowOffsetX:
            out << "\t" << style_var << ".metrics.shadow.offset_x = "
                << (int)value << ";\n";
            break;
        case UiDesignerButtonStyleField::ShadowOffsetY:
            out << "\t" << style_var << ".metrics.shadow.offset_y = "
                << (int)value << ";\n";
            break;
        case UiDesignerButtonStyleField::ShadowAlpha:
            out << "\t" << style_var << ".metrics.shadow.alpha = "
                << max(0, min(255, (int)value)) << ";\n";
            break;
        case UiDesignerButtonStyleField::ShadowColor:
            out << "\t" << style_var << ".metrics.shadow.color = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::ShadowInset:
            out << "\t" << style_var << ".metrics.shadow.inset = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::ShadowMode:
            out << "\t" << style_var << ".metrics.shadow.mode = "
                << (AsString(value) == "Hard" ? "SHADOW_HARD" : "SHADOW_CURVE")
                << ";\n";
            break;
        case UiDesignerButtonStyleField::PressOffsetX:
            out << "\t" << style_var << ".press_offset.x = "
                << (int)value << ";\n";
            break;
        case UiDesignerButtonStyleField::PressOffsetY:
            out << "\t" << style_var << ".press_offset.y = "
                << (int)value << ";\n";
            break;
        case UiDesignerButtonStyleField::Overpaint:
            out << "\t" << style_var << ".overpaint = "
                << max(0, (int)value) << ";\n";
            break;
        case UiDesignerButtonStyleField::UnderlineEnabled:
            out << "\t" << style_var << ".underline = "
                << EmitValue(value) << ";\n";
            break;
        case UiDesignerButtonStyleField::UnderlineWidth:
            out << "\t" << style_var << ".underline_width = "
                << max(0, (int)value) << ";\n";
            break;
        case UiDesignerButtonStyleField::UnderlineOffset:
            out << "\t" << style_var << ".underline_offset = "
                << (int)value << ";\n";
            break;
        default:
            break;
        }
    };

    if(spec.runtime_kind == UiDesignerRuntimeKind::UiColorPicker) {
        out << "\t" << member << ".SetColor(" << EmitValue(Effective("color", Color(58, 132, 255))) << ", false);\n";
        out << "\t" << member << ".SetAlpha(" << (int)Effective("alpha", 255) << ", false);\n";
        out << "\t" << member << ".SetAlphaEnabled(" << EmitValue(Effective("alpha_enabled", true)) << ");\n";
        const String page = AsString(Effective("page_mode", "color"));
        const String channel = AsString(Effective("channel_mode", "rgb_float"));
        const String spectrum = AsString(Effective("spectrum_mode", "hue_strip"));
        const String harmony = AsString(Effective("harmony_mode", "triad"));
        const char *pages[] = {"PAGE_COLOR", "PAGE_PALETTES", "PAGE_GENERATOR"};
        int pi = page == "palettes" ? 1 : page == "generator" ? 2 : 0;
        const char *channels[] = {"CHANNEL_RGB_FLOAT", "CHANNEL_RGB_INT", "CHANNEL_HSV", "CHANNEL_HSL", "CHANNEL_TMI", "CHANNEL_CMYK", "CHANNEL_LAB"};
        const char *channel_names[] = {"rgb_float", "rgb_integer", "hsv", "hsl", "tmi", "cmyk", "lab"};
        int ci = 0; while(ci < 7 && channel != channel_names[ci]) ci++;
        const char *spectra[] = {"SPECTRUM_HSV_RECT", "SPECTRUM_HUE_STRIP", "SPECTRUM_RGB_SPECTRUM", "SPECTRUM_HSV_WHEEL"};
        const char *spectrum_names[] = {"hsv_rectangle", "hue_strip", "rgb_spectrum", "hsv_wheel"};
        int si = 0; while(si < 4 && spectrum != spectrum_names[si]) si++;
        const char *harmonies[] = {"HARMONY_CUSTOM", "HARMONY_ANALOGOUS", "HARMONY_COMPLEMENTARY", "HARMONY_SPLIT_COMPLEMENTARY", "HARMONY_TRIAD", "HARMONY_SQUARE", "HARMONY_COMPOUND", "HARMONY_SHADES", "HARMONY_MONOCHROMATIC", "HARMONY_IMAGE_EXTRACT"};
        const char *harmony_names[] = {"custom", "analogous", "complementary", "split_complementary", "triad", "square", "compound", "shades", "monochromatic", "image_extract"};
        int hi = 0; while(hi < 10 && harmony != harmony_names[hi]) hi++;
        out << "\t" << member << ".SetPageMode(UiColorPicker::" << pages[pi] << ");\n";
        out << "\t" << member << ".SetChannelMode(UiColorPicker::" << channels[min(ci, 6)] << ");\n";
        out << "\t" << member << ".SetSpectrumMode(UiColorPicker::" << spectra[min(si, 3)] << ");\n";
        out << "\t" << member << ".SetHarmonyMode(UiColorPicker::" << harmonies[min(hi, 9)] << ");\n";
        out << "\t" << member << ".SetSlotCount(" << max(1, (int)Effective("slot_count", 4)) << ");\n";
        out << "\t" << member << ".SetActiveSlot(" << max(0, (int)Effective("active_slot", 0)) << ");\n";
    }
    if(spec.FindProperty("role")) {
        const String role = AsString(Property("role", "Standard"));
        if(const UiDesignerThemeAdapter* adapter = UiDesignerGetThemeAdapter(spec)) {
            if(spec.theme && adapter->Supports(spec.runtime_kind)) {
                adapter->EmitSetup(out, member, node, spec);
            }
        }
        else if(spec.runtime_kind == UiDesignerRuntimeKind::UiPanel)
            out << "\t" << member << ".SetCustomStyle(UiTheme::ResolvePanel("
                << EmitRoleExpr(role) << "));\n";
        else if(spec.runtime_kind == UiDesignerRuntimeKind::UiLabel)
            out << "\t" << member << ".SetCustomStyle(UiTheme::ResolveLabel("
                << EmitRoleExpr(role) << "));\n";
        else if(spec.runtime_kind == UiDesignerRuntimeKind::UiGroupPanel)
            out << "\t" << member << ".SetCustomStyle(UiTheme::ResolveGroupPanel("
                << EmitRoleExpr(role) << "));\n";
    }

    if(spec.runtime_kind == UiDesignerRuntimeKind::UiBoxLayout) {
        const String direction = Property("direction", "V");
        out << "\t" << member << ".SetDirection(UiDirection::"
            << (direction == "H" ? "H" : "V") << ");\n";
    }
    if(spec.runtime_kind == UiDesignerRuntimeKind::UiGridLayout)
        out << "\t" << member << ".SetGridSize("
            << (int)Property("columns", 2) << ", "
            << (int)Property("rows", 2) << ");\n";
    if(spec.runtime_kind == UiDesignerRuntimeKind::UiGridLayout)
        out << "\t" << member << ".SetMinCellSize(Size(DPI("
            << max(0, (int)Property("min_cell_width", 10)) << "), DPI("
            << max(0, (int)Property("min_cell_height", 10)) << "));\n";
    if(spec.runtime_kind == UiDesignerRuntimeKind::UiBoxLayout ||
       spec.runtime_kind == UiDesignerRuntimeKind::UiGridLayout)
        out << "\t" << member << ".SetInset(DPI("
            << max(0, (int)Property("inset", 0)) << "));\n";
    if(spec.runtime_kind == UiDesignerRuntimeKind::UiBoxLayout ||
       spec.runtime_kind == UiDesignerRuntimeKind::UiGridLayout)
        out << "\t" << member << ".SetGap(DPI("
            << max(0, (int)Property("gap", 0)) << "));\n";

    if(spec.FindProperty("tooltip"))
        out << "\t" << member << ".Tip(" << EmitValue(
            Effective("tooltip", spec.FindProperty("tooltip")->default_value))
            << ");\n";
    if(spec.FindProperty("icon")) {
        const String icon_name = Effective("icon", spec.FindProperty("icon")->default_value);
        if(IsButtonFamily(spec.runtime_kind))
            out << "\t" << member << ".SetIcon(" << EmitButtonIcon(icon_name) << ");\n";
        else if(icon_name == "ICON_DESIGN_DESCRIPTION_48")
            out << "\t" << member
                << ".SetMedia(ICON_DESIGN_DESCRIPTION_48(), Size(DPI(18), DPI(18)));\n";
    }
    if(spec.FindProperty("icon_render_mode") && IsButtonFamily(spec.runtime_kind))
        out << "\t" << member << ".SetIconRenderMode(" << EmitIconRenderMode(
            AsString(Effective("icon_render_mode", "MonoTint"))) << ");\n";
    if(spec.FindProperty("icon_side"))
        out << "\t" << member << ".SetIconSide(" << EmitAlign(
            AsString(Effective("icon_side", "Left"))) << ");\n";
    if(spec.FindProperty("icon_width") || spec.FindProperty("icon_height")) {
        const int width = max(0, (int)Effective("icon_width", 18));
        const int height = max(0, (int)Effective("icon_height", 18));
        out << "\t" << member << ".SetIconSize(DPI(" << width << "), DPI(" << height << "));\n";
    }
    if(spec.FindProperty("scale_icon_to_content") && IsButtonFamily(spec.runtime_kind))
        out << "\t" << member << ".SetIconScaleToContent("
            << EmitValue(Effective("scale_icon_to_content", false)) << ");\n";
    if(spec.FindProperty("align_h") || spec.FindProperty("align_v")) {
        const String align_h = AsString(Effective("align_h", "Center"));
        const String align_v = AsString(Effective("align_v", "Center"));
        if(IsButtonFamily(spec.runtime_kind))
            out << "\t" << member << ".SetAlign(" << EmitAlign(align_h) << ", "
                << EmitAlign(align_v) << ");\n";
    }
    if(spec.FindProperty("content_gap"))
        out << "\t" << member << ".SetContentGap(DPI("
            << max(0, (int)Effective("content_gap", 4)) << "));\n";
    if(spec.FindProperty("content_inset_left") ||
       spec.FindProperty("content_inset_top") ||
       spec.FindProperty("content_inset_right") ||
       spec.FindProperty("content_inset_bottom")) {
        out << "\t" << member << ".SetContentInset(Rect(DPI("
            << max(0, (int)Effective("content_inset_left", 4)) << "), DPI("
            << max(0, (int)Effective("content_inset_top", 4)) << "), DPI("
            << max(0, (int)Effective("content_inset_right", 4)) << "), DPI("
            << max(0, (int)Effective("content_inset_bottom", 4)) << ")));\n";
    }
    if(spec.FindProperty("click_focus") && IsButtonFamily(spec.runtime_kind))
        out << "\t" << member << ".ClickFocus("
            << EmitValue(Effective("click_focus", true)) << ");\n";

    if(const UiDesignerPropertySpec* text = spec.FindProperty("text")) {
        const Value value = Effective("text", text->default_value);
        if(!IsNull(value)) {
            if(UsesLabelSetter(spec.runtime_kind))
                out << "\t" << member << ".SetLabel(" << EmitValue(value) << ");\n";
            else
                out << "\t" << member << ".SetText(" << EmitValue(value) << ");\n";
        }
    }
    if(const UiDesignerPropertySpec* title = spec.FindProperty("title"))
        out << "\t" << member << ".SetTitle("
            << EmitValue(Effective("title", title->default_value))
            << ");\n";
    if(spec.FindProperty("subtitle"))
        out << "\t" << member << ".SetSubTitle("
            << EmitValue(Effective("subtitle", "Supporting information"))
            << ");\n";
    if(spec.FindProperty("copy"))
        out << "\t" << member << ".SetCopyText("
            << EmitValue(Effective("copy", "Add a short description or place content in the card."))
            << ");\n";
    if(spec.FindProperty("text_align_h") || spec.FindProperty("text_align_v")) {
        const String align_h = AsString(Effective("text_align_h", "Left"));
        const String align_v = AsString(Effective("text_align_v", "Center"));
        if(spec.runtime_kind == UiDesignerRuntimeKind::UiTitleCard)
            out << "\t" << member << ".SetTextAlign(" << EmitAlign(align_h)
                << ", " << EmitAlign(align_v) << ");\n";
    }
    if(spec.FindProperty("media_side"))
        out << "\t" << member << ".SetMediaSide("
            << EmitAlign(AsString(Effective("media_side", "Left"))) << ");\n";
    if(spec.FindProperty("media_align_h") || spec.FindProperty("media_align_v")) {
        const String align_h = AsString(Effective("media_align_h", "Center"));
        const String align_v = AsString(Effective("media_align_v", "Center"));
        if(spec.runtime_kind == UiDesignerRuntimeKind::UiTitleCard)
            out << "\t" << member << ".SetMediaAlign(" << EmitAlign(align_h)
                << ", " << EmitAlign(align_v) << ");\n";
    }
    if(spec.FindProperty("media_reserve"))
        out << "\t" << member << ".SetMediaReserve("
            << max(0, (int)Effective("media_reserve", 72)) << ");\n";
    if(spec.FindProperty("media_min"))
        out << "\t" << member << ".SetMediaMin("
            << max(0, (int)Effective("media_min", 24)) << ");\n";
    if(spec.FindProperty("media_gap"))
        out << "\t" << member << ".SetMediaGap("
            << max(0, (int)Effective("media_gap", 10)) << ");\n";
    if(spec.FindProperty("media_auto_fit"))
        out << "\t" << member << ".SetMediaAutoFit("
            << EmitValue(Effective("media_auto_fit", true)) << ");\n";
    if(spec.FindProperty("media_share_percent"))
        out << "\t" << member << ".SetMediaSharePercent("
            << max(0, (int)Effective("media_share_percent", 0)) << ");\n";
    if(spec.FindProperty("content_inset") && spec.runtime_kind == UiDesignerRuntimeKind::UiTitleCard)
        out << "\t" << member << ".SetContentInset("
            << max(0, (int)Effective("content_inset", 8)) << ");\n";
    if(spec.FindProperty("content_cell_gap"))
        out << "\t" << member << ".SetContentCellGap("
            << max(0, (int)Effective("content_cell_gap", 8)) << ");\n";
    if(spec.FindProperty("show_title_line"))
        out << "\t" << member << ".ShowTitleLine("
            << EmitValue(Effective("show_title_line", true)) << ");\n";
    if(spec.FindProperty("title_line_length") ||
       spec.FindProperty("title_line_thickness") ||
       spec.FindProperty("title_line_style")) {
        out << "\t" << member << ".SetTitleLine("
            << EmitUiSpan(AsString(Effective("title_line_length", "Large"))) << ", "
            << max(0, (int)Effective("title_line_thickness", 1)) << ", "
            << EmitUiLineStyle(AsString(Effective("title_line_style", "Solid"))) << ");\n";
    }
    if(spec.FindProperty("show_card_line"))
        out << "\t" << member << ".ShowCardLine("
            << EmitValue(Effective("show_card_line", false)) << ");\n";
    if(spec.FindProperty("card_line_side"))
        out << "\t" << member << ".SetCardLineSide("
            << EmitAlign(AsString(Effective("card_line_side", "Bottom"))) << ");\n";
    if(spec.FindProperty("card_line_length") ||
       spec.FindProperty("card_line_thickness"))
        out << "\t" << member << ".SetCardLine("
            << EmitUiSpan(AsString(Effective("card_line_length", "Large"))) << ", "
            << max(0, (int)Effective("card_line_thickness", 1)) << ");\n";
    if(spec.FindProperty("card_line_gap"))
        out << "\t" << member << ".SetCardLineGap("
            << max(0, (int)Effective("card_line_gap", 0)) << ");\n";
    if(spec.FindProperty("hover_enabled"))
        out << "\t" << member << ".EnableHover("
            << EmitValue(Effective("hover_enabled", false)) << ");\n";
    if(spec.FindProperty("selectable"))
        out << "\t" << member << ".SetSelectable("
            << EmitValue(Effective("selectable", true)) << ");\n";
    if(spec.FindProperty("checkable"))
        out << "\t" << member << ".SetCheckable("
            << EmitValue(Effective("checkable", false)) << ");\n";
    if(spec.FindProperty("checked")) {
        if(IsButtonFamily(spec.runtime_kind))
            out << "\t" << member << ".SetChecked("
                << EmitValue(Effective("checked", false)) << ");\n";
        else
            out << "\t" << member << ".SetData("
                << EmitValue(Effective("checked", false)) << ");\n";
    }
    if(spec.FindProperty("value")) {
        const Value value = Effective("value", 50);
        if(spec.runtime_kind == UiDesignerRuntimeKind::UiSlider)
            out << "\t" << member << ".SetRange("
                << EmitValue(Effective("minimum", 0)) << ", "
                << EmitValue(Effective("maximum", 100))
                << ").SetValue(" << EmitValue(value) << ");\n";
        else if(spec.runtime_kind == UiDesignerRuntimeKind::UiProgressBar)
            out << "\t" << member << ".Set((int)"
                << EmitValue(value) << ", (int)"
                << EmitValue(Effective("maximum", 100)) << ");\n";
        else
            out << "\t" << member << ".SetData(" << EmitValue(value) << ");\n";
    }
    if(spec.FindProperty("color"))
        out << "\t" << member << ".SetData("
            << EmitValue(Effective("color", Color(58, 132, 255)))
            << ");\n";
    if(spec.FindProperty("visible"))
        out << "\t" << member << ".Show("
            << EmitValue(Effective("visible", true)) << ");\n";
    if(spec.FindProperty("enabled"))
        out << "\t" << member << ".Enable("
            << EmitValue(Effective("enabled", true)) << ");\n";

    const int x = Effective("x", 20);
    const int y = Effective("y", 20);
    const int cx = Effective("width", spec.default_size.cx);
    const int cy = Effective("height", spec.default_size.cy);
    out << "\t" << member << ".SetRect(DPI(" << x << "), DPI(" << y
        << "), DPI(" << cx << "), DPI(" << cy << "));\n";
}

void UiDesignerCodeGenerator::EmitSpacer(
    String& out, const UiDesignerNode& spacer,
    const UiDesignerNode& parent) const
{
    const String p = MemberName(parent);
    const bool is_break = spacer.GetProperty("layout_break", false);
    String chain;
    bool grid = false;

    if(parent.type == "UiBoxLayout") {
        chain = p + (is_break ? ".AddBreak()" : ".AddSpacer()");
        if(!is_break) {
            const bool horizontal = (String)parent.GetProperty("direction", "V") == "H";
            const String main_mode = spacer.GetProperty(
                horizontal ? "h_sizing" : "v_sizing", "Auto");
            const String cross_mode = spacer.GetProperty(
                horizontal ? "v_sizing" : "h_sizing", "Auto");
            const int fixed_main = spacer.GetProperty(
                horizontal ? "fixed_width" : "fixed_height", 0);
            const int fixed_cross = spacer.GetProperty(
                horizontal ? "fixed_height" : "fixed_width", 0);
            const int min_main = spacer.GetProperty(
                horizontal ? "min_width" : "min_height", 0);
            const int max_main = spacer.GetProperty(
                horizontal ? "max_width" : "max_height", 0);
            const int min_cross = spacer.GetProperty(
                horizontal ? "min_height" : "min_width", 0);
            const int max_cross = spacer.GetProperty(
                horizontal ? "max_height" : "max_width", 0);
            const int weight = max(1, (int)(double)spacer.GetProperty("weight", 1.0));

            if(main_mode == "Fixed" && fixed_main > 0)
                chain << ".Fixed(DPI(" << fixed_main << "))";
            else
                chain << ".Expand(" << weight << ")";
            if(min_main || max_main)
                chain << ".MinMaxMain(DPI(" << min_main << "), "
                      << (max_main ? "DPI(" + AsString(max_main) + ")" : "INT_MAX")
                      << ")";
            if(cross_mode == "Fixed" && fixed_cross > 0)
                chain << ".MinMaxCross(DPI(" << fixed_cross << "), DPI("
                      << fixed_cross << "))";
            else if(min_cross || max_cross)
                chain << ".MinMaxCross(DPI(" << min_cross << "), "
                      << (max_cross ? "DPI(" + AsString(max_cross) + ")" : "INT_MAX")
                      << ")";
            if(cross_mode == "Fill")
                chain << ".AlignSelf(UiCrossAlign::Stretch)";
        }
    }
    else if(parent.type == "UiGridLayout") {
        grid = true;
        const int row = spacer.GetProperty("grid_row", 0);
        const int column = spacer.GetProperty("grid_column", 0);
        chain = p + ".AddBlank(" + AsString(row) + ", " + AsString(column) + ")";
        const String hs = spacer.GetProperty("h_sizing", "Auto");
        const String vs = spacer.GetProperty("v_sizing", "Auto");
        if(hs == "Fill") chain << ".ExpandX()";
        if(vs == "Fill") chain << ".ExpandY()";
        const int fixedw = spacer.GetProperty("fixed_width", 0);
        const int fixedh = spacer.GetProperty("fixed_height", 0);
        const int minw = spacer.GetProperty("min_width", 0);
        const int minh = spacer.GetProperty("min_height", 0);
        const int maxw = spacer.GetProperty("max_width", 0);
        const int maxh = spacer.GetProperty("max_height", 0);
        if(hs == "Fixed" && fixedw) chain << ".FixedWidth(DPI(" << fixedw << "))";
        if(vs == "Fixed" && fixedh) chain << ".FixedHeight(DPI(" << fixedh << "))";
        if(minw) chain << ".MinWidth(DPI(" << minw << "))";
        if(minh) chain << ".MinHeight(DPI(" << minh << "))";
        if(maxw) chain << ".MaxWidth(DPI(" << maxw << "))";
        if(maxh) chain << ".MaxHeight(DPI(" << maxh << "))";
    }
    else {
        out << "\t// Unsupported Spacer parent: " << parent.type << "\n";
        return;
    }

    if(spacer.GetProperty("line_enabled", false)) {
        const String align = spacer.GetProperty("line_align", "Center");
        chain << ".LineEnabled()"
              << ".LineOrientation("
              << LineOrientationExpr(spacer.GetProperty("line_orientation", "Horizontal"))
              << ")"
              << ".LineAlign(" << (grid ? GridAlignExpr(align) : BoxAlignExpr(align)) << ")"
              << ".LineThickness(DPI("
              << (int)spacer.GetProperty("line_thickness", 1) << "))"
              << ".LineDash("
              << LineDashExpr(spacer.GetProperty("line_dash", "Solid")) << ")"
              << ".LineInset(DPI("
              << (int)spacer.GetProperty("line_inset", 0) << "))";
        if(spacer.GetProperty("line_color_enabled", false))
            chain << ".LineColorEnabled().LineColor("
                  << EmitColor(spacer.GetProperty("line_color", Color(128, 128, 128)))
                  << ")";
    }
    out << "\t" << chain << ";\n";
}

struct UiDesignerChildAttachContext {
    String& out;
    const String& parent;
    const String& member;
    const UiDesignerNode& child;
    const String& title;
};

typedef void (*UiDesignerChildAttachFn)(UiDesignerChildAttachContext&);

static void AttachRoot(UiDesignerChildAttachContext& c)
{
    if(c.child.GetProperty("width_mode", "Expand") == "Expand" &&
       c.child.GetProperty("height_mode", "Expand") == "Expand")
        c.out << "\tAdd(" << c.member << ".SizePos());\n";
    else
        c.out << "\tAdd(" << c.member << ");\n";
}

static void AttachAdd(UiDesignerChildAttachContext& c)
{
    c.out << "\t" << c.parent << ".Add(" << c.member << ");\n";
}

static void AttachBox(UiDesignerChildAttachContext& c)
{
    c.out << "\t" << c.parent << ".Add(" << c.member << ").Fit();\n";
}

static void AttachGrid(UiDesignerChildAttachContext& c)
{
    c.out << "\t" << c.parent << ".Add(" << c.member << ", "
          << (int)c.child.GetProperty("grid_row", 0) << ", "
          << (int)c.child.GetProperty("grid_column", 0) << ", true);\n";
}

static void AttachAbsolute(UiDesignerChildAttachContext& c)
{
    c.out << "\t" << c.parent << ".Add(" << c.member
          << ", DPI(" << (int)c.child.GetProperty("x", 20)
          << "), DPI(" << (int)c.child.GetProperty("y", 20)
          << "), DPI(" << max(0, (int)c.child.GetProperty("width", 160))
          << "), DPI(" << max(0, (int)c.child.GetProperty("height", 32))
          << "));\n";
}

static void AttachTab(UiDesignerChildAttachContext& c)
{
    c.out << "\t" << c.parent << ".Add(" << c.member << ", "
          << CppString(c.title) << ");\n";
}

static void AttachStack(UiDesignerChildAttachContext& c)
{
    c.out << "\t" << c.parent << ".Add(" << c.member << ", "
          << CppString(c.child.name) << ");\n";
}

static void AttachAccordion(UiDesignerChildAttachContext& c)
{
    const String section = "section_" + AsString(c.child.id);
    c.out << "\tconst int " << section << " = " << c.parent
          << ".AddSection(" << CppString(c.title) << ", true);\n"
          << "\t" << c.parent << ".GetSectionContent(" << section
          << ").Add(" << c.member << ".SizePos());\n";
}

static void AttachSplitter(UiDesignerChildAttachContext& c)
{
    c.out << "\t" << c.parent << " << " << c.member << ";\n";
}

struct UiDesignerChildAdapterEntry {
    const char *id;
    UiDesignerChildAttachFn emit;
};

static const UiDesignerChildAdapterEntry *FindChildAdapter(const String& id)
{
    static const UiDesignerChildAdapterEntry adapters[] = {
        {"root", AttachRoot},
        {"add", AttachAdd},
        {"single", AttachAdd},
        {"box", AttachBox},
        {"grid", AttachGrid},
        {"absolute", AttachAbsolute},
        {"tab", AttachTab},
        {"upp_tab", AttachTab},
        {"stack", AttachStack},
        {"accordion", AttachAccordion},
        {"splitter", AttachSplitter},
        {"quad", AttachSplitter},
        {"upp_splitter", AttachSplitter},
    };
    for(const auto& adapter : adapters)
        if(id == adapter.id)
            return &adapter;
    return nullptr;
}

void UiDesignerCodeGenerator::EmitChildren(
    String& out, const UiDesignerDocument& document,
    const UiDesignerNode& node) const
{
    const String parent = node.id == document.GetRootId()
        ? String() : MemberName(node);
    const UiDesignerControlSpec* parent_spec =
        node.id == document.GetRootId() ? nullptr : catalog_.Find(node.type);
    const String adapter_id = parent_spec ? parent_spec->child_adapter_id : "root";
    const UiDesignerChildAdapterEntry *adapter = FindChildAdapter(adapter_id);

    for(UiDesignerNodeId child_id : node.children) {
        const UiDesignerNode* child = document.Find(child_id);
        if(!child)
            continue;
        const UiDesignerControlSpec* child_spec = catalog_.Find(child->type);
        if(!child_spec)
            continue;
        if(child_spec->IsSemanticItem()) {
            EmitSpacer(out, *child, node);
            continue;
        }

        const String member = MemberName(*child);
        const String title = child->GetProperty(
            "title", child->GetProperty("text", child->name));
        if(adapter) {
            UiDesignerChildAttachContext context{out, parent, member, *child, title};
            adapter->emit(context);
        }
        else
            out << "\t#error Unsupported UiDesigner child adapter "
                << CppString(adapter_id) << "\n";
        EmitChildren(out, document, *child);
    }
}

static String EventLambdaPrefix(const String& event_id)
{
    if(event_id == "WhenSelect")
        return "[=](int, const Value&)";
    if(event_id == "WhenPageChanged" || event_id == "WhenPageMoved" ||
       event_id == "WhenReordered")
        return "[=](int, int)";
    if(event_id == "WhenSectionToggled")
        return "[=](int, bool)";
    if(event_id == "WhenClose" || event_id == "WhenRemoved" ||
       event_id == "WhenAdded" || event_id == "WhenPageRemoved")
        return "[=](int)";
    return "[=]";
}

static String HandlerIdentifier(const String& name)
{
    return SanitizeIdentifier(name.IsEmpty() ? "OnGeneratedAction" : name);
}

void UiDesignerCodeGenerator::EmitBinding(
    String& out, const UiDesignerDocument& document,
    const UiDesignerNode& node,
    const UiDesignerActionBinding& binding) const
{
    if(!binding.enabled)
        return;
    const String member = MemberName(node);
    String body;
    const UiDesignerNode* target = binding.target
        ? document.Find(binding.target) : nullptr;
    const UiDesignerControlSpec* target_spec = target
        ? catalog_.Find(target->type) : nullptr;
    const String target_member = target ? MemberName(*target) : String();

    switch(binding.action) {
    case UiDesignerActionType::CloseWindow:
    case UiDesignerActionType::ExitApplication:
        body = "Close();";
        break;
    case UiDesignerActionType::AcceptDialog:
        body = "Break(IDOK);";
        break;
    case UiDesignerActionType::CancelDialog:
        body = "Break(IDCANCEL);";
        break;
    case UiDesignerActionType::SetProperty:
        if(!target)
            body = "/* Missing SetProperty target */";
        else if(binding.target_property == "visible")
            body = target_member + ".Show(" + EmitValue(binding.value) + ");";
        else if(binding.target_property == "enabled")
            body = target_member + ".Enable(" + EmitValue(binding.value) + ");";
        else if(binding.target_property == "text" && target_spec &&
                UsesLabelSetter(target_spec->runtime_kind))
            body = target_member + ".SetLabel(" + EmitValue(binding.value) + ");";
        else if(binding.target_property == "text")
            body = target_member + ".SetText(" + EmitValue(binding.value) + ");";
        else
            body = target_member + ".SetData(" + EmitValue(binding.value) + ");";
        break;
    case UiDesignerActionType::ToggleProperty:
        if(!target)
            body = "/* Missing ToggleProperty target */";
        else if(binding.target_property == "visible")
            body = target_member + ".Show(!" + target_member + ".IsShown());";
        else if(binding.target_property == "enabled")
            body = target_member + ".Enable(!" + target_member + ".IsEnabled());";
        else
            body = target_member + ".SetData(!(bool)" + target_member + ".GetData());";
        break;
    case UiDesignerActionType::AdjustValue:
        body = target
            ? target_member + ".SetData((double)" + target_member +
              ".GetData() + " + Format("%.12g", binding.delta) + ");"
            : "/* Missing AdjustValue target */";
        break;
    case UiDesignerActionType::ActivatePage:
        body = target
            ? target_member + ".SetData(" + EmitValue(binding.value) + ");"
            : "/* Missing ActivatePage target */";
        break;
    case UiDesignerActionType::CallNamedHandler:
        body = HandlerIdentifier(binding.handler_name) + "();";
        break;
    }

    String event = binding.event_id;
    if(node.type == "UiStack" && event == "WhenAction")
        event = "WhenPageChanged";
    else if(node.type == "UiAccordion" && event == "WhenAction")
        event = "WhenSectionToggled";

    out << "\t" << member << "." << event << " = "
        << EventLambdaPrefix(event) << " { " << body << " };\n";
}

Vector<String> UiDesignerCodeGenerator::CollectHandlers(
    const UiDesignerDocument& document) const
{
    Index<String> handlers;
    for(const UiDesignerNode& node : document.GetNodes())
        for(const UiDesignerActionBinding& binding : node.actions)
            if(binding.action == UiDesignerActionType::CallNamedHandler &&
               !binding.handler_name.IsEmpty())
                handlers.FindAdd(HandlerIdentifier(binding.handler_name));
    Vector<String> result;
    for(int i = 0; i < handlers.GetCount(); i++)
        result.Add(handlers[i]);
    Sort(result);
    return result;
}

String UiDesignerCodeGenerator::GenerateHeader(
    const UiDesignerDocument& document, const String& class_name) const
{
    UiDesignerCodeGenerationOptions options;
    options.package_name = class_name;
    options.class_name = class_name;
    return Generate(document, options).generated_header;
}

String UiDesignerCodeGenerator::GenerateSource(
    const UiDesignerDocument& document, const String& class_name) const
{
    UiDesignerCodeGenerationOptions options;
    options.package_name = class_name;
    options.class_name = class_name;
    return Generate(document, options).generated_source;
}

String UiDesignerCodeGenerator::GeneratePackage(const String& class_name) const
{
    return "description \"Generated UiDesigner application\";\n\n"
           "uses\n\tCtrlLib,\n\tUi;\n\n"
           "mainconfig\n\t\"GUI\" = \"1\";\n\n"
           "file\n\t" + class_name + ".generated.h,\n\t" +
           class_name + ".generated.cpp,\n\t" + class_name + ".h,\n\t" +
           class_name + ".cpp,\n\tmain.cpp;\n";
}

UiDesignerGeneratedProject UiDesignerCodeGenerator::Generate(
    const UiDesignerDocument& document, const String& class_name) const
{
    UiDesignerCodeGenerationOptions options;
    options.package_name = class_name;
    options.class_name = class_name;
    return Generate(document, options);
}

UiDesignerGeneratedProject UiDesignerCodeGenerator::Generate(
    const UiDesignerDocument& document,
    const UiDesignerCodeGenerationOptions& options) const
{
    UiDesignerGeneratedProject result;
    String error;
    if(!UiDesignerValidateGenerationOptions(options, error)) {
        result.diagnostics.Add(error);
        return result;
    }
    if(!catalog_.ValidateDocument(document, error)) {
        result.diagnostics.Add(error);
        return result;
    }
    for(const UiDesignerNode& node : document.GetNodes()) {
        if(node.id == document.GetRootId())
            continue;
        const UiDesignerControlSpec* spec = catalog_.Find(node.type);
        if(!spec)
            continue;
        if(spec->codegen_adapter_id != "control" &&
           spec->codegen_adapter_id != "spacer") {
            result.diagnostics.Add("Unsupported code-generation adapter: " +
                                   spec->codegen_adapter_id);
            return result;
        }
        if((spec->node_flags & UiDesignerNodeContainer) &&
           !FindChildAdapter(spec->child_adapter_id)) {
            result.diagnostics.Add("Unsupported child adapter: " +
                                   spec->child_adapter_id);
            return result;
        }
    }

    const String base = options.class_name + "Generated";
    const String guard = "_Generated_" +
        SanitizeIdentifier(options.namespace_name + "_" + options.class_name) + "_h_";
    const Vector<String> handlers = CollectHandlers(document);

    String gh;
    gh << "#ifndef " << guard << "\n#define " << guard << "\n\n"
       << "#include <CtrlLib/CtrlLib.h>\n#include <Ui/Ui.h>\n"
       << "#include <Ui/UiColorPicker.h>\n\n"
       << NamespaceOpen(options.namespace_name)
       << "class " << base << " : public TopWindow {\n"
       << "public:\n\ttypedef " << base << " CLASSNAME;\n"
       << "\tvoid BuildGeneratedUi();\n\n"
       << "protected:\n\tvirtual void BindActions() {}\n";
    for(const String& handler : handlers)
        gh << "\tvirtual void " << handler << "() {}\n";
    gh << "\n\tvoid BuildControls();\n\tvoid BuildLayout();\n"
       << "\tvoid BindGeneratedActions();\n\n";
    for(const UiDesignerNode& node : document.GetNodes()) {
        if(node.id == document.GetRootId())
            continue;
        const UiDesignerControlSpec* spec = catalog_.Find(node.type);
        if(!spec || spec->IsSemanticItem())
            continue;
        gh << "\t" << spec->runtime_cpp_type << " " << MemberName(node) << ";\n";
    }
    gh << "};\n" << NamespaceClose(options.namespace_name) << "\n#endif\n";

    String gs;
    gs << "#include \"" << options.class_name << ".generated.h\"\n\n"
       << NamespaceOpen(options.namespace_name)
       << "void " << base << "::BuildGeneratedUi()\n{\n"
       << "\tTitle(" << CppString(options.class_name) << ").Sizeable().Zoomable();\n"
       << "\tSetRect(0, 0, DPI(" << document.GetVirtualSize().cx
       << "), DPI(" << document.GetVirtualSize().cy << "));\n"
       << "\tBuildControls();\n\tBuildLayout();\n\tBindGeneratedActions();\n}\n\n"
       << "void " << base << "::BuildControls()\n{\n";
    for(const UiDesignerNode& node : document.GetNodes()) {
        if(node.id == document.GetRootId())
            continue;
        const UiDesignerControlSpec* spec = catalog_.Find(node.type);
        if(spec)
            EmitSetup(gs, node, *spec);
    }
    gs << "}\n\nvoid " << base << "::BuildLayout()\n{\n";
    if(const UiDesignerNode* root = document.Find(document.GetRootId()))
        EmitChildren(gs, document, *root);
    gs << "}\n\nvoid " << base << "::BindGeneratedActions()\n{\n";
    for(const UiDesignerNode& node : document.GetNodes())
        for(const UiDesignerActionBinding& binding : node.actions)
            EmitBinding(gs, document, node, binding);
    gs << "}\n" << NamespaceClose(options.namespace_name);

    const String user_guard = "_" + SanitizeIdentifier(options.class_name) + "_h_";
    String uh;
    uh << "#ifndef " << user_guard << "\n#define " << user_guard << "\n\n"
       << "#include \"" << options.class_name << ".generated.h\"\n\n"
       << NamespaceOpen(options.namespace_name)
       << "class " << options.class_name << " : public " << base << " {\n"
       << "public:\n\ttypedef " << options.class_name << " CLASSNAME;\n"
       << "\t" << options.class_name << "();\n\n"
       << "protected:\n\tvoid BindActions() override;\n";
    for(const String& handler : handlers)
        uh << "\tvoid " << handler << "() override;\n";
    uh << "};\n" << NamespaceClose(options.namespace_name) << "\n#endif\n";

    String us;
    us << "#include \"" << options.class_name << ".h\"\n\n"
       << NamespaceOpen(options.namespace_name)
       << options.class_name << "::" << options.class_name << "()\n{\n"
       << "\tBuildGeneratedUi();\n\tBindActions();\n}\n\n"
       << "void " << options.class_name << "::BindActions()\n{\n"
       << "\t// Add application-owned event wiring here. This file is preserved.\n}\n";
    for(const String& handler : handlers)
        us << "\nvoid " << options.class_name << "::" << handler << "()\n{\n"
           << "\t// User-owned named handler.\n}\n";
    us << NamespaceClose(options.namespace_name);

    String main_cpp;
    main_cpp << "#include \"" << options.class_name << ".h\"\n"
             << "using namespace Upp;\n"
             << "GUI_APP_MAIN { "
             << (options.namespace_name.IsEmpty()
                    ? options.class_name
                    : options.namespace_name + "::" + options.class_name)
             << "().Run(); }\n";

    result.generated_header = gh;
    result.generated_source = gs;
    result.user_header = uh;
    result.user_source = us;
    result.main_source = main_cpp;
    result.header = gh;
    result.source = gs;
    result.package = GeneratePackage(options.class_name);
    result.json = UiDesignerSerialize(document, true);

    auto AddFile = [&](const String& path, const String& content, bool generated) {
        UiDesignerGeneratedFile& file = result.files.Add();
        file.relative_path = path;
        file.content = content;
        file.generator_owned = generated;
    };
    AddFile(options.class_name + ".generated.h", gh, true);
    AddFile(options.class_name + ".generated.cpp", gs, true);
    AddFile(options.class_name + ".h", uh, false);
    AddFile(options.class_name + ".cpp", us, false);
    AddFile("main.cpp", main_cpp, false);
    AddFile(options.package_name + ".upp", result.package, true);
    if(options.include_source_design)
        AddFile("design.json", result.json, true);
    return result;
}

static bool RemoveTree(const String& path)
{
    return !DirectoryExists(path) || DeleteFolderDeep(path);
}

struct UiDesignerPublishEntry : Moveable<UiDesignerPublishEntry> {
    String staged;
    String destination;
    String backup;
    String temporary;
    bool existed = false;
    bool touched = false;
};

static bool RestorePublished(Vector<UiDesignerPublishEntry>& entries,
                             String& diagnostic)
{
    bool ok = true;
    for(int i = entries.GetCount() - 1; i >= 0; i--) {
        UiDesignerPublishEntry& entry = entries[i];
        if(!entry.touched)
            continue;
        if(FileExists(entry.destination) && !FileDelete(entry.destination))
            ok = false;
        if(entry.existed && FileExists(entry.backup)) {
            RealizeDirectory(GetFileFolder(entry.destination));
            if(!FileCopy(entry.backup, entry.destination))
                ok = false;
        }
        if(FileExists(entry.temporary))
            FileDelete(entry.temporary);
    }
    if(!ok)
        diagnostic << "\nRollback was incomplete; inspect the export destination.";
    return ok;
}

bool UiDesignerWriteGeneratedProject(
    const String& folder, const UiDesignerGeneratedProject& project,
    const UiDesignerExportWriteOptions& options,
    Vector<String>& written_files, String& error)
{
    written_files.Clear();
    if(!project.IsValid()) {
        error = project.diagnostics.IsEmpty()
            ? "Generated project is invalid"
            : Join(project.diagnostics, "\n");
        return false;
    }
    if(folder.IsEmpty()) {
        error = "Export folder is empty";
        return false;
    }

    for(const UiDesignerGeneratedFile& file : project.files) {
        const String destination = AppendFileName(folder, file.relative_path);
        if(FileExists(destination)) {
            if(!file.generator_owned && options.preserve_user_files)
                continue;
            if(options.overwrite == UiDesignerOverwritePolicy::RefuseExisting) {
                error = "Export would replace existing file: " + destination;
                return false;
            }
        }
    }

    const bool folder_existed = DirectoryExists(folder);
    if(!RealizeDirectory(folder)) {
        error = "Unable to create export folder: " + folder;
        return false;
    }
    const String stage = AppendFileName(
        folder, ".uidesigner-stage-" + AsString(Uuid::Create()));
    if(!RealizeDirectory(stage)) {
        error = "Unable to create export staging directory";
        return false;
    }

    Vector<UiDesignerPublishEntry> entries;
    bool ok = true;
    for(int i = 0; i < project.files.GetCount(); i++) {
        const UiDesignerGeneratedFile& file = project.files[i];
        const String destination = AppendFileName(folder, file.relative_path);
        if(FileExists(destination) && !file.generator_owned &&
           options.preserve_user_files)
            continue;

        UiDesignerPublishEntry& entry = entries.Add();
        entry.staged = AppendFileName(stage, "new/" + file.relative_path);
        entry.destination = destination;
        entry.backup = AppendFileName(stage, "backup/" + AsString(i));
        entry.temporary = destination + ".uidesigner-tmp-" + AsString(Uuid::Create());
        entry.existed = FileExists(destination);

        RealizeDirectory(GetFileFolder(entry.staged));
        if(!SaveFile(entry.staged, file.content)) {
            error = "Unable to stage " + file.relative_path;
            ok = false;
            break;
        }
        if(entry.existed) {
            RealizeDirectory(GetFileFolder(entry.backup));
            if(!FileCopy(entry.destination, entry.backup)) {
                error = "Unable to back up " + entry.destination;
                ok = false;
                break;
            }
        }
    }

    if(ok) {
        for(UiDesignerPublishEntry& entry : entries) {
            RealizeDirectory(GetFileFolder(entry.destination));
            if(!FileCopy(entry.staged, entry.temporary)) {
                error = "Unable to prepare " + entry.destination;
                ok = false;
                break;
            }
            entry.touched = true;
            if(entry.existed && !FileDelete(entry.destination)) {
                error = "Unable to replace " + entry.destination;
                ok = false;
                break;
            }
            if(!FileMove(entry.temporary, entry.destination)) {
                error = "Unable to publish " + entry.destination;
                ok = false;
                break;
            }
            written_files.Add(entry.destination);
        }
    }

    if(!ok) {
        RestorePublished(entries, error);
        written_files.Clear();
    }
    RemoveTree(stage);
    if(!ok) {
        if(!folder_existed && DirectoryExists(folder))
            DeleteFolderDeep(folder);
        return false;
    }
    error.Clear();
    return true;
}

bool UiDesignerWriteGeneratedProject(
    const String& folder, const String& package_name,
    const UiDesignerGeneratedProject& project, String& error)
{
    (void)package_name;
    UiDesignerExportWriteOptions options;
    options.overwrite = UiDesignerOverwritePolicy::ReplaceGenerated;
    Vector<String> written;
    return UiDesignerWriteGeneratedProject(folder, project, options,
                                           written, error);
}

}
