#ifndef _Ui_UiGraph_UiGraphNodeComponent_h_
#define _Ui_UiGraph_UiGraphNodeComponent_h_

// Internal production component preparation. Shares the region allocator in
// UiNodeGraphPresentation; this is not another layout engine or retained cache.
#include <Ui/UiGraph/UiNodeGraph.h>

namespace Upp {
namespace UiNodeGraphDetail {

// Header/Footer may use independently validated shape bands. Paint and authoring
// picking consume the SAME retained region rather than the old inscribed box.
// A legacy/synthetic snapshot without a band keeps conservative safe clipping.
inline Rect NodeComponentClip(const UiGraphNodePresentation& p,
                              const UiGraphNodeComponentPresentation& c)
{
    Rect region = p.safe;
    if(c.region == UiGraphNodeSlotRegion::Header && !p.header.IsEmpty()) region = p.header;
    if(c.region == UiGraphNodeSlotRegion::Footer && !p.footer.IsEmpty()) region = p.footer;
    return c.slot & region;
}

struct ResolvedNodeComponent {
    UiGraphNodeComponentReason reason = UiGraphNodeComponentReason::None;
    WString text;
    Font base_font;
    Image icon, overview;
    Value value;
    Size authored_size = Size(0, 0);
};

ResolvedNodeComponent ResolveNodeComponent(const UiGraphNodeSlotRule& rule,
                                           const UiGraphNode& node,
                                           const UiGraphNodeStyle& style, bool micro);
void PrepareNodeComponent(const UiGraphNodeSlotRule& rule,
                          const ResolvedNodeComponent& value, double zoom,
                          const UiGraphNode& node, const UiGraphNodeStyle& style,
                          UiGraphNodeComponentPresentation& out, bool micro);
int ComponentPrimitiveCost(const UiGraphNodeComponentPresentation& component);

} // namespace UiNodeGraphDetail
} // namespace Upp
#endif
