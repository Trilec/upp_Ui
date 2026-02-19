#ifndef _Ui_UiStyle_h_
#define _Ui_UiStyle_h_

#include <CtrlCore/CtrlCore.h>
#include <Draw/Draw.h>

namespace Upp {

/*
    UiStyle.h
    =========

    Shared styling primitives for Ui controls.

    Core invariants:
    - StyledSkin splits draw vs geometry semantics:
        * slice         : THICKNESS (l/t/r/b) used ONLY by 9-slice drawing
        * content_inset : THICKNESS (l/t/r/b) used ONLY by geometry/content rect
      NOTE: thickness rects MUST NOT be tested using Rect::IsEmpty().

    - Canonical geometry:
        UiStyledInnerRect(outer, metrics, skin, padding)
        UiStyledOuterSizeFromContent(content, metrics, skin, padding)

      Policy:
        * UiStyledInnerRect allows negative padding (layout-only tool).
        * UiStyledOuterSizeFromContent forbids negative padding (minsize stability).

    - Neutral two-block layout helpers:
        UiMeasureBlocksContent(...)
        UiComputeBlocksLayout(...)

    - UiBlock is minimal: measure + rect storage + payload; NO painting here.
*/

// ---------------------------------------------------------------------------
// Image utilities used by CtrlStyled::SetFaceQuadGradient
// (If you later want them out of style, move to a small UiImageUtil.h included
// by UiStyle.h and UiDraw.h to avoid cyclic includes.)
// ---------------------------------------------------------------------------

inline void FastBlur(ImageBuffer& ib, int radius)
{
    if(radius < 1)
        return;

    Size sz = ib.GetSize();
    int  w  = sz.cx;
    int  h  = sz.cy;

    auto clampi = [=](int val, int max_val) -> int {
        return min(max(val, 0), max_val - 1);
    };

    Buffer<RGBA> temp(w * h);
    RGBA*        src = ib;
    RGBA*        dst = temp;

    // Horizontal pass
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            int r = 0, g = 0, b = 0, a = 0;
            int count = 0;

            for(int k = -radius; k <= radius; k++) {
                int px = clampi(x + k, w);
                const RGBA& p = src[y * w + px];
                r += p.r; g += p.g; b += p.b; a += p.a;
                count++;
            }

            RGBA& d = dst[y * w + x];
            d.r = r / count;
            d.g = g / count;
            d.b = b / count;
            d.a = a / count;
        }
    }

    // Vertical pass
    for(int x = 0; x < w; x++) {
        for(int y = 0; y < h; y++) {
            int r = 0, g = 0, b = 0, a = 0;
            int count = 0;

            for(int k = -radius; k <= radius; k++) {
                int py = clampi(y + k, h);
                const RGBA& p = dst[py * w + x];
                r += p.r; g += p.g; b += p.b; a += p.a;
                count++;
            }

            RGBA& d = src[y * w + x];
            d.r = r / count;
            d.g = g / count;
            d.b = b / count;
            d.a = a / count;
        }
    }
}

inline Image MakeQuadGradientTile(int size,
                                  Color tl, Color tr,
                                  Color bl, Color br,
                                  int blur_radius = 0)
{
    size = max(size, 2);
    ImageBuffer ib(size, size);

    for(int y = 0; y < size; y++) {
        double fy = size <= 1 ? 0.0 : double(y) / double(size - 1);
        Color  left  = Blend(tl, bl, int(fy * 255));
        Color  right = Blend(tr, br, int(fy * 255));

        RGBA* row = ib[y];
        for(int x = 0; x < size; x++) {
            double fx = size <= 1 ? 0.0 : double(x) / double(size - 1);
            Color c = Blend(left, right, int(fx * 255));
            row[x] = c;
            row[x].a = 255;
        }
    }

    if(blur_radius > 0)
        FastBlur(ib, blur_radius);

    return ib;
}

// ---------------------------------------------------------------------------
// Alignment enums  
// ---------------------------------------------------------------------------

enum class UiAlign : byte {
    LEFT   = 0,
    CENTER = 1,
    RIGHT  = 2,
    TOP    = 3,
    BOTTOM = 4,
};

inline Stream& operator%(Stream& s, UiAlign& a)
{
    if(s.IsStoring())
        s % (byte&)a;
    else {
        byte b;
        s % b;
        a = (UiAlign)b;
    }
    return s;
}

enum class UiDirection : byte {
    H = 0,
    V = 1,
};

