#ifndef _UiDesigner_DesignOverlay_UiDesignerDesignMetrics_h_
#define _UiDesigner_DesignOverlay_UiDesignerDesignMetrics_h_

#include <Ui/Ui.h>

namespace Upp {

// Visual constants taken from the exported Designer and Theme Studio layouts.
// Keep these together so shell refinements do not become scattered magic numbers.
struct UiDesignerDesignMetrics {
    static int HeaderHeight()      { return DPI(42); }
    static int FooterHeight()      { return DPI(27); }
    static int RailWidth()         { return DPI(42); }
    static int PanelNormalWidth()  { return DPI(230); }
    static int PanelMediumWidth()  { return DPI(280); }
    static int PanelWideWidth()    { return DPI(340); }
    static int CenterToolbarHeight(){ return DPI(42); }
    static int Gap()               { return DPI(8); }
    static int Inset()             { return DPI(6); }

    // Curt's authored shell uses a restrained radius and a light elevation shadow.
    static int SurfaceRadius()     { return DPI(8); }
    static int SurfaceShadowBlur() { return DPI(6); }
    static int SurfaceShadowY()    { return DPI(2); }
    static int SurfaceShadowAlpha(){ return 24; }
};

} // namespace Upp

#endif
