// Exact preparation, rich/micro node geometry, semantic anchors and edge routes.
// PrepareViewGeometry is the only exact preparation entry point. Camera reuse
// lives in Projection; the spatial index remains in world coordinates.

namespace Upp {

namespace {
constexpr int kLodMicroPathCacheLimit = 64;

Vector<Pointf> LodNodeShapePath(const UiGraphNode& node, const Rect& rect)
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
    case UiGraphNodeShape::Database:
        v << Pointf(l, t + rect.GetHeight() * 0.14)
          << Pointf(l + rect.GetWidth() * 0.18, t)
          << Pointf(r - rect.GetWidth() * 0.18, t)
          << Pointf(r, t + rect.GetHeight() * 0.14)
          << Pointf(r, b - rect.GetHeight() * 0.14)
          << Pointf(r - rect.GetWidth() * 0.18, b)
          << Pointf(l + rect.GetWidth() * 0.18, b)
          << Pointf(l, b - rect.GetHeight() * 0.14);
        return UiGeometry::RoundedPolygon(v, max(radius, rect.GetHeight() * 0.08));
    case UiGraphNodeShape::LegacyRectangle:
    case UiGraphNodeShape::Custom:
        v << Pointf(l, t) << Pointf(r, t) << Pointf(r, b) << Pointf(l, b);
        return v;
    case UiGraphNodeShape::Square:
    case UiGraphNodeShape::RoundedRectangle:
    default:
        v << Pointf(l, t) << Pointf(r, t) << Pointf(r, b) << Pointf(l, b);
        return UiGeometry::RoundedPolygon(v, radius);
    }
}

struct ViewNodeOrderRecord : Moveable<ViewNodeOrderRecord> {
    const UiGraphNode* node = nullptr;
    UiGraphNodeRef ref;
    int z_order = 0;
};
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
    out.compact = UiNodeGraphDetail::IsCompactNode(sz.cx, sz.cy);
    out.micro = UiNodeGraphDetail::IsMicroNode(sz.cx, sz.cy);

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
    double hit_width = EffectiveEdgeHitWidth(style);
    int si = VisualStateIndex(GetEdgeVisualState(edge));
    double paint_scale = min(1.0, max(0.01, zoom_));
    double stroke_extent = max(0.0, style.width[si] * paint_scale * 0.5);
    UiGraphArrowStyle resolved_arrow = edge.arrow == UiGraphArrowStyle::Inherit
                                    ? style.arrow : edge.arrow;
    double arrow_extent = edge.directed && resolved_arrow != UiGraphArrowStyle::None
                        ? max(0.0, style.arrow_size * paint_scale) : 0.0;
    int route_pad = (int)std::ceil(max(hit_width, stroke_extent + arrow_extent));
    out.bounds = RouteBounds(out.points, route_pad);
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


Rect UiNodeGraph::RebuildNodeAndEdges(UiGraphNodeRef ref)
{
    PrepareViewGeometry();
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
    PrepareViewGeometry();
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
    self->PrepareViewGeometry();
    const EdgeGeometry* g = FindEdgeGeometry(ref);
    return g ? g->route_handle_hit : RectC(0, 0, 0, 0);
}

