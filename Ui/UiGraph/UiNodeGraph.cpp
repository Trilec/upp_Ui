#include <Ui/UiGraph/UiNodeGraph.h>
#include <Ui/Ui.h>
#include <Ui/UiRenderLayer.h>

// Preserve the validated retained implementation in this translation unit,
// but keep active render/hierarchy policy separate. H2 aliases only the methods
// whose semantics must become scope-local; the retained implementation remains
// callable for recovery until Windows acceptance is complete.
#define RefreshLayout() do { if(auto_fit_first_paint_ && !first_paint_done_) Ctrl::RefreshLayout(); } while(0)
#define Rectangle                    LegacyRectangle
#define UsesRectangularStyledSurface UsesRectangularStyledSurfaceLegacy
#define Paint                         PaintLegacy
#define PaintGraphGeometry            PaintGraphGeometryLegacy
#define PaintNodeDetails              PaintNodeDetailsLegacy
#define FitToGraph                    FitToGraphLegacy
#define CenterOnNode                  CenterOnNodeLegacy
#define SelectNode                    SelectNodeLegacy
#define SelectEdge                    SelectEdgeLegacy
#define SetData                       SetDataLegacy
#define GetData                       GetDataLegacy
#define Layout                        LayoutLegacy
#include "UiNodeGraphBase.inc"
#undef Layout
#undef GetData
#undef SetData
#undef SelectEdge
#undef SelectNode
#undef CenterOnNode
#undef FitToGraph
#undef PaintNodeDetails
#undef PaintGraphGeometry
#undef Paint
#undef UsesRectangularStyledSurface
#undef Rectangle
#undef RefreshLayout

namespace Upp {
namespace UiNodeGraphRenderMath {

inline double fmod(double value, double period)
{
    if(!(period > 0.0) || !std::isfinite(value) || !std::isfinite(period))
        return 0.0;

    struct RepeatState {
        double value = 0.0;
        double period = 0.0;
        int repeats = 0;
        int64 at_us = 0;
    };
    static thread_local RepeatState state;

    const int64 now = usecs();
    const bool same_burst = value == state.value && period == state.period
                          && now >= state.at_us && now - state.at_us <= 500;
    state.repeats = same_burst ? state.repeats + 1 : 0;
    state.value = value;
    state.period = period;
    state.at_us = now;

    if(state.repeats >= 2) {
        state.repeats = 0;
        return 0.0;
    }
    return std::fmod(value, period);
}

} // namespace UiNodeGraphRenderMath
} // namespace Upp

// Keep the accepted R9/R10 software render policy intact but retain its former
// top-level Paint as a recovery function. H2 owns only the final paint ordering
// required to place Backdrops between the canvas/grid and graph geometry.
#define Paint PaintRenderBase
#define std UiNodeGraphRenderMath
#include "UiNodeGraphRender.inc"
#undef std
#undef Paint

#include "UiNodeGraphHierarchy.inc"
#include "UiNodeGraphHierarchyPaint.inc"
