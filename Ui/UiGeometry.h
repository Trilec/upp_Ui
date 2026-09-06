#ifndef _Ui_UiGeometry_h_
#define _Ui_UiGeometry_h_

/*
    UiGeometry
    ==========

    Purpose
    - Define the first-class screen-space geometry contract for upp_Ui.
    - Centralise explicit tessellation so controls never invent arbitrary
      sample counts or generate detail that cannot affect the final image.
    - Keep geometry independent of colour, theme, Ctrl state, raster caches and
      any particular Draw/Painter backend.

    Contract
    - All coordinates accepted here are FINAL DEVICE/SCREEN PIXELS.
    - Authored units, DPI scaling and view transforms happen before these calls.
    - Explicit curve flattening targets a library-wide positional error of
      0.35 screen pixels inside the documented numeric/work envelope.
    - Callers that need proof across adversarial inputs can request
      TessellationStatus; limits are never silently reported as tolerance PASS.
    - Prefer native Draw/Painter primitives. Normal controls that need a
      reusable authored silhouette should use UiShapes / UiShapePath above this
      layer. Use UiGeometry directly when explicit points/math are genuinely
      required.
    - Dense/high-count scenes (UiNodeGraph is the canonical example) may use
      UiGeometry directly when an intermediate UiShapePath would add unnecessary
      allocation/work.
    - Authored discrete vertices are never simplified implicitly.
*/

#include <Draw/Draw.h>

namespace Upp {

class UiGeometry {
public:
    static double ErrorPx()          { return 0.35; }
    static double VisibleExtentPx()  { return 0.50; }
    static int    MaxCurveSegments() { return 65536; }
    static int    MaxSubdivisionDepth() { return 24; }
    static double MaxCoordinatePx()  { return 1.0e9; }

    struct TessellationStatus {
        bool input_supported = true;
        bool tolerance_met = true;
        bool work_limited = false;
        int  segments = 0;

        bool IsExactContract() const {
            return input_supported && tolerance_met && !work_limited;
        }
    };

    // Presentation significance helper for generated detail. This is a shared
    // policy threshold, not a mathematical claim that smaller coverage is
    // incapable of influencing an antialiased pixel.
    static bool IsVisibleExtent(double extent_px);

    // Basic screen-space line/vector math shared by routing, hit testing,
    // gauges and future chart/diagram controls.
    static double Length(Pointf vector);
    static Pointf Normalize(Pointf vector);
    static double PolylineLength(const Vector<Pointf>& points);
    // Point at a clamped 0..1 fraction of visible polyline arc length.
    // This is deliberately independent of vertex count/tessellation density.
    static Pointf PointAtPolylineFraction(const Vector<Pointf>& points, double fraction);
    static double DistanceToPolyline(Pointf point, const Vector<Pointf>& points);

    // Analytic/general containment. These avoid manufacturing a dense path just
    // to answer a hit-test question.
    static bool ContainsEllipse(const Rect& rect, Pointf point);
    static bool ContainsRoundedRect(const Rect& rect, double radius_px, Pointf point);
    static bool ContainsPolygon(const Vector<Pointf>& polygon, Pointf point);

    // Smallest equal-angle segment count whose circular sagitta stays inside
    // ErrorPx(). radius/sweep are final pixel-space values.
    static int ArcSegments(double radius_px, double sweep_angle,
                           TessellationStatus *status);
    static int ArcSegments(double radius_px, double sweep_angle)
    {
        return ArcSegments(radius_px, sweep_angle, nullptr);
    }

    // Conservative ellipse equivalent. max(rx, ry) bounds the parametric
    // curvature error. Closed ellipses enforce only the minimum topology
    // necessary to remain a closed silhouette; callers do not choose quality.
    static int EllipseSegments(double radius_x_px, double radius_y_px,
                               double sweep_angle, TessellationStatus *status);
    static int EllipseSegments(double radius_x_px, double radius_y_px,
                               double sweep_angle)
    {
        return EllipseSegments(radius_x_px, radius_y_px, sweep_angle, nullptr);
    }

    // Append explicit screen-space geometry. By default the start point is not
    // appended so callers can concatenate arcs without duplicate vertices.
    static void AppendArc(Vector<Pointf>& out, Pointf center,
                          double radius_px, double start_angle,
                          double sweep_angle, bool include_start = false,
                          TessellationStatus *status = nullptr);

    static void AppendEllipse(Vector<Pointf>& out, Pointf center,
                              double radius_x_px, double radius_y_px,
                              double start_angle, double sweep_angle,
                              bool include_start = false,
                              TessellationStatus *status = nullptr);

    // Common explicit stock silhouettes. Prefer native Draw/Painter shapes for
    // paint-only use; these exist for hit-testing, clipping, routing or custom
    // backends that genuinely need points.
    static Vector<Pointf> EllipsePath(const Rect& rect);
    static Vector<Pointf> RoundedRectPath(const Rect& rect, double radius_px);
    static Vector<Pointf> CapsulePath(const Rect& rect);

    // Circular radial areas for gauges, pie/donut charts and polar controls.
    // ArcBandPath is a fill-oriented single-polyline helper. A full annulus
    // necessarily carries radial bridge/seam topology in that representation;
    // use UiShapes::RingSegment (separate outer/inner contours) for a reusable
    // annulus that may also be stroked.
    static Vector<Pointf> ArcBandPath(Pointf center,
                                      double outer_radius_px,
                                      double inner_radius_px,
                                      double start_angle,
                                      double sweep_angle);
    static Vector<Pointf> PiePath(Pointf center, double radius_px,
                                  double start_angle, double sweep_angle);

    // Adaptive De Casteljau flattening in final pixel space. These functions
    // append only as much geometry as ErrorPx() requires.
    static void AppendQuadratic(Vector<Pointf>& out,
                                Pointf p0, Pointf p1, Pointf p2,
                                bool include_start = false,
                                TessellationStatus *status = nullptr);

    static void AppendCubic(Vector<Pointf>& out,
                            Pointf p0, Pointf p1, Pointf p2, Pointf p3,
                            bool include_start = false,
                            TessellationStatus *status = nullptr);

    // Shared corner construction for explicit geometry. RoundedPolyline keeps
    // the first/last point; RoundedPolygon treats the input as a closed shape.
    // Radius is final screen pixels and each rounded corner is flattened by the
    // same ErrorPx() contract.
    static Vector<Pointf> RoundedPolyline(const Vector<Pointf>& points,
                                          double radius_px);
    static Vector<Pointf> RoundedPolygon(const Vector<Pointf>& vertices,
                                         double radius_px);

    // For generated/sample polylines only. Authored polygon vertices should
    // remain authored. This function spends the full ErrorPx() allowance, so
    // do not apply it after another full-budget flattening while claiming a
    // combined ErrorPx() end-to-end bound. Endpoints are always preserved.
    static Vector<Pointf> SimplifyPolyline(const Vector<Pointf>& points);

    static double DistanceToSegment(Pointf p, Pointf a, Pointf b);
};

} // namespace Upp

#endif
