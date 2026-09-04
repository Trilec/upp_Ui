#include <Ui/Ui.h>

#include <cmath>

using namespace Upp;

namespace {

constexpr double PI = 3.14159265358979323846;

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const String& text)
    {
        checks++;
        Cout() << (ok ? "PASS: " : "FAIL: ") << text << '\n';
        if(!ok)
            fails++;
    }
};

bool Near(Pointf a, Pointf b, double eps = 1e-6)
{
    return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps;
}

double Sagitta(double radius, double sweep, int segments)
{
    double theta = std::fabs(sweep) / max(1, segments);
    return radius * (1.0 - std::cos(theta * 0.5));
}

Pointf QuadraticPoint(Pointf p0, Pointf p1, Pointf p2, double t)
{
    double u = 1.0 - t;
    return Pointf(u * u * p0.x + 2.0 * u * t * p1.x + t * t * p2.x,
                  u * u * p0.y + 2.0 * u * t * p1.y + t * t * p2.y);
}

Pointf CubicPoint(Pointf p0, Pointf p1, Pointf p2, Pointf p3, double t)
{
    double u = 1.0 - t;
    return Pointf(u*u*u*p0.x + 3*u*u*t*p1.x + 3*u*t*t*p2.x + t*t*t*p3.x,
                  u*u*u*p0.y + 3*u*u*t*p1.y + 3*u*t*t*p2.y + t*t*t*p3.y);
}

double DistanceToPolyline(Pointf p, const Vector<Pointf>& poly)
{
    if(poly.IsEmpty())
        return 1e100;
    if(poly.GetCount() == 1)
        return UiGeometry::DistanceToSegment(p, poly[0], poly[0]);
    double best = 1e100;
    for(int i = 1; i < poly.GetCount(); i++)
        best = min(best, UiGeometry::DistanceToSegment(p, poly[i - 1], poly[i]));
    return best;
}

double MaxQuadraticError(Pointf p0, Pointf p1, Pointf p2,
                         const Vector<Pointf>& poly)
{
    double worst = 0.0;
    for(int i = 0; i <= 2000; i++) {
        double t = (double)i / 2000.0;
        worst = max(worst, DistanceToPolyline(QuadraticPoint(p0, p1, p2, t), poly));
    }
    return worst;
}

double MaxCubicError(Pointf p0, Pointf p1, Pointf p2, Pointf p3,
                     const Vector<Pointf>& poly)
{
    double worst = 0.0;
    for(int i = 0; i <= 3000; i++) {
        double t = (double)i / 3000.0;
        worst = max(worst, DistanceToPolyline(CubicPoint(p0, p1, p2, p3, t), poly));
    }
    return worst;
}

