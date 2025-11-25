#ifndef _Ui_UiDraw_h_
#define _Ui_UiDraw_h_

#include <Ui/UiStyle.h>
#include <Painter/Painter.h>

namespace Upp {


// -------------------------------------------------------------------------
// fast blur helper code to assist in styles
// -------------------------------------------------------------------------
inline void FastBlur(ImageBuffer& ib, int radius)
{
    if(radius < 1) return;
    Size sz = ib.GetSize();
    int w = sz.cx;
    int h = sz.cy;
    
    // Helper to clamp coordinates
    auto clamp = [](int val, int max_val) { return min(max(val, 0), max_val - 1); };

    // We need a temporary buffer for the two-pass algorithm
    Buffer<RGBA> temp(w * h);
    RGBA* src = ib;
    RGBA* dst = temp;

    // 1. Horizontal Pass
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            int r = 0, g = 0, b = 0, a = 0;
            int count = 0;
            
            // Simple kernel loop (fast enough for small UI elements)
            for(int k = -radius; k <= radius; k++) {
                int px = clamp(x + k, w);
                const RGBA& p = src[y * w + px];
                r += p.r; g += p.g; b += p.b; a += p.a;
                count++;
            }
            
            RGBA& d = dst[y * w + x];
            d.r = r / count; d.g = g / count; d.b = b / count; d.a = a / count;
        }
    }

    // 2. Vertical Pass (Write back to ib)
    for(int x = 0; x < w; x++) {
        for(int y = 0; y < h; y++) {
            int r = 0, g = 0, b = 0, a = 0;
            int count = 0;

            for(int k = -radius; k <= radius; k++) {
                int py = clamp(y + k, h);
                const RGBA& p = dst[py * w + x];
                r += p.r; g += p.g; b += p.b; a += p.a;
                count++;
            }

            RGBA& d = src[y * w + x]; // Write back to original buffer
            d.r = r / count; d.g = g / count; d.b = b / count; d.a = a / count;
        }
    }
}



// Paints the styled "box" for a control:
// - Fills Face (if metrics.face_enabled)
// - Draws Frame (if metrics.frame_enabled && frame_width > 0)
// - Respects radius (rounded corners) and dashed frame (dash_pattern)
// - Uses a fast rectangular path when possible, and BufferPainter for AA
inline void UiPaintFaceFrameDash(Draw& w,
                                 const Rect& outer,
                                 const StyledPalette& palette,
                                 const StyledMetrics& m,
                                 StyledState st)
{
    if(!m.face_enabled && !m.frame_enabled)
        return;

    const int fw     = max(m.frame_width, 0);
    const int radius = max(m.radius, 0);

    // ---------------------------------------------------------------------
    // 1) Fast path: rectangular, no dash, no AA
    // ---------------------------------------------------------------------
    if(radius <= 0 && !m.dashed) {
        if(m.face_enabled)
            w.DrawRect(outer, palette.face[st]);

        if(m.frame_enabled && fw > 0) {
            Rect  fr = outer;
            Color fc = palette.frame[st];

            // top
            w.DrawRect(fr.left, fr.top, fr.GetWidth(), fw, fc);
            // bottom
            w.DrawRect(fr.left, fr.bottom - fw, fr.GetWidth(), fw, fc);
            // left
            w.DrawRect(fr.left, fr.top, fw, fr.GetHeight(), fc);
            // right
            w.DrawRect(fr.right - fw, fr.top, fw, fr.GetHeight(), fc);
        }
        return;
    }

    // ---------------------------------------------------------------------
    // 2) AA rounded/dashed path via BufferPainter
    // ---------------------------------------------------------------------
    Size sz = outer.GetSize();
    if(sz.cx <= 0 || sz.cy <= 0)
        return;

    ImageBuffer ib(sz);
    Fill(~ib, RGBAZero(), ib.GetLength());

    {
        BufferPainter p(ib, MODE_ANTIALIASED);

        // Inset so that the stroke is centered inside the rect
        double inset = fw > 0 ? max(0.5, fw * 0.5) : 0.5;
        double x     = inset;
        double y     = inset;
        double wdt   = sz.cx - 2 * inset;
        double hgt   = sz.cy - 2 * inset;

        int    max_r = min(sz.cx, sz.cy) / 2;
        double rad   = (double)min(radius, max_r);

        p.Begin();
        p.RoundedRectangle(x, y, wdt, hgt, rad);

        if(m.face_enabled)
            p.Fill(palette.face[st]);

        if(m.frame_enabled && fw > 0) {
            if(m.dashed && !m.dash_pattern.IsEmpty())
                p.Dash(m.dash_pattern, 0.0);  // uses Painter dash grammar
            p.Stroke(fw, palette.frame[st]);
        }
        p.End();
    }

    w.DrawImage(outer.left, outer.top, ib);
}


// -------------------------------------------------------------------------
// Focus ring helper: rounded / dashed, matched to StyledMetrics.radius
// -------------------------------------------------------------------------
inline void UiPaintFocusRing(Draw& w,
                             const Rect& outer,
                             const StyledPalette& palette,
                             const StyledMetrics& m,
                             StyledState st,
                             int focus_margin,
                             Color override_color = Null)
{
    if(focus_margin <= 0)
        return;

    Rect r = outer;
    r.Deflate(focus_margin, focus_margin);

    const int radius = max(m.radius, 0);

    // Choose color: explicit override or state-based ink.
    Color color = IsNull(override_color) ? palette.ink[st] : override_color;

    // Fast path: rectangular, no dash, no AA
    if(radius <= 0 && !m.dashed) {
        // top
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, color);
        // bottom
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, color);
        // left
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), color);
        // right
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), color);
        return;
    }

    // AA rounded / dashed focus path via BufferPainter
    Size sz = r.GetSize();
    if(sz.cx <= 0 || sz.cy <= 0)
        return;

    ImageBuffer ib(sz);
    Fill(~ib, RGBAZero(), ib.GetLength());

    {
        BufferPainter p(ib, MODE_ANTIALIASED);

        double inset = 0.5;
        double x     = inset;
        double y     = inset;
        double wdt   = sz.cx - 2 * inset;
        double hgt   = sz.cy - 2 * inset;

        int    max_r = min(sz.cx, sz.cy) / 2;
        double rad   = (double)min(radius, max_r);

        // Thickness: normal < hot < pressed
        double stroke_w =
            (st == ST_PRESSED ? 2.0 :
             st == ST_HOT     ? 1.5 :
                                1.0);

        p.Begin();
        p.RoundedRectangle(x, y, wdt, hgt, rad);
        if(m.dashed && !m.dash_pattern.IsEmpty())
            p.Dash(m.dash_pattern, 0.0);
        p.Stroke(stroke_w, color);
        p.End();
    }

    w.DrawImage(r.left, r.top, ib);
}



} // namespace Upp

#endif
