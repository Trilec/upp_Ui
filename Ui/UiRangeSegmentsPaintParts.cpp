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

} // namespace

void UiRangeSegments::PaintSegmentFill(Draw& w, const SegmentGeometry& sg,
                                       const Geometry& g, Color color, int radius) const
{
    if(!sg.visible || IsNull(color))
        return;

    radius = max(0, radius);
    if(dir_ == UiDirection::H)
        radius = min(radius, g.content.GetHeight() / 2);
    else
        radius = min(radius, g.content.GetWidth() / 2);
    if(radius <= 0) {
        w.DrawRect(sg.rect, color);
        return;
    }

    const bool round_start = dir_ == UiDirection::H
                           ? sg.rect.left <= g.content.left
                           : sg.rect.top <= g.content.top;
    const bool round_end = dir_ == UiDirection::H
                         ? sg.rect.right >= g.content.right
                         : sg.rect.bottom >= g.content.bottom;
    if(!round_start && !round_end) {
        w.DrawRect(sg.rect, color);
        return;
    }

    w.Clip(sg.rect);
    if(dir_ == UiDirection::H) {
        Rect body = sg.rect;
        if(round_start) body.left = min(body.right, g.content.left + radius);
        if(round_end) body.right = max(body.left, g.content.right - radius);
        if(!body.IsEmpty())
            w.DrawRect(body, color);
        Rect band(sg.rect.left, g.content.top + radius,
                  sg.rect.right, g.content.bottom - radius);
        if(!band.IsEmpty())
            w.DrawRect(band, color);
        if(round_start) {
            w.DrawEllipse(RectC(g.content.left, g.content.top, radius * 2, radius * 2), color);
            w.DrawEllipse(RectC(g.content.left, g.content.bottom - radius * 2, radius * 2, radius * 2), color);
        }
        if(round_end) {
            w.DrawEllipse(RectC(g.content.right - radius * 2, g.content.top, radius * 2, radius * 2), color);
            w.DrawEllipse(RectC(g.content.right - radius * 2, g.content.bottom - radius * 2, radius * 2, radius * 2), color);
        }
    }
    else {
        Rect body = sg.rect;
        if(round_start) body.top = min(body.bottom, g.content.top + radius);
        if(round_end) body.bottom = max(body.top, g.content.bottom - radius);
        if(!body.IsEmpty())
            w.DrawRect(body, color);
        Rect band(g.content.left + radius, sg.rect.top,
                  g.content.right - radius, sg.rect.bottom);
        if(!band.IsEmpty())
            w.DrawRect(band, color);
        if(round_start) {
            w.DrawEllipse(RectC(g.content.left, g.content.top, radius * 2, radius * 2), color);
            w.DrawEllipse(RectC(g.content.right - radius * 2, g.content.top, radius * 2, radius * 2), color);
        }
        if(round_end) {
            w.DrawEllipse(RectC(g.content.left, g.content.bottom - radius * 2, radius * 2, radius * 2), color);
            w.DrawEllipse(RectC(g.content.right - radius * 2, g.content.bottom - radius * 2, radius * 2, radius * 2), color);
        }
    }
    w.End();
}

void UiRangeSegments::PaintBoundaryThumb(Draw& w, int index, const Geometry& g,
                                         StyledState state) const
{
    Rect r = BoundaryThumbRect(index, g);
    if(r.IsEmpty())
        return;
    if(index == hot_boundary_ || index == active_boundary_)
        r.Inflate(DPI(1));

    const Style& style = GetEffectiveStyle();
    Color face = FaceColor(style.thumb_palette, state, SColorFace());
    Color frame = style.thumb_palette.frame[state];
    if(IsNull(frame))
        frame = PaletteInk(style.thumb_palette, state, SColorShadow());
    int fw = style.thumb_metrics.frame_enabled ? max(1, style.thumb_metrics.frame_width) : 1;
    w.DrawEllipse(r, face, fw, frame);

    int dot = clamp(style.thumb_dot_diameter, DPI(2), min(r.GetWidth(), r.GetHeight()) - 2 * fw);
    if(dot > 0) {
        Color ink = PaletteInk(style.thumb_palette, state, frame);
        Rect d = RectC(r.CenterPoint().x - dot / 2, r.CenterPoint().y - dot / 2, dot, dot);
        w.DrawEllipse(d, ink);
    }
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
