#include <Ui/UiRangeSegments.h>
#include <Ui/UiDraw.h>

namespace Upp {
namespace {

Color FaceColor(const StyledPalette& p, StyledState st, Color fallback)
{
    const UiFill& f = p.face[st];
    return f.IsSolid() && !IsNull(f.color) ? f.color : fallback;
}

Color PaletteInk(const StyledPalette& p, StyledState st, Color fallback)
{
    Color c = p.ink[st];
    return IsNull(c) ? fallback : c;
}

// A range uses a narrow content strip, not a raster of its surrounding label
// whitespace. Very large authored sizes fall back to bounded tiles; they never
// bypass cache limits by allocating a full-control image.
template <class PaintFn>
Image RenderRangeRaster(Size size, Point origin, PaintFn paint)
{
    ImageBuffer buffer(size);
    buffer.SetKind(IMAGE_ALPHA);
    Fill(~buffer, RGBAZero(), buffer.GetLength());
    BufferPainter painter(buffer, MODE_ANTIALIASED);
    painter.Translate(-origin.x, -origin.y);
    paint(painter);
    painter.Finish();
    return Image(buffer);
}

template <class PaintFn>
void PaintRangeTiles(Draw& w, const Rect& bounds, PaintFn paint)
{
    const int tile_size = 256; // allocation bound, not a geometry/quality setting
    for(int y = bounds.top; y < bounds.bottom;) {
        const int cy = min(tile_size, bounds.bottom - y);
        for(int x = bounds.left; x < bounds.right;) {
            const int cx = min(tile_size, bounds.right - x);
            Rect tile = RectC(x, y, cx, cy);
            if(w.IsPainting(tile))
                w.DrawImage(x, y, RenderRangeRaster(tile.GetSize(), Point(x, y), paint));
            x += cx;
        }
        y += cy;
    }
}

} // namespace

void UiRangeSegments::PaintTrackContent(Draw& w, const Geometry& g,
                                        StyledState state, int radius) const
{
    if(g.content.IsEmpty() || g.segments.IsEmpty())
        return;
    radius = clamp(radius, 0, min(g.content.GetWidth(), g.content.GetHeight()) / 2);
    const Style& style = GetEffectiveStyle();
    Color divider = style.divider_color;
    if(IsNull(divider))
        divider = style.track_palette.frame[state];
    Color selected = style.selected_frame;
    if(IsNull(selected))
        selected = SColorHighlight();
    if(state == ST_DISABLED)
        selected = DisabledColor(selected);
    const int divider_width = max(1, style.divider_width);
    const int selected_width = min(max(1, style.selected_frame_width),
                                   min(g.content.GetWidth(), g.content.GetHeight()) / 2);

    auto paint = [&](Painter& p) {
        p.Begin();
        // One shared silhouette clips EVERY segment, including a second segment
        // entering a curved end when its neighbour is narrower than the radius.
        p.RoundedRectangle(Rectf(g.content), radius).Clip();
        for(const SegmentGeometry& sg : g.segments) {
            if(!sg.visible || IsNull(sg.color))
                continue;
            Color c = sg.color;
            if(state != ST_DISABLED && sg.index == hot_segment_ && !dragging_)
                c = LtColor(c, 10);
            p.DrawRect(sg.rect, c);
        }
        if(show_dividers_) {
            for(int i = 0; i < g.boundaries.GetCount(); i++) {
                Point pt = g.boundaries[i];
                const bool edge = (hot_segment_ >= 0 && (i == hot_segment_ - 1 || i == hot_segment_)) ||
                                  (selected_segment_ >= 0 && (i == selected_segment_ - 1 || i == selected_segment_));
                const bool emphasized = state != ST_DISABLED &&
                    (i == hot_boundary_ || i == active_boundary_ || edge);
                const int width = divider_width + (emphasized ? DPI(1) : 0);
                Color c = i == active_boundary_ ? selected : divider;
                if(dir_ == UiDirection::H)
                    p.DrawRect(pt.x - width / 2, g.content.top + DPI(2), width,
                               max(0, g.content.GetHeight() - DPI(4)), c);
                else
                    p.DrawRect(g.content.left + DPI(2), pt.y - width / 2,
                               max(0, g.content.GetWidth() - DPI(4)), width, c);
            }
        }
        if(selected_width > 0 && selected_segment_ >= 0 && selected_segment_ < g.segments.GetCount()) {
            const Rect& r = g.segments[selected_segment_].rect;
            if(!r.IsEmpty()) {
                p.Begin();
                p.Rectangle(Rectf(r)).Clip();
                const double inset = selected_width * 0.5;
                p.RoundedRectangle(g.content.left + inset, g.content.top + inset,
                                   g.content.GetWidth() - 2 * inset, g.content.GetHeight() - 2 * inset,
                                   max(0.0, radius - inset)).Stroke(selected_width, selected);
                if(dir_ == UiDirection::H) {
                    if(r.left > g.content.left)
                        p.DrawRect(r.left, r.top, selected_width, r.GetHeight(), selected);
                    if(r.right < g.content.right)
                        p.DrawRect(r.right - selected_width, r.top, selected_width, r.GetHeight(), selected);
                }
                else {
                    if(r.top > g.content.top)
                        p.DrawRect(r.left, r.top, r.GetWidth(), selected_width, selected);
                    if(r.bottom < g.content.bottom)
                        p.DrawRect(r.left, r.bottom - selected_width, r.GetWidth(), selected_width, selected);
                }
                p.End();
            }
        }
        p.End();
    };

    UiRasterCachePolicy policy = UiRasterPolicyAA("aa/ui-range-segments/track");
    policy.allow_scale_from_bucket = false;
    Size size = g.content.GetSize();
    if(UiQuantizeRasterSize(size, policy) != size) {
        PaintRangeTiles(w, g.content, paint);
        return;
    }
    Image raster;
    if(dragging_)
        raster = RenderRangeRaster(size, Point(g.content.left, g.content.top), paint);
    else {
        // Keys contain resolved pixel inputs, not scalar values or object IDs.
        // Equivalent repaints and theme round-trips can therefore reuse a raster.
        UiRasterCacheKeyBuilder key("aa/ui-range-segments/track");
        key.Add(size).Add(radius).Add((int)dir_).Add((int)state)
           .Add(show_dividers_).Add(divider).Add(divider_width)
           .Add(selected).Add(selected_width).Add(selected_segment_)
           .Add(active_boundary_).Add(hot_boundary_).Add(hot_segment_)
           .Add(g.segments.GetCount());
        for(const SegmentGeometry& sg : g.segments)
            key.Add(sg.rect.left - g.content.left).Add(sg.rect.top - g.content.top)
               .Add(sg.rect.GetWidth()).Add(sg.rect.GetHeight()).Add(sg.color);
        for(const Point& pt : g.boundaries)
            key.Add(pt.x - g.content.left).Add(pt.y - g.content.top);
        raster = UiRasterCache::Get(key.Build(), policy, [&] {
            return RenderRangeRaster(size, Point(g.content.left, g.content.top), paint);
        });
    }
    if(IsNull(raster))
        PaintRangeTiles(w, g.content, paint);
    else
        w.DrawImage(g.content.left, g.content.top, raster);
}

void UiRangeSegments::PaintBoundaryThumb(Draw& w, int index, const Geometry& g,
                                         StyledState state) const
{
    Rect r = BoundaryThumbRect(index, g);
    if(r.IsEmpty())
        return;
    if(state != ST_DISABLED && (index == hot_boundary_ || index == active_boundary_))
        r.Inflate(DPI(1));
    const Style& style = GetEffectiveStyle();
    Color face = style.thumb_metrics.face_enabled ? FaceColor(style.thumb_palette, state, Null) : Null;
    Color frame = style.thumb_palette.frame[state];
    if(IsNull(frame))
        frame = PaletteInk(style.thumb_palette, state, Null);
    const int fw = style.thumb_metrics.frame_enabled
                 ? clamp(style.thumb_metrics.frame_width, 0, min(r.GetWidth(), r.GetHeight()) / 2) : 0;
    const int dot = clamp(style.thumb_dot_diameter, 0,
                          max(0, min(r.GetWidth(), r.GetHeight()) - 2 * fw));
    Color ink = PaletteInk(style.thumb_palette, state, frame);
    const Size size = r.GetSize();
    auto paint = [=](Painter& p) {
        const double inset = max(0.5, fw * 0.5);
        const double cx = r.left + size.cx * 0.5;
        const double cy = r.top + size.cy * 0.5;
        const double rx = max(0.0, size.cx * 0.5 - inset);
        const double ry = max(0.0, size.cy * 0.5 - inset);
        if(rx > 0.0 && ry > 0.0) {
            p.Begin();
            p.Ellipse(cx, cy, rx, ry);
            if(!IsNull(face))
                p.Fill(face);
            if(fw > 0 && !IsNull(frame))
                p.Stroke(fw, frame);
            p.End();
        }
        if(dot > 0 && !IsNull(ink))
            p.Circle(cx, cy, dot * 0.5).Fill(ink);
    };
    UiRasterCachePolicy policy = UiRasterPolicyAA("aa/ui-range-segments/thumb");
    policy.allow_scale_from_bucket = false;
    if(UiQuantizeRasterSize(size, policy) != size) {
        PaintRangeTiles(w, r, paint);
        return;
    }
    UiRasterCacheKeyBuilder key("aa/ui-range-segments/thumb");
    key.Add(size).Add(face).Add(frame).Add(fw).Add(dot).Add(ink);
    Image raster = UiRasterCache::Get(key.Build(), policy, [=] {
        return RenderRangeRaster(size, Point(r.left, r.top), paint);
    });
    if(IsNull(raster))
        PaintRangeTiles(w, r, paint);
    else
        w.DrawImage(r.left, r.top, raster);
}

void UiRangeSegments::PaintValueLabel(Draw& w, const String& text, Point anchor,
                                      bool boundary_label, const Style& style) const
{
    if(text.IsEmpty())
        return;
    Font font = style.value_font;
    Size ts = GetTextSize(text, font);
    int px = DPI(5), py = DPI(2);
    Rect r;
    if(dir_ == UiDirection::H) {
        int y = boundary_label ? anchor.y - ts.cy - 2 * py - DPI(8)
                               : anchor.y + DPI(6);
        r = RectC(anchor.x - (ts.cx + 2 * px) / 2, y,
                  ts.cx + 2 * px, ts.cy + 2 * py);
    }
    else {
        int x = anchor.x - ts.cx - 2 * px - DPI(8);
        r = RectC(x, anchor.y - (ts.cy + 2 * py) / 2,
                  ts.cx + 2 * px, ts.cy + 2 * py);
    }

    Rect bounds(GetSize());
    if(r.left < bounds.left)
        r.Offset(bounds.left - r.left, 0);
    if(r.right > bounds.right)
        r.Offset(bounds.right - r.right, 0);
    if(r.top < bounds.top)
        r.Offset(0, bounds.top - r.top);
    if(r.bottom > bounds.bottom)
        r.Offset(0, bounds.bottom - r.bottom);

    StyledMetrics metrics;
    metrics.face_enabled = true;
    metrics.frame_enabled = boundary_label;
    metrics.frame_width = 1;
    metrics.radius = DPI(4);
    StyledState state = !IsEnabled() || !IsShowEnabled() ? ST_DISABLED : ST_NORMAL;
    UiPaintStyledBackground(w, r, style.value_palette, metrics, StyledSkin(), state, false);
    Color ink = PaletteInk(style.value_palette, state, SColorText());
    w.DrawText(r.left + (r.GetWidth() - ts.cx) / 2,
               r.top + (r.GetHeight() - ts.cy) / 2,
               text, font, ink);
}


} // namespace Upp
