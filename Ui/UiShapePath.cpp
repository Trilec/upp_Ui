#include <Ui/UiShapePath.h>

#include <cmath>

namespace Upp {
namespace {

double ShapeDistance(Pointf a, Pointf b)
{
    return UiGeometry::Length(a - b);
}

Pointf ArcPoint(Pointf center, double rx, double ry, double angle)
{
    return Pointf(center.x + std::cos(angle) * rx,
                  center.y + std::sin(angle) * ry);
}

} // namespace

UiShapePath& UiShapePath::Clear()
{
    commands_.Clear();
    return *this;
}

UiShapePath& UiShapePath::MoveTo(Pointf point)
{
    UiShapeCommand& command = commands_.Add();
    command.type = UiShapeCommandType::MoveTo;
    command.p1 = point;
    return *this;
}

UiShapePath& UiShapePath::LineTo(Pointf point)
{
    UiShapeCommand& command = commands_.Add();
    command.type = UiShapeCommandType::LineTo;
    command.p1 = point;
    return *this;
}

UiShapePath& UiShapePath::QuadraticTo(Pointf control, Pointf end)
{
    UiShapeCommand& command = commands_.Add();
    command.type = UiShapeCommandType::QuadraticTo;
    command.p1 = control;
    command.p2 = end;
    return *this;
}

UiShapePath& UiShapePath::CubicTo(Pointf control1, Pointf control2, Pointf end)
{
    UiShapeCommand& command = commands_.Add();
    command.type = UiShapeCommandType::CubicTo;
    command.p1 = control1;
    command.p2 = control2;
    command.p3 = end;
    return *this;
}

UiShapePath& UiShapePath::Arc(Pointf center, double radius_px,
                              double start_angle, double sweep_angle)
{
    UiShapeCommand& command = commands_.Add();
    command.type = UiShapeCommandType::Arc;
    command.p1 = center;
    command.radius_x = std::fabs(radius_px);
    command.radius_y = command.radius_x;
    command.start_angle = start_angle;
    command.sweep_angle = sweep_angle;
    return *this;
}

UiShapePath& UiShapePath::EllipseArc(Pointf center,
                                     double radius_x_px, double radius_y_px,
                                     double start_angle, double sweep_angle)
{
    UiShapeCommand& command = commands_.Add();
    command.type = UiShapeCommandType::EllipseArc;
    command.p1 = center;
    command.radius_x = std::fabs(radius_x_px);
    command.radius_y = std::fabs(radius_y_px);
    command.start_angle = start_angle;
    command.sweep_angle = sweep_angle;
    return *this;
}

UiShapePath& UiShapePath::Close()
{
    UiShapeCommand& command = commands_.Add();
    command.type = UiShapeCommandType::Close;
    return *this;
}

Vector<UiShapeContour> UiShapePath::Flatten() const
{
    Vector<UiShapeContour> result;
    int contour_index = -1;
    bool have_current = false;
    Pointf current;
    Pointf start;

    auto begin_contour = [&](Pointf point) {
        UiShapeContour& contour = result.Add();
        contour.points.Add(point);
        contour.closed = false;
        contour_index = result.GetCount() - 1;
        current = start = point;
        have_current = true;
    };

    auto ensure = [&](Pointf point) {
        if(!have_current)
            begin_contour(point);
    };

    auto points = [&]() -> Vector<Pointf>& {
        ASSERT(contour_index >= 0 && contour_index < result.GetCount());
        return result[contour_index].points;
    };

    auto line_to = [&](Pointf point) {
        ensure(point);
        if(ShapeDistance(current, point) > 1e-9)
            points().Add(point);
        current = point;
    };

    for(const UiShapeCommand& command : commands_) {
        switch(command.type) {
        case UiShapeCommandType::MoveTo:
            begin_contour(command.p1);
            break;

        case UiShapeCommandType::LineTo:
            line_to(command.p1);
            break;

        case UiShapeCommandType::QuadraticTo:
            if(!have_current)
                begin_contour(command.p2);
            else {
                UiGeometry::AppendQuadratic(points(),
                                            current, command.p1, command.p2);
                current = command.p2;
            }
            break;

        case UiShapeCommandType::CubicTo:
            if(!have_current)
                begin_contour(command.p3);
            else {
                UiGeometry::AppendCubic(points(),
                                        current, command.p1, command.p2, command.p3);
                current = command.p3;
            }
            break;

        case UiShapeCommandType::Arc: {
            Pointf arc_start = ArcPoint(command.p1, command.radius_x,
                                        command.radius_x, command.start_angle);
            if(!have_current)
                begin_contour(arc_start);
            else if(ShapeDistance(current, arc_start) > 1e-9)
                line_to(arc_start);
            UiGeometry::AppendArc(points(), command.p1, command.radius_x,
                                  command.start_angle, command.sweep_angle);
            current = ArcPoint(command.p1, command.radius_x, command.radius_x,
                               command.start_angle + command.sweep_angle);
            break;
        }

        case UiShapeCommandType::EllipseArc: {
            Pointf arc_start = ArcPoint(command.p1, command.radius_x,
                                        command.radius_y, command.start_angle);
            if(!have_current)
                begin_contour(arc_start);
            else if(ShapeDistance(current, arc_start) > 1e-9)
                line_to(arc_start);
            UiGeometry::AppendEllipse(points(), command.p1,
                                      command.radius_x, command.radius_y,
                                      command.start_angle, command.sweep_angle);
            current = ArcPoint(command.p1, command.radius_x, command.radius_y,
                               command.start_angle + command.sweep_angle);
            break;
        }

        case UiShapeCommandType::Close:
            if(have_current) {
                Vector<Pointf>& p = points();
                if(p.GetCount() > 1 && ShapeDistance(p.Top(), p[0]) <= 1e-9)
                    p.Drop();
                result[contour_index].closed = true;
                current = start;
                have_current = false;
                contour_index = -1;
            }
            break;
        }
    }

    return result;
}

Vector<Pointf> UiShapePath::FlattenSingleContour() const
{
    Vector<UiShapeContour> contours = Flatten();
    if(contours.IsEmpty())
        return Vector<Pointf>();
    return pick(contours[0].points);
}

} // namespace Upp
