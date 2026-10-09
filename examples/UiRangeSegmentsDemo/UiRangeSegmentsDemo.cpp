#include "UiRangeSegmentsDemo.h"

namespace Upp {
namespace {
// Frame Accent is an authored addition to the ordinary frame, not layout padding.
void AddFrameAccentProperties(PropertyEditorModel& model, const String& prefix,
                              const StyledMetrics& metrics, const String& group)
{
    const char* edge[] = { "top", "bottom", "left", "right" };
    const char* label[] = { "Top", "Bottom", "Left", "Right" };
    const int mask[] = { StyledFrameAccent::Top, StyledFrameAccent::Bottom,
                         StyledFrameAccent::Left, StyledFrameAccent::Right };
    auto mark = [](PropertyEditorItem& item) { item.overrideable = true; item.SetDefault(item.value); };
    for(int i = 0; i < 4; i++)
        mark(model.AddBoolean(prefix + edge[i], label[i], bool(metrics.frame_accent.edges & mask[i]), group));
    mark(model.AddNumericInt(prefix + "thickness", "Thickness", metrics.frame_accent.thickness, 0, DPI(12), 1, group).SetUnit("px"));
    mark(model.AddNumericInt(prefix + "alpha", "Opacity", metrics.frame_accent.alpha, 0, 255, 1, group));
    mark(model.AddColor(prefix + "color", "Colour (Null follows frame)", metrics.frame_accent.color, group));
}
void ApplyFrameAccentProperties(StyledMetrics& metrics, const PropertyEditorModel& model, const String& prefix)
{
    const char* edge[] = { "top", "bottom", "left", "right" };
    const int mask[] = { StyledFrameAccent::Top, StyledFrameAccent::Bottom,
                         StyledFrameAccent::Left, StyledFrameAccent::Right };
    for(int i = 0; i < 4; i++) {
        const auto* row = model.Find(prefix + edge[i]);
        if(row && row->override_active) {
            if((bool)row->value) metrics.frame_accent.edges |= mask[i];
            else metrics.frame_accent.edges &= ~mask[i];
        }
    }
    const auto* row = model.Find(prefix + "thickness");
    if(row && row->override_active) metrics.frame_accent.thickness = (int)row->value;
    row = model.Find(prefix + "alpha");
    if(row && row->override_active) metrics.frame_accent.alpha = (int)row->value;
    row = model.Find(prefix + "color");
    if(row && row->override_active) metrics.frame_accent.color = Color(row->value);
}
void EmitFrameAccentProperties(String& code, const PropertyEditorModel& model, const String& prefix,
                               const String& target, const String& declaration, bool& authored)
{
    const char* edge[] = { "top", "bottom", "left", "right" };
    const char* label[] = { "Top", "Bottom", "Left", "Right" };
    for(int i = 0; i < 4; i++) {
        const auto* row = model.Find(prefix + edge[i]);
        if(!row || !row->override_active) continue;
        if(!authored) code << declaration;
        authored = true;
        code << target << ".frame_accent.edges " << ((bool)row->value ? "|= " : "&= ~")
             << "StyledFrameAccent::" << label[i] << ";\n";
    }
    for(const char* field : { "thickness", "alpha", "color" }) {
        const auto* row = model.Find(prefix + field);
        if(!row || !row->override_active) continue;
        if(!authored) code << declaration;
        authored = true;
        code << target << ".frame_accent." << field << " = ";
        if(String(field) == "color") {
            Color color(row->value);
            code << (IsNull(color) ? String("Null") : Format("Color(%d, %d, %d)", color.GetR(), color.GetG(), color.GetB()));
        }
        else code << (int)row->value;
        code << ";\n";
    }
}


PropertyEditorItem& MarkOverride(PropertyEditorItem& item)
{
    item.overrideable = true;
    item.override_active = false;
    item.SetDefault(item.value);
    return item;
}

UiRole ParseRole(const String& value)
{
    if(value == "Subtle") return UiRole::Subtle;
    if(value == "Accent") return UiRole::Accent;
    if(value == "Alert") return UiRole::Alert;
    return UiRole::Standard;
}

UiDirection ParseDirection(const String& value)
{
    return value == "Vertical" ? UiDirection::V : UiDirection::H;
}

UiRangeSegments::PaletteMode ParsePaletteMode(const String& value)
{
    return value == "Gradient samples"
         ? UiRangeSegments::PaletteMode::Gradient
         : UiRangeSegments::PaletteMode::Series;
}

UiRangeSegments::ValueDisplay ParseValueDisplay(const String& value)
{
    return value == "Domain values"
         ? UiRangeSegments::ValueDisplay::Domain
         : UiRangeSegments::ValueDisplay::Percent;
}

String CppString(const String& s)
{
    String out = "\"";
    for(int i = 0; i < s.GetCount(); i++) {
        int c = s[i];
        if(c == '\\') out << "\\\\";
        else if(c == '"') out << "\\\"";
        else if(c == '\n') out << "\\n";
        else if(c == '\r') out << "\\r";
        else if(c == '\t') out << "\\t";
        else out.Cat(c);
    }
    return out << '"';
}

String CppScalar(double value)
{
    // Significant digits preserve the scalar model; readout precision is visual.
    return FormatDouble(value, 17, FD_TOLERANCE(6) | FD_MINIMAL_EXP);
}

String CppColor(Color c)
{
    if(IsNull(c))
        return "Null";
    return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

Color FaceColor(const StyledPalette& palette, StyledState st, Color fallback)
{
    const UiFill& fill = palette.face[st];
    return fill.IsSolid() && !IsNull(fill.color) ? fill.color : fallback;
}

} // namespace

UiRangeSegmentsDemo::UiRangeSegmentsDemo()
{
    Title("UiRangeSegments Demo");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1280), DPI(790));

    UiThemeContext context = UiTheme::GetContext();
    context.preset = UiThemePreset::Minimal;
    context.mode = UiThemeMode::Light;
    UiTheme::Set(context);

    Vector<UiRangeSegment> acts;
    const char* subtitles[] = { "The setup", "Rising action", "The turning point", "The resolution" };
    for(int i = 0; i < 4; i++) {
        UiRangeSegment& act = acts.Add(UiRangeSegment(25, Format("Act %d", i + 1)));
        act.subtitle = subtitles[i];
    }
    ranges_.SetRange(0, 100).SetSegments(acts).SetValueDisplay(UiRangeSegments::ValueDisplay::Percent);

    RegisterPropertyEditorEditors(factory_);
    BuildHeader();
    BuildPreview();
    BuildRightRail();
    BuildInspector();
    BuildOverrides();
    BuildDataModel();
    ConfigureEditors();
    ConnectEvents();
    ApplyTheme();
    SelectPage(0);
    ApplyProjection();
}

void UiRangeSegmentsDemo::Layout()
{
    Rect r = GetSize();
    r.Deflate(DPI(12));
    header_.SetRect(r.left, r.top, r.GetWidth(), DPI(68));

    int top = r.top + DPI(80);
    int body_h = max(0, r.bottom - top);
    int rail_w = min(DPI(430), max(DPI(360), r.GetWidth() / 3));
    int gap = DPI(12);
    int preview_w = max(0, r.GetWidth() - rail_w - gap);
    preview_.SetRect(r.left, top, preview_w, body_h);
    right_.SetRect(r.left + preview_w + gap, top, rail_w, body_h);

    Size ps = preview_.GetSize();
    const int selector_h = DPI(44);
    preview_modes_.SetRect(DPI(12), DPI(8), max(0, ps.cx - DPI(24)), DPI(30));
    int caption_h = DPI(54);
    int area_h = max(0, ps.cy - caption_h - selector_h);
    int rw = min(max(DPI(90), (int)InspectorValue("width", 650)), max(0, ps.cx - DPI(28)));
    int rh = min(max(DPI(70), (int)InspectorValue("height", 220)), max(0, area_h - DPI(28)));
    ranges_.SetRect(max(0, (ps.cx - rw) / 2), selector_h + max(0, (area_h - rh) / 2), rw, rh);
    caption_.SetRect(0, max(0, ps.cy - caption_h), ps.cx, caption_h);

    Size rs = right_.GetSize();
    tools_.SetRect(DPI(4), DPI(4), max(0, rs.cx - DPI(8)), DPI(36));
    pages_.SetRect(DPI(4), DPI(44), max(0, rs.cx - DPI(8)), max(0, rs.cy - DPI(48)));
}

