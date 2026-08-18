#include "UiButtonDemo.h"

namespace Upp {
namespace {

const char *CodeStateId(int state)
{
    static const char *id[] = { "normal", "hot", "pressed", "disabled" };
    return id[minmax(state, 0, 3)];
}

const char *StateCode(int state)
{
    static const char *id[] = { "ST_NORMAL", "ST_HOT", "ST_PRESSED", "ST_DISABLED" };
    return id[minmax(state, 0, 3)];
}

String CppString(const String& value)
{
    String out = "\"";
    for(int i = 0; i < value.GetCount(); i++) {
        int c = value[i];
        if(c == '\\') out << "\\\\";
        else if(c == '"') out << "\\\"";
        else if(c == '\n') out << "\\n";
        else if(c == '\r') out << "\\r";
        else if(c == '\t') out << "\\t";
        else out.Cat(c);
    }
    return out << '"';
}

String CppColor(Color color)
{
    if(IsNull(color))
        return "Null";
    return Format("Color(%d, %d, %d)", color.GetR(), color.GetG(), color.GetB());
}

Value MapValue(const ValueMap& map, const String& key, const Value& fallback)
{
    int q = map.Find(key);
    return q >= 0 ? map.GetValue(q) : fallback;
}

String FillRecipeCode(const Value& value)
{
    if(!value.Is<ValueMap>())
        return "UiFill::None()";

    ValueMap recipe = value;
    String mode = AsString(MapValue(recipe, "mode", "None"));
    if(mode == "Solid")
        return "UiFill::Solid(" + CppColor(Color(MapValue(recipe, "solid", White()))) + ")";
    if(mode == "QuadGradient") {
        return Format("UiFill::ImageFill(MakeQuadGradientTile(DPI(%d), %s, %s, %s, %s, %d))",
                      max(8, (int)MapValue(recipe, "tile_size", 32)),
                      CppColor(Color(MapValue(recipe, "top_left", White()))),
                      CppColor(Color(MapValue(recipe, "top_right", White()))),
                      CppColor(Color(MapValue(recipe, "bottom_left", White()))),
                      CppColor(Color(MapValue(recipe, "bottom_right", White()))),
                      max(0, (int)MapValue(recipe, "blur", 0)));
    }
    return "UiFill::None()";
}

String AlignCode(const String& value)
{
    return "UiAlign::" + ToUpper(value);
}

String IconModeCode(const String& value)
{
    return "UiIconRenderMode::" + value;
}

} // namespace

void UiButtonDemo::UpdateGeneratedCode()
{
    String out;
    out << "#include <Ui/Ui.h>\n\n"
        << "using namespace Upp;\n\n"
        << "UiButton button;\n";

    bool have_overrides = false;
    for(int i = 0; i < pe_model_override.GetCount(); i++)
        if(pe_model_override[i].override_active) {
            have_overrides = true;
            break;
        }

    if(have_overrides) {
        out << "\nUiButton::Style style = UiTheme::ResolveButton();\n";
        for(int i = 0; i < 4; i++) {
            String state = CodeStateId(i);
            if(OverrideActive("face." + state))
                out << "style.palette.face[" << StateCode(i) << "] = "
                    << FillRecipeCode(OverrideValue("face." + state)) << ";\n";
            if(OverrideActive("frame." + state))
                out << "style.palette.frame[" << StateCode(i) << "] = "
                    << CppColor(Color(OverrideValue("frame." + state))) << ";\n";
            if(OverrideActive("ink." + state))
                out << "style.palette.ink[" << StateCode(i) << "] = "
                    << CppColor(Color(OverrideValue("ink." + state))) << ";\n";
            if(OverrideActive("icon." + state))
                out << "style.palette.icon[" << StateCode(i) << "] = "
                    << CppColor(Color(OverrideValue("icon." + state))) << ";\n";
        }

        auto emit_bool = [&](const char *id, const char *field) {
            if(OverrideActive(id))
                out << field << " = " << ((bool)OverrideValue(id) ? "true" : "false") << ";\n";
        };
        auto emit_int = [&](const char *id, const char *field, bool dpi = false) {
            if(OverrideActive(id)) {
                out << field << " = ";
                if(dpi) out << "DPI(";
                out << (int)OverrideValue(id);
                if(dpi) out << ")";
                out << ";\n";
            }
        };

        emit_int("radius", "style.metrics.radius", true);
        emit_bool("transparent", "style.transparent");
        emit_bool("high_contrast", "style.metrics.high_contrast");
        emit_bool("face_enabled", "style.metrics.face_enabled");
        emit_bool("frame_enabled", "style.metrics.frame_enabled");
        emit_int("frame_width", "style.metrics.frame_width", true);
        emit_bool("dashed", "style.metrics.dashed");
        if(OverrideActive("dash_pattern"))
            out << "style.metrics.dash_pattern = " << CppString(AsString(OverrideValue("dash_pattern"))) << ";\n";

        if(OverrideActive("font_face"))
            out << "style.font.FaceName(" << CppString(AsString(OverrideValue("font_face"))) << ");\n";
        if(OverrideActive("font_height"))
            out << "style.font.Height(" << (int)OverrideValue("font_height") << ");\n";
        if(OverrideActive("font_bold"))
            out << "style.font.Bold(" << ((bool)OverrideValue("font_bold") ? "true" : "false") << ");\n";
        if(OverrideActive("font_italic"))
            out << "style.font.Italic(" << ((bool)OverrideValue("font_italic") ? "true" : "false") << ");\n";

        emit_bool("focus_enabled", "style.metrics.focus_enabled");
        emit_int("focus_margin", "style.metrics.focus_margin", true);
        emit_int("focus_alpha", "style.metrics.focus_alpha");
        if(OverrideActive("focus_color"))
            out << "style.metrics.focus_color = " << CppColor(Color(OverrideValue("focus_color"))) << ";\n";

        emit_bool("shadow_enabled", "style.metrics.shadow.enabled");
        emit_int("shadow_distance", "style.metrics.shadow.distance", true);
        emit_int("shadow_x", "style.metrics.shadow.offset_x", true);
        emit_int("shadow_y", "style.metrics.shadow.offset_y", true);
        emit_int("shadow_alpha", "style.metrics.shadow.alpha");
        emit_bool("shadow_inset", "style.metrics.shadow.inset");
        if(OverrideActive("shadow_color"))
            out << "style.metrics.shadow.color = " << CppColor(Color(OverrideValue("shadow_color"))) << ";\n";
        if(OverrideActive("shadow_mode"))
            out << "style.metrics.shadow.mode = "
                << (AsString(OverrideValue("shadow_mode")) == "Hard" ? "SHADOW_HARD" : "SHADOW_CURVE")
                << ";\n";
        if(OverrideActive("shadow_curve")) {
            ValueArray v = OverrideValue("shadow_curve");
            if(v.GetCount() >= 4)
                out << Format("style.metrics.shadow.curve = ShadowCurve { %.4f, %.4f, %.4f, %.4f };\n",
                              (double)v[0], (double)v[1], (double)v[2], (double)v[3]);
        }

        emit_bool("highlight_enabled", "style.metrics.highlight.enabled");
        emit_int("highlight_thickness", "style.metrics.highlight.thickness", true);
        emit_int("highlight_x", "style.metrics.highlight.offset_x", true);
        emit_int("highlight_y", "style.metrics.highlight.offset_y", true);
        emit_int("highlight_alpha", "style.metrics.highlight.alpha");
        if(OverrideActive("highlight_color"))
            out << "style.metrics.highlight.color = "
                << CppColor(Color(OverrideValue("highlight_color"))) << ";\n";
        if(OverrideActive("highlight_style"))
            out << "style.metrics.highlight.style = "
                << ToUpper(AsString(OverrideValue("highlight_style"))) << ";\n";

        if(OverrideActive("skin_image") && !AsString(OverrideValue("skin_image")).IsEmpty())
            out << "style.skin.base = StreamRaster::LoadFileAny("
                << CppString(AsString(OverrideValue("skin_image"))) << ");\n";
        emit_bool("skin_enabled", "style.skin.enabled");
        if(OverrideActive("skin_mode"))
            out << "style.skin.image_mode = UiBackgroundImageMode::"
                << AsString(OverrideValue("skin_mode")) << ";\n";
        emit_int("skin_slice_left", "style.skin.slice.left", true);
        emit_int("skin_slice_top", "style.skin.slice.top", true);
        emit_int("skin_slice_right", "style.skin.slice.right", true);
        emit_int("skin_slice_bottom", "style.skin.slice.bottom", true);
        emit_int("skin_inset_left", "style.skin.content_inset.left", true);
        emit_int("skin_inset_top", "style.skin.content_inset.top", true);
        emit_int("skin_inset_right", "style.skin.content_inset.right", true);
        emit_int("skin_inset_bottom", "style.skin.content_inset.bottom", true);

        emit_int("press_offset_x", "style.press_offset.x", true);
        emit_int("press_offset_y", "style.press_offset.y", true);

        out << "button.SetCustomStyle(style);\n";
    }

    out << "\nbutton.SetText(" << CppString(AsString(InspectorValue("text"))) << ");\n"
        << "button.Tip(" << CppString(AsString(InspectorValue("tooltip"))) << ");\n"
        << "button.Enable(" << ((bool)InspectorValue("enabled") ? "true" : "false") << ");\n";

    String icon_name = AsString(InspectorValue("icon"));
    if(!icon_name.IsEmpty())
        out << "button.SetIcon(UiIconFromName(" << CppString(icon_name) << "));\n";
    else
        out << "button.ClearIcon();\n";

    out << "button.SetIconRenderMode(" << IconModeCode(AsString(InspectorValue("icon_mode"))) << ");\n"
        << "button.SetIconSize(DPI(" << (int)InspectorValue("icon_width")
        << "), DPI(" << (int)InspectorValue("icon_height") << "));\n"
        << "button.SetIconScaleToContent(" << ((bool)InspectorValue("scale_icon") ? "true" : "false") << ");\n"
        << "button.SetIconSide(" << AlignCode(AsString(InspectorValue("icon_side"))) << ");\n"
        << "button.SetAlign(" << AlignCode(AsString(InspectorValue("align_h"))) << ", "
        << AlignCode(AsString(InspectorValue("align_v"))) << ");\n"
        << "button.SetContentGap(DPI(" << (int)InspectorValue("content_gap") << "));\n"
        << "button.SetContentInset(Rect(DPI(" << (int)InspectorValue("inset_left")
        << "), DPI(" << (int)InspectorValue("inset_top")
        << "), DPI(" << (int)InspectorValue("inset_right")
        << "), DPI(" << (int)InspectorValue("inset_bottom") << ")));\n"
        << "button.SetMinSize(Size(DPI(" << (int)InspectorValue("min_width")
        << "), DPI(" << (int)InspectorValue("min_height") << ")));\n"
        << "button.ClickFocus(" << ((bool)InspectorValue("click_focus") ? "true" : "false") << ");\n"
        << "button.SetCheckable(" << ((bool)InspectorValue("checkable") ? "true" : "false") << ");\n";

    if((bool)InspectorValue("checkable"))
        out << "button.SetChecked(" << ((bool)InspectorValue("checked") ? "true" : "false") << ");\n";

    out << "button.SetUnderline(" << ((bool)InspectorValue("underline") ? "true" : "false")
        << ", DPI(" << (int)InspectorValue("underline_width")
        << "), DPI(" << (int)InspectorValue("underline_offset") << "));\n";

    str_generated_code = out;
    edit_generated_code.SetData(str_generated_code);
}

} // namespace Upp
