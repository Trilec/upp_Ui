#include <Ui/UiRangeSegments.h>
#include <Ui/UiDraw.h>

namespace Upp {
namespace {

Color ContrastInk(Color c)
{
    if(IsNull(c))
        return SColorText();
    int luminance = c.GetR() * 299 + c.GetG() * 587 + c.GetB() * 114;
    return luminance >= 150000 ? Color(17, 24, 39) : White();
}

void PaintRectFrame(Draw& w, Rect r, Color c, int width)
{
    if(r.IsEmpty() || IsNull(c) || width <= 0)
        return;
    width = min(width, min(r.GetWidth(), r.GetHeight()) / 2);
    if(width <= 0)
        return;
    w.DrawRect(r.left, r.top, r.GetWidth(), width, c);
    w.DrawRect(r.left, r.bottom - width, r.GetWidth(), width, c);
    w.DrawRect(r.left, r.top + width, width, max(0, r.GetHeight() - 2 * width), c);
    w.DrawRect(r.right - width, r.top + width, width, max(0, r.GetHeight() - 2 * width), c);
}

} // namespace

void UiRangeSegments::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Geometry g = BuildGeometry(GetSize());
    if(g.track.IsEmpty())
        return;

    StyledState base_state = !IsEnabled() || !IsShowEnabled() ? ST_DISABLED : ST_NORMAL;
    UiPaintStyledBackground(w, g.track, style.track_palette, style.track_metrics,
                            style.track_skin, base_state, false);

    int radius = max(0, style.track_metrics.radius -
                         (style.track_metrics.frame_enabled ? style.track_metrics.frame_width : 0));
    for(const SegmentGeometry& sg : g.segments) {
        Color c = sg.color;
        if(sg.index == hot_segment_ && !dragging_)
            c = LtColor(c, 10);
        PaintSegmentFill(w, sg, g, c, radius);
    }

    if(show_dividers_) {
        Color divider = style.divider_color;
        if(IsNull(divider))
            divider = style.track_palette.frame[base_state];
        int normal_width = max(1, style.divider_width);
        for(int i = 0; i < g.boundaries.GetCount(); i++) {
            Point p = g.boundaries[i];
            const bool segment_edge = (hot_segment_ >= 0 && (i == hot_segment_ - 1 || i == hot_segment_)) ||
                                      (selected_segment_ >= 0 && (i == selected_segment_ - 1 || i == selected_segment_));
            const bool emphasized = i == hot_boundary_ || i == active_boundary_ || segment_edge;
            int width = normal_width + (emphasized ? DPI(1) : 0);
            Color c = i == active_boundary_ && !IsNull(style.selected_frame)
                    ? style.selected_frame : divider;
            if(dir_ == UiDirection::H)
                w.DrawRect(p.x - width / 2, g.content.top + DPI(2), width,
                           max(0, g.content.GetHeight() - DPI(4)), c);
            else
                w.DrawRect(g.content.left + DPI(2), p.y - width / 2,
                           max(0, g.content.GetWidth() - DPI(4)), width, c);
        }
    }

    if(selected_segment_ >= 0 && selected_segment_ < g.segments.GetCount()) {
        Color frame = style.selected_frame;
        if(IsNull(frame))
            frame = SColorHighlight();
        if(base_state == ST_DISABLED)
            frame = DisabledColor(frame);
        PaintRectFrame(w, g.segments[selected_segment_].rect, frame,
                       max(1, style.selected_frame_width));
    }

    if(show_labels_) {
        for(const SegmentGeometry& sg : g.segments) {
            if(!sg.visible || sg.index < 0 || sg.index >= segments_.GetCount())
                continue;
            const String& label = segments_[sg.index].label;
            if(label.IsEmpty())
                continue;
            Size ts = GetTextSize(label, style.label_font);
            if(ts.cx + 2 * style.label_padding > sg.rect.GetWidth() ||
               ts.cy + 2 * style.label_padding > sg.rect.GetHeight())
                continue;
            Color ink = ContrastInk(sg.color);
            if(base_state == ST_DISABLED)
                ink = DisabledColor(ink);
            w.DrawText(sg.rect.left + (sg.rect.GetWidth() - ts.cx) / 2,
                       sg.rect.top + (sg.rect.GetHeight() - ts.cy) / 2,
                       label, style.label_font, ink);
        }
    }

    for(int i = 0; i < g.boundaries.GetCount(); i++) {
        StyledState st = base_state;
        if(base_state != ST_DISABLED) {
            const bool segment_edge = (hot_segment_ >= 0 && (i == hot_segment_ - 1 || i == hot_segment_)) ||
                                      (selected_segment_ >= 0 && (i == selected_segment_ - 1 || i == selected_segment_));
            if(dragging_ && i == active_boundary_)
                st = ST_PRESSED;
            else if(i == hot_boundary_ || i == active_boundary_ || segment_edge)
                st = ST_HOT;
        }
        PaintBoundaryThumb(w, i, g, st);
        if(show_boundary_values_)
            PaintValueLabel(w, FormatValueLabel(GetBoundaryValue(i)), g.boundaries[i], true, style);
    }

    if(show_endpoint_values_) {
        int pmin = ValueToPos(min_, g.content);
        int pmax = ValueToPos(max_, g.content);
        if(dir_ == UiDirection::H) {
            Point a(g.content.left + pmin, g.content.bottom);
            Point b(g.content.left + pmax, g.content.bottom);
            PaintValueLabel(w, FormatValueLabel(min_), a, false, style);
            PaintValueLabel(w, FormatValueLabel(max_), b, false, style);
        }
        else {
            Point a(g.content.left, g.content.top + pmin);
            Point b(g.content.left, g.content.top + pmax);
            PaintValueLabel(w, FormatValueLabel(min_), a, false, style);
            PaintValueLabel(w, FormatValueLabel(max_), b, false, style);
        }
    }

    if(HasFocus() && style.track_metrics.focus_enabled) {
        Color fc = style.track_metrics.focus_color;
        if(IsNull(fc))
            fc = SColorHighlight();
        UiPaintFocusShape(w, g.track, style.track_metrics, ST_HOT, fc,
                          0, max(1, style.track_metrics.focus_margin),
                          style.track_metrics.focus_alpha,
                          style.track_metrics.focus_margin,
                          max(1.0, (double)style.track_metrics.focus_margin));
    }
}

Size UiRangeSegments::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    int major = max(DPI(60), style.track_size.cx) +
                (dir_ == UiDirection::H ? style.thumb_size.cx : style.thumb_size.cy) + DPI(8);
    int cross = max(style.track_size.cy,
                    dir_ == UiDirection::H ? style.thumb_size.cy : style.thumb_size.cx);
    if(dir_ == UiDirection::H) {
        cross += (show_boundary_values_ ? DPI(24) : DPI(4));
        cross += (show_endpoint_values_ ? DPI(20) : DPI(4));
        return Size(max(major, user_min_size_.cx), max(cross, user_min_size_.cy));
    }

    cross += (show_boundary_values_ || show_endpoint_values_) ? DPI(60) : DPI(8);
    return Size(max(cross, user_min_size_.cx), max(major, user_min_size_.cy));
}

void UiRangeSegments::SetMinSize(Size sz)
{
    user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    RefreshLayout();
}


} // namespace Upp
