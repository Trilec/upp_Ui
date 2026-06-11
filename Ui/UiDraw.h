#ifndef _Ui_UiDraw_h_
#define _Ui_UiDraw_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiDraw
    ======

    Purpose
    - Shared drawing helpers for styled Ui controls.

    Intent
    - Centralize reusable paint primitives such as styled backgrounds, text,
      icons, focus treatments, caps, and popup composition so controls do not
      duplicate rendering logic.

    Thread context
    - GUI thread for live Draw use.

    Usage
    - Controls call these helpers from render-only Paint() paths after layout
      and cache state have already been resolved.

    Changelog
    - 2026-03: added release-standard file documentation.
    - 2026-03-31: added shared indicator-glyph painters for checkbox/radio/menu reuse.
    - 2026-04: moved icon tint/preserve behavior onto shared UiIconRenderMode
      so controls no longer carry button-local icon rendering policy.
*/

#include <Painter/Painter.h>
#include <cmath>
//#include <CtrlCore/CtrlCore.h>   // AccessKeyBit
#include <CtrlLib/CtrlLib.h>     // DrawSmartText, DisabledImage, DPI, etc.
#include <Ui/UiStyle.h>

namespace Upp {

enum UiCapShape : byte {
    UICAP_FLAT_OPEN = 0,
    UICAP_FLAT_CLOSED,
    UICAP_OPEN,
    UICAP_CLOSED,
    UICAP_LINE,
    UICAP_LINE_OPPOSITE,
    UICAP_NONE,
};

inline void UiPaintFaceFrameDash(Draw& w, const Rect& outer,
                                 const StyledPalette& palette,
                                 const StyledMetrics& m,
                                 StyledState st);

struct UiShadowCacheKey : Moveable<UiShadowCacheKey> {
    int  width = 0;
    int  height = 0;
    int  radius = 0;
    bool inset = false;
    int  mode = 0;
    int  alpha = 0;
    Color color;
    int  distance = 0;
    int  offset_x = 0;
    int  offset_y = 0;
    int  curve_x1 = 0;
    int  curve_y1 = 0;
    int  curve_x2 = 0;
    int  curve_y2 = 0;

