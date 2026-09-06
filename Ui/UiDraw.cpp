#include <Ui/UiDraw.h>
#include <cmath>

namespace Upp {
namespace {

Pointf UiShapePathArcPoint(Pointf center, double rx, double ry, double angle)
{
    return Pointf(center.x + cos(angle) * rx,
                  center.y + sin(angle) * ry);
}

double UiCircularArcClamp01(double v)
{
    if(v < 0.0) return 0.0;
    if(v > 1.0) return 1.0;
    return v;
}

Pointf UiCircularArcPoint(Pointf center, double radius, double angle)
{
    return Pointf(center.x + cos(angle) * radius,
                  center.y + sin(angle) * radius);
}

Pointf UiCircularArcBasisPoint(const Pointf& origin,
                               const Pointf& xaxis, double x,
                               const Pointf& yaxis, double y)
{
    return Pointf(origin.x + xaxis.x * x + yaxis.x * y,
                  origin.y + xaxis.y * x + yaxis.y * y);
}

void UiCircularArcRoundedCapPath(Painter& p, const Pointf& endpoint, double angle,
                                 double sweep_direction, bool start_cap,
                                 double half_width, double roundness)
{
    const double r = half_width * UiCircularArcClamp01(roundness);
    if(r <= 0.000001 || half_width <= 0.000001)
        return;

    Pointf forward(-sin(angle) * sweep_direction,
                   cos(angle) * sweep_direction);
    Pointf outward = start_cap ? Pointf(-forward.x, -forward.y) : forward;
    Pointf normal(cos(angle), sin(angle));

    // Rounded corners grow around a real cap face. At 0% the face is flat;
    // at 100% the face disappears and the quarter circles meet as the exact
    // semicircular cap implied by the current stroke thickness.
    const double kappa = 0.5522847498307936;
    const double face_half = half_width - r;

    Pointf upper = UiCircularArcBasisPoint(endpoint, outward, 0.0, normal, half_width);
    Pointf upper_face = UiCircularArcBasisPoint(endpoint, outward, r, normal, face_half);
    Pointf lower_face = UiCircularArcBasisPoint(endpoint, outward, r, normal, -face_half);
    Pointf lower = UiCircularArcBasisPoint(endpoint, outward, 0.0, normal, -half_width);

    Pointf upper_c1 = UiCircularArcBasisPoint(endpoint, outward, kappa * r, normal, half_width);
    Pointf upper_c2 = UiCircularArcBasisPoint(endpoint, outward, r, normal, face_half + kappa * r);
    Pointf lower_c1 = UiCircularArcBasisPoint(endpoint, outward, r, normal, -face_half - kappa * r);
    Pointf lower_c2 = UiCircularArcBasisPoint(endpoint, outward, kappa * r, normal, -half_width);

    p.Move(upper)
     .Cubic(upper_c1, upper_c2, upper_face)
     .Line(lower_face)
     .Cubic(lower_c1, lower_c2, lower)
     .Close();
}

Image UiCircularArcAngularGradient(Size size, Pointf center,
                                   double start_angle, double sweep_angle,
                                   Color start, Color end)
{
    ImageBuffer ib(max(size.cx, 1), max(size.cy, 1));
    const double tau = 2.0 * M_PI;
    const double direction = sweep_angle < 0.0 ? -1.0 : 1.0;
    const double sweep = min(std::fabs(sweep_angle), tau);

    for(int y = 0; y < ib.GetHeight(); y++) {
        RGBA *row = ib[y];
        for(int x = 0; x < ib.GetWidth(); x++) {
            double angle = std::atan2((y + 0.5) - center.y,
                                      (x + 0.5) - center.x);
            double phase = std::fmod(direction * (angle - start_angle), tau);
            if(phase < 0.0)
                phase += tau;

            double q = 0.0;
            if(sweep >= tau - 0.000001)
                q = phase / tau;
            else if(sweep > 0.000001) {
                if(phase <= sweep)
                    q = phase / sweep;
                else {
                    double distance_to_start = min(phase, tau - phase);
                    double end_delta = std::fabs(phase - sweep);
                    double distance_to_end = min(end_delta, tau - end_delta);
                    q = distance_to_start <= distance_to_end ? 0.0 : 1.0;
                }
            }

            Color c = Blend(start, end,
                            (int)std::round(UiCircularArcClamp01(q) * 255.0));
            row[x] = c;
            row[x].a = 255;
        }
    }
    return ib;
}

} // namespace

void UiPainterShapePath(Painter& painter, const UiShapePath& path)
{
    bool have_current = false;
    Pointf current;

    auto move = [&](Pointf point) {
        painter.Move(point);
        current = point;
        have_current = true;
    };

    auto line = [&](Pointf point) {
        if(!have_current)
            move(point);
        else {
            painter.Line(point);
            current = point;
        }
    };

    for(const UiShapeCommand& command : path.GetCommands()) {
        switch(command.type) {
        case UiShapeCommandType::MoveTo:
            move(command.p1);
            break;

        case UiShapeCommandType::LineTo:
            line(command.p1);
            break;

        case UiShapeCommandType::QuadraticTo:
            if(!have_current)
                move(command.p2);
            else {
                Pointf c1 = current + (command.p1 - current) * (2.0 / 3.0);
                Pointf c2 = command.p2 + (command.p1 - command.p2) * (2.0 / 3.0);
                painter.Cubic(c1, c2, command.p2);
                current = command.p2;
            }
            break;

        case UiShapeCommandType::CubicTo:
            if(!have_current)
                move(command.p3);
            else {
                painter.Cubic(command.p1, command.p2, command.p3);
                current = command.p3;
            }
            break;

        case UiShapeCommandType::Arc: {
            Pointf arc_start = UiShapePathArcPoint(command.p1, command.radius_x,
                                                   command.radius_x,
                                                   command.start_angle);
            if(!have_current)
                move(arc_start);
            else if(UiGeometry::Length(current - arc_start) > 1e-9)
                line(arc_start);
            painter.Arc(command.p1, command.radius_x,
                        command.start_angle, command.sweep_angle);
            current = UiShapePathArcPoint(command.p1, command.radius_x,
                                          command.radius_x,
                                          command.start_angle + command.sweep_angle);
            break;
        }

        case UiShapeCommandType::EllipseArc: {
            // The Painter API used here has verified native circular Arc/Cubic
            // paths but no authored elliptical-arc command. Flatten exactly once
            // through UiGeometry rather than inventing a control-local quality.
            Pointf arc_start = UiShapePathArcPoint(command.p1, command.radius_x,
                                                   command.radius_y,
                                                   command.start_angle);
            if(!have_current)
                move(arc_start);
            else if(UiGeometry::Length(current - arc_start) > 1e-9)
                line(arc_start);

            Vector<Pointf> points;
            points.Add(arc_start);
            UiGeometry::AppendEllipse(points, command.p1,
                                      command.radius_x, command.radius_y,
                                      command.start_angle, command.sweep_angle);
            for(int i = 1; i < points.GetCount(); i++)
                line(points[i]);
            break;
        }

        case UiShapeCommandType::Close:
            if(have_current)
                painter.Close();
            have_current = false;
            break;
        }
    }
}


void UiPaintCircularArc(Painter& p, Size raster_size,
                        const Pointf& center, double radius,
                        double start_angle, double sweep_angle,
                        int thickness, int cap_roundness,
                        Color start, Color end, bool gradient)
{
    if(radius <= 0.0 || thickness <= 0 || std::fabs(sweep_angle) < 0.000001)
        return;

    const double tau = 2.0 * M_PI;
    const bool use_gradient = gradient && start != end;
    Image gradient_brush;
    if(use_gradient)
        gradient_brush = UiCircularArcAngularGradient(raster_size, center,
                                                      start_angle, sweep_angle,
                                                      start, end);

    auto StrokePath = [&] {
        if(use_gradient)
            p.Stroke((double)thickness, gradient_brush, Xform2D::Identity());
        else
            p.Stroke((double)thickness, start);
    };

    if(std::fabs(sweep_angle) >= tau - 0.000001) {
        p.Begin();
        p.Circle(center, radius);
        StrokePath();
        p.End();
        return;
    }

    const int roundness = clamp(cap_roundness, 0, 100);
    const bool native_round_cap = roundness >= 100;
    const double direction = sweep_angle < 0.0 ? -1.0 : 1.0;
    Pointf first = UiCircularArcPoint(center, radius, start_angle);
    Pointf last = UiCircularArcPoint(center, radius, start_angle + sweep_angle);

    // Intermediate custom caps are filled separately from the stroked arc.
    // Give the centerline stroke a bounded sub-pixel overlap under each cap so
    // Painter antialiasing cannot leave a hairline where the two shapes meet.
    double paint_start = start_angle;
    double paint_sweep = sweep_angle;
    if(roundness > 0 && !native_round_cap) {
        const double arc_length = radius * std::fabs(sweep_angle);
        const double overlap_px = min(0.5, arc_length * 0.20);
        const double overlap_angle = radius > 0.000001 ? overlap_px / radius : 0.0;
        paint_start -= direction * overlap_angle;
        paint_sweep += direction * overlap_angle * 2.0;
    }

    p.Begin();
    p.Move(UiCircularArcPoint(center, radius, paint_start))
     .Arc(center, radius, paint_start, paint_sweep);
    p.LineCap(native_round_cap ? LINECAP_ROUND : LINECAP_BUTT);
    StrokePath();
    p.End();

    if(roundness > 0 && !native_round_cap) {
        const double half_width = thickness / 2.0;
        const double q = roundness / 100.0;

        auto FillCap = [&](const Pointf& endpoint, double angle, bool at_start) {
            p.Begin();
            UiCircularArcRoundedCapPath(p, endpoint, angle, direction, at_start,
                                        half_width, q);
            if(use_gradient)
                p.Fill(gradient_brush, Xform2D::Identity());
            else
                p.Fill(start);
            p.End();
        };

        FillCap(first, start_angle, true);
        FillCap(last, start_angle + sweep_angle, false);
    }
}

} // namespace Upp
