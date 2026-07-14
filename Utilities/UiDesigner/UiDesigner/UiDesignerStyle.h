#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerStyle_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerStyle_h_

#include <Ui/Ui.h>

namespace Upp {

struct UiDesignerStyleMetrics {
    static int HeaderHeight()          { return DPI(42); }
    static int FooterHeight()          { return DPI(27); }
    static int RailWidth()             { return DPI(56); }
    static int PanelNormalWidth()      { return DPI(230); }
    static int PanelMediumWidth()      { return DPI(280); }
    static int PanelWideWidth()        { return DPI(340); }
    static int DesignerToolbarHeight() { return DPI(42); }
    static int Gap()                   { return DPI(8); }
    static int HeaderInset()           { return DPI(6); }
    static int LeftPillInset()         { return DPI(20); }
    static int RightPillInset()        { return DPI(19); }

    // Broad shell surfaces are gently rounded; icon-holder strips are capsules.
    static int SurfaceRadius()         { return DPI(8); }
    static int PillRadius()            { return DPI(25); }

    // Restrained elevation from the authored design.
    static int ShadowDistance()        { return DPI(6); }
    static int ShadowOffsetY()         { return DPI(2); }
    static int ShadowAlpha()           { return 24; }
};

inline void UiDesignerApplyShadow(StyledMetrics& metrics)
{
    metrics.shadow.enabled = true;
    metrics.shadow.distance = UiDesignerStyleMetrics::ShadowDistance();
    metrics.shadow.offset_x = 0;
    metrics.shadow.offset_y = UiDesignerStyleMetrics::ShadowOffsetY();
    metrics.shadow.alpha = UiDesignerStyleMetrics::ShadowAlpha();
    metrics.shadow.color = Black();
    metrics.shadow.inset = false;
    metrics.shadow.mode = SHADOW_CURVE;
    metrics.shadow.curve = ShadowSoft();
}

inline UiPanel::Style UiDesignerSurfaceStyle(UiRole role = UiRole::Subtle)
{
    UiPanel::Style style = UiTheme::ResolvePanel(role);
    style.metrics.radius = UiDesignerStyleMetrics::SurfaceRadius();
    UiDesignerApplyShadow(style.metrics);
    return style;
}

inline UiPanel::Style UiDesignerPillStyle(UiRole role = UiRole::Subtle)
{
    UiPanel::Style style = UiTheme::ResolvePanel(role);
    style.metrics.radius = UiDesignerStyleMetrics::PillRadius();
    UiDesignerApplyShadow(style.metrics);
    return style;
}

} // namespace Upp

#endif