inline dword GetHashValue(UiDirection d) { return (byte)d; }

inline Stream& operator%(Stream& s, UiDirection& dir)
{
    if(s.IsStoring())
        s % (byte&)dir;
    else {
        byte b;
        s % b;
        dir = (UiDirection)b;
    }
    return s;
}

enum class UiCrossAlign : byte {
    Auto    = 0,
    Stretch = 1,
    Start   = 2,
    Center  = 3,
    End     = 4,
};

enum UiLineStyle : byte {
    SOLID = 0,
    DASHED,
    DOTTED,
};

enum UiSpan : byte {
    NONE = 0,
    SMALL,
    MEDIUM,
    LARGE,
};

inline dword GetHashValue(UiCrossAlign a) { return (byte)a; }

inline Stream& operator%(Stream& s, UiCrossAlign& a)
{
    if(s.IsStoring())
        s % (byte&)a;
    else {
        byte b;
        s % b;
        a = (UiCrossAlign)b;
    }
    return s;
}

// ============================================================================
// DATA TYPES & STATE
// ============================================================================

enum StyledState : int {
    ST_NORMAL   = 0,
    ST_HOT      = 1,
    ST_PRESSED  = 2,
    ST_DISABLED = 3,
};

struct UiFill : Moveable<UiFill> {
    enum Kind : byte {
        NONE  = 0,
        SOLID = 1,
        IMAGE = 2,
    };

    Kind  kind = NONE;
    Color color;
    Image image;

    UiFill() {}
    UiFill(Color c) { *this = c; }

    UiFill& operator=(Color c)
    {
        kind  = SOLID;
        color = c;
        image = Image();
        return *this;
    }

    static UiFill None()
    {
        UiFill f;
        f.kind  = NONE;
        f.color = Null;
        return f;
    }

    static UiFill Solid(Color c) { return UiFill(c); }

    static UiFill ImageFill(const Image& img)
    {
        UiFill f;
        f.kind  = IMAGE;
        f.image = img;
        return f;
    }

    bool IsNone()  const { return kind == NONE; }
    bool IsSolid() const { return kind == SOLID; }
    bool IsImage() const { return kind == IMAGE; }

    void Serialize(Stream& s)
    {
        int k = kind;
        s % k % color % image;
        if(s.IsLoading())
            kind = (Kind)k;
    }
};

struct StyledPalette {
    UiFill face[4];
    Color  frame[4];
    Color  ink[4];

    void Serialize(Stream& s)
    {
        for(int i = 0; i < 4; i++) face[i].Serialize(s);
        for(int i = 0; i < 4; i++) s % frame[i];
        for(int i = 0; i < 4; i++) s % ink[i];
    }
};

struct StyledShadow {
    bool  enabled  = false;
    int   offset_x = 0;
    int   offset_y = DPI(2);
    int   blur     = DPI(6);
    int   spread   = 0;
    int   alpha    = 90;
    Color color    = Black();

    void Serialize(Stream& s)
    {
        s % enabled % offset_x % offset_y % blur % spread % alpha % color;
    }
};

struct StyledHighlight {
    bool  enabled   = false;
    int   thickness = 1;
    int   offset_x  = 0;
    int   offset_y  = 0;
    int   alpha     = 120;
    Color color     = White();
    UiLineStyle style = SOLID;

    void Serialize(Stream& s)
    {
        int st = (int)style;
        s % enabled % thickness % offset_x % offset_y % alpha % color % st;
        style = (UiLineStyle)st;
    }
};

struct StyledMetrics {
    Font text_font      = StdFont();
    bool use_text_font  = false;

    // Geometry-only padding applied inside the skin "face" bounds.
    // Public API: SetPadding(...)
    // This is for density / breathing room (NOT for compensating skin shadows).
    // Thickness-rect semantics (l/t/r/b). Negative values are not allowed.
    Rect content_padding = Rect(0, 0, 0, 0);

    int radius          = DPI(4);
    int frame_width     = DPI(1);

    bool frame_enabled  = true;
    bool face_enabled   = true;

    bool dashed         = false;
    String dash_pattern = "5,5";

    bool high_contrast  = false;

    StyledShadow   shadow;
    StyledHighlight highlight;

    void Serialize(Stream& s)
    {
        s % text_font % use_text_font
          % content_padding
          % radius % frame_width
          % frame_enabled % face_enabled
          % dashed % dash_pattern
          % high_contrast
          % shadow % highlight;
    }
};

