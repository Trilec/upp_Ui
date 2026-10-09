#include <Ui/UiDraw.h>
#include <cmath>

namespace Upp {

// Accent ownership uses 45-degree corner bisectors. Combining adjacent sides
// produces one clipping region, so translucent accents have no doubled seams.
void UiPaintFrameAccent(Draw& w, const Rect& surface,
                        const StyledPalette& palette, const StyledMetrics& metrics,
                        StyledState state)
{
    const StyledFrameAccent& accent = metrics.frame_accent;
    if(surface.IsEmpty() || !accent.IsVisible())
        return;
    Color color = IsNull(accent.color) ? palette.frame[state] : accent.color;
    if(IsNull(color))
        return;
    const int edges = accent.edges & StyledFrameAccent::All;
    const int alpha = clamp(accent.alpha, 0, 255);
    const Size size = surface.GetSize();
    const int fw = max(0, metrics.frame_width);
    const bool frame = metrics.frame_enabled && fw > 0 && !IsNull(palette.frame[state]);
    const int authored_radius = max(0, metrics.radius);
    const double inset = authored_radius > 0 ? (fw > 0 ? max(0.5, fw * 0.5) : 0.5) : 0.0;
    const double border_inset = authored_radius > 0 ? inset + (frame ? fw * 0.5 : 0.0)
                                                    : (frame ? fw : 0.0);
    const Rectf outer(border_inset, border_inset,
                      size.cx - border_inset, size.cy - border_inset);
    if(outer.IsEmpty())
        return;
    // Painter clamps the shared frame's centreline radius to its dimensions.
    const double centre_radius = min<double>(authored_radius,
        max(0.0, min(size.cx - 2 * inset, size.cy - 2 * inset) * 0.5));
    const double radius = max(0.0, centre_radius - (frame ? fw * 0.5 : 0.0));
    const double thickness = min<double>(accent.thickness,
                                         min(outer.GetWidth(), outer.GetHeight()) * 0.5);
    if(thickness <= 0)
        return;
    if(authored_radius == 0 && alpha == 255) {
        const int x = surface.left + (int)outer.left, y = surface.top + (int)outer.top;
        const int width = (int)outer.GetWidth(), height = (int)outer.GetHeight();
        const int t = max(1, (int)thickness);
        if(edges & StyledFrameAccent::Top) w.DrawRect(x, y, width, t, color);
        if(edges & StyledFrameAccent::Bottom) w.DrawRect(x, y + height - t, width, t, color);
        if(edges & StyledFrameAccent::Left) w.DrawRect(x, y, t, height, color);
        if(edges & StyledFrameAccent::Right) w.DrawRect(x + width - t, y, t, height, color);
        return;
    }

    // Rasterize only disjoint boundary bands, in bounded exact tiles. A large
    // panel never allocates a full-panel accent buffer or an unbounded fallback.
    Rect bounds((int)floor(outer.left), (int)floor(outer.top),
                (int)ceil(outer.right), (int)ceil(outer.bottom));
    const int depth = (int)ceil(radius + thickness) + 1;
    const int top = min(depth, bounds.GetHeight());
    const int bottom = min(depth, bounds.GetHeight() - top);
    const int left = min(depth, bounds.GetWidth());
    const int right = min(depth, bounds.GetWidth() - left);
    const Rect bands[] = {
        Rect(bounds.left, bounds.top, bounds.right, bounds.top + top),
        Rect(bounds.left, bounds.bottom - bottom, bounds.right, bounds.bottom),
        Rect(bounds.left, bounds.top + top, bounds.left + left, bounds.bottom - bottom),
        Rect(bounds.right - right, bounds.top + top, bounds.right, bounds.bottom - bottom)
    };
    UiRasterCachePolicy policy = UiRasterPolicyAA("aa/frame-accent");
    policy.allow_scale_from_bucket = false;
    policy.max_axis = 128;
    policy.max_single_image_bytes = 128 * 128 * 4;
    for(const Rect& band : bands) {
        for(int y = band.top; y < band.bottom; y += 128) {
            for(int x = band.left; x < band.right; x += 128) {
                const Rect tile(x, y, min(x + 128, band.right), min(y + 128, band.bottom));
                if(!w.IsPainting(tile + surface.TopLeft()))
                    continue;
                // Cull tiles wholly outside the outer contour or inside the
                // inner contour. Large radii must not rasterize transparent
                // interior tiles merely because their boundary bands are deep.
                auto Distance = [](Pointf point, const Rectf& rect, double rad) {
                    double qx = fabs(point.x - (rect.left + rect.right) * 0.5)
                              - (rect.GetWidth() * 0.5 - rad);
                    double qy = fabs(point.y - (rect.top + rect.bottom) * 0.5)
                              - (rect.GetHeight() * 0.5 - rad);
                    return hypot(max(qx, 0.0), max(qy, 0.0))
                         + min(max(qx, qy), 0.0) - rad;
                };
                Pointf centre((tile.left + tile.right) * 0.5,
                              (tile.top + tile.bottom) * 0.5);
                if(Distance(centre, outer, radius) >
                   hypot(tile.GetWidth(), tile.GetHeight()) * 0.5 + 1.0)
                    continue;
                Rectf inner = outer;
                inner.Deflate(thickness);
                if(!inner.IsEmpty()) {
                    double inner_radius = max(0.0, radius - thickness);
                    if(Distance(Pointf(tile.left, tile.top), inner, inner_radius) < -1.0 &&
                       Distance(Pointf(tile.right, tile.top), inner, inner_radius) < -1.0 &&
                       Distance(Pointf(tile.left, tile.bottom), inner, inner_radius) < -1.0 &&
                       Distance(Pointf(tile.right, tile.bottom), inner, inner_radius) < -1.0)
                        continue;
                }
                // Painter's empty-path clip is not a drawable empty region.
                // Reject disjoint tiles before clipping, including corner-only
                // tiles outside a selected side's 45-degree ownership polygon.
                const double l=outer.left, t=outer.top, r=outer.right, b=outer.bottom;
                const double d=min(outer.GetWidth(),outer.GetHeight())*0.5;
                const Pointf regions[4][4] = {
                    {Pointf(l,t),Pointf(r,t),Pointf(r-d,t+d),Pointf(l+d,t+d)},
                    {Pointf(r,b),Pointf(l,b),Pointf(l+d,b-d),Pointf(r-d,b-d)},
                    {Pointf(l,b),Pointf(l,t),Pointf(l+d,t+d),Pointf(l+d,b-d)},
                    {Pointf(r,t),Pointf(r,b),Pointf(r-d,b-d),Pointf(r-d,t+d)}
                };
                bool intersects=false;
                for(int side=0;side<4 && !intersects;side++) {
                    if(!(edges & (1 << side))) continue;
                    bool separated=false;
                    for(int i=0;i<4;i++) {
                        Pointf a=regions[side][i], v=regions[side][(i+1)%4]-a;
                        if(v.x==0 && v.y==0) continue;
                        double px=v.y>=0 ? tile.left : tile.right;
                        double py=v.x>=0 ? tile.bottom : tile.top;
                        if(v.x*(py-a.y)-v.y*(px-a.x)<=0) { separated=true; break; }
                    }
                    intersects=!separated;
                }
                if(!intersects) continue;
                UiRasterCacheKeyBuilder key("aa/frame-accent");
                key.Add(size).Add(tile.left).Add(tile.top).Add(tile.GetSize())
                   .Add(authored_radius).Add(fw).Add(frame).Add(edges)
                   .Add(accent.thickness).Add(alpha).Add(color);
                Image image = UiRasterCache::Get(key.Build(), policy, [=] {
                    ImageBuffer buffer(tile.GetSize());
                    buffer.SetKind(IMAGE_ALPHA);
                    Fill(~buffer, RGBAZero(), buffer.GetLength());
                    BufferPainter p(buffer, MODE_ANTIALIASED);
                    p.Translate(-tile.left, -tile.top);
                    p.Begin();
                    const double l = outer.left, t = outer.top, r = outer.right, b = outer.bottom;
                    const double d = min(outer.GetWidth(), outer.GetHeight()) * 0.5;
                    if(edges & StyledFrameAccent::Top)
                        p.Move(l,t).Line(r,t).Line(r-d,t+d).Line(l+d,t+d).Close();
                    if(edges & StyledFrameAccent::Right)
                        p.Move(r,t).Line(r,b).Line(r-d,b-d).Line(r-d,t+d).Close();
                    if(edges & StyledFrameAccent::Bottom)
                        p.Move(r,b).Line(l,b).Line(l+d,b-d).Line(r-d,b-d).Close();
                    if(edges & StyledFrameAccent::Left)
                        p.Move(l,b).Line(l,t).Line(l+d,t+d).Line(l+d,b-d).Close();
                    p.Clip();
                    p.RoundedRectangle(l, t, outer.GetWidth(), outer.GetHeight(), radius);
                    const double iw = outer.GetWidth() - 2 * thickness;
                    const double ih = outer.GetHeight() - 2 * thickness;
                    if(iw > 0 && ih > 0)
                        p.RoundedRectangle(l + thickness, t + thickness, iw, ih,
                                           max(0.0, radius - thickness));
                    p.EvenOdd().Opacity(alpha / 255.0).Fill(color);
                    p.End();
                    p.Finish();
                    return Image(buffer);
                });
                w.DrawImage(surface.left + tile.left, surface.top + tile.top, image);
            }
        }
    }
}

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
