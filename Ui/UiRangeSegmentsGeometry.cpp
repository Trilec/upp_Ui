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

Rect UiRangeSegments::BuildTrackRect(Size size, const Style& style) const
{
    Rect outer(size);
    if(outer.IsEmpty())
        return outer;

    int cross = max(DPI(6), style.track_size.cy);
    int thumb_major = dir_ == UiDirection::H ? style.thumb_size.cx : style.thumb_size.cy;
    int edge_pad = max(DPI(8), thumb_major / 2 + DPI(2));

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
    return g;
}

UiRangeSegments::Geometry UiRangeSegments::GetGeometry(Size size) const
{
    return BuildGeometry(size);
}

Rect UiRangeSegments::BoundaryThumbRect(int index, const Geometry& geometry) const
{
    if(index < 0 || index >= geometry.boundaries.GetCount())
        return Rect();
    const Style& style = GetEffectiveStyle();
    Size sz(max(DPI(8), style.thumb_size.cx), max(DPI(8), style.thumb_size.cy));
    Point c = geometry.boundaries[index];
    return RectC(c.x - sz.cx / 2, c.y - sz.cy / 2, sz.cx, sz.cy);
}

int UiRangeSegments::HitBoundary(Point p, const Geometry& geometry) const
{
    int best = -1;
    int best_distance = INT_MAX;
    for(int i = 0; i < geometry.boundaries.GetCount(); i++) {
        Rect r = BoundaryThumbRect(i, geometry).Inflated(DPI(3));
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