// SKIN: slice is THICKNESS for drawing, content_inset is THICKNESS for geometry.
// Public API for content_inset: SetInset(...)
struct StyledSkin {
    Image base;
    Rect slice;
    Rect content_inset;
    bool enabled;

    StyledSkin()
        : slice(0, 0, 0, 0)
        , content_inset(0, 0, 0, 0)
        , enabled(false)
    {}

    void Serialize(Stream& s)
    {
        // If you need backward style-stream compatibility, keep the old order
        // and append content_inset at the end.
        s % base % slice % content_inset % enabled;
    }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

inline Color LtColor(Color base, int pct) { return Blend(base, White(), pct); }
inline Color DkColor(Color base, int pct) { return Blend(base, Black(), pct); }
inline Color DisabledColor(Color base)    { return Blend(base, SColorDisabled(), 50); }

inline StyledState ResolveStyledState(bool enabled, bool hot, bool pressed)
{
    if(!enabled)  return ST_DISABLED;
    if(pressed)   return ST_PRESSED;
    if(hot)       return ST_HOT;
    return ST_NORMAL;
}

// Thickness rect helpers -----------------------------------------------------

inline bool UiIsZeroThicknessRect(const Rect& t)
{
    return t.left == 0 && t.top == 0 && t.right == 0 && t.bottom == 0;
}

inline Rect UiNonNegativeThickness(const Rect& t)
{
    return Rect(max(t.left, 0), max(t.top, 0), max(t.right, 0), max(t.bottom, 0));
}

// Apply thickness to a rect (negative expands, positive shrinks).
inline Rect UiApplyThicknessRect(const Rect& base, const Rect& thickness)
{
    Rect r = base;

    r.left   += thickness.left;
    r.top    += thickness.top;
    r.right  -= thickness.right;
    r.bottom -= thickness.bottom;

    // Clamp collapse to midlines of the *original base* (stable and predictable).
    if(r.right < r.left) {
        int mid = (base.left + base.right) / 2;
        r.left = r.right = mid;
    }
    if(r.bottom < r.top) {
        int mid = (base.top + base.bottom) / 2;
        r.top = r.bottom = mid;
    }

    return r;
}

// Margin helper (same semantics as thickness: negative expands)
inline Rect UiApplyMarginRect(const Rect& base, const Rect& margin)
{
    return UiApplyThicknessRect(base, margin);
}

// Effective frame width for geometry and sizing.
inline int UiResolvedFrameWidth(const StyledMetrics& m, const StyledSkin& skin)
{
    if(!m.frame_enabled)
        return 0;

    if(m.frame_width <= 0)
        return 0;

    return m.frame_width;
}

// ============================================================================
// Canonical geometry helpers
// ============================================================================

inline Rect UiStyledInnerRect(const Rect& outer,
                              const StyledMetrics& m,
                              const StyledSkin& skin,
                              const Rect& padding = Rect(0, 0, 0, 0))
{
    Rect r = outer;
    if(r.IsEmpty())
        return r;

    // 1) frame
    int fw = UiResolvedFrameWidth(m, skin);
    if(fw > 0)
        r.Deflate(fw, fw);

    // 2) skin content inset (face bounds; geometry-only)
    Rect ci = UiNonNegativeThickness(skin.content_inset);
    if(!UiIsZeroThicknessRect(ci))
        r = UiApplyThicknessRect(r, ci);

    // 3) styled content padding (density; geometry-only)
    Rect cp = UiNonNegativeThickness(m.content_padding);
    if(!UiIsZeroThicknessRect(cp))
        r = UiApplyThicknessRect(r, cp);

    // 4) optional extra padding (layout-only; may be negative)
    if(!UiIsZeroThicknessRect(padding))
        r = UiApplyThicknessRect(r, padding);

    return r;
}

// "Face" rect for focus rings and skin-aligned overlays.
// Applies frame + skin.content_inset, but NOT content_padding.
inline Rect UiStyledFaceRect(const Rect& outer,
                             const StyledMetrics& m,
                             const StyledSkin& skin)
{
    Rect r = outer;
    if(r.IsEmpty())
        return r;

    int fw = UiResolvedFrameWidth(m, skin);
    if(fw > 0)
        r.Deflate(fw, fw);

    Rect ci = UiNonNegativeThickness(skin.content_inset);
    if(!UiIsZeroThicknessRect(ci))
        r = UiApplyThicknessRect(r, ci);

    return r;
}

inline Size UiStyledOuterSizeFromContent(Size content,
                                         const StyledMetrics& m,
                                         const StyledSkin& skin,
                                         const Rect& padding = Rect(0, 0, 0, 0))
{
    // Option A policy: forbid negative padding in minsize math.
    ASSERT(padding.left   >= 0);
    ASSERT(padding.top    >= 0);
    ASSERT(padding.right  >= 0);
    ASSERT(padding.bottom >= 0);

    Rect pad = UiNonNegativeThickness(padding);
    Rect ci  = UiNonNegativeThickness(skin.content_inset);
    Rect cp  = UiNonNegativeThickness(m.content_padding);

    int fw = UiResolvedFrameWidth(m, skin);

    int w = max(content.cx, 0);
    int h = max(content.cy, 0);

    w += pad.left + pad.right + ci.left + ci.right + cp.left + cp.right + 2 * fw;
    h += pad.top  + pad.bottom + ci.top + ci.bottom + cp.top + cp.bottom + 2 * fw;

    return Size(w, h);
}

// ============================================================================
// Neutral 2-block layout (support + main)
// ============================================================================

struct UiBlocksLayout {
    Rect support;
    Rect main;
};

// Measurement: negative margins do NOT reduce minsize (pos()).
inline Size UiMeasureBlocksContent(Size support_natural,
                                   Size main_natural,
                                   const Rect& support_margin,
                                   const Rect& main_margin,
                                   UiAlign stack_dir,
                                   bool have_support,
                                   bool have_main,
                                   int empty_w,
                                   int empty_h,
                                   int min_support_side)
{
    auto pos = [](int v) -> int { return v > 0 ? v : 0; };

    // Enforce consistent support minimum whenever support is present
    // (so measure == layout rules).
    if(have_support) {
        support_natural.cx = max(support_natural.cx, min_support_side);
        support_natural.cy = max(support_natural.cy, min_support_side);
    }

    int w = 0, h = 0;

    if(!have_support && !have_main) {
        w = empty_w;
        h = empty_h;
    }
    else if(have_support && !have_main) {
        w = support_natural.cx + pos(support_margin.left) + pos(support_margin.right);
        h = support_natural.cy + pos(support_margin.top)  + pos(support_margin.bottom);
    }
    else if(!have_support && have_main) {
        w = main_natural.cx + pos(main_margin.left) + pos(main_margin.right);
        h = main_natural.cy + pos(main_margin.top)  + pos(main_margin.bottom);
    }
    else {
        switch(stack_dir) {
        case UiAlign::TOP:
        case UiAlign::BOTTOM:
            w = max(main_natural.cx + pos(main_margin.left) + pos(main_margin.right),
                    support_natural.cx + pos(support_margin.left) + pos(support_margin.right));
            h = main_natural.cy + support_natural.cy
              + pos(main_margin.top) + pos(main_margin.bottom)
              + pos(support_margin.top) + pos(support_margin.bottom);
            break;

        case UiAlign::RIGHT:
        case UiAlign::LEFT:
        default:
            w = main_natural.cx + support_natural.cx
              + pos(main_margin.left) + pos(main_margin.right)
              + pos(support_margin.left) + pos(support_margin.right);
            h = max(main_natural.cy + pos(main_margin.top) + pos(main_margin.bottom),
                    support_natural.cy + pos(support_margin.top) + pos(support_margin.bottom));
            break;
        }
    }

    return Size(w, h);
}

inline UiBlocksLayout UiComputeBlocksLayout(const Rect& content,
                                            Size support_natural,
                                            Size main_natural,
                                            UiAlign align_h,
                                            UiAlign align_v,
                                            UiAlign stack_dir,
                                            const Rect& support_margin,
                                            const Rect& main_margin,
                                            int min_support_side = DPI(16))
{
    UiBlocksLayout lr;

    if(content.IsEmpty())
        return lr;

    bool have_support = support_natural.cx > 0 && support_natural.cy > 0;
    bool have_main    = main_natural.cx > 0 && main_natural.cy > 0;

    if(!have_support && !have_main)
        return lr;

    int cw = content.GetWidth();
    int ch = content.GetHeight();

    Size support_sz = support_natural;
    Size main_sz    = main_natural;

    // Enforce consistent support minimum when present.
    if(have_support) {
        support_sz.cx = max(support_sz.cx, min_support_side);
        support_sz.cy = max(support_sz.cy, min_support_side);
        support_sz.cx = min(support_sz.cx, cw);
        support_sz.cy = min(support_sz.cy, ch);
    }

    auto HOffset = [&](int outer, int inner) -> int {
        switch(align_h) {
        case UiAlign::RIGHT:  return outer - inner;
        case UiAlign::CENTER: return (outer - inner) / 2;
        case UiAlign::LEFT:
        default:              return 0;
        }
    };

    auto VOffset = [&](int outer, int inner) -> int {
        switch(align_v) {
        case UiAlign::BOTTOM: return outer - inner;
        case UiAlign::CENTER: return (outer - inner) / 2;
        case UiAlign::TOP:
        default:              return 0;
        }
    };

    if(have_support && have_main) {
        if(stack_dir == UiAlign::TOP || stack_dir == UiAlign::BOTTOM) {
            main_sz.cx = min(main_sz.cx, cw);

            int total_h = support_sz.cy + main_sz.cy;
            if(total_h > ch) {
                int max_support_h = max(0, ch - main_sz.cy);
                support_sz.cy = min(support_sz.cy, max_support_h);
                total_h = support_sz.cy + main_sz.cy;
            }

            int block_w = min(max(support_sz.cx, main_sz.cx), cw);

            int x0 = content.left + HOffset(cw, block_w);
            int y0 = content.top  + VOffset(ch, total_h);

            if(stack_dir == UiAlign::TOP) {
                int sx = x0 + (block_w - support_sz.cx) / 2;
                int sy = y0;
                lr.support = Rect(sx, sy, sx + support_sz.cx, sy + support_sz.cy);

                int mx = x0 + (block_w - main_sz.cx) / 2;
                int my = lr.support.bottom;
                lr.main = Rect(mx, my, mx + main_sz.cx, my + main_sz.cy);
            }
            else {
                int mx = x0 + (block_w - main_sz.cx) / 2;
                int my = y0;
                lr.main = Rect(mx, my, mx + main_sz.cx, my + main_sz.cy);

                int sx = x0 + (block_w - support_sz.cx) / 2;
                int sy = lr.main.bottom;
                lr.support = Rect(sx, sy, sx + support_sz.cx, sy + support_sz.cy);
            }
        }
        else {
            main_sz.cx = min(main_sz.cx, cw);

            int total_w = support_sz.cx + main_sz.cx;
            if(total_w > cw) {
                int max_support_w = max(0, cw - main_sz.cx);
                support_sz.cx = min(support_sz.cx, max_support_w);
                total_w = support_sz.cx + main_sz.cx;
            }

            int block_h = min(max(support_sz.cy, main_sz.cy), ch);

            int x0 = content.left + HOffset(cw, total_w);
            int y0 = content.top  + VOffset(ch, block_h);

            if(stack_dir == UiAlign::RIGHT) {
                int my = y0 + (block_h - main_sz.cy) / 2;
                lr.main = Rect(x0, my, x0 + main_sz.cx, my + main_sz.cy);

                int sx = lr.main.right;
                int sy = y0 + (block_h - support_sz.cy) / 2;
                lr.support = Rect(sx, sy, sx + support_sz.cx, sy + support_sz.cy);
            }
            else {
                int sy = y0 + (block_h - support_sz.cy) / 2;
                lr.support = Rect(x0, sy, x0 + support_sz.cx, sy + support_sz.cy);

                int mx = lr.support.right;
                int my = y0 + (block_h - main_sz.cy) / 2;
                lr.main = Rect(mx, my, mx + main_sz.cx, my + main_sz.cy);
            }
        }
    }
    else if(have_support) {
        int side = min(cw, ch);
        side = max(side, min_support_side);
        side = min(side, cw);
        side = min(side, ch);

        int x = content.left + HOffset(cw, side);
        int y = content.top  + VOffset(ch, side);
        lr.support = Rect(x, y, x + side, y + side);
    }
    else {
        main_sz.cx = min(main_sz.cx, cw);
        main_sz.cy = min(main_sz.cy, ch);

        int x = content.left + HOffset(cw, main_sz.cx);
        int y = content.top  + VOffset(ch, main_sz.cy);
        lr.main = Rect(x, y, x + main_sz.cx, y + main_sz.cy);
    }

    // Apply margins last (negative expands).
    if(have_support && !lr.support.IsEmpty())
        lr.support = UiApplyMarginRect(lr.support, support_margin);

    if(have_main && !lr.main.IsEmpty())
        lr.main = UiApplyMarginRect(lr.main, main_margin);

    return lr;
}

// ============================================================================
// UiBlock (minimal: measure + rect + payload; NO painting)
// ============================================================================

enum class UiBlockKind : byte {
    EMPTY   = 0,
    TEXT    = 1,
    IMAGE   = 2,
    CTRLREF = 3,
};

struct UiBlockMeasureCtx {
    uint64 style_serial = 0; // optional: owner bumps on style/font/DPI change
};

struct UiBlock {
    UiBlockKind kind = UiBlockKind::EMPTY;

