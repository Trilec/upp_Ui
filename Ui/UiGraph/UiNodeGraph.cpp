#include <Ui/UiGraph/UiNodeGraph.h>
#include <Ui/Ui.h>
#include <Ui/UiRenderLayer.h>

// Preserve the validated retained implementation in this translation unit,
// but keep the active render policy separate. The retained source still owns
// model/spatial/interaction/geometry behaviour; UiNodeGraphRender.inc owns the
// current software paint policy so render experiments do not keep growing this
// staging wrapper.
//
// R10A keeps the retained implementation source-compatible with its historical
// enum vocabulary while canonical authored code uses Rectangle/Ellipse. Mapping
// old Rectangle to wire-0 LegacyRectangle here is important: canonical Rectangle
// has a distinct wire value and owns corner_radius semantics.
//
// The retained implementation historically called RefreshLayout() after view,
// model and style methods that had already synchronously rebuilt geometry and
// attached controls. Layout() then invalidated that fresh geometry and rebuilt it
// again. Keep layout scheduling only for the one case that genuinely requires a
// later Layout pass: pending first-paint auto-fit. Natural host resize Layout
// callbacks are unaffected by this source-level compatibility shim.
#define RefreshLayout() do { if(auto_fit_first_paint_ && !first_paint_done_) Ctrl::RefreshLayout(); } while(0)
#define Rectangle                    LegacyRectangle
#define UsesRectangularStyledSurface UsesRectangularStyledSurfaceLegacy
#define Paint                         PaintLegacy
#define PaintGraphGeometry            PaintGraphGeometryLegacy
#define PaintNodeDetails              PaintNodeDetailsLegacy
#include "UiNodeGraphBase.inc"
#undef PaintNodeDetails
#undef PaintGraphGeometry
#undef Paint
#undef UsesRectangularStyledSurface
#undef Rectangle
#undef RefreshLayout

namespace Upp {
namespace UiNodeGraphRenderMath {

// Direct dashed/dotted drawing advances a floating phase to exact pattern
// boundaries. Binary rounding can leave the remaining step below the ULP of both
// phase and segment position; the caller then asks fmod() with exactly the same
// phase forever. Detect that no-progress signature without perturbing ordinary
// remainder math. Two identical calls are allowed (the dot grid legitimately asks
// for X/Y phases that can match); a third identical call inside the same short
// execution burst snaps to cycle start, guaranteeing the patterned loop can make
// a real step. Frame-to-frame repeats reset by the time window.
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

// UiNodeGraphRender.inc currently uses std::fmod only for direct dash phase and
// cached-grid phase. Redirect those local calls to the no-progress guard above
// without changing the rest of the translation unit or public API.
#define std UiNodeGraphRenderMath
#include "UiNodeGraphRender.inc"
#undef std
