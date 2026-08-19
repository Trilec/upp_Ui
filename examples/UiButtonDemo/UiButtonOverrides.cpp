#include "UiButtonDemo.h"

namespace Upp {
namespace {

const char *OverrideStateId(int state)
{
    static const char *id[] = { "normal", "hot", "pressed", "disabled" };
    return id[minmax(state, 0, 3)];
}

ValueMap FillRecipe(const UiFill& fill, Color fallback)
{
    ValueMap recipe;
    if(fill.IsSolid()) {
        recipe.Set("mode", "Solid");
        recipe.Set("solid", IsNull(fill.color) ? fallback : fill.color);
    }
    else {
        recipe.Set("mode", "None");
        recipe.Set("solid", fallback);
    }
    return recipe;
}

PropertyEditorItem& MarkOverride(PropertyEditorItem& item)
{
    item.overrideable = true;
    item.override_active = false;
    item.SetDefault(item.value);
    return item;
}

} // namespace

void UiButtonDemo::BuildOverrideModel()
{
    UiButton::Style base = UiTheme::ResolveButton();
    static const char *labels[] = { "Normal", "Hot", "Pressed", "Disabled" };

    MarkOverride(pe_model_override.AddNumericInt("radius", "Radius", base.metrics.radius,
                                                 0, 96, 1, "General"));
    MarkOverride(pe_model_override.AddBoolean("transparent", "Transparent", base.transparent,
                                              "General"));
    MarkOverride(pe_model_override.AddBoolean("high_contrast", "High contrast", base.metrics.high_contrast,
                                              "General"));

    MarkOverride(pe_model_override.AddBoolean("face_enabled", "Enabled", base.metrics.face_enabled,
                                              "Face"));
    for(int i = 0; i < 4; i++)
        MarkOverride(pe_model_override.Add("face." + String(OverrideStateId(i)), labels[i],
                     PropertyEditorKind::FillRecipe,
                     FillRecipe(base.palette.face[i], White()), "Face"));

    MarkOverride(AddPropertyImage(pe_model_override, "skin_image", "Image", String(),
                                  "button-demo-image", "Face/Skin"));
    MarkOverride(pe_model_override.AddBoolean("skin_enabled", "Enabled", base.skin.enabled,
                                              "Face/Skin"));
    MarkOverride(pe_model_override.AddChoice("skin_mode", "Mode",
                    base.skin.image_mode == UiBackgroundImageMode::Fit ? "Fit" : "Fill", "Face/Skin")
                    .AddChoice("Fill", "Fill").AddChoice("Fit", "Fit"));
    MarkOverride(pe_model_override.AddNumericInt("skin_slice_left", "Left", base.skin.slice.left,
                                                  0, 128, 1, "Face/Skin/Slice"));
    MarkOverride(pe_model_override.AddNumericInt("skin_slice_top", "Top", base.skin.slice.top,
                                                  0, 128, 1, "Face/Skin/Slice"));
    MarkOverride(pe_model_override.AddNumericInt("skin_slice_right", "Right", base.skin.slice.right,
                                                  0, 128, 1, "Face/Skin/Slice"));
    MarkOverride(pe_model_override.AddNumericInt("skin_slice_bottom", "Bottom", base.skin.slice.bottom,
                                                  0, 128, 1, "Face/Skin/Slice"));
    MarkOverride(pe_model_override.AddNumericInt("skin_inset_left", "Left", base.skin.content_inset.left,
                                                  0, 128, 1, "Face/Skin/Content Inset"));
    MarkOverride(pe_model_override.AddNumericInt("skin_inset_top", "Top", base.skin.content_inset.top,
                                                  0, 128, 1, "Face/Skin/Content Inset"));
    MarkOverride(pe_model_override.AddNumericInt("skin_inset_right", "Right", base.skin.content_inset.right,
                                                  0, 128, 1, "Face/Skin/Content Inset"));
    MarkOverride(pe_model_override.AddNumericInt("skin_inset_bottom", "Bottom", base.skin.content_inset.bottom,
                                                  0, 128, 1, "Face/Skin/Content Inset"));

    MarkOverride(pe_model_override.AddBoolean("frame_enabled", "Enabled", base.metrics.frame_enabled,
                                              "Frame"));
    MarkOverride(pe_model_override.AddNumericInt("frame_width", "Width", base.metrics.frame_width,
                                                  0, 24, 1, "Frame"));
    MarkOverride(pe_model_override.AddBoolean("dashed", "Dashed", base.metrics.dashed, "Frame"));
    MarkOverride(pe_model_override.AddText("dash_pattern", "Dash pattern", base.metrics.dash_pattern,
                                           "Frame"));
    for(int i = 0; i < 4; i++)
        MarkOverride(pe_model_override.AddColor("frame." + String(OverrideStateId(i)), labels[i],
                     IsNull(base.palette.frame[i]) ? Color(180, 186, 196) : base.palette.frame[i],
                     "Frame"));

    for(int i = 0; i < 4; i++)
        MarkOverride(pe_model_override.AddColor("ink." + String(OverrideStateId(i)), labels[i],
                     IsNull(base.palette.ink[i]) ? SColorText() : base.palette.ink[i], "Ink"));

    for(int i = 0; i < 4; i++)
        MarkOverride(pe_model_override.AddColor("icon." + String(OverrideStateId(i)), labels[i],
                     IsNull(base.palette.icon[i]) ?
                         (IsNull(base.palette.ink[i]) ? SColorText() : base.palette.ink[i]) :
                         base.palette.icon[i],
                     "Icon"));

    String font_face = base.font.GetFaceName();
    MarkOverride(AddPropertyFont(pe_model_override, "font_face", "Face", font_face, "Typography"));
    MarkOverride(pe_model_override.AddNumericInt("font_height", "Height", max(1, base.font.GetHeight()),
                                                  6, 96, 1, "Typography"));
    MarkOverride(pe_model_override.AddBoolean("font_bold", "Bold", base.font.IsBold(), "Typography"));
    MarkOverride(pe_model_override.AddBoolean("font_italic", "Italic", base.font.IsItalic(), "Typography"));

    MarkOverride(pe_model_override.AddBoolean("focus_enabled", "Enabled", base.metrics.focus_enabled,
                                              "Focus"));
    MarkOverride(pe_model_override.AddNumericInt("focus_margin", "Margin", base.metrics.focus_margin,
                                                  0, 20, 1, "Focus"));
    MarkOverride(pe_model_override.AddNumericInt("focus_alpha", "Alpha", base.metrics.focus_alpha,
                                                  0, 255, 1, "Focus"));
    MarkOverride(pe_model_override.AddColor("focus_color", "Colour",
                    IsNull(base.metrics.focus_color) ? Color(37, 99, 235) : base.metrics.focus_color,
                    "Focus"));

    MarkOverride(pe_model_override.AddBoolean("shadow_enabled", "Enabled", base.metrics.shadow.enabled,
                                              "Shadow"));
    MarkOverride(pe_model_override.AddNumericInt("shadow_distance", "Distance", base.metrics.shadow.distance,
                                                  0, 64, 1, "Shadow"));
    MarkOverride(pe_model_override.AddNumericInt("shadow_x", "Offset X", base.metrics.shadow.offset_x,
                                                  -64, 64, 1, "Shadow"));
    MarkOverride(pe_model_override.AddNumericInt("shadow_y", "Offset Y", base.metrics.shadow.offset_y,
                                                  -64, 64, 1, "Shadow"));
    MarkOverride(pe_model_override.AddNumericInt("shadow_alpha", "Alpha", base.metrics.shadow.alpha,
                                                  0, 255, 1, "Shadow"));
    MarkOverride(pe_model_override.AddColor("shadow_color", "Colour", base.metrics.shadow.color, "Shadow"));
    MarkOverride(pe_model_override.AddBoolean("shadow_inset", "Inset", base.metrics.shadow.inset, "Shadow"));
    MarkOverride(pe_model_override.AddChoice("shadow_mode", "Mode",
                    base.metrics.shadow.mode == SHADOW_HARD ? "Hard" : "Curve", "Shadow")
                    .AddChoice("Hard", "Hard").AddChoice("Curve", "Curve"));
    Value curve = PropertyEditorMakeBezierCurve(base.metrics.shadow.curve.x1,
                                                 base.metrics.shadow.curve.y1,
                                                 base.metrics.shadow.curve.x2,
                                                 base.metrics.shadow.curve.y2);
    PropertyEditorItem& shadow_curve =
        pe_model_override.AddBezierCurve("shadow_curve", "Falloff curve", curve, "Shadow");
    shadow_curve.SetRange(0.0, 1.0, 0.001);
    MarkOverride(shadow_curve);

    MarkOverride(pe_model_override.AddBoolean("highlight_enabled", "Enabled",
                                              base.metrics.highlight.enabled, "Highlight"));
    MarkOverride(pe_model_override.AddNumericInt("highlight_thickness", "Thickness",
                                                  base.metrics.highlight.thickness, 0, 24, 1, "Highlight"));
    MarkOverride(pe_model_override.AddNumericInt("highlight_x", "Offset X",
                                                  base.metrics.highlight.offset_x, -32, 32, 1, "Highlight"));
    MarkOverride(pe_model_override.AddNumericInt("highlight_y", "Offset Y",
                                                  base.metrics.highlight.offset_y, -32, 32, 1, "Highlight"));
    MarkOverride(pe_model_override.AddNumericInt("highlight_alpha", "Alpha",
                                                  base.metrics.highlight.alpha, 0, 255, 1, "Highlight"));
    MarkOverride(pe_model_override.AddColor("highlight_color", "Colour",
                                            base.metrics.highlight.color, "Highlight"));
    String highlight_style = base.metrics.highlight.style == DASHED ? "Dashed" :
                             base.metrics.highlight.style == DOTTED ? "Dotted" : "Solid";
    MarkOverride(pe_model_override.AddChoice("highlight_style", "Style", highlight_style, "Highlight")
                    .AddChoice("Solid", "Solid")
                    .AddChoice("Dashed", "Dashed")
                    .AddChoice("Dotted", "Dotted"));

    MarkOverride(pe_model_override.AddNumericInt("press_offset_x", "Offset X", base.press_offset.x,
                                                  -16, 16, 1, "Pressed Content Offset"));
    MarkOverride(pe_model_override.AddNumericInt("press_offset_y", "Offset Y", base.press_offset.y,
                                                  -16, 16, 1, "Pressed Content Offset"));

    pe_model_override.SetGroupSubtitle("General", "whole-control appearance");
    pe_model_override.SetGroupSubtitle("Face", "state fill plus optional image skin");
    pe_model_override.SetGroupSubtitle("Frame", "border geometry and state colours");
    pe_model_override.SetGroupSubtitle("Ink", "text colours by interaction state");
    pe_model_override.SetGroupSubtitle("Icon", "icon tint by interaction state");
    pe_model_override.SetGroupSubtitle("Pressed Content Offset", "paint-only icon/text displacement while pressed");
    pe_model_override.StructureChanged();
    UpdateOverrideSummaries();
}

void UiButtonDemo::UpdateOverrideSummaries()
{
    VectorMap<String, int> active, total;
    for(int i = 0; i < pe_model_override.GetCount(); i++) {
        const PropertyEditorItem& item = pe_model_override[i];
        total.GetAdd(item.group, 0)++;
        if(item.override_active)
            active.GetAdd(item.group, 0)++;
    }
    for(int i = 0; i < total.GetCount(); i++)
        pe_model_override.SetGroupSubtitle(total.GetKey(i),
            Format("%d of %d local", active.Get(total.GetKey(i), 0), total[i]));
}

} // namespace Upp
