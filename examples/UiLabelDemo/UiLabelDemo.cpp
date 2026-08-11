#include "UiLabelDemo.h"

namespace Upp {
namespace {

enum LabelStateIndex {
    LABEL_NORMAL,
    LABEL_HOT,
    LABEL_PRESSED,
    LABEL_DISABLED,
};

const char *StateId(int state)
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

ValueMap SolidRecipe(Color color)
{
    ValueMap recipe;
    recipe.Set("mode", "Solid");
    recipe.Set("solid", color);
    return recipe;
}

Value MapValue(const ValueMap& map, const String& key, const Value& fallback)
{
    const int q = map.Find(key);
    return q >= 0 ? map.GetValue(q) : fallback;
}

Color FillFallback(const UiFill& fill, Color fallback)
{
    return fill.IsSolid() && !IsNull(fill.color) ? fill.color : fallback;
}

void ApplyFillRecipe(UiFill& target, const Value& value)
{
    if(!value.Is<ValueMap>())
        return;
    ValueMap recipe = value;
    const String mode = AsString(MapValue(recipe, "mode", "None"));
    if(mode == "Solid") {
        target = UiFill::Solid(Color(MapValue(recipe, "solid", White())));
        return;
    }
    if(mode == "QuadGradient") {
        const Color tl(MapValue(recipe, "top_left", White()));
        const Color tr(MapValue(recipe, "top_right", tl));
        const Color bl(MapValue(recipe, "bottom_left", tl));
        const Color br(MapValue(recipe, "bottom_right", tr));
        const int tile = max(8, (int)MapValue(recipe, "tile_size", 32));
        const int blur = max(0, (int)MapValue(recipe, "blur", 0));
        target = UiFill::ImageFill(MakeQuadGradientTile(tile, tl, tr, bl, br, blur));
        return;
    }
    target = UiFill::None();
}

String FillRecipeCode(const Value& value)
{
    if(!value.Is<ValueMap>())
        return "UiFill::None()";
    ValueMap recipe = value;
    const String mode = AsString(MapValue(recipe, "mode", "None"));
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

UiRole ParseRole(const String& value)
{
    if(value == "Subtle") return UiRole::Subtle;
    if(value == "Accent") return UiRole::Accent;
    if(value == "Alert") return UiRole::Alert;
    return UiRole::Standard;
}

UiTextSize ParseTextSize(const String& value)
{
    if(value == "H1") return UiTextSize::H1;
    if(value == "H2") return UiTextSize::H2;
    if(value == "H3") return UiTextSize::H3;
    return UiTextSize::Body;
}

UiAlign ParseAlign(const String& value)
{
    const String normalized = ToLower(value);
    if(normalized == "center") return UiAlign::CENTER;
    if(normalized == "right") return UiAlign::RIGHT;
    if(normalized == "top") return UiAlign::TOP;
    if(normalized == "bottom") return UiAlign::BOTTOM;
    return UiAlign::LEFT;
}

String RoleCode(const String& value) { return "UiRole::" + value; }
String TextSizeCode(const String& value) { return "UiTextSize::" + value; }
String AlignCode(const String& value)
{
    return "UiAlign::" + ToUpper(value);
}

PropertyEditorItem& MarkOverride(PropertyEditorItem& item)
{
    item.overrideable = true;
    item.override_active = false;
    item.SetDefault(item.value);
    return item;
}

} // namespace

UiLabelDemo::UiLabelDemo()
{
    Title("UiLabel Demo");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1220), DPI(780));

    UiThemeContext context = UiTheme::GetContext();
    context.preset = UiThemePreset::Minimal;
    context.mode = UiThemeMode::Light;
    UiTheme::Set(context);

    RegisterPropertyEditorV1Editors(pe_factory);
    pe_factory.RegisterPicker("label-demo-image",
        [=](Value& value, Ctrl *owner) { return PickImage(value, owner); });
    pe_factory.RegisterThumbnailProvider("label-demo-image",
        [=](const Value& value) { return LoadImageValue(value); });

    BuildHeader();
    BuildPreview();
    BuildRightRail();
    BuildInspectorModel();
    BuildOverrideModel();
    ConfigureEditors();
    ConnectEvents();
    ApplyTheme();
    SelectPage(0);
    ApplyProjection();
}

void UiLabelDemo::BuildHeader()
{
    Add(tc_header);
    tc_header.SetTitle("UiLabel")
           .SetSubTitle("Live PropertyEditor, complete style overrides and copyable C++")
           .SetMedia(ICON_DESIGN_WIDGETS_48())
           .SetMediaSide(UiAlign::LEFT)
           .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
           .SetMediaAutoFit(true)
           .ShowTitleLine(false)
           .SetContentInset(DPI(8))
           .SetContentCell(box_header_actions);
    box_header_actions.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    box_header_actions.AddSpacer(1).Expand(1);
    btn_theme.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark theme");
    btn_help.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("About this reference demo");
    btn_exit.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close demo");
    box_header_actions.Add(btn_theme).Fixed(DPI(34));
    box_header_actions.Add(btn_help).Fixed(DPI(34));
    box_header_actions.Add(btn_exit).Fixed(DPI(34));
}

void UiLabelDemo::BuildPreview()
{
    Add(pnl_preview);
    pnl_preview.Add(lbl_preview);
    pnl_preview.Add(lbl_preview_caption);
    lbl_preview_caption.SetText("Centered live UiLabel preview")
                    .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
}

