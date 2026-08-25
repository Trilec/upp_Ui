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

double GraphTextScale(double zoom)
{
    zoom = max(0.01, zoom);
    if(zoom <= 1.0)
        return max(0.58, std::pow(zoom, 0.45));
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

Vector<Pointf> RoundedPolygon(const Vector<Pointf>& vertices, double radius, int samples = 5)
{
    Vector<Pointf> out;
    int n = vertices.GetCount();
    if(n < 3)
        return clone(vertices);
    radius = max(0.0, radius);
    samples = max(1, samples);

    for(int i = 0; i < n; i++) {
        Pointf prev = vertices[(i + n - 1) % n];
        Pointf cur = vertices[i];
        Pointf next = vertices[(i + 1) % n];
        double a_len = VectorLength(prev - cur);
        double b_len = VectorLength(next - cur);
        double r = min(radius, min(a_len, b_len) * 0.45);
        Pointf entry = a_len > 1e-9 ? cur + (prev - cur) * (r / a_len) : cur;
        Pointf exit = b_len > 1e-9 ? cur + (next - cur) * (r / b_len) : cur;
        if(out.IsEmpty() || VectorLength(out.Top() - entry) > 0.01)
            out.Add(entry);
        for(int k = 1; k <= samples; k++) {
            double t = (double)k / samples;
            double u = 1.0 - t;
            out.Add(Pointf(u * u * entry.x + 2.0 * u * t * cur.x + t * t * exit.x,
                           u * u * entry.y + 2.0 * u * t * cur.y + t * t * exit.y));
        }
    }
    return out;
}

Vector<Pointf> EllipsePath(const Rect& r, int samples = 40)
{
    Vector<Pointf> out;
    double cx = (r.left + r.right) * 0.5;
    double cy = (r.top + r.bottom) * 0.5;
    double rx = max(1.0, r.GetWidth() * 0.5);
    double ry = max(1.0, r.GetHeight() * 0.5);
    samples = max(12, samples);
    for(int i = 0; i < samples; i++) {
        double a = 2.0 * 3.14159265358979323846 * i / samples;
        out.Add(Pointf(cx + std::cos(a) * rx, cy + std::sin(a) * ry));
    }
    return out;
}

Vector<Pointf> CapsulePath(const Rect& r, int samples = 16)
{
    Vector<Pointf> out;
    double l = r.left, t = r.top, rr = r.right, b = r.bottom;
    double w = max(1, r.GetWidth()), h = max(1, r.GetHeight());
    samples = max(6, samples);
    if(w >= h) {
        double radius = h * 0.5;
        double cy = (t + b) * 0.5;
        double lc = l + radius;
        double rc = rr - radius;
        for(int i = 0; i <= samples; i++) {
            double a = -3.14159265358979323846 * 0.5
                     + 3.14159265358979323846 * i / samples;
            out.Add(Pointf(rc + std::cos(a) * radius,
                           cy + std::sin(a) * radius));
        }
        for(int i = 0; i <= samples; i++) {
            double a = 3.14159265358979323846 * 0.5
                     + 3.14159265358979323846 * i / samples;
            out.Add(Pointf(lc + std::cos(a) * radius,
                           cy + std::sin(a) * radius));
        }
    }
    else {
        double radius = w * 0.5;
        double cx = (l + rr) * 0.5;
        double tc = t + radius;
        double bc = b - radius;
        for(int i = 0; i <= samples; i++) {
            double a = 0.0 + 3.14159265358979323846 * i / samples;
            out.Add(Pointf(cx + std::cos(a) * radius,
                           bc + std::sin(a) * radius));
        }
        for(int i = 0; i <= samples; i++) {
            double a = 3.14159265358979323846
                     + 3.14159265358979323846 * i / samples;
            out.Add(Pointf(cx + std::cos(a) * radius,
                           tc + std::sin(a) * radius));
        }
    }
    return out;
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
        return EllipsePath(rect);
    case UiGraphNodeShape::Diamond:
        v << Pointf(cx, t) << Pointf(r, cy) << Pointf(cx, b) << Pointf(l, cy);
        return RoundedPolygon(v, radius);
    case UiGraphNodeShape::Triangle:
        v << Pointf(cx, t) << Pointf(r, b) << Pointf(l, b);
        return RoundedPolygon(v, radius);
    case UiGraphNodeShape::Hexagon: {
        double dx = rect.GetWidth() * 0.24;
        v << Pointf(l + dx, t) << Pointf(r - dx, t) << Pointf(r, cy)
          << Pointf(r - dx, b) << Pointf(l + dx, b) << Pointf(l, cy);
        return RoundedPolygon(v, radius);
    }
    case UiGraphNodeShape::Capsule:
        return CapsulePath(rect);
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
        return RoundedPolygon(v, max(radius, min(w, h) * 0.10), 6);
    }
    case UiGraphNodeShape::Document: {
        double fold = min(rect.GetWidth(), rect.GetHeight()) * 0.22;
        v << Pointf(l, t) << Pointf(r - fold, t) << Pointf(r, t + fold)
          << Pointf(r, b) << Pointf(l, b);
        return RoundedPolygon(v, radius);
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
        return RoundedPolygon(v, max(radius, rect.GetHeight() * 0.08), 5);
    }
    case UiGraphNodeShape::Square:
    case UiGraphNodeShape::Rectangle:
    case UiGraphNodeShape::RoundedRectangle:
    case UiGraphNodeShape::Custom:
    default:
        v << Pointf(l, t) << Pointf(r, t) << Pointf(r, b) << Pointf(l, b);
        return RoundedPolygon(v,
                              node.shape == UiGraphNodeShape::Rectangle ||
                              node.shape == UiGraphNodeShape::Custom ? 0.0 : radius,
                              6);
    }
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

