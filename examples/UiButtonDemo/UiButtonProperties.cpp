#include "UiButtonDemo.h"

namespace Upp {
namespace {

const char *ProjectionStateId(int state)
{
    static const char *id[] = { "normal", "hot", "pressed", "disabled" };
    return id[minmax(state, 0, 3)];
}

Value MapValue(const ValueMap& map, const String& key, const Value& fallback)
{
    int q = map.Find(key);
    return q >= 0 ? map.GetValue(q) : fallback;
}

void ApplyFillRecipe(UiFill& target, const Value& value)
{
    if(!value.Is<ValueMap>())
        return;
    ValueMap recipe = value;
    String mode = AsString(MapValue(recipe, "mode", "None"));
    if(mode == "Solid") {
        target = UiFill::Solid(Color(MapValue(recipe, "solid", White())));
        return;
    }
    if(mode == "QuadGradient") {
        Color tl(MapValue(recipe, "top_left", White()));
        Color tr(MapValue(recipe, "top_right", tl));
        Color bl(MapValue(recipe, "bottom_left", tl));
        Color br(MapValue(recipe, "bottom_right", tr));
        int tile = max(8, (int)MapValue(recipe, "tile_size", 32));
        int blur = max(0, (int)MapValue(recipe, "blur", 0));
        target = UiFill::ImageFill(MakeQuadGradientTile(tile, tl, tr, bl, br, blur));
        return;
    }
    target = UiFill::None();
}

UiAlign ParseAlign(const String& value)
{
    String v = ToLower(value);
    if(v == "center") return UiAlign::CENTER;
    if(v == "right") return UiAlign::RIGHT;
    if(v == "top") return UiAlign::TOP;
    if(v == "bottom") return UiAlign::BOTTOM;
    return UiAlign::LEFT;
}

UiIconRenderMode ParseIconMode(const String& value)
{
    if(value == "Auto") return UiIconRenderMode::Auto;
    if(value == "PreserveColor") return UiIconRenderMode::PreserveColor;
    return UiIconRenderMode::MonoTint;
}

} // namespace

void UiButtonDemo::BuildInspectorModel()
{
    pe_model_inspector.AddText("text", "Text", "Click Me", "Content")
                      .SetDefault("Click Me");
    pe_model_inspector.AddText("tooltip", "Tooltip", "Interactive UiButton preview", "Content")
                      .SetDefault("Interactive UiButton preview");
    AddPropertyIcon(pe_model_inspector, "icon", "Icon", "ICON_DESIGN_WIDGETS_48", "Content");
    pe_model_inspector.AddChoice("icon_mode", "Icon rendering", "MonoTint", "Content")
                      .AddChoice("Auto", "Auto")
                      .AddChoice("PreserveColor", "Preserve colour")
                      .AddChoice("MonoTint", "Monochrome tint");

    pe_model_inspector.AddNumericInt("preview_width", "Preview width", 220, 80, 760, 1, "Layout")
                      .SetUnit("px");
    pe_model_inspector.AddNumericInt("preview_height", "Preview height", 52, 28, 240, 1, "Layout")
                      .SetUnit("px");
    pe_model_inspector.AddNumericInt("min_width", "Minimum width", 120, 0, 760, 1, "Layout")
                      .SetUnit("px");
    pe_model_inspector.AddNumericInt("min_height", "Minimum height", 38, 0, 240, 1, "Layout")
                      .SetUnit("px");
    pe_model_inspector.AddChoice("align_h", "Horizontal alignment", "Center", "Layout")
                      .AddChoice("Left", "Left").AddChoice("Center", "Center").AddChoice("Right", "Right");
    pe_model_inspector.AddChoice("align_v", "Vertical alignment", "Center", "Layout")
                      .AddChoice("Top", "Top").AddChoice("Center", "Center").AddChoice("Bottom", "Bottom");
    AddPropertyMatrix(pe_model_inspector, "icon_side", "Icon side", "left", "Cardinal4", "Layout");
    pe_model_inspector.AddNumericInt("icon_width", "Icon width", 20, 0, 128, 1, "Layout")
                      .SetUnit("px");
    pe_model_inspector.AddNumericInt("icon_height", "Icon height", 20, 0, 128, 1, "Layout")
                      .SetUnit("px");
    pe_model_inspector.AddNumericInt("content_gap", "Content gap", 8, 0, 80, 1, "Layout")
                      .SetUnit("px");
    pe_model_inspector.AddNumericInt("inset_left", "Inset left", 14, 0, 80, 1, "Layout/Content Inset")
                      .SetUnit("px");
    pe_model_inspector.AddNumericInt("inset_top", "Inset top", 8, 0, 80, 1, "Layout/Content Inset")
                      .SetUnit("px");
    pe_model_inspector.AddNumericInt("inset_right", "Inset right", 14, 0, 80, 1, "Layout/Content Inset")
                      .SetUnit("px");
    pe_model_inspector.AddNumericInt("inset_bottom", "Inset bottom", 8, 0, 80, 1, "Layout/Content Inset")
                      .SetUnit("px");
    pe_model_inspector.AddBoolean("scale_icon", "Scale icon to content", false, "Layout");

    pe_model_inspector.AddBoolean("enabled", "Enabled", true, "Behaviour");
    pe_model_inspector.AddBoolean("click_focus", "Click takes focus", true, "Behaviour");
    pe_model_inspector.AddBoolean("checkable", "Checkable", false, "Behaviour");
    pe_model_inspector.AddBoolean("checked", "Checked", false, "Behaviour");
    pe_model_inspector.AddBoolean("underline", "Underline", false, "Behaviour/Underline");
    pe_model_inspector.AddNumericInt("underline_width", "Thickness", 1, 0, 12, 1, "Behaviour/Underline")
                      .SetUnit("px");
    pe_model_inspector.AddNumericInt("underline_offset", "Offset", 2, -12, 24, 1, "Behaviour/Underline")
                      .SetUnit("px");

    pe_model_inspector.SetGroupSubtitle("Content", "text and icon media");
    pe_model_inspector.SetGroupSubtitle("Layout", "real UiButton content geometry");
    pe_model_inspector.SetGroupSubtitle("Behaviour", "focus, checked state and underline");
    pe_model_inspector.StructureChanged();
}

void UiButtonDemo::ApplyProjection()
{
    UiButton::Style style = UiTheme::ResolveButton();

    for(int i = 0; i < 4; i++) {
        String state = ProjectionStateId(i);
        if(OverrideActive("face." + state))
            ApplyFillRecipe(style.palette.face[i], OverrideValue("face." + state));
        if(OverrideActive("frame." + state))
            style.palette.frame[i] = Color(OverrideValue("frame." + state));
        if(OverrideActive("ink." + state))
            style.palette.ink[i] = Color(OverrideValue("ink." + state));
        if(OverrideActive("icon." + state))
            style.palette.icon[i] = Color(OverrideValue("icon." + state));
    }

#define APPLY_BOOL(ID, FIELD) if(OverrideActive(ID)) FIELD = (bool)OverrideValue(ID)
#define APPLY_INT(ID, FIELD)  if(OverrideActive(ID)) FIELD = (int)OverrideValue(ID)

    APPLY_INT("radius", style.metrics.radius);
    APPLY_BOOL("transparent", style.transparent);
    APPLY_BOOL("high_contrast", style.metrics.high_contrast);
    APPLY_BOOL("face_enabled", style.metrics.face_enabled);

    APPLY_BOOL("frame_enabled", style.metrics.frame_enabled);
    APPLY_INT("frame_width", style.metrics.frame_width);
    APPLY_BOOL("dashed", style.metrics.dashed);
    if(OverrideActive("dash_pattern"))
        style.metrics.dash_pattern = AsString(OverrideValue("dash_pattern"));

    if(OverrideActive("font_face")) {
        String face = AsString(OverrideValue("font_face"));
        if(!face.IsEmpty())
            style.font.FaceName(face);
    }
    if(OverrideActive("font_height"))
        style.font.Height((int)OverrideValue("font_height"));
    if(OverrideActive("font_bold"))
        style.font.Bold((bool)OverrideValue("font_bold"));
    if(OverrideActive("font_italic"))
        style.font.Italic((bool)OverrideValue("font_italic"));

    APPLY_BOOL("focus_enabled", style.metrics.focus_enabled);
    APPLY_INT("focus_margin", style.metrics.focus_margin);
    APPLY_INT("focus_alpha", style.metrics.focus_alpha);
    if(OverrideActive("focus_color"))
        style.metrics.focus_color = Color(OverrideValue("focus_color"));

    APPLY_BOOL("shadow_enabled", style.metrics.shadow.enabled);
    APPLY_INT("shadow_distance", style.metrics.shadow.distance);
    APPLY_INT("shadow_x", style.metrics.shadow.offset_x);
    APPLY_INT("shadow_y", style.metrics.shadow.offset_y);
    APPLY_INT("shadow_alpha", style.metrics.shadow.alpha);
    if(OverrideActive("shadow_color"))
        style.metrics.shadow.color = Color(OverrideValue("shadow_color"));
    APPLY_BOOL("shadow_inset", style.metrics.shadow.inset);
    if(OverrideActive("shadow_mode"))
        style.metrics.shadow.mode =
            AsString(OverrideValue("shadow_mode")) == "Hard" ? SHADOW_HARD : SHADOW_CURVE;
    if(OverrideActive("shadow_curve")) {
        ValueArray v = OverrideValue("shadow_curve");
        if(v.GetCount() >= 4)
            style.metrics.shadow.curve =
                ShadowCurve { (double)v[0], (double)v[1], (double)v[2], (double)v[3] };
    }

    APPLY_BOOL("highlight_enabled", style.metrics.highlight.enabled);
    APPLY_INT("highlight_thickness", style.metrics.highlight.thickness);
    APPLY_INT("highlight_x", style.metrics.highlight.offset_x);
    APPLY_INT("highlight_y", style.metrics.highlight.offset_y);
    APPLY_INT("highlight_alpha", style.metrics.highlight.alpha);
    if(OverrideActive("highlight_color"))
        style.metrics.highlight.color = Color(OverrideValue("highlight_color"));
    if(OverrideActive("highlight_style")) {
        String v = AsString(OverrideValue("highlight_style"));
        style.metrics.highlight.style = v == "Dashed" ? DASHED : v == "Dotted" ? DOTTED : SOLID;
    }

    if(OverrideActive("skin_image"))
        style.skin.base = LoadImageValue(OverrideValue("skin_image"));
    APPLY_BOOL("skin_enabled", style.skin.enabled);
    if(OverrideActive("skin_mode"))
        style.skin.image_mode =
            AsString(OverrideValue("skin_mode")) == "Fit" ? UiBackgroundImageMode::Fit
                                                          : UiBackgroundImageMode::Fill;
    APPLY_INT("skin_slice_left", style.skin.slice.left);
    APPLY_INT("skin_slice_top", style.skin.slice.top);
    APPLY_INT("skin_slice_right", style.skin.slice.right);
    APPLY_INT("skin_slice_bottom", style.skin.slice.bottom);
    APPLY_INT("skin_inset_left", style.skin.content_inset.left);
    APPLY_INT("skin_inset_top", style.skin.content_inset.top);
    APPLY_INT("skin_inset_right", style.skin.content_inset.right);
    APPLY_INT("skin_inset_bottom", style.skin.content_inset.bottom);

    APPLY_INT("press_offset_x", style.press_offset.x);
    APPLY_INT("press_offset_y", style.press_offset.y);

#undef APPLY_BOOL
#undef APPLY_INT

    btn_preview.SetCustomStyle(style);
    btn_preview.SetText(AsString(InspectorValue("text")));
    btn_preview.Tip(AsString(InspectorValue("tooltip")));
    btn_preview.Enable((bool)InspectorValue("enabled"));

    String icon_name = AsString(InspectorValue("icon"));
    if(icon_name.IsEmpty())
        btn_preview.ClearIcon();
    else
        btn_preview.SetIcon(UiIconFromName(icon_name));
    btn_preview.SetIconRenderMode(ParseIconMode(AsString(InspectorValue("icon_mode"))));
    btn_preview.SetIconSize((int)InspectorValue("icon_width"), (int)InspectorValue("icon_height"));
    btn_preview.SetIconScaleToContent((bool)InspectorValue("scale_icon"));
    btn_preview.SetIconSide(ParseAlign(AsString(InspectorValue("icon_side"))));
    btn_preview.SetAlign(ParseAlign(AsString(InspectorValue("align_h"))),
                         ParseAlign(AsString(InspectorValue("align_v"))));
    btn_preview.SetContentGap((int)InspectorValue("content_gap"));
    btn_preview.SetContentInset(Rect((int)InspectorValue("inset_left"),
                                     (int)InspectorValue("inset_top"),
                                     (int)InspectorValue("inset_right"),
                                     (int)InspectorValue("inset_bottom")));
    btn_preview.ClickFocus((bool)InspectorValue("click_focus"));
    btn_preview.SetCheckable((bool)InspectorValue("checkable"));
    btn_preview.SetChecked((bool)InspectorValue("checked"));
    btn_preview.SetUnderline((bool)InspectorValue("underline"),
                             (int)InspectorValue("underline_width"),
                             (int)InspectorValue("underline_offset"));
    btn_preview.SetMinSize(Size((int)InspectorValue("min_width"),
                                (int)InspectorValue("min_height")));

    if(!(bool)InspectorValue("checkable") && (bool)InspectorValue("checked")) {
        pe_model_inspector.SetValue("checked", false);
        pe_inspector.RefreshModel();
    }

    UpdateOverrideSummaries();
    UpdateGeneratedCode();
    UpdateStatus();
    RefreshLayout();
}

} // namespace Upp