    Rect rect   = Rect(0, 0, 0, 0);
    Rect margin = Rect(0, 0, 0, 0);

    // Optional caching hooks (owner-managed serial bumps).
    uint64 content_serial = 1;

    // TEXT payload (thin wrapper over owner-cached shaping/splitting)
    const Vector<String>* text_lines = nullptr;
    const Vector<Size>*   text_line_sizes = nullptr;

    // IMAGE payload
    Image image;

    // CTRLREF payload
    Ctrl* ctrl = nullptr;

    bool IsPresent() const
    {
        switch(kind) {
        case UiBlockKind::TEXT:    return text_lines && text_line_sizes && !text_lines->IsEmpty();
        case UiBlockKind::IMAGE:   return !IsNull(image);
        case UiBlockKind::CTRLREF: return ctrl != nullptr;
        default:                  return false;
        }
    }

    Size Measure(const UiBlockMeasureCtx&) const
    {
        switch(kind) {
        case UiBlockKind::TEXT:
            if(text_line_sizes && !text_line_sizes->IsEmpty()) {
                int max_w = 0;
                int sum_h = 0;
                for(const Size& s : *text_line_sizes) {
                    max_w = max(max_w, s.cx);
                    sum_h += s.cy;
                }
                return Size(max_w, sum_h);
            }
            return Size(0, 0);

        case UiBlockKind::IMAGE:
            return IsNull(image) ? Size(0, 0) : image.GetSize();

        case UiBlockKind::CTRLREF:
            return ctrl ? ctrl->GetMinSize() : Size(0, 0);

        default:
            return Size(0, 0);
        }
    }