void UiRangeSegmentsDemo::BuildHeader()
{
    Add(header_);
    header_.SetTitle("UiRangeSegments")
           .SetSubTitle("One scalar model: compact ranges or editable Act cards, with live percentages")
           .ShowTitleLine(false)
           .SetContentInset(DPI(8))
           .SetContentCell(header_actions_);

    header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    header_actions_.AddSpacer(1).Expand(1);
    theme_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark theme");
    help_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("About this demo");
    exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close demo");
    header_actions_.Add(theme_).Fixed(DPI(34));
    header_actions_.Add(help_).Fixed(DPI(34));
    header_actions_.Add(exit_).Fixed(DPI(34));
}

void UiRangeSegmentsDemo::BuildPreview()
{
    Add(preview_);
    preview_.Add(preview_modes_);
    preview_modes_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    range_mode_.SetText("Range").SetCheckable();
    cards_mode_.SetText("Cards").SetCheckable();
    preview_modes_.Add(range_mode_).Fixed(DPI(76));
    preview_modes_.Add(cards_mode_).Fixed(DPI(76));
    preview_.Add(ranges_);
    preview_.Add(caption_);
    caption_.SetText("Drag a boundary to redistribute the two neighbouring spans. Percentages follow their actual share of the fixed domain; horizontal and vertical use the same data.")
            .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
}

void UiRangeSegmentsDemo::SelectPreview(bool cards)
{
    cards_preview_ = cards;
    const bool vertical = AsString(InspectorValue("direction")) == "Vertical";
    inspector_model_.SetValue("width", vertical ? 240 : 720, false);
    inspector_model_.SetValue("height", vertical ? 440 : (cards ? 160 : 220), false);
    inspector_model_.SetValue("show_segment_values", cards, false);
    for(const char* id : { "show_boundary_values", "show_endpoint_values", "show_values_on_interaction" })
        inspector_model_.SetValue(id, !cards, false);
    for(const char* id : { "width", "height", "show_segment_values", "show_boundary_values", "show_endpoint_values", "show_values_on_interaction" })
        inspector_model_.ValueChanged(id);
    ApplyProjection();
}

void UiRangeSegmentsDemo::ApplyCardPreset(UiRangeSegments::Style& style) const
{
    const bool vertical = ranges_.GetDirection() == UiDirection::V;
    const Color face = FaceColor(style.track_palette, ST_NORMAL, SColorPaper());
    const Color ink = IsNull(style.value_palette.ink[ST_NORMAL]) ? SColorText() : style.value_palette.ink[ST_NORMAL];
    for(int i = 0; i < style.series_count; i++)
        style.series[i] = Blend(face, ink, 7 + i * 2);
    style.track_size.cy = vertical ? DPI(240) : DPI(64);
    style.track_metrics.radius = DPI(8);
    style.track_metrics.frame_width = DPI(1);
    style.track_metrics.frame_enabled = true;
    style.thumb_shape = UiRangeSegments::ThumbShape::RoundedRectangle;
    style.thumb_size = Size(DPI(6), DPI(28));
    style.thumb_rotate_with_direction = true;
    style.thumb_hover_growth = DPI(1);
    style.thumb_metrics.radius = DPI(3);
    style.thumb_metrics.frame_width = DPI(1);
    style.thumb_metrics.face_enabled = true;
    style.thumb_metrics.frame_enabled = true;
    style.thumb_dot_diameter = 0;
    for(int state = 0; state < 4; state++) {
        style.thumb_palette.face[state] = UiFill::Solid(Blend(face, ink, state == ST_PRESSED ? 30 : state == ST_HOT ? 14 : 0));
        style.thumb_palette.frame[state] = Blend(face, ink, state == ST_DISABLED ? 35 : state == ST_HOT ? 150 : 110);
    }
    style.label_font.Height(DPI(13)).Bold();
    style.subtitle_font.Height(DPI(10));
    style.right_font.Height(DPI(12)).Bold();
    style.label_align = UiAlign::DEFAULT;
    style.label_padding = DPI(12);
    style.label_gap = DPI(3);
    style.value_gap = DPI(12);
}

void UiRangeSegmentsDemo::EmitCardPreset(String& code) const
{
    code << "// Cards: a small composition over the current role and theme.\n";
    code << "const Color face = style.track_palette.face[ST_NORMAL].IsSolid() ? style.track_palette.face[ST_NORMAL].color : SColorPaper();\n";
    code << "const Color ink = IsNull(style.value_palette.ink[ST_NORMAL]) ? SColorText() : style.value_palette.ink[ST_NORMAL];\n";
    code << "for(int i = 0; i < style.series_count; i++)\n";
    code << "    style.series[i] = Blend(face, ink, 7 + i * 2);\n";
    code << "style.track_size.cy = ranges.GetDirection() == UiDirection::V ? DPI(240) : DPI(64);\n";
    code << "style.track_metrics.radius = DPI(8);\n";
    code << "style.track_metrics.frame_width = DPI(1);\n";
    code << "style.track_metrics.frame_enabled = true;\n";
    code << "style.thumb_shape = UiRangeSegments::ThumbShape::RoundedRectangle;\n";
    code << "style.thumb_size = Size(DPI(6), DPI(28));\n";
    code << "style.thumb_rotate_with_direction = true;\n";
    code << "style.thumb_hover_growth = DPI(1);\n";
    code << "style.thumb_metrics.radius = DPI(3);\n";
    code << "style.thumb_metrics.frame_width = DPI(1);\n";
    code << "style.thumb_metrics.face_enabled = true;\n";
    code << "style.thumb_metrics.frame_enabled = true;\n";
    code << "style.thumb_dot_diameter = 0;\n";
    code << "for(int state = 0; state < 4; state++) {\n";
    code << "    style.thumb_palette.face[state] = UiFill::Solid(Blend(face, ink, state == ST_PRESSED ? 30 : state == ST_HOT ? 14 : 0));\n";
    code << "    style.thumb_palette.frame[state] = Blend(face, ink, state == ST_DISABLED ? 35 : state == ST_HOT ? 150 : 110);\n";
    code << "}\n";
    code << "style.label_font.Height(DPI(13)).Bold();\n";
    code << "style.subtitle_font.Height(DPI(10));\n";
    code << "style.right_font.Height(DPI(12)).Bold();\n";
    code << "style.label_align = UiAlign::DEFAULT;\n";
    code << "style.label_padding = DPI(12);\n";
    code << "style.label_gap = DPI(3);\n";
    code << "style.value_gap = DPI(12);\n";
}

void UiRangeSegmentsDemo::BuildRightRail()
{
    Add(right_);
    right_.Add(tools_);
    right_.Add(pages_);
    tools_.SetGap(DPI(4)).SetInset(Rect(DPI(2), 0, DPI(2), 0)).SetAlignItems(UiCrossAlign::Center);

    inspector_mode_.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Inspector");
    overrides_mode_.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Theme Overrides");
    data_mode_.SetIcon(ICON_DESIGN_WIDGETS_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Segment Data");
    code_mode_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Generated C++");
    tools_.Add(inspector_mode_).Fixed(DPI(38));
    tools_.Add(overrides_mode_).Fixed(DPI(38));
    tools_.Add(data_mode_).Fixed(DPI(38));
    tools_.Add(code_mode_).Fixed(DPI(38));
    tools_.AddSpacer(1).Expand(1);

    pages_.Add(inspector_page_, "inspector");
    pages_.Add(overrides_page_, "overrides");
    pages_.Add(data_page_, "data");
    pages_.Add(code_page_, "code");
    inspector_page_.Add(inspector_.SizePos());
    overrides_page_.Add(overrides_.SizePos());
    data_page_.Add(data_.SizePos());
    code_page_.Add(code_);
    code_.HSizePos(DPI(6), DPI(6)).VSizePos(DPI(42), DPI(6));
    code_page_.Add(copy_.RightPos(DPI(8), DPI(32)).TopPos(DPI(6), DPI(30)));
    code_.SetReadOnly();
    copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16), DPI(16)).Tip("Copy generated C++");
}

