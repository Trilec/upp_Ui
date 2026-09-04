#include <Ui/UiShapes.h>

#include <cmath>

namespace Upp {
namespace {

constexpr double SHAPE_PI = 3.14159265358979323846;
constexpr double SHAPE_TAU = 2.0 * SHAPE_PI;

double Width(const Rectf& rect)
{
    return rect.right - rect.left;
}

double Height(const Rectf& rect)
{
    return rect.bottom - rect.top;
}

bool Empty(const Rectf& rect)
{
    return Width(rect) <= 0.0 || Height(rect) <= 0.0;
}

Pointf Centre(const Rectf& rect)
{
    return Pointf((rect.left + rect.right) * 0.5,
                  (rect.top + rect.bottom) * 0.5);
}

Pointf MapNormalized(const Rectf& rect, Pointf normalized, UiShapeSide direction)
{
    double x = normalized.x;
    double y = normalized.y;
    switch(direction) {
    case UiShapeSide::Left:
        x = 1.0 - x;
        y = 1.0 - y;
        break;
    case UiShapeSide::Top: {
        double ox = x;
        x = y;
        y = 1.0 - ox;
        break;
    }
    case UiShapeSide::Bottom: {
        double ox = x;
        x = 1.0 - y;
        y = ox;
        break;
    }
    case UiShapeSide::Right:
    default:
        break;
    }
    return Pointf(rect.left + Width(rect) * x,
                  rect.top + Height(rect) * y);
}

Vector<Pointf> MapPolygon(const Rectf& rect,
                          const Vector<Pointf>& normalized,
                          UiShapeSide direction)
{
    Vector<Pointf> out;
    out.Reserve(normalized.GetCount());
    for(const Pointf& p : normalized)
        out.Add(MapNormalized(rect, p, direction));
    return out;
}

Pointf ArcPoint(Pointf center, double radius, double angle)
{
    return Pointf(center.x + std::cos(angle) * radius,
                  center.y + std::sin(angle) * radius);
}

void AddRoundedCorner(UiShapePath& path,
                      Pointf corner, Pointf before, Pointf after,
                      double radius, bool first)
{
    double len_before = UiGeometry::Length(before - corner);
    double len_after = UiGeometry::Length(after - corner);
    double r = min(max(0.0, radius), min(len_before, len_after) * 0.45);
    if(r <= 0.0001) {
        if(first)
            path.MoveTo(corner);
        else
            path.LineTo(corner);
        return;
    }

    Pointf entry = corner + (before - corner) * (r / max(len_before, 1e-12));
    Pointf exit = corner + (after - corner) * (r / max(len_after, 1e-12));
    if(first)
        path.MoveTo(entry);
    else
        path.LineTo(entry);
    path.QuadraticTo(corner, exit);
}

void AppendRoundedRectClockwise(UiShapePath& path, const Rectf& rect,
                                double radius)
{
    double w = Width(rect), h = Height(rect);
    radius = min(max(0.0, radius), max(0.0, min(w, h) * 0.5));
    double l = rect.left, t = rect.top, r = rect.right, b = rect.bottom;

    if(radius <= 0.0001) {
        path.MoveTo(Pointf(l, t))
            .LineTo(Pointf(r, t))
            .LineTo(Pointf(r, b))
            .LineTo(Pointf(l, b))
            .Close();
        return;
    }

    path.MoveTo(Pointf(l + radius, t))
        .LineTo(Pointf(r - radius, t))
        .Arc(Pointf(r - radius, t + radius), radius,
             -SHAPE_PI * 0.5, SHAPE_PI * 0.5)
        .LineTo(Pointf(r, b - radius))
        .Arc(Pointf(r - radius, b - radius), radius,
             0.0, SHAPE_PI * 0.5)
        .LineTo(Pointf(l + radius, b))
        .Arc(Pointf(l + radius, b - radius), radius,
             SHAPE_PI * 0.5, SHAPE_PI * 0.5)
        .LineTo(Pointf(l, t + radius))
        .Arc(Pointf(l + radius, t + radius), radius,
             SHAPE_PI, SHAPE_PI * 0.5)
        .Close();
}

} // namespace

UiShapePath UiShapes::Polygon(const Vector<Pointf>& points, bool close)
{
    UiShapePath path;
    if(points.IsEmpty())
        return path;

    path.MoveTo(points[0]);
    for(int i = 1; i < points.GetCount(); i++)
        path.LineTo(points[i]);
    if(close)
        path.Close();
    return path;
}

UiShapePath UiShapes::RoundedPolygon(const Vector<Pointf>& points, double radius_px)
{
    if(points.GetCount() < 3 || radius_px <= 0.0001)
        return Polygon(points, true);

    UiShapePath path;
    int n = points.GetCount();
    for(int i = 0; i < n; i++) {
        Pointf before = points[(i + n - 1) % n];
        Pointf corner = points[i];
        Pointf after = points[(i + 1) % n];
        AddRoundedCorner(path, corner, before, after, radius_px, i == 0);
    }
    path.Close();
    return path;
}

UiShapePath UiShapes::Rectangle(const Rectf& rect)
{
    UiShapePath path;
    if(Empty(rect))
        return path;
    path.MoveTo(Pointf(rect.left, rect.top))
        .LineTo(Pointf(rect.right, rect.top))
        .LineTo(Pointf(rect.right, rect.bottom))
        .LineTo(Pointf(rect.left, rect.bottom))
        .Close();
    return path;
}

UiShapePath UiShapes::RoundedRectangle(const Rectf& rect, double radius_px)
{
    UiShapePath path;
    if(Empty(rect))
        return path;

    AppendRoundedRectClockwise(path, rect, radius_px);
    return path;
}

UiShapePath UiShapes::Capsule(const Rectf& rect)
{
    return RoundedRectangle(rect, min(Width(rect), Height(rect)) * 0.5);
}

UiShapePath UiShapes::Ellipse(const Rectf& rect)
{
    UiShapePath path;
    if(Empty(rect))
        return path;

    Pointf center = Centre(rect);
    double rx = Width(rect) * 0.5;
    double ry = Height(rect) * 0.5;
    path.MoveTo(Pointf(center.x + rx, center.y))
        .EllipseArc(center, rx, ry, 0.0, SHAPE_TAU)
        .Close();
    return path;
}

UiShapePath UiShapes::RegularPolygon(const Rectf& rect, int sides,
                                     double rotation_radians)
{
    UiShapePath path;
    if(Empty(rect) || sides < 3)
        return path;

    Pointf center = Centre(rect);
    double radius = min(Width(rect), Height(rect)) * 0.5;
    Vector<Pointf> points;
    points.Reserve(sides);
    for(int i = 0; i < sides; i++) {
        double a = rotation_radians + SHAPE_TAU * i / sides;
        points.Add(Pointf(center.x + std::cos(a) * radius,
                          center.y + std::sin(a) * radius));
    }
    return Polygon(points, true);
}

UiShapePath UiShapes::Star(const Rectf& rect, int points,
                           double inner_ratio, double rotation_radians)
{
    UiShapePath path;
    if(Empty(rect) || points < 2)
        return path;

    inner_ratio = minmax(inner_ratio, 0.05, 0.95);
    Pointf center = Centre(rect);
    double outer = min(Width(rect), Height(rect)) * 0.5;
    double inner = outer * inner_ratio;

    Vector<Pointf> vertices;
    vertices.Reserve(points * 2);
    for(int i = 0; i < points * 2; i++) {
        double a = rotation_radians + SHAPE_PI * i / points;
        double radius = (i & 1) ? inner : outer;
        vertices.Add(Pointf(center.x + std::cos(a) * radius,
                            center.y + std::sin(a) * radius));
    }
    return Polygon(vertices, true);
}

UiShapePath UiShapes::Arrow(const Rectf& rect, UiShapeSide direction,
                            double head_ratio, double shaft_ratio)
{
    if(Empty(rect))
        return UiShapePath();

    head_ratio = minmax(head_ratio, 0.15, 0.80);
    shaft_ratio = minmax(shaft_ratio, 0.08, 0.90);
    double half_shaft = shaft_ratio * 0.5;

    Vector<Pointf> normalized;
    normalized << Pointf(0.0, 0.5 - half_shaft)
               << Pointf(1.0 - head_ratio, 0.5 - half_shaft)
               << Pointf(1.0 - head_ratio, 0.0)
               << Pointf(1.0, 0.5)
               << Pointf(1.0 - head_ratio, 1.0)
               << Pointf(1.0 - head_ratio, 0.5 + half_shaft)
               << Pointf(0.0, 0.5 + half_shaft);
    return Polygon(MapPolygon(rect, normalized, direction), true);
}

UiShapePath UiShapes::Chevron(const Rectf& rect, UiShapeSide direction,
                              double notch_ratio)
{
    if(Empty(rect))
        return UiShapePath();

    notch_ratio = minmax(notch_ratio, 0.15, 0.80);
    Vector<Pointf> normalized;
    normalized << Pointf(0.0, 0.0)
               << Pointf(1.0 - notch_ratio, 0.0)
               << Pointf(1.0, 0.5)
               << Pointf(1.0 - notch_ratio, 1.0)
               << Pointf(0.0, 1.0)
               << Pointf(notch_ratio, 0.5);
    return Polygon(MapPolygon(rect, normalized, direction), true);
}

UiShapePath UiShapes::ChamferedRectangle(const Rectf& rect, double chamfer_px)
{
    if(Empty(rect))
        return UiShapePath();

    double c = min(max(0.0, chamfer_px),
                   max(0.0, min(Width(rect), Height(rect)) * 0.5));
    Vector<Pointf> points;
    points << Pointf(rect.left + c, rect.top)
           << Pointf(rect.right - c, rect.top)
           << Pointf(rect.right, rect.top + c)
           << Pointf(rect.right, rect.bottom - c)
           << Pointf(rect.right - c, rect.bottom)
           << Pointf(rect.left + c, rect.bottom)
           << Pointf(rect.left, rect.bottom - c)
           << Pointf(rect.left, rect.top + c);
    return Polygon(points, true);
}

UiShapePath UiShapes::Callout(const Rectf& body, UiShapeSide tail_side,
                              double tail_position,
                              double tail_width_px,
                              double tail_depth_px,
                              double radius_px)
{
    UiShapePath path;
    if(Empty(body))
        return path;

    tail_position = minmax(tail_position, 0.0, 1.0);
    tail_width_px = max(0.0, tail_width_px);
    tail_depth_px = max(0.0, tail_depth_px);

    auto add_tail = [&](UiShapeSide side, Pointf from, Pointf to) {
        if(side != tail_side || tail_width_px <= 0.0 || tail_depth_px <= 0.0)
            return;

        bool horizontal = side == UiShapeSide::Top || side == UiShapeSide::Bottom;
        double length = horizontal ? std::fabs(to.x - from.x) : std::fabs(to.y - from.y);
        double half = min(tail_width_px * 0.5, length * 0.45);
        if(half <= 0.0)
            return;

        if(horizontal) {
            double lo = min(from.x, to.x) + half;
            double hi = max(from.x, to.x) - half;
            double center = lo <= hi ? lo + (hi - lo) * tail_position
                                     : (from.x + to.x) * 0.5;
            bool forward = to.x >= from.x;
            Pointf first(center + (forward ? -half : half), from.y);
            Pointf second(center + (forward ? half : -half), from.y);
            double tip_y = from.y + (side == UiShapeSide::Top ? -tail_depth_px
                                                               : tail_depth_px);
            path.LineTo(first).LineTo(Pointf(center, tip_y)).LineTo(second);
        }
        else {
            double lo = min(from.y, to.y) + half;
            double hi = max(from.y, to.y) - half;
            double center = lo <= hi ? lo + (hi - lo) * tail_position
                                     : (from.y + to.y) * 0.5;
            bool forward = to.y >= from.y;
            Pointf first(from.x, center + (forward ? -half : half));
            Pointf second(from.x, center + (forward ? half : -half));
            double tip_x = from.x + (side == UiShapeSide::Left ? -tail_depth_px
                                                                : tail_depth_px);
            path.LineTo(first).LineTo(Pointf(tip_x, center)).LineTo(second);
        }
    };

    double w = Width(body), h = Height(body);
    double radius = min(max(0.0, radius_px),
                        max(0.0, min(w, h) * 0.5));
    double l = body.left, t = body.top, r = body.right, b = body.bottom;

    if(radius <= 0.0001) {
        path.MoveTo(Pointf(l, t));
        add_tail(UiShapeSide::Top, Pointf(l, t), Pointf(r, t));
        path.LineTo(Pointf(r, t));
        add_tail(UiShapeSide::Right, Pointf(r, t), Pointf(r, b));
        path.LineTo(Pointf(r, b));
        add_tail(UiShapeSide::Bottom, Pointf(r, b), Pointf(l, b));
        path.LineTo(Pointf(l, b));
        add_tail(UiShapeSide::Left, Pointf(l, b), Pointf(l, t));
        path.Close();
        return path;
    }

    path.MoveTo(Pointf(l + radius, t));
    add_tail(UiShapeSide::Top,
             Pointf(l + radius, t), Pointf(r - radius, t));
    path.LineTo(Pointf(r - radius, t))
        .Arc(Pointf(r - radius, t + radius), radius,
             -SHAPE_PI * 0.5, SHAPE_PI * 0.5);

    add_tail(UiShapeSide::Right,
             Pointf(r, t + radius), Pointf(r, b - radius));
    path.LineTo(Pointf(r, b - radius))
        .Arc(Pointf(r - radius, b - radius), radius,
             0.0, SHAPE_PI * 0.5);

    add_tail(UiShapeSide::Bottom,
             Pointf(r - radius, b), Pointf(l + radius, b));
    path.LineTo(Pointf(l + radius, b))
        .Arc(Pointf(l + radius, b - radius), radius,
             SHAPE_PI * 0.5, SHAPE_PI * 0.5);

    add_tail(UiShapeSide::Left,
             Pointf(l, b - radius), Pointf(l, t + radius));
    path.LineTo(Pointf(l, t + radius))
        .Arc(Pointf(l + radius, t + radius), radius,
             SHAPE_PI, SHAPE_PI * 0.5)
        .Close();
    return path;
}

UiShapePath UiShapes::Tag(const Rectf& rect, UiShapeSide point_side,
                          double point_depth_px, double hole_radius_px)
{
    if(Empty(rect))
        return UiShapePath();

    bool horizontal = point_side == UiShapeSide::Left || point_side == UiShapeSide::Right;
    double main_extent = horizontal ? Width(rect) : Height(rect);
    point_depth_px = min(max(0.0, point_depth_px), max(0.0, main_extent * 0.5));

    Vector<Pointf> normalized;
    double depth_ratio = main_extent > 0.0 ? point_depth_px / main_extent : 0.0;
    normalized << Pointf(0.0, 0.0)
               << Pointf(1.0 - depth_ratio, 0.0)
               << Pointf(1.0, 0.5)
               << Pointf(1.0 - depth_ratio, 1.0)
               << Pointf(0.0, 1.0);
    Vector<Pointf> points = MapPolygon(rect, normalized, point_side);
    UiShapePath path;
    if(!points.IsEmpty()) {
        path.MoveTo(points[0]);
        for(int i = 1; i < points.GetCount(); i++)
            path.LineTo(points[i]);
        path.Close();
    }

    if(hole_radius_px > 0.0) {
        Pointf hole = MapNormalized(rect, Pointf(0.18, 0.5), point_side);
        double hr = min(hole_radius_px, min(Width(rect), Height(rect)) * 0.20);
        if(hr >= UiGeometry::VisibleExtentPx()) {
            path.MoveTo(Pointf(hole.x + hr, hole.y))
                .Arc(hole, hr, 0.0, -SHAPE_TAU)
                .Close();
        }
    }
    return path;
}

UiShapePath UiShapes::Cloud(const Rectf& rect, double radius_px)
{
    if(Empty(rect))
        return UiShapePath();

    double w = Width(rect), h = Height(rect);
    Vector<Pointf> points;
    points << Pointf(rect.left + w * 0.08, rect.top + h * 0.62)
           << Pointf(rect.left + w * 0.05, rect.top + h * 0.42)
           << Pointf(rect.left + w * 0.20, rect.top + h * 0.28)
           << Pointf(rect.left + w * 0.36, rect.top + h * 0.30)
           << Pointf(rect.left + w * 0.45, rect.top + h * 0.10)
           << Pointf(rect.left + w * 0.66, rect.top + h * 0.12)
           << Pointf(rect.left + w * 0.75, rect.top + h * 0.30)
           << Pointf(rect.left + w * 0.92, rect.top + h * 0.36)
           << Pointf(rect.left + w * 0.95, rect.top + h * 0.60)
           << Pointf(rect.left + w * 0.82, rect.top + h * 0.80)
           << Pointf(rect.left + w * 0.18, rect.top + h * 0.82);
    return RoundedPolygon(points, max(radius_px, min(w, h) * 0.10));
}

UiShapePath UiShapes::Document(const Rectf& rect, double fold_ratio,
                               double radius_px)
{
    if(Empty(rect))
        return UiShapePath();

    fold_ratio = minmax(fold_ratio, 0.05, 0.45);
    double fold = min(Width(rect), Height(rect)) * fold_ratio;
    Vector<Pointf> points;
    points << Pointf(rect.left, rect.top)
           << Pointf(rect.right - fold, rect.top)
           << Pointf(rect.right, rect.top + fold)
           << Pointf(rect.right, rect.bottom)
           << Pointf(rect.left, rect.bottom);
    return RoundedPolygon(points, radius_px);
}

UiShapePath UiShapes::Database(const Rectf& rect, double cap_ratio,
                               double radius_px)
{
    if(Empty(rect))
        return UiShapePath();

    cap_ratio = minmax(cap_ratio, 0.04, 0.30);
    double w = Width(rect), h = Height(rect);
    double cap = h * cap_ratio;
    Vector<Pointf> points;
    points << Pointf(rect.left, rect.top + cap)
           << Pointf(rect.left + w * 0.18, rect.top)
           << Pointf(rect.right - w * 0.18, rect.top)
           << Pointf(rect.right, rect.top + cap)
           << Pointf(rect.right, rect.bottom - cap)
           << Pointf(rect.right - w * 0.18, rect.bottom)
           << Pointf(rect.left + w * 0.18, rect.bottom)
           << Pointf(rect.left, rect.bottom - cap);
    return RoundedPolygon(points, max(radius_px, h * 0.08));
}

UiShapePath UiShapes::RingSegment(Pointf center,
                                  double outer_radius_px,
                                  double inner_radius_px,
                                  double start_angle,
                                  double sweep_angle)
{
    UiShapePath path;
    double outer = max(0.0, std::fabs(outer_radius_px));
    double inner = min(outer, max(0.0, std::fabs(inner_radius_px)));
    if(!UiGeometry::IsVisibleExtent(outer) || std::fabs(sweep_angle) <= 1e-9)
        return path;

    double direction = sweep_angle < 0.0 ? -1.0 : 1.0;
    sweep_angle = direction * min(std::fabs(sweep_angle), SHAPE_TAU);
    bool full = std::fabs(sweep_angle) >= SHAPE_TAU - 1e-6;
    Pointf outer_start = ArcPoint(center, outer, start_angle);

    if(full && inner >= UiGeometry::VisibleExtentPx()) {
        path.MoveTo(outer_start)
            .Arc(center, outer, start_angle, sweep_angle)
            .Close();
        Pointf inner_start = ArcPoint(center, inner, start_angle + sweep_angle);
        path.MoveTo(inner_start)
            .Arc(center, inner, start_angle + sweep_angle, -sweep_angle)
            .Close();
        return path;
    }

    path.MoveTo(outer_start)
        .Arc(center, outer, start_angle, sweep_angle);

    if(inner >= UiGeometry::VisibleExtentPx()) {
        Pointf inner_end = ArcPoint(center, inner, start_angle + sweep_angle);
        path.LineTo(inner_end)
            .Arc(center, inner, start_angle + sweep_angle, -sweep_angle);
    }
    else if(!full)
        path.LineTo(center);

    path.Close();
    return path;
}

UiShapePath UiShapes::Pie(Pointf center, double radius_px,
                          double start_angle, double sweep_angle)
{
    return RingSegment(center, radius_px, 0.0, start_angle, sweep_angle);
}

} // namespace Upp