    bool operator==(const UiShadowCacheKey& b) const
    {
        return width == b.width && height == b.height && radius == b.radius
            && inset == b.inset && mode == b.mode && alpha == b.alpha
            && color == b.color && distance == b.distance
            && offset_x == b.offset_x && offset_y == b.offset_y
            && curve_x1 == b.curve_x1 && curve_y1 == b.curve_y1
            && curve_x2 == b.curve_x2 && curve_y2 == b.curve_y2;
    }
};

inline hash_t GetHashValue(const UiShadowCacheKey& k)
{
    CombineHash h;
    h << k.width << k.height << k.radius << k.inset << k.mode << k.alpha << k.color.GetRaw()
      << k.distance << k.offset_x << k.offset_y
      << k.curve_x1 << k.curve_y1 << k.curve_x2 << k.curve_y2;
    return h;
}

template <class Key, class Factory>
inline Image UiGetCachedImage(const Key& key, Factory factory, int max_items = 128)
{
    static StaticMutex s_mutex;
    static Index<Key>   s_keys;
    static Vector<Image> s_images;

    Mutex::Lock __(s_mutex);
    int idx = s_keys.Find(key);
    if(idx >= 0)
        return s_images[idx];

    Image img = factory();
    if(s_keys.GetCount() >= max_items) {
        s_keys.Remove(0);
        s_images.Remove(0);
    }
    s_keys.Add(key);
    s_images.Add(img);
    return img;
}

inline UiAlign UiCapOpenSide(UiAlign tab_side)
{
    switch(tab_side) {
    case UiAlign::TOP: return UiAlign::BOTTOM;
    case UiAlign::BOTTOM: return UiAlign::TOP;
    case UiAlign::LEFT: return UiAlign::RIGHT;
    case UiAlign::RIGHT: return UiAlign::LEFT;
    default: return UiAlign::BOTTOM;
    }
}

inline void UiPaintStyledCap(Draw& w,
                             const Rect& outer,
                             const StyledPalette& palette,
                             const StyledMetrics& metrics,
                             StyledState st,
                             UiAlign tab_side,
                             UiCapShape shape)
{
    if(outer.IsEmpty() || shape == UICAP_NONE)
        return;

    Color edge = palette.frame[st];
    const UiFill& ff = palette.face[st];
    Color face = ff.IsSolid() ? ff.color : Null;
    int fw = max(1, metrics.frame_width);
    int radius = max(0, metrics.radius);

    if(shape == UICAP_LINE || shape == UICAP_LINE_OPPOSITE) {
        if(IsNull(edge))
            return;
        int th = max(1, fw);
        UiAlign line_side = (shape == UICAP_LINE) ? UiCapOpenSide(tab_side) : tab_side;
        switch(line_side) {
        case UiAlign::TOP:    w.DrawRect(outer.left, outer.top, outer.GetWidth(), th, edge); break;
        case UiAlign::BOTTOM: w.DrawRect(outer.left, outer.bottom - th, outer.GetWidth(), th, edge); break;
        case UiAlign::LEFT:   w.DrawRect(outer.left, outer.top, th, outer.GetHeight(), edge); break;
        case UiAlign::RIGHT:  w.DrawRect(outer.right - th, outer.top, th, outer.GetHeight(), edge); break;
        default: break;
        }
        return;
    }

    if(shape == UICAP_CLOSED) {
        StyledMetrics m = metrics;
        UiPaintFaceFrameDash(w, outer, palette, m, st);
        return;
    }

    if(shape == UICAP_FLAT_CLOSED) {
        if(!IsNull(face))
            w.DrawRect(outer, face);
        if(IsNull(edge))
            return;

        UiAlign open = UiCapOpenSide(tab_side);
        int rr = max(0, min(radius, min(outer.GetWidth(), outer.GetHeight()) / 2 - 1));
        int l = outer.left;
        int t = outer.top;
        int r = outer.right;
        int b = outer.bottom;

        if(open == UiAlign::BOTTOM) {
            w.DrawRect(l + rr, t, max(0, outer.GetWidth() - rr * 2), fw, edge);
            w.DrawRect(l, t + rr, fw, max(0, outer.GetHeight() - rr), edge);
            w.DrawRect(r - fw, t + rr, fw, max(0, outer.GetHeight() - rr), edge);
            w.DrawRect(l, b - fw, outer.GetWidth(), fw, edge);
            for(int x = 0; x < rr; x++) {
                double q = (double)x / max(1, rr - 1);
                int y = t + (int)((1.0 - q) * (1.0 - q) * (rr - 1));
                w.DrawRect(l + x, y, 1, 1, edge);
                w.DrawRect(r - 1 - x, y, 1, 1, edge);
            }
            return;
        }
        if(open == UiAlign::TOP) {
            w.DrawRect(l + rr, b - fw, max(0, outer.GetWidth() - rr * 2), fw, edge);
            w.DrawRect(l, t, fw, max(0, outer.GetHeight() - rr), edge);
            w.DrawRect(r - fw, t, fw, max(0, outer.GetHeight() - rr), edge);
            w.DrawRect(l, t, outer.GetWidth(), fw, edge);
            for(int x = 0; x < rr; x++) {
                double q = (double)x / max(1, rr - 1);
                int y = b - 1 - (int)((1.0 - q) * (1.0 - q) * (rr - 1));
                w.DrawRect(l + x, y, 1, 1, edge);
                w.DrawRect(r - 1 - x, y, 1, 1, edge);
            }
            return;
        }
        if(open == UiAlign::RIGHT) {
            w.DrawRect(l, t + rr, fw, max(0, outer.GetHeight() - rr * 2), edge);
            w.DrawRect(l, t, max(0, outer.GetWidth() - rr), fw, edge);
            w.DrawRect(l, b - fw, max(0, outer.GetWidth() - rr), fw, edge);
            w.DrawRect(r - fw, t, fw, outer.GetHeight(), edge);
            for(int y = 0; y < rr; y++) {
                double q = (double)y / max(1, rr - 1);
                int x = l + (int)((1.0 - q) * (1.0 - q) * (rr - 1));
                w.DrawRect(x, t + y, 1, 1, edge);
                w.DrawRect(x, b - 1 - y, 1, 1, edge);
            }
            return;
        }
        if(open == UiAlign::LEFT) {
            w.DrawRect(r - fw, t + rr, fw, max(0, outer.GetHeight() - rr * 2), edge);
            w.DrawRect(l + rr, t, max(0, outer.GetWidth() - rr), fw, edge);
            w.DrawRect(l + rr, b - fw, max(0, outer.GetWidth() - rr), fw, edge);
            w.DrawRect(l, t, fw, outer.GetHeight(), edge);
            for(int y = 0; y < rr; y++) {
                double q = (double)y / max(1, rr - 1);
                int x = r - 1 - (int)((1.0 - q) * (1.0 - q) * (rr - 1));
                w.DrawRect(x, t + y, 1, 1, edge);
                w.DrawRect(x, b - 1 - y, 1, 1, edge);
            }
            return;
        }
    }

    int rr = max(0, min(radius, min(outer.GetWidth(), outer.GetHeight()) / 2 - 1));
    int ext = rr;

    bool vertical = (tab_side == UiAlign::LEFT || tab_side == UiAlign::RIGHT);
    int base_w = outer.GetWidth();
    int base_h = outer.GetHeight();
    int mw = (vertical ? base_h : base_w) + ext * 2;
    int mh = (vertical ? base_w : base_h) + ext * 2;

    double L = ext;
    double T = ext;
    double R = L + (vertical ? base_h : base_w) - 1;
    double B = T + (vertical ? base_w : base_h) - 1;
    double rad = min<double>(rr, min(R - L, B - T) * 0.5);

    auto AddArc = [&](Vector<Pointf>& out, double cx, double cy, double a0, double a1) {
        if(rad <= 0)
            return;
        int steps = max(6, (int)rad * 2);
        for(int i = 1; i <= steps; i++) {
            double q = (double)i / steps;
            double a = a0 + (a1 - a0) * q;
            out.Add(Pointf(cx + cos(a) * rad, cy + sin(a) * rad));
        }
    };

    Vector<Pointf> fill_poly;
    fill_poly.Add(Pointf(L + rad, T));
    fill_poly.Add(Pointf(R - rad, T));
    AddArc(fill_poly, R - rad, T + rad, -M_PI * 0.5, 0.0);

    if(shape == UICAP_OPEN && rad > 0) {
        fill_poly.Add(Pointf(R, B - rad));
        AddArc(fill_poly, R + rad, B - rad, M_PI, M_PI * 0.5);
        fill_poly.Add(Pointf(L - rad, B));
        AddArc(fill_poly, L - rad, B - rad, M_PI * 0.5, 0.0);
        fill_poly.Add(Pointf(L, T + rad));
    }
    else {
        fill_poly.Add(Pointf(R, B));
        fill_poly.Add(Pointf(R + rad, B));
        fill_poly.Add(Pointf(L - rad, B));
        fill_poly.Add(Pointf(L, B));
        fill_poly.Add(Pointf(L, T + rad));
    }
    AddArc(fill_poly, L + rad, T + rad, M_PI, M_PI * 1.5);

    Vector<Pointf> border_poly;
    if(shape == UICAP_OPEN && rad > 0) {
        border_poly.Add(Pointf(L - rad, B));
        AddArc(border_poly, L - rad, B - rad, M_PI * 0.5, 0.0);
        border_poly.Add(Pointf(L, T + rad));
        AddArc(border_poly, L + rad, T + rad, M_PI, M_PI * 1.5);
        border_poly.Add(Pointf(R - rad, T));
        AddArc(border_poly, R - rad, T + rad, -M_PI * 0.5, 0.0);
        border_poly.Add(Pointf(R, B - rad));
        AddArc(border_poly, R + rad, B - rad, M_PI, M_PI * 0.5);
    }
    else {
        border_poly.Add(Pointf(L - rad, B));
        border_poly.Add(Pointf(L, B));
        border_poly.Add(Pointf(L, T + rad));
        AddArc(border_poly, L + rad, T + rad, M_PI, M_PI * 1.5);
        border_poly.Add(Pointf(R - rad, T));
        AddArc(border_poly, R - rad, T + rad, -M_PI * 0.5, 0.0);
        border_poly.Add(Pointf(R, B));
        border_poly.Add(Pointf(R + rad, B));
    }

    auto Map = [&](double u, double v) -> Pointf {
        double x = u;
        double y = v;
        switch(tab_side) {
        case UiAlign::TOP:
            break;
        case UiAlign::BOTTOM:
            y = (mh - 1) - v;
            break;
        case UiAlign::LEFT:
            x = v;
            y = (mw - 1) - u;
            break;
        case UiAlign::RIGHT:
            x = (mh - 1) - v;
            y = u;
            break;
        default:
            break;
        }
        return Pointf(x, y);
    };

    int tw = base_w + ext * 2;
    int th = base_h + ext * 2;
    ImageBuffer ib(tw, th);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    BufferPainter p(ib, MODE_ANTIALIASED);

    if(!IsNull(face)) {
        p.Begin();
        p.Move(Map(fill_poly[0].x, fill_poly[0].y));
        for(int i = 1; i < fill_poly.GetCount(); i++)
            p.Line(Map(fill_poly[i].x, fill_poly[i].y));
        p.Close();
        p.Fill(face);
        p.End();
    }

    if(!IsNull(edge)) {
        p.Begin();
        p.Move(Map(border_poly[0].x, border_poly[0].y));
        for(int i = 1; i < border_poly.GetCount(); i++)
            p.Line(Map(border_poly[i].x, border_poly[i].y));
        p.Stroke((double)fw, edge);
        p.End();
    }

    w.DrawImage(outer.left - ext, outer.top - ext, ib);
}

// -------------------------------------------------------------------------
// Image alpha helper
// -------------------------------------------------------------------------
inline Image UiImageMultiplyAlpha(const Image& src, int alpha)
{
    if(IsNull(src))
        return src;
    alpha = clamp(alpha, 0, 255);
    if(alpha >= 255)
        return src;
    if(alpha <= 0)
        return Image();

    Image       tmp = src;
    ImageBuffer ib(tmp);
    ib.SetKind(IMAGE_ALPHA);

    // Image is (typically) premultiplied; scale RGB and A together.
    RGBA* p = ib;
    int   n = ib.GetLength();
    for(int i = 0; i < n; i++) {
        RGBA& px = p[i];
        px.r = (byte)((px.r * alpha + 127) / 255);
        px.g = (byte)((px.g * alpha + 127) / 255);
        px.b = (byte)((px.b * alpha + 127) / 255);
        px.a = (byte)((px.a * alpha + 127) / 255);
    }

    return Image(ib);
}

/*
    UiDraw.h
    ========

    Changelog (migration notes):
    - 9-slice skin thickness: use UiIsZeroThicknessRect(skin.slice) and pass skin.slice
      to UiDraw9Slice (do NOT use Rect::IsEmpty() for thickness-rect checks).
    - Access-key drawing: DrawSmartText expects an access-key bitmask, not a raw wchar.
      UiPaintStyledText now converts wchar -> AccessKeyBit(wchar).
    - Multiline empty-line measurement: empty lines contribute height but not width
      (Size(0, fontHeight)) to avoid accidental min-width inflation for texts like "\n".
*/

static void sPaintSpinArrow(Draw& w, Size sz, bool up, Color c)
{
    int w2 = sz.cx / 2;
    int h2 = sz.cy / 2;
    int s  = min(w2, h2) / 2 + 1;

    if(up) {
        w.DrawLine(w2,       h2 - s / 2, w2 - s, h2 + s / 2, 2, c);
        w.DrawLine(w2,       h2 - s / 2, w2 + s, h2 + s / 2, 2, c);
    }
    else {
        w.DrawLine(w2,       h2 + s / 2, w2 - s, h2 - s / 2, 2, c);
        w.DrawLine(w2,       h2 + s / 2, w2 + s, h2 - s / 2, 2, c);
    }
}

// ============================================================================
// 9-slice helper
// ============================================================================
//
// UiDraw9Slice(w, Destination_rect, source_image,
//              Rect(inset_left, inset_top, inset_right, inset_bottom))
//
inline void UiDraw9Slice(Draw& w, const Rect& dst, const Image& src, const Rect& inset)
{
    if(IsNull(src) || dst.IsEmpty())
        return;

    Size sz = src.GetSize();
    if(sz.cx <= 0 || sz.cy <= 0) {
        w.DrawImage(dst, src);
        return;
    }

    int l = max(inset.left,   0);
    int t = max(inset.top,    0);
    int r = max(inset.right,  0);
    int b = max(inset.bottom, 0);

    if(l + r > sz.cx) {
        int extra = l + r - sz.cx;
        l -= extra / 2;
        r -= extra - extra / 2;
        l = max(l, 0);
        r = max(r, 0);
    }
    if(t + b > sz.cy) {
        int extra = t + b - sz.cy;
        t -= extra / 2;
        b -= extra - extra / 2;
        t = max(t, 0);
        b = max(b, 0);
    }

    if(l == 0 && t == 0 && r == 0 && b == 0) {
        w.DrawImage(dst, src);
        return;
    }

    int s_l = 0;
    int s_t = 0;
    int s_r = sz.cx;
    int s_b = sz.cy;

    int s_l2 = s_l + l;
    int s_r2 = s_r - r;
    int s_t2 = s_t + t;
    int s_b2 = s_b - b;

    int d_l = dst.left;
    int d_t = dst.top;
    int d_r = dst.right;
    int d_b = dst.bottom;

    int d_l2 = d_l + l;
    int d_r2 = d_r - r;
    int d_t2 = d_t + t;
    int d_b2 = d_b - b;

    if(d_l2 > d_r2) {
        int mid = (d_l + d_r) / 2;
        d_l2 = d_r2 = mid;
    }
    if(d_t2 > d_b2) {
        int mid = (d_t + d_b) / 2;
        d_t2 = d_b2 = mid;
    }

    // Corners ---------------------------------------------------------------
    w.DrawImage(Rect(d_l,  d_t,  d_l2, d_t2), src, Rect(s_l,  s_t,  s_l2, s_t2)); // TL
    w.DrawImage(Rect(d_r2, d_t,  d_r,  d_t2), src, Rect(s_r2, s_t,  s_r,  s_t2)); // TR
    w.DrawImage(Rect(d_l,  d_b2, d_l2, d_b),  src, Rect(s_l,  s_b2, s_l2, s_b));  // BL
    w.DrawImage(Rect(d_r2, d_b2, d_r,  d_b),  src, Rect(s_r2, s_b2, s_r,  s_b));  // BR

    // Edges -----------------------------------------------------------------
    if(d_r2 > d_l2) {
        w.DrawImage(Rect(d_l2, d_t,  d_r2, d_t2), src, Rect(s_l2, s_t,  s_r2, s_t2)); // Top
        w.DrawImage(Rect(d_l2, d_b2, d_r2, d_b),  src, Rect(s_l2, s_b2, s_r2, s_b));  // Bottom
    }

    if(d_b2 > d_t2) {
        w.DrawImage(Rect(d_l,  d_t2, d_l2, d_b2), src, Rect(s_l,  s_t2, s_l2, s_b2)); // Left
        w.DrawImage(Rect(d_r2, d_t2, d_r,  d_b2), src, Rect(s_r2, s_t2, s_r,  s_b2)); // Right
    }

    // Centre ----------------------------------------------------------------
    if(d_r2 > d_l2 && d_b2 > d_t2) {
        w.DrawImage(Rect(d_l2, d_t2, d_r2, d_b2), src, Rect(s_l2, s_t2, s_r2, s_b2));
    }
}

// -------------------------------------------------------------------------
// Styled icon helper
// -------------------------------------------------------------------------
inline void UiPaintStyledIcon(Draw& w,
                              const Rect& area,
                              const Image& src,
                              bool scale,
                              bool preserve_aspect,
                              UiIconRenderMode render_mode,
                              Color ink,
                              bool enabled)
{
    if(area.IsEmpty() || IsNull(src))
        return;

    Image img = src;

    const bool mono = render_mode == UiIconRenderMode::MonoTint;
    const bool preserve_color = render_mode == UiIconRenderMode::PreserveColor;

    // Disabled handling: only auto-gray when the icon is not explicitly
    // being tinted or preserved by the caller's render policy.
    if(!enabled && !mono && !preserve_color)
        img = DisabledImage(img);

    Size src_sz = img.GetSize();
    if(src_sz.cx <= 0 || src_sz.cy <= 0)
        return;

    int dst_w = src_sz.cx;
    int dst_h = src_sz.cy;

    if(scale) {
        // Exact-size callers such as UiButton/UiLabel pass preserve_aspect=false
        // so explicit icon width/height mean the rendered icon size, not a fit box.
        if(preserve_aspect) {
            double sx = (double)area.GetWidth()  / src_sz.cx;
            double sy = (double)area.GetHeight() / src_sz.cy;
            double s  = min(sx, sy);

            if(s > 0) {
                dst_w = max(1, int(src_sz.cx * s + 0.5));
                dst_h = max(1, int(src_sz.cy * s + 0.5));
            }
        }
        else {
            dst_w = max(1, area.GetWidth());
            dst_h = max(1, area.GetHeight());
        }
    }

    int img_x = area.left + (area.GetWidth()  - dst_w) / 2;
    int img_y = area.top  + (area.GetHeight() - dst_h) / 2;

    Image draw_img = img;
    if(scale && (dst_w != src_sz.cx || dst_h != src_sz.cy))
        draw_img = CachedRescale(img, Size(dst_w, dst_h));

    if(mono && !IsNull(ink))
        w.DrawImage(img_x, img_y, draw_img, ink);
    else
        w.DrawImage(img_x, img_y, draw_img);
}

// -------------------------------------------------------------------------
// Paints the styled "box" for a control
// -------------------------------------------------------------------------
inline void UiPaintFaceFrameDash(Draw& w, const Rect& outer,
                                 const StyledPalette& palette,
                                 const StyledMetrics& m,
                                 StyledState st)
{
    if(!m.face_enabled && !m.frame_enabled)
        return;

    const int fw     = max(m.frame_width, 0);
    const int radius = max(m.radius, 0);

    const UiFill& ff = palette.face[st];

    if(radius <= 0 && !m.dashed) {
        if(m.face_enabled && !ff.IsNone()) {
            if(ff.IsSolid())
                w.DrawRect(outer, ff.color);
            else if(ff.IsImage() && !IsNull(ff.image))
                w.DrawImage(outer, ff.image);
        }

        if(m.frame_enabled && fw > 0 && !IsNull(palette.frame[st])) {
            Rect  fr = outer;
            Color fc = palette.frame[st];

            w.DrawRect(fr.left,             fr.top,           fr.GetWidth(), fw, fc);
            w.DrawRect(fr.left,             fr.bottom - fw,   fr.GetWidth(), fw, fc);
            w.DrawRect(fr.left,             fr.top,           fw,            fr.GetHeight(), fc);
            w.DrawRect(fr.right - fw,       fr.top,           fw,            fr.GetHeight(), fc);
        }
        return;
    }

    Size sz = outer.GetSize();
    if(sz.cx <= 0 || sz.cy <= 0)
        return;

    ImageBuffer ib(sz);
    Fill(~ib, RGBAZero(), ib.GetLength());

    {
        BufferPainter p(ib, MODE_ANTIALIASED);

        double inset = fw > 0 ? max(0.5, fw * 0.5) : 0.5;
        double x     = inset;
        double y     = inset;
        double wdt   = sz.cx - 2 * inset;
        double hgt   = sz.cy - 2 * inset;

        int    max_r = min(sz.cx, sz.cy) / 2;
        double rad   = (double)min(radius, max_r);

        p.Begin();
        p.RoundedRectangle(x, y, wdt, hgt, rad);

        if(m.face_enabled && !ff.IsNone()) {
            if(ff.IsSolid()) {
                p.Fill(ff.color);
            }
            else if(ff.IsImage() && !IsNull(ff.image)) {
                Size isz = ff.image.GetSize();
                if(isz.cx > 0 && isz.cy > 0) {
                    double sx = wdt / isz.cx;
                    double sy = hgt / isz.cy;

                    Xform2D xf = Xform2D::Scale(sx, sy)
                               * Xform2D::Translation(x, y);

                    p.Fill(ff.image, xf, FILL_FAST);
                }
            }
        }

        if(m.frame_enabled && fw > 0 && !IsNull(palette.frame[st])) {
            if(m.dashed && !m.dash_pattern.IsEmpty())
                p.Dash(m.dash_pattern, 0.0);
            p.Stroke(fw, palette.frame[st]);
        }
        p.End();
    }

    w.DrawImage(outer.left, outer.top, ib);
}

// -------------------------------------------------------------------------
// Focus ring helper
// -------------------------------------------------------------------------
inline void UiPaintFocusShape(Draw& w,
                              Rect outer,
                              const StyledMetrics& metrics,
                              StyledState st,
                              Color color,
                              int inset = 0,
                              int outset = 0,
                              int alpha = 255,
                              int radius_adjust = 0,
                              double stroke_override = 0.0)
{
    if(IsNull(color) || alpha <= 0)
        return;
    if(inset > 0)
        outer.Deflate(inset, inset);
    if(outset > 0)
        outer.Inflate(outset, outset);
    if(outer.IsEmpty())
        return;
    Size sz = outer.GetSize();
    if(sz.cx <= 0 || sz.cy <= 0)
        return;
    ImageBuffer ib(sz);
    Fill(~ib, RGBAZero(), ib.GetLength());
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        double inset_px = 0.5;
        double x = inset_px;
        double y = inset_px;
        double wdt = sz.cx - 2 * inset_px;
        double hgt = sz.cy - 2 * inset_px;
        int max_r = min(sz.cx, sz.cy) / 2;
        double rad = (double)min(max(0, metrics.radius + radius_adjust), max_r);
        double stroke_w = stroke_override > 0.0
                        ? stroke_override
                        : (st == ST_PRESSED ? 2.0 : (st == ST_HOT ? 1.5 : 1.0));
        p.Begin();
        if(rad > 0)
            p.RoundedRectangle(x, y, wdt, hgt, rad);
        else
            p.Rectangle(x, y, wdt, hgt);
        if(metrics.dashed && !metrics.dash_pattern.IsEmpty())
            p.Dash(metrics.dash_pattern, 0.0);
        RGBA c = color;
        c.a = (byte)clamp(alpha, 0, 255);
        p.Stroke(stroke_w, c);
        p.End();
    }
    w.DrawImage(outer.left, outer.top, ib);
}