    void SetRectLayout(const Rect& r)
    {
        rect = r;
        if(kind == UiBlockKind::CTRLREF && ctrl)
            ctrl->SetRect(r);
    }
};

// ============================================================================
// CtrlStyled (CRTP mixin)
// ============================================================================

template <class T>
class CtrlStyled {
protected:
    StyledPalette& StyledPaletteRef() { return static_cast<T&>(*this).StyledPaletteRef(); }
    StyledMetrics& StyledMetricsRef() { return static_cast<T&>(*this).StyledMetricsRef(); }
    StyledSkin&    StyledSkinRef()    { return static_cast<T&>(*this).StyledSkinRef();    }
    void           OnStyleChanged()   { static_cast<T&>(*this).OnStyleChanged();          }
    T&             Self()             { return static_cast<T&>(*this);                    }

public:
    T& SetBaseColors(Color face, Color frame, Color ink, int hot_pct = 12, int press_pct = 14)
    {
        SetFaceColor(face, hot_pct, press_pct);
        SetFrameColor(frame, hot_pct, press_pct);
        SetInkColor(ink, hot_pct, press_pct);
        return Self();
    }

    T& SetFaceColor(Color base, int hot_pct = 12, int press_pct = 14)
    {
        StyledPalette& p = StyledPaletteRef();

        Color hot      = LtColor(base, hot_pct);
        Color pressed  = DkColor(base, press_pct);
        Color disabled = DisabledColor(base);

        p.face[ST_NORMAL]   = UiFill::Solid(base);
        p.face[ST_HOT]      = UiFill::Solid(hot);
        p.face[ST_PRESSED]  = UiFill::Solid(pressed);
        p.face[ST_DISABLED] = UiFill::Solid(disabled);

        OnStyleChanged();
        return Self();
    }