void UiLabelDemo::BuildRightRail()
{
    Add(pnl_right_rail);
    pnl_right_rail.Add(box_right_tools);
    pnl_right_rail.Add(stk_right_pages);
    box_right_tools.SetGap(DPI(4)).SetInset(Rect(DPI(2), 0, DPI(2), 0))
                    .SetAlignItems(UiCrossAlign::Center);
    btn_inspector_mode.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Inspector");
    btn_overrides_mode.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Theme Overrides");
    btn_code_mode.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Generated C++");
    box_right_tools.Add(btn_inspector_mode).Fixed(DPI(38));
    box_right_tools.Add(btn_overrides_mode).Fixed(DPI(38));
    box_right_tools.Add(btn_code_mode).Fixed(DPI(38));
    box_right_tools.AddSpacer(1).Expand(1);

    stk_right_pages.Add(pnl_inspector_page, "inspector");
    stk_right_pages.Add(pnl_overrides_page, "overrides");
    stk_right_pages.Add(pnl_code_page, "code");
    pnl_inspector_page.Add(pe_inspector.SizePos());
    pnl_overrides_page.Add(pe_overrides.SizePos());
    pnl_code_page.Add(edit_generated_code);
    edit_generated_code.HSizePos(DPI(6), DPI(6)).VSizePos(DPI(42), DPI(6));
    pnl_code_page.Add(btn_copy_code.RightPos(DPI(8), DPI(32)).TopPos(DPI(6), DPI(30)));
    btn_copy_code.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16), DPI(16)).Tip("Copy generated C++");
    edit_generated_code.SetReadOnly();
}

void UiLabelDemo::BuildInspectorModel()
{
    pe_model_inspector.AddText("text", "Text", "UiLabel preview", "Content")
                    .SetDefault("UiLabel preview");
    pe_model_inspector.AddText("tooltip", "Tooltip", "Interactive UiLabel preview", "Content")
                    .SetDefault("Interactive UiLabel preview");
    pe_model_inspector.AddBoolean("rich_demo", "Rich span example", false, "Content")
                    .SetHelp("Uses UiLabel's real rich span API instead of replacing the control.");
    AddPropertyIcon(pe_model_inspector, "icon", "Icon", "ICON_DESIGN_WIDGETS_48", "Content");
    pe_model_inspector.AddChoice("icon_mode", "Icon rendering", "MonoTint", "Content")
                    .AddChoice("Auto", "Auto").AddChoice("PreserveColor", "Preserve colour")
                    .AddChoice("MonoTint", "Monochrome tint");

    pe_model_inspector.AddChoice("role", "Role", "Standard", "Theme")
                    .AddChoice("Standard", "Standard").AddChoice("Subtle", "Subtle")
                    .AddChoice("Accent", "Accent").AddChoice("Alert", "Alert");
    pe_model_inspector.AddChoice("text_size", "Text size", "Body", "Theme")
                    .AddChoice("Body", "Body").AddChoice("H1", "H1")
                    .AddChoice("H2", "H2").AddChoice("H3", "H3");

    pe_model_inspector.AddNumericInt("width", "Preview width", 360, 80, 760, 1, "Layout").SetUnit("px");
    pe_model_inspector.AddNumericInt("height", "Preview height", 150, 30, 420, 1, "Layout").SetUnit("px");
    pe_model_inspector.AddChoice("align_h", "Horizontal alignment", "Center", "Layout")
                    .AddChoice("Left", "Left").AddChoice("Center", "Center").AddChoice("Right", "Right");
    pe_model_inspector.AddChoice("align_v", "Vertical alignment", "Center", "Layout")
                    .AddChoice("Top", "Top").AddChoice("Center", "Center").AddChoice("Bottom", "Bottom");
    AddPropertyMatrix(pe_model_inspector, "icon_side", "Icon side", "top",
                      "Cardinal4", "Layout");
    pe_model_inspector.AddNumericInt("icon_width", "Icon width", 28, 0, 128, 1, "Layout").SetUnit("px");
    pe_model_inspector.AddNumericInt("icon_height", "Icon height", 28, 0, 128, 1, "Layout").SetUnit("px");
    pe_model_inspector.AddNumericInt("content_gap", "Content gap", 10, 0, 80, 1, "Layout").SetUnit("px");
    pe_model_inspector.AddBoolean("scale_icon", "Scale icon to content", false, "Layout");
    pe_model_inspector.AddBoolean("enabled", "Enabled", true, "Behaviour");
    pe_model_inspector.AddBoolean("selectable", "Selectable text", true, "Behaviour");

    pe_model_inspector.SetGroupSubtitle("Content", "text, rich spans and icon media");
    pe_model_inspector.SetGroupSubtitle("Theme", "base role before local overrides");
    pe_model_inspector.SetGroupSubtitle("Layout", "real UiLabel content geometry");
    pe_model_inspector.StructureChanged();
}