UiGraphEdgeRef UiNodeGraph::HitTestEdgeRouteHandle(Point p) const
{
    UiNodeGraph* self = const_cast<UiNodeGraph*>(this);
    self->PrepareViewGeometry();
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
    if(node.shape == UiGraphNodeShape::LegacyRectangle ||
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


void UiNodeGraph::PrepareViewGeometry()
{
    if(batch_update_depth_ > 0 || view_update_depth_ > 0)
        return;
    if(!geometry_dirty_ && model_ && model_revision_ == model_->GetRevision() &&
       geometry_zoom_ == zoom_ && geometry_pan_ == pan_ && geometry_size_ == GetSize())
        return;
    int64 started = usecs();
    RebuildViewGeometry();
    last_geometry_prepare_usecs_ = max<int64>(0, usecs() - started);
}

void UiNodeGraph::BuildViewNodeGeometry(const UiGraphNode& node, NodeGeometry& out,
                                            bool record_evidence)
{
    Point p = WorldToScreen(GetDisplayNodePosition(node));
    Size sz(max(1, fround(node.size.cx * zoom_)), max(1, fround(node.size.cy * zoom_)));
    if(node.shape == UiGraphNodeShape::Square || node.shape == UiGraphNodeShape::Circle) {
        int side = max(sz.cx, sz.cy);
        sz = Size(side, side);
    }

    // Projected size is the authoritative detail boundary. A 20px-wide node is
    // micro regardless of the abstract zoom value; constructing title/content/
    // port-hit geometry for it only creates invisible work and was the dominant
    // intermediate-zoom 10k stall exposed by the P1 live profile.
    const bool overview_geometry = UiNodeGraphDetail::IsMicroNode(sz.cx, sz.cy);

    if(!overview_geometry) {
        BuildNodeGeometry(node, out);
        if(record_evidence)
            last_geometry_path_vertex_count_ += out.hit_path.GetCount();
        return;
    }

    UiGraphVisualState node_state = GetNodeVisualState(node);
    int64 resolve_started = record_evidence ? usecs() : 0;
    UiGraphNodeStyle style = ResolveNodeStyle(node, node_state);
    int64 resolve_us = record_evidence ? max<int64>(0, usecs() - resolve_started) : 0;
    int64 scale_started = record_evidence ? usecs() : 0;
    StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_);
    int64 scale_us = record_evidence ? max<int64>(0, usecs() - scale_started) : 0;
    if(record_evidence) {
        last_geometry_style_resolve_usecs_ += resolve_us;
        last_geometry_style_scale_usecs_ += scale_us;
        last_geometry_style_usecs_ += resolve_us + scale_us;
    }

    out.ref = node.ref;
    out.rect = RectC(p.x, p.y, sz.cx, sz.cy);
    out.paint_bounds = out.rect;
    out.z_order = node.z_order;
    out.compact = true;
    out.micro = true;
    out.surface = UiStyledSurfaceRect(out.rect, metrics);

    int64 silhouette_started = record_evidence ? usecs() : 0;
    int cached_path = -1;
    for(int i = 0; i < micro_path_cache_.GetCount(); i++) {
        const MicroPathCacheEntry& entry = micro_path_cache_[i];
        if(entry.shape == node.shape
           && entry.surface_size == out.surface.GetSize()
           && entry.source_cx == node.size.cx
           && entry.source_cy == node.size.cy
           && entry.corner_radius == node.corner_radius) {
            cached_path = i;
            break;
        }
    }

    if(cached_path >= 0) {
        const Vector<Pointf>& local = micro_path_cache_[cached_path].local_path;
        out.hit_path.SetCount(local.GetCount());
        for(int i = 0; i < local.GetCount(); i++)
            out.hit_path[i] = Pointf(local[i].x + out.surface.left,
                                     local[i].y + out.surface.top);
        if(record_evidence)
            last_geometry_path_cache_hit_count_++;
    }
    else {
        out.hit_path = LodNodeShapePath(node, out.surface);
        if(record_evidence)
            last_geometry_path_cache_miss_count_++;
        if(micro_path_cache_.GetCount() < kLodMicroPathCacheLimit) {
            MicroPathCacheEntry& entry = micro_path_cache_.Add();
            entry.shape = node.shape;
            entry.surface_size = out.surface.GetSize();
            entry.source_cx = node.size.cx;
            entry.source_cy = node.size.cy;
            entry.corner_radius = node.corner_radius;
            entry.local_path.SetCount(out.hit_path.GetCount());
            for(int i = 0; i < out.hit_path.GetCount(); i++)
                entry.local_path[i] = Pointf(out.hit_path[i].x - out.surface.left,
                                             out.hit_path[i].y - out.surface.top);
        }
    }
    if(record_evidence)
        last_geometry_silhouette_usecs_ += max<int64>(0, usecs() - silhouette_started);

    // Visible edges still need stable authored-side anchors, but micro geometry
    // deliberately carries no text/content/control/label/hit rectangles. Reuse
    // four index buffers so each node does not allocate side-port temporaries.
    int64 anchor_started = record_evidence ? usecs() : 0;
    for(int si = 0; si < 4; si++)
        micro_side_port_scratch_[si].SetCount(0);

    for(int i = 0; i < node.ports.GetCount(); i++) {
        if(!node.ports[i].visible)
            continue;
        UiGraphPortSide side = ResolvePortSide(node.ports[i]);
        int si = side == UiGraphPortSide::Left ? 0
               : side == UiGraphPortSide::Right ? 1
               : side == UiGraphPortSide::Top ? 2 : 3;
        micro_side_port_scratch_[si].Add(i);
    }
    for(int si = 0; si < 4; si++)
        Sort(micro_side_port_scratch_[si], [&](int a, int b) {
            return node.ports[a].order != node.ports[b].order
                 ? node.ports[a].order < node.ports[b].order : a < b;
        });

    for(int si = 0; si < 4; si++) {
        const Vector<int>& indexes = micro_side_port_scratch_[si];
        int count = indexes.GetCount();
        if(count == 0)
            continue;
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
            out.anchors.Add(port.id, ShapeBoundaryAnchor(out.hit_path, out.surface, side, axis));
        }
    }
    if(record_evidence)
        last_geometry_anchor_usecs_ += max<int64>(0, usecs() - anchor_started);

    int inflate = max(1, fround(2 * min(1.0, max(0.25, zoom_))));
    int state_index = VisualStateIndex(node_state);
    Color paint_frame = node_state == UiGraphVisualState::Selected
                      ? SelectedFrameColor(style) : style.palette.frame[state_index];
    int paint_frame_width = IsNull(paint_frame) ? 0 : max(1, metrics.frame_width);
    if(node_state == UiGraphVisualState::Selected)
        paint_frame_width = max(paint_frame_width,
                                max(DPI(2), DPI(fround(lod_policy_.selection_outline_width))));
    if(paint_frame_width > 0)
        inflate = max(inflate, LodMicroStrokePad(paint_frame_width));
    if(IsNodeSelected(node.ref))
        inflate = max(inflate, LodMicroStrokePad(
            max(paint_frame_width, DPI(fround(lod_policy_.selection_outline_width)))));
    out.paint_bounds = out.surface.Inflated(inflate);
    out.paint_bounds |= out.rect;

    if(record_evidence) {
        last_geometry_lod_node_count_++;
        last_geometry_path_vertex_count_ += out.hit_path.GetCount();
    }
}