// -------------------------------------------------------------------------
// Default styled background / foreground helpers
// -------------------------------------------------------------------------
inline void UiPaintStyledBackground(Draw& w,
                                    const Rect& outer,
                                    const StyledPalette& palette,
                                    const StyledMetrics& metrics,
                                    const StyledSkin&    skin,
                                    StyledState          st, bool focus)
{
    if(outer.IsEmpty())
        return;

    (void)focus;

    Rect surface_outer = UiStyledSurfaceRect(outer, metrics);

    static StaticMutex s_shadow_cache_mutex;
    static Index<UiShadowCacheKey> s_shadow_cache_keys;
    static Vector<Image> s_shadow_cache_images;

    auto MakeShadowCacheKey = [&](const StyledShadow& ly, Size sz, int radius) -> UiShadowCacheKey {
        UiShadowCacheKey key;
        key.width = sz.cx;
        key.height = sz.cy;
        key.radius = radius;
        key.inset = ly.inset;
        key.mode = (int)ly.mode;
        key.alpha = ly.alpha;
        key.color = ly.color;
        key.distance = ly.distance;
        key.offset_x = ly.offset_x;
        key.offset_y = ly.offset_y;
        key.curve_x1 = (int)std::round(ly.curve.x1 * 1000.0);
        key.curve_y1 = (int)std::round(ly.curve.y1 * 1000.0);
        key.curve_x2 = (int)std::round(ly.curve.x2 * 1000.0);
        key.curve_y2 = (int)std::round(ly.curve.y2 * 1000.0);
        return key;
    };

    auto PaintShadow = [&](const StyledShadow& ly) {
        if(!ly.enabled)
            return;

        int extent = UiResolveShadowExtentPx(ly);
        if(extent <= 0)
            return;

        Point off = UiResolveShadowOffset(ly);
        Size osz = surface_outer.GetSize();
        if(osz.cx <= 0 || osz.cy <= 0)
            return;

        int pad = ly.inset ? 0 : (extent + max(abs(off.x), abs(off.y)) + 2);
        Size cache_sz = ly.inset ? osz : Size(osz.cx + pad * 2, osz.cy + pad * 2);
        UiShadowCacheKey cache_key = MakeShadowCacheKey(ly, cache_sz, metrics.radius);
        {
            Mutex::Lock __(s_shadow_cache_mutex);
            int idx = s_shadow_cache_keys.Find(cache_key);
            if(idx >= 0) {
                const Image& img = s_shadow_cache_images[idx];
                if(ly.inset)
                    w.DrawImage(surface_outer.left, surface_outer.top, img);
                else
                    w.DrawImage(surface_outer.left - pad, surface_outer.top - pad, img);
                return;
            }
        }

        Image img;
                if(ly.inset) {
            ImageBuffer ib(osz);
            Fill(~ib, RGBAZero(), ib.GetLength());
            {
                BufferPainter p(ib, MODE_ANTIALIASED);
                Rect base(0, 0, osz.cx, osz.cy);
                for(int i = 0; i < extent; i++) {
                    // Sample at the center of each shadow band. Sampling the
                    // outer edge directly causes distance=1 to evaluate at 1.0,
                    // which collapses the first visible step to zero alpha.
                    double t = ((double)i + 0.5) / (double)max(1, extent);
                    double curve_alpha = ly.mode == SHADOW_HARD ? 1.0 : (1.0 - UiShadowCurveEval(ly.curve, t));
                    int alpha = clamp((int)std::round(ly.alpha * curve_alpha), 0, 255);
                    if(alpha <= 0)
                        continue;
                    Rect rr = base;
                    rr.Deflate(i, i);
                    rr.Offset(off);
                    if(rr.GetWidth() <= 1 || rr.GetHeight() <= 1)
                        break;
                    double rad = (double)max(0, metrics.radius - i);
                    RGBA c = alpha * ly.color;
                    p.Begin();
                    if(rad > 0)
                        p.RoundedRectangle(rr.left + 0.5, rr.top + 0.5, rr.GetWidth() - 1.0, rr.GetHeight() - 1.0, min<double>(rad, min(rr.GetWidth(), rr.GetHeight()) / 2.0));
                    else
                        p.Rectangle(rr.left + 0.5, rr.top + 0.5, rr.GetWidth() - 1.0, rr.GetHeight() - 1.0);
                    if(ly.mode == SHADOW_HARD)
                        p.Fill(c);
                    else
                        p.Stroke(1.0, c);
                    p.End();
                }
            }
            Premultiply(ib);
            img = Image(ib);
            w.DrawImage(surface_outer.left, surface_outer.top, img);
        }
        else {
            Size sz(osz.cx + pad * 2, osz.cy + pad * 2);
            if(sz.cx <= 0 || sz.cy <= 0)
                return;
            ImageBuffer seed_ib(sz);
            seed_ib.SetKind(IMAGE_ALPHA);
            Fill(~seed_ib, RGBAZero(), seed_ib.GetLength());
            Rect base(pad, pad, pad + osz.cx, pad + osz.cy);
            Rect seed = base;
            double rad = (double)max(0, metrics.radius);
            RGBA c = White();
            c.a = 255;
            {
                BufferPainter p(seed_ib, MODE_ANTIALIASED);
                p.Begin();
                if(rad > 0)
                    p.RoundedRectangle(seed.left + 0.5, seed.top + 0.5, seed.GetWidth() - 1.0, seed.GetHeight() - 1.0, min<double>(rad, min(seed.GetWidth(), seed.GetHeight()) / 2.0));
                else
                    p.Rectangle(seed.left + 0.5, seed.top + 0.5, seed.GetWidth() - 1.0, seed.GetHeight() - 1.0);
                p.Fill(c);
                p.End();
            }
            // Cut only the true interior so the shadow can meet the control edge cleanly.
            ImageBuffer cutoff_ib(sz);
            cutoff_ib.SetKind(IMAGE_ALPHA);
            Fill(~cutoff_ib, RGBAZero(), cutoff_ib.GetLength());
            {
                BufferPainter p(cutoff_ib, MODE_ANTIALIASED);
                Rect cutoff = seed;
                cutoff.Deflate(1, 1);
                double cutoff_rad = max(0.0, rad - 1.0);
                if(cutoff.GetWidth() > 1 && cutoff.GetHeight() > 1) {
                    p.Begin();
                    if(cutoff_rad > 0)
                        p.RoundedRectangle(cutoff.left + 0.5, cutoff.top + 0.5, cutoff.GetWidth() - 1.0, cutoff.GetHeight() - 1.0, min<double>(cutoff_rad, min(cutoff.GetWidth(), cutoff.GetHeight()) / 2.0));
                    else
                        p.Rectangle(cutoff.left + 0.5, cutoff.top + 0.5, cutoff.GetWidth() - 1.0, cutoff.GetHeight() - 1.0);
                    p.Fill(c);
                    p.End();
                }
            }
            ImageBuffer ib(sz);
            Fill(~ib, RGBAZero(), ib.GetLength());
            Rect cutoff = seed;
            cutoff.Deflate(1, 1);
            double cutoff_rad = max(0.0, rad - 1.0);
            bool smooth_shells = false;
            {
                BufferPainter p(ib, MODE_ANTIALIASED);
                if(ly.mode == SHADOW_HARD) {
                    Rect rr = seed;
                    rr.Inflate(extent, extent);
                    rr.Offset(off);
                    double rr_rad = max(0.0, rad + extent);
                    RGBA fill = clamp(ly.alpha, 0, 255) * ly.color;
                    p.Begin();
                    if(rr_rad > 0)
                        p.RoundedRectangle(rr.left + 0.5, rr.top + 0.5, rr.GetWidth() - 1.0, rr.GetHeight() - 1.0, min<double>(rr_rad, min(rr.GetWidth(), rr.GetHeight()) / 2.0));
                    else
                        p.Rectangle(rr.left + 0.5, rr.top + 0.5, rr.GetWidth() - 1.0, rr.GetHeight() - 1.0);
                    p.Fill(fill);
                    p.End();
                }
                else {
                    smooth_shells = true;
                    int next_alpha = 0;
                    for(int d = extent; d >= 1; --d) {
                        // Build the shadow from differential filled shells.
                        // Each band contributes only the opacity needed to
                        // reach the target alpha at that distance, which keeps
                        // the edge full-strength and avoids visible ring gaps.
                        double t = ((double)d - 0.5) / (double)max(1, extent);
                        double curve_alpha = 1.0 - UiShadowCurveEval(ly.curve, t);
                        int target_alpha = clamp((int)std::round(ly.alpha * curve_alpha), 0, 255);
                        int shell_alpha = max(0, target_alpha - next_alpha);
                        next_alpha = target_alpha;
                        if(shell_alpha <= 0)
                            continue;

                        double ox = (double)off.x * (double)d / (double)max(1, extent);
                        double oy = (double)off.y * (double)d / (double)max(1, extent);
                        double x = (double)seed.left - d + ox + 0.5;
                        double y = (double)seed.top - d + oy + 0.5;
                        double wdt = seed.GetWidth() + d * 2 - 1.0;
                        double hgt = seed.GetHeight() + d * 2 - 1.0;
                        double rr_rad = max(0.0, rad + d);
                        RGBA fill = shell_alpha * ly.color;
                        p.Begin();
                        if(rr_rad > 0)
                            p.RoundedRectangle(x, y, wdt, hgt, min<double>(rr_rad, min(wdt, hgt) / 2.0));
                        else
                            p.Rectangle(x, y, wdt, hgt);
                        p.Fill(fill);
                        p.End();
                    }
                }
            }
            if(smooth_shells && extent > 1)
                FastBlur(ib, 1);
            {
                // Cut interior after smoothing so the shadow meets the control
                // edge cleanly without reintroducing visible shell steps.
                BufferPainter p(ib, MODE_ANTIALIASED);
                RGBA erase = RGBAZero();
                p.Begin();
                if(rad > 0)
                    p.RoundedRectangle(cutoff.left + 0.5, cutoff.top + 0.5, cutoff.GetWidth() - 1.0, cutoff.GetHeight() - 1.0, min<double>(cutoff_rad, min(cutoff.GetWidth(), cutoff.GetHeight()) / 2.0));
                else
                    p.Rectangle(cutoff.left + 0.5, cutoff.top + 0.5, cutoff.GetWidth() - 1.0, cutoff.GetHeight() - 1.0);
                p.Fill(erase);
                p.End();
            }
            Premultiply(ib);
            img = Image(ib);
            w.DrawImage(surface_outer.left - pad, surface_outer.top - pad, img);
        }

        Mutex::Lock __(s_shadow_cache_mutex);
        if(s_shadow_cache_keys.GetCount() >= 128) {
            s_shadow_cache_keys.Remove(0);
            s_shadow_cache_images.Remove(0);
        }
        if(s_shadow_cache_keys.Find(cache_key) < 0) {
            s_shadow_cache_keys.Add(cache_key);
            s_shadow_cache_images.Add(img);
        }
    };

    auto PaintShadowStack = [&](const StyledShadow& sh, bool inset_only) {
        if(!sh.enabled || sh.alpha <= 0)
            return;
        if(sh.inset != inset_only)
            return;
        PaintShadow(sh);
    };

    auto PaintHighlight = [&](const StyledHighlight& hl) {
        if(!hl.enabled || hl.alpha <= 0)
            return;

        Rect r = outer;
        r.Offset(hl.offset_x, hl.offset_y);
        int th = max(1, hl.thickness);
        Color c = Blend(hl.color, White(), clamp(hl.alpha, 0, 255));

        if(hl.style == SOLID) {
            w.DrawRect(r.left, r.top, r.GetWidth(), th, c);
            w.DrawRect(r.left, r.top, th, r.GetHeight(), c);
        }
        else {
            int seg = hl.style == DASHED ? DPI(8) : DPI(2);
            int gap = hl.style == DASHED ? DPI(5) : DPI(4);
            int ex = r.left;
            while(ex < r.right) {
                int run = min(seg, r.right - ex);
                w.DrawRect(ex, r.top, run, th, c);
                ex += seg + gap;
            }
            int ey = r.top;
            while(ey < r.bottom) {
                int run = min(seg, r.bottom - ey);
                w.DrawRect(r.left, ey, th, run, c);
                ey += seg + gap;
            }
        }
    };

    Rect r = UiStyledSurfaceRect(outer, metrics);
    PaintShadowStack(metrics.shadow, false);

    bool          skin_drawn = false;
    StyledMetrics mm         = metrics;

    if(skin.enabled && !IsNull(skin.base)) {
        // slice is THICKNESS; never test with Rect::IsEmpty()
        if(UiIsZeroThicknessRect(skin.slice))
            w.DrawImage(r, skin.base);
        else
            UiDraw9Slice(w, r, skin.base, skin.slice);

         mm.face_enabled = false;
    }

    UiPaintFaceFrameDash(w, r, palette, mm, st);
    PaintShadowStack(metrics.shadow, true);
    PaintHighlight(metrics.highlight);
}