void UiRangeSegmentsDemo::BuildInspector()
{
    inspector_model_.AddNumericDouble("minimum", "Minimum", 0.0, -100000.0, 100000.0, 1.0, "Domain");
    inspector_model_.AddNumericDouble("maximum", "Maximum", 100.0, -100000.0, 100000.0, 1.0, "Domain");
    inspector_model_.AddNumericDouble("step", "Step", 1.0, 0.0, 10000.0, 0.1, "Domain");
    inspector_model_.AddNumericDouble("min_span", "Minimum span", 0.0, 0.0, 10000.0, 1.0, "Domain");
    inspector_model_.AddNumericInt("segment_count", "Segments", 4, 1, 8, 1, "Segments");

    inspector_model_.AddChoice("direction", "Direction", "Horizontal", "Presentation")
                    .AddChoice("Horizontal", "Horizontal").AddChoice("Vertical", "Vertical");
    inspector_model_.AddBoolean("reverse", "Reverse direction", false, "Presentation");
    inspector_model_.AddChoice("palette_mode", "Palette", "Series", "Presentation")
                    .AddChoice("Series", "Series").AddChoice("Gradient samples", "Gradient samples");
    inspector_model_.AddChoice("value_display", "Values", "Percent", "Presentation")
                    .AddChoice("Percent", "Percent").AddChoice("Domain values", "Domain values");
    inspector_model_.AddNumericInt("precision", "Decimals", 0, 0, 6, 1, "Presentation");
    inspector_model_.AddBoolean("show_labels", "Title and subtitle", true, "Presentation");
    inspector_model_.AddBoolean("show_segment_values", "Segment percentages", true, "Presentation");
    inspector_model_.AddChoice("value_side", "Value side", "Right", "Presentation")
                    .AddChoice("Left", "Left").AddChoice("Right", "Right");
    inspector_model_.AddBoolean("show_boundary_values", "Boundary values", false, "Presentation");
    inspector_model_.AddBoolean("show_endpoint_values", "Endpoint values", false, "Presentation");
    inspector_model_.AddBoolean("show_values_on_interaction", "Values while moving", false, "Presentation");
    inspector_model_.AddBoolean("show_dividers", "Divider lines", true, "Presentation");

    inspector_model_.AddChoice("role", "Role", "Standard", "Theme")
                    .AddChoice("Standard", "Standard").AddChoice("Subtle", "Subtle")
                    .AddChoice("Accent", "Accent").AddChoice("Alert", "Alert");
    inspector_model_.AddNumericInt("width", "Preview width", 720, 90, 1000, 1, "Layout").SetUnit("px");
    inspector_model_.AddNumericInt("height", "Preview height", 160, 70, 800, 1, "Layout").SetUnit("px");
    inspector_model_.AddBoolean("enabled", "Enabled", true, "Behaviour");

    inspector_model_.SetGroupSubtitle("Domain", "one fixed scalar range; boundary edits never move its endpoints");
    inspector_model_.SetGroupSubtitle("Segments", "changing count creates equal contiguous spans; edit labels/weights on Data");
    inspector_model_.SetGroupSubtitle("Presentation", "labels and numeric readouts are optional; vertical uses the same scalar model");
    inspector_model_.StructureChanged();
}

void UiRangeSegmentsDemo::BuildOverrides()
{
    UiRangeSegments probe;
    UiRangeSegments::Style base = probe.GetStyle();
    if(cards_preview_)
        ApplyCardPreset(base);
    AddFrameAccentProperties(override_model_, "track_metrics.frame_accent.", base.track_metrics, "Track / Frame Accent");
    MarkOverride(override_model_.AddColor("track.face", "Face", FaceColor(base.track_palette, ST_NORMAL, SColorFace()), "Track"));
    MarkOverride(override_model_.AddColor("thumb.face", "Face", FaceColor(base.thumb_palette, ST_NORMAL, SColorPaper()), "Boundary Thumb"));
    MarkOverride(override_model_.AddNumericInt("track.height", "Thickness", base.track_size.cy, 1, 300, 1, "Track").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("track.radius", "Radius", base.track_metrics.radius, 0, 60, 1, "Track").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("track.frame_width", "Frame width", base.track_metrics.frame_width, 0, 8, 1, "Track").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("thumb.width", "Width", base.thumb_size.cx, 1, 60, 1, "Boundary Thumb").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("thumb.height", "Height", base.thumb_size.cy, 1, 100, 1, "Boundary Thumb").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("thumb.radius", "Radius", base.thumb_metrics.radius, 0, 60, 1, "Boundary Thumb").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("thumb.frame_width", "Frame width", base.thumb_metrics.frame_width, 0, 8, 1, "Boundary Thumb").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("thumb.dot", "Centre dot", base.thumb_dot_diameter, 0, 16, 1, "Boundary Thumb").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("thumb.hover_growth", "Hover growth", base.thumb_hover_growth, 0, 12, 1, "Boundary Thumb").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("selected.width", "Selected width", base.selected_frame_width, 1, 8, 1, "Selection").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("label.padding", "Content inset", base.label_padding, 0, 40, 1, "Text Layout").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("label.gap", "Title/subtitle gap", base.label_gap, 0, 24, 1, "Text Layout").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("value.gap", "Value gap", base.value_gap, 0, 40, 1, "Text Layout").SetUnit("px"));
    MarkOverride(override_model_.AddBoolean("track.face_enabled", "Show face", base.track_metrics.face_enabled, "Track"));
    MarkOverride(override_model_.AddBoolean("track.frame_enabled", "Show frame", base.track_metrics.frame_enabled, "Track"));
    MarkOverride(override_model_.AddBoolean("thumb.face_enabled", "Show face", base.thumb_metrics.face_enabled, "Boundary Thumb"));
    MarkOverride(override_model_.AddBoolean("thumb.frame_enabled", "Show frame", base.thumb_metrics.frame_enabled, "Boundary Thumb"));
    MarkOverride(override_model_.AddBoolean("thumb.rotate", "Rotate in vertical mode", base.thumb_rotate_with_direction, "Boundary Thumb"));
    MarkOverride(override_model_.AddColor("track.frame", "Frame", base.track_palette.frame[ST_NORMAL], "Track"));
    MarkOverride(override_model_.AddColor("thumb.frame", "Frame", base.thumb_palette.frame[ST_NORMAL], "Boundary Thumb"));
    MarkOverride(override_model_.AddColor("divider", "Divider", base.divider_color, "Selection"));
    MarkOverride(override_model_.AddColor("selected", "Selected frame", base.selected_frame, "Selection"));
    MarkOverride(override_model_.AddColor("label.color", "Title ink (Null = contrast)", base.label_color, "Text Layout"));
    MarkOverride(override_model_.AddColor("subtitle.color", "Subtitle ink (Null = contrast)", base.subtitle_color, "Text Layout"));
    MarkOverride(override_model_.AddColor("value.color", "Value ink (Null = contrast)", base.value_color, "Text Layout"));
    MarkOverride(override_model_.AddChoice("thumb.shape", "Shape", base.thumb_shape == UiRangeSegments::ThumbShape::Ellipse ? "Ellipse" : "Rounded rectangle", "Boundary Thumb")
                 .AddChoice("Ellipse", "Ellipse").AddChoice("Rounded rectangle", "Rounded rectangle"));
    MarkOverride(override_model_.AddChoice("label.align", "Title/subtitle alignment", "Automatic", "Text Layout")
                 .AddChoice("Automatic", "Automatic").AddChoice("Left", "Left")
                 .AddChoice("Centre", "Centre").AddChoice("Right", "Right"));
    for(int i = 0; i < 6; i++)
        MarkOverride(override_model_.AddColor("series." + AsString(i), Format("Series %d", i + 1), base.series[i], "Series"));
    MarkOverride(AddPropertyFont(override_model_, "label.face", "Face", UiFonts::Selection(base.label_font), "Title Typography"));
    MarkOverride(override_model_.AddNumericInt("label.font", "Size", base.label_font.GetHeight(), 7, 36, 1, "Title Typography").SetUnit("px"));
    MarkOverride(override_model_.AddBoolean("label.bold", "Bold", base.label_font.IsBold(), "Title Typography"));
    MarkOverride(override_model_.AddBoolean("label.italic", "Italic", base.label_font.IsItalic(), "Title Typography"));
    MarkOverride(AddPropertyFont(override_model_, "subtitle.face", "Face", UiFonts::Selection(base.subtitle_font), "Subtitle Typography"));
    MarkOverride(override_model_.AddNumericInt("subtitle.font", "Size", base.subtitle_font.GetHeight(), 7, 36, 1, "Subtitle Typography").SetUnit("px"));
    MarkOverride(override_model_.AddBoolean("subtitle.bold", "Bold", base.subtitle_font.IsBold(), "Subtitle Typography"));
    MarkOverride(override_model_.AddBoolean("subtitle.italic", "Italic", base.subtitle_font.IsItalic(), "Subtitle Typography"));
    MarkOverride(AddPropertyFont(override_model_, "right.face", "Face", UiFonts::Selection(base.right_font), "Segment percentage Typography"));
    MarkOverride(override_model_.AddNumericInt("right.font", "Size", base.right_font.GetHeight(), 7, 36, 1, "Segment percentage Typography").SetUnit("px"));
    MarkOverride(override_model_.AddBoolean("right.bold", "Bold", base.right_font.IsBold(), "Segment percentage Typography"));
    MarkOverride(override_model_.AddBoolean("right.italic", "Italic", base.right_font.IsItalic(), "Segment percentage Typography"));
    MarkOverride(AddPropertyFont(override_model_, "value.face", "Face", UiFonts::Selection(base.value_font), "Boundary readout Typography"));
    MarkOverride(override_model_.AddNumericInt("value.font", "Size", base.value_font.GetHeight(), 7, 36, 1, "Boundary readout Typography").SetUnit("px"));
    MarkOverride(override_model_.AddBoolean("value.bold", "Bold", base.value_font.IsBold(), "Boundary readout Typography"));
    MarkOverride(override_model_.AddBoolean("value.italic", "Italic", base.value_font.IsItalic(), "Boundary readout Typography"));
    override_model_.SetGroupSubtitle("Text Layout", "Automatic keeps title/subtitle opposite the percentage; Null ink follows contrast");
    override_model_.SetGroupSubtitle("Boundary Thumb", "Visual dimensions are independent of the minimum accessible drag target");
    override_model_.StructureChanged();
}

