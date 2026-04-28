#ifndef _Ui_UiPanel_h_
#define _Ui_UiPanel_h_

/*
    UiPanel
    =======

    Purpose
    - Styled container control for painted panel surfaces.

    Intent
    - Keep appearance separate from child layout by exposing one shared panel
      style contract used by composite controls and demos.

    Thread context
    - GUI thread only.

    Usage
    - Use as a visual surface/container when you need painted chrome around
      child controls.

    Changelog
    - 2026-03: added release-standard header documentation.
*/

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

class UiPanel : public Ctrl, public CtrlStyled<UiPanel> {
public:
    typedef UiPanel CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        bool transparent = false;

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % transparent;
        }
    };

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_style_override_ = false;
    Size user_min_size_ = Size(0, 0);

    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();

public:
    UiPanel();

    UiPanel& SetStyle(const Style& s);
    UiPanel& ClearStyleOverride();
    bool HasStyleOverride() const { return has_style_override_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetEffectiveStyle() const;
    static const Style& StyleDefault();

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().skin; }

    void OnStyleChanged();

    virtual Size GetMinSize() const override;

    UiPanel& SetSizeMin(Size sz);
    UiPanel& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    UiPanel& SetSizeFixed(Size sz) { return SetSizeMin(sz); }
    UiPanel& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&, const StyledSkin&, StyledState, bool> WhenPaintBackground;
    Event<Draw&, const Rect&, const StyledPalette&, const StyledMetrics&, const StyledSkin&, StyledState, bool> WhenPaintForeground;

    virtual void Paint(Draw& w) override;
};

} // namespace Upp

#endif
