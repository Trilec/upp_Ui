#include "UiRangeSegmentsDemo.h"

namespace Upp {
namespace {

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

    Vector<UiRangeSegment> lod;
    lod.Add(UiRangeSegment(35, "Normal"));
    lod.Add(UiRangeSegment(25, "LOD 1"));
    lod.Add(UiRangeSegment(22, "LOD 2"));
    lod.Add(UiRangeSegment(18, "LOD 3"));
    ranges_.SetRange(0, 100).SetSegments(lod).SetValueDisplay(UiRangeSegments::ValueDisplay::Percent);

    RegisterPropertyEditorV1Editors(factory_);
    BuildHeader();
    BuildPreview();
    BuildRightRail();
    BuildInspector();
    BuildOverrides();
    BuildDataModel();
    ConfigureEditors();
    ConnectEvents();
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
    int caption_h = DPI(54);
    int area_h = max(0, ps.cy - caption_h);
    int rw = min(max(DPI(90), (int)InspectorValue("width", 650)), max(0, ps.cx - DPI(28)));
    int rh = min(max(DPI(70), (int)InspectorValue("height", 110)), max(0, area_h - DPI(28)));
    ranges_.SetRect(max(0, (ps.cx - rw) / 2), max(0, (area_h - rh) / 2), rw, rh);
    caption_.SetRect(0, max(0, ps.cy - caption_h), ps.cx, caption_h);