inline void UiPaintStyledForeground(Draw& w,
                                    const Rect& outer,
                                    const StyledPalette& palette,
                                    const StyledMetrics& metrics,
                                    const StyledSkin& skin,
                                    StyledState st,
                                    bool has_focus)
{
    if(!has_focus || !metrics.focus_enabled)
        return;

    Rect face = UiStyledFaceRect(outer, metrics, skin);
    if(face.IsEmpty())
        return;

        Color focus_color = metrics.focus_color;
    if(IsNull(focus_color))
        focus_color = SColorHighlight();

    UiPaintFocusShape(w,
                      face,
                      metrics,
                      st,
                      focus_color,
                      metrics.focus_margin,
                      0,
                      metrics.focus_alpha,
                      0,
                      0.0);
}

// -------------------------------------------------------------------------
// Unified surface paint contract (for panel-like controls)
// -------------------------------------------------------------------------
// Intended call pattern inside control Paint():
//  1) Run WhenPaintBackground hook first (if present), then mark bg_handled.
//  2) Paint control-specific content (text/media/custom overlays).
//  3) Run WhenPaintForeground hook (if present), then mark fg_handled.
//  4) Call UiPaintStyledSurface(...) once as the default fallback policy.
//
// UiPaintStyledSurface behavior:
//  - If !background_handled: paints default styled background.
//  - If !foreground_handled: paints default styled foreground
//    (focus ring only; still gated by metrics.focus_enabled and has_focus).
//
// This keeps hook precedence consistent across controls:
//  user hooks always win, defaults fill only missing layers.
inline void UiPaintStyledSurface(Draw& w,
                                 const Rect& outer,
                                 const StyledPalette& palette,
                                 const StyledMetrics& metrics,
                                 const StyledSkin& skin,
                                 StyledState st,
                                 bool has_focus,
                                 bool background_handled,
                                 bool foreground_handled)
{
    if(outer.IsEmpty())
        return;

    if(!background_handled)
        UiPaintStyledBackground(w, outer, palette, metrics, skin, st, has_focus);

    if(!foreground_handled)
        UiPaintStyledForeground(w, outer, palette, metrics, skin, st, has_focus);
}