void UiLabelDemo::BuildOverrideModel()
{
    UiLabel::Style base = UiTheme::ResolveLabel(UiRole::Standard, UiTextSize::Body);
    static const char *labels[] = { "Normal", "Hot", "Pressed", "Disabled" };
    for(int i = 0; i < 4; i++) {
        MarkOverride(pe_model_override.Add("face." + String(StateId(i)), labels[i],
                     PropertyEditorKind::FillRecipe,
                     SolidRecipe(FillFallback(base.palette.face[i], White())), "Face"));
        MarkOverride(pe_model_override.AddColor("frame." + String(StateId(i)), labels[i],
                     IsNull(base.palette.frame[i]) ? Color(180, 186, 196) : base.palette.frame[i], "Frame"));
        MarkOverride(pe_model_override.AddColor("ink." + String(StateId(i)), labels[i],
                     IsNull(base.palette.ink[i]) ? SColorText() : base.palette.ink[i], "Text Ink"));
        MarkOverride(pe_model_override.AddColor("icon." + String(StateId(i)), labels[i],
                     IsNull(base.palette.icon[i]) ? SColorText() : base.palette.icon[i], "Icon Ink"));
    }

    MarkOverride(pe_model_override.AddBoolean("face_enabled", "Face enabled", base.metrics.face_enabled, "Frame and Face"));
    MarkOverride(pe_model_override.AddBoolean("frame_enabled", "Frame enabled", base.metrics.frame_enabled, "Frame and Face"));
    MarkOverride(pe_model_override.AddNumericInt("frame_width", "Frame width", base.metrics.frame_width, 0, 24, 1, "Frame and Face"));
    MarkOverride(pe_model_override.AddNumericInt("radius", "Radius", base.metrics.radius, 0, 96, 1, "Frame and Face"));
    MarkOverride(pe_model_override.AddBoolean("dashed", "Dashed frame", base.metrics.dashed, "Frame and Face"));
    MarkOverride(pe_model_override.AddText("dash_pattern", "Dash pattern", base.metrics.dash_pattern, "Frame and Face"));
    MarkOverride(pe_model_override.AddBoolean("transparent", "Transparent", base.transparent, "Frame and Face"));

    String face = base.font.GetFaceName();
    MarkOverride(AddPropertyFont(pe_model_override, "font_face", "Font face", face, "Typography"));
    MarkOverride(pe_model_override.AddNumericInt("font_height", "Font height", max(1, base.font.GetHeight()), 6, 96, 1, "Typography"));
    MarkOverride(pe_model_override.AddBoolean("font_bold", "Bold", base.font.IsBold(), "Typography"));
    MarkOverride(pe_model_override.AddBoolean("font_italic", "Italic", base.font.IsItalic(), "Typography"));
    MarkOverride(pe_model_override.AddBoolean("underline", "Underline", base.underline, "Typography"));
    MarkOverride(pe_model_override.AddNumericInt("underline_width", "Underline width", base.underline_width, 1, 12, 1, "Typography"));
    MarkOverride(pe_model_override.AddNumericInt("underline_offset", "Underline offset", base.underline_offset, -20, 40, 1, "Typography"));
    MarkOverride(pe_model_override.AddBoolean("nowrap", "No wrap", base.nowrap, "Typography"));

    MarkOverride(pe_model_override.AddNumericInt("margin_left", "Left", base.metrics.content_margin.left, 0, 80, 1, "Content Margin"));
    MarkOverride(pe_model_override.AddNumericInt("margin_top", "Top", base.metrics.content_margin.top, 0, 80, 1, "Content Margin"));
    MarkOverride(pe_model_override.AddNumericInt("margin_right", "Right", base.metrics.content_margin.right, 0, 80, 1, "Content Margin"));
    MarkOverride(pe_model_override.AddNumericInt("margin_bottom", "Bottom", base.metrics.content_margin.bottom, 0, 80, 1, "Content Margin"));

    MarkOverride(pe_model_override.AddBoolean("focus_enabled", "Focus frame", base.metrics.focus_enabled, "Focus"));
    MarkOverride(pe_model_override.AddNumericInt("focus_margin", "Focus margin", base.metrics.focus_margin, 0, 20, 1, "Focus"));
    MarkOverride(pe_model_override.AddNumericInt("focus_alpha", "Focus alpha", base.metrics.focus_alpha, 0, 255, 1, "Focus"));
    MarkOverride(pe_model_override.AddColor("focus_color", "Focus colour", IsNull(base.metrics.focus_color) ? Color(37, 99, 235) : base.metrics.focus_color, "Focus"));
    MarkOverride(pe_model_override.AddBoolean("high_contrast", "High contrast", base.metrics.high_contrast, "Focus"));

    MarkOverride(pe_model_override.AddBoolean("shadow_enabled", "Enabled", base.metrics.shadow.enabled, "Shadow"));
    MarkOverride(pe_model_override.AddNumericInt("shadow_distance", "Distance", base.metrics.shadow.distance, 0, 64, 1, "Shadow"));
    MarkOverride(pe_model_override.AddNumericInt("shadow_x", "Offset X", base.metrics.shadow.offset_x, -64, 64, 1, "Shadow"));
    MarkOverride(pe_model_override.AddNumericInt("shadow_y", "Offset Y", base.metrics.shadow.offset_y, -64, 64, 1, "Shadow"));
    MarkOverride(pe_model_override.AddNumericInt("shadow_alpha", "Alpha", base.metrics.shadow.alpha, 0, 255, 1, "Shadow"));
    MarkOverride(pe_model_override.AddColor("shadow_color", "Colour", base.metrics.shadow.color, "Shadow"));
    MarkOverride(pe_model_override.AddBoolean("shadow_inset", "Inset", base.metrics.shadow.inset, "Shadow"));
    MarkOverride(pe_model_override.AddChoice("shadow_mode", "Mode", base.metrics.shadow.mode == SHADOW_HARD ? "Hard" : "Curve", "Shadow")
                 .AddChoice("Hard", "Hard").AddChoice("Curve", "Curve"));
    Value curve = PropertyEditorMakeBezierCurve(base.metrics.shadow.curve.x1, base.metrics.shadow.curve.y1,
                                                 base.metrics.shadow.curve.x2, base.metrics.shadow.curve.y2);
    PropertyEditorItem& shadow_curve = pe_model_override.AddBezierCurve("shadow_curve", "Falloff curve", curve, "Shadow");
    shadow_curve.SetRange(0.0, 1.0, 0.001);
    MarkOverride(shadow_curve);

    MarkOverride(pe_model_override.AddBoolean("highlight_enabled", "Enabled", base.metrics.highlight.enabled, "Highlight"));
    MarkOverride(pe_model_override.AddNumericInt("highlight_thickness", "Thickness", base.metrics.highlight.thickness, 0, 24, 1, "Highlight"));
    MarkOverride(pe_model_override.AddNumericInt("highlight_x", "Offset X", base.metrics.highlight.offset_x, -32, 32, 1, "Highlight"));
    MarkOverride(pe_model_override.AddNumericInt("highlight_y", "Offset Y", base.metrics.highlight.offset_y, -32, 32, 1, "Highlight"));
    MarkOverride(pe_model_override.AddNumericInt("highlight_alpha", "Alpha", base.metrics.highlight.alpha, 0, 255, 1, "Highlight"));
    MarkOverride(pe_model_override.AddColor("highlight_color", "Colour", base.metrics.highlight.color, "Highlight"));
    MarkOverride(pe_model_override.AddChoice("highlight_style", "Style", "Solid", "Highlight")
                 .AddChoice("Solid", "Solid").AddChoice("Dashed", "Dashed").AddChoice("Dotted", "Dotted"));

    MarkOverride(AddPropertyImage(pe_model_override, "skin_image", "Image", String(), "label-demo-image", "Image Skin"));
    MarkOverride(pe_model_override.AddBoolean("skin_enabled", "Enabled", false, "Image Skin"));
    MarkOverride(pe_model_override.AddChoice("skin_mode", "Mode", "Fill", "Image Skin")
                 .AddChoice("Fill", "Fill").AddChoice("Fit", "Fit"));
    const char *skin_names[] = { "skin_slice_left", "skin_slice_top", "skin_slice_right", "skin_slice_bottom",
                                 "skin_inset_left", "skin_inset_top", "skin_inset_right", "skin_inset_bottom" };
    const char *skin_labels[] = { "Slice left", "Slice top", "Slice right", "Slice bottom",
                                  "Inset left", "Inset top", "Inset right", "Inset bottom" };
    for(int i = 0; i < 8; i++)
        MarkOverride(pe_model_override.AddNumericInt(skin_names[i], skin_labels[i], 0, 0, 128, 1, "Image Skin"));

    pe_model_override.StructureChanged();
    UpdateOverrideSummaries();
}

