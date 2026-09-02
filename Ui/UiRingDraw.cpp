#include <Ui/UiRingDraw.h>
#include <cmath>

namespace Upp {
namespace {

double Clamp01(double v)
{
    if(v < 0.0) return 0.0;
    if(v > 1.0) return 1.0;
    return v;
}

Pointf BasisPoint(const Pointf& origin,
                  const Pointf& xaxis, double x,
                  const Pointf& yaxis, double y)
{
    return Pointf(origin.x + xaxis.x * x + yaxis.x * y,
                  origin.y + xaxis.y * x + yaxis.y * y);
}

void RoundedCapPath(Painter& p, const Pointf& endpoint, double angle,
                    double sweep_direction, bool start_cap,
                    double half_width, double roundness)
{
    const double r = half_width * Clamp01(roundness);
    if(r <= 0.000001 || half_width <= 0.000001)
        return;

    Pointf forward(-sin(angle) * sweep_direction,
                   cos(angle) * sweep_direction);
    Pointf outward = start_cap ? Pointf(-forward.x, -forward.y) : forward;
    Pointf normal(cos(angle), sin(angle));

    // Rounded corners around a real cap face. At 0% the face is fully flat;
    // at 100% the central face disappears and the two quarter circles meet as
    // the exact semicircular cap implied by the current stroke thickness.
    const double kappa = 0.5522847498307936;
    const double face_half = half_width - r;

    Pointf upper = BasisPoint(endpoint, outward, 0.0, normal, half_width);
    Pointf upper_face = BasisPoint(endpoint, outward, r, normal, face_half);
    Pointf lower_face = BasisPoint(endpoint, outward, r, normal, -face_half);
    Pointf lower = BasisPoint(endpoint, outward, 0.0, normal, -half_width);

    Pointf upper_c1 = BasisPoint(endpoint, outward, kappa * r, normal, half_width);
    Pointf upper_c2 = BasisPoint(endpoint, outward, r, normal, face_half + kappa * r);
    Pointf lower_c1 = BasisPoint(endpoint, outward, r, normal, -face_half - kappa * r);
    Pointf lower_c2 = BasisPoint(endpoint, outward, kappa * r, normal, -half_width);

    p.Move(upper)
     .Cubic(upper_c1, upper_c2, upper_face)
     .Line(lower_face)
     .Cubic(lower_c1, lower_c2, lower)
     .Close();
}

Image AngularGradientBrush(Size size, Pointf center,
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
                            (int)std::round(Clamp01(q) * 255.0));
            row[x] = c;
            row[x].a = 255;
        }
    }
    return ib;
}

} // namespace

Pointf UiRingArcPoint(Pointf center, double radius, double angle)
{
    return Pointf(center.x + cos(angle) * radius,
                  center.y + sin(angle) * radius);
}

void UiPaintRingArc(Painter& p, Size raster_size,
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
        gradient_brush = AngularGradientBrush(raster_size, center,
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
    Pointf first = UiRingArcPoint(center, radius, start_angle);

    p.Begin();
    p.Move(first).Arc(center, radius, start_angle, sweep_angle);
    p.LineCap(native_round_cap ? LINECAP_ROUND : LINECAP_BUTT);
    StrokePath();
    p.End();

    if(roundness > 0 && !native_round_cap) {
        const double direction = sweep_angle < 0.0 ? -1.0 : 1.0;
        const double half_width = thickness / 2.0;
        const double q = roundness / 100.0;
        Pointf last = UiRingArcPoint(center, radius, start_angle + sweep_angle);

        auto FillCap = [&](const Pointf& endpoint, double angle, bool at_start) {
            p.Begin();
            RoundedCapPath(p, endpoint, angle, direction, at_start,
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

Image UiRenderProgressRingRaster(Size raster_size,
                                 double radius,
                                 double start_angle,
                                 double sweep_angle,
                                 int thickness,
                                 int cap_roundness,
                                 Color track,
                                 Color progress_start,
                                 Color progress_end,
                                 bool gradient)
{
    if(raster_size.cx <= 0 || raster_size.cy <= 0)
        return Image();

    ImageBuffer ib(raster_size);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    Pointf center(raster_size.cx / 2.0, raster_size.cy / 2.0);
    BufferPainter p(ib, MODE_ANTIALIASED);
    if(radius > 0.0 && thickness > 0 && !IsNull(track))
        p.Circle(center, radius).Stroke((double)thickness, track);
    UiPaintRingArc(p, raster_size, center, radius,
                   start_angle, sweep_angle, thickness, cap_roundness,
                   progress_start, progress_end, gradient);
    p.Finish();
    return Image(ib);
}

} // namespace Upp