inline void UiPaintFaceFrameDashAlpha(Draw& w, const Rect& outer,
                                      const StyledPalette& palette,
                                      const StyledMetrics& m,
                                      StyledState st,
                                      int alpha)
{
    alpha = clamp(alpha, 0, 255);
    if(alpha >= 255) {
        UiPaintFaceFrameDash(w, outer, palette, m, st);
        return;
    }
    if(alpha <= 0)
        return;

    if(!m.face_enabled && !m.frame_enabled)
        return;

    const int fw     = max(m.frame_width, 0);
    const int radius = max(m.radius, 0);

    const UiFill& ff = palette.face[st];

    Size sz = outer.GetSize();
    if(sz.cx <= 0 || sz.cy <= 0)
        return;

    ImageBuffer ib(sz);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    {
        BufferPainter p(ib, MODE_ANTIALIASED);

        double inset = fw > 0 ? max(0.5, fw * 0.5) : 0.5;
        double x     = inset;
        double y     = inset;
        double wdt   = sz.cx - 2 * inset;
        double hgt   = sz.cy - 2 * inset;

        int    max_r = min(sz.cx, sz.cy) / 2;
        double rad   = (double)min(radius, max_r);

        p.Begin();
        p.RoundedRectangle(x, y, wdt, hgt, rad);

        if(m.face_enabled && !ff.IsNone()) {
            if(ff.IsSolid()) {
                p.Fill(alpha * ff.color);
            }
            else if(ff.IsImage() && !IsNull(ff.image)) {
                Image img = UiImageMultiplyAlpha(ff.image, alpha);
                Size isz = img.GetSize();
                if(isz.cx > 0 && isz.cy > 0) {
                    double sx = wdt / isz.cx;
                    double sy = hgt / isz.cy;

                    Xform2D xf = Xform2D::Scale(sx, sy)
                               * Xform2D::Translation(x, y);

                    p.Fill(img, xf, FILL_FAST);
                }
            }
        }

        if(m.frame_enabled && fw > 0 && !IsNull(palette.frame[st])) {
            if(m.dashed && !m.dash_pattern.IsEmpty())
                p.Dash(m.dash_pattern, 0.0);
            p.Stroke(fw, alpha * palette.frame[st]);
        }
        p.End();
    }

    w.DrawImage(outer.left, outer.top, ib);
}