void UiLabelDemo::ConfigureEditors()
{
    pe_inspector.SetFactory(&pe_factory);
    pe_overrides.SetFactory(&pe_factory);
    pe_inspector.SetModel(&pe_model_inspector);
    pe_overrides.SetModel(&pe_model_override);
    pe_inspector.SetLabelRatio(38);
    pe_overrides.SetLabelRatio(38);
    PropertyEditorStyle style = PropertyEditorStyle::System();
    style.show_group_summaries = true;
    pe_inspector.SetStyle(style);
    pe_overrides.SetStyle(style);
}

void UiLabelDemo::ConnectEvents()
{
    btn_inspector_mode.WhenAction = [=] { SelectPage(0); };
    btn_overrides_mode.WhenAction = [=] { SelectPage(1); };
    btn_code_mode.WhenAction = [=] { SelectPage(2); };
    btn_theme.WhenAction = [=] { ToggleTheme(); };
    btn_exit.WhenAction = [=] { Close(); };
    btn_help.WhenAction = [=] {
        PromptOK("UiLabel reference builder\n\nInspector authors the control API. Theme Overrides activate individual local style fields. Code is regenerated from the same state and is directly copyable.");
    };
    btn_copy_code.WhenAction = [=] { WriteClipboardText(str_generated_code); };

    auto changed = [=](String, Value) { ApplyProjection(); };
    pe_inspector.WhenPreview = changed;
    pe_inspector.WhenCommit = changed;
    pe_overrides.WhenPreview = changed;
    pe_overrides.WhenCommit = changed;
    pe_inspector.WhenReset = [=](String id) { ResetProperty(pe_model_inspector, id); };
    pe_overrides.WhenReset = [=](String id) { ResetProperty(pe_model_override, id); };
    pe_overrides.WhenOverride = [=](String id, bool active) { SetOverrideActive(id, active); };
}

Value UiLabelDemo::InspectorValue(const String& id) const
{
    const PropertyEditorItem *item = pe_model_inspector.Find(id);
    return item ? item->value : Value();
}

Value UiLabelDemo::OverrideValue(const String& id) const
{
    const PropertyEditorItem *item = pe_model_override.Find(id);
    return item ? item->value : Value();
}

bool UiLabelDemo::OverrideActive(const String& id) const
{
    const PropertyEditorItem *item = pe_model_override.Find(id);
    return item && item->override_active;
}

