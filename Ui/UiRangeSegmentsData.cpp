#include <Ui/UiRangeSegments.h>

namespace Upp {

UiRangeSegments& UiRangeSegments::SetPaletteMode(PaletteMode mode)
{
    if(palette_mode_ != mode) {
        palette_mode_ = mode;
        Refresh();
    }
    return *this;
}

UiRangeSegments& UiRangeSegments::SetSeriesColor(int index, Color color)
{
    if(index < 0 || index >= MAX_SERIES_COLORS)
        return *this;
    Style& s = StyleEdit();
    s.series[index] = color;
    s.series_count = max(s.series_count, index + 1);
    Refresh();
    return *this;
}

Color UiRangeSegments::GetSeriesColor(int index) const
{
    const Style& s = GetEffectiveStyle();
    if(index < 0 || index >= MAX_SERIES_COLORS)
        return Null;
    return s.series[index];
}

UiRangeSegments& UiRangeSegments::SetPalette(const Vector<Color>& colors)
{
    Style& s = StyleEdit();
    int n = min(colors.GetCount(), MAX_SERIES_COLORS);
    for(int i = 0; i < n; i++)
        s.series[i] = colors[i];
    s.series_count = max(1, n);
    Refresh();
    return *this;
}

UiRangeSegments& UiRangeSegments::ShowLabels(bool on)
{
    if(show_labels_ != on) {
        show_labels_ = on;
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiRangeSegments& UiRangeSegments::ShowBoundaryValues(bool on)
{
    if(show_boundary_values_ != on) {
        show_boundary_values_ = on;
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiRangeSegments& UiRangeSegments::ShowEndpointValues(bool on)
{
    if(show_endpoint_values_ != on) {
        show_endpoint_values_ = on;
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiRangeSegments& UiRangeSegments::ShowValuesOnInteraction(bool on)
{
    if(show_values_on_interaction_ != on) {
        show_values_on_interaction_ = on;
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiRangeSegments& UiRangeSegments::ShowDividers(bool on)
{
    if(show_dividers_ != on) {
        show_dividers_ = on;
        Refresh();
    }
    return *this;
}

UiRangeSegments& UiRangeSegments::SetValueDisplay(ValueDisplay display)
{
    if(value_display_ != display) {
        value_display_ = display;
        Refresh();
    }
    return *this;
}

UiRangeSegments& UiRangeSegments::SetValuePrecision(int decimals)
{
    value_precision_ = clamp(decimals, 0, 8);
    RefreshLayout();
    Refresh();
    return *this;
}

UiRangeSegments& UiRangeSegments::SetTrackSize(Size sz)
{
    Style& s = StyleEdit();
    s.track_size = Size(max(DPI(60), sz.cx), max(DPI(6), sz.cy));
    RefreshLayout();
    Refresh();
    return *this;
}

UiRangeSegments& UiRangeSegments::SetThumbSize(Size sz)
{
    Style& s = StyleEdit();
    s.thumb_size = Size(max(DPI(8), sz.cx), max(DPI(8), sz.cy));
    RefreshLayout();
    Refresh();
    return *this;
}

void UiRangeSegments::SetData(const Value& value)
{
    if(IsNull(value) || !value.Is<ValueArray>())
        return;

    ValueArray array = value;
    Vector<UiRangeSegment> segments;
    for(int i = 0; i < array.GetCount(); i++) {
        const Value& entry = array[i];
        if(entry.Is<ValueMap>()) {
            ValueMap map = entry;
            UiRangeSegment segment;
            int q = map.Find("span");
            segment.span = q >= 0 && !IsNull(map.GetValue(q)) ? (double)map.GetValue(q) : 0.0;
            q = map.Find("label");
            if(q >= 0 && !IsNull(map.GetValue(q)))
                segment.label = AsString(map.GetValue(q));
            q = map.Find("color");
            if(q >= 0 && !IsNull(map.GetValue(q)))
                segment.color = Color(map.GetValue(q));
            q = map.Find("data");
            if(q >= 0)
                segment.data = map.GetValue(q);
            segments.Add(segment);
        }
        else if(IsNumber(entry))
            segments.Add(UiRangeSegment((double)entry));
    }
    SetSegments(segments);
}

Value UiRangeSegments::GetData() const
{
    ValueArray array;
    for(const UiRangeSegment& segment : segments_) {
        ValueMap map;
        map.Add("span", segment.span);
        map.Add("label", segment.label);
        if(!IsNull(segment.color))
            map.Add("color", segment.color);
        if(!IsNull(segment.data))
            map.Add("data", segment.data);
        array.Add(map);
    }
    return array;
}


} // namespace Upp
