#ifndef _Ui_UiGraph_UiGraphNodeComponent_h_
#define _Ui_UiGraph_UiGraphNodeComponent_h_

// Internal production component preparation. Shares the region allocator in
// UiNodeGraphPresentation; this is not another layout engine or retained cache.
#include <Ui/UiGraph/UiNodeGraph.h>

namespace Upp {
namespace UiNodeGraphDetail {

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
