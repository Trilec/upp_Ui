#ifndef _Ui_UiPanel_h_
#define _Ui_UiPanel_h_

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

/*
    UiPanel
    =======

    Lightweight styled background panel.

    - Owns StyledPalette / StyledMetrics / StyledSkin via CtrlStyled<UiPanel>.
    - Typically used as a visual container / card / group background.
    - No text/icon, just background + optional focus ring/overlays.
    - Exposes WhenPaintBackground / WhenPaintForeground hooks
      with full palette/metrics/skin context.
*/

class UiPanel : public Ctrl, public CtrlStyled<UiPanel> {
public:
    typedef UiPanel CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        // If true, panel will be Transparent() and let parent paint the background.
        // If false, panel uses BackPaint() and draws its own.
        bool transparent = false;

        // If true, default foreground pass may draw a focus ring when HasFocus().
        bool show_focus  = false;

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % transparent % show_focus;
        }
    };

private:
    Style style_;
    Size  user_min_size_ = Size(0, 0); // explicit outer min-size hint

public:
    UiPanel();

    // Styling ---------------------------------------------------------------

    UiPanel& SetStyle(const Style& s);
    const Style& GetStyle() const { return style_; }

    // Preset styles
    static const Style& StyleDefault(); // light card / panel
    static const Style& StyleDark();    // dark card
    static const Style& StyleFlat();    // flat, square-edged panel

    // CtrlStyled integration
    StyledPalette& StyledPaletteRef() { return style_.palette; }
    StyledMetrics& StyledMetricsRef() { return style_.metrics; }
    StyledSkin&    StyledSkinRef()    { return style_.skin;    }

    void OnStyleChanged();

    // Sizing ---------------------------------------------------------------

    virtual Size GetMinSize() const override;

    UiPanel& SetSizeMin(Size sz);
    UiPanel& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    UiPanel& SetSizeFixed(Size sz)        { return SetSizeMin(sz); }
    UiPanel& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    // Painting hooks -------------------------------------------------------

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintBackground;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintForeground;

    // Ctrl overrides -------------------------------------------------------

    virtual void Paint(Draw& w) override;
};

} // namespace Upp

#endif