void UiRangeSegmentsDemo::BuildDataModel()
{
    data_model_.Clear(false);
    data_segment_count_ = ranges_.GetSegmentCount();
    UiRangeSegments::Geometry g = ranges_.GetGeometry(Size(640, 220));
    double domain = max(1.0, ranges_.GetMax() - ranges_.GetMin());

    for(int i = 0; i < data_segment_count_; i++) {
        String group = Format("Segment %d", i + 1);
        const UiRangeSegment& segment = ranges_.GetSegment(i);
        Color resolved = i < g.segments.GetCount() ? g.segments[i].color : Color(100, 116, 139);
        data_model_.AddText(Format("segment.%d.label", i), "Title", segment.label, group);
        data_model_.AddText(Format("segment.%d.subtitle", i), "Subtitle", segment.subtitle, group);
        data_model_.AddNumericDouble(Format("segment.%d.span", i), "Span / weight", segment.span,
                                     0.0, domain, ranges_.GetStep(), group);
        data_model_.AddBoolean(Format("segment.%d.explicit", i), "Explicit colour", !IsNull(segment.color), group);
        data_model_.AddColor(Format("segment.%d.color", i), "Colour",
                             IsNull(segment.color) ? resolved : segment.color, group);
    }
    data_model_.SetGroupSubtitle("Segment 1", "spans are normalized to the fixed domain; labels and optional colours remain authored data");
    data_model_.StructureChanged();
}

void UiRangeSegmentsDemo::ConfigureEditors()
{
    inspector_.SetFactory(&factory_);
    overrides_.SetFactory(&factory_);
    data_.SetFactory(&factory_);
    inspector_.SetModel(&inspector_model_);
    overrides_.SetModel(&override_model_);
    data_.SetModel(&data_model_);
    inspector_.SetLabelRatio(46);
    overrides_.SetLabelRatio(46);
    data_.SetLabelRatio(46);

    PropertyEditorStyle style = PropertyEditorStyle::System();
    style.show_group_summaries = true;
    inspector_.SetStyle(style);
    overrides_.SetStyle(style);
    data_.SetStyle(style);
}

void UiRangeSegmentsDemo::ConnectEvents()
{
    auto changed = [=](String id, Value) {
        if(id == "direction") {
            const bool vertical = AsString(InspectorValue("direction")) == "Vertical";
            inspector_model_.SetValue("width", vertical ? 240 : 720, false);
            inspector_model_.SetValue("height", vertical ? 440 : (cards_preview_ ? 160 : 220), false);
            inspector_model_.ValueChanged("width");
            inspector_model_.ValueChanged("height");
        }
        ApplyProjection();
    };
    inspector_.WhenPreview = changed;
    inspector_.WhenCommit = changed;
    overrides_.WhenPreview = changed;
    overrides_.WhenCommit = changed;
    data_.WhenPreview = changed;
    data_.WhenCommit = changed;

    inspector_.WhenReset = [=](String id) { ResetProperty(inspector_model_, id); };
    overrides_.WhenReset = [=](String id) { ResetProperty(override_model_, id); };
    data_.WhenReset = [=](String id) { ResetProperty(data_model_, id); };
    overrides_.WhenOverride = [=](String id, bool active) { SetOverrideActive(id, active); };

    range_mode_.WhenAction = [=] { SelectPreview(false); };
    cards_mode_.WhenAction = [=] { SelectPreview(true); };
    inspector_mode_.WhenAction = [=] { SelectPage(0); };
    overrides_mode_.WhenAction = [=] { SelectPage(1); };
    data_mode_.WhenAction = [=] { SelectPage(2); };
    code_mode_.WhenAction = [=] { SelectPage(3); };
    theme_.WhenAction = [=] { ToggleTheme(); };
    help_.WhenAction = [=] {
        PromptOK("UiRangeSegments reference demo\n\nThe control owns one fixed scalar domain and N contiguous labelled spans. Dragging an internal boundary handle changes only the two neighbouring spans. Data exposes titles, subtitles, weights and optional explicit colours; the automatic palette can be discrete or sampled as a gradient.");
    };
    exit_.WhenAction = [=] { Break(); };
    copy_.WhenAction = [=] { WriteClipboardText(generated_); };

    ranges_.WhenChanging = [=] {
        if(syncing_projection_)
            return;
        SyncDataSpans();
        UpdateGeneratedCode();
    };
    ranges_.WhenAction = [=] {
        SyncDataSpans();
        UpdateGeneratedCode();
    };
}

Value UiRangeSegmentsDemo::InspectorValue(const String& id, const Value& fallback) const
{
    const PropertyEditorItem *item = inspector_model_.Find(id);
    return item ? item->value : fallback;
}

Value UiRangeSegmentsDemo::OverrideValue(const String& id, const Value& fallback) const
{
    const PropertyEditorItem *item = override_model_.Find(id);
    return item ? item->value : fallback;
}

Value UiRangeSegmentsDemo::DataValue(const String& id, const Value& fallback) const
{
    const PropertyEditorItem *item = data_model_.Find(id);
    return item ? item->value : fallback;
}

bool UiRangeSegmentsDemo::OverrideActive(const String& id) const
{
    const PropertyEditorItem *item = override_model_.Find(id);
    return item && item->override_active;
}

void UiRangeSegmentsDemo::ResetProperty(PropertyEditorModel& model, const String& id)
{
    PropertyEditorItem *item = model.Find(id);
    if(!item || !item->resettable)
        return;
    model.SetValue(id, item->default_value);
    ApplyProjection();
}

void UiRangeSegmentsDemo::SetOverrideActive(const String& id, bool active)
{
    PropertyEditorItem *item = override_model_.Find(id);
    if(!item || !item->overrideable)
        return;
    item->override_active = active;
    override_model_.ValueChanged(id);
    ApplyProjection();
}

void UiRangeSegmentsDemo::ApplyDataProjection()
{
    if(data_segment_count_ != ranges_.GetSegmentCount())
        return;

    Vector<UiRangeSegment> segments;
    for(int i = 0; i < data_segment_count_; i++) {
        String p = Format("segment.%d.", i);
        String label = AsString(DataValue(p + "label", Format("Segment %d", i + 1)));
        double span = (double)DataValue(p + "span", 1.0);
        bool explicit_color = (bool)DataValue(p + "explicit", false);
        Color color = explicit_color ? Color(DataValue(p + "color", Color(100,116,139))) : Null;
        Value payload = i < ranges_.GetSegmentCount() ? ranges_.GetSegment(i).data : Value();
        UiRangeSegment& segment = segments.Add(UiRangeSegment(span, label, color, payload));
        segment.subtitle = AsString(DataValue(p + "subtitle"));
    }
    ranges_.SetSegments(segments);
    SyncDataSpans();
}

