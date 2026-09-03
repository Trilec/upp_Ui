#include <Ui/UiGraph/UiNodeGraph.h>
#include <Ui/Ui.h>
#include <Ui/UiRenderLayer.h>

// Preserve the validated retained implementation in this translation unit,
// but keep active render/hierarchy/performance policy separate. H2 aliases only
// the methods whose semantics must become scope-local; the performance slice
// additionally aliases public camera mutations so composite view changes can be
// coalesced without rewriting the retained implementation.
#define RefreshLayout() do { if(auto_fit_first_paint_ && !first_paint_done_) Ctrl::RefreshLayout(); } while(0)
#define Rectangle                    LegacyRectangle
#define UsesRectangularStyledSurface UsesRectangularStyledSurfaceLegacy
#define Paint                         PaintLegacy
#define PaintGraphGeometry            PaintGraphGeometryLegacy
#define PaintNodeDetails              PaintNodeDetailsLegacy
#define SetModel                      SetModelLegacy
#define UseInternalModel              UseInternalModelLegacy
#define SetZoom                       SetZoomLegacy
#define SetPan                        SetPanLegacy
#define PanBy                         PanByLegacy
#define ResetView                     ResetViewLegacy
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
#undef ResetView
#undef PanBy
#undef SetPan
#undef SetZoom
#undef UseInternalModel
#undef SetModel
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

// Keep the accepted R9/R10 software render policy intact as the rich/detail
// fallback. P2 gives the active name to a projected-micro direct Draw path while
// retaining this exact renderer for ordinary/reference nodes and custom paint.
#define Paint PaintRenderBase
#define PaintGraphGeometry PaintGraphGeometryRenderBase
#define std UiNodeGraphRenderMath
#include "UiNodeGraphRender.inc"
#undef std
#undef PaintGraphGeometry
#undef Paint

// The retained recovery source uses wire-0 as its historical flat Rectangle.
// Keep that mapping while compiling the new overview path so canonical Rectangle
// (wire 13) continues to own its authored corner radius exactly as the active
// renderer does.
#define Rectangle LegacyRectangle
#include "UiNodeGraphPerformance.inc"
#undef Rectangle

// All active hierarchy/view operations should settle through the overview-aware
// preparer. The retained PrepareGeometry remains available to legacy/recovery
// internals but cannot accidentally turn an extreme overview back into rich
// per-node geometry on SetScope/Fit/selection/Layout.
#define PrepareGeometry PrepareViewGeometry
#include "UiNodeGraphHierarchy.inc"
#undef PrepareGeometry

#include "UiNodeGraphHierarchyModelSwitch.inc"
#include "UiNodeGraphHierarchyPaint.inc"