// -------------------------------------------------------------------------
// Shared styled text helpers (multiline)
// -------------------------------------------------------------------------
inline int UiStyledTextLineGap()
{
    return DPI(2);
}

inline void UiBuildStyledTextLines(const String& text,
                                   const Font&   font,
                                   Vector<String>& out_lines,
                                   Vector<Size>&   out_sizes)
{
    out_lines.Clear();
    out_sizes.Clear();

    if(text.IsEmpty())
        return;

    const int len   = text.GetCount();
    int       start = 0;

    for(int i = 0; i <= len; i++) {
        if(i == len || text[i] == '\n') {
            String line = text.Mid(start, i - start);
            out_lines.Add(line);

            // Empty lines contribute height (font) but do not inflate width.
            Size sz = line.IsEmpty()
                      ? Size(0, GetTextSize(" ", font).cy)
                      : GetTextSize(line, font);
            out_sizes.Add(sz);

            start = i + 1;
        }
    }
}

inline Size UiMeasureStyledTextBlock(const Vector<Size>& line_sizes)
{
    if(line_sizes.IsEmpty())
        return Size(0, 0);

    int gap   = UiStyledTextLineGap();
    int max_w = 0;
    int sum_h = 0;

    for(const Size& s : line_sizes) {
        max_w = max(max_w, s.cx);
        sum_h += s.cy;
    }

    if(line_sizes.GetCount() > 1)
        sum_h += gap * (line_sizes.GetCount() - 1);

    return Size(max_w, sum_h);
}

inline void UiPaintStyledText(Draw& w,
                              const Rect& area,
                              const Vector<String>& lines,
                              const Vector<Size>&   line_sizes,
                              UiAlign align_h,
                              UiAlign align_v,
                              const Font& f,
                              Color ink,
                              wchar accesskey,
                              bool underline = false,
                              int  underline_width  = DPI(1),
                              int  underline_offset = 0)
{
    if(area.IsEmpty() || lines.IsEmpty())
        return;

    ASSERT(lines.GetCount() == line_sizes.GetCount());



    const int LINE_GAP = UiStyledTextLineGap();
    const int count = lines.GetCount();

    int total_h = 0;
    for(int i = 0; i < count; i++)
        total_h += line_sizes[i].cy;
    if(count > 1)
        total_h += LINE_GAP * (count - 1);

    int start_y;
    switch(align_v) {
    case UiAlign::BOTTOM:
        start_y = area.bottom - total_h;
        break;
    case UiAlign::CENTER:
        start_y = area.top + (area.GetHeight() - total_h) / 2;
        break;
    case UiAlign::TOP:
    default:
        start_y = area.top;
        break;
    }

    int y = start_y;

    // IMPORTANT:
    // - Use Ctrl::AccessKeyBit (real U++ API per Ctrl.cpp), not AccessKeyBit.
    // - Only pass a nonzero mask if the text actually contains '&' mnemonic markup.
    dword ak = 0;
    if(accesskey) {
        int c = ToUpper((int)accesskey);
        ak = Ctrl::AccessKeyBit(c);
    }

    int underline_baseline = y;

    for(int i = 0; i < count; i++) {
        const String& line = lines[i];
        const Size&   sz   = line_sizes[i];

        int line_x;
        switch(align_h) {
        case UiAlign::CENTER:
            line_x = area.left + (area.GetWidth() - sz.cx) / 2;
            break;
        case UiAlign::RIGHT:
            line_x = area.right - sz.cx;
            break;
        case UiAlign::LEFT:
        default:
            line_x = area.left;
            break;
        }

        int max_w = area.right - line_x;
        if(max_w > 0) {
            if(ak) {
                DrawSmartText(w,
                              line_x,
                              y,
                              max_w,
                              line,
                              f,
                              ink,
                              ak);
            }
            else {
                w.DrawText(line_x, y, line, f, ink);
            }

            underline_baseline = y + sz.cy;
        }

        // Access key only on the first non-empty line.
        if(ak && !line.IsEmpty())
            ak = 0;

        y += sz.cy;
        if(i + 1 < count)
            y += LINE_GAP;
    }

    if(underline && area.right > area.left) {
        int uw = max(underline_width, DPI(1));
        int ul_y = underline_baseline + underline_offset;

        w.DrawRect(area.left,
                   ul_y,
                   area.GetWidth(),
                   uw,
                   ink);
    }
}

// -------------------------------------------------------------------------
// Flash/Overlay helper
// -------------------------------------------------------------------------
inline void UiPaintFlash(Draw& w, const Rect& outer, int radius, Color color, int alpha)
{
    if(alpha <= 0 || IsNull(color))
        return;

    alpha  = clamp(alpha, 0, 255);
    radius = max(radius, 0);

    if(radius <= 0) {
        ImageBuffer ib(1, 1);
        ib.SetKind(IMAGE_ALPHA);
        ib[0][0] = alpha * color;
        w.DrawImage(outer, ib);
        return;
    }

    Size sz = outer.GetSize();
    if(sz.cx <= 0 || sz.cy <= 0)
        return;

    ImageBuffer ib(sz);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    {
        BufferPainter p(ib, MODE_ANTIALIASED);

        double inset = 0.5;
        double x     = inset;
        double y     = inset;
        double wdt   = sz.cx - 2 * inset;
        double hgt   = sz.cy - 2 * inset;

        int max_r = min(sz.cx, sz.cy) / 2;
        int rad   = min(radius, max_r);

        p.Begin();
        p.RoundedRectangle(x, y, wdt, hgt, rad);
        p.Fill(alpha * color);
        p.End();
    }

    w.DrawImage(outer.left, outer.top, ib);
}

