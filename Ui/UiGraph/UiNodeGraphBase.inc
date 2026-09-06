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
    case UiGraphNodeShape::Rectangle:
    case UiGraphNodeShape::RoundedRectangle:
    case UiGraphNodeShape::Custom:
    default:
        v << Pointf(l, t) << Pointf(r, t) << Pointf(r, b) << Pointf(l, b);
        return UiGeometry::RoundedPolygon(v,
                              node.shape == UiGraphNodeShape::Rectangle ||
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
        p.Begin();
        p.Circle(tip, max(0.5, size * 0.45));
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

bool UsesRectangularStyledSurface(UiGraphNodeShape shape)
{
    return shape == UiGraphNodeShape::Rectangle ||
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
    if(node.shape == UiGraphNodeShape::Rectangle ||
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
    PrepareGeometry();
    UpdateAttachedCtrls();
    RefreshLayout();
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
    PrepareGeometry();
    UpdateAttachedCtrls();
    RefreshLayout();
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
    PrepareGeometry();
    UpdateAttachedCtrls();
    RefreshLayout();
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
    case UiGraphNodeShape::Rectangle:
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

    PrepareGeometry();
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

    PrepareGeometry();
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
    PrepareGeometry();
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
        PrepareGeometry();
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
        PrepareGeometry();
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

    RefreshLayout();
    if(selection_changed)
        WhenSelection();
}

UiNodeGraph& UiNodeGraph::SetModel(UiGraphModel& model)
{
    if(model_ == &model)
        return *this;
    ASSERT(batch_update_depth_ == 0);
    if(batch_update_depth_ > 0)
        return *this;

    if(interaction_ != InteractionMode::None || interaction_capture_owned_)
        CancelMode();
    if(!node_ctrls_.IsEmpty())
        ClearNodeCtrls();
    hot_node_ = UiGraphNodeRef();
    hot_edge_ = UiGraphEdgeRef();
    hot_port_ = UiGraphPortRef();

    model_ = &model;
    BindModel(model);
    model_revision_ = -1;
    selected_nodes_.Clear();
    selected_edges_.Clear();
    if(auto_fit_first_paint_)
        first_paint_done_ = false;
    InvalidateSpatialIndex();
    EnsureSpatialIndex();
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
    WhenSelection();
    return *this;
}

UiNodeGraph& UiNodeGraph::UseInternalModel()
{
    return SetModel(internal_model_);
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
    RefreshLayout();
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
    RefreshLayout();
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
    PrepareGeometry();
    RefreshLayout();
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
    RefreshLayout();
    return *this;
}

void UiNodeGraph::ClampZoom()
{
    const Style& style = GetEffectiveStyle();
    zoom_ = minmax(zoom_, max(0.01, style.min_zoom), max(style.min_zoom, style.max_zoom));
}

UiNodeGraph& UiNodeGraph::SetZoom(double zoom, Point anchor)
{
    const Style& style = GetEffectiveStyle();
    zoom = minmax(zoom, max(0.01, style.min_zoom), max(style.min_zoom, style.max_zoom));
    if(abs(zoom - zoom_) < 1e-9)
        return *this;
    if(anchor.x < 0 || anchor.y < 0)
        anchor = Point(GetSize().cx / 2, GetSize().cy / 2);
    Pointf world = ScreenToWorld(anchor);
    zoom_ = zoom;
    pan_ = Pointf(anchor.x - world.x * zoom_, anchor.y - world.y * zoom_);
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
    WhenViewport();
    return *this;
}

UiNodeGraph& UiNodeGraph::SetPan(Pointf pan)
{
    if(pan_ == pan)
        return *this;
    pan_ = pan;
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
    WhenViewport();
    return *this;
}

UiNodeGraph& UiNodeGraph::PanBy(Pointf delta) { return SetPan(pan_ + delta); }

UiNodeGraph& UiNodeGraph::ResetView()
{
    zoom_ = 1.0;
    pan_ = Pointf(0, 0);
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
    WhenViewport();
    return *this;
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

UiNodeGraph& UiNodeGraph::FitToGraph(bool selection_only)
{
    if(!model_ || model_->GetNodeCount() == 0)
        return ResetView();
    bool have = false;
    double minx = 0, miny = 0, maxx = 0, maxy = 0;
    for(int i = 0; i < model_->GetNodeCount(); i++) {
        const UiGraphNode& node = model_->GetNode(i);
        if(!node.visible || (selection_only && !IsNodeSelected(node.ref)))
            continue;
        Pointf pos = GetDisplayNodePosition(node);
        if(!have) {
            minx = pos.x; miny = pos.y; maxx = pos.x + node.size.cx; maxy = pos.y + node.size.cy;
            have = true;
        }
        else {
            minx = min(minx, pos.x); miny = min(miny, pos.y);
            maxx = max(maxx, pos.x + node.size.cx); maxy = max(maxy, pos.y + node.size.cy);
        }
    }
    if(!have)
        return *this;
    const Style& style = GetEffectiveStyle();
    Size sz = GetSize();
    double avail_w = max(1, sz.cx - style.fit_margin * 2);
    double avail_h = max(1, sz.cy - style.fit_margin * 2);
    double world_w = max(1.0, maxx - minx);
    double world_h = max(1.0, maxy - miny);
    zoom_ = min(1.0, min(avail_w / world_w, avail_h / world_h));
    ClampZoom();
    pan_ = Pointf(sz.cx * 0.5 - (minx + maxx) * 0.5 * zoom_,
                  sz.cy * 0.5 - (miny + maxy) * 0.5 * zoom_);
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
    WhenViewport();
    return *this;
}

UiNodeGraph& UiNodeGraph::CenterOnNode(UiGraphNodeRef ref)
{
    const UiGraphNode* node = model_ ? model_->FindNode(ref) : nullptr;
    if(!node) return *this;
    Pointf pos = GetDisplayNodePosition(*node);
    Size sz = GetSize();
    pan_ = Pointf(sz.cx * 0.5 - (pos.x + node->size.cx * 0.5) * zoom_,
                  sz.cy * 0.5 - (pos.y + node->size.cy * 0.5) * zoom_);
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
    WhenViewport();
    return *this;
}

void UiNodeGraph::ClearSelection()
{
    if(selected_nodes_.IsEmpty() && selected_edges_.IsEmpty()) return;
    Rect damage = GetSelectionDamage();
    selected_nodes_.Clear();
    selected_edges_.Clear();
    InvalidateGeometry();
    PrepareGeometry();
    damage |= GetSelectionDamage();
    RefreshDamage(damage);
    WhenSelection();
}

UiNodeGraph& UiNodeGraph::SelectNode(UiGraphNodeRef node, bool additive)
{
    if(!model_ || !model_->Contains(node)) return *this;
    Rect damage = GetSelectionDamage() | GetNodeDamage(node);
    if(!additive || !multi_selection_) { selected_nodes_.Clear(); selected_edges_.Clear(); }
    int i = selected_nodes_.Find(node.id);
    if(additive && i >= 0) selected_nodes_.Remove(i); else selected_nodes_.FindAdd(node.id);
    InvalidateGeometry();
    PrepareGeometry();
    damage |= GetSelectionDamage() | GetNodeDamage(node);
    RefreshDamage(damage);
    WhenSelection();
    return *this;
}

UiNodeGraph& UiNodeGraph::SelectEdge(UiGraphEdgeRef edge, bool additive)
{
    if(!model_ || !model_->Contains(edge)) return *this;
    Rect damage = GetSelectionDamage() | GetEdgeDamage(edge);
    if(!additive || !multi_selection_) { selected_nodes_.Clear(); selected_edges_.Clear(); }
    int i = selected_edges_.Find(edge.id);
    if(additive && i >= 0) selected_edges_.Remove(i); else selected_edges_.FindAdd(edge.id);
    InvalidateGeometry();
    PrepareGeometry();
    damage |= GetSelectionDamage() | GetEdgeDamage(edge);
    RefreshDamage(damage);
    WhenSelection();
    return *this;
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
    PrepareGeometry();
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

void UiNodeGraph::SetData(const Value& v)
{
    Rect damage = GetSelectionDamage();
    selected_nodes_.Clear();
    selected_edges_.Clear();
    if(!IsNull(v)) {
        ValueArray values;
        if(v.Is<ValueArray>()) values = v; else values.Add(v);
        for(int i = 0; i < values.GetCount(); i++) {
            UiGraphNodeRef ref = ResolveSelectionNode(values[i]);
            if(ref.IsValid()) selected_nodes_.FindAdd(ref.id);
        }
    }
    InvalidateGeometry();
    PrepareGeometry();
    damage |= GetSelectionDamage();
    RefreshDamage(damage);
    WhenSelection();
}

Value UiNodeGraph::GetData() const
{
    if(selected_nodes_.GetCount() == 1)
        return GetSelectionToken(UiGraphNodeRef{selected_nodes_[0]});
    ValueArray values;
    for(int i = 0; i < selected_nodes_.GetCount(); i++)
        values.Add(GetSelectionToken(UiGraphNodeRef{selected_nodes_[i]}));
    return values;
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

void UiNodeGraph::InvalidateGeometry() { geometry_dirty_ = true; }

void UiNodeGraph::BuildNodeGeometry(const UiGraphNode& node, NodeGeometry& out)
{
    UiGraphNodeStyle style = ResolveNodeStyle(node, GetNodeVisualState(node));
    Font title_font = ScaleGraphFont(style.title_font, zoom_);
    Font subtitle_font = ScaleGraphFont(style.subtitle_font, zoom_);
    Font description_font = ScaleGraphFont(style.description_font, zoom_);
    Font port_font = ScaleGraphFont(style.port_font, zoom_);
    Point p = WorldToScreen(GetDisplayNodePosition(node));
    Size sz(max(1, fround(node.size.cx * zoom_)), max(1, fround(node.size.cy * zoom_)));
    if(node.shape == UiGraphNodeShape::Square || node.shape == UiGraphNodeShape::Circle) {
        int side = max(sz.cx, sz.cy);
        sz = Size(side, side);
    }
    out.ref = node.ref;
    out.rect = RectC(p.x, p.y, sz.cx, sz.cy);
    out.paint_bounds = out.rect;
    out.z_order = node.z_order;
    out.compact = sz.cx < 120 || sz.cy < 72;
    out.micro = sz.cx < 38 || sz.cy < 26;

    StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_);
    StyledSkin skin = ScaleNodeSkin(style.skin, zoom_);
    out.surface = UiStyledSurfaceRect(out.rect, metrics);
    out.content = GetShapeSafeContentRect(node, UiStyledInnerRect(out.rect, metrics, skin), out.surface);
    out.hit_path = NodeShapePath(node, out.surface);

    Rect heading = out.content;
    Ctrl* child = GetNodeCtrl(node.ref);
    bool use_child = child && !node.collapsed && !out.content.IsEmpty()
                  && zoom_ >= max(style.content_cell_min_zoom, lod_policy_.secondary_text_zoom);
    if(use_child) {
        int gap = ScaleMetric(style.content_cell_gap, zoom_);
        Size child_min = child->GetMinSize();
        double child_scale = min(1.0, max(0.45, GraphTextScale(zoom_)));
        child_min.cx = max(1, fround(child_min.cx * child_scale));
        child_min.cy = max(1, fround(child_min.cy * child_scale));
        int reserve = ScaleMetric(style.content_cell_reserve, zoom_);
        if(style.content_cell_side == UiAlign::LEFT || style.content_cell_side == UiAlign::RIGHT) {
            reserve = max(reserve, max(0, child_min.cx));
            reserve = min(reserve, max(0, out.content.GetWidth() - gap));
            if(style.content_cell_side == UiAlign::LEFT) {
                out.control = Rect(out.content.left, out.content.top, out.content.left + reserve, out.content.bottom);
                heading.left = min(heading.right, out.control.right + gap);
            }
            else {
                out.control = Rect(out.content.right - reserve, out.content.top, out.content.right, out.content.bottom);
                heading.right = max(heading.left, out.control.left - gap);
            }
        }
        else {
            reserve = max(reserve, max(0, child_min.cy));
            reserve = min(reserve, max(0, out.content.GetHeight() - gap));
            if(style.content_cell_side == UiAlign::TOP) {
                out.control = Rect(out.content.left, out.content.top, out.content.right, out.content.top + reserve);
                heading.top = min(heading.bottom, out.control.bottom + gap);
            }
            else {
                out.control = Rect(out.content.left, out.content.bottom - reserve, out.content.right, out.content.bottom);
                heading.bottom = max(heading.top, out.control.top - gap);
            }
        }
    }

    int header_h = out.compact
                 ? heading.GetHeight()
                 : min(heading.GetHeight(), max(0, ScaleMetric(style.header_height, zoom_)));
    out.header = Rect(heading.left, heading.top, heading.right, heading.top + header_h);
    Rect text_lane = out.header;
    bool have_icon = zoom_ >= lod_policy_.icon_zoom
                  && !out.micro && style.show_icon && !IsNull(node.icon) && !out.header.IsEmpty();
    if(have_icon) {
        Size authored = node.icon_size.cx > 0 && node.icon_size.cy > 0 ? node.icon_size : style.icon_size;
        Size icon_size = ScaleGraphIcon(authored, zoom_);
        icon_size.cx = min(icon_size.cx, max(0, out.header.GetWidth()));
        icon_size.cy = min(icon_size.cy, max(0, out.header.GetHeight()));
        int gap = max(1, fround(style.icon_text_gap * GraphTextScale(zoom_)));
        if(style.icon_side == UiAlign::RIGHT) {
            out.icon = RectC(out.header.right - icon_size.cx, out.header.top + (out.header.GetHeight() - icon_size.cy) / 2, icon_size.cx, icon_size.cy);
            text_lane.right = max(text_lane.left, out.icon.left - gap);
        }
        else {
            out.icon = RectC(out.header.left, out.header.top + (out.header.GetHeight() - icon_size.cy) / 2, icon_size.cx, icon_size.cy);
            text_lane.left = min(text_lane.right, out.icon.right + gap);
        }
    }

    double title_floor = max(0.16, lod_policy_.title_zoom - 0.16);
    double secondary_floor = max(title_floor, lod_policy_.secondary_text_zoom - 0.12);
    int title_h = zoom_ < title_floor || node.title.IsEmpty()
                ? 0 : max(1, GetTextSize(node.title, title_font).cy);
    int subtitle_h = zoom_ < secondary_floor || node.subtitle.IsEmpty()
                   ? 0 : max(1, GetTextSize(node.subtitle, subtitle_font).cy);
    int title_gap = title_h > 0 && subtitle_h > 0 ? max(1, fround(style.title_subtitle_gap * GraphTextScale(zoom_))) : 0;
    int text_top = text_lane.top + max(0, (text_lane.GetHeight() - title_h - title_gap - subtitle_h) / 2);
    if(title_h > 0) {
        out.title = Rect(text_lane.left, text_top, text_lane.right, min(text_lane.bottom, text_top + title_h));
        text_top = out.title.bottom + title_gap;
    }
    if(subtitle_h > 0)
        out.subtitle = Rect(text_lane.left, text_top, text_lane.right, min(text_lane.bottom, text_top + subtitle_h));
    if(zoom_ >= lod_policy_.full_detail_zoom && !out.compact && style.show_description
       && !node.description.IsEmpty() && heading.bottom > out.header.bottom) {
        int top = min(heading.bottom, out.header.bottom + ScaleMetric(style.subtitle_description_gap, zoom_));
        int min_h = GetTextSize("Ag", description_font).cy;
        if(heading.bottom - top >= min_h)
            out.description = Rect(heading.left, top, heading.right, heading.bottom);
    }

    Vector<int> side_ports[4];
    for(int i = 0; i < node.ports.GetCount(); i++) {
        if(!node.ports[i].visible) continue;
        UiGraphPortSide side = ResolvePortSide(node.ports[i]);
        int si = side == UiGraphPortSide::Left ? 0 : side == UiGraphPortSide::Right ? 1 : side == UiGraphPortSide::Top ? 2 : 3;
        side_ports[si].Add(i);
    }
    for(int si = 0; si < 4; si++)
        Sort(side_ports[si], [&](int a, int b) {
            return node.ports[a].order != node.ports[b].order ? node.ports[a].order < node.ports[b].order : a < b;
        });

    for(int si = 0; si < 4; si++) {
        const Vector<int>& indexes = side_ports[si];
        int count = indexes.GetCount();
        if(count == 0) continue;
        bool vertical = si < 2;
        UiGraphPortSide side = si == 0 ? UiGraphPortSide::Left
                             : si == 1 ? UiGraphPortSide::Right
                             : si == 2 ? UiGraphPortSide::Top
                                       : UiGraphPortSide::Bottom;
        double span = vertical ? out.surface.GetHeight() : out.surface.GetWidth();
        double inset = count <= 1 ? 0.0
                                  : min(span * 0.22, max(4.0, style.port_spacing * zoom_ * 0.30));
        double start = vertical ? out.surface.top + inset : out.surface.left + inset;
        double end = vertical ? out.surface.bottom - inset : out.surface.right - inset;
        if(end < start) {
            double centre = vertical ? (out.surface.top + out.surface.bottom) * 0.5
                                     : (out.surface.left + out.surface.right) * 0.5;
            start = end = centre;
        }
        double step = count <= 1 ? 0.0 : (end - start) / (count - 1);
        for(int n = 0; n < count; n++) {
            const UiGraphPort& port = node.ports[indexes[n]];
            double axis = count == 1 ? (start + end) * 0.5 : start + step * n;
            Point anchor = ShapeBoundaryAnchor(out.hit_path, out.surface, side, axis);
            out.anchors.Add(port.id, anchor);
            if(zoom_ >= lod_policy_.port_zoom) {
                int hit = EffectivePortHitRadius(style);
                Rect hit_rect = RectC(anchor.x - hit, anchor.y - hit, hit * 2 + 1, hit * 2 + 1);
                out.port_hits.Add(port.id, hit_rect);
                out.paint_bounds |= hit_rect;
            }
            if(zoom_ >= lod_policy_.port_label_zoom && style.show_port_labels) {
                String label = PortLabel(port, style.show_port_type);
                Size tsz = GetTextSize(label, port_font);
                int gap = max(1, fround(style.port_label_gap * max(0.45, zoom_)));
                Rect lr;
                if(si == 0) lr = RectC(anchor.x + gap, anchor.y - tsz.cy / 2, tsz.cx, tsz.cy);
                else if(si == 1) lr = RectC(anchor.x - gap - tsz.cx, anchor.y - tsz.cy / 2, tsz.cx, tsz.cy);
                else if(si == 2) lr = RectC(anchor.x - tsz.cx / 2, anchor.y + gap, tsz.cx, tsz.cy);
                else lr = RectC(anchor.x - tsz.cx / 2, anchor.y - gap - tsz.cy, tsz.cx, tsz.cy);
                out.port_labels.Add(port.id, lr);
                out.paint_bounds |= lr;
            }
        }
    }
    out.paint_bounds = out.paint_bounds.Inflated(max(1, fround(2 * min(1.0, max(0.25, zoom_)))));
    if(extension_bounds_.node_paint_margin_px > 0)
        out.paint_bounds |= out.surface.Inflated(extension_bounds_.node_paint_margin_px);
}

Vector<Pointf> UiNodeGraph::BuildStraightRoutePx(Pointf source, Pointf target, const Vector<Pointf>& waypoints)
{
    if(waypoints.GetCount() == 1) {
        double tolerance = max(2.0, min(8.0, VectorLength(target - source) * 0.04));
        if(DistanceToSegment(waypoints[0], source, target) <= tolerance) {
            Vector<Pointf> direct;
            direct << source << target;
            return direct;
        }
    }
    Vector<Pointf> route;
    route.Add(source); route.Append(clone(waypoints)); route.Add(target);
    return SimplifyRoute(route);
}

Vector<Pointf> UiNodeGraph::BuildBezierRoutePx(Pointf source, UiGraphPortSide source_side,
                                                  Pointf target, UiGraphPortSide target_side,
                                                  double tension,
                                                  const Vector<Pointf>& waypoints)
{
    Vector<Pointf> route;
    double distance = max(40.0, VectorLength(target - source));
    double handle = max(24.0, distance * minmax(tension, 0.05, 1.25));

    // Route construction receives projected screen-space endpoints. Preserve the
    // authored cubic controls and let UiGeometry decide the explicit polyline
    // density from the shared 0.35px device-space error contract.
    route.Add(source);

    // The normal editor exposes one midpoint handle. With an authored midpoint,
    // use two C1-continuous cubic halves instead of translating both endpoint
    // controls by an arbitrary bias. This keeps the endpoint port tangents,
    // passes exactly through the dragged point, and prevents large downward or
    // sideways drags from making the old endpoint handles backtrack and flip.
    if(!waypoints.IsEmpty()) {
        Pointf middle = waypoints[waypoints.GetCount() / 2];
        Pointf mid_dir = NormalizeVector(target - source);
        if(VectorLength(mid_dir) <= 1e-9)
            mid_dir = NormalizeVector(SideVector(source_side) - SideVector(target_side));
        if(VectorLength(mid_dir) <= 1e-9)
            mid_dir = Pointf(1, 0);

        double a_len = max(1.0, VectorLength(middle - source));
        double b_len = max(1.0, VectorLength(target - middle));
        double a_handle = min(handle, max(6.0, a_len * 0.45));
        double b_handle = min(handle, max(6.0, b_len * 0.45));
        double mid_handle = min(max(8.0, distance * 0.18),
                                max(8.0, min(a_len, b_len) * 0.40));

        Pointf a1 = source + SideVector(source_side) * a_handle;
        Pointf a2 = middle - mid_dir * mid_handle;
        Pointf b1 = middle + mid_dir * mid_handle;
        Pointf b2 = target + SideVector(target_side) * b_handle;

        UiGeometry::AppendCubic(route, source, a1, a2, middle);
        UiGeometry::AppendCubic(route, middle, b1, b2, target);
        return route;
    }

    Pointf c1 = source + SideVector(source_side) * handle;
    Pointf c2 = target + SideVector(target_side) * handle;
    UiGeometry::AppendCubic(route, source, c1, c2, target);
    return route;
}

Vector<Pointf> UiNodeGraph::SimplifyRoute(const Vector<Pointf>& input)
{
    Vector<Pointf> out;
    for(const Pointf& p : input) {
        if(!out.IsEmpty() && VectorLength(out.Top() - p) < 0.01) continue;
        if(out.GetCount() >= 2) {
            Pointf a = out[out.GetCount() - 2], b = out.Top();
            Pointf ab = b - a, bp = p - b;
            if(abs(ab.x * bp.y - ab.y * bp.x) < 0.01 && (ab.x * bp.x + ab.y * bp.y) >= 0.0) {
                out.Top() = p;
                continue;
            }
        }
        out.Add(p);
    }
    return out;
}

Vector<Pointf> UiNodeGraph::RoundPolyline(const Vector<Pointf>& route, double radius)
{
    return UiGeometry::RoundedPolyline(route, radius);
}

Vector<Pointf> UiNodeGraph::BuildOrthogonalRoutePx(Pointf source, UiGraphPortSide source_side,
                                                      Pointf target, UiGraphPortSide target_side,
                                                      double lead, double corner_radius,
                                                      const Vector<Pointf>& waypoints)
{
    lead = max(0.0, lead);
    Pointf a = source + SideVector(source_side) * lead;
    Pointf b = target + SideVector(target_side) * lead;
    bool sh = source_side == UiGraphPortSide::Left || source_side == UiGraphPortSide::Right;
    bool th = target_side == UiGraphPortSide::Left || target_side == UiGraphPortSide::Right;

    if(waypoints.GetCount() == 1) {
        Pointf middle = waypoints[0];
        Pointf default_middle((a.x + b.x) * 0.5, (a.y + b.y) * 0.5);
        Vector<Pointf> route;
        route << source << a;

        if(sh && th) {
            // A single handle controls a stable corridor instead of being treated
            // as an arbitrary sequence point. Predominantly vertical motion makes
            // a horizontal detour; predominantly horizontal motion moves the
            // vertical bridge. Neither case changes topology while dragging.
            if(abs(middle.y - default_middle.y) >= abs(middle.x - default_middle.x))
                route << Pointf(a.x, middle.y) << Pointf(b.x, middle.y);
            else
                route << Pointf(middle.x, a.y) << Pointf(middle.x, b.y);
        }
        else if(!sh && !th) {
            if(abs(middle.x - default_middle.x) >= abs(middle.y - default_middle.y))
                route << Pointf(middle.x, a.y) << Pointf(middle.x, b.y);
            else
                route << Pointf(a.x, middle.y) << Pointf(b.x, middle.y);
        }
        else if(sh)
            route << Pointf(b.x, a.y);
        else
            route << Pointf(a.x, b.y);

        route << b << target;
        return RoundPolyline(SimplifyRoute(route), corner_radius);
    }

    if(!waypoints.IsEmpty()) {
        Vector<Pointf> route;
        route.Add(source);
        Pointf current = source;
        Vector<Pointf> guides = clone(waypoints); guides.Add(target);
        for(int i = 0; i < guides.GetCount(); i++) {
            Pointf next = guides[i];
            if(abs(current.x - next.x) > 0.01 && abs(current.y - next.y) > 0.01) {
                bool horizontal_first = i == 0
                    ? (source_side == UiGraphPortSide::Left || source_side == UiGraphPortSide::Right)
                    : abs(next.x - current.x) >= abs(next.y - current.y);
                route.Add(horizontal_first ? Pointf(next.x, current.y) : Pointf(current.x, next.y));
            }
            route.Add(next); current = next;
        }
        return RoundPolyline(SimplifyRoute(route), corner_radius);
    }

    Vector<Pointf> route; route << source << a;
    if(sh && th) {
        double mx = (a.x + b.x) * 0.5; route << Pointf(mx, a.y) << Pointf(mx, b.y);
    }
    else if(!sh && !th) {
        double my = (a.y + b.y) * 0.5; route << Pointf(a.x, my) << Pointf(b.x, my);
    }
    else if(sh) route << Pointf(b.x, a.y);
    else route << Pointf(a.x, b.y);
    route << b << target;
    return RoundPolyline(SimplifyRoute(route), corner_radius);
}

void UiNodeGraph::BuildEdgeGeometry(const UiGraphEdge& edge, EdgeGeometry& out)
{
    UiGraphPortSide source_side = UiGraphPortSide::Right, target_side = UiGraphPortSide::Left;
    Point source = GetPortAnchor(edge.source, &source_side);
    Point target = GetPortAnchor(edge.target, &target_side);
    UiGraphEdgeStyle style = ResolveEdgeStyle(edge, GetEdgeVisualState(edge));
    UiGraphRouteStyle route_style = edge.route == UiGraphRouteStyle::Inherit ? style.route : edge.route;
    Vector<Pointf> display_waypoints = GetDisplayEdgeWaypoints(edge);
    Vector<Pointf> waypoints;
    for(const Pointf& p : display_waypoints) {
        Point q = WorldToScreen(p); waypoints.Add(Pointf(q.x, q.y));
    }
    Pointf a(source.x, source.y), b(target.x, target.y);
    Vector<Pointf> route;
    out.ref = edge.ref;
    out.simplified = false;
    out.route_handle = Point(0, 0);
    out.route_handle_hit = RectC(0, 0, 0, 0);

    if(zoom_ < lod_policy_.minimal_edge_zoom) {
        route << a << b;
        out.simplified = true;
    }
    else {
        double route_detail = SmoothUnit(zoom_, lod_policy_.minimal_edge_zoom,
                                         lod_policy_.full_detail_zoom);
        if(route_style == UiGraphRouteStyle::Straight)
            route = BuildStraightRoutePx(a, b, waypoints);
        else if(route_style == UiGraphRouteStyle::Orthogonal)
            route = BuildOrthogonalRoutePx(a, source_side, b, target_side,
                                         style.orthogonal_lead * zoom_,
                                         style.orthogonal_radius * zoom_ * route_detail,
                                         waypoints);
        else if(route_style == UiGraphRouteStyle::Custom && WhenBuildCustomRoute
                && zoom_ >= lod_policy_.edge_simplify_zoom)
            route = WhenBuildCustomRoute(edge, a, source_side, b, target_side, style);
        else if(route_style == UiGraphRouteStyle::Custom) {
            route << a << b;
            out.simplified = true;
        }
        else
            route = BuildBezierRoutePx(a, source_side, b, target_side,
                                     style.bezier_tension, waypoints);
        if(zoom_ < lod_policy_.edge_simplify_zoom)
            out.simplified = true;
    }

    // Keep prepared edge routes in sub-pixel screen coordinates. Quantization
    // happens only at the integer Draw/legacy-overlay boundary.
    out.points = clone(route);
    int interaction = (int)std::ceil(EffectiveEdgeHitWidth(style));
    out.bounds = RouteBounds(out.points, interaction);
    if(extension_bounds_.edge_paint_margin_px > 0)
        out.bounds = out.bounds.Inflated(extension_bounds_.edge_paint_margin_px);
    if(!out.points.IsEmpty()) {
        // Route midpoint is a geometric position, not a vertex-index position.
        // Adaptive tessellation can legitimately collapse a straight Bezier to
        // two endpoints; choosing points[count/2] would then select the target
        // endpoint and move the edit handle as tessellation density changes.
        Pointf midpoint = UiGeometry::PointAtPolylineFraction(route, 0.5);
        out.label_point = Point(fround(midpoint.x), fround(midpoint.y));
        bool editable_route = route_style != UiGraphRouteStyle::Custom &&
                              zoom_ >= lod_policy_.route_edit_zoom &&
                              (IsEdgeSelected(edge.ref) || hot_edge_ == edge.ref ||
                               (interaction_ == InteractionMode::EdgeRouteDrag && route_edge_ == edge.ref));
        if(editable_route) {
            if(!waypoints.IsEmpty())
                out.route_handle = Point(fround(waypoints[waypoints.GetCount() / 2].x),
                                         fround(waypoints[waypoints.GetCount() / 2].y));
            else
                out.route_handle = out.label_point;
            // Keep the visible handle compact, but make the interaction target
            // large enough to remain practical on short connectors.
            int hr = DPI(8);
            out.route_handle_hit = RectC(out.route_handle.x - hr,
                                         out.route_handle.y - hr,
                                         hr * 2 + 1, hr * 2 + 1);
            out.bounds |= out.route_handle_hit.Inflated(DPI(2));
            out.label_point = Point(out.route_handle.x,
                                    out.route_handle.y - DPI(13));
        }
        if(zoom_ >= lod_policy_.edge_label_zoom && !edge.title.IsEmpty()) {
            Font label_font = ScaleGraphFont(style.label_font, zoom_);
            Size ts = GetTextSize(edge.title, label_font);
            out.bounds |= RectC(out.label_point.x - ts.cx / 2 - DPI(5),
                                out.label_point.y - ts.cy / 2 - DPI(3),
                                ts.cx + DPI(10), ts.cy + DPI(6));
        }
    }
}

void UiNodeGraph::PrepareGeometry()
{
    // One exact preparation authority: retained callers keep this compatibility
    // name, but every scene rebuild now flows through projected-size LOD.
    PrepareViewGeometry();
}

void UiNodeGraph::RebuildGeometry()
{
    RebuildViewGeometry();
}

Rect UiNodeGraph::RebuildNodeAndEdges(UiGraphNodeRef ref)
{
    PrepareGeometry();
    Rect damage = GetNodeDamage(ref);
    if(!model_ || geometry_dirty_)
        return damage;

    int i = node_geometry_.Find(ref.id);
    bool old_micro = i >= 0 && node_geometry_[i].micro;
    int old_vertices = i >= 0 ? node_geometry_[i].hit_path.GetCount() : 0;
    const UiGraphNode* node = model_->FindNode(ref);

    bool prepare_node = false;
    if(node && node->visible && model_->GetNodeScope(ref) == GetScope()) {
        WorldRect view = GetViewportWorldBounds(GetPaintQueryMargin());
        prepare_node = GetNodeWorldBounds(*node).Intersects(view);
        if(!prepare_node)
            for(UiGraphEdgeRef edge : model_->GetNodeEdges(ref)) {
                int q = edge_world_bounds_.Find(edge.id);
                if(q >= 0 && edge_world_bounds_[q].Intersects(view)) {
                    prepare_node = true;
                    break;
                }
            }
    }

    if(prepare_node) {
        NodeGeometry fresh;
        BuildViewNodeGeometry(*node, fresh, false);
        if(i >= 0)
            node_geometry_[i] = pick(fresh);
        else
            node_geometry_.Add(ref.id, pick(fresh));

        const NodeGeometry* current = FindNodeGeometry(ref);
        bool new_micro = current && current->micro;
        int new_vertices = current ? current->hit_path.GetCount() : 0;
        last_geometry_lod_node_count_ += (new_micro ? 1 : 0) - (old_micro ? 1 : 0);
        last_geometry_path_vertex_count_ += new_vertices - old_vertices;
    }
    else if(i >= 0) {
        node_geometry_.Remove(i);
        if(old_micro)
            last_geometry_lod_node_count_ = max(0, last_geometry_lod_node_count_ - 1);
        last_geometry_path_vertex_count_ = max(0, last_geometry_path_vertex_count_ - old_vertices);
    }

    if(model_)
        for(UiGraphEdgeRef edge : model_->GetNodeEdges(ref))
            damage |= RebuildEdge(edge);
    damage |= GetNodeDamage(ref);
    prepared_geometry_revision_++;
    return damage;
}

Rect UiNodeGraph::RebuildEdge(UiGraphEdgeRef ref)
{
    PrepareGeometry();
    Rect damage = GetEdgeDamage(ref);
    if(!model_ || geometry_dirty_)
        return damage;

    int i = edge_geometry_.Find(ref.id);
    const UiGraphEdge* edge = model_->FindEdge(ref);
    bool valid = edge && edge->visible
              && model_->GetNodeScope(edge->source.node) == GetScope()
              && model_->FindPort(edge->source) && model_->FindPort(edge->target);

    bool prepare_edge = false;
    if(valid && zoom_ >= lod_policy_.edge_hide_zoom) {
        int q = edge_world_bounds_.Find(ref.id);
        WorldRect view = GetViewportWorldBounds(GetPaintQueryMargin());
        prepare_edge = q >= 0 && edge_world_bounds_[q].Intersects(view);
        prepare_edge |= IsEdgeSelected(ref) || hot_edge_ == ref
                     || (interaction_ == InteractionMode::EdgeRouteDrag && route_edge_ == ref);
    }

    if(!prepare_edge) {
        if(i >= 0) {
            edge_geometry_.Remove(i);
            prepared_geometry_revision_++;
        }
        return damage;
    }

    auto ensure_endpoint = [&](UiGraphNodeRef node_ref) {
        if(node_geometry_.Find(node_ref.id) >= 0)
            return;
        const UiGraphNode* node = model_->FindNode(node_ref);
        if(!node || !node->visible)
            return;
        NodeGeometry fresh;
        BuildViewNodeGeometry(*node, fresh, false);
        last_geometry_lod_node_count_ += fresh.micro ? 1 : 0;
        last_geometry_path_vertex_count_ += fresh.hit_path.GetCount();
        node_geometry_.Add(node_ref.id, pick(fresh));
    };
    ensure_endpoint(edge->source.node);
    ensure_endpoint(edge->target.node);

    EdgeGeometry fresh;
    BuildEdgeGeometry(*edge, fresh);
    if(i >= 0)
        edge_geometry_[i] = pick(fresh);
    else
        edge_geometry_.Add(ref.id, pick(fresh));
    damage |= GetEdgeDamage(ref);
    prepared_geometry_revision_++;
    return damage;
}

Rect UiNodeGraph::GetNodeDamage(UiGraphNodeRef ref) const
{
    Rect damage;
    const NodeGeometry* g = FindNodeGeometry(ref); if(g) damage = g->paint_bounds;
    if(model_) for(UiGraphEdgeRef edge : model_->GetNodeEdges(ref)) damage |= GetEdgeDamage(edge);
    return damage;
}

Rect UiNodeGraph::GetEdgeDamage(UiGraphEdgeRef ref) const
{
    const EdgeGeometry* g = FindEdgeGeometry(ref); return g ? g->bounds : RectC(0, 0, 0, 0);
}

Rect UiNodeGraph::GetSelectionDamage() const
{
    Rect damage;
    for(int i = 0; i < selected_nodes_.GetCount(); i++) damage |= GetNodeDamage(UiGraphNodeRef{selected_nodes_[i]});
    for(int i = 0; i < selected_edges_.GetCount(); i++) damage |= GetEdgeDamage(UiGraphEdgeRef{selected_edges_[i]});
    return damage;
}

void UiNodeGraph::RefreshDamage(Rect damage)
{
    damage &= Rect(Point(0, 0), GetSize());
    if(!damage.IsEmpty()) Refresh(damage);
}

const UiNodeGraph::NodeGeometry* UiNodeGraph::FindNodeGeometry(UiGraphNodeRef ref) const
{
    int i = node_geometry_.Find(ref.id); return i >= 0 ? &node_geometry_[i] : nullptr;
}

const UiNodeGraph::EdgeGeometry* UiNodeGraph::FindEdgeGeometry(UiGraphEdgeRef ref) const
{
    int i = edge_geometry_.Find(ref.id); return i >= 0 ? &edge_geometry_[i] : nullptr;
}

Rect UiNodeGraph::GetEdgeRouteHandleRect(UiGraphEdgeRef ref) const
{
    UiNodeGraph* self = const_cast<UiNodeGraph*>(this);
    self->PrepareGeometry();
    const EdgeGeometry* g = FindEdgeGeometry(ref);
    return g ? g->route_handle_hit : RectC(0, 0, 0, 0);
}

UiGraphEdgeRef UiNodeGraph::HitTestEdgeRouteHandle(Point p) const
{
    UiNodeGraph* self = const_cast<UiNodeGraph*>(this);
    self->PrepareGeometry();
    if(zoom_ < lod_policy_.route_edit_zoom)
        return UiGraphEdgeRef();
    for(int i = selected_edges_.GetCount() - 1; i >= 0; --i) {
        UiGraphEdgeRef ref{selected_edges_[i]};
        const EdgeGeometry* g = FindEdgeGeometry(ref);
        if(g && !g->route_handle_hit.IsEmpty() && g->route_handle_hit.Contains(p))
            return ref;
    }
    if(hot_edge_.IsValid()) {
        const EdgeGeometry* g = FindEdgeGeometry(hot_edge_);
        if(g && !g->route_handle_hit.IsEmpty() && g->route_handle_hit.Contains(p))
            return hot_edge_;
    }
    return UiGraphEdgeRef();
}

Point UiNodeGraph::GetPortAnchor(const UiGraphPortRef& port, UiGraphPortSide* resolved_side) const
{
    const UiGraphNode* node = model_ ? model_->FindNode(port.node) : nullptr;
    const UiGraphPort* pp = model_ ? model_->FindPort(port) : nullptr;
    if(resolved_side) *resolved_side = pp ? ResolvePortSide(*pp) : UiGraphPortSide::Auto;
    const NodeGeometry* g = FindNodeGeometry(port.node);
    if(g) {
        int i = g->anchors.Find(port.port_id);
        return i >= 0 ? g->anchors[i] : g->surface.CenterPoint();
    }
    return node ? WorldToScreen(node->position + Pointf(node->size.cx * 0.5, node->size.cy * 0.5)) : Point(0, 0);
}

Rect UiNodeGraph::RouteBounds(const Vector<Pointf>& route, int inflate)
{
    if(route.IsEmpty())
        return RectC(0, 0, 0, 0);
    double l = route[0].x, t = route[0].y, r = route[0].x, b = route[0].y;
    for(const Pointf& p : route) {
        l = min(l, p.x);
        t = min(t, p.y);
        r = max(r, p.x);
        b = max(b, p.y);
    }
    Rect out((int)std::floor(l), (int)std::floor(t),
             (int)std::ceil(r) + 1, (int)std::ceil(b) + 1);
    return out.Inflated(max(0, inflate));
}

double UiNodeGraph::DistanceToSegment(Pointf p, Pointf a, Pointf b)
{
    return UiGeometry::DistanceToSegment(p, a, b);
}

bool UiNodeGraph::PointInNodeGeometry(const UiGraphNode& node, const NodeGeometry& g, Point p) const
{
    if(node.shape == UiGraphNodeShape::Custom && WhenHitTestCustomShape)
        return WhenHitTestCustomShape(node, g.surface, p);
    if(g.surface.IsEmpty() || !g.surface.Contains(p))
        return false;
    if(node.shape == UiGraphNodeShape::Rectangle ||
       node.shape == UiGraphNodeShape::Square ||
       node.shape == UiGraphNodeShape::Custom)
        return true;
    if(!g.hit_path.IsEmpty())
        return PointInPolygon(g.hit_path, Pointf(p.x, p.y));
    return ShapeContains(node, g.surface, p);
}

UiGraphPortRef UiNodeGraph::HitTestPort(Point p) const
{
    return HitTestPortSpatial(p);
}

UiGraphNodeRef UiNodeGraph::HitTestNode(Point p) const
{
    return HitTestNodeSpatial(p);
}

UiGraphEdgeRef UiNodeGraph::HitTestEdge(Point p) const
{
    return HitTestEdgeSpatial(p);
}

void UiNodeGraph::PaintGrid(Draw& w, const Rect& outer) const
{
    const Style& style = GetEffectiveStyle();
    if(!style.show_grid || style.grid_size <= 0)
        return;

    const int hierarchy = max(2, style.major_grid_every);
    const double base_spacing = style.grid_size * zoom_;
    if(base_spacing <= 0.0)
        return;

    Color paper = ResolveFace(style.canvas_palette.face[ST_NORMAL], SColorPaper());
    // The grid is orientation chrome rather than authored content. Keep the
    // default hierarchy quieter than node frames while preserving custom hue.
    Color minor_base = BlendGraphColor(paper, style.grid_minor, 0.80);
    Color major_base = BlendGraphColor(paper, style.grid_major, 0.90);

    auto draw_level = [&](double step, Color color) {
        if(step < 3.0 || IsNull(color))
            return;
        double ox = std::fmod(pan_.x, step);
        double oy = std::fmod(pan_.y, step);
        if(ox < 0) ox += step;
        if(oy < 0) oy += step;
        for(double x = outer.left + ox; x < outer.right; x += step)
            w.DrawLine(fround(x), outer.top, fround(x), outer.bottom, 1, color);
        for(double y = outer.top + oy; y < outer.bottom; y += step)
            w.DrawLine(outer.left, fround(y), outer.right, fround(y), 1, color);
    };

    // Promote every Nth grid line into the next hierarchy whenever the current
    // screen spacing becomes too dense. The previous level fades rather than
    // popping off, while the next (N*N) level becomes the stable visual anchor.
    // This repeats for hosts that permit zoom levels below the demo minimum.
    int active_level = 0;
    double active_spacing = base_spacing;
    while(active_spacing < 8.0 && active_level < 8) {
        active_spacing *= hierarchy;
        active_level++;
    }

    if(active_level == 0) {
        draw_level(base_spacing, minor_base);
        draw_level(base_spacing * hierarchy, major_base);
    }
    else {
        double finer_spacing = active_spacing / hierarchy;
        double finer_factor = SmoothUnit(finer_spacing, 3.0, 8.0);
        if(finer_factor > 0.02)
            draw_level(finer_spacing, BlendGraphColor(paper, minor_base, finer_factor));

        double active_factor = SmoothUnit(active_spacing, 8.0, 22.0);
        double active_strength = 0.70 + 0.30 * active_factor;
        draw_level(active_spacing,
                   BlendGraphColor(paper, major_base, active_strength));
        draw_level(active_spacing * hierarchy, major_base);
    }

    if(style.show_origin) {
        Point origin = WorldToScreen(Pointf(0, 0));
        w.DrawLine(origin.x, outer.top, origin.x, outer.bottom, 1, Color(248, 113, 113));
        w.DrawLine(outer.left, origin.y, outer.right, origin.y, 1, Color(96, 165, 250));
    }
}

void UiNodeGraph::PaintEdge(Painter& p, const UiGraphEdge& edge, const EdgeGeometry& g,
                            const UiGraphEdgeStyle& style, UiGraphVisualState state)
{
    if(zoom_ < lod_policy_.edge_hide_zoom)
        return;
    int si = VisualStateIndex(state);
    Color color = style.color[si];
    double detail = EdgeDetailFactor();
    double width = style.width[si] * min(1.0, max(0.01, zoom_)) * (0.35 + 0.65 * detail);
    if(zoom_ < lod_policy_.edge_simplify_zoom)
        width = min(width, 1.0);
    if(width <= 0.02)
        return;
    UiLineStyle line_style = style.line_style;
    if(edge.stroke == UiGraphStrokeStyle::Solid) line_style = SOLID;
    else if(edge.stroke == UiGraphStrokeStyle::Dashed) line_style = DASHED;
    else if(edge.stroke == UiGraphStrokeStyle::Dotted) line_style = DOTTED;
    if(g.simplified || zoom_ < lod_policy_.edge_simplify_zoom)
        line_style = SOLID;
    StrokePolyline(p, g.points, width, color, line_style,
                   style.dash_length * zoom_, style.dash_gap * zoom_);

    double arrow_factor = SmoothUnit(zoom_, lod_policy_.arrow_zoom,
                                     lod_policy_.full_detail_zoom);
    if(edge.directed && arrow_factor > 0.01) {
        UiGraphArrowStyle arrow = edge.arrow == UiGraphArrowStyle::Inherit ? style.arrow : edge.arrow;
        double arrow_size = style.arrow_size * min(1.0, max(0.01, zoom_)) * arrow_factor;
        PaintArrow(p, g.points, arrow, arrow_size, color);
    }
}

void UiNodeGraph::PaintNodeSurface(Draw& w, const UiGraphNode& node, const NodeGeometry& g,
                                   const UiGraphNodeStyle& style, UiGraphVisualState state)
{
    bool handled = false;
    if(WhenPaintNodeBackground) WhenPaintNodeBackground(w, node, g.rect, style, state, handled);
    if(handled || !UsesRectangularStyledSurface(node.shape)) return;
    StyledPalette palette = style.palette;
    StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_);
    StyledSkin skin = ScaleNodeSkin(style.skin, zoom_);
    double shadow_detail = ShadowDetailFactor();
    if(g.micro || shadow_detail <= 0.01)
        metrics.shadow.enabled = false;
    else
        metrics.shadow.alpha = clamp(fround(metrics.shadow.alpha * shadow_detail), 0, 255);
    if(node.shape == UiGraphNodeShape::Rectangle) metrics.radius = 0;
    else if(node.shape == UiGraphNodeShape::Capsule) metrics.radius = max(0, min(g.surface.GetWidth(), g.surface.GetHeight()) / 2);
    else metrics.radius = max(0, fround(node.corner_radius * zoom_));
    if(state == UiGraphVisualState::Selected) {
        metrics.frame_enabled = true;
        metrics.frame_width = max(metrics.frame_width,
                                  max(DPI(2), DPI(fround(lod_policy_.selection_outline_width))));
        palette.frame[ST_PRESSED] = SelectedFrameColor(style);
    }
    UiPaintStyledBackground(w, g.rect, palette, metrics, skin, ToStyledState(state), false);
}

void UiNodeGraph::PaintNodeDetails(Painter& p, const UiGraphNode& node, const NodeGeometry& g,
                                   const UiGraphNodeStyle& style, UiGraphVisualState state)
{
    int si = VisualStateIndex(state); Rect rect = g.surface;
    bool rectangular = UsesRectangularStyledSurface(node.shape);
    bool custom_body = node.shape == UiGraphNodeShape::Custom && WhenPaintCustomShape && WhenPaintCustomShape(p, node, rect, style, state);
    StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_);
    if(!rectangular && !custom_body && !g.hit_path.IsEmpty()) {
        const StyledShadow& shadow = metrics.shadow;
        double shadow_detail = ShadowDetailFactor();
        if(!g.micro && shadow_detail > 0.01 && shadow.enabled && !shadow.inset
           && shadow.alpha > 0 && shadow.distance > 0) {
            int layers = shadow.mode == SHADOW_HARD ? 1
                       : shadow_detail >= 0.72 ? 5
                       : shadow_detail >= 0.36 ? 3 : 1;
            int outer_target = 0;
            for(int layer = layers; layer >= 1; --layer) {
                double t = ((double)layer - 0.5) / max(1, layers);
                double spread = shadow.mode == SHADOW_HARD ? 0.0
                              : shadow.distance * (double)layer / layers;
                double curve_alpha = shadow.mode == SHADOW_HARD
                                   ? 1.0
                                   : 1.0 - UiShadowCurveEval(shadow.curve, t);
                int target_alpha = clamp(fround(shadow.alpha * shadow_detail * curve_alpha), 0, 255);
                int layer_alpha = shadow.mode == SHADOW_HARD
                                ? clamp(fround(shadow.alpha * shadow_detail), 0, 255)
                                : max(0, target_alpha - outer_target);
                outer_target = target_alpha;
                if(layer_alpha <= 0)
                    continue;
                Vector<Pointf> shape = InflatePath(g.hit_path, spread);
                Pointf off(shadow.offset_x, shadow.offset_y);
                for(Pointf& q : shape)
                    q += off;
                FillPath(p, shape, PremultipliedRGBA(shadow.color, layer_alpha));
            }
        }
        FillPath(p, g.hit_path, ResolveFace(style.palette.face[si], White()));
        Color frame = state == UiGraphVisualState::Selected ? SelectedFrameColor(style)
                                                            : style.palette.frame[si];
        double frame_width = max(0.50, (double)metrics.frame_width);
        if(state == UiGraphVisualState::Selected)
            frame_width = max(frame_width,
                              (double)max(DPI(2), DPI(fround(lod_policy_.selection_outline_width))));
        StrokeStyledPath(p, g.hit_path, frame_width, frame, metrics);
    }
    if(zoom_ >= lod_policy_.secondary_text_zoom && !g.micro && !custom_body
       && style.show_header_band && !node.collapsed && g.header.GetHeight() > 0) {
        Rect hr = g.header.Deflated(max(1, fround(2 * zoom_)), max(1, fround(2 * zoom_)));
        Vector<Pointf> header; header << Pointf(hr.left, hr.top) << Pointf(hr.right, hr.top) << Pointf(hr.right, hr.bottom) << Pointf(hr.left, hr.bottom);
        FillPath(p, UiGeometry::RoundedPolygon(header, max(0.0, node.corner_radius * zoom_ * 0.55)), style.header_face[si]);
    }

    double port_visual_floor = max(0.12, lod_policy_.port_zoom - 0.04);
    double port_visual_full = max(port_visual_floor + 0.08, lod_policy_.port_zoom + 0.16);
    double port_factor = SmoothUnit(zoom_, port_visual_floor, port_visual_full);
    if(port_factor <= 0.01)
        return;

    int port_radius = max(1, fround(style.port_radius * min(1.0, max(0.20, zoom_))));
    Color node_face = ResolveFace(style.palette.face[si], White());
    for(int i = 0; i < g.anchors.GetCount(); i++) {
        String port_id = g.anchors.GetKey(i); const UiGraphPort* port = model_->FindPort(UiGraphPortRef{node.ref, port_id});
        if(!port) continue;
        Point anchor = g.anchors[i];
        Color raw_fill = IsNull(port->color) ? UiGraphDefaultTypeColor(port->type) : port->color;
        UiGraphPortRef candidate{node.ref, port_id};
        double local_factor = port_factor;
        if(connection_source_.IsValid() && candidate == connection_target_) {
            raw_fill = connection_decision_.IsAllowed() ? Color(34, 197, 94) : Color(239, 68, 68);
            local_factor = max(local_factor, 0.65);
        }
        Color fill = BlendGraphColor(node_face, raw_fill, local_factor);
        Rect pr = RectC(anchor.x - port_radius, anchor.y - port_radius, port_radius * 2 + 1, port_radius * 2 + 1);
        if(port->direction == UiGraphPortDirection::Input) {
            p.Begin();
            p.Ellipse(pr.left + 0.5, pr.top + 0.5,
                      max(1.0, pr.GetWidth() - 1.0),
                      max(1.0, pr.GetHeight() - 1.0));
            p.Fill(fill);
            p.End();
        }
        else {
            Vector<Pointf> shape;
            if(port->direction == UiGraphPortDirection::Bidirectional)
                shape << Pointf(anchor.x, pr.top) << Pointf(pr.right, anchor.y)
                      << Pointf(anchor.x, pr.bottom) << Pointf(pr.left, anchor.y);
            else
                shape << Pointf(pr.left, pr.top) << Pointf(pr.right, pr.top)
                      << Pointf(pr.right, pr.bottom) << Pointf(pr.left, pr.bottom);
            FillPath(p, shape, fill);
        }
    }
}

void UiNodeGraph::PaintNodeText(Draw& w, const UiGraphNode& node, const NodeGeometry& g,
                                const UiGraphNodeStyle& style, UiGraphVisualState state)
{
    int si = VisualStateIndex(state);
    Font title_font = ScaleGraphFont(style.title_font, zoom_);
    Font subtitle_font = ScaleGraphFont(style.subtitle_font, zoom_);
    Font description_font = ScaleGraphFont(style.description_font, zoom_);
    Font port_font = ScaleGraphFont(style.port_font, zoom_);
    if(!g.icon.IsEmpty() && !IsNull(node.icon)) {
        UiIconRenderMode mode = node.icon_render_mode == UiIconRenderMode::Auto ? style.icon_render_mode : node.icon_render_mode;
        UiPaintStyledIcon(w, g.icon, node.icon, true, true, mode, style.palette.icon[si], node.enabled && IsEnabled());
    }

    Color face = ResolveFace(style.palette.face[si], White());
    auto paint_one = [&](const String& text, const Rect& r, Font font, Color ink,
                         UiAlign valign, UiAlign halign, double visibility) {
        if(text.IsEmpty() || r.IsEmpty() || visibility <= 0.01)
            return;
        ink = BlendGraphColor(face, ink, visibility);
        Size ts = GetTextSize(text, font);
        if(ts.cx <= r.GetWidth()) {
            Vector<String> lines; Vector<Size> sizes; lines.Add(text); sizes.Add(ts);
            UiPaintStyledText(w, r, lines, sizes, halign, valign, font, ink, 0, false, 0, 0);
        }
        else {
            int y = r.top + max(0, (r.GetHeight() - font.GetHeight()) / 2);
            DrawTextEllipsis(w, r.left, y, max(0, r.GetWidth()), text, "...", font, ink);
        }
    };

    double title_floor = max(0.16, lod_policy_.title_zoom - 0.16);
    double secondary_floor = max(title_floor, lod_policy_.secondary_text_zoom - 0.12);
    double title_visibility = SmoothUnit(zoom_, title_floor, lod_policy_.title_zoom + 0.08);
    double secondary_visibility = SmoothUnit(zoom_, secondary_floor,
                                             lod_policy_.secondary_text_zoom + 0.08);
    UiAlign title_align = g.compact ? UiAlign::CENTER : style.text_align_h;
    paint_one(node.title, g.title, title_font, style.title_ink[si],
              UiAlign::CENTER, title_align, title_visibility);
    paint_one(node.subtitle, g.subtitle, subtitle_font, style.subtitle_ink[si],
              UiAlign::CENTER, style.text_align_h, secondary_visibility);
    if(!g.compact && style.show_description && !node.description.IsEmpty() && !g.description.IsEmpty()) {
        Vector<String> lines; Vector<Size> sizes; UiBuildStyledTextLines(node.description, description_font, lines, sizes);
        UiPaintStyledText(w, g.description, lines, sizes, style.text_align_h, UiAlign::TOP, description_font, style.description_ink[si], 0, false, 0, 0);
    }
    if(style.show_port_labels && !node.collapsed && zoom_ >= lod_policy_.port_label_zoom)
        for(int i = g.port_labels.GetCount() - 1; i >= 0; --i) {
            const UiGraphPort* port = model_->FindPort(UiGraphPortRef{node.ref, g.port_labels.GetKey(i)});
            if(!port) continue;
            String label = PortLabel(*port, style.show_port_type); Vector<String> lines; Vector<Size> sizes;
            lines.Add(label); sizes.Add(GetTextSize(label, port_font));
            UiPaintStyledText(w, g.port_labels[i], lines, sizes, UiAlign::LEFT, UiAlign::CENTER,
                              port_font, style.port_label_ink[si], 0, false, 0, 0);
        }
}

void UiNodeGraph::PaintConnectionPreview(Painter& p)
{
    if(interaction_ != InteractionMode::Connect || !connection_source_.IsValid()
       || zoom_ < lod_policy_.edge_simplify_zoom)
        return;
    UiGraphPortSide source_side = UiGraphPortSide::Right, target_side = UiGraphPortSide::Left;
    Point a = GetPortAnchor(connection_source_, &source_side); Point b = last_point_;
    if(connection_target_.IsValid()) b = GetPortAnchor(connection_target_, &target_side);
    Vector<Pointf> route = BuildBezierRoutePx(Pointf(a.x, a.y), source_side,
                                             Pointf(b.x, b.y), target_side,
                                             0.35);
    Color color = connection_target_.IsValid() ? (connection_decision_.IsAllowed() ? Color(34,197,94) : Color(239,68,68)) : Color(59,130,246);
    StrokePolyline(p, route, 2.0, color, DASHED, 7.0, 5.0);
}

void UiNodeGraph::PaintMarquee(Draw& w) const
{
    if(interaction_ != InteractionMode::Marquee || marquee_.IsEmpty()) return;
    const Style& style = GetEffectiveStyle();
    RGBA fill = PremultipliedRGBA(style.selection_box_fill,
                                  minmax(style.selection_box_alpha, 0, 255));

    ImageBuffer ib(Size(1, 1));
    BufferPainter bp(ib, MODE_ANTIALIASED);
    bp.Clear(fill);
    bp.Finish();
    Image tile = ib;
    w.DrawImage(marquee_, tile);

    const int border = DPI(1);
    w.DrawRect(marquee_.left, marquee_.top, marquee_.GetWidth(), border, style.selection_box_frame);
    w.DrawRect(marquee_.left, marquee_.bottom - border, marquee_.GetWidth(), border, style.selection_box_frame);
    w.DrawRect(marquee_.left, marquee_.top, border, marquee_.GetHeight(), style.selection_box_frame);
    w.DrawRect(marquee_.right - border, marquee_.top, border, marquee_.GetHeight(), style.selection_box_frame);
}

void UiNodeGraph::PaintGraphGeometry(Draw& w)
{
    Size size = GetSize(); if(size.cx <= 0 || size.cy <= 0) return;
    Rect viewport(Point(0,0), size);
    Rect paint = w.GetPaintRect() & viewport;
    if(paint.IsEmpty()) return;

    last_paint_node_visit_count_ = 0;
    last_paint_edge_visit_count_ = 0;
    last_painted_node_count_ = 0;
    last_painted_edge_count_ = 0;
    last_simplified_edge_count_ = 0;
    last_edge_paint_usecs_ = 0;
    last_node_paint_usecs_ = 0;

    WorldRect paint_world;
    paint_world.Include(ScreenToWorld(paint.TopLeft()));
    paint_world.Include(ScreenToWorld(paint.BottomRight()));
    paint_world = paint_world.Inflated(GetPaintQueryMargin() / max(zoom_, 0.01));
    Index<UiGraphId> node_ids;
    Index<UiGraphId> edge_ids;
    QuerySpatial(paint_world, node_ids, edge_ids,
                 SPATIAL_QUERY_NODES | SPATIAL_QUERY_EDGES | SPATIAL_QUERY_REDUCE_OVERVIEW);
    for(int i = 0; i < drag_preview_positions_.GetCount(); i++) {
        UiGraphNodeRef ref{drag_preview_positions_.GetKey(i)};
        node_ids.FindAdd(ref.id);
        if(model_ && zoom_ >= lod_policy_.edge_hide_zoom)
            for(UiGraphEdgeRef edge : model_->GetNodeEdges(ref))
                edge_ids.FindAdd(edge.id);
    }
    if(interaction_ == InteractionMode::EdgeRouteDrag && route_edge_.IsValid())
        edge_ids.FindAdd(route_edge_.id);

    Vector<int> paint_nodes;
    paint_nodes.Reserve(node_ids.GetCount());
    for(int i = 0; i < node_ids.GetCount(); i++) {
        int q = node_geometry_.Find(node_ids[i]);
        if(q >= 0 && !(node_geometry_[q].paint_bounds & paint).IsEmpty())
            paint_nodes.Add(q);
    }
    Sort(paint_nodes, [&](int a, int b) {
        const NodeGeometry& ga = node_geometry_[a];
        const NodeGeometry& gb = node_geometry_[b];
        return ga.z_order != gb.z_order ? ga.z_order < gb.z_order : ga.ref.id < gb.ref.id;
    });

    Vector<int> paint_edges;
    paint_edges.Reserve(edge_ids.GetCount());
    if(zoom_ >= lod_policy_.edge_hide_zoom)
        for(int i = 0; i < edge_ids.GetCount(); i++) {
            int q = edge_geometry_.Find(edge_ids[i]);
            if(q >= 0 && !(edge_geometry_[q].bounds & paint).IsEmpty())
                paint_edges.Add(q);
        }

    last_paint_node_visit_count_ = paint_nodes.GetCount();
    last_paint_edge_visit_count_ = paint_edges.GetCount();

    int64 edge_started = usecs();
    if(!paint_edges.IsEmpty() || interaction_ == InteractionMode::Connect) {
        ImageBuffer edge_buffer(paint.GetSize());
        BufferPainter ep(edge_buffer, MODE_ANTIALIASED);
        ep.Clear(RGBAZero());
        ep.Translate(-paint.left, -paint.top);
        for(int q : paint_edges) {
            const EdgeGeometry& g = edge_geometry_[q];
            const UiGraphEdge* edge = model_->FindEdge(g.ref); if(!edge) continue;
            last_painted_edge_count_++;
            if(g.simplified)
                last_simplified_edge_count_++;
            UiGraphVisualState state = GetEdgeVisualState(*edge);
            PaintEdge(ep, *edge, g, ResolveEdgeStyle(*edge, state), state);
        }
        PaintConnectionPreview(ep);
        ep.Finish();
        w.DrawImage(paint.left, paint.top, edge_buffer);
    }

    if(zoom_ >= lod_policy_.edge_label_zoom)
        for(int q : paint_edges) {
            const EdgeGeometry& g = edge_geometry_[q];
            const UiGraphEdge* edge = model_->FindEdge(g.ref); if(!edge) continue;
            UiGraphVisualState state = GetEdgeVisualState(*edge); UiGraphEdgeStyle style = ResolveEdgeStyle(*edge,state); int si = VisualStateIndex(state);
            if(!edge->title.IsEmpty()) {
                Font label_font = ScaleGraphFont(style.label_font, zoom_);
                Size ts=GetTextSize(edge->title,label_font); Rect lr=RectC(g.label_point.x-ts.cx/2-DPI(4),g.label_point.y-ts.cy/2-DPI(2),ts.cx+DPI(8),ts.cy+DPI(4));
                if(style.draw_label_background) w.DrawRect(lr,style.label_background);
                w.DrawText(lr.left+DPI(4),lr.top+DPI(2),edge->title,label_font,style.label_ink[si]);
            }
            if(WhenPaintEdgeOverlay && zoom_ >= lod_policy_.edge_simplify_zoom) {
                Vector<Point> overlay_points;
                overlay_points.Reserve(g.points.GetCount());
                for(const Pointf& q : g.points)
                    overlay_points.Add(Point(fround(q.x), fround(q.y)));
                WhenPaintEdgeOverlay(w, *edge, overlay_points, state);
            }
        }
    last_edge_paint_usecs_ = max<int64>(0, usecs() - edge_started);

    int64 node_started = usecs();
    for(int q : paint_nodes) {
        const NodeGeometry& g = node_geometry_[q];
        const UiGraphNode* node = model_->FindNode(g.ref); if(!node) continue;
        last_painted_node_count_++;
        UiGraphVisualState state = GetNodeVisualState(*node); PaintNodeSurface(w,*node,g,ResolveNodeStyle(*node,state),state);
    }

    ImageBuffer node_buffer(paint.GetSize());
    BufferPainter np(node_buffer, MODE_ANTIALIASED);
    np.Clear(RGBAZero());
    np.Translate(-paint.left, -paint.top);
    for(int q : paint_nodes) {
        const NodeGeometry& g = node_geometry_[q];
        const UiGraphNode* node = model_->FindNode(g.ref); if(!node) continue;
        UiGraphVisualState state = GetNodeVisualState(*node); PaintNodeDetails(np,*node,g,ResolveNodeStyle(*node,state),state);
    }
    np.Finish();
    w.DrawImage(paint.left,paint.top,node_buffer);

    double content_floor = max(0.24, lod_policy_.secondary_text_zoom - 0.08);
    for(int q : paint_nodes) {
        const NodeGeometry& g=node_geometry_[q];
        const UiGraphNode* node=model_->FindNode(g.ref); if(!node) continue;
        UiGraphVisualState state=GetNodeVisualState(*node); UiGraphNodeStyle style=ResolveNodeStyle(*node,state);
        if(WhenPaintNodeContent && zoom_ >= content_floor && !g.content.IsEmpty())
            WhenPaintNodeContent(w,*node,g.content,style,state);
        if(!g.title.IsEmpty() || !g.subtitle.IsEmpty() || !g.icon.IsEmpty()
           || !g.description.IsEmpty() || !g.port_labels.IsEmpty())
            PaintNodeText(w,*node,g,style,state);
        if(WhenPaintNodeOverlay && zoom_ >= lod_policy_.secondary_text_zoom)
            WhenPaintNodeOverlay(w,*node,g.surface,state);
        bool handled=false;
        if(WhenPaintNodeForeground)
            WhenPaintNodeForeground(w,*node,g.surface,style,state,handled);
        // Graph control focus belongs to the canvas, not every individual node.
        // The old default foreground call painted a second focus frame on every
        // unselected rectangular/capsule node and was both visually wrong and a
        // large avoidable per-node paint cost. Hosts still retain the explicit
        // foreground hook above when they need custom interaction chrome.
    }

    // Route-edit handles are interaction chrome. Draw them after node bodies and
    // text so short connectors cannot have their only grab handle hidden by a
    // neighbouring node or port marker.
    bool have_route_handles = false;
    for(int q : paint_edges)
        if(!edge_geometry_[q].route_handle_hit.IsEmpty()) {
            have_route_handles = true;
            break;
        }
    if(have_route_handles) {
        ImageBuffer handle_buffer(paint.GetSize());
        BufferPainter hp(handle_buffer, MODE_ANTIALIASED);
        hp.Clear(RGBAZero());
        hp.Translate(-paint.left, -paint.top);
        const Color blue = Color(37, 99, 235);
        for(int q : paint_edges) {
            const EdgeGeometry& g = edge_geometry_[q];
            if(g.route_handle_hit.IsEmpty())
                continue;
            int radius = DPI(4);
            Rect rr = RectC(g.route_handle.x - radius, g.route_handle.y - radius,
                            radius * 2 + 1, radius * 2 + 1);
            hp.Begin();
            hp.Ellipse(rr.left + 0.5, rr.top + 0.5,
                       max(1.0, rr.GetWidth() - 1.0),
                       max(1.0, rr.GetHeight() - 1.0));
            hp.Fill(Color(219, 234, 254));
            hp.Stroke(max(1.0, (double)DPI(1)), blue);
            hp.End();
        }
        hp.Finish();
        w.DrawImage(paint.left, paint.top, handle_buffer);
    }

    bool have_preview = false;
    for(int q : paint_nodes)
        if(marquee_preview_nodes_.Find(node_geometry_[q].ref.id) >= 0
           && !IsNodeSelected(node_geometry_[q].ref)) { have_preview = true; break; }
    if(have_preview) {
        ImageBuffer preview_buffer(paint.GetSize());
        BufferPainter pp(preview_buffer, MODE_ANTIALIASED);
        pp.Clear(RGBAZero());
        pp.Translate(-paint.left, -paint.top);
        const Color selection = GetEffectiveStyle().selection_box_frame;
        RGBA wash = PremultipliedRGBA(selection, 18);
        for(int q : paint_nodes) {
            const NodeGeometry& g = node_geometry_[q];
            if(marquee_preview_nodes_.Find(g.ref.id) >= 0 && !IsNodeSelected(g.ref) && !g.hit_path.IsEmpty()) {
                FillPath(pp, g.hit_path, wash);
                StrokePath(pp, g.hit_path, 1.0, selection, true);
            }
        }
        pp.Finish();
        w.DrawImage(paint.left, paint.top, preview_buffer);
    }

    // Built-in shapes now have one authoritative frame. Selection changes that
    // frame's colour/thickness at body paint time rather than drawing a second,
    // slightly different silhouette over it. Custom bodies retain a fallback
    // selection outline because Graph cannot assume how their callback paints.
    bool have_custom_selected = false;
    for(int q : paint_nodes) {
        const UiGraphNode* node = model_->FindNode(node_geometry_[q].ref);
        if(node && node->shape == UiGraphNodeShape::Custom && IsNodeSelected(node->ref)) {
            have_custom_selected = true;
            break;
        }
    }
    if(have_custom_selected) {
        ImageBuffer selection_buffer(paint.GetSize());
        BufferPainter sp(selection_buffer, MODE_ANTIALIASED);
        sp.Clear(RGBAZero());
        sp.Translate(-paint.left, -paint.top);
        double stroke_width = max(2.0, (double)DPI(fround(lod_policy_.selection_outline_width)));
        for(int q : paint_nodes) {
            const NodeGeometry& g = node_geometry_[q];
            const UiGraphNode* node = model_->FindNode(g.ref);
            if(!node || node->shape != UiGraphNodeShape::Custom || !IsNodeSelected(g.ref) || g.hit_path.IsEmpty())
                continue;
            UiGraphNodeStyle style = ResolveNodeStyle(*node, UiGraphVisualState::Selected);
            StrokePath(sp, g.hit_path, stroke_width, SelectedFrameColor(style), true);
        }
        sp.Finish();
        w.DrawImage(paint.left, paint.top, selection_buffer);
    }
    last_node_paint_usecs_ = max<int64>(0, usecs() - node_started);
}

void UiNodeGraph::Paint(Draw& w)
{
    int64 started = usecs();
    Rect outer(Point(0,0),GetSize()); const Style& style=GetEffectiveStyle();
    UiPaintStyledBackground(w,outer,style.canvas_palette,style.canvas_metrics,style.canvas_skin,ST_NORMAL,HasFocus());
    PaintGrid(w,outer);
    if(model_) PaintGraphGeometry(w);
    PaintMarquee(w);
    UiPaintStyledForeground(w,outer,style.canvas_palette,style.canvas_metrics,style.canvas_skin,ST_NORMAL,HasFocus());
    last_paint_usecs_ = max<int64>(0, usecs() - started);
}

void UiNodeGraph::Layout()
{
    SyncThemeStyle();
    if(auto_fit_first_paint_ && !first_paint_done_ && model_ && model_->GetNodeCount() > 0) {
        first_paint_done_ = true;
        FitToGraph(false);
        return;
    }
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
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

} // namespace Upp