void UiNodeGraph::RebuildViewGeometry()
{
    int64 reset_started = usecs();
    node_geometry_.Clear();
    edge_geometry_.Clear();
    micro_path_cache_.Clear();
    last_node_candidate_count_ = 0;
    last_edge_candidate_count_ = 0;
    last_hidden_edge_count_ = 0;
    last_geometry_lod_node_count_ = 0;
    last_geometry_path_vertex_count_ = 0;
    last_geometry_spatial_usecs_ = 0;
    last_geometry_query_usecs_ = 0;
    last_geometry_sort_usecs_ = 0;
    last_geometry_node_usecs_ = 0;
    last_geometry_style_usecs_ = 0;
    last_geometry_style_resolve_usecs_ = 0;
    last_geometry_style_scale_usecs_ = 0;
    last_geometry_silhouette_usecs_ = 0;
    last_geometry_anchor_usecs_ = 0;
    last_geometry_edge_usecs_ = 0;
    last_geometry_path_cache_hit_count_ = 0;
    last_geometry_path_cache_miss_count_ = 0;
    last_geometry_reset_usecs_ = max<int64>(0, usecs() - reset_started);

    if(!model_) {
        model_revision_ = -1;
        geometry_zoom_ = zoom_;
        geometry_pan_ = pan_;
        geometry_size_ = GetSize();
        geometry_dirty_ = false;
        geometry_build_serial_++;
        prepared_geometry_revision_++;
        return;
    }

    int64 spatial_started = usecs();
    EnsureSpatialIndex();
    last_geometry_spatial_usecs_ = max<int64>(0, usecs() - spatial_started);

    int64 query_started = usecs();
    for(int i = selected_nodes_.GetCount() - 1; i >= 0; --i)
        if(!model_->Contains(UiGraphNodeRef{selected_nodes_[i]}))
            selected_nodes_.Remove(i);
    for(int i = selected_edges_.GetCount() - 1; i >= 0; --i)
        if(!model_->Contains(UiGraphEdgeRef{selected_edges_[i]}))
            selected_edges_.Remove(i);

    Index<UiGraphId> node_candidates;
    Index<UiGraphId> edge_candidates;
    QuerySpatial(GetViewportWorldBounds(GetPaintQueryMargin()), node_candidates, edge_candidates,
                 SPATIAL_QUERY_NODES | SPATIAL_QUERY_EDGES | SPATIAL_QUERY_REDUCE_OVERVIEW);

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
    last_geometry_query_usecs_ = max<int64>(0, usecs() - query_started);

    int64 sort_started = usecs();
    Vector<ViewNodeOrderRecord> order;
    order.Reserve(node_candidates.GetCount());
    for(int i = 0; i < node_candidates.GetCount(); i++) {
        UiGraphNodeRef ref{node_candidates[i]};
        const UiGraphNode* node = model_->FindNode(ref);
        if(node && node->visible) {
            ViewNodeOrderRecord& record = order.Add();
            record.node = node;
            record.ref = ref;
            record.z_order = node->z_order;
        }
    }
    Sort(order, [](const ViewNodeOrderRecord& a, const ViewNodeOrderRecord& b) {
        return a.z_order != b.z_order ? a.z_order < b.z_order : a.ref.id < b.ref.id;
    });
    last_geometry_sort_usecs_ = max<int64>(0, usecs() - sort_started);

    int64 node_started = usecs();
    for(const ViewNodeOrderRecord& record : order) {
        NodeGeometry g;
        BuildViewNodeGeometry(*record.node, g);
        node_geometry_.Add(record.ref.id, pick(g));
    }
    last_geometry_node_usecs_ = max<int64>(0, usecs() - node_started);

    int64 edge_started = usecs();
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
    last_geometry_edge_usecs_ = max<int64>(0, usecs() - edge_started);

    model_revision_ = model_->GetRevision();
    geometry_zoom_ = zoom_;
    geometry_pan_ = pan_;
    geometry_size_ = GetSize();
    geometry_dirty_ = false;
    geometry_build_serial_++;
    prepared_geometry_revision_++;
}

} // namespace Upp
