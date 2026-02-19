#ifndef _Ui_UiDraw_h_
#define _Ui_UiDraw_h_

#include <Painter/Painter.h>
//#include <CtrlCore/CtrlCore.h>   // AccessKeyBit
#include <CtrlLib/CtrlLib.h>     // DrawSmartText, DisabledImage, DPI, etc.
#include <Ui/UiStyle.h>

namespace Upp {

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
                              bool mono,
                              Color ink,
                              bool enabled)
{
    if(area.IsEmpty() || IsNull(src))
        return;

    Image img = src;

    // Disabled handling: only auto-gray if we are not in mono-tint mode.
    if(!enabled && !mono)
        img = DisabledImage(img);

    Size src_sz = img.GetSize();
    if(src_sz.cx <= 0 || src_sz.cy <= 0)
        return;

    int dst_w = src_sz.cx;
    int dst_h = src_sz.cy;

    if(scale) {
        double sx = (double)area.GetWidth()  / src_sz.cx;
        double sy = (double)area.GetHeight() / src_sz.cy;
        double s  = min(sx, sy);

        if(s > 0) {
            dst_w = max(1, int(src_sz.cx * s + 0.5));
            dst_h = max(1, int(src_sz.cy * s + 0.5));
        }
    }

    int img_x = area.left + (area.GetWidth()  - dst_w) / 2;
    int img_y = area.top  + (area.GetHeight() - dst_h) / 2;

    Image draw_img = img;
    if(scale && (dst_w != src_sz.cx || dst_h != src_sz.cy))
        draw_img = CachedRescale(img, Size(dst_w, dst_h));

    if(mono && enabled)
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
inline void UiPaintFocusRing(Draw& w, const Rect& outer, const StyledPalette& palette,
                             const StyledMetrics& m, StyledState st, int focus_margin,
                             Color override_color = Null)
{
    if(focus_margin <= 0)
        return;

    Rect r = outer;
    r.Deflate(focus_margin, focus_margin);

    const int radius = max(m.radius, 0);

    Color color = IsNull(override_color) ? palette.ink[st] : override_color;

    if(radius <= 0 && !m.dashed) {
        w.DrawRect(r.left,         r.top,          r.GetWidth(), 1, color);
        w.DrawRect(r.left,         r.bottom - 1,   r.GetWidth(), 1, color);
        w.DrawRect(r.left,         r.top,          1,            r.GetHeight(), color);
        w.DrawRect(r.right - 1,    r.top,          1,            r.GetHeight(), color);
        return;
    }

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

        double stroke_w =
            (st == ST_PRESSED ? 2.0 :
             st == ST_HOT     ? 1.5 : 1.0);

        p.Begin();
        p.RoundedRectangle(x, y, wdt, hgt, rad);
        if(m.dashed && !m.dash_pattern.IsEmpty())
            p.Dash(m.dash_pattern, 0.0);
        p.Stroke(stroke_w, color);
        p.End();
    }

    w.DrawImage(r.left, r.top, ib);
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

    auto PaintSoftShadow = [&](const StyledShadow& sh) {
        if(!sh.enabled || sh.alpha <= 0)
            return;

        int blur = max(0, sh.blur);
        int spread = max(0, sh.spread);
        int pad = blur + spread + max(abs(sh.offset_x), abs(sh.offset_y)) + 2;

        Size osz = outer.GetSize();
        Size sz(osz.cx + pad * 2, osz.cy + pad * 2);
        if(sz.cx <= 0 || sz.cy <= 0)
            return;

        ImageBuffer ib(sz);
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());

        {
            BufferPainter p(ib, MODE_ANTIALIASED);
            int rr = max(0, metrics.radius + spread);
            int x = pad + sh.offset_x - spread;
            int y = pad + sh.offset_y - spread;
            int cx = osz.cx + spread * 2;
            int cy = osz.cy + spread * 2;

            p.Begin();
            if(rr > 0)
                p.RoundedRectangle(x, y, cx, cy, rr);
            else
                p.Rectangle(x, y, cx, cy);

            RGBA c = sh.color;
            c.a = (byte)clamp(sh.alpha, 0, 255);
            p.Fill(c);
            p.End();
        }

        if(blur > 0)
            FastBlur(ib, blur);

        w.DrawImage(outer.left - pad, outer.top - pad, Image(ib));
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

    PaintSoftShadow(metrics.shadow);

    Rect r = outer;
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
    PaintHighlight(metrics.highlight);
}

inline void UiPaintStyledForeground(Draw& w,
                                    const Rect& outer,
                                    const StyledPalette& palette,
                                    const StyledMetrics& metrics,
                                    const StyledSkin& skin,
                                    StyledState st,
                                    bool has_focus,
                                    int focus_margin = DPI(1),
                                    Color focus_color = Null)
{
    if(!has_focus)
        return;

    Rect face = UiStyledFaceRect(outer, metrics, skin);
    if(face.IsEmpty())
        return;

    if(IsNull(focus_color))
        focus_color = SColorHighlight();

    UiPaintFocusRing(w, face, palette, metrics, st, focus_margin, focus_color);
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
//  - If !foreground_handled && focus_enabled: paints default styled foreground
//    (focus ring only; still gated by has_focus internally).
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
                                 bool foreground_handled,
                                 bool focus_enabled = true,
                                 int focus_margin = DPI(1),
                                 Color focus_color = Null)
{
    if(outer.IsEmpty())
        return;

    if(!background_handled)
        UiPaintStyledBackground(w, outer, palette, metrics, skin, st, has_focus);

    if(!foreground_handled && focus_enabled)
        UiPaintStyledForeground(w, outer, palette, metrics, skin, st, has_focus, focus_margin, focus_color);
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
            DrawSmartText(w,
                          line_x,
                          y,
                          max_w,
                          line,
                          f,
                          ink,
                          ak);

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

inline void UiPaintFlash(Draw& w, const Rect& outer, const StyledMetrics& m,
                         Color color, int alpha)
{
    UiPaintFlash(w, outer, max(m.radius, 0), color, alpha);
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

    static StaticMutex                   s_mutex;
    static VectorMap<const void*, Image> s_cache;

    Mutex::Lock __(s_mutex);

    int idx = s_cache.Find(data);
    if(idx >= 0)
        return s_cache[idx];

    Image img = is_rle
        ? UiDecodeInlineIconRle(payload, w, (int)h)
        : UiDecodeInlineIconRaw(payload, w, (int)h);

    s_cache.Add(data, img);
    return img;
}

inline Image UiMakeIcon(const void* data)
{
    return UiMakeIcon((const unsigned char*)data);
}

} // namespace Upp

#endif