void UiLabelDemo::ApplyProjection()
{
    UiLabel::Style style = UiTheme::ResolveLabel(ParseRole(AsString(InspectorValue("role"))),
                                                  ParseTextSize(AsString(InspectorValue("text_size"))));
    for(int i = 0; i < 4; i++) {
        String state = StateId(i);
        if(OverrideActive("face." + state)) ApplyFillRecipe(style.palette.face[i], OverrideValue("face." + state));
        if(OverrideActive("frame." + state)) style.palette.frame[i] = Color(OverrideValue("frame." + state));
        if(OverrideActive("ink." + state)) style.palette.ink[i] = Color(OverrideValue("ink." + state));
        if(OverrideActive("icon." + state)) style.palette.icon[i] = Color(OverrideValue("icon." + state));
    }
#define APPLY_BOOL(id, field) if(OverrideActive(id)) field = (bool)OverrideValue(id)
#define APPLY_INT(id, field) if(OverrideActive(id)) field = (int)OverrideValue(id)
    APPLY_BOOL("face_enabled", style.metrics.face_enabled);
    APPLY_BOOL("frame_enabled", style.metrics.frame_enabled);
    APPLY_INT("frame_width", style.metrics.frame_width);
    APPLY_INT("radius", style.metrics.radius);
    APPLY_BOOL("dashed", style.metrics.dashed);
    if(OverrideActive("dash_pattern")) style.metrics.dash_pattern = AsString(OverrideValue("dash_pattern"));
    APPLY_BOOL("transparent", style.transparent);
    if(OverrideActive("font_face")) style.font.FaceName(AsString(OverrideValue("font_face")));
    if(OverrideActive("font_height")) style.font.Height((int)OverrideValue("font_height"));
    if(OverrideActive("font_bold")) style.font.Bold((bool)OverrideValue("font_bold"));
    if(OverrideActive("font_italic")) style.font.Italic((bool)OverrideValue("font_italic"));
    APPLY_BOOL("underline", style.underline);
    APPLY_INT("underline_width", style.underline_width);
    APPLY_INT("underline_offset", style.underline_offset);
    APPLY_BOOL("nowrap", style.nowrap);
    if(OverrideActive("margin_left")) style.metrics.content_margin.left = (int)OverrideValue("margin_left");
    if(OverrideActive("margin_top")) style.metrics.content_margin.top = (int)OverrideValue("margin_top");
    if(OverrideActive("margin_right")) style.metrics.content_margin.right = (int)OverrideValue("margin_right");
    if(OverrideActive("margin_bottom")) style.metrics.content_margin.bottom = (int)OverrideValue("margin_bottom");
    APPLY_BOOL("focus_enabled", style.metrics.focus_enabled);
    APPLY_INT("focus_margin", style.metrics.focus_margin);
    APPLY_INT("focus_alpha", style.metrics.focus_alpha);
    if(OverrideActive("focus_color")) style.metrics.focus_color = Color(OverrideValue("focus_color"));
    APPLY_BOOL("high_contrast", style.metrics.high_contrast);
    APPLY_BOOL("shadow_enabled", style.metrics.shadow.enabled);
    APPLY_INT("shadow_distance", style.metrics.shadow.distance);
    APPLY_INT("shadow_x", style.metrics.shadow.offset_x);
    APPLY_INT("shadow_y", style.metrics.shadow.offset_y);
    APPLY_INT("shadow_alpha", style.metrics.shadow.alpha);
    if(OverrideActive("shadow_color")) style.metrics.shadow.color = Color(OverrideValue("shadow_color"));
    APPLY_BOOL("shadow_inset", style.metrics.shadow.inset);
    if(OverrideActive("shadow_mode")) style.metrics.shadow.mode = AsString(OverrideValue("shadow_mode")) == "Hard" ? SHADOW_HARD : SHADOW_CURVE;
    if(OverrideActive("shadow_curve")) {
        ValueArray v = OverrideValue("shadow_curve");
        if(v.GetCount() >= 4) style.metrics.shadow.curve = ShadowCurve { (double)v[0], (double)v[1], (double)v[2], (double)v[3] };
    }
    APPLY_BOOL("highlight_enabled", style.metrics.highlight.enabled);
    APPLY_INT("highlight_thickness", style.metrics.highlight.thickness);
    APPLY_INT("highlight_x", style.metrics.highlight.offset_x);
    APPLY_INT("highlight_y", style.metrics.highlight.offset_y);
    APPLY_INT("highlight_alpha", style.metrics.highlight.alpha);
    if(OverrideActive("highlight_color")) style.metrics.highlight.color = Color(OverrideValue("highlight_color"));
    if(OverrideActive("highlight_style")) {
        String v = AsString(OverrideValue("highlight_style"));
        style.metrics.highlight.style = v == "Dashed" ? DASHED : v == "Dotted" ? DOTTED : SOLID;
    }
    if(OverrideActive("skin_image")) style.skin.base = LoadImageValue(OverrideValue("skin_image"));
    APPLY_BOOL("skin_enabled", style.skin.enabled);
    if(OverrideActive("skin_mode")) style.skin.image_mode = AsString(OverrideValue("skin_mode")) == "Fit" ? UiBackgroundImageMode::Fit : UiBackgroundImageMode::Fill;
    if(OverrideActive("skin_slice_left")) style.skin.slice.left = (int)OverrideValue("skin_slice_left");
    if(OverrideActive("skin_slice_top")) style.skin.slice.top = (int)OverrideValue("skin_slice_top");
    if(OverrideActive("skin_slice_right")) style.skin.slice.right = (int)OverrideValue("skin_slice_right");
    if(OverrideActive("skin_slice_bottom")) style.skin.slice.bottom = (int)OverrideValue("skin_slice_bottom");
    if(OverrideActive("skin_inset_left")) style.skin.content_inset.left = (int)OverrideValue("skin_inset_left");
    if(OverrideActive("skin_inset_top")) style.skin.content_inset.top = (int)OverrideValue("skin_inset_top");
    if(OverrideActive("skin_inset_right")) style.skin.content_inset.right = (int)OverrideValue("skin_inset_right");
    if(OverrideActive("skin_inset_bottom")) style.skin.content_inset.bottom = (int)OverrideValue("skin_inset_bottom");
#undef APPLY_BOOL
#undef APPLY_INT

    lbl_preview.SetCustomStyle(style);
    lbl_preview.Tip(AsString(InspectorValue("tooltip")));
    lbl_preview.Enable((bool)InspectorValue("enabled"));
    lbl_preview.SetSelectable((bool)InspectorValue("selectable"));
    lbl_preview.SetAlign(ParseAlign(AsString(InspectorValue("align_h"))), ParseAlign(AsString(InspectorValue("align_v"))));
    lbl_preview.SetIconSide(ParseAlign(AsString(InspectorValue("icon_side"))));
    lbl_preview.SetContentGap((int)InspectorValue("content_gap"));
    lbl_preview.SetIconSize((int)InspectorValue("icon_width"), (int)InspectorValue("icon_height"));
    lbl_preview.SetIconScaleToContent((bool)InspectorValue("scale_icon"));
    String icon_name = AsString(InspectorValue("icon"));
    UiIconRenderMode mode = AsString(InspectorValue("icon_mode")) == "Auto" ? UiIconRenderMode::Auto :
                            AsString(InspectorValue("icon_mode")) == "PreserveColor" ? UiIconRenderMode::PreserveColor : UiIconRenderMode::MonoTint;
    lbl_preview.SetIcon(UiIconFromName(icon_name), mode);
    if((bool)InspectorValue("rich_demo")) {
        lbl_preview.EnableRich().ClearSpans();
        lbl_preview.AddTextSpan("UiLabel ", Null, true)
                .AddIconSpan(UiIconFromName(icon_name), mode)
                .AddTextSpan(" rich spans", Null, false, true);
    }
    else {
        lbl_preview.EnableRich(false).SetText(AsString(InspectorValue("text")));
    }
    UpdateGeneratedCode();
    RefreshLayout();
}

