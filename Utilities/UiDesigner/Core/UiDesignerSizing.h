#ifndef _Utilities_UiDesigner_Core_UiDesignerSizing_h_
#define _Utilities_UiDesigner_Core_UiDesignerSizing_h_

#include "UiDesignerDocument.h"

namespace Upp {

struct UiDesignerAxisSizing : Moveable<UiDesignerAxisSizing> {
    String mode = "Fit";
    int fixed = 0;
    int min = 0;
    int max = 0;
    int natural = 1;
};

struct UiDesignerBoxSizing : Moveable<UiDesignerBoxSizing> {
    UiDesignerAxisSizing main;
    UiDesignerAxisSizing cross;
    String cross_align = "Center";
    int weight = 1;
};

struct UiDesignerGridSizing : Moveable<UiDesignerGridSizing> {
    bool scale_x = false;
    bool scale_y = false;
    Size fixed = Size(0, 0);
    Size min = Size(0, 0);
    Size max = Size(INT_MAX, INT_MAX);
    String align_x = "Auto";
    String align_y = "Auto";
};

inline UiDesignerAxisSizing UiDesignerResolveAxisSizing(
    const UiDesignerNode& node, const char *mode_id, const char *fixed_id,
    const char *min_id, const char *max_id, int natural,
    const char *fallback_mode = "Fit")
{
    UiDesignerAxisSizing sizing;
    sizing.mode = AsString(node.GetProperty(mode_id, fallback_mode));
    if(sizing.mode != "Fixed" && sizing.mode != "Expand")
        sizing.mode = "Fit";
    sizing.fixed = max(0, (int)node.GetProperty(fixed_id, 0));
    sizing.min = max(0, (int)node.GetProperty(min_id, 0));
    sizing.max = max(0, (int)node.GetProperty(max_id, 0));
    sizing.natural = max(1, natural);
    return sizing;
}

inline int UiDesignerResolveAxisExtent(const UiDesignerAxisSizing& sizing,
                                       int available)
{
    int value = sizing.mode == "Expand" ? available :
                sizing.mode == "Fixed" ? (sizing.fixed > 0 ? sizing.fixed : sizing.natural) :
                sizing.natural;
    value = max(value, sizing.min);
    if(sizing.max > 0)
        value = min(value, sizing.max);
    return max(1, value);
}

inline UiCrossAlign UiDesignerResolveBoxAlign(const String& align)
{
    if(align == "Center") return UiCrossAlign::Center;
    if(align == "Right" || align == "Bottom") return UiCrossAlign::End;
    if(align == "Stretch" || align == "Fill") return UiCrossAlign::Stretch;
    return UiCrossAlign::Start;
}

inline UiDesignerBoxSizing UiDesignerResolveBoxSizing(const UiDesignerNode& node,
                                                      bool horizontal,
                                                      int main_natural,
                                                      int cross_natural)
{
    UiDesignerBoxSizing sizing;
    sizing.main = UiDesignerResolveAxisSizing(
        node, horizontal ? "width_mode" : "height_mode",
        horizontal ? "fixed_width" : "fixed_height",
        horizontal ? "min_width" : "min_height",
        horizontal ? "max_width" : "max_height",
        main_natural, "Fit");
    sizing.cross = UiDesignerResolveAxisSizing(
        node, horizontal ? "height_mode" : "width_mode",
        horizontal ? "fixed_height" : "fixed_width",
        horizontal ? "min_height" : "min_width",
        horizontal ? "max_height" : "max_width",
        cross_natural, "Fit");
    sizing.cross_align = AsString(node.GetProperty(
        horizontal ? "cell_align_y" : "cell_align_x", "Center"));
    sizing.weight = max(1, (int)(double)node.GetProperty("weight", 1.0));
    return sizing;
}

inline UiDesignerGridSizing UiDesignerResolveGridSizing(const UiDesignerNode& node)
{
    UiDesignerGridSizing sizing;
    const String width_mode = AsString(node.GetProperty("width_mode", "Fit"));
    const String height_mode = AsString(node.GetProperty("height_mode", "Fit"));
    sizing.scale_x = width_mode == "Expand";
    sizing.scale_y = height_mode == "Expand";
    sizing.fixed = Size(
        max(0, (int)node.GetProperty("fixed_width", 0)),
        max(0, (int)node.GetProperty("fixed_height", 0)));
    sizing.min = Size(
        max(0, (int)node.GetProperty("min_width", 0)),
        max(0, (int)node.GetProperty("min_height", 0)));
    sizing.max = Size(
        max(0, (int)node.GetProperty("max_width", 0)),
        max(0, (int)node.GetProperty("max_height", 0)));
    sizing.align_x = AsString(node.GetProperty("cell_align_x", "Auto"));
    sizing.align_y = AsString(node.GetProperty("cell_align_y", "Auto"));
    return sizing;
}

inline int UiDesignerGridAlignValue(const String& align)
{
    if(align == "Center") return 1;
    if(align == "End" || align == "Right" || align == "Bottom") return 2;
    if(align == "Stretch" || align == "Fill") return 3;
    return 0;
}

}

#endif