    T& SetFrameColor(Color base, int hot_pct = 12, int press_pct = 14)
    {
        StyledPalette& p = StyledPaletteRef();
        p.frame[ST_NORMAL]   = base;
        p.frame[ST_HOT]      = LtColor(base, hot_pct);
        p.frame[ST_PRESSED]  = DkColor(base, press_pct);
        p.frame[ST_DISABLED] = DisabledColor(base);
        OnStyleChanged();
        return Self();
    }

    T& SetInkColor(Color base, int hot_pct = 0, int press_pct = 0)
    {
        StyledPalette& p = StyledPaletteRef();
        p.ink[ST_NORMAL]   = base;
        p.ink[ST_HOT]      = hot_pct   ? LtColor(base, hot_pct)    : base;
        p.ink[ST_PRESSED]  = press_pct ? DkColor(base, press_pct)  : base;
        p.ink[ST_DISABLED] = DisabledColor(base);
        OnStyleChanged();
        return Self();
    }

    T& SetFaceQuadGradient(Color tl, Color tr, Color bl, Color br, int tile_size = 32, int blur_radius = 0)
    {
        StyledPalette& p = StyledPaletteRef();
        Image tile = MakeQuadGradientTile(tile_size, tl, tr, bl, br, blur_radius);

        UiFill grad = UiFill::ImageFill(tile);
        for(int i = 0; i < 4; i++)
            p.face[i] = grad;

        OnStyleChanged();
        return Self(); 
    }