void UiRangeSegmentsDemo::SyncDataSpans()
{
    if(data_segment_count_ != ranges_.GetSegmentCount())
        return;
    for(int i = 0; i < data_segment_count_; i++) {
        String id = Format("segment.%d.span", i);
        const Value value = ranges_.GetSegmentSpan(i);
        if(data_model_.Find(id)->value != value) {
            data_model_.SetValue(id, value, false);
            data_model_.ValueChanged(id);
        }
    }
}

void UiRangeSegmentsDemo::SyncInheritedOverrides(const UiRangeSegments::Style& style)
{
    auto SetIfInherited = [&](const String& id, const Value& value) {
        PropertyEditorItem* item = override_model_.Find(id);
        if(!item)
            return;
        item->SetDefault(value);
        if(!item->override_active && item->value != value) {
            override_model_.SetValue(id, value, false);
            override_model_.ValueChanged(id);
        }
    };
    SetIfInherited("track.face", FaceColor(style.track_palette, ST_NORMAL, SColorFace()));
    SetIfInherited("thumb.face", FaceColor(style.thumb_palette, ST_NORMAL, SColorPaper()));
    SetIfInherited("track.height", style.track_size.cy);
    SetIfInherited("track.radius", style.track_metrics.radius);
    SetIfInherited("track.frame_width", style.track_metrics.frame_width);
    SetIfInherited("thumb.width", style.thumb_size.cx);
    SetIfInherited("thumb.height", style.thumb_size.cy);
    SetIfInherited("thumb.radius", style.thumb_metrics.radius);
    SetIfInherited("thumb.frame_width", style.thumb_metrics.frame_width);
    SetIfInherited("thumb.dot", style.thumb_dot_diameter);
    SetIfInherited("thumb.hover_growth", style.thumb_hover_growth);
    SetIfInherited("selected.width", style.selected_frame_width);
    SetIfInherited("label.padding", style.label_padding);
    SetIfInherited("label.gap", style.label_gap);
    SetIfInherited("value.gap", style.value_gap);
    SetIfInherited("track.face_enabled", style.track_metrics.face_enabled);
    SetIfInherited("track.frame_enabled", style.track_metrics.frame_enabled);
    SetIfInherited("thumb.face_enabled", style.thumb_metrics.face_enabled);
    SetIfInherited("thumb.frame_enabled", style.thumb_metrics.frame_enabled);
    SetIfInherited("thumb.rotate", style.thumb_rotate_with_direction);
    SetIfInherited("track.frame", style.track_palette.frame[ST_NORMAL]);
    SetIfInherited("thumb.frame", style.thumb_palette.frame[ST_NORMAL]);
    SetIfInherited("divider", style.divider_color);
    SetIfInherited("selected", style.selected_frame);
    SetIfInherited("label.color", style.label_color);
    SetIfInherited("subtitle.color", style.subtitle_color);
    SetIfInherited("value.color", style.value_color);
    SetIfInherited("label.face", UiFonts::Selection(style.label_font));
    SetIfInherited("label.font", style.label_font.GetHeight());
    SetIfInherited("label.bold", style.label_font.IsBold());
    SetIfInherited("label.italic", style.label_font.IsItalic());
    SetIfInherited("subtitle.face", UiFonts::Selection(style.subtitle_font));
    SetIfInherited("subtitle.font", style.subtitle_font.GetHeight());
    SetIfInherited("subtitle.bold", style.subtitle_font.IsBold());
    SetIfInherited("subtitle.italic", style.subtitle_font.IsItalic());
    SetIfInherited("right.face", UiFonts::Selection(style.right_font));
    SetIfInherited("right.font", style.right_font.GetHeight());
    SetIfInherited("right.bold", style.right_font.IsBold());
    SetIfInherited("right.italic", style.right_font.IsItalic());
    SetIfInherited("value.face", UiFonts::Selection(style.value_font));
    SetIfInherited("value.font", style.value_font.GetHeight());
    SetIfInherited("value.bold", style.value_font.IsBold());
    SetIfInherited("value.italic", style.value_font.IsItalic());
    SetIfInherited("thumb.shape", style.thumb_shape == UiRangeSegments::ThumbShape::Ellipse ? "Ellipse" : "Rounded rectangle");
    SetIfInherited("label.align", style.label_align == UiAlign::LEFT ? "Left" : style.label_align == UiAlign::RIGHT ? "Right" : style.label_align == UiAlign::CENTER ? "Centre" : "Automatic");
    for(int i = 0; i < 6; i++)
        SetIfInherited("series." + AsString(i), style.series[i]);
}

bool UiRangeSegmentsDemo::ApplyOverrides(UiRangeSegments::Style& style) const
{
    bool any = false;
    for(const PropertyEditorItem& item : override_model_.GetItems())
        any = any || item.override_active;
    ApplyFrameAccentProperties(style.track_metrics, override_model_, "track_metrics.frame_accent.");
    if(OverrideActive("track.face")) style.track_palette.face[ST_NORMAL] = UiFill::Solid(Color(OverrideValue("track.face")));
    if(OverrideActive("thumb.face")) style.thumb_palette.face[ST_NORMAL] = UiFill::Solid(Color(OverrideValue("thumb.face")));
    if(OverrideActive("track.height")) style.track_size.cy = clamp((int)OverrideValue("track.height"), 1, 300);
    if(OverrideActive("track.radius")) style.track_metrics.radius = clamp((int)OverrideValue("track.radius"), 0, 60);
    if(OverrideActive("track.frame_width")) style.track_metrics.frame_width = clamp((int)OverrideValue("track.frame_width"), 0, 8);
    if(OverrideActive("thumb.width")) style.thumb_size.cx = clamp((int)OverrideValue("thumb.width"), 1, 60);
    if(OverrideActive("thumb.height")) style.thumb_size.cy = clamp((int)OverrideValue("thumb.height"), 1, 100);
    if(OverrideActive("thumb.radius")) style.thumb_metrics.radius = clamp((int)OverrideValue("thumb.radius"), 0, 60);
    if(OverrideActive("thumb.frame_width")) style.thumb_metrics.frame_width = clamp((int)OverrideValue("thumb.frame_width"), 0, 8);
    if(OverrideActive("thumb.dot")) style.thumb_dot_diameter = clamp((int)OverrideValue("thumb.dot"), 0, 16);
    if(OverrideActive("thumb.hover_growth")) style.thumb_hover_growth = clamp((int)OverrideValue("thumb.hover_growth"), 0, 12);
    if(OverrideActive("selected.width")) style.selected_frame_width = clamp((int)OverrideValue("selected.width"), 1, 8);
    if(OverrideActive("label.padding")) style.label_padding = clamp((int)OverrideValue("label.padding"), 0, 40);
    if(OverrideActive("label.gap")) style.label_gap = clamp((int)OverrideValue("label.gap"), 0, 24);
    if(OverrideActive("value.gap")) style.value_gap = clamp((int)OverrideValue("value.gap"), 0, 40);
    if(OverrideActive("track.face_enabled")) style.track_metrics.face_enabled = (bool)OverrideValue("track.face_enabled");
    if(OverrideActive("track.frame_enabled")) style.track_metrics.frame_enabled = (bool)OverrideValue("track.frame_enabled");
    if(OverrideActive("thumb.face_enabled")) style.thumb_metrics.face_enabled = (bool)OverrideValue("thumb.face_enabled");
    if(OverrideActive("thumb.frame_enabled")) style.thumb_metrics.frame_enabled = (bool)OverrideValue("thumb.frame_enabled");
    if(OverrideActive("thumb.rotate")) style.thumb_rotate_with_direction = (bool)OverrideValue("thumb.rotate");
    if(OverrideActive("track.frame")) style.track_palette.frame[ST_NORMAL] = Color(OverrideValue("track.frame"));
    if(OverrideActive("thumb.frame")) style.thumb_palette.frame[ST_NORMAL] = Color(OverrideValue("thumb.frame"));
    if(OverrideActive("divider")) style.divider_color = Color(OverrideValue("divider"));
    if(OverrideActive("selected")) style.selected_frame = Color(OverrideValue("selected"));
    if(OverrideActive("label.color")) style.label_color = Color(OverrideValue("label.color"));
    if(OverrideActive("subtitle.color")) style.subtitle_color = Color(OverrideValue("subtitle.color"));
    if(OverrideActive("value.color")) style.value_color = Color(OverrideValue("value.color"));
    if(OverrideActive("label.face")) UiFonts::ApplySelection(style.label_font, AsString(OverrideValue("label.face")));
    if(OverrideActive("label.font")) style.label_font.Height(clamp((int)OverrideValue("label.font"), 7, 36));
    if(OverrideActive("label.bold")) style.label_font.Bold((bool)OverrideValue("label.bold"));
    if(OverrideActive("label.italic")) style.label_font.Italic((bool)OverrideValue("label.italic"));
    if(OverrideActive("subtitle.face")) UiFonts::ApplySelection(style.subtitle_font, AsString(OverrideValue("subtitle.face")));
    if(OverrideActive("subtitle.font")) style.subtitle_font.Height(clamp((int)OverrideValue("subtitle.font"), 7, 36));
    if(OverrideActive("subtitle.bold")) style.subtitle_font.Bold((bool)OverrideValue("subtitle.bold"));
    if(OverrideActive("subtitle.italic")) style.subtitle_font.Italic((bool)OverrideValue("subtitle.italic"));
    if(OverrideActive("right.face")) UiFonts::ApplySelection(style.right_font, AsString(OverrideValue("right.face")));
    if(OverrideActive("right.font")) style.right_font.Height(clamp((int)OverrideValue("right.font"), 7, 36));
    if(OverrideActive("right.bold")) style.right_font.Bold((bool)OverrideValue("right.bold"));
    if(OverrideActive("right.italic")) style.right_font.Italic((bool)OverrideValue("right.italic"));
    if(OverrideActive("value.face")) UiFonts::ApplySelection(style.value_font, AsString(OverrideValue("value.face")));
    if(OverrideActive("value.font")) style.value_font.Height(clamp((int)OverrideValue("value.font"), 7, 36));
    if(OverrideActive("value.bold")) style.value_font.Bold((bool)OverrideValue("value.bold"));
    if(OverrideActive("value.italic")) style.value_font.Italic((bool)OverrideValue("value.italic"));
    if(OverrideActive("thumb.shape")) style.thumb_shape = AsString(OverrideValue("thumb.shape")) == "Ellipse" ? UiRangeSegments::ThumbShape::Ellipse : UiRangeSegments::ThumbShape::RoundedRectangle;
    if(OverrideActive("label.align")) {
        String align = AsString(OverrideValue("label.align"));
        style.label_align = align == "Left" ? UiAlign::LEFT : align == "Right" ? UiAlign::RIGHT : align == "Centre" ? UiAlign::CENTER : UiAlign::DEFAULT;
    }
    for(int i = 0; i < 6; i++) {
        String id = "series." + AsString(i);
        if(OverrideActive(id)) {
            style.series[i] = Color(OverrideValue(id));
            style.series_count = max(style.series_count, i + 1);
        }
    }
    return any;
}