void UiLabelDemo::UpdateOverrideSummaries()
{
    VectorMap<String, int> active, total;
    for(int n = 0; n < pe_model_override.GetCount(); n++) {
        const PropertyEditorItem& item = pe_model_override[n];
        total.GetAdd(item.group, 0)++;
        if(item.override_active) active.GetAdd(item.group, 0)++;
    }
    for(int i = 0; i < total.GetCount(); i++)
        pe_model_override.SetGroupSubtitle(total.GetKey(i),
            Format("%d of %d local", active.Get(total.GetKey(i), 0), total[i]));
}

void UiLabelDemo::SetOverrideActive(const String& id, bool active)
{
    PropertyEditorItem *item = pe_model_override.Find(id);
    if(!item) return;
    item->override_active = active;
    pe_model_override.StructureChanged();
    UpdateOverrideSummaries();
    pe_overrides.RefreshModel();
    ApplyProjection();
}

void UiLabelDemo::ResetProperty(PropertyEditorModel& model, const String& id)
{
    PropertyEditorItem *item = model.Find(id);
    if(!item || !item->resettable) return;
    model.SetValue(id, item->default_value);
    ApplyProjection();
}

void UiLabelDemo::UpdateGeneratedCode()
{
    String out;
    out << "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\n"
           "class LabelExample : public ParentCtrl {\npublic:\n    UiLabel label;\n\n    LabelExample()\n    {\n        Add(label.HCenterPos(DPI(" << (int)InspectorValue("width")
        << ")).VCenterPos(DPI(" << (int)InspectorValue("height") << ")));\n";
    const String role = AsString(InspectorValue("role"));
    const String size = AsString(InspectorValue("text_size"));
    bool any_override = false;
    for(int n = 0; n < pe_model_override.GetCount(); n++)
        any_override |= pe_model_override[n].override_active;
    if(any_override) {
        out << "        UiLabel::Style style = UiTheme::ResolveLabel(" << RoleCode(role) << ", " << TextSizeCode(size) << ");\n";
        for(int i = 0; i < 4; i++) {
            String state = StateId(i);
            if(OverrideActive("face." + state)) out << "        style.palette.face[" << StateCode(i) << "] = " << FillRecipeCode(OverrideValue("face." + state)) << ";\n";
            if(OverrideActive("frame." + state)) out << "        style.palette.frame[" << StateCode(i) << "] = " << CppColor(Color(OverrideValue("frame." + state))) << ";\n";
            if(OverrideActive("ink." + state)) out << "        style.palette.ink[" << StateCode(i) << "] = " << CppColor(Color(OverrideValue("ink." + state))) << ";\n";
            if(OverrideActive("icon." + state)) out << "        style.palette.icon[" << StateCode(i) << "] = " << CppColor(Color(OverrideValue("icon." + state))) << ";\n";
        }
        auto emit_bool = [&](const char *id, const char *field) { if(OverrideActive(id)) out << "        " << field << " = " << ((bool)OverrideValue(id) ? "true" : "false") << ";\n"; };
        auto emit_int = [&](const char *id, const char *field, bool dpi = false) { if(OverrideActive(id)) out << "        " << field << " = " << (dpi ? "DPI(" : "") << (int)OverrideValue(id) << (dpi ? ")" : "") << ";\n"; };
        emit_bool("face_enabled", "style.metrics.face_enabled"); emit_bool("frame_enabled", "style.metrics.frame_enabled");
        emit_int("frame_width", "style.metrics.frame_width", true); emit_int("radius", "style.metrics.radius", true);
        emit_bool("dashed", "style.metrics.dashed"); emit_bool("transparent", "style.transparent");
        if(OverrideActive("dash_pattern")) out << "        style.metrics.dash_pattern = " << CppString(AsString(OverrideValue("dash_pattern"))) << ";\n";
        if(OverrideActive("font_face")) out << "        style.font.FaceName(" << CppString(AsString(OverrideValue("font_face"))) << ");\n";
        if(OverrideActive("font_height")) out << "        style.font.Height(" << (int)OverrideValue("font_height") << ");\n";
        if(OverrideActive("font_bold")) out << "        style.font.Bold(" << ((bool)OverrideValue("font_bold") ? "true" : "false") << ");\n";
        if(OverrideActive("font_italic")) out << "        style.font.Italic(" << ((bool)OverrideValue("font_italic") ? "true" : "false") << ");\n";
        emit_bool("underline", "style.underline"); emit_int("underline_width", "style.underline_width", true); emit_int("underline_offset", "style.underline_offset", true); emit_bool("nowrap", "style.nowrap");
        const char *margin_id[] = { "margin_left", "margin_top", "margin_right", "margin_bottom" };
        const char *margin_field[] = { "left", "top", "right", "bottom" };
        for(int i = 0; i < 4; i++) if(OverrideActive(margin_id[i])) out << "        style.metrics.content_margin." << margin_field[i] << " = DPI(" << (int)OverrideValue(margin_id[i]) << ");\n";
        emit_bool("focus_enabled", "style.metrics.focus_enabled"); emit_int("focus_margin", "style.metrics.focus_margin", true); emit_int("focus_alpha", "style.metrics.focus_alpha"); emit_bool("high_contrast", "style.metrics.high_contrast");
        if(OverrideActive("focus_color")) out << "        style.metrics.focus_color = " << CppColor(Color(OverrideValue("focus_color"))) << ";\n";
        emit_bool("shadow_enabled", "style.metrics.shadow.enabled"); emit_int("shadow_distance", "style.metrics.shadow.distance", true); emit_int("shadow_x", "style.metrics.shadow.offset_x", true); emit_int("shadow_y", "style.metrics.shadow.offset_y", true); emit_int("shadow_alpha", "style.metrics.shadow.alpha"); emit_bool("shadow_inset", "style.metrics.shadow.inset");
        if(OverrideActive("shadow_color")) out << "        style.metrics.shadow.color = " << CppColor(Color(OverrideValue("shadow_color"))) << ";\n";
        if(OverrideActive("shadow_mode")) out << "        style.metrics.shadow.mode = " << (AsString(OverrideValue("shadow_mode")) == "Hard" ? "SHADOW_HARD" : "SHADOW_CURVE") << ";\n";
        if(OverrideActive("shadow_curve")) { ValueArray v = OverrideValue("shadow_curve"); out << Format("        style.metrics.shadow.curve = ShadowCurve { %.4f, %.4f, %.4f, %.4f };\n", (double)v[0], (double)v[1], (double)v[2], (double)v[3]); }
        emit_bool("highlight_enabled", "style.metrics.highlight.enabled"); emit_int("highlight_thickness", "style.metrics.highlight.thickness", true); emit_int("highlight_x", "style.metrics.highlight.offset_x", true); emit_int("highlight_y", "style.metrics.highlight.offset_y", true); emit_int("highlight_alpha", "style.metrics.highlight.alpha");
        if(OverrideActive("highlight_color")) out << "        style.metrics.highlight.color = " << CppColor(Color(OverrideValue("highlight_color"))) << ";\n";
        if(OverrideActive("highlight_style")) out << "        style.metrics.highlight.style = " << ToUpper(AsString(OverrideValue("highlight_style"))) << ";\n";
        if(OverrideActive("skin_image") && !AsString(OverrideValue("skin_image")).IsEmpty()) out << "        style.skin.base = StreamRaster::LoadFileAny(" << CppString(AsString(OverrideValue("skin_image"))) << ");\n";
        emit_bool("skin_enabled", "style.skin.enabled");
        if(OverrideActive("skin_mode")) out << "        style.skin.image_mode = UiBackgroundImageMode::" << AsString(OverrideValue("skin_mode")) << ";\n";
        const char *skin_id[] = { "skin_slice_left", "skin_slice_top", "skin_slice_right", "skin_slice_bottom", "skin_inset_left", "skin_inset_top", "skin_inset_right", "skin_inset_bottom" };
        const char *skin_field[] = { "style.skin.slice.left", "style.skin.slice.top", "style.skin.slice.right", "style.skin.slice.bottom", "style.skin.content_inset.left", "style.skin.content_inset.top", "style.skin.content_inset.right", "style.skin.content_inset.bottom" };
        for(int i = 0; i < 8; i++) emit_int(skin_id[i], skin_field[i], true);
        out << "        label.SetCustomStyle(style);\n";
    }
    else
        out << "        label.SetCustomStyle(UiTheme::ResolveLabel(" << RoleCode(role) << ", " << TextSizeCode(size) << "));\n";

    String icon = AsString(InspectorValue("icon"));
    String icon_mode = AsString(InspectorValue("icon_mode"));
    if((bool)InspectorValue("rich_demo")) {
        out << "        label.EnableRich()\n"
               "             .AddTextSpan(\"UiLabel \", Null, true)\n"
               "             .AddIconSpan(UiIconFromName(" << CppString(icon) << "), UiIconRenderMode::" << icon_mode << ")\n"
               "             .AddTextSpan(\" rich spans\", Null, false, true);\n";
    }
    else
        out << "        label.SetText(" << CppString(AsString(InspectorValue("text"))) << ");\n";
    out << "        label.Tip(" << CppString(AsString(InspectorValue("tooltip"))) << ");\n";
    out << "        label.SetIcon(UiIconFromName(" << CppString(icon) << "), UiIconRenderMode::" << icon_mode << ")\n"
           "             .SetIconSize(DPI(" << (int)InspectorValue("icon_width") << "), DPI(" << (int)InspectorValue("icon_height") << "))\n"
           "             .SetIconSide(" << AlignCode(AsString(InspectorValue("icon_side"))) << ")\n"
           "             .SetContentGap(DPI(" << (int)InspectorValue("content_gap") << "))\n"
           "             .SetIconScaleToContent(" << ((bool)InspectorValue("scale_icon") ? "true" : "false") << ")\n"
           "             .SetAlign(" << AlignCode(AsString(InspectorValue("align_h"))) << ", " << AlignCode(AsString(InspectorValue("align_v"))) << ")\n"
           "             .SetSelectable(" << ((bool)InspectorValue("selectable") ? "true" : "false") << ")\n"
           "             .Enable(" << ((bool)InspectorValue("enabled") ? "true" : "false") << ");\n"
           "    }\n};\n";
    str_generated_code = out;
    edit_generated_code.SetData(str_generated_code);
}