    // Metrics
    T& EnableFrame(bool on = true) { StyledMetricsRef().frame_enabled = on; OnStyleChanged(); return Self(); }
    T& SetFrameWidth(int w)        { StyledMetricsRef().frame_width = max(w, 0); OnStyleChanged(); return Self(); }
    T& EnableFace(bool on = true)  { StyledMetricsRef().face_enabled = on; OnStyleChanged(); return Self(); }
    T& SetRadius(int r)            { StyledMetricsRef().radius = max(r, 0); OnStyleChanged(); return Self(); }
    T& EnableDash(bool on = true)  { StyledMetricsRef().dashed = on; StyledMetricsRef().frame_enabled = on; OnStyleChanged(); return Self(); }
    T& SetDashPattern(const String& p) { StyledMetricsRef().dash_pattern = p; OnStyleChanged(); return Self(); }
    T& HighContrast(bool on = true){ StyledMetricsRef().high_contrast = on; OnStyleChanged(); return Self(); }

    // Skin
    T& SetFill9Slice(const Image& img, const Rect& slice, bool settheframe = false)
    {
        StyledSkin& s = StyledSkinRef();
        s.base           = img;
        s.slice          = UiNonNegativeThickness(slice);
        s.enabled        = true;
        StyledMetricsRef().frame_enabled = settheframe;
        
        OnStyleChanged();
        return Self();
    }

    T& SetFill9Slice(const Image& img, int thickness, bool settheframe = false)
    {
        thickness = max(thickness, 0);
        return SetFill9Slice(img, Rect(thickness, thickness, thickness, thickness), settheframe);
    }

    // Geometry-only content padding (density/breathing room inside face bounds).
    // Public API: SetPadding(...)
    T& SetPadding(const Rect& pad)
    {
        StyledMetricsRef().content_padding = UiNonNegativeThickness(pad);
        OnStyleChanged();
        return Self();
    }

    T& SetPadding(int l, int t, int r, int b)
    {
        return SetPadding(Rect(l, t, r, b));
    }

    T& SetPadding(int all)
    {
        return SetPadding(all, all, all, all);
    }

    T& ClearPadding()
    {
        StyledMetricsRef().content_padding = Rect(0, 0, 0, 0);
        OnStyleChanged();
        return Self();
    }

    // Geometry-only inset that defines skin/content face bounds (independent of image fill).
    // Public API: SetInset(...)
    T& SetInset(const Rect& inset)
    {
        StyledSkinRef().content_inset = UiNonNegativeThickness(inset);
        OnStyleChanged();
        return Self();
    }

    T& SetInset(int l, int t, int r, int b)
    {
        return SetInset(Rect(l, t, r, b));
    }

    T& SetInset(int all)
    {
        return SetInset(all, all, all, all);
    }

    T& ClearInset()
    {
        StyledSkinRef().content_inset = Rect(0, 0, 0, 0);
        OnStyleChanged();
        return Self();
    }

    T& ClearFill9Slice()
    {
        StyledSkin& s = StyledSkinRef();
        s.base           = Image();
        s.slice          = Rect(0, 0, 0, 0);
        s.content_inset  = Rect(0, 0, 0, 0);
        s.enabled        = false;
        OnStyleChanged();
        return Self();
    }
};

} // namespace Upp

#endif
