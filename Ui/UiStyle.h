#ifndef _Ui_UiStyle_h_
#define _Ui_UiStyle_h_

#include <CtrlCore/CtrlCore.h>
#include <Draw/Draw.h>

namespace Upp {


// ---------------------------------------------------------------------------
// Shared alignment enums for Ui controls
// ---------------------------------------------------------------------------
//
// UiAlignH / UiAlignV are generic alignment hints that any Ui control can use
// for text/content alignment inside its rect.
//
// UiImageLayout describes relative placement of an image vs text (left/right/
// top/bottom). Controls that combine icon + label (buttons, accordions,
// dropdown rows, tabs, etc.) should use this instead of bespoke enums.
//

enum UiAlignH : byte {
    UIALIGN_LEFT   = 0,
    UIALIGN_HCENTER,
    UIALIGN_RIGHT,
};

enum UiAlignV : byte {
    UIALIGN_TOP    = 0,
    UIALIGN_VCENTER,
    UIALIGN_BOTTOM,
};

enum UiImageLayout : byte {
    UIIMAGE_LEFT   = 0,
    UIIMAGE_RIGHT,
    UIIMAGE_TOP,
    UIIMAGE_BOTTOM,
};


// ============================================================================
// DATA TYPES & STATE
// ============================================================================

// Unified visual state indexing for all styled controls
enum StyledState : int {
    ST_NORMAL   = 0, // Default state
    ST_HOT      = 1, // Hovered or focused
    ST_PRESSED  = 2, // Active / clicking / checked
    ST_DISABLED = 3, // Disabled / inactive
};


// 1. PALETTE: Colors per state (Face / Frame / Ink)
struct StyledPalette {
    Color face[4];   // Background Fill ("Face")
    Color frame[4];  // Border Stroke ("Frame")
    Color ink[4];    // Text or Icon ("Ink")

    void Serialize(Stream& s)
    {
        for(int i = 0; i < 4; i++) s % face[i];
        for(int i = 0; i < 4; i++) s % frame[i];
        for(int i = 0; i < 4; i++) s % ink[i];
    }
};

// 2. METRICS: Geometry and flags
struct StyledMetrics {
    Font  text_font     = StdFont();
    bool  use_text_font = false; // If true, control should SetFont(text_font)

    int   radius        = DPI(4); // Corner radius
    int   frame_width   = DPI(1); // Border thickness

    bool  frame_enabled = true;  // Draw border
    bool  face_enabled  = true;  // Draw background fill (Face)

    bool  dashed        = false; // Dashed border
    String dash_pattern = "5,5";

    bool  high_contrast = false; // Accessibility hint (control-specific handling)

    void Serialize(Stream& s)
    {
        s % text_font
          % use_text_font
          % radius
          % frame_width
          % frame_enabled
          % face_enabled
          % dashed
          % dash_pattern
          % high_contrast;
    }
};

// 3. SKIN: Image-based 9-slice styling
//
// inset encodes the center region as two Points (p1 = topleft, p2 = bottomright)
// in image coordinates. It is exactly what the dev branch Draw9Slice takes.
struct StyledSkin {
    Image base;                         // Source image for 9-slice
    Rect  inset          = Rect(0, 0, 0, 0); // center rect in image coords
    bool  enabled        = false;       // If true, overrides Face fill
    bool  includes_frame = false;       // If true, suppress separate Frame drawing

    void Serialize(Stream& s)
    {
        s % base
          % inset
          % enabled
          % includes_frame;
    }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------


// Lighten towards white
inline Color LtColor(Color base, int pct)
{
    return Blend(base, White(), pct);
}

// Darken towards black
inline Color DkColor(Color base, int pct)
{
    return Blend(base, Black(), pct);
}

// Disabled blend using Chameleon semantic disabled color
inline Color DisabledColor(Color base)
{
    return Blend(base, SColorDisabled(), 50);
}

// Standard state resolver for all controls
inline StyledState ResolveStyledState(bool enabled, bool hot, bool pressed)
{
    if(!enabled) return ST_DISABLED;
    if(pressed)  return ST_PRESSED;
    if(hot)      return ST_HOT;
    return ST_NORMAL;
}

// ---------------------------------------------------------------------------
// 9-SLICE DRAWING HELPER
// ---------------------------------------------------------------------------
//
// Signature mirrors the dev-branch Draw9Slice to ease future swap:
//
//     void Draw9Slice(Draw& w, const Rect& r,
//                     const Image& img, Point p1, Point p2);
//
// p1, p2 define the stretchable center rectangle in image coordinates.
//
// There is also a convenience overload with uniform integer margin:
//
//     void Draw9Slice(Draw& w, const Rect& r,
//                     const Image& img, int margin);
//

// Thin wrapper so Ui can depend on the core implementation and still keep a
// namespaced helper if we ever need to customize behavior.
inline void UiDraw9Slice(Draw& w, const Rect& dest, const Image& img,
                         Point p1, Point p2)
{
    if(IsNull(img) || dest.IsEmpty())
        return;
    Draw9Slice(w, dest, img, p1, p2);
}

// Convenience overload with uniform integer margin (like dev Draw9Slice)
inline void UiDraw9Slice(Draw& w, const Rect& dest, const Image& img,
                         int margin)
{
    if(IsNull(img) || dest.IsEmpty()) {
        if(!IsNull(img))
            w.DrawImage(dest, img);
        return;
    }
    Draw9Slice(w, dest, img, margin);
}

// ============================================================================
// PART 2: THE INTERFACE MIXIN (CRTP)
// ============================================================================
//
// Usage pattern (example):
//
// class UiButton : public Ctrl, public CtrlStyled<UiButton> {
// public:
//     struct Style : ChStyle<Style> {
//         StyledPalette palette;
//         StyledMetrics metrics;
//         StyledSkin    skin;
//         // ... extras ...
//     };
// private:
//     Style style_;
// public:
//     StyledPalette& StyledPaletteRef() { return style_.palette; }
//     StyledMetrics& StyledMetricsRef() { return style_.metrics; }
//     StyledSkin&    StyledSkinRef()    { return style_.skin;    }
//     void           OnStyleChanged();  // Rebuild + Refresh
// };

template <class T>
class CtrlStyled {
protected:
    // Derived must implement these
    StyledPalette& StyledPaletteRef() { return static_cast<T&>(*this).StyledPaletteRef(); }
    StyledMetrics& StyledMetricsRef() { return static_cast<T&>(*this).StyledMetricsRef(); }
    StyledSkin&    StyledSkinRef()    { return static_cast<T&>(*this).StyledSkinRef();    }
    void           OnStyleChanged()   { static_cast<T&>(*this).OnStyleChanged();          }
    T&             Self()             { return static_cast<T&>(*this);                    }

public:
    // -----------------------------------------------------------------------
    // Palette mutators (Face / Frame / Ink)
    // -----------------------------------------------------------------------

