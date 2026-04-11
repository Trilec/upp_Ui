#ifndef _Ui_UiScrollPanel_h_
#define _Ui_UiScrollPanel_h_

/*
    UiScrollPanel
    =============

    Purpose
    - Styled scrolling container with owned viewport/content handling.

    Intent
    - Wrap child content with painted panel chrome and explicit scroll policy
      without leaking scrollbar internals into the public surface.

    Thread context
    - GUI thread only.

    Usage
    - Add child content to the panel and choose the scroll mode that matches
      the intended viewport behavior.

    Changelog
    - 2026-03: added release-standard header documentation.
    - 2026-04: switched to UiScrollBar ownership and tightened scrollbar
      placement so the reserved gutter reads more like a native inset rail.
*/

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiScrollBar.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

enum UiScrollPanelMode : byte {
    UIPANELSCROLL_AUTO = 0,
    UIPANELSCROLL_VERTICAL,
    UIPANELSCROLL_HORIZONTAL,
    UIPANELSCROLL_NONE,
};

class UiScrollPanel : public Ctrl, public CtrlStyled<UiScrollPanel> {
public:
    typedef UiScrollPanel CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;
        bool transparent = false;

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin % transparent;
        }
    };

    UiScrollPanel();

    static const Style& StyleDefault();

    UiScrollPanel& SetStyle(const Style& s);
    UiScrollPanel& ClearStyleOverride();
    bool           HasStyleOverride() const { return has_style_override_; }
    const Style&   GetStyle() const { return GetEffectiveStyle(); }

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().skin; }
    void OnStyleChanged();

    UiScrollPanel& SetScrollMode(UiScrollPanelMode m);
    UiScrollPanelMode GetScrollMode() const { return mode_; }

    UiScrollPanel& SetSizeMin(Size sz) { user_min_size_ = sz; RefreshLayout(); Refresh(); return *this; }
    UiScrollPanel& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    ParentCtrl& Content() { return content_; }
    const ParentCtrl& Content() const { return content_; }

    Point GetScrollPos() const { return origin_; }
    UiScrollPanel& SetScrollPos(Point p);

    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintBackground;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintForeground;

private:
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    Rect MeasureContentBounds() const;
    Rect GetFaceRect() const;
    Rect GetViewportRect() const;
    void UpdateScrollbars();
    void ApplyScroll();
    void SyncScrollBarStyles();

private:
    Style style_;
    uint64 theme_revision_ = 0;
    bool has_style_override_ = false;
    UiScrollPanelMode mode_ = UIPANELSCROLL_AUTO;

    UiScrollBar sbx_;
    UiScrollBar sby_;
    ParentCtrl content_;

    Point origin_ = Point(0, 0);
    Size  content_size_ = Size(0, 0);
    Rect  content_bounds_ = Rect(0, 0, 0, 0);

    bool updating_sb_ = false;
    Size user_min_size_ = Size(0, 0);
};

}

#endif



