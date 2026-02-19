#ifndef _Ui_UiScrollPanel_h_
#define _Ui_UiScrollPanel_h_

#include <CtrlLib/CtrlLib.h>
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
        bool show_focus  = false;

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin % transparent % show_focus;
        }
    };

    UiScrollPanel();

    static const Style& StyleDefault();
    static const Style& StyleFlat();

    UiScrollPanel& SetStyle(const Style& s);
    const Style& GetStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return style_.palette; }
    StyledMetrics& StyledMetricsRef() { return style_.metrics; }
    StyledSkin&    StyledSkinRef()    { return style_.skin; }
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

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintBackground;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintForeground;

private:
    Rect MeasureContentBounds() const;
    void UpdateScrollbars();
    void ApplyScroll();

private:
    Style style_;
    UiScrollPanelMode mode_ = UIPANELSCROLL_AUTO;

    ScrollBars sb_;
    ParentCtrl content_;

    Point origin_ = Point(0, 0);
    Size  content_size_ = Size(0, 0);
    Rect  content_bounds_ = Rect(0, 0, 0, 0);

    bool updating_sb_ = false;
    Size user_min_size_ = Size(0, 0);
};

}

#endif
