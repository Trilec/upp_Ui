#include <Ui/UiGeometry.h>

#include <cmath>
#include <climits>
#include <limits>

namespace Upp {
namespace {

double Sq(double v)
{
    return v * v;
}

double SqDistance(Pointf a, Pointf b)
{
    return Sq(a.x - b.x) + Sq(a.y - b.y);
}

bool Finite(Pointf p)
{
    return std::isfinite(p.x) && std::isfinite(p.y);
}

bool QuadraticNeedsSplit(Pointf p0, Pointf p1, Pointf p2, double tolerance2)
{
    Pointf d = p2 - p0;
    double q = d.x * d.x + d.y * d.y;
    if(q <= 1e-30)
        return SqDistance(p1, p0) > tolerance2;

    Pointf pd = p1 - p0;
    double u = (pd.x * d.x + pd.y * d.y) / q;
    if(u <= 0.0 || u >= 1.0)
        return true;

    Pointf projection(p0.x + d.x * u, p0.y + d.y * u);
    return SqDistance(p1, projection) > tolerance2;
}

bool CubicNeedsSplit(Pointf p0, Pointf p1, Pointf p2, Pointf p3,
                     double tolerance2)
{
    Pointf d = p3 - p0;
    double q = d.x * d.x + d.y * d.y;
    if(q <= 1e-30)
        return max(SqDistance(p1, p0), SqDistance(p2, p0)) > tolerance2;

    auto outside = [&](Pointf p) {
        Pointf pd = p - p0;
        double u = (pd.x * d.x + pd.y * d.y) / q;
        if(u <= 0.0 || u >= 1.0)
            return true;
        Pointf projection(p0.x + d.x * u, p0.y + d.y * u);
        return SqDistance(p, projection) > tolerance2;
    };
    return outside(p1) || outside(p2);
}

void AppendQuadraticRec(Vector<Pointf>& out,
                        Pointf p0, Pointf p1, Pointf p2,
                        double tolerance2, int depth)
{
    if(depth >= 16 || !QuadraticNeedsSplit(p0, p1, p2, tolerance2)) {
        out.Add(p2);
        return;
    }

    Pointf p01 = (p0 + p1) * 0.5;
    Pointf p12 = (p1 + p2) * 0.5;
    Pointf mid = (p01 + p12) * 0.5;
    AppendQuadraticRec(out, p0, p01, mid, tolerance2, depth + 1);
    AppendQuadraticRec(out, mid, p12, p2, tolerance2, depth + 1);
}

void AppendCubicRec(Vector<Pointf>& out,
                    Pointf p0, Pointf p1, Pointf p2, Pointf p3,
                    double tolerance2, int depth)
{
    if(depth >= 16 || !CubicNeedsSplit(p0, p1, p2, p3, tolerance2)) {
        out.Add(p3);
        return;
    }

    Pointf p01 = (p0 + p1) * 0.5;
    Pointf p12 = (p1 + p2) * 0.5;
    Pointf p23 = (p2 + p3) * 0.5;
    Pointf p012 = (p01 + p12) * 0.5;
    Pointf p123 = (p12 + p23) * 0.5;
    Pointf mid = (p012 + p123) * 0.5;

    AppendCubicRec(out, p0, p01, p012, mid, tolerance2, depth + 1);
    AppendCubicRec(out, mid, p123, p23, p3, tolerance2, depth + 1);
}

void SimplifyRange(const Vector<Pointf>& points, int first, int last,
                   double tolerance, Vector<byte>& keep)
{
    if(last <= first + 1)
        return;

    double farthest = -1.0;
    int index = -1;
    for(int i = first + 1; i < last; i++) {
        double d = UiGeometry::DistanceToSegment(points[i], points[first], points[last]);
        if(d > farthest) {
            farthest = d;
            index = i;
        }
    }

    if(index >= 0 && farthest > tolerance) {
        keep[index] = 1;
        SimplifyRange(points, first, index, tolerance, keep);
        SimplifyRange(points, index, last, tolerance, keep);
    }
}

} // namespace

bool UiGeometry::IsVisibleExtent(double extent_px)
{
    return std::isfinite(extent_px) && std::fabs(extent_px) >= VisibleExtentPx();
}

double UiGeometry::Length(Pointf vector)
{
    return std::sqrt(vector.x * vector.x + vector.y * vector.y);
}

Pointf UiGeometry::Normalize(Pointf vector)
{
    double length = Length(vector);
    return length > 1e-12 ? vector * (1.0 / length) : Pointf(0, 0);
}

double UiGeometry::PolylineLength(const Vector<Pointf>& points)
{
    double total = 0.0;
    for(int i = 1; i < points.GetCount(); i++)
        total += Length(points[i] - points[i - 1]);
    return total;
}

double UiGeometry::DistanceToPolyline(Pointf point, const Vector<Pointf>& points)
{
    if(points.IsEmpty())
        return std::numeric_limits<double>::infinity();
    if(points.GetCount() == 1)
        return Length(point - points[0]);

    double best = std::numeric_limits<double>::infinity();
    for(int i = 1; i < points.GetCount(); i++)
        best = min(best, DistanceToSegment(point, points[i - 1], points[i]));
    return best;
}

bool UiGeometry::ContainsEllipse(const Rect& rect, Pointf point)
{
    if(rect.IsEmpty())
        return false;
    double rx = rect.GetWidth() * 0.5;
    double ry = rect.GetHeight() * 0.5;
    if(rx <= 0.0 || ry <= 0.0)
        return false;
    double cx = (rect.left + rect.right) * 0.5;
    double cy = (rect.top + rect.bottom) * 0.5;
    double dx = (point.x - cx) / rx;
    double dy = (point.y - cy) / ry;
    return dx * dx + dy * dy <= 1.0 + 1e-12;
}

bool UiGeometry::ContainsRoundedRect(const Rect& rect, double radius_px, Pointf point)
{
    if(rect.IsEmpty() || point.x < rect.left || point.x > rect.right ||
       point.y < rect.top || point.y > rect.bottom)
        return false;

    double radius = min(max(0.0, radius_px),
                        max(0.0, min(rect.GetWidth(), rect.GetHeight()) * 0.5));
    if(radius <= 0.0001)
        return true;

    double inner_left = rect.left + radius;
    double inner_right = rect.right - radius;
    double inner_top = rect.top + radius;
    double inner_bottom = rect.bottom - radius;
    if(point.x >= inner_left && point.x <= inner_right)
        return true;
    if(point.y >= inner_top && point.y <= inner_bottom)
        return true;

    double cx = point.x < inner_left ? inner_left : inner_right;
    double cy = point.y < inner_top ? inner_top : inner_bottom;
    return Sq(point.x - cx) + Sq(point.y - cy) <= radius * radius + 1e-12;
}

bool UiGeometry::ContainsPolygon(const Vector<Pointf>& polygon, Pointf point)
{
    if(polygon.GetCount() < 3)
        return false;

    bool inside = false;
    for(int i = 0, j = polygon.GetCount() - 1; i < polygon.GetCount(); j = i++) {
        const Pointf& a = polygon[j];
        const Pointf& b = polygon[i];
        if(DistanceToSegment(point, a, b) <= 1e-9)
            return true;

        bool crosses = ((a.y > point.y) != (b.y > point.y)) &&
                       (point.x < (b.x - a.x) * (point.y - a.y) /
                                      ((b.y - a.y) == 0.0 ? 1e-30 : (b.y - a.y)) + a.x);
        if(crosses)
            inside = !inside;
    }
    return inside;
}

int UiGeometry::ArcSegments(double radius_px, double sweep_angle)
{
    const double radius = std::fabs(radius_px);
    const double sweep = std::fabs(sweep_angle);
    if(!std::isfinite(radius) || !std::isfinite(sweep) || sweep <= 0.000001)
        return 1;

    const double tau = 2.0 * 3.14159265358979323846;
    const int topology_min = sweep >= tau - 0.000001 ? 3 : 1;

    const double error = ErrorPx();
    if(radius <= error)
        return topology_min;

    // Circular chord sagitta:
    //   e = r * (1 - cos(theta / 2))
    // Solve for theta and choose the smallest equal-angle segment count.
    const double cosine = minmax(1.0 - error / radius, -1.0, 1.0);
    const double max_angle = 2.0 * std::acos(cosine);
    if(!std::isfinite(max_angle) || max_angle <= 0.000001)
        return topology_min;

    double required = std::ceil(sweep / max_angle);
    if(required >= (double)INT_MAX)
        return INT_MAX;
    return max(topology_min, (int)required);
}

int UiGeometry::EllipseSegments(double radius_x_px, double radius_y_px,
                                double sweep_angle)
{
    int segments = ArcSegments(max(std::fabs(radius_x_px), std::fabs(radius_y_px)),
                               sweep_angle);
    const double tau = 2.0 * 3.14159265358979323846;
    if(std::fabs(sweep_angle) >= tau - 0.000001)
        segments = max(3, segments);
    return segments;
}

void UiGeometry::AppendArc(Vector<Pointf>& out, Pointf center,
                           double radius_px, double start_angle,
                           double sweep_angle, bool include_start)
{
    if(!Finite(center) || !std::isfinite(radius_px) ||
       !std::isfinite(start_angle) || !std::isfinite(sweep_angle))
        return;

    double radius = std::fabs(radius_px);
    int segments = ArcSegments(radius, sweep_angle);
    int first = include_start ? 0 : 1;
    for(int i = first; i <= segments; i++) {
        double q = (double)i / (double)segments;
        double a = start_angle + sweep_angle * q;
        out.Add(Pointf(center.x + std::cos(a) * radius,
                       center.y + std::sin(a) * radius));
    }
}

void UiGeometry::AppendEllipse(Vector<Pointf>& out, Pointf center,
                               double radius_x_px, double radius_y_px,
                               double start_angle, double sweep_angle,
                               bool include_start)
{
    if(!Finite(center) || !std::isfinite(radius_x_px) ||
       !std::isfinite(radius_y_px) || !std::isfinite(start_angle) ||
       !std::isfinite(sweep_angle))
        return;

    double rx = std::fabs(radius_x_px);
    double ry = std::fabs(radius_y_px);
    int segments = EllipseSegments(rx, ry, sweep_angle);
    int first = include_start ? 0 : 1;
    for(int i = first; i <= segments; i++) {
        double q = (double)i / (double)segments;
        double a = start_angle + sweep_angle * q;
        out.Add(Pointf(center.x + std::cos(a) * rx,
                       center.y + std::sin(a) * ry));
    }
}

Vector<Pointf> UiGeometry::EllipsePath(const Rect& rect)
{
    Vector<Pointf> out;
    if(rect.IsEmpty())
        return out;
    Pointf center((rect.left + rect.right) * 0.5,
                  (rect.top + rect.bottom) * 0.5);
    double rx = max(0.0, rect.GetWidth() * 0.5);
    double ry = max(0.0, rect.GetHeight() * 0.5);
    AppendEllipse(out, center, rx, ry, 0.0,
                  2.0 * 3.14159265358979323846, true);
    if(out.GetCount() > 1 && SqDistance(out.Top(), out[0]) <= 0.0001)
        out.Drop();
    return out;
}

Vector<Pointf> UiGeometry::RoundedRectPath(const Rect& rect, double radius_px)
{
    Vector<Pointf> out;
    if(rect.IsEmpty())
        return out;

    double l = rect.left, t = rect.top, r = rect.right, b = rect.bottom;
    double radius = min(max(0.0, radius_px),
                        max(0.0, min(rect.GetWidth(), rect.GetHeight()) * 0.5));
    if(radius <= 0.0001) {
        out << Pointf(l, t) << Pointf(r, t) << Pointf(r, b) << Pointf(l, b);
        return out;
    }

    const double pi = 3.14159265358979323846;
    auto add_distinct = [&](Pointf p) {
        if(out.IsEmpty() || SqDistance(out.Top(), p) > 0.0001)
            out.Add(p);
    };

    add_distinct(Pointf(l + radius, t));
    add_distinct(Pointf(r - radius, t));
    AppendArc(out, Pointf(r - radius, t + radius), radius, -pi * 0.5, pi * 0.5);
    add_distinct(Pointf(r, b - radius));
    AppendArc(out, Pointf(r - radius, b - radius), radius, 0.0, pi * 0.5);
    add_distinct(Pointf(l + radius, b));
    AppendArc(out, Pointf(l + radius, b - radius), radius, pi * 0.5, pi * 0.5);
    add_distinct(Pointf(l, t + radius));
    AppendArc(out, Pointf(l + radius, t + radius), radius, pi, pi * 0.5);
    if(out.GetCount() > 1 && SqDistance(out.Top(), out[0]) <= 0.0001)
        out.Drop();
    return out;
}

Vector<Pointf> UiGeometry::CapsulePath(const Rect& rect)
{
    if(rect.IsEmpty())
        return Vector<Pointf>();
    return RoundedRectPath(rect, min(rect.GetWidth(), rect.GetHeight()) * 0.5);
}

Vector<Pointf> UiGeometry::ArcBandPath(Pointf center,
                                       double outer_radius_px,
                                       double inner_radius_px,
                                       double start_angle,
                                       double sweep_angle)
{
    Vector<Pointf> out;
    if(!Finite(center) || !std::isfinite(outer_radius_px) ||
       !std::isfinite(inner_radius_px) || !std::isfinite(start_angle) ||
       !std::isfinite(sweep_angle) || std::fabs(sweep_angle) <= 0.000001)
        return out;

    double outer = max(0.0, std::fabs(outer_radius_px));
    double inner = min(outer, max(0.0, std::fabs(inner_radius_px)));
    if(!IsVisibleExtent(outer))
        return out;

    const double tau = 2.0 * 3.14159265358979323846;
    const bool closed_sweep = std::fabs(sweep_angle) >= tau - 0.000001;

    Vector<Pointf> outer_path;
    AppendArc(outer_path, center, outer, start_angle, sweep_angle, true);
    for(const Pointf& p : outer_path)
        out.Add(p);

    if(inner >= VisibleExtentPx()) {
        Vector<Pointf> inner_path;
        AppendArc(inner_path, center, inner, start_angle + sweep_angle,
                  -sweep_angle, true);
        for(const Pointf& p : inner_path)
            out.Add(p);
    }
    else if(closed_sweep) {
        // A full disk has no radial seam. Keep one closed outer silhouette and
        // remove the duplicate endpoint emitted by the full sweep.
        if(out.GetCount() > 1 && SqDistance(out.Top(), out[0]) <= 0.0001)
            out.Drop();
    }
    else if(out.IsEmpty() || SqDistance(out.Top(), center) > 0.0001)
        out.Add(center);

    // Partial sectors need no repeated closing vertex because callers close the
    // contour. Full annuli keep both seam endpoints intentionally: a single
    // contour needs those exact radial bridges to represent the inner hole.
    if(!closed_sweep && out.GetCount() > 1 &&
       SqDistance(out.Top(), out[0]) <= 0.0001)
        out.Drop();
    return out;
}

Vector<Pointf> UiGeometry::PiePath(Pointf center, double radius_px,
                                   double start_angle, double sweep_angle)
{
    return ArcBandPath(center, radius_px, 0.0, start_angle, sweep_angle);
}

void UiGeometry::AppendQuadratic(Vector<Pointf>& out,
                                 Pointf p0, Pointf p1, Pointf p2,
                                 bool include_start)
{
    if(!Finite(p0) || !Finite(p1) || !Finite(p2))
        return;
    if(include_start)
        out.Add(p0);
    AppendQuadraticRec(out, p0, p1, p2, Sq(ErrorPx()), 0);
}

void UiGeometry::AppendCubic(Vector<Pointf>& out,
                             Pointf p0, Pointf p1, Pointf p2, Pointf p3,
                             bool include_start)
{
    if(!Finite(p0) || !Finite(p1) || !Finite(p2) || !Finite(p3))
        return;
    if(include_start)
        out.Add(p0);
    AppendCubicRec(out, p0, p1, p2, p3, Sq(ErrorPx()), 0);
}

Vector<Pointf> UiGeometry::RoundedPolyline(const Vector<Pointf>& points,
                                               double radius_px)
{
    if(points.GetCount() < 3 || radius_px <= 0.0001)
        return clone(points);

    Vector<Pointf> out;
    out.Add(points[0]);

    for(int i = 1; i + 1 < points.GetCount(); i++) {
        Pointf prev = points[i - 1];
        Pointf cur = points[i];
        Pointf next = points[i + 1];
        double a_len = std::sqrt(SqDistance(prev, cur));
        double b_len = std::sqrt(SqDistance(next, cur));
        double r = min(max(0.0, radius_px), min(a_len, b_len) * 0.45);
        if(r <= 0.0001) {
            if(SqDistance(out.Top(), cur) > 0.0001)
                out.Add(cur);
            continue;
        }

        Pointf entry = a_len > 1e-9 ? cur + (prev - cur) * (r / a_len) : cur;
        Pointf exit = b_len > 1e-9 ? cur + (next - cur) * (r / b_len) : cur;
        if(SqDistance(out.Top(), entry) > 0.0001)
            out.Add(entry);
        AppendQuadratic(out, entry, cur, exit);
    }

    if(out.IsEmpty() || SqDistance(out.Top(), points.Top()) > 0.0001)
        out.Add(points.Top());
    return out;
}

Vector<Pointf> UiGeometry::RoundedPolygon(const Vector<Pointf>& vertices,
                                              double radius_px)
{
    if(vertices.GetCount() < 3 || radius_px <= 0.0001)
        return clone(vertices);

    Vector<Pointf> out;
    for(int i = 0; i < vertices.GetCount(); i++) {
        Pointf prev = vertices[(i + vertices.GetCount() - 1) % vertices.GetCount()];
        Pointf cur = vertices[i];
        Pointf next = vertices[(i + 1) % vertices.GetCount()];
        double a_len = std::sqrt(SqDistance(prev, cur));
        double b_len = std::sqrt(SqDistance(next, cur));
        double r = min(max(0.0, radius_px), min(a_len, b_len) * 0.45);
        if(r <= 0.0001) {
            if(out.IsEmpty() || SqDistance(out.Top(), cur) > 0.0001)
                out.Add(cur);
            continue;
        }

        Pointf entry = a_len > 1e-9 ? cur + (prev - cur) * (r / a_len) : cur;
        Pointf exit = b_len > 1e-9 ? cur + (next - cur) * (r / b_len) : cur;
        if(out.IsEmpty() || SqDistance(out.Top(), entry) > 0.0001)
            out.Add(entry);
        AppendQuadratic(out, entry, cur, exit);
    }
    return out;
}

double UiGeometry::DistanceToSegment(Pointf p, Pointf a, Pointf b)
{
    Pointf d = b - a;
    double q = d.x * d.x + d.y * d.y;
    if(q <= 1e-30)
        return std::sqrt(SqDistance(p, a));

    Pointf ap = p - a;
    double t = (ap.x * d.x + ap.y * d.y) / q;
    t = minmax(t, 0.0, 1.0);
    Pointf nearest(a.x + d.x * t, a.y + d.y * t);
    return std::sqrt(SqDistance(p, nearest));
}

Vector<Pointf> UiGeometry::SimplifyPolyline(const Vector<Pointf>& points)
{
    if(points.GetCount() <= 2)
        return clone(points);

    Vector<byte> keep;
    keep.SetCount(points.GetCount());
    for(int i = 0; i < keep.GetCount(); i++)
        keep[i] = 0;
    keep[0] = keep.Top() = 1;

    SimplifyRange(points, 0, points.GetCount() - 1, ErrorPx(), keep);

    Vector<Pointf> out;
    out.Reserve(points.GetCount());
    for(int i = 0; i < points.GetCount(); i++)
        if(keep[i])
            out.Add(points[i]);
    return out;
}

} // namespace Upp
