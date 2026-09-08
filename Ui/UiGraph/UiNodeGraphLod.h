#ifndef _Ui_UiNodeGraphLod_h_
#define _Ui_UiNodeGraphLod_h_

#include <Ui/UiGraph/UiNodeGraph.h>

namespace Upp {
namespace UiNodeGraphDetail {

// Internal, allocation-free presentation decisions. GUI thread; no model or
// cache mutation. Geometry, camera compatibility and both paint backends use
// these decisions rather than maintaining competing feature thresholds.
// Include after UiNodeGraph.h; this is not an additional public control API.
inline bool IsMicroNode(int width, int height) { return width < 38 || height < 26; }
inline bool IsCompactNode(int width, int height) { return width < 120 || height < 72; }
inline double TitlePaintFloor(const UiNodeGraph::LodPolicy& lod)
{
    return max(0.24, lod.title_zoom - 0.04);
}
inline double ContentPaintFloor(const UiNodeGraph::LodPolicy& lod)
{
    return max(0.24, lod.secondary_text_zoom - 0.08);
}
inline double PortPaintFloor(const UiNodeGraph::LodPolicy& lod)
{
    return max(0.12, lod.port_zoom - 0.04);
}
inline bool ShowNodePorts(bool micro, double zoom, const UiNodeGraph::LodPolicy& lod)
{
    if(micro)
        return false; // Anchors remain semantic; a rich fallback adds no glyphs.
    double low = PortPaintFloor(lod);
    double high = max(low + 0.08, lod.port_zoom + 0.16);
    double t = minmax((zoom - low) / (high - low), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t) > 0.01;
}

enum class EdgePaintBackend { Hidden, Direct, Painter };

inline EdgePaintBackend ResolveEdgePaintBackend(const UiGraphEdge& edge,
                                                const UiGraphEdgeStyle& style,
                                                bool simplified, double zoom,
                                                const UiNodeGraph::LodPolicy& lod)
{
    if(zoom < lod.edge_hide_zoom)
        return EdgePaintBackend::Hidden;
    if(!simplified && zoom >= lod.edge_simplify_zoom)
        return EdgePaintBackend::Painter;
    UiGraphArrowStyle arrow = edge.arrow == UiGraphArrowStyle::Inherit ? style.arrow : edge.arrow;
    if(edge.directed && arrow != UiGraphArrowStyle::None && arrow != UiGraphArrowStyle::Open)
        return EdgePaintBackend::Painter;
    return EdgePaintBackend::Direct;
}

} // namespace UiNodeGraphDetail
} // namespace Upp

#endif