    // Sets base Face, Frame, Ink and auto-generates all 4 states
    T& SetBaseColors(Color face, Color frame, Color ink,
                     int hot_pct = 12, int press_pct = 14)
    {
        SetFaceColor(face, hot_pct, press_pct);
        SetFrameColor(frame, hot_pct, press_pct);
        SetInkColor(ink, hot_pct, press_pct);
        return Self();
    }

    // Face (background)
    T& SetFaceColor(Color base, int hot_pct = 12, int press_pct = 14)
    {
        StyledPalette& p = StyledPaletteRef();
        p.face[ST_NORMAL]   = base;
        p.face[ST_HOT]      = LtColor(base, hot_pct);
        p.face[ST_PRESSED]  = DkColor(base, press_pct);
        p.face[ST_DISABLED] = DisabledColor(base);
        OnStyleChanged();
        return Self();
    }

    // Frame (border)
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

    // Ink (text / icon)
    T& SetInkColor(Color base, int hot_pct = 0, int press_pct = 0)
    {
        StyledPalette& p = StyledPaletteRef();
        p.ink[ST_NORMAL]   = base;
        p.ink[ST_HOT]      = hot_pct   ? LtColor(base, hot_pct)   : base;
        p.ink[ST_PRESSED]  = press_pct ? DkColor(base, press_pct) : base;
        p.ink[ST_DISABLED] = DisabledColor(base);
        OnStyleChanged();
        return Self();
    }

    // -----------------------------------------------------------------------
    // Metrics mutators (geometry & flags)
    // -----------------------------------------------------------------------

    T& EnableFrame(bool on = true)
    {
        StyledMetricsRef().frame_enabled = on;
        OnStyleChanged();
        return Self();
    }

    T& SetFrameWidth(int w)
    {
        StyledMetricsRef().frame_width = max(w, 0);
        OnStyleChanged();
        return Self();
    }

    T& EnableFace(bool on = true)
    {
        StyledMetricsRef().face_enabled = on;
        OnStyleChanged();
        return Self();
    }

    T& SetRadius(int r)
    {
        StyledMetricsRef().radius = max(r, 0);
        OnStyleChanged();
        return Self();
    }

    T& EnableDash(bool on = true)
    {
        StyledMetricsRef().dashed = on;
        OnStyleChanged();
        return Self();
    }

    T& SetDashPattern(const String& p)
    {
        StyledMetricsRef().dash_pattern = p;
        OnStyleChanged();
        return Self();
    }

    T& HighContrast(bool on = true)
    {
        StyledMetricsRef().high_contrast = on;
        OnStyleChanged();
        return Self();
    }

    // -----------------------------------------------------------------------
    // Skin mutators (9-slice / image-based)
    // -----------------------------------------------------------------------

    // Rect-based inset (center region in image coords)
    T& SetFill9Slice(const Image& img, const Rect& inset,
                     bool includes_frame = false)
    {
        StyledSkin& s = StyledSkinRef();
        s.base = img;
        s.inset = inset;
        s.enabled = true;
        s.includes_frame = includes_frame;
        OnStyleChanged();
        return Self();
    }

    // Convenient overload: single integer margin (like Draw9Slice(..., int margin))
    T& SetFill9Slice(const Image& img, int margin,
                     bool includes_frame = false)
    {
        StyledSkin& s = StyledSkinRef();
        s.base = img;

        Size sz = img.GetSize();
        if(margin <= 0 || sz.cx <= 2 * margin || sz.cy <= 2 * margin)
            s.inset = Rect(0, 0, 0, 0); // fall back to simple stretch
        else
            s.inset = Rect(margin,
                           margin,
                           sz.cx - margin - 1,
                           sz.cy - margin - 1);

        s.enabled = true;
        s.includes_frame = includes_frame;
        OnStyleChanged();
        return Self();
    }

    T& ClearFill9Slice()
    {
        StyledSkinRef().enabled = false;
        OnStyleChanged();
        return Self();
    }
};



} // namespace Upp

#endif