void PrintArc(double radius, double sweep)
{
    int n = UiGeometry::ArcSegments(radius, sweep);
    Cout() << "UI_GEOMETRY_PROFILE primitive=arc"
           << " radius_px=" << radius
           << " sweep=" << sweep
           << " segments=" << n
           << " sagitta_px=" << Sagitta(radius, sweep, n)
           << '\n';
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;

    t.Expect(std::fabs(UiGeometry::ErrorPx() - 0.35) < 1e-12,
             "one library-wide explicit geometry error budget is 0.35 device pixels");
    t.Expect(!UiGeometry::IsVisibleExtent(0.49) && UiGeometry::IsVisibleExtent(0.50),
             "pixel significance is expressed in final device pixels");

    int arc4 = UiGeometry::ArcSegments(4.0, 2.0 * PI);
    int arc10 = UiGeometry::ArcSegments(10.0, 2.0 * PI);
    int arc100 = UiGeometry::ArcSegments(100.0, 2.0 * PI);
    t.Expect(arc4 < arc10 && arc10 < arc100,
             "circle detail grows only with projected pixel radius");
    t.Expect(Sagitta(4.0, 2.0 * PI, arc4) <= UiGeometry::ErrorPx() + 1e-9
             && Sagitta(10.0, 2.0 * PI, arc10) <= UiGeometry::ErrorPx() + 1e-9
             && Sagitta(100.0, 2.0 * PI, arc100) <= UiGeometry::ErrorPx() + 1e-9,
             "adaptive circles remain inside the 0.35px sagitta contract");

    int quarter100 = UiGeometry::ArcSegments(100.0, PI * 0.5);
    t.Expect(quarter100 < arc100,
             "short arc sweep does not pay full-circle geometry cost");
    t.Expect(UiGeometry::EllipseSegments(100.0, 20.0, 2.0 * PI) == arc100,
             "ellipse tessellation is conservatively bounded by projected major radius");

    Vector<Pointf> arc;
    UiGeometry::AppendArc(arc, Pointf(50, 50), 10.0, 0.0, PI * 0.5, true);
    t.Expect(arc.GetCount() == UiGeometry::ArcSegments(10.0, PI * 0.5) + 1
             && Near(arc[0], Pointf(60, 50))
             && Near(arc.Top(), Pointf(50, 60), 1e-5),
             "explicit arc builder preserves exact endpoints and adaptive count");

    Pointf q0(0, 0), q1(50, 100), q2(100, 0);
    Vector<Pointf> q;
    UiGeometry::AppendQuadratic(q, q0, q1, q2, true);
    double qerr = MaxQuadraticError(q0, q1, q2, q);
    t.Expect(Near(q[0], q0) && Near(q.Top(), q2),
             "quadratic flattening preserves endpoints");
    t.Expect(qerr <= UiGeometry::ErrorPx() + 0.03,
             "quadratic flattening remains within the screen-space contract");

    Pointf c0(0, 0), c1(40, 160), c2(160, -80), c3(220, 20);
    Vector<Pointf> cubic;
    UiGeometry::AppendCubic(cubic, c0, c1, c2, c3, true);
    double cerr = MaxCubicError(c0, c1, c2, c3, cubic);
    t.Expect(Near(cubic[0], c0) && Near(cubic.Top(), c3),
             "cubic flattening preserves endpoints");
    t.Expect(cerr <= UiGeometry::ErrorPx() + 0.04,
             "cubic flattening remains within the screen-space contract");

    Vector<Pointf> small_cubic;
    UiGeometry::AppendCubic(small_cubic,
                            Pointf(0, 0), Pointf(4, 16),
                            Pointf(16, -8), Pointf(22, 2), true);
    t.Expect(small_cubic.GetCount() < cubic.GetCount(),
             "the same cubic shape projected smaller generates less geometry");

    Vector<Pointf> elbow;
    elbow << Pointf(0, 0) << Pointf(50, 0) << Pointf(50, 50);
    Vector<Pointf> rounded = UiGeometry::RoundedPolyline(elbow, 10.0);
    t.Expect(Near(rounded[0], elbow[0]) && Near(rounded.Top(), elbow.Top())
             && rounded.GetCount() > elbow.GetCount(),
             "rounded polyline preserves endpoints and adaptively resolves the corner");

    Vector<Pointf> box;
    box << Pointf(0, 0) << Pointf(100, 0) << Pointf(100, 40) << Pointf(0, 40);
    Vector<Pointf> flat_box = UiGeometry::RoundedPolygon(box, 0.0);
    Vector<Pointf> rounded_box = UiGeometry::RoundedPolygon(box, 8.0);
    t.Expect(flat_box.GetCount() == box.GetCount()
             && rounded_box.GetCount() > box.GetCount(),
             "zero-radius authored polygon stays discrete while rounded polygon adds only required curve geometry");

    Vector<Pointf> sampled;
    for(int i = 0; i <= 100; i++)
        sampled.Add(Pointf(i, (i & 1) ? 0.08 : -0.08));
    Vector<Pointf> simplified = UiGeometry::SimplifyPolyline(sampled);
    t.Expect(simplified.GetCount() == 2
             && Near(simplified[0], sampled[0])
             && Near(simplified.Top(), sampled.Top()),
             "generated sub-pixel polyline noise collapses to its visible endpoints");

    double d = UiGeometry::DistanceToSegment(Pointf(5, 3), Pointf(0, 0), Pointf(10, 0));
    t.Expect(std::fabs(d - 3.0) < 1e-9,
             "shared line-segment distance remains exact for analytic geometry consumers");

    t.Expect(std::fabs(UiGeometry::Length(Pointf(3, 4)) - 5.0) < 1e-9
             && Near(UiGeometry::Normalize(Pointf(3, 4)), Pointf(0.6, 0.8)),
             "line/vector helpers provide one reusable screen-space basis");

    Vector<Pointf> line;
    line << Pointf(0, 0) << Pointf(3, 4) << Pointf(6, 4);
    t.Expect(std::fabs(UiGeometry::PolylineLength(line) - 8.0) < 1e-9
             && std::fabs(UiGeometry::DistanceToPolyline(Pointf(3, 2), line) - 1.2) < 1e-9,
             "polyline length and distance remain exact for routing and hit-testing consumers");

    Vector<Pointf> band = UiGeometry::ArcBandPath(Pointf(50, 50), 20.0, 12.0,
                                                  -PI * 0.5, PI * 1.5);
    Vector<Pointf> pie = UiGeometry::PiePath(Pointf(50, 50), 20.0,
                                             -PI * 0.5, PI * 0.5);
    t.Expect(band.GetCount() > 4 && pie.GetCount() > 3,
             "radial band and pie silhouettes compose the shared adaptive arc contract");
    t.Expect(UiGeometry::ArcBandPath(Pointf(0, 0), 0.49, 0.0, 0.0, PI).IsEmpty(),
             "sub-pixel radial areas do not generate meaningless explicit geometry");
    Vector<Pointf> full_disk = UiGeometry::PiePath(Pointf(50, 50), 20.0, 0.0, 2.0 * PI);
    bool full_disk_has_center = false;
    for(const Pointf& p : full_disk)
        if(Near(p, Pointf(50, 50))) {
            full_disk_has_center = true;
            break;
        }
    t.Expect(!full_disk_has_center && full_disk.GetCount() == UiGeometry::ArcSegments(20.0, 2.0 * PI),
             "full disk is one adaptive outer silhouette with no artificial radial seam");

    Rect analytic = RectC(10, 10, 80, 40);
    Vector<Pointf> analytic_box;
    analytic_box << Pointf(10, 10) << Pointf(90, 10)
                 << Pointf(90, 50) << Pointf(10, 50);
    t.Expect(UiGeometry::ContainsEllipse(analytic, Pointf(50, 30))
             && !UiGeometry::ContainsEllipse(analytic, Pointf(10, 10)),
             "ellipse containment is analytic and does not require a sampled path");
    t.Expect(UiGeometry::ContainsRoundedRect(analytic, 10.0, Pointf(20, 20))
             && !UiGeometry::ContainsRoundedRect(analytic, 10.0, Pointf(10, 10)),
             "rounded-rectangle containment is analytic");
    t.Expect(UiGeometry::ContainsPolygon(analytic_box, Pointf(50, 30))
             && !UiGeometry::ContainsPolygon(analytic_box, Pointf(100, 30)),
             "general polygon containment is shared without manufacturing extra geometry");

    PrintArc(4.0, 2.0 * PI);
    PrintArc(10.0, 2.0 * PI);
    PrintArc(100.0, 2.0 * PI);
    Cout() << "UI_GEOMETRY_PROFILE primitive=quadratic"
           << " vertices=" << q.GetCount()
           << " measured_error_px=" << qerr << '\n';
    Cout() << "UI_GEOMETRY_PROFILE primitive=cubic"
           << " vertices=" << cubic.GetCount()
           << " measured_error_px=" << cerr << '\n';
    Cout() << "UI_GEOMETRY_PROFILE primitive=radial_band"
           << " vertices=" << band.GetCount() << '\n';

    Cout() << "UI_GEOMETRY_CONTRACT_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
