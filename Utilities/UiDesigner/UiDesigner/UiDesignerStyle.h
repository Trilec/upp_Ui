#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerStyle_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerStyle_h_

#include <Ui/Ui.h>

namespace Upp {

struct UiDesignerStyleMetrics {
    static int HeaderHeight()       { return DPI(42); }
    static int FooterHeight()       { return DPI(27); }
    static int RailWidth()          { return DPI(56); }
    static int PanelNormalWidth()   { return DPI(230); }
    static int PanelMediumWidth()   { return DPI(280); }
    static int PanelWideWidth()     { return DPI(340); }
    static int DesignerToolbarHeight() { return DPI(42); }
    static int Gap()                { return DPI(8); }
    static int HeaderInset()        { return DPI(6); }
    static int LeftPillInset()      { return DPI(20); }
    static int RightPillInset()     { return DPI(19); }

    // The broad surfaces are gently rounded; the icon holders are capsules.
    static int SurfaceRadius()      { return DPI(8); }
    static int PillRadius()         { return DPI(25); }

    // Curt's design uses restrained elevation rather than a heavy card shadow.
    static int ShadowBlur()         { return DPI(6); }
    static int ShadowOffsetY()      { return DPI(2); }
    static int ShadowAlpha()        { return 24; }
};

inline UiPanel::Style UiDesignerSurfaceStyle(UiRole role = UiRole::Subtle)
{
    UiPanel::Style style = UiTheme::ResolvePanel(role);
    style.metrics.radius = UiDesignerStyleMetrics::SurfaceRadius();
    style.metrics.shadow_blur = UiDesignerStyleMetrics::ShadowBlur();
    style.metrics.shadow_offset = Point(0, UiDesignerStyleMetrics::ShadowOffsetY());
    style.palette.shadow = Color(0, 0, 0, UiDesignerStyleMetrics::ShadowAlpha());
    return style;
}

inline UiPanel::Style UiDesignerPillStyle(UiRole role = UiRole::Subtle)
{
    UiPanel::Style style = UiTheme::ResolvePanel(role);
    style.metrics.radius = UiDesignerStyleMetrics::PillRadius();
    style.metrics.shadow_blur = UiDesignerStyleMetrics::ShadowBlur();
    style.metrics.shadow_offset = Point(0, UiDesignerStyleMetrics::ShadowOffsetY());
    style.palette.shadow = Color(0, 0, 0, UiDesignerStyleMetrics::ShadowAlpha());
    return style;
}

} // namespace Upp

#endif