void UiRangeSegmentsDemo::ApplyProjection()
{
    if(syncing_projection_)
        return;
    syncing_projection_ = true;

    double mn = (double)InspectorValue("minimum", 0.0);
    double mx = (double)InspectorValue("maximum", 100.0);
    if(mx < mn)
        Swap(mx, mn);
    int count = clamp((int)InspectorValue("segment_count", 4), 1, 8);

    ranges_.SetRole(ParseRole(AsString(InspectorValue("role", "Standard"))));
    ranges_.SetRange(mn, mx)
           .SetStep((double)InspectorValue("step", 1.0))
           .SetMinimumSegmentSpan((double)InspectorValue("min_span", 0.0))
           .SetDirection(ParseDirection(AsString(InspectorValue("direction", "Horizontal"))))
           .SetReverse((bool)InspectorValue("reverse", false))
           .SetPaletteMode(ParsePaletteMode(AsString(InspectorValue("palette_mode", "Series"))))
           .SetValueDisplay(ParseValueDisplay(AsString(InspectorValue("value_display", "Percent"))))
           .SetValuePrecision((int)InspectorValue("precision", 0))
           .ShowLabels((bool)InspectorValue("show_labels", true))
           .ShowSegmentValues((bool)InspectorValue("show_segment_values", true))
           .ShowBoundaryValues((bool)InspectorValue("show_boundary_values", true))
           .ShowEndpointValues((bool)InspectorValue("show_endpoint_values", true))
           .ShowValuesOnInteraction((bool)InspectorValue("show_values_on_interaction", true))
           .ShowDividers((bool)InspectorValue("show_dividers", true));

    if(count != ranges_.GetSegmentCount()) {
        ranges_.SetSegmentCount(count);
        BuildDataModel();
        data_.SetModel(&data_model_);
    }
    else
        ApplyDataProjection();

    UiRangeSegments probe;
    probe.SetRole(ranges_.GetRole());
    UiRangeSegments::Style base = probe.GetStyle();
    if(cards_preview_)
        ApplyCardPreset(base);
    base.value_side = AsString(InspectorValue("value_side", "Right")) == "Left" ? UiAlign::LEFT : UiAlign::RIGHT;
    SyncInheritedOverrides(base);
    const bool authored = ApplyOverrides(base);
    if(cards_preview_ || authored || base.value_side != probe.GetStyle().value_side)
        ranges_.SetCustomStyle(base);
    else
        ranges_.ClearCustomStyle();

    range_mode_.SetChecked(!cards_preview_);
    cards_mode_.SetChecked(cards_preview_);
    ranges_.Enable((bool)InspectorValue("enabled", true));
    SyncDataSpans();
    UpdateGeneratedCode();
    Layout();
    ranges_.Refresh();
    syncing_projection_ = false;
}

void UiRangeSegmentsDemo::SelectPage(int page)
{
    pages_.SetActivePage(page);
    inspector_mode_.SetChecked(page == 0);
    overrides_mode_.SetChecked(page == 1);
    data_mode_.SetChecked(page == 2);
    code_mode_.SetChecked(page == 3);
}

void UiRangeSegmentsDemo::ToggleTheme()
{
    UiThemeContext context = UiTheme::GetContext();
    context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
    UiTheme::Set(context);
    Ctrl::SwapDarkLight();
    ApplyTheme();
    ApplyProjection();
}

void UiRangeSegmentsDemo::ApplyTheme()
    {
        const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
        theme_.SetIcon(dark ? ICON_ACTION_LIGHT_MODE_48() : ICON_ACTION_DARK_MODE_48());
        window_face_ = UiTheme::ResolvePanel(UiPanelRole::Surface).palette.face[ST_NORMAL].color;
        header_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
        UiPanel::Style surface = UiTheme::ResolvePanel(UiPanelRole::Surface);
        const Color panel_face = dark ? Color(18, 18, 18) : Color(245, 245, 245);
        surface.transparent = false;
        surface.metrics.face_enabled = true;
        surface.metrics.frame_enabled = true;
        surface.metrics.frame_width = DPI(1);
        surface.metrics.radius = DPI(8);
        surface.metrics.shadow.enabled = false;
        surface.metrics.focus_enabled = false;
        for(int state = 0; state < 4; state++) {
            surface.palette.face[state] = UiFill::Solid(panel_face);
            surface.palette.frame[state] = dark ? Color(48, 48, 48) : Color(220, 220, 220);
        }
        preview_.SetCustomStyle(surface);
        right_.SetCustomStyle(surface);
        UiPanel::Style page_style = surface;
        page_style.transparent = true;
        page_style.metrics.face_enabled = page_style.metrics.frame_enabled = false;
        for(UiPanel* panel : { &inspector_page_, &overrides_page_, &code_page_, &data_page_ })
            panel->SetCustomStyle(page_style);
        for(UiLabel* label : { &caption_ })
            label->SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        const auto mode = dark
                        ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
        inspector_.SetPaletteMode(mode);
        overrides_.SetPaletteMode(mode);
        data_.SetPaletteMode(mode);
        for(PropertyEditor* editor : { &inspector_, &overrides_, &data_ }) {
            PropertyEditorStyle editor_style = editor->GetStyle();
            editor_style.show_frame = false;
            editor_style.background = panel_face;
            editor_style.show_group_summaries = true;
            editor->SetStyle(editor_style);
        }
        for(UiToolButton* button : { &theme_, &help_, &exit_, &inspector_mode_, &overrides_mode_, &code_mode_, &data_mode_, &copy_ }) {
            UiToolButton::Style style = UiTheme::ResolveToolButton(UiRole::Standard);
            style.transparent = true;
            style.metrics.face_enabled = style.metrics.frame_enabled = false;
            style.metrics.focus_enabled = false;
            style.metrics.shadow.enabled = false;
            style.underline = false;
            for(int state = 0; state < 4; state++) {
                style.palette.face[state] = UiFill::None();
                style.palette.frame[state] = Null;
            }
            const Color neutral = dark ? Color(180, 180, 180) : Color(110, 110, 110);
            style.palette.icon[ST_NORMAL] = neutral;
            style.palette.icon[ST_HOT] = dark ? White() : Color(32, 32, 32);
            style.palette.icon[ST_PRESSED] = Color(0, 120, 212);
            style.palette.icon[ST_DISABLED] = Blend(neutral, panel_face, 150);
            button->SetCustomStyle(style);
        }
        UiToolButton::Style exit_style = exit_.GetStyle();
        exit_style.palette.icon[ST_NORMAL] = Color(200, 60, 60);
        exit_style.palette.icon[ST_HOT] = Color(240, 85, 85);
        exit_style.palette.icon[ST_PRESSED] = Color(180, 45, 45);
        exit_.SetCustomStyle(exit_style);
        Refresh();
    }