bool UiLabelDemo::PickImage(Value& value, Ctrl *)
{
    FileSel selector;
    selector.Type("Images", "*.png *.bmp *.jpg *.jpeg");
    if(!AsString(value).IsEmpty()) selector.Set(AsString(value));
    if(!selector.ExecuteOpen("Choose label skin image")) return false;
    value = ~selector;
    return true;
}

Image UiLabelDemo::LoadImageValue(const Value& value) const
{
    String path = AsString(value);
    return path.IsEmpty() ? Image() : StreamRaster::LoadFileAny(path);
}

void UiLabelDemo::SelectPage(int page)
{
    page = minmax(page, 0, 2);
    stk_right_pages.SetActivePage(page);
    btn_inspector_mode.SetChecked(page == 0);
    btn_overrides_mode.SetChecked(page == 1);
    btn_code_mode.SetChecked(page == 2);
}

void UiLabelDemo::ToggleTheme()
{
    UiThemeContext context = UiTheme::GetContext();
    context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
    UiTheme::Set(context);
    Ctrl::SwapDarkLight();
    ApplyTheme();
    ApplyProjection();
}

void UiLabelDemo::ApplyTheme()
{
    UiTitleCard::Style header_style = UiTheme::ResolveTitleCard(UiRole::Accent);
    header_style.title_line = false;
    header_style.card_line = true;
    header_style.card_line_style = SOLID;
    header_style.card_line_length = LARGE;
    header_style.card_line_side = UiAlign::BOTTOM;
    header_style.card_line_thickness = DPI(1);
    header_style.card_line_gap = 0;
    header_style.card_line_color_enabled = true;
    header_style.card_line_color = Color(0, 120, 212);
    header_style.media_tint_mono = true;
    tc_header.SetCustomStyle(header_style);
    pnl_preview.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
    pnl_right_rail.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
    pnl_inspector_page.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
    pnl_overrides_page.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
    pnl_code_page.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
    lbl_preview_caption.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
    btn_exit.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
    pe_inspector.SetPaletteMode(UiTheme::GetContext().mode == UiThemeMode::Dark ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light);
    pe_overrides.SetPaletteMode(UiTheme::GetContext().mode == UiThemeMode::Dark ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light);
}

