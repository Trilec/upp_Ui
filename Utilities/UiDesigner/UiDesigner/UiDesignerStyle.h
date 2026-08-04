#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerStyle_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerStyle_h_

#include <Ui/Ui.h>
#include <Utilities/UiDesigner/ThemeCore/UiDesignerTheme.h>

namespace Upp {

struct UiDesignerStyleMetrics {
    enum {
        PanelNormalPixels = 250,
        PanelMediumPixels = 324,
        PanelWidePixels = 346,
        InspectorNormalPixels = 324,
        InspectorMediumPixels = 364,
        InspectorWidePixels = 404,
    };

    static int HeaderHeight()          { return DPI(63); }
    static int FooterHeight()          { return DPI(27); }
    static int RailWidth()             { return DPI(56); }
    // The left catalog column retains the reference shell profile.
    static int PanelNormalWidth()      { return DPI(PanelNormalPixels); }
    static int PanelMediumWidth()      { return DPI(PanelMediumPixels); }
    static int PanelWideWidth()        { return DPI(PanelWidePixels); }
    // Inspector content now needs the former middle width as its minimum.
    // Each subsequent press adds a clear forty-pixel working increment.
    static int InspectorNormalWidth()  { return DPI(InspectorNormalPixels); }
    static int InspectorMediumWidth()  { return DPI(InspectorMediumPixels); }
    static int InspectorWideWidth()    { return DPI(InspectorWidePixels); }
    static int DesignerToolbarHeight() { return DPI(63); }
    static int Gap()                   { return DPI(8); }
    static int HeaderInset()           { return DPI(6); }
    static int LeftPillInset()         { return DPI(20); }
    static int RightPillInset()        { return DPI(19); }
    static int SurfaceRadius()         { return DPI(8); }
    static int PillRadius()            { return DPI(25); }
    static int ShadowDistance()        { return DPI(6); }
    static int ShadowOffsetY()         { return DPI(2); }
    static int ShadowAlpha()           { return 24; }
};

inline void UiDesignerApplyShadow(
    StyledMetrics& metrics,
    const UiDesignerThemeSnapshot& theme = UiDesignerThemeSnapshot())
{
    metrics.shadow.enabled = theme.shadows;
    metrics.shadow.distance = DPI(theme.shadow_distance);
    metrics.shadow.offset_x = 0;
    metrics.shadow.offset_y = DPI(theme.shadow_offset_y);
    metrics.shadow.alpha = theme.shadow_alpha;
    metrics.shadow.color = Black();
    metrics.shadow.inset = false;
    metrics.shadow.mode = SHADOW_CURVE;
    metrics.shadow.curve = ShadowSoft();
}

inline UiPanel::Style UiDesignerSurfaceStyle(
    UiRole role = UiRole::Subtle,
    const UiDesignerThemeSnapshot& theme = UiDesignerThemeSnapshot())
{
    UiPanel::Style style = UiTheme::ResolvePanel(role);
    style.metrics.radius = DPI(theme.radius);
    style.metrics.frame_width = DPI(theme.border_width);
    UiDesignerApplyShadow(style.metrics, theme);
    return style;
}

inline UiPanel::Style UiDesignerPillStyle(
    UiRole role = UiRole::Subtle,
    const UiDesignerThemeSnapshot& theme = UiDesignerThemeSnapshot())
{
    UiPanel::Style style = UiTheme::ResolvePanel(role);
    style.metrics.radius = DPI(theme.pill_radius);
    style.metrics.frame_width = DPI(theme.border_width);
    UiDesignerApplyShadow(style.metrics, theme);
    return style;
}

}

#endif
