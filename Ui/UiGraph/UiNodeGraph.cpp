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

Pointf NormalizeVector(Pointf v)
{
    double d = std::sqrt(v.x * v.x + v.y * v.y);
    return d > 1e-9 ? Pointf(v.x / d, v.y / d) : Pointf(0, 0);
}

double VectorLength(Pointf v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
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
        v << Pointf(l, t) << Pointf(r, t) << Pointf(r, b) << Pointf(l, b);
        return RoundedPolygon(v, min(rect.GetWidth(), rect.GetHeight()) * 0.5, 8);
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

void StrokeSegment(Painter& p, Pointf a, Pointf b, double width, Color color)
{
    p.Begin();
    p.Move(a);
    p.Line(b);
    p.Stroke(width, color);
    p.End();
}

void StrokePolyline(Painter& p, const Vector<Point>& points, double width, Color color,
                    UiLineStyle line_style, double dash_length, double dash_gap)
{
    if(points.GetCount() < 2)
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

    double on = line_style == DOTTED ? max(width, 1.5) : max(1.0, dash_length);
    double off = max(1.0, dash_gap);
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
       points.GetCount() < 2 || size <= 0.0)
        return;
    Pointf tip(points.Top().x, points.Top().y);
    Pointf prev(points[points.GetCount() - 2].x, points[points.GetCount() - 2].y);
    Pointf dir = NormalizeVector(tip - prev);
    if(VectorLength(dir) <= 1e-9)
        return;
    Pointf side(-dir.y, dir.x);
    Pointf base = tip - dir * size;

    if(arrow == UiGraphArrowStyle::Open) {
        StrokeSegment(p, tip, base + side * size * 0.55, 1.5, color);
        StrokeSegment(p, tip, base - side * size * 0.55, 1.5, color);
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
        s.edge.color[i] = i == ST_HOT ? standard.accent_hot
                        : i == ST_PRESSED ? standard.accent_pressed
                        : i == ST_DISABLED ? standard.frame_disabled
                        : standard.frame_pressed;
        s.edge.label_ink[i] = i == ST_DISABLED ? standard.ink_disabled : standard.ink;
    }

    bool dark = UiThemeDetail::ResolveEffectiveMode(ctx.mode) == UiThemeMode::Dark;
    s.grid_minor = dark ? Color(43, 43, 43) : Color(226, 232, 240);
    s.grid_major = dark ? Color(58, 58, 58) : Color(203, 213, 225);
    s.edge.label_background = standard.face;
    s.selection_box_fill = standard.accent;
    s.selection_box_frame = standard.accent_pressed;

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
        s.node.metrics.shadow.offset_y = DPI(2);
        s.node.metrics.shadow.alpha = 42;
        s.node.metrics.shadow.color = Color(15, 23, 42);
        s.node.metrics.shadow.mode = SHADOW_CURVE;
        s.node.metrics.shadow.curve = ShadowSoft();
        s.node.skin = StyledSkin();
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

UiNodeGraph& UiNodeGraph::SetNodeStyleClass(const String& name, const UiGraphNodeStyle& style)
{
    if(name.IsEmpty())
        return *this;
    int i = node_styles_.Find(name);
    if(i < 0) node_styles_.Add(name, style); else node_styles_[i] = style;
    // Node style classes can change local prepared geometry (margins, ports,
    // child-control lane) but not model world occupancy or edge routing.
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    RefreshLayout();
    Refresh();
    return *this;
}

UiNodeGraph& UiNodeGraph::RemoveNodeStyleClass(const String& name)
{
    int i = node_styles_.Find(name);
    if(i >= 0) {
        node_styles_.Remove(i);
        InvalidateGeometry();
        PrepareGeometry();
        UpdateAttachedCtrls();
        RefreshLayout();
        Refresh();
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
    for(UiGraphModel* bound : bound_models_)
        if(bound == &model)
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

void UiNodeGraph::HandleModelChange(const UiGraphChange& change)
{
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
}

UiNodeGraph& UiNodeGraph::SetModel(UiGraphModel& model)
{
    if(model_ == &model)
        return *this;
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
    for(int i = 0; i < node_ctrls_.GetCount(); i++)
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
                    zoom_ >= style.content_cell_min_zoom &&
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
    // Fit may zoom out, but never auto-enlarge above the authored 1:1 scale.
    // This keeps a small graph visually representative instead of ballooning it.
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
    out.micro = sz.cx < 52 || sz.cy < 34;

    StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_);
    StyledSkin skin = ScaleNodeSkin(style.skin, zoom_);
    out.surface = UiStyledSurfaceRect(out.rect, metrics);
    out.content = UiStyledInnerRect(out.rect, metrics, skin);
    out.hit_path = NodeShapePath(node, out.surface);

    Rect heading = out.content;
    Ctrl* child = GetNodeCtrl(node.ref);
    if(child && !node.collapsed && !out.content.IsEmpty()) {
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
    bool have_icon = !out.compact && style.show_icon && !IsNull(node.icon) && !out.header.IsEmpty();
    if(have_icon) {
        Size icon_size = node.icon_size.cx > 0 && node.icon_size.cy > 0 ? node.icon_size : style.icon_size;
        icon_size.cx = min(ScaleMetric(icon_size.cx, max(0.75, zoom_)), max(0, out.header.GetWidth()));
        icon_size.cy = min(ScaleMetric(icon_size.cy, max(0.75, zoom_)), max(0, out.header.GetHeight()));
        int gap = ScaleMetric(style.icon_text_gap, zoom_);
        if(style.icon_side == UiAlign::RIGHT) {
            out.icon = RectC(out.header.right - icon_size.cx, out.header.top + (out.header.GetHeight() - icon_size.cy) / 2, icon_size.cx, icon_size.cy);
            text_lane.right = max(text_lane.left, out.icon.left - gap);
        }
        else {
            out.icon = RectC(out.header.left, out.header.top + (out.header.GetHeight() - icon_size.cy) / 2, icon_size.cx, icon_size.cy);
            text_lane.left = min(text_lane.right, out.icon.right + gap);
        }
    }

    int title_h = out.micro || node.title.IsEmpty() ? 0 : max(1, GetTextSize(node.title, style.title_font).cy);
    int subtitle_h = out.compact || node.subtitle.IsEmpty() ? 0 : max(1, GetTextSize(node.subtitle, style.subtitle_font).cy);
    int title_gap = title_h > 0 && subtitle_h > 0 ? ScaleMetric(style.title_subtitle_gap, zoom_) : 0;
    int text_top = text_lane.top + max(0, (text_lane.GetHeight() - title_h - title_gap - subtitle_h) / 2);
    if(title_h > 0) {
        out.title = Rect(text_lane.left, text_top, text_lane.right, min(text_lane.bottom, text_top + title_h));
        text_top = out.title.bottom + title_gap;
    }
    if(subtitle_h > 0)
        out.subtitle = Rect(text_lane.left, text_top, text_lane.right, min(text_lane.bottom, text_top + subtitle_h));
    if(!out.compact && style.show_description && !node.description.IsEmpty() && heading.bottom > out.header.bottom) {
        int top = min(heading.bottom, out.header.bottom + ScaleMetric(style.subtitle_description_gap, zoom_));
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
            int hit = max(DPI(4), fround(style.port_hit_radius * zoom_));
            Rect hit_rect = RectC(anchor.x - hit, anchor.y - hit, hit * 2 + 1, hit * 2 + 1);
            out.port_hits.Add(port.id, hit_rect);
            out.paint_bounds |= hit_rect;
            if(!out.compact && style.show_port_labels) {
                String label = PortLabel(port, style.show_port_type);
                Size tsz = GetTextSize(label, style.port_font);
                int gap = max(2, fround(style.port_label_gap * zoom_));
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
    out.paint_bounds = out.paint_bounds.Inflated(max(2, fround(2 * max(0.75, zoom_))));
}

Vector<Pointf> UiNodeGraph::BuildStraightRoute(Pointf source, Pointf target, const Vector<Pointf>& waypoints)
{
    Vector<Pointf> route;
    route.Add(source); route.Append(clone(waypoints)); route.Add(target);
    return SimplifyRoute(route);
}

Vector<Pointf> UiNodeGraph::BuildBezierRoute(Pointf source, UiGraphPortSide source_side,
                                              Pointf target, UiGraphPortSide target_side,
                                              double tension, int samples)
{
    Vector<Pointf> route;
    double distance = max(40.0, VectorLength(target - source));
    double handle = max(24.0, distance * minmax(tension, 0.05, 1.25));
    Pointf c1 = source + SideVector(source_side) * handle;
    Pointf c2 = target + SideVector(target_side) * handle;
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
    Vector<Pointf> waypoints;
    for(const Pointf& p : edge.waypoints) {
        Point q = WorldToScreen(p); waypoints.Add(Pointf(q.x, q.y));
    }
    Pointf a(source.x, source.y), b(target.x, target.y);
    Vector<Pointf> route;
    if(route_style == UiGraphRouteStyle::Straight)
        route = BuildStraightRoute(a, b, waypoints);
    else if(route_style == UiGraphRouteStyle::Orthogonal)
        route = BuildOrthogonalRoute(a, source_side, b, target_side,
                                     style.orthogonal_lead * zoom_, style.orthogonal_radius * zoom_, waypoints);
    else if(route_style == UiGraphRouteStyle::Custom && WhenBuildCustomRoute)
        route = WhenBuildCustomRoute(edge, a, source_side, b, target_side, style);
    else
        route = BuildBezierRoute(a, source_side, b, target_side, style.bezier_tension);
    out.ref = edge.ref;
    for(const Pointf& q : route) out.points.Add(Point(fround(q.x), fround(q.y)));
    out.bounds = RouteBounds(out.points, fround(style.interaction_width));
    if(!out.points.IsEmpty()) {
        out.label_point = out.points[out.points.GetCount() / 2];
        if(!edge.title.IsEmpty()) {
            Size ts = GetTextSize(edge.title, style.label_font);
            out.bounds |= RectC(out.label_point.x - ts.cx / 2 - DPI(5), out.label_point.y - ts.cy / 2 - DPI(3), ts.cx + DPI(10), ts.cy + DPI(6));
        }
    }
}

void UiNodeGraph::PrepareGeometry()
{
    if(!geometry_dirty_ && model_ && model_revision_ == model_->GetRevision() &&
       geometry_zoom_ == zoom_ && geometry_pan_ == pan_ && geometry_size_ == GetSize())
        return;
    RebuildGeometry();
}

void UiNodeGraph::RebuildGeometry()
{
    node_geometry_.Clear();
    edge_geometry_.Clear();
    last_node_candidate_count_ = 0;
    last_edge_candidate_count_ = 0;
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
    QuerySpatial(GetViewportWorldBounds(160.0), node_candidates, edge_candidates);

    // Drag previews deliberately remain outside the authoritative spatial index
    // until committed. Keep the active drag and its incident edges prepared.
    for(int i = 0; i < drag_preview_positions_.GetCount(); i++) {
        UiGraphNodeRef ref{drag_preview_positions_.GetKey(i)};
        node_candidates.FindAdd(ref.id);
        for(UiGraphEdgeRef edge : model_->GetNodeEdges(ref))
            edge_candidates.FindAdd(edge.id);
    }

    // Exact edge anchors need endpoint geometry even when the endpoint itself is
    // just outside the viewport margin.
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
    Rect damage = GetNodeDamage(ref);
    InvalidateGeometry();
    PrepareGeometry();
    damage |= GetNodeDamage(ref);
    return damage;
}

Rect UiNodeGraph::RebuildEdge(UiGraphEdgeRef ref)
{
    Rect damage = GetEdgeDamage(ref);
    InvalidateGeometry();
    PrepareGeometry();
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
    UiNodeGraph* self = const_cast<UiNodeGraph*>(this); self->PrepareGeometry();
    last_port_hit_candidate_count_ = 0;
    if(!model_) return UiGraphPortRef();
    for(int n = node_geometry_.GetCount() - 1; n >= 0; --n) {
        last_port_hit_candidate_count_++;
        const NodeGeometry& g = node_geometry_[n]; const UiGraphNode* node = model_->FindNode(g.ref);
        if(!node) continue;
        for(int i = g.port_hits.GetCount() - 1; i >= 0; --i)
            if(g.port_hits[i].Contains(p)) return UiGraphPortRef{node->ref, g.port_hits.GetKey(i)};
    }
    return UiGraphPortRef();
}

UiGraphNodeRef UiNodeGraph::HitTestNode(Point p) const
{
    UiNodeGraph* self = const_cast<UiNodeGraph*>(this); self->PrepareGeometry();
    last_node_hit_candidate_count_ = 0;
    for(int i = node_geometry_.GetCount() - 1; i >= 0; --i) {
        last_node_hit_candidate_count_++;
        const NodeGeometry& g = node_geometry_[i]; const UiGraphNode* node = model_ ? model_->FindNode(g.ref) : nullptr;
        if(node && PointInNodeGeometry(*node, g, p)) return g.ref;
    }
    return UiGraphNodeRef();
}

UiGraphEdgeRef UiNodeGraph::HitTestEdge(Point p) const
{
    UiNodeGraph* self = const_cast<UiNodeGraph*>(this); self->PrepareGeometry();
    last_edge_hit_candidate_count_ = 0;
    if(!model_) return UiGraphEdgeRef();
    for(int i = edge_geometry_.GetCount() - 1; i >= 0; --i) {
        last_edge_hit_candidate_count_++;
        const EdgeGeometry& g = edge_geometry_[i];
        if(!g.bounds.Contains(p)) continue;
        const UiGraphEdge* edge = model_->FindEdge(g.ref);
        if(!edge || !edge->selectable) continue;
        UiGraphEdgeStyle style = ResolveEdgeStyle(*edge, GetEdgeVisualState(*edge));
        for(int n = 1; n < g.points.GetCount(); n++)
            if(DistanceToSegment(Pointf(p.x, p.y), Pointf(g.points[n-1].x, g.points[n-1].y), Pointf(g.points[n].x, g.points[n].y)) <= style.interaction_width)
                return g.ref;
    }
    return UiGraphEdgeRef();
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
    int si = VisualStateIndex(state); Color color = style.color[si];
    double width = max(0.5, style.width[si] * max(0.65, zoom_));
    UiLineStyle line_style = style.line_style;
    if(edge.stroke == UiGraphStrokeStyle::Solid) line_style = SOLID;
    else if(edge.stroke == UiGraphStrokeStyle::Dashed) line_style = DASHED;
    else if(edge.stroke == UiGraphStrokeStyle::Dotted) line_style = DOTTED;
    StrokePolyline(p, g.points, width, color, line_style, style.dash_length * zoom_, style.dash_gap * zoom_);
    UiGraphArrowStyle arrow = edge.arrow == UiGraphArrowStyle::Inherit ? style.arrow : edge.arrow;
    if(edge.directed) PaintArrow(p, g.points, arrow, style.arrow_size * max(0.75, zoom_), color);
}

void UiNodeGraph::PaintNodeSurface(Draw& w, const UiGraphNode& node, const NodeGeometry& g,
                                   const UiGraphNodeStyle& style, UiGraphVisualState state)
{
    bool handled = false;
    if(WhenPaintNodeBackground) WhenPaintNodeBackground(w, node, g.rect, style, state, handled);
    if(handled || !UsesRectangularStyledSurface(node.shape)) return;
    StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_); StyledSkin skin = ScaleNodeSkin(style.skin, zoom_);
    if(node.shape == UiGraphNodeShape::Rectangle) metrics.radius = 0;
    else if(node.shape == UiGraphNodeShape::Capsule) metrics.radius = max(0, min(g.surface.GetWidth(), g.surface.GetHeight()) / 2);
    else metrics.radius = max(0, fround(node.corner_radius * zoom_));
    UiPaintStyledBackground(w, g.rect, style.palette, metrics, skin, ToStyledState(state), false);
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
        if(shadow.enabled && !shadow.inset && shadow.alpha > 0 && shadow.distance > 0) {
            int extent = max(1, shadow.distance);
            for(int d = extent; d >= 1; --d) {
                double t = ((double)d - 0.5) / max(1, extent);
                double falloff = shadow.mode == SHADOW_HARD ? 1.0 : (1.0 - UiShadowCurveEval(shadow.curve, t));
                int alpha = clamp((int)std::round(shadow.alpha * falloff / extent), 0, 255);
                if(alpha <= 0) continue;
                Vector<Pointf> shape = clone(g.hit_path); Pointf off((double)shadow.offset_x * d / extent, (double)shadow.offset_y * d / extent);
                for(Pointf& q : shape) q += off;
                RGBA rgba(shadow.color); rgba.a = (byte)alpha; FillPath(p, shape, rgba);
            }
        }
        FillPath(p, g.hit_path, ResolveFace(style.palette.face[si], White()));
        StrokePath(p, g.hit_path, max(0.75, (double)metrics.frame_width), style.palette.frame[si], true);
    }
    if(!g.micro && !custom_body && style.show_header_band && !node.collapsed && g.header.GetHeight() > 0) {
        Rect hr = g.header.Deflated(max(1, fround(2 * zoom_)), max(1, fround(2 * zoom_)));
        Vector<Pointf> header; header << Pointf(hr.left, hr.top) << Pointf(hr.right, hr.top) << Pointf(hr.right, hr.bottom) << Pointf(hr.left, hr.bottom);
        FillPath(p, RoundedPolygon(header, max(0.0, node.corner_radius * zoom_ * 0.55), 5), style.header_face[si]);
    }
    int port_radius = max(2, fround(style.port_radius * max(0.75, zoom_)));
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
        FillPath(p, shape, fill); StrokePath(p, shape, max(1.0, zoom_), frame, true);
    }
}

void UiNodeGraph::PaintNodeText(Draw& w, const UiGraphNode& node, const NodeGeometry& g,
                                const UiGraphNodeStyle& style, UiGraphVisualState state)
{
    int si = VisualStateIndex(state);
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
    paint_one(node.title, g.title, style.title_font, style.title_ink[si], UiAlign::CENTER, title_align);
    paint_one(node.subtitle, g.subtitle, style.subtitle_font, style.subtitle_ink[si], UiAlign::CENTER, style.text_align_h);
    if(!g.compact && style.show_description && !node.description.IsEmpty() && !g.description.IsEmpty()) {
        Vector<String> lines; Vector<Size> sizes; UiBuildStyledTextLines(node.description, style.description_font, lines, sizes);
        UiPaintStyledText(w, g.description, lines, sizes, style.text_align_h, UiAlign::TOP, style.description_font, style.description_ink[si], 0, false, 0, 0);
    }
    if(!g.compact && style.show_port_labels && !node.collapsed)
        for(int i = g.port_labels.GetCount() - 1; i >= 0; --i) {
            const UiGraphPort* port = model_->FindPort(UiGraphPortRef{node.ref, g.port_labels.GetKey(i)});
            if(!port) continue;
            String label = PortLabel(*port, style.show_port_type); Vector<String> lines; Vector<Size> sizes;
            lines.Add(label); sizes.Add(GetTextSize(label, style.port_font));
            UiPaintStyledText(w, g.port_labels[i], lines, sizes, UiAlign::LEFT, UiAlign::CENTER,
                              style.port_font, style.port_label_ink[si], 0, false, 0, 0);
        }
}

void UiNodeGraph::PaintConnectionPreview(Painter& p)
{
    if(interaction_ != InteractionMode::Connect || !connection_source_.IsValid()) return;
    UiGraphPortSide source_side = UiGraphPortSide::Right, target_side = UiGraphPortSide::Left;
    Point a = GetPortAnchor(connection_source_, &source_side); Point b = last_point_;
    if(connection_target_.IsValid()) b = GetPortAnchor(connection_target_, &target_side);
    Vector<Pointf> route = BuildBezierRoute(Pointf(a.x, a.y), source_side, Pointf(b.x, b.y), target_side, 0.35, 20);
    Vector<Point> points; for(const Pointf& q : route) points.Add(Point(fround(q.x), fround(q.y)));
    Color color = connection_target_.IsValid() ? (connection_decision_.IsAllowed() ? Color(34,197,94) : Color(239,68,68)) : Color(59,130,246);
    StrokePolyline(p, points, 2.0, color, DASHED, 7.0, 5.0);
}

void UiNodeGraph::PaintMarquee(Draw& w) const
{
    if(interaction_ != InteractionMode::Marquee || marquee_.IsEmpty()) return;
    const Style& style = GetEffectiveStyle(); RGBA fill(style.selection_box_fill); fill.a = (byte)minmax(style.selection_box_alpha, 0, 255);
    ImageBuffer ib(marquee_.GetSize()); BufferPainter p(ib, MODE_ANTIALIASED); p.Clear(fill); w.DrawImage(marquee_.left, marquee_.top, ib);
    w.DrawRect(marquee_.left, marquee_.top, marquee_.GetWidth(), 1, style.selection_box_frame);
    w.DrawRect(marquee_.left, marquee_.bottom - 1, marquee_.GetWidth(), 1, style.selection_box_frame);
    w.DrawRect(marquee_.left, marquee_.top, 1, marquee_.GetHeight(), style.selection_box_frame);
    w.DrawRect(marquee_.right - 1, marquee_.top, 1, marquee_.GetHeight(), style.selection_box_frame);
}

void UiNodeGraph::PaintGraphGeometry(Draw& w)
{
    Size size = GetSize(); if(size.cx <= 0 || size.cy <= 0) return;
    Rect viewport(Point(0,0), size);
    last_paint_node_visit_count_ = 0;
    last_paint_edge_visit_count_ = 0;
    last_painted_node_count_ = 0;
    last_painted_edge_count_ = 0;

    // Connections are deliberately painted first. Node surfaces/content and
    // selection chrome are composited over them, matching graph-scene depth.
    ImageBuffer edge_buffer(size); BufferPainter ep(edge_buffer, MODE_ANTIALIASED); ep.Clear(RGBAZero());
    for(int i = 0; i < edge_geometry_.GetCount(); i++) {
        last_paint_edge_visit_count_++;
        const EdgeGeometry& g = edge_geometry_[i];
        if((g.bounds & viewport).IsEmpty()) continue;
        const UiGraphEdge* edge = model_->FindEdge(g.ref); if(!edge) continue;
        last_painted_edge_count_++;
        UiGraphVisualState state = GetEdgeVisualState(*edge);
        PaintEdge(ep, *edge, g, ResolveEdgeStyle(*edge, state), state);
    }
    PaintConnectionPreview(ep);
    w.DrawImage(0,0,edge_buffer);

    // Edge labels/overlays are still below nodes so a node always wins visual
    // depth over a connection that crosses it.
    for(int i = 0; i < edge_geometry_.GetCount(); i++) {
        const EdgeGeometry& g = edge_geometry_[i];
        if((g.bounds & viewport).IsEmpty()) continue;
        const UiGraphEdge* edge = model_->FindEdge(g.ref); if(!edge) continue;
        UiGraphVisualState state = GetEdgeVisualState(*edge); UiGraphEdgeStyle style = ResolveEdgeStyle(*edge,state); int si = VisualStateIndex(state);
        if(!edge->title.IsEmpty()) {
            Size ts=GetTextSize(edge->title,style.label_font); Rect lr=RectC(g.label_point.x-ts.cx/2-DPI(4),g.label_point.y-ts.cy/2-DPI(2),ts.cx+DPI(8),ts.cy+DPI(4));
            if(style.draw_label_background) w.DrawRect(lr,style.label_background);
            w.DrawText(lr.left+DPI(4),lr.top+DPI(2),edge->title,style.label_font,style.label_ink[si]);
        }
        if(WhenPaintEdgeOverlay) WhenPaintEdgeOverlay(w,*edge,g.points,state);
    }

    for(int i = 0; i < node_geometry_.GetCount(); i++) {
        last_paint_node_visit_count_++;
        const NodeGeometry& g = node_geometry_[i]; if((g.paint_bounds & viewport).IsEmpty()) continue;
        const UiGraphNode* node = model_->FindNode(g.ref); if(!node) continue;
        last_painted_node_count_++;
        UiGraphVisualState state = GetNodeVisualState(*node); PaintNodeSurface(w,*node,g,ResolveNodeStyle(*node,state),state);
    }

    ImageBuffer node_buffer(size); BufferPainter np(node_buffer, MODE_ANTIALIASED); np.Clear(RGBAZero());
    for(int i = 0; i < node_geometry_.GetCount(); i++) {
        const NodeGeometry& g = node_geometry_[i]; if((g.paint_bounds & viewport).IsEmpty()) continue;
        const UiGraphNode* node = model_->FindNode(g.ref); if(!node) continue;
        UiGraphVisualState state = GetNodeVisualState(*node); PaintNodeDetails(np,*node,g,ResolveNodeStyle(*node,state),state);
    }
    w.DrawImage(0,0,node_buffer);

    for(int i = 0; i < node_geometry_.GetCount(); i++) {
        const NodeGeometry& g=node_geometry_[i]; if((g.paint_bounds&viewport).IsEmpty()) continue;
        const UiGraphNode* node=model_->FindNode(g.ref); if(!node) continue;
        UiGraphVisualState state=GetNodeVisualState(*node); UiGraphNodeStyle style=ResolveNodeStyle(*node,state);
        PaintNodeText(w,*node,g,style,state); if(WhenPaintNodeOverlay) WhenPaintNodeOverlay(w,*node,g.rect,state);
        bool handled=false; if(WhenPaintNodeForeground) WhenPaintNodeForeground(w,*node,g.rect,style,state,handled);
        if(!handled && UsesRectangularStyledSurface(node->shape)) {
            StyledMetrics metrics=ScaleNodeMetrics(style.metrics,zoom_); StyledSkin skin=ScaleNodeSkin(style.skin,zoom_);
            if(node->shape==UiGraphNodeShape::Rectangle) metrics.radius=0;
            else if(node->shape==UiGraphNodeShape::Capsule) metrics.radius=max(0,min(g.surface.GetWidth(),g.surface.GetHeight())/2);
            else metrics.radius=max(0,fround(node->corner_radius*zoom_));
            UiPaintStyledForeground(w,g.rect,style.palette,metrics,skin,ToStyledState(state),HasFocus()&&IsNodeSelected(node->ref));
        }
    }
}

void UiNodeGraph::Paint(Draw& w)
{
    Rect outer(Point(0,0),GetSize()); const Style& style=GetEffectiveStyle();
    UiPaintStyledBackground(w,outer,style.canvas_palette,style.canvas_metrics,style.canvas_skin,ST_NORMAL,HasFocus());
    PaintGrid(w,outer);
    if(model_) PaintGraphGeometry(w);
    PaintMarquee(w);
    UiPaintStyledForeground(w,outer,style.canvas_palette,style.canvas_metrics,style.canvas_skin,ST_NORMAL,HasFocus());
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