void UiLabelDemo::Layout()
{
    Rect client = GetSize();
    const int pad = DPI(12), gap = DPI(10), header_h = DPI(72);
    const int right_w = min(DPI(430), max(DPI(330), client.GetWidth() * 35 / 100));
    tc_header.SetRect(pad, pad, max(0, client.GetWidth() - 2 * pad), header_h);
    int top = pad + header_h + gap;
    int body_h = max(0, client.GetHeight() - top - pad);
    int preview_w = max(0, client.GetWidth() - 3 * pad - right_w);
    pnl_preview.SetRect(pad, top, preview_w, body_h);
    pnl_right_rail.SetRect(pad + preview_w + gap, top, right_w, body_h);

    Rect pr = pnl_preview.GetSize();
    int width = min((int)InspectorValue("width"), max(0, pr.GetWidth() - DPI(48)));
    int height = min((int)InspectorValue("height"), max(0, pr.GetHeight() - DPI(100)));
    lbl_preview.SetRect((pr.GetWidth() - width) / 2, (pr.GetHeight() - height) / 2, width, height);
    lbl_preview_caption.SetRect(DPI(18), max(0, pr.bottom - DPI(44)), max(0, pr.GetWidth() - DPI(36)), DPI(28));

    Rect rr = pnl_right_rail.GetSize();
    box_right_tools.SetRect(0, 0, max(0, rr.GetWidth()), DPI(42));
    stk_right_pages.SetRect(DPI(6), DPI(52), max(0, rr.GetWidth() - DPI(12)), max(0, rr.GetHeight() - DPI(58)));
}

} // namespace Upp
