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
// boundaries. Binary rounding can otherwise leave the remaining step smaller
// than the ULP of both phase and segment position, producing a non-progressing
// while loop. Keep the active render include on a remainder function that nudges
// positive phase values by a sub-pixel epsilon. This is far below raster precision
// but guarantees forward progress at dash boundaries. Negative grid phases retain
// normal fmod semantics because the grid code deliberately normalizes them later.
inline double fmod(double value, double period)
{
    if(!(period > 0.0) || !std::isfinite(value) || !std::isfinite(period))
        return 0.0;
    double r = std::fmod(value, period);
    if(r <= 0.0)
        return r;
    const double eps = max(1e-9, fabs(period) * 1e-12);
    if(r <= eps || period - r <= eps)
        return 0.0;
    r += eps;
    return r < period ? r : 0.0;
}

} // namespace UiNodeGraphRenderMath
} // namespace Upp

// UiNodeGraphRender.inc currently uses std::fmod only for direct dash phase and
// cached-grid phase. Redirect those local calls to the progress-safe remainder
// above without changing the rest of the translation unit or public API.
#define std UiNodeGraphRenderMath
#include "UiNodeGraphRender.inc"
#undef std