void StrokePolyline(Painter& p, const Vector<Point>& points, double width, Color color,
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

void PaintArrow(Painter& p, const Vector<Point>& points,
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
        Rect rr(fround(tip.x - size * 0.45), fround(tip.y - size * 0.45),
                fround(tip.x + size * 0.45), fround(tip.y + size * 0.45));
        FillPath(p, EllipsePath(rr, 24), color);
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
    s.grid_minor = dark ? Color(43, 43, 43) : Color(226, 232, 240);
    s.grid_major = dark ? Color(58, 58, 58) : Color(203, 213, 225);
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
    if(polygon.GetCount() < 3)
        return false;
    bool inside = false;
    for(int i = 0, j = polygon.GetCount() - 1; i < polygon.GetCount(); j = i++) {
        const Pointf& a = polygon[j];
        const Pointf& b = polygon[i];
        if(DistanceToSegment(point, a, b) <= 0.75)
            return true;
        bool crosses = ((a.y > point.y) != (b.y > point.y)) &&
                       (point.x < (b.x - a.x) * (point.y - a.y) /
                                      ((b.y - a.y) == 0.0 ? 1e-12 : (b.y - a.y)) + a.x);
        if(crosses)
            inside = !inside;
    }
    return inside;
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
    return lod_policy_.paint_query_margin_low
         + (lod_policy_.paint_query_margin_full - lod_policy_.paint_query_margin_low) * t;
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

    bool reset_like = change.kind == UiGraphChangeKind::Reset ||
                      change.kind == UiGraphChangeKind::Cleared;
    bool selection_changed = ReconcileModelState(reset_like);

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
    case UiGraphChangeKind::NodeRemoved:
        RemoveSpatialNode(change.node);
        break;
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
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
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
        if(node_ctrls_[i] == &ctrl && node_ctrls_.GetKey(i) != node.id)
            node_ctrls_.Remove(i);
    int i = GetNodeCtrlIndex(node);
    if(i >= 0 && node_ctrls_[i] == &ctrl)
        return *this;
    if(i >= 0) {
        Ptr<Ctrl> old = node_ctrls_[i];
        node_ctrls_.Remove(i);
        if(old && old->GetParent() == this)
            old->Remove();
    }
    node_ctrls_.Add(node.id, Ptr<Ctrl>(&ctrl));
    if(ctrl.GetParent() != this)
        Add(ctrl);
    ctrl.Show();
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
    for(int i = node_ctrls_.GetCount() - 1; i >= 0; --i) {
        UiGraphNodeRef ref{node_ctrls_.GetKey(i)};
        Ptr<Ctrl> ctrl = node_ctrls_[i];
        if(!ctrl) {
            node_ctrls_.Remove(i);
            continue;
        }
        const UiGraphNode* node = model_ ? model_->FindNode(ref) : nullptr;
        const NodeGeometry* geometry = FindNodeGeometry(ref);
        if(!node) {
            node_ctrls_.Remove(i);
            if(ctrl->GetParent() == this) ctrl->Remove();
            continue;
        }
        UiGraphNodeStyle style = ResolveNodeStyle(*node, GetNodeVisualState(*node));
        bool show = node->visible && !node->collapsed && geometry &&
                    zoom_ >= max(style.content_cell_min_zoom, lod_policy_.secondary_text_zoom) &&
                    !geometry->control.IsEmpty() && !(geometry->control & viewport).IsEmpty();
        if(show) {
            if(ctrl->GetParent() != this) Add(*ctrl);
            ctrl->SetRect(geometry->control);
            ctrl->Show();
        }
        else
            ctrl->Hide();
    }
}

void UiNodeGraph::ChildRemoved(Ctrl *child)
{
    for(int i = node_ctrls_.GetCount() - 1; i >= 0; --i)
        if(node_ctrls_[i] == child)
            node_ctrls_.Remove(i);
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

    int title_h = out.micro || zoom_ < lod_policy_.title_zoom || node.title.IsEmpty()
                ? 0 : max(1, GetTextSize(node.title, title_font).cy);
    int subtitle_h = zoom_ < lod_policy_.secondary_text_zoom || node.subtitle.IsEmpty()
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
        double inset = max(5.0, style.port_spacing * zoom_ * 0.35);
        double start = vertical ? out.surface.top + inset : out.surface.left + inset;
        double end = vertical ? out.surface.bottom - inset : out.surface.right - inset;
        if(vertical && !out.compact && out.header.bottom < out.surface.bottom - inset)
            start = max(start, (double)out.header.bottom + inset);
        if(end < start) {
            double centre = vertical ? (out.surface.top + out.surface.bottom) * 0.5
                                     : (out.surface.left + out.surface.right) * 0.5;
            start = end = centre;
        }
        double step = count <= 1 ? 0.0 : (end - start) / (count - 1);
        for(int n = 0; n < count; n++) {
            const UiGraphPort& port = node.ports[indexes[n]];
            double axis = count == 1 ? (start + end) * 0.5 : start + step * n;
            Point anchor;
            if(si == 0) anchor = Point(out.surface.left, fround(axis));
            else if(si == 1) anchor = Point(out.surface.right, fround(axis));
            else if(si == 2) anchor = Point(fround(axis), out.surface.top);
            else anchor = Point(fround(axis), out.surface.bottom);
            out.anchors.Add(port.id, anchor);
            if(zoom_ >= lod_policy_.port_zoom) {
                int hit = max(DPI(2), fround(style.port_hit_radius * max(0.35, zoom_)));
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
}

Vector<Pointf> UiNodeGraph::BuildStraightRoute(Pointf source, Pointf target, const Vector<Pointf>& waypoints)
{
    Vector<Pointf> route;
    route.Add(source); route.Append(clone(waypoints)); route.Add(target);
    return SimplifyRoute(route);
}

Vector<Pointf> UiNodeGraph::BuildBezierRoute(Pointf source, UiGraphPortSide source_side,
                                              Pointf target, UiGraphPortSide target_side,
                                              double tension, int samples,
                                              const Vector<Pointf>& waypoints)
{
    Vector<Pointf> route;
    double distance = max(40.0, VectorLength(target - source));
    double handle = max(24.0, distance * minmax(tension, 0.05, 1.25));
    Pointf c1 = source + SideVector(source_side) * handle;
    Pointf c2 = target + SideVector(target_side) * handle;

    // A normal user sees one midpoint handle. For Bezier routes the middle
    // waypoint means "make the cubic pass here at t=.5". Moving both tangent
    // controls by 4/3 of the midpoint delta yields that exact result without
    // exposing two expert-only tangent handles.
    if(!waypoints.IsEmpty()) {
        Pointf desired = waypoints[waypoints.GetCount() / 2];
        Pointf baseline((source.x + target.x + 3.0 * c1.x + 3.0 * c2.x) / 8.0,
                        (source.y + target.y + 3.0 * c1.y + 3.0 * c2.y) / 8.0);
        Pointf bias = (desired - baseline) * (4.0 / 3.0);
        c1 += bias;
        c2 += bias;
    }

    samples = max(4, samples);
    for(int i = 0; i <= samples; i++) {
        double t = (double)i / samples, u = 1.0 - t;
        route.Add(Pointf(u*u*u*source.x + 3*u*u*t*c1.x + 3*u*t*t*c2.x + t*t*t*target.x,
                         u*u*u*source.y + 3*u*u*t*c1.y + 3*u*t*t*c2.y + t*t*t*target.y));
    }
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

Vector<Pointf> UiNodeGraph::RoundPolyline(const Vector<Pointf>& route, double radius, int samples)
{
    if(route.GetCount() < 3 || radius <= 0.0) return clone(route);
    Vector<Pointf> out;
    out.Add(route[0]);
    for(int i = 1; i + 1 < route.GetCount(); i++) {
        Pointf prev = route[i - 1], cur = route[i], next = route[i + 1];
        double la = VectorLength(prev - cur), lb = VectorLength(next - cur);
        double r = min(radius, min(la, lb) * 0.45);
        Pointf entry = la > 1e-9 ? cur + (prev - cur) * (r / la) : cur;
        Pointf exit = lb > 1e-9 ? cur + (next - cur) * (r / lb) : cur;
        out.Add(entry);
        int n = max(1, samples);
        for(int k = 1; k <= n; k++) {
            double t = (double)k / n, u = 1.0 - t;
            out.Add(Pointf(u*u*entry.x + 2*u*t*cur.x + t*t*exit.x,
                           u*u*entry.y + 2*u*t*cur.y + t*t*exit.y));
        }
    }
    out.Add(route.Top());
    return SimplifyRoute(out);
}

Vector<Pointf> UiNodeGraph::BuildOrthogonalRoute(Pointf source, UiGraphPortSide source_side,
                                                  Pointf target, UiGraphPortSide target_side,
                                                  double lead, double corner_radius,
                                                  const Vector<Pointf>& waypoints)
{
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

    lead = max(0.0, lead);
    Pointf a = source + SideVector(source_side) * lead;
    Pointf b = target + SideVector(target_side) * lead;
    bool sh = source_side == UiGraphPortSide::Left || source_side == UiGraphPortSide::Right;
    bool th = target_side == UiGraphPortSide::Left || target_side == UiGraphPortSide::Right;
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
            route = BuildStraightRoute(a, b, waypoints);
        else if(route_style == UiGraphRouteStyle::Orthogonal)
            route = BuildOrthogonalRoute(a, source_side, b, target_side,
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
        else {
            int samples = max(4, fround(5.0 + 19.0 * route_detail));
            route = BuildBezierRoute(a, source_side, b, target_side,
                                     style.bezier_tension, samples, waypoints);
        }
        if(zoom_ < lod_policy_.edge_simplify_zoom)
            out.simplified = true;
    }

    for(const Pointf& q : route)
        out.points.Add(Point(fround(q.x), fround(q.y)));
    int interaction = fround(max(2.0, style.interaction_width * max(0.25, min(1.0, zoom_))));
    out.bounds = RouteBounds(out.points, interaction);
    if(!out.points.IsEmpty()) {
        out.label_point = out.points[out.points.GetCount() / 2];
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
            int hr = max(DPI(5), DPI(6));
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
    if(batch_update_depth_ > 0)
        return;
    if(!geometry_dirty_ && model_ && model_revision_ == model_->GetRevision() &&
       geometry_zoom_ == zoom_ && geometry_pan_ == pan_ && geometry_size_ == GetSize())
        return;
    int64 started = usecs();
    RebuildGeometry();
    last_geometry_prepare_usecs_ = max<int64>(0, usecs() - started);
}

void UiNodeGraph::RebuildGeometry()
{
    node_geometry_.Clear();
    edge_geometry_.Clear();
    last_node_candidate_count_ = 0;
    last_edge_candidate_count_ = 0;
    last_hidden_edge_count_ = 0;
    if(!model_) {
        model_revision_ = -1;
        geometry_zoom_ = zoom_;
        geometry_pan_ = pan_;
        geometry_size_ = GetSize();
        geometry_dirty_ = false;
        geometry_build_serial_++;
        return;
    }

    EnsureSpatialIndex();
    for(int i = selected_nodes_.GetCount() - 1; i >= 0; --i)
        if(!model_->Contains(UiGraphNodeRef{selected_nodes_[i]})) selected_nodes_.Remove(i);
    for(int i = selected_edges_.GetCount() - 1; i >= 0; --i)
        if(!model_->Contains(UiGraphEdgeRef{selected_edges_[i]})) selected_edges_.Remove(i);

    Index<UiGraphId> node_candidates;
    Index<UiGraphId> edge_candidates;
    QuerySpatial(GetViewportWorldBounds(GetPaintQueryMargin()), node_candidates, edge_candidates);

    for(int i = 0; i < drag_preview_positions_.GetCount(); i++) {
        UiGraphNodeRef ref{drag_preview_positions_.GetKey(i)};
        node_candidates.FindAdd(ref.id);
        if(zoom_ >= lod_policy_.edge_hide_zoom)
            for(UiGraphEdgeRef edge : model_->GetNodeEdges(ref))
                edge_candidates.FindAdd(edge.id);
    }
    if(interaction_ == InteractionMode::EdgeRouteDrag && route_edge_.IsValid())
        edge_candidates.FindAdd(route_edge_.id);

    if(zoom_ >= lod_policy_.edge_hide_zoom)
        for(int i = 0; i < edge_candidates.GetCount(); i++) {
            const UiGraphEdge* edge = model_->FindEdge(UiGraphEdgeRef{edge_candidates[i]});
            if(edge) {
                node_candidates.FindAdd(edge->source.node.id);
                node_candidates.FindAdd(edge->target.node.id);
            }
        }

    last_node_candidate_count_ = node_candidates.GetCount();
    last_edge_candidate_count_ = edge_candidates.GetCount();

    Vector<UiGraphNodeRef> order;
    order.Reserve(node_candidates.GetCount());
    for(int i = 0; i < node_candidates.GetCount(); i++) {
        UiGraphNodeRef ref{node_candidates[i]};
        const UiGraphNode* node = model_->FindNode(ref);
        if(node && node->visible)
            order.Add(ref);
    }
    Sort(order, [&](UiGraphNodeRef a, UiGraphNodeRef b) {
        const UiGraphNode* na = model_->FindNode(a);
        const UiGraphNode* nb = model_->FindNode(b);
        if(!na || !nb) return a.id < b.id;
        return na->z_order != nb->z_order ? na->z_order < nb->z_order : a.id < b.id;
    });
    for(UiGraphNodeRef ref : order) {
        const UiGraphNode* node = model_->FindNode(ref);
        if(!node) continue;
        NodeGeometry g;
        BuildNodeGeometry(*node, g);
        node_geometry_.Add(ref.id, pick(g));
    }

    if(zoom_ < lod_policy_.edge_hide_zoom)
        last_hidden_edge_count_ = edge_candidates.GetCount();
    else
        for(int i = 0; i < edge_candidates.GetCount(); i++) {
            UiGraphEdgeRef ref{edge_candidates[i]};
            const UiGraphEdge* edge = model_->FindEdge(ref);
            if(!edge || !edge->visible || !model_->FindPort(edge->source) || !model_->FindPort(edge->target))
                continue;
            EdgeGeometry g;
            BuildEdgeGeometry(*edge, g);
            edge_geometry_.Add(ref.id, pick(g));
        }

    model_revision_ = model_->GetRevision();
    geometry_zoom_ = zoom_;
    geometry_pan_ = pan_;
    geometry_size_ = GetSize();
    geometry_dirty_ = false;
    geometry_build_serial_++;
}

Rect UiNodeGraph::RebuildNodeAndEdges(UiGraphNodeRef ref)
{
    PrepareGeometry();
    Rect damage = GetNodeDamage(ref);
    if(!model_ || geometry_dirty_)
        return damage;

    int i = node_geometry_.Find(ref.id);
    const UiGraphNode* node = model_->FindNode(ref);
    if(i >= 0) {
        if(node && node->visible) {
            NodeGeometry g;
            BuildNodeGeometry(*node, g);
            node_geometry_[i] = pick(g);
        }
        else
            node_geometry_.Remove(i);
    }

    if(model_)
        for(UiGraphEdgeRef edge : model_->GetNodeEdges(ref))
            damage |= RebuildEdge(edge);
    damage |= GetNodeDamage(ref);
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
    if(!edge || !edge->visible || !model_->FindPort(edge->source) || !model_->FindPort(edge->target)) {
        if(i >= 0)
            edge_geometry_.Remove(i);
        return damage;
    }

    EdgeGeometry g;
    BuildEdgeGeometry(*edge, g);
    if(i >= 0)
        edge_geometry_[i] = pick(g);
    else
        edge_geometry_.Add(ref.id, pick(g));
    damage |= GetEdgeDamage(ref);
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

Rect UiNodeGraph::RouteBounds(const Vector<Point>& route, int inflate)
{
    if(route.IsEmpty()) return Rect(0, 0, 0, 0);
    Rect out(route[0], Size(1, 1));
    for(const Point& p : route) out |= Rect(p, Size(1, 1));
    return out.Inflated(max(0, inflate));
}

double UiNodeGraph::DistanceToSegment(Pointf p, Pointf a, Pointf b)
{
    Pointf ab = b - a; double len2 = ab.x * ab.x + ab.y * ab.y;
    if(len2 <= 1e-9) return VectorLength(p - a);
    double t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2;
    t = minmax(t, 0.0, 1.0);
    return VectorLength(p - (a + ab * t));
}

bool UiNodeGraph::PointInNodeGeometry(const UiGraphNode& node, const NodeGeometry& g, Point p) const
{
    if(node.shape == UiGraphNodeShape::Custom && WhenHitTestCustomShape)
        return WhenHitTestCustomShape(node, g.surface, p);
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
    if(!style.show_grid || style.grid_size <= 0) return;
    double spacing = style.grid_size * zoom_;
    if(spacing < 4.0) return;
    int major_every = max(1, style.major_grid_every);
    double ox = std::fmod(pan_.x, spacing), oy = std::fmod(pan_.y, spacing);
    if(ox < 0) ox += spacing; if(oy < 0) oy += spacing;
    int column = fround((outer.left - pan_.x) / spacing);
    for(double x = outer.left + ox; x < outer.right; x += spacing, column++) {
        bool major = ((column % major_every) + major_every) % major_every == 0;
        w.DrawLine(fround(x), outer.top, fround(x), outer.bottom, 1, major ? style.grid_major : style.grid_minor);
    }
    int row = fround((outer.top - pan_.y) / spacing);
    for(double y = outer.top + oy; y < outer.bottom; y += spacing, row++) {
        bool major = ((row % major_every) + major_every) % major_every == 0;
        w.DrawLine(outer.left, fround(y), outer.right, fround(y), 1, major ? style.grid_major : style.grid_minor);
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
        FillPath(p, RoundedPolygon(header, max(0.0, node.corner_radius * zoom_ * 0.55), 5), style.header_face[si]);
    }
    if(zoom_ < lod_policy_.port_zoom)
        return;
    int port_radius = max(1, fround(style.port_radius * min(1.0, max(0.32, zoom_))));
    for(int i = 0; i < g.anchors.GetCount(); i++) {
        String port_id = g.anchors.GetKey(i); const UiGraphPort* port = model_->FindPort(UiGraphPortRef{node.ref, port_id});
        if(!port) continue;
        Point anchor = g.anchors[i]; Color fill = IsNull(port->color) ? UiGraphDefaultTypeColor(port->type) : port->color; Color frame = style.port_frame[si];
        UiGraphPortRef candidate{node.ref, port_id};
        if(connection_source_.IsValid() && candidate == connection_target_)
            frame = connection_decision_.IsAllowed() ? Color(34, 197, 94) : Color(239, 68, 68);
        Rect pr = RectC(anchor.x - port_radius, anchor.y - port_radius, port_radius * 2 + 1, port_radius * 2 + 1);
        Vector<Pointf> shape;
        if(port->direction == UiGraphPortDirection::Input) shape = EllipsePath(pr, 24);
        else if(port->direction == UiGraphPortDirection::Bidirectional) {
            shape << Pointf(anchor.x, pr.top) << Pointf(pr.right, anchor.y) << Pointf(anchor.x, pr.bottom) << Pointf(pr.left, anchor.y);
            shape = RoundedPolygon(shape, port_radius * 0.2, 3);
        }
        else {
            shape << Pointf(pr.left, pr.top) << Pointf(pr.right, pr.top) << Pointf(pr.right, pr.bottom) << Pointf(pr.left, pr.bottom);
            shape = RoundedPolygon(shape, port_radius * 0.25, 3);
        }
        FillPath(p, shape, fill); StrokePath(p, shape, max(0.50, min(1.0, zoom_)), frame, true);
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
    auto paint_one = [&](const String& text, const Rect& r, Font font, Color ink, UiAlign valign, UiAlign halign) {
        if(text.IsEmpty() || r.IsEmpty()) return;
        Vector<String> lines; Vector<Size> sizes; lines.Add(text); sizes.Add(GetTextSize(text, font));
        UiPaintStyledText(w, r, lines, sizes, halign, valign, font, ink, 0, false, 0, 0);
    };
    UiAlign title_align = g.compact ? UiAlign::CENTER : style.text_align_h;
    paint_one(node.title, g.title, title_font, style.title_ink[si], UiAlign::CENTER, title_align);
    paint_one(node.subtitle, g.subtitle, subtitle_font, style.subtitle_ink[si], UiAlign::CENTER, style.text_align_h);
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
    Vector<Pointf> route = BuildBezierRoute(Pointf(a.x, a.y), source_side,
                                             Pointf(b.x, b.y), target_side,
                                             0.35, 20);
    Vector<Point> points; for(const Pointf& q : route) points.Add(Point(fround(q.x), fround(q.y)));
    Color color = connection_target_.IsValid() ? (connection_decision_.IsAllowed() ? Color(34,197,94) : Color(239,68,68)) : Color(59,130,246);
    StrokePolyline(p, points, 2.0, color, DASHED, 7.0, 5.0);
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
    QuerySpatial(paint_world, node_ids, edge_ids);
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
            if(!g.route_handle_hit.IsEmpty()) {
                Color blue = Color(37, 99, 235);
                int radius = max(DPI(4), g.route_handle_hit.GetWidth() / 2 - DPI(1));
                Rect rr = RectC(g.route_handle.x - radius, g.route_handle.y - radius,
                                radius * 2 + 1, radius * 2 + 1);
                FillPath(ep, EllipsePath(rr, 20), Color(219, 234, 254));
                StrokePath(ep, EllipsePath(rr, 20), max(1.0, (double)DPI(1)), blue, true);
            }
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
            if(WhenPaintEdgeOverlay && zoom_ >= lod_policy_.edge_simplify_zoom)
                WhenPaintEdgeOverlay(w,*edge,g.points,state);
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

    for(int q : paint_nodes) {
        const NodeGeometry& g=node_geometry_[q];
        const UiGraphNode* node=model_->FindNode(g.ref); if(!node) continue;
        UiGraphVisualState state=GetNodeVisualState(*node); UiGraphNodeStyle style=ResolveNodeStyle(*node,state);
        if(zoom_ >= lod_policy_.title_zoom)
            PaintNodeText(w,*node,g,style,state);
        if(WhenPaintNodeOverlay && zoom_ >= lod_policy_.secondary_text_zoom)
            WhenPaintNodeOverlay(w,*node,g.surface,state);
        bool handled=false;
        if(WhenPaintNodeForeground)
            WhenPaintNodeForeground(w,*node,g.surface,style,state,handled);
        if(!handled && UsesRectangularStyledSurface(node->shape)) {
            StyledMetrics metrics=ScaleNodeMetrics(style.metrics,zoom_); StyledSkin skin=ScaleNodeSkin(style.skin,zoom_);
            if(node->shape==UiGraphNodeShape::Rectangle) metrics.radius=0;
            else if(node->shape==UiGraphNodeShape::Capsule) metrics.radius=max(0,min(g.surface.GetWidth(),g.surface.GetHeight())/2);
            else metrics.radius=max(0,fround(node->corner_radius*zoom_));
            metrics.shadow.enabled = false;
            UiPaintStyledForeground(w,g.rect,style.palette,metrics,skin,ToStyledState(state),HasFocus()&&!IsNodeSelected(node->ref));
        }
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
        FitToGraph();
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