void UiRangeSegmentsDemo::UpdateGeneratedCode()
{
    String out;
    out << "UiRangeSegments ranges;\n";
    out << "ranges.SetRange(" << CppScalar(ranges_.GetMin()) << ", "
        << CppScalar(ranges_.GetMax()) << ")\n";
    out << "      .SetStep(" << CppScalar(ranges_.GetStep()) << ")\n";
    out << "      .SetMinimumSegmentSpan(" << CppScalar(ranges_.GetMinimumSegmentSpan()) << ")\n";
    out << "      .SetDirection(" << (ranges_.GetDirection() == UiDirection::V ? "UiDirection::V" : "UiDirection::H") << ")\n";
    out << "      .SetReverse(" << (ranges_.IsReversed() ? "true" : "false") << ")\n";
    out << "      .SetPaletteMode(UiRangeSegments::PaletteMode::"
        << (ranges_.GetPaletteMode() == UiRangeSegments::PaletteMode::Gradient ? "Gradient" : "Series") << ")\n";
    out << "      .SetValueDisplay(UiRangeSegments::ValueDisplay::"
        << (ranges_.GetValueDisplay() == UiRangeSegments::ValueDisplay::Percent ? "Percent" : "Domain") << ")\n";
    out << "      .SetValuePrecision(" << ranges_.GetValuePrecision() << ")\n";
    out << "      .ShowLabels(" << (ranges_.AreLabelsShown() ? "true" : "false") << ")\n";
    out << "      .ShowSegmentValues(" << (ranges_.AreSegmentValuesShown() ? "true" : "false") << ")\n";
    out << "      .ShowBoundaryValues(" << (ranges_.AreBoundaryValuesShown() ? "true" : "false") << ")\n";
    out << "      .ShowEndpointValues(" << (ranges_.AreEndpointValuesShown() ? "true" : "false") << ")\n";
    out << "      .ShowValuesOnInteraction(" << (ranges_.AreValuesShownOnInteraction() ? "true" : "false") << ")\n";
    out << "      .ShowDividers(" << (ranges_.AreDividersShown() ? "true" : "false") << ");\n\n";

    out << "Vector<UiRangeSegment> segments;\n";
    for(const UiRangeSegment& segment : ranges_.GetSegments()) {
        out << "segments.Add(UiRangeSegment(" << CppScalar(segment.span)
            << ", " << CppString(segment.label) << ", " << CppColor(segment.color) << "));\n";
        if(!segment.subtitle.IsEmpty())
            out << "segments.Top().subtitle = " << CppString(segment.subtitle) << ";\n";
    }
    out << "ranges.SetSegments(segments);\n";

    if(ranges_.GetRole() != UiRole::Standard) {
        String role = ranges_.GetRole() == UiRole::Subtle ? "Subtle"
                    : ranges_.GetRole() == UiRole::Accent ? "Accent" : "Alert";
        out << "ranges.SetRole(UiRole::" << role << ");\n";
    }

    if(ranges_.HasCustomStyle()) {
        out << "\nUiRangeSegments::Style style = ranges.GetStyle();\n";
        if(cards_preview_)
            EmitCardPreset(out);
        out << "style.value_side = UiAlign::" << (ranges_.GetStyle().value_side == UiAlign::LEFT ? "LEFT" : "RIGHT") << ";\n";
        { bool authored = true; EmitFrameAccentProperties(out, override_model_, "track_metrics.frame_accent.", "style.track_metrics", String(), authored); }
        if(OverrideActive("track.face")) out << "style.track_palette.face[ST_NORMAL] = UiFill::Solid(" << CppColor(Color(OverrideValue("track.face"))) << ");\n";
        if(OverrideActive("thumb.face")) out << "style.thumb_palette.face[ST_NORMAL] = UiFill::Solid(" << CppColor(Color(OverrideValue("thumb.face"))) << ");\n";
        if(OverrideActive("track.height")) out << "style.track_size.cy = " << clamp((int)OverrideValue("track.height"), 1, 300) << ";\n";
        if(OverrideActive("track.radius")) out << "style.track_metrics.radius = " << clamp((int)OverrideValue("track.radius"), 0, 60) << ";\n";
        if(OverrideActive("track.frame_width")) out << "style.track_metrics.frame_width = " << clamp((int)OverrideValue("track.frame_width"), 0, 8) << ";\n";
        if(OverrideActive("thumb.width")) out << "style.thumb_size.cx = " << clamp((int)OverrideValue("thumb.width"), 1, 60) << ";\n";
        if(OverrideActive("thumb.height")) out << "style.thumb_size.cy = " << clamp((int)OverrideValue("thumb.height"), 1, 100) << ";\n";
        if(OverrideActive("thumb.radius")) out << "style.thumb_metrics.radius = " << clamp((int)OverrideValue("thumb.radius"), 0, 60) << ";\n";
        if(OverrideActive("thumb.frame_width")) out << "style.thumb_metrics.frame_width = " << clamp((int)OverrideValue("thumb.frame_width"), 0, 8) << ";\n";
        if(OverrideActive("thumb.dot")) out << "style.thumb_dot_diameter = " << clamp((int)OverrideValue("thumb.dot"), 0, 16) << ";\n";
        if(OverrideActive("thumb.hover_growth")) out << "style.thumb_hover_growth = " << clamp((int)OverrideValue("thumb.hover_growth"), 0, 12) << ";\n";
        if(OverrideActive("selected.width")) out << "style.selected_frame_width = " << clamp((int)OverrideValue("selected.width"), 1, 8) << ";\n";
        if(OverrideActive("label.padding")) out << "style.label_padding = " << clamp((int)OverrideValue("label.padding"), 0, 40) << ";\n";
        if(OverrideActive("label.gap")) out << "style.label_gap = " << clamp((int)OverrideValue("label.gap"), 0, 24) << ";\n";
        if(OverrideActive("value.gap")) out << "style.value_gap = " << clamp((int)OverrideValue("value.gap"), 0, 40) << ";\n";
        if(OverrideActive("track.face_enabled")) out << "style.track_metrics.face_enabled = " << ((bool)OverrideValue("track.face_enabled") ? "true" : "false") << ";\n";
        if(OverrideActive("track.frame_enabled")) out << "style.track_metrics.frame_enabled = " << ((bool)OverrideValue("track.frame_enabled") ? "true" : "false") << ";\n";
        if(OverrideActive("thumb.face_enabled")) out << "style.thumb_metrics.face_enabled = " << ((bool)OverrideValue("thumb.face_enabled") ? "true" : "false") << ";\n";
        if(OverrideActive("thumb.frame_enabled")) out << "style.thumb_metrics.frame_enabled = " << ((bool)OverrideValue("thumb.frame_enabled") ? "true" : "false") << ";\n";
        if(OverrideActive("thumb.rotate")) out << "style.thumb_rotate_with_direction = " << ((bool)OverrideValue("thumb.rotate") ? "true" : "false") << ";\n";
        if(OverrideActive("track.frame")) out << "style.track_palette.frame[ST_NORMAL] = " << CppColor(Color(OverrideValue("track.frame"))) << ";\n";
        if(OverrideActive("thumb.frame")) out << "style.thumb_palette.frame[ST_NORMAL] = " << CppColor(Color(OverrideValue("thumb.frame"))) << ";\n";
        if(OverrideActive("divider")) out << "style.divider_color = " << CppColor(Color(OverrideValue("divider"))) << ";\n";
        if(OverrideActive("selected")) out << "style.selected_frame = " << CppColor(Color(OverrideValue("selected"))) << ";\n";
        if(OverrideActive("label.color")) out << "style.label_color = " << CppColor(Color(OverrideValue("label.color"))) << ";\n";
        if(OverrideActive("subtitle.color")) out << "style.subtitle_color = " << CppColor(Color(OverrideValue("subtitle.color"))) << ";\n";
        if(OverrideActive("value.color")) out << "style.value_color = " << CppColor(Color(OverrideValue("value.color"))) << ";\n";
        if(OverrideActive("label.face")) out << "UiFonts::ApplySelection(style.label_font, " << CppString(AsString(OverrideValue("label.face"))) << ");\n";
        if(OverrideActive("label.font")) out << "style.label_font.Height(" << clamp((int)OverrideValue("label.font"), 7, 36) << ");\n";
        if(OverrideActive("label.bold")) out << "style.label_font.Bold(" << ((bool)OverrideValue("label.bold" ) ? "true" : "false") << ");\n";
        if(OverrideActive("label.italic")) out << "style.label_font.Italic(" << ((bool)OverrideValue("label.italic" ) ? "true" : "false") << ");\n";
        if(OverrideActive("subtitle.face")) out << "UiFonts::ApplySelection(style.subtitle_font, " << CppString(AsString(OverrideValue("subtitle.face"))) << ");\n";
        if(OverrideActive("subtitle.font")) out << "style.subtitle_font.Height(" << clamp((int)OverrideValue("subtitle.font"), 7, 36) << ");\n";
        if(OverrideActive("subtitle.bold")) out << "style.subtitle_font.Bold(" << ((bool)OverrideValue("subtitle.bold" ) ? "true" : "false") << ");\n";
        if(OverrideActive("subtitle.italic")) out << "style.subtitle_font.Italic(" << ((bool)OverrideValue("subtitle.italic" ) ? "true" : "false") << ");\n";
        if(OverrideActive("right.face")) out << "UiFonts::ApplySelection(style.right_font, " << CppString(AsString(OverrideValue("right.face"))) << ");\n";
        if(OverrideActive("right.font")) out << "style.right_font.Height(" << clamp((int)OverrideValue("right.font"), 7, 36) << ");\n";
        if(OverrideActive("right.bold")) out << "style.right_font.Bold(" << ((bool)OverrideValue("right.bold" ) ? "true" : "false") << ");\n";
        if(OverrideActive("right.italic")) out << "style.right_font.Italic(" << ((bool)OverrideValue("right.italic" ) ? "true" : "false") << ");\n";
        if(OverrideActive("value.face")) out << "UiFonts::ApplySelection(style.value_font, " << CppString(AsString(OverrideValue("value.face"))) << ");\n";
        if(OverrideActive("value.font")) out << "style.value_font.Height(" << clamp((int)OverrideValue("value.font"), 7, 36) << ");\n";
        if(OverrideActive("value.bold")) out << "style.value_font.Bold(" << ((bool)OverrideValue("value.bold" ) ? "true" : "false") << ");\n";
        if(OverrideActive("value.italic")) out << "style.value_font.Italic(" << ((bool)OverrideValue("value.italic" ) ? "true" : "false") << ");\n";
        if(OverrideActive("thumb.shape")) out << "style.thumb_shape = UiRangeSegments::ThumbShape::" << (ranges_.GetStyle().thumb_shape == UiRangeSegments::ThumbShape::Ellipse ? "Ellipse" : "RoundedRectangle") << ";\n";
        if(OverrideActive("label.align")) {
            const UiAlign align = ranges_.GetStyle().label_align;
            out << "style.label_align = UiAlign::" << (align == UiAlign::LEFT ? "LEFT" : align == UiAlign::RIGHT ? "RIGHT" : align == UiAlign::CENTER ? "CENTER" : "DEFAULT") << ";\n";
        }
        for(int i = 0; i < 6; i++) {
            String id = "series." + AsString(i);
            if(OverrideActive(id)) {
                out << "style.series[" << i << "] = " << CppColor(Color(OverrideValue(id))) << ";\n";
                out << "style.series_count = max(style.series_count, " << i + 1 << ");\n";
            }
        }
        out << "ranges.SetCustomStyle(style);\n";
    }

    String body = out;
    out = "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\nclass RangeSegmentsExample : public ParentCtrl {\npublic:\n    UiRangeSegments ranges;\n    RangeSegmentsExample()\n    {\n        Add(ranges.SizePos());\n";
    for(const String& line : Split(body, '\n', false)) {
        if(line == "UiRangeSegments ranges;")
            continue;
        if(!line.IsEmpty())
            out << "        " << line;
        out << "\n";
    }
    out << "    }\n};\n";
    generated_ = out;
    code_.SetData(generated_);
}

} // namespace Upp