    Size rs = right_.GetSize();
    tools_.SetRect(DPI(4), DPI(4), max(0, rs.cx - DPI(8)), DPI(36));
    pages_.SetRect(DPI(4), DPI(44), max(0, rs.cx - DPI(8)), max(0, rs.cy - DPI(48)));
}

void UiRangeSegmentsDemo::BuildHeader()
{
    Add(header_);
    header_.SetTitle("UiRangeSegments")
           .SetSubTitle("Several labelled scalar spans sharing one locked domain")
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
    preview_.Add(ranges_);
    preview_.Add(caption_);
    caption_.SetText("Hover a segment to identify its range. Drag a circular boundary thumb to redistribute only the two neighbouring spans; the outer domain remains fixed.")
            .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
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
    inspector_model_.AddBoolean("show_labels", "Segment labels", true, "Presentation");
    inspector_model_.AddBoolean("show_boundary_values", "Boundary values", true, "Presentation");
    inspector_model_.AddBoolean("show_endpoint_values", "Endpoint values", true, "Presentation");
    inspector_model_.AddBoolean("show_dividers", "Divider lines", true, "Presentation");

    inspector_model_.AddChoice("role", "Role", "Standard", "Theme")
                    .AddChoice("Standard", "Standard").AddChoice("Subtle", "Subtle")
                    .AddChoice("Accent", "Accent").AddChoice("Alert", "Alert");
    inspector_model_.AddNumericInt("width", "Preview width", 650, 90, 900, 1, "Layout").SetUnit("px");
    inspector_model_.AddNumericInt("height", "Preview height", 110, 70, 600, 1, "Layout").SetUnit("px");
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

    MarkOverride(override_model_.AddColor("track.face", "Face", FaceColor(base.track_palette, ST_NORMAL, Color(241,245,249)), "Track"));
    MarkOverride(override_model_.AddColor("track.frame", "Frame", base.track_palette.frame[ST_NORMAL], "Track"));
    MarkOverride(override_model_.AddNumericInt("track.height", "Thickness", base.track_size.cy, 6, 80, 1, "Track").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("track.radius", "Radius", base.track_metrics.radius, 0, 40, 1, "Track").SetUnit("px"));

    MarkOverride(override_model_.AddColor("thumb.face", "Face", FaceColor(base.thumb_palette, ST_NORMAL, White()), "Boundary Thumb"));
    MarkOverride(override_model_.AddColor("thumb.frame", "Frame", base.thumb_palette.frame[ST_NORMAL], "Boundary Thumb"));
    MarkOverride(override_model_.AddNumericInt("thumb.size", "Diameter", base.thumb_size.cx, 8, 40, 1, "Boundary Thumb").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("thumb.dot", "Centre dot", base.thumb_dot_diameter, 2, 16, 1, "Boundary Thumb").SetUnit("px"));

    MarkOverride(override_model_.AddColor("divider", "Divider", base.divider_color, "Selection"));
    MarkOverride(override_model_.AddColor("selected", "Selected frame", base.selected_frame, "Selection"));
    MarkOverride(override_model_.AddNumericInt("selected.width", "Selected width", base.selected_frame_width, 1, 8, 1, "Selection").SetUnit("px"));

    for(int i = 0; i < 6; i++)
        MarkOverride(override_model_.AddColor("series." + AsString(i), Format("Series %d", i + 1), base.series[i], "Series"));

    MarkOverride(override_model_.AddNumericInt("label.font", "Label size", base.label_font.GetHeight(), 7, 32, 1, "Typography").SetUnit("px"));
    MarkOverride(override_model_.AddNumericInt("value.font", "Value size", base.value_font.GetHeight(), 7, 28, 1, "Typography").SetUnit("px"));

    override_model_.SetGroupSubtitle("Series", "deterministic inherited palette; Gradient samples interpolates across these anchors");
    override_model_.SetGroupSubtitle("Boundary Thumb", "small circular threshold handle; direct native ellipse drawing keeps drag lightweight");
    override_model_.StructureChanged();
}

void UiRangeSegmentsDemo::BuildDataModel()
{
    data_model_.Clear(false);
    data_segment_count_ = ranges_.GetSegmentCount();
    UiRangeSegments::Geometry g = ranges_.GetGeometry(Size(640, 110));
    double domain = max(1.0, ranges_.GetMax() - ranges_.GetMin());

    for(int i = 0; i < data_segment_count_; i++) {
        String group = Format("Segment %d", i + 1);
        const UiRangeSegment& segment = ranges_.GetSegment(i);
        Color resolved = i < g.segments.GetCount() ? g.segments[i].color : Color(100, 116, 139);
        data_model_.AddText(Format("segment.%d.label", i), "Label", segment.label, group);
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
    auto changed = [=](String, Value) { ApplyProjection(); };
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

    inspector_mode_.WhenAction = [=] { SelectPage(0); };
    overrides_mode_.WhenAction = [=] { SelectPage(1); };
    data_mode_.WhenAction = [=] { SelectPage(2); };
    code_mode_.WhenAction = [=] { SelectPage(3); };
    theme_.WhenAction = [=] { ToggleTheme(); };
    help_.WhenAction = [=] {
        PromptOK("UiRangeSegments reference demo\n\nThe control owns one fixed scalar domain and N contiguous labelled spans. Dragging an internal circular thumb changes only the two neighbouring spans. Data exposes labels, weights and optional explicit colours; the automatic palette can be discrete or sampled as a gradient.");
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
    override_model_.StructureChanged();
    overrides_.RefreshModel();
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
        segments.Add(UiRangeSegment(span, label, color, payload));
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
        data_model_.SetValue(id, ranges_.GetSegmentSpan(i), false);
    }
    data_.RefreshModel();
}

void UiRangeSegmentsDemo::SyncInheritedOverrides(const UiRangeSegments::Style& style)
{
    auto SetIfInherited = [&](const char *id, const Value& value) {
        if(!OverrideActive(id))
            override_model_.SetValue(id, value, false);
    };
    SetIfInherited("track.face", FaceColor(style.track_palette, ST_NORMAL, Color(241,245,249)));
    SetIfInherited("track.frame", style.track_palette.frame[ST_NORMAL]);
    SetIfInherited("track.height", style.track_size.cy);
    SetIfInherited("track.radius", style.track_metrics.radius);
    SetIfInherited("thumb.face", FaceColor(style.thumb_palette, ST_NORMAL, White()));
    SetIfInherited("thumb.frame", style.thumb_palette.frame[ST_NORMAL]);
    SetIfInherited("thumb.size", style.thumb_size.cx);
    SetIfInherited("thumb.dot", style.thumb_dot_diameter);
    SetIfInherited("divider", style.divider_color);
    SetIfInherited("selected", style.selected_frame);
    SetIfInherited("selected.width", style.selected_frame_width);
    for(int i = 0; i < 6; i++) {
        String id = "series." + AsString(i);
        if(!OverrideActive(id))
            override_model_.SetValue(id, style.series[i], false);
    }
    SetIfInherited("label.font", style.label_font.GetHeight());
    SetIfInherited("value.font", style.value_font.GetHeight());
    overrides_.RefreshModel();
}

bool UiRangeSegmentsDemo::ApplyOverrides(UiRangeSegments::Style& style) const
{
    bool any = false;
    auto ColorOverride = [&](const char *id, Color& target) {
        if(OverrideActive(id)) {
            target = Color(OverrideValue(id));
            any = true;
        }
    };
    if(OverrideActive("track.face")) {
        style.track_palette.face[ST_NORMAL] = UiFill::Solid(Color(OverrideValue("track.face")));
        any = true;
    }
    ColorOverride("track.frame", style.track_palette.frame[ST_NORMAL]);
    if(OverrideActive("track.height")) { style.track_size.cy = max(6, (int)OverrideValue("track.height")); any = true; }
    if(OverrideActive("track.radius")) { style.track_metrics.radius = max(0, (int)OverrideValue("track.radius")); any = true; }
    if(OverrideActive("thumb.face")) {
        style.thumb_palette.face[ST_NORMAL] = UiFill::Solid(Color(OverrideValue("thumb.face")));
        any = true;
    }
    ColorOverride("thumb.frame", style.thumb_palette.frame[ST_NORMAL]);
    if(OverrideActive("thumb.size")) {
        int d = max(8, (int)OverrideValue("thumb.size"));
        style.thumb_size = Size(d, d);
        any = true;
    }
    if(OverrideActive("thumb.dot")) { style.thumb_dot_diameter = max(2, (int)OverrideValue("thumb.dot")); any = true; }
    ColorOverride("divider", style.divider_color);
    ColorOverride("selected", style.selected_frame);
    if(OverrideActive("selected.width")) { style.selected_frame_width = max(1, (int)OverrideValue("selected.width")); any = true; }
    for(int i = 0; i < 6; i++) {
        String id = "series." + AsString(i);
        if(OverrideActive(id)) {
            style.series[i] = Color(OverrideValue(id));
            style.series_count = max(style.series_count, i + 1);
            any = true;
        }
    }
    if(OverrideActive("label.font")) { style.label_font.Height(max(7, (int)OverrideValue("label.font"))); any = true; }
    if(OverrideActive("value.font")) { style.value_font.Height(max(7, (int)OverrideValue("value.font"))); any = true; }
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
           .ShowBoundaryValues((bool)InspectorValue("show_boundary_values", true))
           .ShowEndpointValues((bool)InspectorValue("show_endpoint_values", true))
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
    SyncInheritedOverrides(base);
    if(ApplyOverrides(base))
        ranges_.SetCustomStyle(base);
    else
        ranges_.ClearCustomStyle();

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
    ApplyProjection();
}

void UiRangeSegmentsDemo::UpdateGeneratedCode()
{
    String out;
    out << "UiRangeSegments ranges;\n";
    out << "ranges.SetRange(" << FormatDoubleFix(ranges_.GetMin(), 2) << ", "
        << FormatDoubleFix(ranges_.GetMax(), 2) << ")\n";
    out << "      .SetStep(" << FormatDoubleFix(ranges_.GetStep(), 2) << ")\n";
    out << "      .SetMinimumSegmentSpan(" << FormatDoubleFix(ranges_.GetMinimumSegmentSpan(), 2) << ")\n";
    out << "      .SetDirection(" << (ranges_.GetDirection() == UiDirection::V ? "UiDirection::V" : "UiDirection::H") << ")\n";
    out << "      .SetReverse(" << (ranges_.IsReversed() ? "true" : "false") << ")\n";
    out << "      .SetPaletteMode(UiRangeSegments::PaletteMode::"
        << (ranges_.GetPaletteMode() == UiRangeSegments::PaletteMode::Gradient ? "Gradient" : "Series") << ")\n";
    out << "      .SetValueDisplay(UiRangeSegments::ValueDisplay::"
        << (ranges_.GetValueDisplay() == UiRangeSegments::ValueDisplay::Percent ? "Percent" : "Domain") << ")\n";
    out << "      .SetValuePrecision(" << ranges_.GetValuePrecision() << ")\n";
    out << "      .ShowLabels(" << (ranges_.AreLabelsShown() ? "true" : "false") << ")\n";
    out << "      .ShowBoundaryValues(" << (ranges_.AreBoundaryValuesShown() ? "true" : "false") << ")\n";
    out << "      .ShowEndpointValues(" << (ranges_.AreEndpointValuesShown() ? "true" : "false") << ")\n";
    out << "      .ShowDividers(" << (ranges_.AreDividersShown() ? "true" : "false") << ");\n\n";

    out << "Vector<UiRangeSegment> segments;\n";
    for(const UiRangeSegment& segment : ranges_.GetSegments()) {
        out << "segments.Add(UiRangeSegment(" << FormatDoubleFix(segment.span, 3)
            << ", " << CppString(segment.label) << ", " << CppColor(segment.color) << "));\n";
    }
    out << "ranges.SetSegments(segments);\n";

    if(ranges_.GetRole() != UiRole::Standard) {
        String role = ranges_.GetRole() == UiRole::Subtle ? "Subtle"
                    : ranges_.GetRole() == UiRole::Accent ? "Accent" : "Alert";
        out << "ranges.SetRole(UiRole::" << role << ");\n";
    }

    if(ranges_.HasCustomStyle()) {
        out << "\n// Active Theme Overrides\n";
        out << "UiRangeSegments::Style style = ranges.GetStyle();\n";
        if(OverrideActive("track.face")) out << "style.track_palette.face[ST_NORMAL] = UiFill::Solid(" << CppColor(Color(OverrideValue("track.face"))) << ");\n";
        if(OverrideActive("track.frame")) out << "style.track_palette.frame[ST_NORMAL] = " << CppColor(Color(OverrideValue("track.frame"))) << ";\n";
        if(OverrideActive("track.height")) out << "style.track_size.cy = " << (int)OverrideValue("track.height") << ";\n";
        if(OverrideActive("track.radius")) out << "style.track_metrics.radius = " << (int)OverrideValue("track.radius") << ";\n";
        if(OverrideActive("thumb.face")) out << "style.thumb_palette.face[ST_NORMAL] = UiFill::Solid(" << CppColor(Color(OverrideValue("thumb.face"))) << ");\n";
        if(OverrideActive("thumb.frame")) out << "style.thumb_palette.frame[ST_NORMAL] = " << CppColor(Color(OverrideValue("thumb.frame"))) << ";\n";
        if(OverrideActive("thumb.size")) out << "style.thumb_size = Size(" << (int)OverrideValue("thumb.size") << ", " << (int)OverrideValue("thumb.size") << ");\n";
        if(OverrideActive("thumb.dot")) out << "style.thumb_dot_diameter = " << (int)OverrideValue("thumb.dot") << ";\n";
        if(OverrideActive("divider")) out << "style.divider_color = " << CppColor(Color(OverrideValue("divider"))) << ";\n";
        if(OverrideActive("selected")) out << "style.selected_frame = " << CppColor(Color(OverrideValue("selected"))) << ";\n";
        if(OverrideActive("selected.width")) out << "style.selected_frame_width = " << (int)OverrideValue("selected.width") << ";\n";
        for(int i = 0; i < 6; i++) {
            String id = "series." + AsString(i);
            if(OverrideActive(id))
                out << "style.series[" << i << "] = " << CppColor(Color(OverrideValue(id))) << ";\n";
        }
        if(OverrideActive("label.font")) out << "style.label_font.Height(" << (int)OverrideValue("label.font") << ");\n";
        if(OverrideActive("value.font")) out << "style.value_font.Height(" << (int)OverrideValue("value.font") << ");\n";
        out << "ranges.SetCustomStyle(style);\n";
    }

    generated_ = out;
    code_.SetData(generated_);
}

} // namespace Upp
