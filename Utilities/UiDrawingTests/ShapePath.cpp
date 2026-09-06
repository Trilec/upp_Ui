#include <Ui/Ui.h>

#include <cmath>

using namespace Upp;

namespace {

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
    return UiGeometry::Length(a - b) <= eps;
}

Rectf Bounds(const Vector<UiShapeContour>& contours)
{
    bool have = false;
    double l = 0, t = 0, r = 0, b = 0;
    for(const UiShapeContour& contour : contours)
        for(const Pointf& p : contour.points) {
            if(!have) {
                l = r = p.x;
                t = b = p.y;
                have = true;
            }
            else {
                l = min(l, p.x);
                t = min(t, p.y);
                r = max(r, p.x);
                b = max(b, p.y);
            }
        }
    return have ? Rectf(l, t, r, b) : Rectf(0, 0, 0, 0);
}

bool HasPointOutside(const Vector<UiShapeContour>& contours, const Rectf& rect)
{
    for(const UiShapeContour& contour : contours)
        for(const Pointf& p : contour.points)
            if(p.x < rect.left - 1e-6 || p.x > rect.right + 1e-6 ||
               p.y < rect.top - 1e-6 || p.y > rect.bottom + 1e-6)
                return true;
    return false;
}

} // namespace

int RunShapePathSuite()
{
    TestCtx t;

    UiShapePath authored;
    authored.MoveTo(Pointf(0, 0))
            .LineTo(Pointf(20, 0))
            .QuadraticTo(Pointf(30, 20), Pointf(40, 0))
            .CubicTo(Pointf(50, -20), Pointf(70, 20), Pointf(80, 0));
    Vector<UiShapeContour> authored_flat = authored.Flatten();
    t.Expect(authored.GetCommandCount() == 4,
             "general path retains authored Move/Line/Quadratic/Cubic commands");
    t.Expect(authored_flat.GetCount() == 1 && !authored_flat[0].closed,
             "open authored path flattens to one open contour");
    t.Expect(authored_flat[0].points.GetCount() > 4
             && Near(authored_flat[0].points[0], Pointf(0, 0))
             && Near(authored_flat[0].points.Top(), Pointf(80, 0)),
             "curve commands flatten adaptively while preserving endpoints");

    UiShapePath arc_path;
    arc_path.MoveTo(Pointf(20, 10))
            .Arc(Pointf(10, 10), 10.0, 0.0, 3.14159265358979323846)
            .Close();
    Vector<UiShapeContour> arc_flat = arc_path.Flatten();
    t.Expect(arc_flat.GetCount() == 1 && arc_flat[0].closed,
             "circular arc participates in a closed authored contour");
    t.Expect(arc_flat[0].points.GetCount() == UiGeometry::ArcSegments(10.0, 3.14159265358979323846) + 1,
             "shape-path circular arc consumes the shared 0.35px tessellation contract");

    Rectf box(10, 20, 110, 80);
    Vector<UiShapeContour> rectangle = UiShapes::Rectangle(box).Flatten();
    Vector<UiShapeContour> rounded = UiShapes::RoundedRectangle(box, 12.0).Flatten();
    Vector<UiShapeContour> capsule = UiShapes::Capsule(box).Flatten();
    Vector<UiShapeContour> ellipse = UiShapes::Ellipse(box).Flatten();
    t.Expect(rectangle.GetCount() == 1 && rectangle[0].closed
             && rectangle[0].points.GetCount() == 4,
             "stock rectangle preserves four authored corners");
    t.Expect(rounded.GetCount() == 1 && rounded[0].closed
             && rounded[0].points.GetCount() > rectangle[0].points.GetCount(),
             "stock rounded rectangle resolves corners only through adaptive arcs");
    t.Expect(capsule.GetCount() == 1 && capsule[0].closed
             && capsule[0].points.GetCount() >= rounded[0].points.GetCount(),
             "stock capsule is a reusable radius-derived rounded silhouette");
    t.Expect(ellipse.GetCount() == 1 && ellipse[0].closed
             && ellipse[0].points.GetCount() == UiGeometry::EllipseSegments(50.0, 30.0, 2.0 * 3.14159265358979323846),
             "stock ellipse delegates explicit detail to UiGeometry");

    Vector<UiShapeContour> hex = UiShapes::RegularPolygon(box, 6).Flatten();
    Vector<UiShapeContour> star = UiShapes::Star(box, 5).Flatten();
    t.Expect(hex.GetCount() == 1 && hex[0].points.GetCount() == 6,
             "regular polygon covers arbitrary authored N-gons without new shape APIs");
    t.Expect(star.GetCount() == 1 && star[0].points.GetCount() == 10,
             "generic star covers N-point star/burst topology");

    Vector<UiShapeContour> arrow = UiShapes::Arrow(box, UiShapeSide::Right).Flatten();
    Vector<UiShapeContour> arrow_up = UiShapes::Arrow(box, UiShapeSide::Top).Flatten();
    Vector<UiShapeContour> chevron = UiShapes::Chevron(box, UiShapeSide::Left).Flatten();
    t.Expect(arrow.GetCount() == 1 && arrow[0].points.GetCount() == 7,
             "directional arrow is one reusable parameterised polygon");
    t.Expect(arrow_up.GetCount() == 1 && arrow_up[0].closed,
             "directional mapping rotates stock shapes without new geometry code");
    t.Expect(chevron.GetCount() == 1 && chevron[0].points.GetCount() == 6,
             "chevron is available as a reusable directional silhouette");

    Vector<UiShapeContour> chamfer = UiShapes::ChamferedRectangle(box, 8.0).Flatten();
    t.Expect(chamfer.GetCount() == 1 && chamfer[0].points.GetCount() == 8,
             "chamfered rectangle uses authored discrete corners without tessellation");

    UiShapePath callout_path = UiShapes::Callout(box, UiShapeSide::Bottom,
                                                 0.65, 18.0, 12.0, 8.0);
    Vector<UiShapeContour> callout = callout_path.Flatten();
    t.Expect(callout.GetCount() == 1 && callout[0].closed
             && HasPointOutside(callout, box),
             "callout adds a parameterised tail without control-specific path code");

    UiShapePath tag_path = UiShapes::Tag(box, UiShapeSide::Right, 16.0, 4.0);
    Vector<UiShapeContour> tag = tag_path.Flatten();
    t.Expect(tag.GetCount() == 2 && tag[0].closed && tag[1].closed,
             "tag supports a second reversed contour for a reusable hole");

    Vector<UiShapeContour> cloud = UiShapes::Cloud(box).Flatten();
    Vector<UiShapeContour> document = UiShapes::Document(box).Flatten();
    Vector<UiShapeContour> database = UiShapes::Database(box).Flatten();
    t.Expect(cloud.GetCount() == 1 && cloud[0].closed && cloud[0].points.GetCount() > 11,
             "cloud silhouette is shared above the geometry layer");
    t.Expect(document.GetCount() == 1 && document[0].closed
             && document[0].points.GetCount() >= 5,
             "document/fold silhouette is reusable outside Graph");
    t.Expect(database.GetCount() == 1 && database[0].closed
             && database[0].points.GetCount() > 8,
             "database/cylinder silhouette is reusable outside Graph");

    UiShapePath quarter_ring = UiShapes::RingSegment(Pointf(50, 50), 30.0, 18.0,
                                                     0.0, 3.14159265358979323846 * 0.5);
    UiShapePath full_ring = UiShapes::RingSegment(Pointf(50, 50), 30.0, 18.0,
                                                  0.0, 2.0 * 3.14159265358979323846);
    Vector<UiShapeContour> quarter_ring_flat = quarter_ring.Flatten();
    Vector<UiShapeContour> full_ring_flat = full_ring.Flatten();
    t.Expect(quarter_ring_flat.GetCount() == 1 && quarter_ring_flat[0].closed,
             "partial ring is one closed radial contour");
    t.Expect(full_ring_flat.GetCount() == 2
             && full_ring_flat[0].closed && full_ring_flat[1].closed,
             "full annulus uses separate opposite-winding outer/inner contours");

    Vector<UiShapeContour> pie = UiShapes::Pie(Pointf(50, 50), 24.0,
                                               0.0, 3.14159265358979323846).Flatten();
    t.Expect(pie.GetCount() == 1 && pie[0].closed,
             "pie/sector is available without a control-specific radial builder");

    Vector<Pointf> rounded_poly_input;
    rounded_poly_input << Pointf(0, 0) << Pointf(100, 0)
                       << Pointf(100, 50) << Pointf(0, 50);
    Vector<UiShapeContour> rounded_poly = UiShapes::RoundedPolygon(rounded_poly_input, 10.0).Flatten();
    t.Expect(rounded_poly.GetCount() == 1 && rounded_poly[0].points.GetCount() > 4,
             "generic rounded polygon authors quadratic corners then UiGeometry flattens them");

    Rectf callout_bounds = Bounds(callout);
    t.Expect(callout_bounds.bottom > box.bottom,
             "stock-shape bounds reflect authored tail geometry rather than clipping to body");

    ImageBuffer buffer(140, 100);
    buffer.SetKind(IMAGE_ALPHA);
    Fill(~buffer, RGBAZero(), buffer.GetLength());
    BufferPainter painter(buffer, MODE_ANTIALIASED);
    painter.Begin();
    UiPainterShapePath(painter, UiShapes::Star(Rectf(10, 10, 90, 90), 5));
    painter.Fill(Black());
    painter.End();
    painter.Finish();
    bool painted = false;
    RGBA *pixels = ~buffer;
    for(int i = 0; i < buffer.GetLength(); i++)
        if(pixels[i].a != 0) {
            painted = true;
            break;
        }
    t.Expect(painted,
             "shared Painter adapter renders authored stock shapes without bespoke control code");

    ImageBuffer tag_buffer(130, 90);
    tag_buffer.SetKind(IMAGE_ALPHA);
    Fill(~tag_buffer, RGBAZero(), tag_buffer.GetLength());
    BufferPainter tag_painter(tag_buffer, MODE_ANTIALIASED);
    tag_painter.Begin();
    UiPainterShapePath(tag_painter,
        UiShapes::Tag(Rectf(10, 10, 110, 70), UiShapeSide::Left, 16.0, 5.0));
    tag_painter.Fill(Black());
    tag_painter.End();
    tag_painter.Finish();
    RGBA *tag_pixels = ~tag_buffer;
    RGBA hole_pixel = tag_pixels[40 * tag_buffer.GetWidth() + 92];
    t.Expect(hole_pixel.a < 32,
             "opposite-winding stock tag contour punches a real Painter hole after direction rotation");

    Cout() << "UI_SHAPE_PATH_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    return t.fails ? 1 : 0;
}