namespace Upp {
void UiRangeSegmentsDemo::Paint(Draw& draw) { draw.DrawRect(GetSize(), window_face_); }
}

namespace Upp {
void UiRangeSegmentsDemo::ExportGenerated(const String& directory)
{
    RealizeDirectory(directory);
    UpdateGeneratedCode();
    SaveFile(AppendFileName(directory, "UiRangeSegmentsDemo_default.cpp"), generated_);
    for(bool cards : { false, true }) {
        for(const char* direction : { "Horizontal", "Vertical" }) {
            for(const char* side : { "Left", "Right" }) {
                inspector_model_.SetValue("direction", direction, false);
                inspector_model_.SetValue("value_side", side, false);
                SelectPreview(cards);
                String file = Format("UiRangeSegmentsDemo_%s_%s_%s.cpp", cards ? "Cards" : "Range", direction, side);
                SaveFile(AppendFileName(directory, file), generated_);
            }
        }
    }
    // One real authored fixture exercises data escaping and each new style family.
    inspector_model_.SetValue("direction", "Horizontal", false);
    inspector_model_.SetValue("value_side", "Left", false);
    SelectPreview(true);
    data_model_.SetValue("segment.0.label", "Act \"one\"", false);
    data_model_.SetValue("segment.0.subtitle", "A subtitle\nwith a second line", false);
    auto author = [&](const char* id, const Value& value) {
        PropertyEditorItem* item = override_model_.Find(id);
        item->override_active = true;
        override_model_.SetValue(id, value, false);
    };
    author("thumb.width", 7); author("thumb.height", 32);
    author("thumb.radius", 2); author("thumb.frame_width", 2);
    author("thumb.face_enabled", false); author("thumb.dot", 0);
    author("thumb.hover_growth", 2);
    author("label.gap", 5); author("value.gap", 16);
    author("label.align", "Right");
    author("label.color", Color(30, 50, 70));
    author("subtitle.color", Color(70, 80, 90));
    author("value.color", Color(40, 60, 80));
    author("label.font", 13); author("subtitle.font", 10);
    author("right.font", 12); author("value.font", 11);
    author("subtitle.italic", true);
    ApplyProjection();
    SaveFile(AppendFileName(directory, "UiRangeSegmentsDemo_authored.cpp"), generated_);

    inspector_model_.SetValue("minimum", 0.0012345678901234567, false);
    inspector_model_.SetValue("maximum", 4.987654321098765, false);
    inspector_model_.SetValue("step", 0.00037, false);
    inspector_model_.SetValue("min_span", 0.00081, false);
    inspector_model_.SetValue("precision", 1, false);
    const double weights[] = { 0.12345678901234567, 0.456789123456789,
                               1.2345678901234567, 3.141592653589793 };
    for(int i = 0; i < 4; i++)
        data_model_.SetValue(Format("segment.%d.span", i), weights[i], false);
    ApplyProjection();
    SaveFile(AppendFileName(directory, "UiRangeSegmentsDemo_fractional.cpp"), generated_);
    // Native doubles are an independent source-model baseline for export checks.
    String scalar_model;
    auto append_scalar = [&](double value) {
        scalar_model.Cat(reinterpret_cast<const char*>(&value), sizeof(value));
    };
    append_scalar(ranges_.GetMin()); append_scalar(ranges_.GetMax());
    append_scalar(ranges_.GetStep()); append_scalar(ranges_.GetMinimumSegmentSpan());
    for(const UiRangeSegment& segment : ranges_.GetSegments())
        append_scalar(segment.span);
    SaveFile(AppendFileName(directory, "UiRangeSegmentsDemo_fractional-model.bin"), scalar_model);
}
}
