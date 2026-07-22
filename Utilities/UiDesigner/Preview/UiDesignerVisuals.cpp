#include "UiDesignerVisuals.h"

namespace Upp {

Color UiDesignerStableLayoutColor(UiDesignerNodeId node, int depth, Color parent_color)
{
    static const Color palette[] = {
        Color(125, 211, 252),
        Color(167, 243, 208),
        Color(254, 240, 138),
        Color(252, 211, 77),
        Color(196, 181, 253),
        Color(251, 191, 36),
        Color(165, 180, 252),
        Color(134, 239, 172),
        Color(253, 186, 116),
        Color(244, 114, 182),
        Color(147, 197, 253),
        Color(192, 132, 252),
    };
    CombineHash h;
    h << (int64)node << depth;
    const int count = __countof(palette);
    int index = (int)((dword)h % count);
    Color color = palette[index];
    if(color == parent_color)
        color = palette[(index + 5) % count];
    return color;
}

}