// Later V1 audit pass:
// Recheck cached-vs-hot usage for UiPaintFlash, UiPaintIndicatorRadioDot,
// UiPaintFaceFrameDashAlpha, and rounded popup compositor helpers. They are
// acceptable only when their raster work is cached or clearly off the steady
// state Paint() hot path.

inline void UiPaintFlash(Draw& w, const Rect& outer, const StyledMetrics& m,
                         Color color, int alpha)
{
    UiPaintFlash(w, outer, max(m.radius, 0), color, alpha);
}

// -------------------------------------------------------------------------
// Indicator glyph helpers
// -------------------------------------------------------------------------

inline bool UiPaintCenteredScaledImage(Draw& w, const Rect& outer, const Image& img,
                                       int inset_x = DPI(3), int inset_y = DPI(3))
{
    if(outer.IsEmpty() || IsNull(img))
        return false;

    Rect rr = outer.Deflated(max(0, inset_x), max(0, inset_y));
    if(rr.GetWidth() <= 0 || rr.GetHeight() <= 0)
        return false;

    Size isz = img.GetSize();
    if(isz.cx <= 0 || isz.cy <= 0)
        return false;

    double sx = (double)rr.GetWidth() / isz.cx;
    double sy = (double)rr.GetHeight() / isz.cy;
    double s = min(sx, sy);
    int dw = max(1, (int)floor(isz.cx * s + 0.5));
    int dh = max(1, (int)floor(isz.cy * s + 0.5));
    Image scaled = CachedRescale(img, Size(dw, dh));
    int x = rr.left + (rr.GetWidth() - dw) / 2;
    int y = rr.top + (rr.GetHeight() - dh) / 2;
    w.DrawImage(x, y, scaled);
    return true;
}

inline void UiPaintIndicatorBar(Draw& w, const Rect& outer, Color ink, int thickness,
                                int horizontal_inset)
{
    if(outer.IsEmpty())
        return;
    int t = max(1, thickness);
    int cx = max(1, outer.GetWidth() - horizontal_inset * 2);
    int y = outer.top + outer.GetHeight() / 2;
    w.DrawRect(outer.left + horizontal_inset, y, cx, t, ink);
}

inline void UiPaintIndicatorCheckStroke(Draw& w, const Rect& outer, Color ink, int thickness,
                                        int left_inset, int mid_y_offset,
                                        int bottom_inset, int right_inset, int top_inset)
{
    if(outer.IsEmpty())
        return;
    int t = max(1, thickness);
    int cx = outer.left + outer.GetWidth() / 2 - DPI(1);
    int cy = outer.top + outer.GetHeight() / 2 + mid_y_offset;
    w.DrawLine(outer.left + left_inset, cy, cx, outer.bottom - bottom_inset, t, ink);
    w.DrawLine(cx, outer.bottom - bottom_inset, outer.right - right_inset, outer.top + top_inset, t, ink);
}

inline void UiPaintIndicatorRadioDot(Draw& w, const Rect& outer, Color ink,
                                     int inset, int radius_percent = 100, int min_side = DPI(8))
{
    Rect mark_area = outer.Deflated(max(0, inset), max(0, inset));
    if(mark_area.IsEmpty())
        return;

    int mark_side = min(mark_area.GetWidth(), mark_area.GetHeight());
    int dot = max(min_side, (mark_side * 76) / 100);
    dot = min(dot, mark_side);

    double cx = mark_area.left + mark_area.GetWidth() * 0.5;
    double cy = mark_area.top + mark_area.GetHeight() * 0.5;
    double x = cx - dot * 0.5;
    double y = cy - dot * 0.5;

    ImageBuffer ib(max(1, dot + 4), max(1, dot + 4));
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    BufferPainter p(ib, MODE_ANTIALIASED);
    p.Clear(RGBAZero());
    if(radius_percent >= 95) {
        p.Circle(ib.GetWidth() * 0.5, ib.GetHeight() * 0.5, dot * 0.5);
    }
    else {
        double rr = max(0.0, dot * clamp(radius_percent, 0, 100) / 200.0);
        p.RoundedRectangle((ib.GetWidth() - dot) * 0.5,
                           (ib.GetHeight() - dot) * 0.5,
                           dot,
                           dot,
                           rr);
    }
    p.Fill(ink);

    int dx = fround(x) - 2;
    int dy = fround(y) - 2;
    w.DrawImage(dx, dy, Image(ib));
}

inline void UiPaintCapsule(Draw& w, const Rect& r, Color fill)
{
    if(r.IsEmpty() || IsNull(fill))
        return;

    if(r.GetWidth() <= 2 || r.GetHeight() <= 2) {
        w.DrawRect(r, fill);
        return;
    }

    if(r.GetWidth() == r.GetHeight()) {
        w.DrawEllipse(r, fill);
        return;
    }

    if(r.GetWidth() > r.GetHeight()) {
        int cap = r.GetHeight();
        int body_w = max(0, r.GetWidth() - cap);
        if(body_w > 0)
            w.DrawRect(r.left + cap / 2, r.top, body_w, r.GetHeight(), fill);
        w.DrawEllipse(RectC(r.left, r.top, cap, cap), fill);
        w.DrawEllipse(RectC(r.right - cap, r.top, cap, cap), fill);
        return;
    }

    int cap = r.GetWidth();
    int body_h = max(0, r.GetHeight() - cap);
    if(body_h > 0)
        w.DrawRect(r.left, r.top + cap / 2, r.GetWidth(), body_h, fill);
    w.DrawEllipse(RectC(r.left, r.top, cap, cap), fill);
    w.DrawEllipse(RectC(r.left, r.bottom - cap, cap, cap), fill);
}

inline Image UiGetCachedAACircleImage(Size sz, Color fill)
{
    sz.cx = max(1, sz.cx);
    sz.cy = max(1, sz.cy);
    String key = Format("aa-circle|%d|%d|%d|%d|%d|%d",
                        sz.cx, sz.cy, fill.GetR(), fill.GetG(), fill.GetB(), IsNull(fill) ? 1 : 0);
    return UiGetCachedImage(key, [=] {
        ImageBuffer ib(sz);
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());
        if(IsNull(fill))
            return Image(ib);

        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.Ellipse(0.5, 0.5, max(1.0, sz.cx - 1.0), max(1.0, sz.cy - 1.0));
        p.Fill(fill);
        p.End();
        return Image(ib);
    });
}

inline Image UiGetCachedAACapsuleImage(Size sz, Color fill)
{
    sz.cx = max(1, sz.cx);
    sz.cy = max(1, sz.cy);
    String key = Format("aa-capsule|%d|%d|%d|%d|%d|%d",
                        sz.cx, sz.cy, fill.GetR(), fill.GetG(), fill.GetB(), IsNull(fill) ? 1 : 0);
    return UiGetCachedImage(key, [=] {
        ImageBuffer ib(sz);
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());
        if(IsNull(fill))
            return Image(ib);

        int radius = max(0, min(sz.cx, sz.cy) / 2);
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        if(radius > 0)
            p.RoundedRectangle(0.5, 0.5, max(1.0, sz.cx - 1.0), max(1.0, sz.cy - 1.0), radius);
        else
            p.Rectangle(0.5, 0.5, max(1.0, sz.cx - 1.0), max(1.0, sz.cy - 1.0));
        p.Fill(fill);
        p.End();
        return Image(ib);
    });
}

