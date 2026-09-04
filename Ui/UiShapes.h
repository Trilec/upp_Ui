#ifndef _Ui_UiShapes_h_
#define _Ui_UiShapes_h_

/*
    UiShapes
    ========

    Purpose
    - Provide reusable parameterised authored silhouettes for controls,
      diagrams, charts, markers and future editors.
    - Keep named stock shapes above UiGeometry so adding a silhouette never
      creates another tessellation policy.
    - This is the preferred reusable silhouette layer for normal controls.
      Dense/high-count scenes may use UiGeometry directly when constructing a
      UiShapePath per item would be an avoidable allocation tax.

    All dimensions are final device/screen pixels. Apply DPI/view transforms
    before calling these builders. No stock builder owns a tessellation-quality
    setting; curve detail remains UiGeometry-owned.
*/

#include <Ui/UiShapePath.h>

namespace Upp {

class UiShapes {
public:
    static UiShapePath Polygon(const Vector<Pointf>& points, bool close = true);
    static UiShapePath RoundedPolygon(const Vector<Pointf>& points, double radius_px);

    static UiShapePath Rectangle(const Rectf& rect);
    static UiShapePath RoundedRectangle(const Rectf& rect, double radius_px);
    static UiShapePath Capsule(const Rectf& rect);
    static UiShapePath Ellipse(const Rectf& rect);

    static UiShapePath RegularPolygon(const Rectf& rect, int sides,
                                      double rotation_radians = -1.57079632679489661923);
    static UiShapePath Star(const Rectf& rect, int points,
                            double inner_ratio = 0.48,
                            double rotation_radians = -1.57079632679489661923);

    static UiShapePath Arrow(const Rectf& rect, UiShapeSide direction,
                             double head_ratio = 0.42,
                             double shaft_ratio = 0.38);
    static UiShapePath Chevron(const Rectf& rect, UiShapeSide direction,
                               double notch_ratio = 0.45);
    static UiShapePath ChamferedRectangle(const Rectf& rect, double chamfer_px);

    static UiShapePath Callout(const Rectf& body, UiShapeSide tail_side,
                               double tail_position = 0.5,
                               double tail_width_px = 16.0,
                               double tail_depth_px = 10.0,
                               double radius_px = 6.0);

    static UiShapePath Tag(const Rectf& rect, UiShapeSide point_side,
                           double point_depth_px = 14.0,
                           double hole_radius_px = 0.0);

    static UiShapePath Cloud(const Rectf& rect, double radius_px = 0.0);
    static UiShapePath Document(const Rectf& rect, double fold_ratio = 0.22,
                                double radius_px = 0.0);
    static UiShapePath Database(const Rectf& rect, double cap_ratio = 0.14,
                                double radius_px = 0.0);

    static UiShapePath RingSegment(Pointf center,
                                   double outer_radius_px,
                                   double inner_radius_px,
                                   double start_angle,
                                   double sweep_angle);
    static UiShapePath Pie(Pointf center, double radius_px,
                           double start_angle, double sweep_angle);
};

} // namespace Upp

#endif
