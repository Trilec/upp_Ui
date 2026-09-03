namespace Upp {
namespace {

constexpr double kUiGraphGeometryLodErrorPx = 0.40;
constexpr double kUiGraphPi = 3.14159265358979323846;

Vector<Pointf> LodRoundedPolygon(const Vector<Pointf>& vertices,
                                 double radius, int max_samples)
{
    if(vertices.GetCount() < 3)
        return clone(vertices);
    radius = max(0.0, radius);
    if(radius <= 0.0001)
        return clone(vertices);

    const int samples = UiArcSegmentsForPixels(radius, kUiGraphPi * 0.5,
                                                kUiGraphGeometryLodErrorPx,
                                                1, max(1, max_samples));
    Vector<Pointf> out;
    for(int i = 0; i < vertices.GetCount(); i++) {
        Pointf prev = vertices[(i + vertices.GetCount() - 1) % vertices.GetCount()];
        Pointf cur = vertices[i];
        Pointf next = vertices[(i + 1) % vertices.GetCount()];
        double a_len = VectorLength(prev - cur);
        double b_len = VectorLength(next - cur);
        double r = min(radius, min(a_len, b_len) * 0.45);
        if(r <= 0.0001) {
            if(out.IsEmpty() || VectorLength(out.Top() - cur) > 0.01)
                out.Add(cur);
            continue;
        }
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

Vector<Pointf> LodEllipsePath(const Rect& r)
{
    Vector<Pointf> out;
    double cx = (r.left + r.right) * 0.5;
    double cy = (r.top + r.bottom) * 0.5;
    double rx = max(0.5, r.GetWidth() * 0.5);
    double ry = max(0.5, r.GetHeight() * 0.5);
    int samples = UiArcSegmentsForPixels(max(rx, ry), 2.0 * kUiGraphPi,
                                         kUiGraphGeometryLodErrorPx, 8, 40);
    out.Reserve(samples);
    for(int i = 0; i < samples; i++) {
        double a = 2.0 * kUiGraphPi * i / samples;
        out.Add(Pointf(cx + std::cos(a) * rx,
                       cy + std::sin(a) * ry));
    }
    return out;
}

Vector<Pointf> LodCapsulePath(const Rect& r)
{
    Vector<Pointf> out;
    double l = r.left, t = r.top, rr = r.right, b = r.bottom;
    double w = max(1, r.GetWidth()), h = max(1, r.GetHeight());
    double radius = min(w, h) * 0.5;
    int samples = UiArcSegmentsForPixels(radius, kUiGraphPi,
                                         kUiGraphGeometryLodErrorPx, 3, 16);
    if(w >= h) {
        double cy = (t + b) * 0.5;
        double lc = l + radius;
        double rc = rr - radius;
        for(int i = 0; i <= samples; i++) {
            double a = -kUiGraphPi * 0.5 + kUiGraphPi * i / samples;
            out.Add(Pointf(rc + std::cos(a) * radius,
                           cy + std::sin(a) * radius));
        }
        for(int i = 0; i <= samples; i++) {
            double a = kUiGraphPi * 0.5 + kUiGraphPi * i / samples;
            out.Add(Pointf(lc + std::cos(a) * radius,
                           cy + std::sin(a) * radius));
        }
    }
    else {
        double cx = (l + rr) * 0.5;
        double tc = t + radius;
        double bc = b - radius;
        for(int i = 0; i <= samples; i++) {
            double a = kUiGraphPi * i / samples;
            out.Add(Pointf(cx + std::cos(a) * radius,
                           bc + std::sin(a) * radius));
        }
        for(int i = 0; i <= samples; i++) {
            double a = kUiGraphPi + kUiGraphPi * i / samples;
            out.Add(Pointf(cx + std::cos(a) * radius,
                           tc + std::sin(a) * radius));
        }
    }
    return out;
}

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
        return LodEllipsePath(rect);
    case UiGraphNodeShape::Diamond:
        v << Pointf(cx, t) << Pointf(r, cy) << Pointf(cx, b) << Pointf(l, cy);
        return LodRoundedPolygon(v, radius, 5);
    case UiGraphNodeShape::Triangle:
        v << Pointf(cx, t) << Pointf(r, b) << Pointf(l, b);
        return LodRoundedPolygon(v, radius, 5);
    case UiGraphNodeShape::Hexagon: {
        double dx = rect.GetWidth() * 0.24;
        v << Pointf(l + dx, t) << Pointf(r - dx, t) << Pointf(r, cy)
          << Pointf(r - dx, b) << Pointf(l + dx, b) << Pointf(l, cy);
        return LodRoundedPolygon(v, radius, 5);
    }
    case UiGraphNodeShape::Capsule:
        return LodCapsulePath(rect);
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
        return LodRoundedPolygon(v, max(radius, min(w, h) * 0.10), 6);
    }
    case UiGraphNodeShape::Document: {
        double fold = min(rect.GetWidth(), rect.GetHeight()) * 0.22;
        v << Pointf(l, t) << Pointf(r - fold, t) << Pointf(r, t + fold)
          << Pointf(r, b) << Pointf(l, b);
        return LodRoundedPolygon(v, radius, 5);
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
        return LodRoundedPolygon(v, max(radius, rect.GetHeight() * 0.08), 5);
    case UiGraphNodeShape::Rectangle:
    case UiGraphNodeShape::Custom:
        v << Pointf(l, t) << Pointf(r, t) << Pointf(r, b) << Pointf(l, b);
        return v;
    case UiGraphNodeShape::Square:
    case UiGraphNodeShape::RoundedRectangle:
    default:
        v << Pointf(l, t) << Pointf(r, t) << Pointf(r, b) << Pointf(l, b);
        return LodRoundedPolygon(v, radius, 6);
    }
}

} // namespace

UiNodeGraph& UiNodeGraph::BeginViewUpdate()
{
    if(view_update_depth_ == 0) {
        view_update_dirty_ = false;
        view_update_viewport_changed_ = false;
        view_update_selection_changed_ = false;
        view_update_scope_changed_ = false;
        view_update_scope_ = UiGraphScopeRef();
        view_update_geometry_serial_start_ = geometry_build_serial_;
        view_update_spatial_serial_start_ = spatial_build_serial_;
        last_view_update_geometry_build_count_ = 0;
        last_view_update_spatial_build_count_ = 0;
    }
    view_update_depth_++;
    return *this;
}

UiNodeGraph& UiNodeGraph::EndViewUpdate()
{
    ASSERT(view_update_depth_ > 0);
    if(view_update_depth_ <= 0)
        return *this;
    view_update_depth_--;
    if(view_update_depth_ == 0)
        FlushViewUpdate();
    return *this;
}

bool UiNodeGraph::DeferViewUpdate(bool viewport_changed, bool selection_changed)
{
    InvalidateGeometry();
    if(view_update_depth_ <= 0)
        return false;

    view_update_dirty_ = true;
    view_update_viewport_changed_ |= viewport_changed;
    view_update_selection_changed_ |= selection_changed;
    return true;
}

void UiNodeGraph::QueueScopeChanged(UiGraphScopeRef scope)
{
    if(view_update_depth_ <= 0) {
        WhenScopeChanged(scope);
        return;
    }
    view_update_scope_changed_ = true;
    view_update_scope_ = scope;
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

void UiNodeGraph::BuildViewNodeGeometry(const UiGraphNode& node, NodeGeometry& out)
{
    Point p = WorldToScreen(GetDisplayNodePosition(node));
    Size sz(max(1, fround(node.size.cx * zoom_)), max(1, fround(node.size.cy * zoom_)));
    if(node.shape == UiGraphNodeShape::Square || node.shape == UiGraphNodeShape::Circle) {
        int side = max(sz.cx, sz.cy);
        sz = Size(side, side);
    }

    const double title_floor = max(0.16, lod_policy_.title_zoom - 0.16);
    const double port_visual_floor = max(0.12, lod_policy_.port_zoom - 0.04);
    const double content_floor = max(0.24, lod_policy_.secondary_text_zoom - 0.08);
    const bool projected_micro = sz.cx < 38 || sz.cy < 26;
    const bool overview_geometry = projected_micro
                                && zoom_ < title_floor
                                && zoom_ < port_visual_floor
                                && zoom_ < content_floor;

    if(!overview_geometry) {
        BuildNodeGeometry(node, out);
        last_geometry_path_vertex_count_ += out.hit_path.GetCount();
        return;
    }

    UiGraphNodeStyle style = ResolveNodeStyle(node, GetNodeVisualState(node));
    out.ref = node.ref;
    out.rect = RectC(p.x, p.y, sz.cx, sz.cy);
    out.paint_bounds = out.rect;
    out.z_order = node.z_order;
    out.compact = true;
    out.micro = true;

    StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_);
    out.surface = UiStyledSurfaceRect(out.rect, metrics);
    out.hit_path = LodNodeShapePath(node, out.surface);

    // Even when ports are visually suppressed, visible edges still need stable
    // boundary anchors. Preserve authored port ordering but skip all text,
    // content, control, label and hit-rectangle geometry at this overview level.
    Vector<int> side_ports[4];
    for(int i = 0; i < node.ports.GetCount(); i++) {
        if(!node.ports[i].visible)
            continue;
        UiGraphPortSide side = ResolvePortSide(node.ports[i]);
        int si = side == UiGraphPortSide::Left ? 0
               : side == UiGraphPortSide::Right ? 1
               : side == UiGraphPortSide::Top ? 2 : 3;
        side_ports[si].Add(i);
    }
    for(int si = 0; si < 4; si++)
        Sort(side_ports[si], [&](int a, int b) {
            return node.ports[a].order != node.ports[b].order
                 ? node.ports[a].order < node.ports[b].order : a < b;
        });

    for(int si = 0; si < 4; si++) {
        const Vector<int>& indexes = side_ports[si];
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

    int inflate = max(1, fround(2 * min(1.0, max(0.25, zoom_))));
    if(IsNodeSelected(node.ref))
        inflate = max(inflate, DPI(fround(lod_policy_.selection_outline_width)) + DPI(2));
    out.paint_bounds = out.paint_bounds.Inflated(inflate);

    last_geometry_lod_node_count_++;
    last_geometry_path_vertex_count_ += out.hit_path.GetCount();
}

void UiNodeGraph::RebuildViewGeometry()
{
    node_geometry_.Clear();
    edge_geometry_.Clear();
    last_node_candidate_count_ = 0;
    last_edge_candidate_count_ = 0;
    last_hidden_edge_count_ = 0;
    last_geometry_lod_node_count_ = 0;
    last_geometry_path_vertex_count_ = 0;

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
        if(!model_->Contains(UiGraphNodeRef{selected_nodes_[i]}))
            selected_nodes_.Remove(i);
    for(int i = selected_edges_.GetCount() - 1; i >= 0; --i)
        if(!model_->Contains(UiGraphEdgeRef{selected_edges_[i]}))
            selected_edges_.Remove(i);

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
        if(!na || !nb)
            return a.id < b.id;
        return na->z_order != nb->z_order ? na->z_order < nb->z_order : a.id < b.id;
    });

    for(UiGraphNodeRef ref : order) {
        const UiGraphNode* node = model_->FindNode(ref);
        if(!node)
            continue;
        NodeGeometry g;
        BuildViewNodeGeometry(*node, g);
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

void UiNodeGraph::FlushViewUpdate()
{
    ASSERT(view_update_depth_ == 0);

    const bool dirty = view_update_dirty_;
    const bool viewport_changed = view_update_viewport_changed_;
    const bool selection_changed = view_update_selection_changed_;
    const bool scope_changed = view_update_scope_changed_;
    const UiGraphScopeRef scope = view_update_scope_;

    view_update_dirty_ = false;
    view_update_viewport_changed_ = false;
    view_update_selection_changed_ = false;
    view_update_scope_changed_ = false;
    view_update_scope_ = UiGraphScopeRef();

    if(dirty) {
        PrepareViewGeometry();
        UpdateAttachedCtrls();
        Refresh();
    }

    last_view_update_geometry_build_count_ = max(0, geometry_build_serial_ - view_update_geometry_serial_start_);
    last_view_update_spatial_build_count_ = max(0, spatial_build_serial_ - view_update_spatial_serial_start_);

    if(dirty || viewport_changed || selection_changed || scope_changed)
        view_update_flush_serial_++;

    // Publish events only after the final exact scene exists. This prevents a
    // host callback from observing an intermediate model/scope/camera state.
    if(selection_changed)
        WhenSelection();
    if(scope_changed)
        WhenScopeChanged(scope);
    if(viewport_changed)
        WhenViewport();
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

    if(DeferViewUpdate(true, false))
        return *this;

    PrepareViewGeometry();
    UpdateAttachedCtrls();
    Refresh();
    WhenViewport();
    return *this;
}

UiNodeGraph& UiNodeGraph::SetPan(Pointf pan)
{
    if(pan_ == pan)
        return *this;
    pan_ = pan;

    if(DeferViewUpdate(true, false))
        return *this;

    PrepareViewGeometry();
    UpdateAttachedCtrls();
    Refresh();
    WhenViewport();
    return *this;
}

UiNodeGraph& UiNodeGraph::PanBy(Pointf delta)
{
    return SetPan(pan_ + delta);
}

UiNodeGraph& UiNodeGraph::ResetView()
{
    if(abs(zoom_ - 1.0) < 1e-9 && pan_ == Pointf(0, 0))
        return *this;
    zoom_ = 1.0;
    pan_ = Pointf(0, 0);

    if(DeferViewUpdate(true, false))
        return *this;

    PrepareViewGeometry();
    UpdateAttachedCtrls();
    Refresh();
    WhenViewport();
    return *this;
}

} // namespace Upp