inline Image UiGetCachedAARingImage(Size sz,
                                    Color frame,
                                    Color ring,
                                    Color face,
                                    int frame_width,
                                    int ring_width)
{
    sz.cx = max(1, sz.cx);
    sz.cy = max(1, sz.cy);
    frame_width = max(0, frame_width);
    ring_width = max(0, ring_width);
    String key = Format("aa-ring|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d",
                        sz.cx, sz.cy,
                        frame.GetR(), frame.GetG(), frame.GetB(), IsNull(frame) ? 1 : 0,
                        ring.GetR(), ring.GetG(), ring.GetB(), IsNull(ring) ? 1 : 0,
                        face.GetR(), face.GetG(), face.GetB(), IsNull(face) ? 1 : 0,
                        frame_width, ring_width);
    return UiGetCachedImage(key, [=] {
        ImageBuffer ib(sz);
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());

        BufferPainter p(ib, MODE_ANTIALIASED);
        double cx = sz.cx * 0.5;
        double cy = sz.cy * 0.5;
        double radius = max(0.5, min(sz.cx, sz.cy) * 0.5 - 0.5);
        if(!IsNull(frame)) {
            p.Begin();
            p.Circle(cx, cy, radius);
            p.Fill(frame);
            p.End();
        }

        double ring_radius = radius - max(0, frame_width);
        if(ring_radius > 0.0 && !IsNull(ring)) {
            p.Begin();
            p.Circle(cx, cy, ring_radius);
            p.Fill(ring);
            p.End();
        }

        double core_radius = ring_radius - max(0, ring_width);
        if(core_radius > 0.0 && !IsNull(face)) {
            p.Begin();
            p.Circle(cx, cy, core_radius);
            p.Fill(face);
            p.End();
        }

        return Image(ib);
    });
}

inline Image UiGetCachedAARoundedRectImage(Size sz,
                                           int radius,
                                           Color face,
                                           Color frame,
                                           int frame_width)
{
    sz.cx = max(1, sz.cx);
    sz.cy = max(1, sz.cy);
    radius = max(0, min(radius, min(sz.cx, sz.cy) / 2));
    frame_width = max(0, frame_width);
    String key = Format("aa-rounded|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d",
                        sz.cx, sz.cy, radius, frame_width,
                        face.GetR(), face.GetG(), face.GetB(), IsNull(face) ? 1 : 0,
                        frame.GetR(), frame.GetG(), frame.GetB(), IsNull(frame) ? 1 : 0);
    return UiGetCachedImage(key, [=] {
        ImageBuffer ib(sz);
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());

        BufferPainter p(ib, MODE_ANTIALIASED);
        double fw = frame_width > 0 ? max(1.0, (double)frame_width) : 0.0;
        double inset = fw > 0 ? fw * 0.5 : 0.5;
        double x = inset;
        double y = inset;
        double cx = max(1.0, sz.cx - inset * 2);
        double cy = max(1.0, sz.cy - inset * 2);
        double rr = max(0.0, (double)radius - (fw > 0 ? fw * 0.5 : 0.0));

        p.Begin();
        if(radius > 0)
            p.RoundedRectangle(x, y, cx, cy, rr);
        else
            p.Rectangle(x, y, cx, cy);
        if(!IsNull(face))
            p.Fill(face);
        if(frame_width > 0 && !IsNull(frame))
            p.Stroke(fw, frame);
        p.End();
        return Image(ib);
    });
}

inline Image UiGetCachedRoundedBadgeImage(Size sz, int radius, Color face,
                                          int stroke_width = 1, Color stroke = Null)
{
    return UiGetCachedAARoundedRectImage(sz, radius, face, stroke, stroke_width);
}

// -------------------------------------------------------------------------
// Rounded popup compositor (single-call internal draw path)
// -------------------------------------------------------------------------
inline void UiPaintRoundedPopupComposited(Draw& w,
                                          const Rect& outer,
                                          const Image& src,
                                          int radius,
                                          Color matte,
                                          int frame_width = 0,
                                          Color frame_color = Null)
{
    if(outer.IsEmpty() || IsNull(src))
        return;

    Size sz = src.GetSize();
    if(sz.cx <= 0 || sz.cy <= 0)
        return;

    radius = max(0, min(radius, min(sz.cx, sz.cy) / 2));

    if(radius <= 0) {
        w.DrawImage(outer.left, outer.top, src);
    }
    else {
        ImageBuffer out(sz);
        ImageBuffer mask(sz);
        mask.SetKind(IMAGE_ALPHA);
        Fill(~mask, RGBAZero(), mask.GetLength());

        {
            BufferPainter p(mask, MODE_ANTIALIASED);
            p.Begin();
            p.RoundedRectangle(0.5, 0.5, sz.cx - 1.0, sz.cy - 1.0, radius);
            p.Fill(White());
            p.End();
        }

        if(IsNull(matte))
            matte = SColorPaper();

        const RGBA* sp = ~src;
        RGBA* dp = ~out;
        RGBA* mp = ~mask;
        int n = out.GetLength();
        for(int i = 0; i < n; i++) {
            int a = int(mp[i].a);
            dp[i].r = (byte)((int(sp[i].r) * a + int(matte.GetR()) * (255 - a) + 127) / 255);
            dp[i].g = (byte)((int(sp[i].g) * a + int(matte.GetG()) * (255 - a) + 127) / 255);
            dp[i].b = (byte)((int(sp[i].b) * a + int(matte.GetB()) * (255 - a) + 127) / 255);
            dp[i].a = 255;
        }

        w.DrawImage(outer.left, outer.top, Image(out));
    }

    if(frame_width > 0 && !IsNull(frame_color)) {
        ImageBuffer fb(sz);
        fb.SetKind(IMAGE_ALPHA);
        Fill(~fb, RGBAZero(), fb.GetLength());

        BufferPainter p(fb, MODE_ANTIALIASED);
        double fw = max(1, frame_width);
        double x = fw * 0.5;
        double y = fw * 0.5;
        double cx = max(1.0, sz.cx - fw);
        double cy = max(1.0, sz.cy - fw);
        double rad = max(0.0, (double)radius - fw * 0.5);

        p.Begin();
        if(radius > 0)
            p.RoundedRectangle(x, y, cx, cy, rad);
        else
            p.Rectangle(x, y, cx, cy);
        p.Stroke(fw, frame_color);
        p.End();

        w.DrawImage(outer.left, outer.top, Image(fb));
    }
}

// -------------------------------------------------------------------------
// Inline icon factory (RAW + RLE)
// -------------------------------------------------------------------------
inline Image UiDecodeInlineIconRaw(const unsigned char* payload, int w, int h)
{
    if(!payload || w <= 0 || h <= 0)
        return Image();

    ImageBuffer ib(w, h);
    RGBA*       dst = ib;
    const unsigned char* src = payload;

    int total = w * h;
    for(int i = 0; i < total; i++) {
        RGBA& px = dst[i];
        px.r = *src++;
        px.g = *src++;
        px.b = *src++;
        px.a = *src++;
    }

    return Image(ib);
}

inline Image UiDecodeInlineIconRle(const unsigned char* payload, int w, int h)
{
    if(!payload || w <= 0 || h <= 0)
        return Image();

    ImageBuffer ib(w, h);
    RGBA*       dst = ib;

    const unsigned char* src = payload;
    int total   = w * h;
    int written = 0;

    // Current UiIcons.h payload format:
    //   [uint16 run_len][uint8 r][uint8 g][uint8 b][uint8 a] ...
    while(written < total) {
        int count = (int)src[0] | ((int)src[1] << 8);
        src += 2;

        RGBA px;
        px.r = *src++;
        px.g = *src++;
        px.b = *src++;
        px.a = *src++;

        int run = min(max(0, count), total - written);
        for(int i = 0; i < run; i++)
            dst[written++] = px;

        if(count <= 0)
            break;
    }

    ASSERT(written == total);

    return Image(ib);
}

inline Image UiMakeIcon(const unsigned char* data)
{
    if(!data)
        return Image();

    unsigned int w_raw = (unsigned int)data[0] | ((unsigned int)data[1] << 8);
    unsigned int h     = (unsigned int)data[2] | ((unsigned int)data[3] << 8);

    bool is_rle = (w_raw & 0x8000u) != 0;
    int  w      = (int)(w_raw & 0x7FFFu);

    if(w <= 0 || (int)h <= 0)
        return Image();

    const unsigned char* payload = data + 4;

    return UiGetCachedImage(data, [=] {
        return is_rle
            ? UiDecodeInlineIconRle(payload, w, (int)h)
            : UiDecodeInlineIconRaw(payload, w, (int)h);
    }, 256);
}

inline Image UiMakeIcon(const void* data)
{
    return UiMakeIcon((const unsigned char*)data);
}

} // namespace Upp

#endif

