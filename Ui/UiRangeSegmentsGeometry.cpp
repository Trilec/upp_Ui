#include <Ui/UiRangeSegments.h>
#include <Ui/UiTheme.h>

namespace Upp {
namespace {

double ClampRangeGeometryValue(double v, double lo, double hi)
{
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

} // namespace

Size UiRangeSegments::ResolveThumbSize(const Style& style) const
{
    Size size(max(1, style.thumb_size.cx), max(1, style.thumb_size.cy));
    if(dir_ == UiDirection::V && style.thumb_rotate_with_direction)
        Swap(size.cx, size.cy);
    return size;
}

Rect UiRangeSegments::BuildTrackRect(Size size, const Style& style) const
{
    Rect outer(size);
    if(outer.IsEmpty())
        return outer;

    int cross = max(DPI(6), style.track_size.cy);
    Size thumb = ResolveThumbSize(style);
    int thumb_major = dir_ == UiDirection::H ? thumb.cx : thumb.cy;
    int major = dir_ == UiDirection::H ? size.cx : size.cy;
    int edge_pad = min(max(DPI(8), thumb_major / 2 + DPI(2)), major / 2);

    if(dir_ == UiDirection::H) {
        int top_reserve = show_boundary_values_ ? DPI(24) : DPI(4);
        int bottom_reserve = show_endpoint_values_ ? DPI(20) : DPI(4);
        int available_h = max(0, size.cy - top_reserve - bottom_reserve);
        cross = min(cross, available_h);
        int y = top_reserve + max(0, (available_h - cross) / 2);
        int width = max(0, size.cx - 2 * edge_pad);
        return RectC(edge_pad, y, width, cross);
    }

    int left_reserve = (show_boundary_values_ || show_endpoint_values_) ? DPI(56) : DPI(4);
    int right_reserve = DPI(4);
    int available_w = max(0, size.cx - left_reserve - right_reserve);
    cross = min(cross, available_w);
    int x = left_reserve + max(0, (available_w - cross) / 2);
    int height = max(0, size.cy - 2 * edge_pad);
    return RectC(x, edge_pad, cross, height);
}

int UiRangeSegments::ValueToPos(double value, const Rect& track) const
{
    int length = dir_ == UiDirection::H ? track.GetWidth() : track.GetHeight();
    if(length <= 0 || max_ <= min_)
        return 0;
    double t = (ClampRangeGeometryValue(value, min_, max_) - min_) / (max_ - min_);
    if(reversed_)
        t = 1.0 - t;
    return clamp(fround(t * length), 0, length);
}

double UiRangeSegments::PosToValue(int pos, const Rect& track) const
{
    int length = dir_ == UiDirection::H ? track.GetWidth() : track.GetHeight();
    if(length <= 0 || max_ <= min_)
        return min_;
    double t = ClampRangeGeometryValue((double)pos / length, 0.0, 1.0);
    if(reversed_)
        t = 1.0 - t;
    return NormalizeValue(min_ + t * (max_ - min_));
}

UiRangeSegments::Geometry UiRangeSegments::BuildGeometry(Size size) const
{
    Geometry g;
    g.outer = Rect(size);
    const Style& style = GetEffectiveStyle();
    g.track = BuildTrackRect(size, style);
    if(g.track.IsEmpty())
        return g;

    g.content = UiStyledInnerRect(g.track, style.track_metrics, style.track_skin);
    if(g.content.IsEmpty())
        g.content = g.track;

    double start = min_;
    int p0 = ValueToPos(start, g.content);
    for(int i = 0; i < segments_.GetCount(); i++) {
        SegmentGeometry sg;
        sg.index = i;
        sg.start = start;
        sg.end = i + 1 == segments_.GetCount() ? max_ : start + segments_[i].span;
        sg.color = ResolveSegmentColor(i, segments_[i], IsEnabled() && IsShowEnabled());
        int p1 = ValueToPos(sg.end, g.content);
        int lo = min(p0, p1);
        int hi = max(p0, p1);
        if(dir_ == UiDirection::H)
            sg.rect = Rect(g.content.left + lo, g.content.top,
                           g.content.left + hi, g.content.bottom);
        else
            sg.rect = Rect(g.content.left, g.content.top + lo,
                           g.content.right, g.content.top + hi);
        sg.visible = !sg.rect.IsEmpty();
        g.segments.Add(sg);
        if(i < GetBoundaryCount()) {
            if(dir_ == UiDirection::H)
                g.boundaries.Add(Point(g.content.left + p1, g.content.CenterPoint().y));
            else
                g.boundaries.Add(Point(g.content.CenterPoint().x, g.content.top + p1));
        }
        start = sg.end;
        p0 = p1;
    }
    Size thumb = ResolveThumbSize(style);
    // Bound authored thumbs to the available control, independently of their
    // hit target. Painting and input consume these same projected rectangles.
    thumb.cx = min(thumb.cx, max(1, size.cx));
    thumb.cy = min(thumb.cy, max(1, size.cy));
    for(int i = 0; i < g.boundaries.GetCount(); i++) {
        Point centre = g.boundaries[i];
        Rect rect = RectC(centre.x - thumb.cx / 2, centre.y - thumb.cy / 2,
                          thumb.cx, thumb.cy);
        if(IsEnabled() && IsShowEnabled() && (i == hot_boundary_ || i == active_boundary_)) {
            int growth = min(max(0, style.thumb_hover_growth), min(size.cx, size.cy) / 2);
            rect.Inflate(growth);
        }
        g.boundary_thumbs.Add(rect & g.outer);
    }
    for(SegmentGeometry& segment : g.segments)
        BuildLabelGeometry(segment, g, style);
    return g;
}

void UiRangeSegments::BuildLabelGeometry(SegmentGeometry& sg, const Geometry& g,
                                        const Style& style) const
{
    if(!sg.visible || (!show_labels_ && !show_segment_values_))
        return;
    Rect area = sg.rect;
    int padding = clamp(style.label_padding, 0, min(area.GetWidth(), area.GetHeight()) / 2);
    area.Deflate(padding);
    // Text must clear adjacent boundary handles, including a tall horizontal
    // handle in a vertical stack. Never reserve an unrelated row's handle.
    for(int i : {sg.index - 1, sg.index}) {
        if(i < 0 || i >= g.boundary_thumbs.GetCount()) continue;
        const Rect& thumb = g.boundary_thumbs[i];
        if(dir_ == UiDirection::H) {
            if(thumb.CenterPoint().x <= sg.rect.left)
                area.left = max(area.left, thumb.right + DPI(2));
            else area.right = min(area.right, thumb.left - DPI(2));
        }
        else {
            if(thumb.CenterPoint().y <= sg.rect.top)
                area.top = max(area.top, thumb.bottom + DPI(2));
            else area.bottom = min(area.bottom, thumb.top - DPI(2));
        }
    }
    if(area.IsEmpty()) return;
    const UiRangeSegment& segment = segments_[sg.index];
    bool title = show_labels_ && !segment.label.IsEmpty();
    bool subtitle = show_labels_ && !segment.subtitle.IsEmpty();
    int title_height = title ? GetTextSize(segment.label, style.label_font).cy : 0;
    int subtitle_height = subtitle ? GetTextSize(segment.subtitle, style.subtitle_font).cy : 0;
    int gap = clamp(style.label_gap, 0, area.GetHeight());
    if(title && subtitle && (int64)title_height + subtitle_height + gap > area.GetHeight())
        subtitle = false; // Small rows keep the heading readable first.
    int height = title_height + (subtitle ? subtitle_height : 0) + (title && subtitle ? gap : 0);
    int y = area.top + max(0, (area.GetHeight() - height) / 2);
    UiAlign side = style.value_side == UiAlign::LEFT ? UiAlign::LEFT : UiAlign::RIGHT;
    sg.label_align = style.label_align;
    if(sg.label_align != UiAlign::LEFT && sg.label_align != UiAlign::RIGHT && sg.label_align != UiAlign::CENTER)
        sg.label_align = show_segment_values_ ? (side == UiAlign::RIGHT ? UiAlign::LEFT : UiAlign::RIGHT)
                       : subtitle ? UiAlign::LEFT : UiAlign::CENTER;
    Rect text = area;
    if(show_segment_values_) {
        sg.value_text = GetSegmentValueText(sg.index);
        Size value_size = GetTextSize(sg.value_text, style.right_font);
        int width = min(value_size.cx, area.GetWidth());
        int value_y = title && subtitle ? y + (title_height - value_size.cy) / 2
                                       : area.top + (area.GetHeight() - value_size.cy) / 2;
        int x = side == UiAlign::LEFT ? area.left : area.right - width;
        sg.value_rect = RectC(x, value_y, width, value_size.cy) & area;
        int space = clamp(style.value_gap, 0, area.GetWidth());
        if(side == UiAlign::LEFT) text.left = min(text.right, sg.value_rect.right + space);
        else text.right = max(text.left, sg.value_rect.left - space);
    }
    if(text.IsEmpty()) return;
    if(title) sg.label_rect = RectC(text.left, y, text.GetWidth(), title_height) & area;
    if(subtitle) sg.subtitle_rect = RectC(text.left, y + (title ? title_height + gap : 0),
                                        text.GetWidth(), subtitle_height) & area;
}

UiRangeSegments::Geometry UiRangeSegments::GetGeometry(Size size) const
{
    return BuildGeometry(size);
}

Rect UiRangeSegments::BoundaryThumbRect(int index, const Geometry& geometry) const
{
    if(index < 0 || index >= geometry.boundary_thumbs.GetCount())
        return Rect();
    return geometry.boundary_thumbs[index];
}

int UiRangeSegments::HitBoundary(Point p, const Geometry& geometry) const
{
    int best = -1;
    int best_distance = INT_MAX;
    for(int i = 0; i < geometry.boundaries.GetCount(); i++) {
        Rect r = BoundaryThumbRect(i, geometry).Inflated(DPI(3));
        r.Inflate(max(0, (DPI(14) - r.GetWidth() + 1) / 2),
                  max(0, (DPI(14) - r.GetHeight() + 1) / 2));
        if(!r.Contains(p))
            continue;
        Point c = geometry.boundaries[i];
        int d = dir_ == UiDirection::H ? abs(p.x - c.x) : abs(p.y - c.y);
        if(d < best_distance) {
            best_distance = d;
            best = i;
        }
    }
    return best;
}

int UiRangeSegments::HitSegment(Point p, const Geometry& geometry) const
{
    if(!geometry.content.Contains(p))
        return -1;
    for(const SegmentGeometry& sg : geometry.segments)
        if(sg.visible && sg.rect.Contains(p))
            return sg.index;
    return -1;
}

Size UiRangeSegments::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    Size thumb = ResolveThumbSize(style);
    int major = (int)min<int64>(INT_MAX, (int64)max(DPI(60), style.track_size.cx) +
                (dir_ == UiDirection::H ? thumb.cx : thumb.cy) + DPI(8));
    int64 cross = max(style.track_size.cy,
                      dir_ == UiDirection::H ? thumb.cy : thumb.cx);
    if(dir_ == UiDirection::H) {
        cross += (show_boundary_values_ ? DPI(24) : DPI(4));
        cross += (show_endpoint_values_ ? DPI(20) : DPI(4));
        return Size(max(major, user_min_size_.cx), max((int)min<int64>(INT_MAX,cross), user_min_size_.cy));
    }

    cross += (show_boundary_values_ || show_endpoint_values_) ? DPI(60) : DPI(8);
    return Size(max((int)min<int64>(INT_MAX,cross), user_min_size_.cx), max(major, user_min_size_.cy));
}

void UiRangeSegments::SetMinSize(Size sz)
{
    user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    RefreshLayout();
}


} // namespace Upp
