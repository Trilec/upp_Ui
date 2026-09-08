// Control lifetime, style resolution, model notifications and attached controls.
// Shared shape/style helpers retain internal linkage in UiNodeGraph.cpp.

#include <Ui/UiGraph/UiNodeGraph.h>
#include <Ui/UiTheme.h>

#include <cmath>

namespace Upp {

namespace {
template <class T>
void NodeGraphSerializeEnum(Stream& s, T& value)
{
    byte b = (byte)value;
    s % b;
    if(s.IsLoading())
        value = (T)b;
}

Color ResolveFace(const UiFill& fill, Color fallback)
{
    return fill.IsSolid() && !IsNull(fill.color) ? fill.color : fallback;
}

RGBA PremultipliedRGBA(Color color, int alpha)
{
    alpha = clamp(alpha, 0, 255);
    if(IsNull(color) || alpha == 0)
        return RGBAZero();
    auto scale = [alpha](int channel) {
        return (byte)((channel * alpha + 127) / 255);
    };
    RGBA out;
    out.r = scale(color.GetR());
    out.g = scale(color.GetG());
    out.b = scale(color.GetB());
    out.a = (byte)alpha;
    return out;
}

Pointf NormalizeVector(Pointf v)
{
    double d = std::sqrt(v.x * v.x + v.y * v.y);
    return d > 1e-9 ? Pointf(v.x / d, v.y / d) : Pointf(0, 0);
}

double VectorLength(Pointf v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

double SmoothUnit(double value, double low, double high)
{
    if(high <= low)
        return value >= high ? 1.0 : 0.0;
    double t = minmax((value - low) / (high - low), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

Color BlendGraphColor(Color background, Color foreground, double amount)
{
    if(IsNull(foreground))
        return foreground;
    if(IsNull(background))
        background = White();
    amount = minmax(amount, 0.0, 1.0);
    return Color(fround(background.GetR() + (foreground.GetR() - background.GetR()) * amount),
                 fround(background.GetG() + (foreground.GetG() - background.GetG()) * amount),
                 fround(background.GetB() + (foreground.GetB() - background.GetB()) * amount));
}

double GraphTextScale(double zoom)
{
    zoom = max(0.01, zoom);
    if(zoom <= 1.0)
        return max(0.42, std::pow(zoom, 0.45));
    return min(2.0, std::sqrt(zoom));
}

Font ScaleGraphFont(Font font, double zoom)
{
    int authored = max(1, font.GetHeight());
    int height = max(DPI(6), fround(authored * GraphTextScale(zoom)));
    font.Height(height);
    return font;
}

Size ScaleGraphIcon(Size authored, double zoom)
{
    double scale = GraphTextScale(zoom);
    return Size(max(1, fround(authored.cx * scale)),
                max(1, fround(authored.cy * scale)));
}

Color SelectedFrameColor(const UiGraphNodeStyle& style)
{
    return IsNull(style.metrics.focus_color) ? Color(37, 99, 235)
                                              : style.metrics.focus_color;
}

Vector<Pointf> NodeShapePath(const UiGraphNode& node, const Rect& rect)
{
    Vector<Pointf> v;
    double sx = node.size.cx > 0.0 ? rect.GetWidth() / node.size.cx : 1.0;
    double sy = node.size.cy > 0.0 ? rect.GetHeight() / node.size.cy : 1.0;
    double radius = max(0.0, node.corner_radius * max(0.01, min(sx, sy)));
    double l = rect.left, t = rect.top, r = rect.right, b = rect.bottom;
    double cx = (l + r) * 0.5, cy = (t + b) * 0.5;

    switch(node.shape) {
    case UiGraphNodeShape::Circle:
    case UiGraphNodeShape::Ellipse:
        return UiGeometry::EllipsePath(rect);
    case UiGraphNodeShape::Diamond:
        v << Pointf(cx, t) << Pointf(r, cy) << Pointf(cx, b) << Pointf(l, cy);
        return UiGeometry::RoundedPolygon(v, radius);
    case UiGraphNodeShape::Triangle:
        v << Pointf(cx, t) << Pointf(r, b) << Pointf(l, b);
        return UiGeometry::RoundedPolygon(v, radius);
    case UiGraphNodeShape::Hexagon: {
        double dx = rect.GetWidth() * 0.24;
        v << Pointf(l + dx, t) << Pointf(r - dx, t) << Pointf(r, cy)
          << Pointf(r - dx, b) << Pointf(l + dx, b) << Pointf(l, cy);
        return UiGeometry::RoundedPolygon(v, radius);
    }
    case UiGraphNodeShape::Capsule:
        return UiGeometry::CapsulePath(rect);
    case UiGraphNodeShape::Cloud: {
        double w = rect.GetWidth(), h = rect.GetHeight();
        v << Pointf(l + w * 0.08, t + h * 0.62)
          << Pointf(l + w * 0.05, t + h * 0.42)
          << Pointf(l + w * 0.20, t + h * 0.28)
          << Pointf(l + w * 0.36, t + h * 0.30)
          << Pointf(l + w * 0.45, t + h * 0.10)
          << Pointf(l + w * 0.66, t + h * 0.12)
          << Pointf(l + w * 0.75, t + h * 0.30)
          << Pointf(l + w * 0.92, t + h * 0.36)
          << Pointf(l + w * 0.95, t + h * 0.60)
          << Pointf(l + w * 0.82, t + h * 0.80)
          << Pointf(l + w * 0.18, t + h * 0.82);
        return UiGeometry::RoundedPolygon(v, max(radius, min(w, h) * 0.10));
    }
    case UiGraphNodeShape::Document: {
        double fold = min(rect.GetWidth(), rect.GetHeight()) * 0.22;
        v << Pointf(l, t) << Pointf(r - fold, t) << Pointf(r, t + fold)
          << Pointf(r, b) << Pointf(l, b);
        return UiGeometry::RoundedPolygon(v, radius);
    }
    case UiGraphNodeShape::Database: {
        v << Pointf(l, t + rect.GetHeight() * 0.14)
          << Pointf(l + rect.GetWidth() * 0.18, t)
          << Pointf(r - rect.GetWidth() * 0.18, t)
          << Pointf(r, t + rect.GetHeight() * 0.14)
          << Pointf(r, b - rect.GetHeight() * 0.14)
          << Pointf(r - rect.GetWidth() * 0.18, b)
          << Pointf(l + rect.GetWidth() * 0.18, b)
          << Pointf(l, b - rect.GetHeight() * 0.14);
        return UiGeometry::RoundedPolygon(v, max(radius, rect.GetHeight() * 0.08));
    }
    case UiGraphNodeShape::Square:
    case UiGraphNodeShape::LegacyRectangle:
    case UiGraphNodeShape::RoundedRectangle:
    case UiGraphNodeShape::Custom:
    default:
        v << Pointf(l, t) << Pointf(r, t) << Pointf(r, b) << Pointf(l, b);
        return UiGeometry::RoundedPolygon(v,
                              node.shape == UiGraphNodeShape::LegacyRectangle ||
                              node.shape == UiGraphNodeShape::Custom ? 0.0 : radius);
    }
}

Point ShapeBoundaryAnchor(const Vector<Pointf>& path, const Rect& fallback,
                          UiGraphPortSide side, double axis)
{
    bool vertical_side = side == UiGraphPortSide::Left || side == UiGraphPortSide::Right;
    bool choose_min = side == UiGraphPortSide::Left || side == UiGraphPortSide::Top;
    double best = 0.0;
    bool found = false;
    auto consider = [&](double value) {
        if(!found || (choose_min ? value < best : value > best))
            best = value;
        found = true;
    };

    for(int i = 0; i < path.GetCount(); i++) {
        Pointf a = path[i];
        Pointf b = path[(i + 1) % path.GetCount()];
        if(vertical_side) {
            if(abs(a.y - b.y) <= 1e-9) {
                if(abs(axis - a.y) <= 0.75) {
                    consider(a.x);
                    consider(b.x);
                }
                continue;
            }
            if(axis < min(a.y, b.y) - 1e-9 || axis > max(a.y, b.y) + 1e-9)
                continue;
            double t = (axis - a.y) / (b.y - a.y);
            if(t >= -1e-9 && t <= 1.0 + 1e-9)
                consider(a.x + (b.x - a.x) * t);
        }
        else {
            if(abs(a.x - b.x) <= 1e-9) {
                if(abs(axis - a.x) <= 0.75) {
                    consider(a.y);
                    consider(b.y);
                }
                continue;
            }
            if(axis < min(a.x, b.x) - 1e-9 || axis > max(a.x, b.x) + 1e-9)
                continue;
            double t = (axis - a.x) / (b.x - a.x);
            if(t >= -1e-9 && t <= 1.0 + 1e-9)
                consider(a.y + (b.y - a.y) * t);
        }
    }

    if(vertical_side) {
        int y = fround(minmax(axis, (double)fallback.top, (double)fallback.bottom));
        int x = found ? fround(best)
                      : (side == UiGraphPortSide::Left ? fallback.left : fallback.right);
        return Point(x, y);
    }
    int x = fround(minmax(axis, (double)fallback.left, (double)fallback.right));
    int y = found ? fround(best)
                  : (side == UiGraphPortSide::Top ? fallback.top : fallback.bottom);
    return Point(x, y);
}

Vector<Pointf> InflatePath(const Vector<Pointf>& path, double amount)
{
    if(path.IsEmpty() || amount <= 0.0)
        return clone(path);
    double left = path[0].x, right = path[0].x;
    double top = path[0].y, bottom = path[0].y;
    for(const Pointf& p : path) {
        left = min(left, p.x); right = max(right, p.x);
        top = min(top, p.y); bottom = max(bottom, p.y);
    }
    double width = max(1.0, right - left);
    double height = max(1.0, bottom - top);
    Pointf centre((left + right) * 0.5, (top + bottom) * 0.5);
    double sx = (width + amount * 2.0) / width;
    double sy = (height + amount * 2.0) / height;
    Vector<Pointf> out;
    out.Reserve(path.GetCount());
    for(const Pointf& p : path)
        out.Add(Pointf(centre.x + (p.x - centre.x) * sx,
                       centre.y + (p.y - centre.y) * sy));
    return out;
}

void PainterPath(Painter& p, const Vector<Pointf>& points)
{
    if(points.IsEmpty())
        return;
    p.Move(points[0]);
    for(int i = 1; i < points.GetCount(); i++)
        p.Line(points[i]);
    p.Close();
}

void FillPath(Painter& p, const Vector<Pointf>& points, Color color)
{
    if(points.GetCount() < 3 || IsNull(color))
        return;
    p.Begin();
    PainterPath(p, points);
    p.Fill(color);
    p.End();
}

void FillPath(Painter& p, const Vector<Pointf>& points, RGBA color)
{
    if(points.GetCount() < 3)
        return;
    p.Begin();
    PainterPath(p, points);
    p.Fill(color);
    p.End();
}

void StrokePath(Painter& p, const Vector<Pointf>& points, double width, Color color, bool close = true)
{
    if(points.GetCount() < 2 || width <= 0.0 || IsNull(color))
        return;
    p.Begin();
    p.Move(points[0]);
    for(int i = 1; i < points.GetCount(); i++)
        p.Line(points[i]);
    if(close)
        p.Close();
    p.Stroke(width, color);
    p.End();
}

void StrokeStyledPath(Painter& p, const Vector<Pointf>& points, double width, Color color,
                      const StyledMetrics& metrics)
{
    if(points.GetCount() < 2 || width <= 0.0 || IsNull(color))
        return;
    p.Begin();
    p.Move(points[0]);
    for(int i = 1; i < points.GetCount(); i++)
        p.Line(points[i]);
    p.Close();
    if(metrics.dashed && !metrics.dash_pattern.IsEmpty())
        p.Dash(metrics.dash_pattern, 0.0);
    p.Stroke(width, color);
    p.End();
}

void StrokeSegment(Painter& p, Pointf a, Pointf b, double width, Color color)
{
    if(width <= 0.02)
        return;
    p.Begin();
    p.Move(a);
    p.Line(b);
    p.Stroke(width, color);
    p.End();
}

void StrokePolyline(Painter& p, const Vector<Pointf>& points, double width, Color color,
                    UiLineStyle line_style, double dash_length, double dash_gap)
{
    if(points.GetCount() < 2 || width <= 0.02)
        return;
    if(line_style == SOLID) {
        p.Begin();
        p.Move(Pointf(points[0].x, points[0].y));
        for(int i = 1; i < points.GetCount(); i++)
            p.Line(Pointf(points[i].x, points[i].y));
        p.Stroke(width, color);
        p.End();
        return;
    }

    double on = line_style == DOTTED ? max(width, 0.35) : max(0.35, dash_length);
    double off = max(0.35, dash_gap);
    bool drawing = true;
    double remaining = on;
    for(int i = 1; i < points.GetCount(); i++) {
        Pointf a(points[i - 1].x, points[i - 1].y);
        Pointf b(points[i].x, points[i].y);
        Pointf d = b - a;
        double len = VectorLength(d);
        if(len <= 1e-9)
            continue;
        Pointf unit = d * (1.0 / len);
        double used = 0.0;
        while(used < len - 1e-9) {
            double step = min(remaining, len - used);
            if(drawing)
                StrokeSegment(p, a + unit * used, a + unit * (used + step), width, color);
            used += step;
            remaining -= step;
            if(remaining <= 1e-9) {
                drawing = !drawing;
                remaining = drawing ? on : off;
            }
        }
    }
}

void PaintArrow(Painter& p, const Vector<Pointf>& points,
                UiGraphArrowStyle arrow, double size, Color color)
{
    if(arrow == UiGraphArrowStyle::Inherit || arrow == UiGraphArrowStyle::None ||
       points.GetCount() < 2 || size <= 0.20)
        return;
    Pointf tip(points.Top().x, points.Top().y);
    Pointf prev(points[points.GetCount() - 2].x, points[points.GetCount() - 2].y);
    Pointf dir = NormalizeVector(tip - prev);
    if(VectorLength(dir) <= 1e-9)
        return;
    Pointf side(-dir.y, dir.x);
    Pointf base = tip - dir * size;

    if(arrow == UiGraphArrowStyle::Open) {
        double stroke = max(0.35, min(1.5, size * 0.22));
        StrokeSegment(p, tip, base + side * size * 0.55, stroke, color);
        StrokeSegment(p, tip, base - side * size * 0.55, stroke, color);
        return;
    }
    if(arrow == UiGraphArrowStyle::Circle) {
        // The route endpoint is the node boundary. Centering the marker on that
        // endpoint leaves half of it underneath the node, which is painted
        // afterward and visually clips the circle. Keep the marker tangent to
        // the endpoint instead: its forward edge touches the port while the
        // complete marker remains on the connector side.
        double radius = max(0.5, size * 0.45);
        Pointf centre = tip - dir * radius;
        p.Begin();
        p.Circle(centre, radius);
        p.Fill(color);
        p.End();
        return;
    }

    Vector<Pointf> shape;
    if(arrow == UiGraphArrowStyle::Diamond)
        shape << tip << base + side * size * 0.5 << tip - dir * size * 1.55 << base - side * size * 0.5;
    else
        shape << tip << base + side * size * 0.55 << base - side * size * 0.55;
    FillPath(p, shape, color);
}

String PortLabel(const UiGraphPort& port, bool show_type)
{
    String label = port.title.IsEmpty() ? port.id : port.title;
    if(show_type)
        label << "  ·  " << UiGraphDataTypeName(port.type, port.custom_type);
    return label;
}

StyledState ToStyledState(UiGraphVisualState state)
{
    switch(state) {
    case UiGraphVisualState::Hot:      return ST_HOT;
    case UiGraphVisualState::Selected: return ST_PRESSED;
    case UiGraphVisualState::Disabled: return ST_DISABLED;
    case UiGraphVisualState::Normal:
    default:                           return ST_NORMAL;
    }
}

int ScaleMetric(int value, double zoom)
{
    if(value <= 0)
        return 0;
    return max(1, fround(value * max(0.01, zoom)));
}

Rect ScaleThickness(const Rect& value, double zoom)
{
    return Rect(ScaleMetric(value.left, zoom), ScaleMetric(value.top, zoom),
                ScaleMetric(value.right, zoom), ScaleMetric(value.bottom, zoom));
}

StyledMetrics ScaleNodeMetrics(const StyledMetrics& source, double zoom)
{
    StyledMetrics out = source;
    out.content_margin = ScaleThickness(source.content_margin, zoom);
    out.radius = ScaleMetric(source.radius, zoom);
    out.frame_width = ScaleMetric(source.frame_width, zoom);
    out.focus_margin = ScaleMetric(source.focus_margin, zoom);
    out.highlight.thickness = ScaleMetric(source.highlight.thickness, zoom);
    out.highlight.offset_x = fround(source.highlight.offset_x * zoom);
    out.highlight.offset_y = fround(source.highlight.offset_y * zoom);
    out.shadow.distance = ScaleMetric(source.shadow.distance, zoom);
    out.shadow.offset_x = fround(source.shadow.offset_x * zoom);
    out.shadow.offset_y = fround(source.shadow.offset_y * zoom);
    return out;
}

StyledSkin ScaleNodeSkin(const StyledSkin& source, double zoom)
{
    StyledSkin out = source;
    out.slice = source.slice;
    out.content_inset = ScaleThickness(source.content_inset, zoom);
    return out;
}

bool UsesLegacyRectangularStyledSurface(UiGraphNodeShape shape)
{
    return shape == UiGraphNodeShape::LegacyRectangle ||
           shape == UiGraphNodeShape::RoundedRectangle ||
           shape == UiGraphNodeShape::Square ||
           shape == UiGraphNodeShape::Capsule;
}

UiRole ToUiRole(UiGraphNodeRole role)
{
    switch(role) {
    case UiGraphNodeRole::Subtle: return UiRole::Subtle;
    case UiGraphNodeRole::Accent: return UiRole::Accent;
    case UiGraphNodeRole::Alert:  return UiRole::Alert;
    default:                      return UiRole::Standard;
    }
}

void ApplyRoleColors(UiGraphNodeStyle& style, const UiThemeDetail::MinimalRoleColors& c)
{
    Color face[4] = { c.face, c.face_hot, c.face_pressed, c.face_disabled };
    Color frame[4] = { c.frame, c.frame_hot, c.frame_pressed, c.frame_disabled };
    Color ink[4] = { c.ink, c.ink_hot, c.ink_pressed, c.ink_disabled };
    for(int i = 0; i < 4; i++) {
        style.palette.face[i] = UiFill::Solid(face[i]);
        style.palette.frame[i] = frame[i];
        style.palette.ink[i] = ink[i];
        style.palette.icon[i] = ink[i];
        style.header_face[i] = Blend(face[i], i == ST_DISABLED ? frame[i] : Black(), 18);
        style.title_ink[i] = ink[i];
        style.subtitle_ink[i] = Blend(ink[i], face[i], 70);
        style.description_ink[i] = Blend(ink[i], face[i], 88);
        style.port_label_ink[i] = ink[i];
        style.port_frame[i] = frame[i];
    }
}

UiNodeGraph::Style ResolveNodeGraphTheme(const UiThemeContext& context)
{
    UiThemeContext ctx = UiThemeDetail::NormalizeContext(context);
    UiNodeGraph::Style s = UiNodeGraph::StyleDefault();
    UiThemeDetail::MinimalRoleColors standard = UiThemeDetail::MinimalRole(ctx.mode, UiRole::Standard);
    ApplyRoleColors(s.node, standard);

    for(int i = 0; i < 4; i++) {
        s.canvas_palette.face[i] = UiFill::Solid(standard.face);
        s.canvas_palette.frame[i] = standard.frame;
        s.canvas_palette.ink[i] = standard.ink;
        s.canvas_palette.icon[i] = standard.ink;
        s.edge.color[i] = i == ST_HOT ? Color(59, 130, 246)
                        : i == ST_PRESSED ? Color(37, 99, 235)
                        : i == ST_DISABLED ? standard.frame_disabled
                        : standard.frame_pressed;
        s.edge.label_ink[i] = i == ST_DISABLED ? standard.ink_disabled : standard.ink;
    }

    bool dark = UiThemeDetail::ResolveEffectiveMode(ctx.mode) == UiThemeMode::Dark;
    s.grid_minor = dark ? Color(43, 43, 43) : Color(233, 237, 243);
    s.grid_major = dark ? Color(58, 58, 58) : Color(208, 218, 229);
    s.edge.label_background = standard.face;
    // Graph selection is interaction chrome, not a Standard-role accent.
    // Keep it recognisably blue in every theme; the fill remains deliberately light.
    s.selection_box_fill = Color(59, 130, 246);
    s.selection_box_frame = Color(59, 130, 246);

    switch(ctx.preset) {
    case UiThemePreset::Linear:
        s.canvas_metrics.radius = 0;
        s.node.metrics.radius = 0;
        s.node.metrics.shadow.enabled = false;
        break;
    case UiThemePreset::Solid:
        s.node.metrics.frame_width = 0;
        s.node.metrics.shadow.enabled = false;
        break;
    case UiThemePreset::Outline:
        s.node.metrics.shadow.enabled = false;
        s.node.metrics.frame_width = DPI(2);
        break;
    case UiThemePreset::Compact:
        s.grid_size = DPI(16);
        s.node.metrics.content_margin = Rect(DPI(7), DPI(6), DPI(7), DPI(6));
        s.node.header_height = DPI(42);
        s.node.port_spacing = DPI(19);
        s.node.content_cell_reserve = DPI(25);
        s.fit_margin = DPI(24);
        break;
    case UiThemePreset::Layered:
        s.node.metrics.shadow.enabled = true;
        s.node.metrics.shadow.distance = DPI(7);
        s.node.metrics.shadow.alpha = 52;
        break;
    case UiThemePreset::Pill:
        s.canvas_metrics.radius = DPI(10);
        s.node.metrics.radius = DPI(16);
        break;
    case UiThemePreset::Minimal:
    default:
        break;
    }
    return s;
}

} // namespace

bool UiNodeGraph::PointInPolygon(const Vector<Pointf>& polygon, Pointf point)
{
    // Graph's historical boundary-inclusive hit contract allows a small
    // interaction tolerance around the prepared silhouette.
    if(UiGeometry::ContainsPolygon(polygon, point))
        return true;
    for(int i = 0; i < polygon.GetCount(); i++)
        if(UiGeometry::DistanceToSegment(point, polygon[i],
                                         polygon[(i + 1) % polygon.GetCount()]) <= 0.75)
            return true;
    return false;
}

bool UiNodeGraph::ShapeContains(const UiGraphNode& node, const Rect& surface, Point point)
{
    if(surface.IsEmpty() || !surface.Contains(point))
        return false;
    if(node.shape == UiGraphNodeShape::LegacyRectangle ||
       node.shape == UiGraphNodeShape::Square ||
       node.shape == UiGraphNodeShape::Custom)
        return true;
    return PointInPolygon(NodeShapePath(node, surface), Pointf(point.x, point.y));
}

void UiGraphNodeStyle::Serialize(Stream& s)
{
    s % palette % metrics % skin;
    for(int i = 0; i < 4; i++)
        s % header_face[i] % title_ink[i] % subtitle_ink[i]
          % description_ink[i] % port_frame[i] % port_label_ink[i];
    s % title_font % subtitle_font % description_font % port_font
      % text_align_h % icon_side % icon_render_mode % icon_size
      % header_height % title_subtitle_gap % subtitle_description_gap
      % icon_text_gap % port_radius % port_hit_radius % port_label_gap
      % port_spacing % content_cell_side % content_cell_reserve
      % content_cell_gap % content_cell_min_zoom
      % show_header_band % show_icon % show_description
      % show_port_labels % show_port_type;
    if(s.IsLoading()) {
        icon_size.cx = max(0, icon_size.cx);
        icon_size.cy = max(0, icon_size.cy);
        content_cell_reserve = max(0, content_cell_reserve);
        content_cell_gap = max(0, content_cell_gap);
        content_cell_min_zoom = max(0.0, content_cell_min_zoom);
    }
}

void UiGraphEdgeStyle::Serialize(Stream& s)
{
    for(int i = 0; i < 4; i++)
        s % color[i] % label_ink[i] % width[i];
    NodeGraphSerializeEnum(s, route);
    NodeGraphSerializeEnum(s, line_style);
    NodeGraphSerializeEnum(s, arrow);
    s % label_font % bezier_tension % orthogonal_lead % orthogonal_radius
      % dash_length % dash_gap % arrow_size % interaction_width
      % draw_label_background % label_background;
}

void UiNodeGraph::Style::Serialize(Stream& s)
{
    s % canvas_palette % canvas_metrics % canvas_skin;
    node.Serialize(s);
    edge.Serialize(s);
    s % grid_minor % grid_major % grid_size % major_grid_every
      % show_grid % snap_to_grid % show_origin % min_zoom % max_zoom
      % zoom_step % fit_margin % selection_box_fill % selection_box_frame
      % selection_box_alpha;
}

const UiNodeGraph::Style& UiNodeGraph::StyleDefault()
{
    static Style s;
    ONCELOCK {
        for(int i = 0; i < 4; i++) {
            s.canvas_palette.face[i] = UiFill::Solid(Color(248, 250, 252));
            s.canvas_palette.frame[i] = Color(203, 213, 225);
            s.canvas_palette.ink[i] = Color(15, 23, 42);
            s.canvas_palette.icon[i] = Color(71, 85, 105);

            s.node.palette.face[i] = UiFill::Solid(White());
            s.node.palette.frame[i] = Color(148, 163, 184);
            s.node.palette.ink[i] = Color(15, 23, 42);
            s.node.palette.icon[i] = Color(71, 85, 105);
            s.node.header_face[i] = Color(241, 245, 249);
            s.node.title_ink[i] = Color(15, 23, 42);
            s.node.subtitle_ink[i] = Color(100, 116, 139);
            s.node.description_ink[i] = Color(71, 85, 105);
            s.node.port_frame[i] = White();
            s.node.port_label_ink[i] = Color(51, 65, 85);
            s.edge.color[i] = Color(100, 116, 139);
            s.edge.label_ink[i] = Color(51, 65, 85);
        }
        s.node.palette.frame[ST_HOT] = Color(59, 130, 246);
        s.node.palette.frame[ST_PRESSED] = Color(37, 99, 235);
        s.node.palette.frame[ST_DISABLED] = Color(203, 213, 225);
        s.node.palette.face[ST_DISABLED] = UiFill::Solid(Color(241, 245, 249));
        s.node.palette.ink[ST_DISABLED] = Color(148, 163, 184);
        s.edge.color[ST_HOT] = Color(59, 130, 246);
        s.edge.color[ST_PRESSED] = Color(37, 99, 235);
        s.edge.color[ST_DISABLED] = Color(203, 213, 225);

        s.canvas_metrics = StyledMetrics();
        s.canvas_metrics.face_enabled = true;
        s.canvas_metrics.frame_enabled = true;
        s.canvas_metrics.frame_width = DPI(1);
        s.canvas_metrics.radius = DPI(4);
        s.canvas_metrics.focus_enabled = true;
        s.canvas_metrics.focus_color = Color(59, 130, 246);
        s.canvas_metrics.focus_alpha = 170;
        s.canvas_metrics.focus_margin = DPI(2);
        s.canvas_skin = StyledSkin();

        s.node.metrics = StyledMetrics();
        s.node.metrics.face_enabled = true;
        s.node.metrics.frame_enabled = true;
        s.node.metrics.frame_width = DPI(1);
        s.node.metrics.radius = DPI(10);
        s.node.metrics.content_margin = Rect(DPI(10), DPI(9), DPI(10), DPI(9));
        s.node.metrics.focus_enabled = true;
        s.node.metrics.focus_margin = DPI(1);
        s.node.metrics.focus_alpha = 190;
        s.node.metrics.focus_color = Color(37, 99, 235);
        s.node.metrics.shadow.enabled = true;
        s.node.metrics.shadow.distance = DPI(5);
        s.node.metrics.shadow.offset_x = DPI(2);
        s.node.metrics.shadow.offset_y = DPI(2);
        s.node.metrics.shadow.alpha = 42;
        s.node.metrics.shadow.color = Color(15, 23, 42);
        s.node.metrics.shadow.mode = SHADOW_CURVE;
        s.node.metrics.shadow.curve = ShadowSoft();
        s.node.skin = StyledSkin();
        s.node.content_cell_min_zoom = 0.42;
        s.selection_box_fill = Color(59, 130, 246);
        s.selection_box_frame = Color(59, 130, 246);
    }
    return s;
}

UiGraphNodeStyle UiNodeGraph::StyleForRole(const UiGraphNodeStyle& base, UiGraphNodeRole role)
{
    if(role == UiGraphNodeRole::Standard)
        return base;
    UiGraphNodeStyle out = base;
    UiThemeContext ctx = UiTheme::GetContext();
    ApplyRoleColors(out, UiThemeDetail::MinimalRole(ctx.mode, ToUiRole(role)));
    return out;
}

UiNodeGraph::UiNodeGraph()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
    , model_(&internal_model_)
{
    BackPaint();
    WantFocus();
    SyncThemeStyle();
    BindModel(internal_model_);
}

UiNodeGraph::~UiNodeGraph()
{
    CancelMode();
    node_ctrls_.Clear();
}

UiNodeGraph::Style& UiNodeGraph::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    theme_revision_ = 0;
    return style_;
}

const UiNodeGraph::Style& UiNodeGraph::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiNodeGraph*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiNodeGraph::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    themed_style_ = ResolveNodeGraphTheme(UiTheme::GetContext());
    theme_revision_ = revision;
}

UiNodeGraph& UiNodeGraph::SetCustomStyle(const Style& style)
{
    style_ = style;
    has_custom_style_ = true;
    theme_revision_ = 0;
    OnStyleChanged();
    return *this;
}

UiNodeGraph& UiNodeGraph::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    style_ = StyleDefault();
    has_custom_style_ = false;
    theme_revision_ = 0;
    OnStyleChanged();
    return *this;
}

void UiNodeGraph::OnStyleChanged()
{
    ClampZoom();
    InvalidateSpatialIndex();
    InvalidateGeometry();
    PrepareViewGeometry();
    UpdateAttachedCtrls();
    RequestInitialFitLayout();
    Refresh();
}

UiNodeGraph& UiNodeGraph::SetExtensionBounds(const ExtensionBounds& bounds)
{
    extension_bounds_ = bounds;
    extension_bounds_.node_paint_margin_px = max(0, extension_bounds_.node_paint_margin_px);
    extension_bounds_.node_hit_margin_px = max(0, extension_bounds_.node_hit_margin_px);
    extension_bounds_.edge_paint_margin_px = max(0, extension_bounds_.edge_paint_margin_px);
    extension_bounds_.resolved_port_hit_radius_px = max(0, extension_bounds_.resolved_port_hit_radius_px);
    extension_bounds_.resolved_edge_hit_width_px = max(0, extension_bounds_.resolved_edge_hit_width_px);
    extension_bounds_.custom_route_world_margin = max(0.0, extension_bounds_.custom_route_world_margin);

    InvalidateSpatialIndex();
    InvalidateGeometry();
    PrepareViewGeometry();
    UpdateAttachedCtrls();
    RequestInitialFitLayout();
    Refresh();
    return *this;
}

UiNodeGraph& UiNodeGraph::SetLodPolicy(const LodPolicy& policy)
{
    lod_policy_ = policy;
    lod_policy_.full_detail_zoom = max(0.01, lod_policy_.full_detail_zoom);
    lod_policy_.edge_simplify_zoom = min(lod_policy_.full_detail_zoom,
                                         max(0.01, lod_policy_.edge_simplify_zoom));
    lod_policy_.minimal_edge_zoom = min(lod_policy_.edge_simplify_zoom,
                                        max(0.01, lod_policy_.minimal_edge_zoom));
    lod_policy_.edge_hide_zoom = min(lod_policy_.minimal_edge_zoom,
                                     max(0.0, lod_policy_.edge_hide_zoom));
    lod_policy_.edge_label_zoom = max(0.0, lod_policy_.edge_label_zoom);
    lod_policy_.arrow_zoom = max(0.0, lod_policy_.arrow_zoom);
    lod_policy_.shadow_zoom = max(0.0, lod_policy_.shadow_zoom);
    lod_policy_.icon_zoom = max(0.0, lod_policy_.icon_zoom);
    lod_policy_.title_zoom = max(0.0, lod_policy_.title_zoom);
    lod_policy_.secondary_text_zoom = max(0.0, lod_policy_.secondary_text_zoom);
    lod_policy_.port_zoom = max(0.0, lod_policy_.port_zoom);
    lod_policy_.port_label_zoom = max(lod_policy_.port_zoom, lod_policy_.port_label_zoom);
    lod_policy_.route_edit_zoom = max(lod_policy_.edge_simplify_zoom, lod_policy_.route_edit_zoom);
    lod_policy_.paint_query_margin_low = max(0.0, lod_policy_.paint_query_margin_low);
    lod_policy_.paint_query_margin_full = max(lod_policy_.paint_query_margin_low,
                                              lod_policy_.paint_query_margin_full);
    lod_policy_.selection_outline_width = max(1.0, lod_policy_.selection_outline_width);
    InvalidateGeometry();
    PrepareViewGeometry();
    UpdateAttachedCtrls();
    RequestInitialFitLayout();
    Refresh();
    return *this;
}

double UiNodeGraph::EdgeDetailFactor() const
{
    return SmoothUnit(zoom_, lod_policy_.edge_hide_zoom, lod_policy_.full_detail_zoom);
}

double UiNodeGraph::ShadowDetailFactor() const
{
    return SmoothUnit(zoom_, lod_policy_.shadow_zoom, lod_policy_.full_detail_zoom);
}

double UiNodeGraph::GetPaintQueryMargin() const
{
    double t = SmoothUnit(zoom_, lod_policy_.minimal_edge_zoom, lod_policy_.full_detail_zoom);
    double normal = lod_policy_.paint_query_margin_low
                  + (lod_policy_.paint_query_margin_full - lod_policy_.paint_query_margin_low) * t;
    double extension = max(max(extension_bounds_.node_paint_margin_px,
                               extension_bounds_.node_hit_margin_px),
                           extension_bounds_.edge_paint_margin_px);
    extension = max(extension, (double)extension_bounds_.resolved_port_hit_radius_px);
    extension = max(extension, (double)extension_bounds_.resolved_edge_hit_width_px);
    return max(normal, extension + DPI(2));
}

double UiNodeGraph::EffectiveEdgeHitWidth(const UiGraphEdgeStyle& style) const
{
    return max(2.0, style.interaction_width * max(0.25, min(1.0, zoom_)));
}

double UiNodeGraph::MaxEdgeHitWidthForBroadPhase() const
{
    double width = EffectiveEdgeHitWidth(GetEffectiveStyle().edge);
    for(int i = 0; i < edge_styles_.GetCount(); i++)
        width = max(width, EffectiveEdgeHitWidth(edge_styles_[i]));
    width = max(width, (double)extension_bounds_.resolved_edge_hit_width_px);
    return width;
}

int UiNodeGraph::EffectivePortHitRadius(const UiGraphNodeStyle& style) const
{
    return max(DPI(2), fround(style.port_hit_radius * max(0.35, zoom_)));
}

int UiNodeGraph::MaxPortHitRadiusForBroadPhase() const
{
    int radius = EffectivePortHitRadius(GetEffectiveStyle().node);
    for(int i = 0; i < node_styles_.GetCount(); i++)
        radius = max(radius, EffectivePortHitRadius(node_styles_[i]));
    radius = max(radius, extension_bounds_.resolved_port_hit_radius_px);
    return radius;
}

Rect UiNodeGraph::GetShapeSafeContentRect(const UiGraphNode& node, const Rect& content,
                                          const Rect& surface) const
{
    if(content.IsEmpty() || surface.IsEmpty())
        return RectC(0, 0, 0, 0);

    auto centred = [&](double width_factor, double height_factor,
                       double offset_y = 0.0) {
        int w = max(1, fround(surface.GetWidth() * width_factor));
        int h = max(1, fround(surface.GetHeight() * height_factor));
        int x = surface.left + (surface.GetWidth() - w) / 2;
        int y = surface.top + (surface.GetHeight() - h) / 2
              + fround(surface.GetHeight() * offset_y);
        return content & RectC(x, y, w, h);
    };

    switch(node.shape) {
    case UiGraphNodeShape::Circle:
    case UiGraphNodeShape::Ellipse:
        return centred(0.68, 0.68);
    case UiGraphNodeShape::Diamond:
        return centred(0.50, 0.50);
    case UiGraphNodeShape::Triangle:
        return centred(0.46, 0.40, 0.12);
    case UiGraphNodeShape::Hexagon:
        return centred(0.72, 0.82);
    case UiGraphNodeShape::Capsule:
        if(surface.GetWidth() >= surface.GetHeight()) {
            int inset = max(1, fround(surface.GetHeight() * 0.28));
            return content & Rect(surface.left + inset, surface.top,
                                  surface.right - inset, surface.bottom);
        }
        else {
            int inset = max(1, fround(surface.GetWidth() * 0.28));
            return content & Rect(surface.left, surface.top + inset,
                                  surface.right, surface.bottom - inset);
        }
    case UiGraphNodeShape::Cloud:
        return centred(0.64, 0.56);
    case UiGraphNodeShape::Document: {
        int fold = max(1, fround(min(surface.GetWidth(), surface.GetHeight()) * 0.22));
        return content & Rect(surface.left, surface.top,
                              max(surface.left, surface.right - fold), surface.bottom);
    }
    case UiGraphNodeShape::Database:
        return centred(0.82, 0.64);
    case UiGraphNodeShape::LegacyRectangle:
    case UiGraphNodeShape::RoundedRectangle:
    case UiGraphNodeShape::Square:
    case UiGraphNodeShape::Custom:
    default:
        return content;
    }
}

UiNodeGraph& UiNodeGraph::SetNodeStyleClass(const String& name, const UiGraphNodeStyle& style)
{
    if(name.IsEmpty())
        return *this;

    PrepareViewGeometry();
    Vector<UiGraphNodeRef> affected;
    Rect damage;
    if(model_)
        for(int n = 0; n < node_geometry_.GetCount(); n++) {
            UiGraphNodeRef ref = node_geometry_[n].ref;
            const UiGraphNode* node = model_->FindNode(ref);
            if(node && node->style_class == name) {
                affected.Add(ref);
                damage |= GetNodeDamage(ref);
            }
        }

    int i = node_styles_.Find(name);
    if(i < 0) node_styles_.Add(name, style); else node_styles_[i] = style;

    for(UiGraphNodeRef ref : affected)
        damage |= RebuildNodeAndEdges(ref);
    if(!affected.IsEmpty()) {
        UpdateAttachedCtrls();
        RefreshDamage(damage);
    }
    return *this;
}

UiNodeGraph& UiNodeGraph::RemoveNodeStyleClass(const String& name)
{
    int i = node_styles_.Find(name);
    if(i < 0)
        return *this;

    PrepareViewGeometry();
    Vector<UiGraphNodeRef> affected;
    Rect damage;
    if(model_)
        for(int n = 0; n < node_geometry_.GetCount(); n++) {
            UiGraphNodeRef ref = node_geometry_[n].ref;
            const UiGraphNode* node = model_->FindNode(ref);
            if(node && node->style_class == name) {
                affected.Add(ref);
                damage |= GetNodeDamage(ref);
            }
        }

    node_styles_.Remove(i);
    for(UiGraphNodeRef ref : affected)
        damage |= RebuildNodeAndEdges(ref);
    if(!affected.IsEmpty()) {
        UpdateAttachedCtrls();
        RefreshDamage(damage);
    }
    return *this;
}

UiNodeGraph& UiNodeGraph::SetEdgeStyleClass(const String& name, const UiGraphEdgeStyle& style)
{
    if(name.IsEmpty())
        return *this;
    int i = edge_styles_.Find(name);
    if(i < 0) edge_styles_.Add(name, style); else edge_styles_[i] = style;
    InvalidateSpatialIndex();
    InvalidateGeometry();
    PrepareViewGeometry();
    Refresh();
    return *this;
}

UiNodeGraph& UiNodeGraph::RemoveEdgeStyleClass(const String& name)
{
    int i = edge_styles_.Find(name);
    if(i >= 0) {
        edge_styles_.Remove(i);
        InvalidateSpatialIndex();
        InvalidateGeometry();
        PrepareViewGeometry();
        Refresh();
    }
    return *this;
}

void UiNodeGraph::ClearStyleClasses()
{
    node_styles_.Clear();
    edge_styles_.Clear();
    OnStyleChanged();
}

const UiGraphNodeStyle& UiNodeGraph::FindNodeStyleClass(const String& name) const
{
    int i = name.IsEmpty() ? -1 : node_styles_.Find(name);
    return i >= 0 ? node_styles_[i] : GetEffectiveStyle().node;
}

const UiGraphEdgeStyle& UiNodeGraph::FindEdgeStyleClass(const String& name) const
{
    int i = name.IsEmpty() ? -1 : edge_styles_.Find(name);
    return i >= 0 ? edge_styles_[i] : GetEffectiveStyle().edge;
}

int UiNodeGraph::VisualStateIndex(UiGraphVisualState state)
{
    return minmax((int)state, 0, 3);
}

UiGraphNodeStyle UiNodeGraph::ResolveNodeStyle(const UiGraphNode& node, UiGraphVisualState state) const
{
    UiGraphNodeStyle out = StyleForRole(GetEffectiveStyle().node, node.role);
    int i = node.style_class.IsEmpty() ? -1 : node_styles_.Find(node.style_class);
    if(i >= 0)
        out = node_styles_[i];
    UiNodeGraph* self = const_cast<UiNodeGraph*>(this);
    if(self->WhenResolveNodeStyle)
        self->WhenResolveNodeStyle(node, state, out);
    return out;
}

UiGraphEdgeStyle UiNodeGraph::ResolveEdgeStyle(const UiGraphEdge& edge, UiGraphVisualState state) const
{
    UiGraphEdgeStyle out = FindEdgeStyleClass(edge.style_class);
    UiNodeGraph* self = const_cast<UiNodeGraph*>(this);
    if(self->WhenResolveEdgeStyle)
        self->WhenResolveEdgeStyle(edge, state, out);
    return out;
}

void UiNodeGraph::BindModel(UiGraphModel& model)
{
    for(int i = 0; i < bound_models_.GetCount(); i++)
        if(bound_models_[i] == &model)
            return;
    bound_models_.Add(&model);
    Ptr<UiNodeGraph> self = this;
    UiGraphModel* observed = &model;
    model.WhenGraphChange << [self, observed](const UiGraphChange& change) {
        if(!self || self->model_ != observed)
            return;
        self->HandleModelChange(change);
    };
}

bool UiNodeGraph::ReconcileModelState(bool reset_like)
{
    bool selection_changed = false;

    for(int i = selected_nodes_.GetCount() - 1; i >= 0; --i)
        if(!model_ || !model_->Contains(UiGraphNodeRef{selected_nodes_[i]})) {
            selected_nodes_.Remove(i);
            selection_changed = true;
        }
    for(int i = selected_edges_.GetCount() - 1; i >= 0; --i)
        if(!model_ || !model_->Contains(UiGraphEdgeRef{selected_edges_[i]})) {
            selected_edges_.Remove(i);
            selection_changed = true;
        }

    if(reset_like) {
        for(int i = node_ctrls_.GetCount() - 1; i >= 0; --i) {
            UiGraphNodeRef ref{node_ctrls_.GetKey(i)};
            if(model_ && model_->Contains(ref))
                continue;
            Ptr<Ctrl> ctrl = node_ctrls_[i];
            int a = active_node_ctrls_.Find(ref.id);
            if(a >= 0)
                active_node_ctrls_.Remove(a);
            node_ctrls_.Remove(i);
            if(ctrl && ctrl->GetParent() == this)
                ctrl->Remove();
        }
        hot_node_ = UiGraphNodeRef();
        hot_edge_ = UiGraphEdgeRef();
        hot_port_ = UiGraphPortRef();
    }
    else {
        if(hot_node_.IsValid()) {
            const UiGraphNode* node = model_ ? model_->FindNode(hot_node_) : nullptr;
            if(!node || !node->visible)
                hot_node_ = UiGraphNodeRef();
        }
        if(hot_edge_.IsValid()) {
            const UiGraphEdge* edge = model_ ? model_->FindEdge(hot_edge_) : nullptr;
            if(!edge || !edge->visible)
                hot_edge_ = UiGraphEdgeRef();
        }
        if(hot_port_.IsValid()) {
            const UiGraphNode* node = model_ ? model_->FindNode(hot_port_.node) : nullptr;
            const UiGraphPort* port = model_ ? model_->FindPort(hot_port_) : nullptr;
            if(!node || !node->visible || !port || !port->visible)
                hot_port_ = UiGraphPortRef();
        }
    }

    for(int i = marquee_preview_nodes_.GetCount() - 1; i >= 0; --i)
        if(!model_ || !model_->Contains(UiGraphNodeRef{marquee_preview_nodes_[i]}))
            marquee_preview_nodes_.Remove(i);

    bool cancel_interaction = reset_like &&
                              (interaction_ == InteractionMode::NodeDrag ||
                               interaction_ == InteractionMode::EdgeRouteDrag ||
                               interaction_ == InteractionMode::Connect ||
                               interaction_ == InteractionMode::Marquee);

    if(!cancel_interaction && interaction_ == InteractionMode::NodeDrag) {
        for(int i = 0; i < drag_start_positions_.GetCount(); i++) {
            UiGraphNodeRef ref{drag_start_positions_.GetKey(i)};
            const UiGraphNode* node = model_ ? model_->FindNode(ref) : nullptr;
            Pointf start = drag_start_positions_[i];
            if(!node || !node->visible || !node->movable ||
               abs(node->position.x - start.x) > 1e-9 ||
               abs(node->position.y - start.y) > 1e-9) {
                cancel_interaction = true;
                break;
            }
        }
    }

    if(!cancel_interaction && interaction_ == InteractionMode::EdgeRouteDrag) {
        const UiGraphEdge* edge = model_ ? model_->FindEdge(route_edge_) : nullptr;
        if(!edge || !edge->visible || edge->waypoints.GetCount() != route_before_waypoints_.GetCount())
            cancel_interaction = true;
        else
            for(int i = 0; i < edge->waypoints.GetCount(); i++)
                if(VectorLength(edge->waypoints[i] - route_before_waypoints_[i]) > 1e-9) {
                    cancel_interaction = true;
                    break;
                }
    }

    if(!cancel_interaction && interaction_ == InteractionMode::Connect) {
        const UiGraphPort* source = model_ ? model_->FindPort(connection_source_) : nullptr;
        if(!source || !source->visible || !source->enabled || !source->ProvidesOutput())
            cancel_interaction = true;
        else if(connection_target_.IsValid()) {
            const UiGraphPort* target = model_->FindPort(connection_target_);
            if(!target || !target->visible || !target->enabled || !target->AcceptsInput()) {
                connection_target_ = UiGraphPortRef();
                connection_decision_ = UiGraphConnectionDecision();
            }
            else
                connection_decision_ = model_->ValidateConnection(connection_source_, connection_target_);
        }
    }

    if(cancel_interaction)
        CancelMode();

    return selection_changed;
}

void UiNodeGraph::HandleModelChange(const UiGraphChange& change)
{
    if(batch_update_depth_ > 0) {
        RecordBatchModelChange(change);
        return;
    }

    const bool reset_like = change.kind == UiGraphChangeKind::Reset ||
                            change.kind == UiGraphChangeKind::Cleared;
    bool selection_changed = ReconcileModelState(reset_like);
    Rect damage;
    if(change.node.IsValid())
        damage |= GetNodeDamage(change.node);
    if(change.edge.IsValid())
        damage |= GetEdgeDamage(change.edge);

    EnsureSpatialIndex();
    switch(change.kind) {
    case UiGraphChangeKind::NodeAdded:
    case UiGraphChangeKind::NodeUpdated:
    case UiGraphChangeKind::PortAdded:
    case UiGraphChangeKind::PortUpdated:
    case UiGraphChangeKind::PortRemoved:
        UpdateSpatialNode(change.node);
        if(model_)
            for(UiGraphEdgeRef edge : model_->GetNodeEdges(change.node))
                UpdateSpatialEdge(edge);
        break;
    case UiGraphChangeKind::NodeRemoved: {
        RemoveSpatialNode(change.node);
        int q = GetNodeCtrlIndex(change.node);
        if(q >= 0) {
            Ptr<Ctrl> ctrl = node_ctrls_[q];
            node_ctrls_.Remove(q);
            int a = active_node_ctrls_.Find(change.node.id);
            if(a >= 0)
                active_node_ctrls_.Remove(a);
            if(ctrl && ctrl->GetParent() == this)
                ctrl->Remove();
        }
        break;
    }
    case UiGraphChangeKind::EdgeAdded:
    case UiGraphChangeKind::EdgeUpdated:
        UpdateSpatialEdge(change.edge);
        break;
    case UiGraphChangeKind::EdgeRemoved:
        RemoveSpatialEdge(change.edge);
        break;
    case UiGraphChangeKind::Reset:
    case UiGraphChangeKind::Cleared:
    default:
        InvalidateSpatialIndex();
        EnsureSpatialIndex();
        break;
    }

    model_revision_ = model_ ? model_->GetRevision() : -1;
    const bool structural_like =
           change.kind == UiGraphChangeKind::NodeScopeChanged
        || change.kind == UiGraphChangeKind::SubgraphAdded
        || change.kind == UiGraphChangeKind::SubgraphUpdated
        || change.kind == UiGraphChangeKind::SubgraphRemoved
        || change.kind == UiGraphChangeKind::BackdropAdded
        || change.kind == UiGraphChangeKind::BackdropUpdated
        || change.kind == UiGraphChangeKind::BackdropRemoved;
    const bool full_prepare = reset_like || structural_like
                           || zoom_ < lod_policy_.minimal_edge_zoom;
    if(full_prepare) {
        InvalidateGeometry();
        PrepareViewGeometry();
        UpdateAttachedCtrls();
        Refresh();
    }
    else {
        switch(change.kind) {
        case UiGraphChangeKind::NodeAdded:
        case UiGraphChangeKind::NodeUpdated:
        case UiGraphChangeKind::PortAdded:
        case UiGraphChangeKind::PortUpdated:
        case UiGraphChangeKind::PortRemoved:
        case UiGraphChangeKind::NodeRemoved:
            if(change.node.IsValid())
                damage |= RebuildNodeAndEdges(change.node);
            break;
        case UiGraphChangeKind::EdgeAdded:
        case UiGraphChangeKind::EdgeUpdated:
        case UiGraphChangeKind::EdgeRemoved:
            if(change.edge.IsValid())
                damage |= RebuildEdge(change.edge);
            break;
        default:
            break;
        }
        UpdateAttachedCtrls();
        RefreshDamage(damage.Inflated(DPI(4)));
    }

    RequestInitialFitLayout();
    if(selection_changed)
        WhenSelection();
}


int UiNodeGraph::GetNodeCtrlIndex(UiGraphNodeRef node) const
{
    return node.IsValid() ? node_ctrls_.Find(node.id) : -1;
}

UiNodeGraph& UiNodeGraph::SetNodeCtrl(UiGraphNodeRef node, Ctrl& ctrl)
{
    if(!model_ || !model_->Contains(node))
        return *this;
    Rect damage = GetNodeDamage(node);

    for(int i = node_ctrls_.GetCount() - 1; i >= 0; --i)
        if(node_ctrls_[i] == &ctrl && node_ctrls_.GetKey(i) != node.id) {
            UiGraphId old_id = node_ctrls_.GetKey(i);
            int a = active_node_ctrls_.Find(old_id);
            if(a >= 0)
                active_node_ctrls_.Remove(a);
            node_ctrls_.Remove(i);
            if(ctrl.GetParent() == this) {
                node_ctrl_activation_detach_ = true;
                ctrl.Remove();
                node_ctrl_activation_detach_ = false;
            }
        }

    int i = GetNodeCtrlIndex(node);
    if(i >= 0 && node_ctrls_[i] == &ctrl)
        return *this;
    if(i >= 0) {
        Ptr<Ctrl> old = node_ctrls_[i];
        int a = active_node_ctrls_.Find(node.id);
        if(a >= 0)
            active_node_ctrls_.Remove(a);
        node_ctrls_.Remove(i);
        if(old && old->GetParent() == this)
            old->Remove();
    }

    node_ctrls_.Add(node.id, Ptr<Ctrl>(&ctrl));
    // A registered control is not automatically an active child. Visibility,
    // scope and LOD decide activation in UpdateAttachedCtrls().
    ctrl.Hide();
    damage |= RebuildNodeAndEdges(node);
    UpdateAttachedCtrls();
    RequestInitialFitLayout();
    RefreshDamage(damage);
    return *this;
}

UiNodeGraph& UiNodeGraph::ClearNodeCtrl(UiGraphNodeRef node)
{
    int i = GetNodeCtrlIndex(node);
    if(i < 0)
        return *this;
    Rect damage = GetNodeDamage(node);
    Ptr<Ctrl> ctrl = node_ctrls_[i];
    node_ctrls_.Remove(i);
    int a = active_node_ctrls_.Find(node.id);
    if(a >= 0)
        active_node_ctrls_.Remove(a);
    if(ctrl && ctrl->GetParent() == this)
        ctrl->Remove();
    damage |= RebuildNodeAndEdges(node);
    UpdateAttachedCtrls();
    RequestInitialFitLayout();
    RefreshDamage(damage);
    return *this;
}

void UiNodeGraph::ClearNodeCtrls()
{
    Vector<Ptr<Ctrl>> controls;
    for(int i = node_ctrls_.GetCount() - 1; i >= 0; --i)
        if(node_ctrls_[i]) controls.Add(node_ctrls_[i]);
    node_ctrls_.Clear();
    active_node_ctrls_.Clear();
    last_node_ctrl_candidate_count_ = 0;
    for(Ptr<Ctrl> ctrl : controls)
        if(ctrl && ctrl->GetParent() == this) ctrl->Remove();
    InvalidateGeometry();
    PrepareViewGeometry();
    RequestInitialFitLayout();
    Refresh();
}

Ctrl* UiNodeGraph::GetNodeCtrl(UiGraphNodeRef node) const
{
    int i = GetNodeCtrlIndex(node);
    return i >= 0 && node_ctrls_[i] ? ~node_ctrls_[i] : nullptr;
}

Rect UiNodeGraph::GetNodeCtrlRect(UiGraphNodeRef node) const
{
    const NodeGeometry* geometry = FindNodeGeometry(node);
    return geometry ? geometry->control : RectC(0, 0, 0, 0);
}

void UiNodeGraph::UpdateAttachedCtrls()
{
    Rect viewport(Point(0, 0), GetSize());
    last_node_ctrl_candidate_count_ = 0;

    auto should_show = [&](UiGraphNodeRef ref, Ptr<Ctrl> ctrl) -> bool {
        if(!ctrl)
            return false;
        const UiGraphNode* node = model_ ? model_->FindNode(ref) : nullptr;
        const NodeGeometry* geometry = FindNodeGeometry(ref);
        if(!node || !geometry)
            return false;
        UiGraphNodeStyle style = ResolveNodeStyle(*node, GetNodeVisualState(*node));
        return node->visible && !node->collapsed
            && model_->GetNodeScope(ref) == GetScope()
            && zoom_ >= max(style.content_cell_min_zoom, lod_policy_.secondary_text_zoom)
            && !geometry->control.IsEmpty()
            && !(geometry->control & viewport).IsEmpty();
    };

    // First update only controls that were actually active. This is bounded by
    // the visible embedded-control population, not by every registered binding.
    for(int i = active_node_ctrls_.GetCount() - 1; i >= 0; --i) {
        UiGraphId id = active_node_ctrls_[i];
        int q = node_ctrls_.Find(id);
        last_node_ctrl_candidate_count_++;
        if(q < 0 || !node_ctrls_[q]) {
            active_node_ctrls_.Remove(i);
            continue;
        }
        UiGraphNodeRef ref{id};
        Ptr<Ctrl> ctrl = node_ctrls_[q];
        if(!should_show(ref, ctrl)) {
            ctrl->Hide();
            if(ctrl->GetParent() == this) {
                node_ctrl_activation_detach_ = true;
                ctrl->Remove();
                node_ctrl_activation_detach_ = false;
            }
            active_node_ctrls_.Remove(i);
            continue;
        }
        const NodeGeometry* geometry = FindNodeGeometry(ref);
        ctrl->SetRect(geometry->control);
        ctrl->Show();
    }

    // Then consider only prepared nodes that have a registered control. Prepared
    // geometry is already viewport/LOD bounded, so offscreen registrations do not
    // create per-camera linear work.
    for(int i = 0; i < node_geometry_.GetCount(); i++) {
        UiGraphId id = node_geometry_.GetKey(i);
        if(active_node_ctrls_.Find(id) >= 0)
            continue;
        int q = node_ctrls_.Find(id);
        if(q < 0)
            continue;
        last_node_ctrl_candidate_count_++;
        Ptr<Ctrl> ctrl = node_ctrls_[q];
        if(!ctrl) {
            node_ctrls_.Remove(q);
            continue;
        }
        UiGraphNodeRef ref{id};
        if(!should_show(ref, ctrl))
            continue;
        if(ctrl->GetParent() != this)
            Add(*ctrl);
        ctrl->SetRect(node_geometry_[i].control);
        ctrl->Show();
        active_node_ctrls_.FindAdd(id);
    }
}

void UiNodeGraph::ChildRemoved(Ctrl *child)
{
    if(!node_ctrl_activation_detach_)
        for(int i = node_ctrls_.GetCount() - 1; i >= 0; --i)
            if(node_ctrls_[i] == child) {
                UiGraphId id = node_ctrls_.GetKey(i);
                int a = active_node_ctrls_.Find(id);
                if(a >= 0)
                    active_node_ctrls_.Remove(a);
                node_ctrls_.Remove(i);
            }
    Ctrl::ChildRemoved(child);
}

UiNodeGraph& UiNodeGraph::SetEditable(bool on)
{
    if(editable_ == on)
        return *this;
    editable_ = on;
    if(!on) CancelMode();
    Refresh();
    return *this;
}

UiNodeGraph& UiNodeGraph::EnableInternalMutation(bool on)
{
    internal_mutation_ = on;
    return *this;
}

UiNodeGraph& UiNodeGraph::SetMultiSelection(bool on)
{
    multi_selection_ = on;
    if(!on && selected_nodes_.GetCount() > 1) {
        UiGraphId keep = selected_nodes_[0];
        selected_nodes_.Clear();
        selected_nodes_.Add(keep);
        NotifySelection();
    }
    return *this;
}

UiNodeGraph& UiNodeGraph::SetAutoFitOnFirstPaint(bool on)
{
    auto_fit_first_paint_ = on;
    first_paint_done_ = false;
    RequestInitialFitLayout();
    return *this;
}

void UiNodeGraph::ClampZoom()
{
    const Style& style = GetEffectiveStyle();
    zoom_ = minmax(zoom_, max(0.01, style.min_zoom), max(style.min_zoom, style.max_zoom));
}


Point UiNodeGraph::WorldToScreen(Pointf world) const
{
    return Point(fround(world.x * zoom_ + pan_.x), fround(world.y * zoom_ + pan_.y));
}

Pointf UiNodeGraph::ScreenToWorld(Point screen) const
{
    double z = max(zoom_, 1e-9);
    return Pointf((screen.x - pan_.x) / z, (screen.y - pan_.y) / z);
}

Pointf UiNodeGraph::GetDisplayNodePosition(const UiGraphNode& node) const
{
    int i = drag_preview_positions_.Find(node.ref.id);
    return i >= 0 ? drag_preview_positions_[i] : node.position;
}

Vector<Pointf> UiNodeGraph::GetDisplayEdgeWaypoints(const UiGraphEdge& edge) const
{
    if(interaction_ == InteractionMode::EdgeRouteDrag && route_edge_ == edge.ref)
        return clone(route_preview_waypoints_);
    return clone(edge.waypoints);
}


void UiNodeGraph::ClearSelection()
{
    if(selected_nodes_.IsEmpty() && selected_edges_.IsEmpty()) return;
    Rect damage = GetSelectionDamage();
    selected_nodes_.Clear();
    selected_edges_.Clear();
    InvalidateGeometry();
    PrepareViewGeometry();
    damage |= GetSelectionDamage();
    RefreshDamage(damage);
    WhenSelection();
}


bool UiNodeGraph::IsNodeSelected(UiGraphNodeRef node) const { return node.IsValid() && selected_nodes_.Find(node.id) >= 0; }
bool UiNodeGraph::IsEdgeSelected(UiGraphEdgeRef edge) const { return edge.IsValid() && selected_edges_.Find(edge.id) >= 0; }

Vector<UiGraphNodeRef> UiNodeGraph::GetSelectedNodes() const
{
    Vector<UiGraphNodeRef> out;
    for(int i = 0; i < selected_nodes_.GetCount(); i++)
        if(model_ && model_->Contains(UiGraphNodeRef{selected_nodes_[i]}))
            out.Add(UiGraphNodeRef{selected_nodes_[i]});
    return out;
}

Vector<UiGraphEdgeRef> UiNodeGraph::GetSelectedEdges() const
{
    Vector<UiGraphEdgeRef> out;
    for(int i = 0; i < selected_edges_.GetCount(); i++)
        if(model_ && model_->Contains(UiGraphEdgeRef{selected_edges_[i]}))
            out.Add(UiGraphEdgeRef{selected_edges_[i]});
    return out;
}

void UiNodeGraph::NotifySelection()
{
    InvalidateGeometry();
    PrepareViewGeometry();
    UpdateAttachedCtrls();
    Refresh();
    WhenSelection();
}

Value UiNodeGraph::GetSelectionToken(UiGraphNodeRef ref) const
{
    const UiGraphNode* node = model_ ? model_->FindNode(ref) : nullptr;
    return node ? (IsNull(node->data) ? Value(ref.id) : node->data) : Value();
}

UiGraphNodeRef UiNodeGraph::ResolveSelectionNode(const Value& token) const
{
    if(!model_ || IsNull(token)) return UiGraphNodeRef();
    if(IsNumber(token)) {
        UiGraphNodeRef by_id{(int64)token};
        if(model_->Contains(by_id)) return by_id;
    }
    for(int i = 0; i < model_->GetNodeCount(); i++) {
        const UiGraphNode& node = model_->GetNode(i);
        if(!IsNull(node.data) && node.data == token) return node.ref;
    }
    return UiGraphNodeRef();
}


UiGraphVisualState UiNodeGraph::GetNodeVisualState(const UiGraphNode& node) const
{
    if(!node.enabled || !IsEnabled()) return UiGraphVisualState::Disabled;
    if(IsNodeSelected(node.ref)) return UiGraphVisualState::Selected;
    if(hot_node_ == node.ref) return UiGraphVisualState::Hot;
    return UiGraphVisualState::Normal;
}

UiGraphVisualState UiNodeGraph::GetEdgeVisualState(const UiGraphEdge& edge) const
{
    if(!edge.enabled || !IsEnabled()) return UiGraphVisualState::Disabled;
    if(IsEdgeSelected(edge.ref)) return UiGraphVisualState::Selected;
    if(hot_edge_ == edge.ref) return UiGraphVisualState::Hot;
    return UiGraphVisualState::Normal;
}

UiGraphPortSide UiNodeGraph::ResolvePortSide(const UiGraphPort& port)
{
    if(port.side != UiGraphPortSide::Auto) return port.side;
    if(port.direction == UiGraphPortDirection::Output) return UiGraphPortSide::Right;
    if(port.direction == UiGraphPortDirection::Bidirectional) return UiGraphPortSide::Bottom;
    return UiGraphPortSide::Left;
}

Pointf UiNodeGraph::SideVector(UiGraphPortSide side)
{
    switch(side) {
    case UiGraphPortSide::Left: return Pointf(-1, 0);
    case UiGraphPortSide::Right: return Pointf(1, 0);
    case UiGraphPortSide::Top: return Pointf(0, -1);
    case UiGraphPortSide::Bottom: return Pointf(0, 1);
    default: return Pointf(1, 0);
    }
}


Size UiNodeGraph::GetMinSize() const { return Size(DPI(240), DPI(160)); }

Rect UiNodeGraph::GetGraphScreenBounds(bool selection_only) const
{
    Rect out; bool have=false;
    for(int i=0;i<node_geometry_.GetCount();i++) {
        const NodeGeometry& g=node_geometry_[i]; if(selection_only&&!IsNodeSelected(g.ref)) continue;
        if(!have) { out=g.rect; have=true; } else out|=g.rect;
    }
    return out;
}

void UiNodeGraph::RequestInitialFitLayout()
{
    if(auto_fit_first_paint_ && !first_paint_done_)
        Ctrl::RefreshLayout();
}

} // namespace Upp

