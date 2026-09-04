namespace Upp {
namespace {

constexpr double kUiGraphPi = 3.14159265358979323846;

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
    case UiGraphNodeShape::Rectangle:
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

Vector<Point> LodIntegerPath(const Vector<Pointf>& path)
{
    Vector<Point> out;
    out.Reserve(path.GetCount());
    for(const Pointf& p : path) {
        Point q(fround(p.x), fround(p.y));
        if(out.IsEmpty() || out.Top() != q)
            out.Add(q);
    }
    if(out.GetCount() > 1 && out.Top() == out[0])
        out.Drop();
    return out;
}

} // namespace

void UiNodeGraph::PaintGraphGeometry(Draw& w)
{
    Size size = GetSize();
    if(size.cx <= 0 || size.cy <= 0 || !model_) {
        PaintGraphGeometryRenderBase(w);
        return;
    }
    Rect viewport(Point(0, 0), size);
    Rect paint = w.GetPaintRect() & viewport;
    if(paint.IsEmpty())
        return;

    // This fast path is deliberately narrow: normal viewing of projected-micro
    // nodes below edge-label/detail scale. Rich/reference nodes, active gestures,
    // custom body/foreground/overlay hooks and non-direct arrow styles retain the
    // accepted renderer unchanged.
    if(interaction_ != InteractionMode::None
       || !marquee_preview_nodes_.IsEmpty()
       || zoom_ >= lod_policy_.edge_label_zoom
       || WhenPaintNodeBackground
       || WhenPaintNodeForeground
       || WhenPaintNodeOverlay) {
        PaintGraphGeometryRenderBase(w);
        return;
    }

    WorldRect paint_world;
    paint_world.Include(ScreenToWorld(paint.TopLeft()));
    paint_world.Include(ScreenToWorld(paint.BottomRight()));
    paint_world = paint_world.Inflated(GetPaintQueryMargin() / max(zoom_, 0.01));
    Index<UiGraphId> node_ids;
    Index<UiGraphId> edge_ids;
    QuerySpatial(paint_world, node_ids, edge_ids);

    Vector<int> paint_nodes;
    paint_nodes.Reserve(node_ids.GetCount());
    for(int i = 0; i < node_ids.GetCount(); i++) {
        int q = node_geometry_.Find(node_ids[i]);
        if(q < 0 || (node_geometry_[q].paint_bounds & paint).IsEmpty())
            continue;
        if(!node_geometry_[q].micro) {
            PaintGraphGeometryRenderBase(w);
            return;
        }
        const UiGraphNode* node = model_->FindNode(node_geometry_[q].ref);
        if(!node || node->shape == UiGraphNodeShape::Custom) {
            PaintGraphGeometryRenderBase(w);
            return;
        }
        paint_nodes.Add(q);
    }
    if(paint_nodes.IsEmpty()) {
        PaintGraphGeometryRenderBase(w);
        return;
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
            if(q < 0 || (edge_geometry_[q].bounds & paint).IsEmpty())
                continue;
            const UiGraphEdge* edge = model_->FindEdge(edge_geometry_[q].ref);
            if(!edge)
                continue;
            UiGraphVisualState state = GetEdgeVisualState(*edge);
            UiGraphEdgeStyle style = ResolveEdgeStyle(*edge, state);
            UiGraphArrowStyle arrow = edge->arrow == UiGraphArrowStyle::Inherit ? style.arrow : edge->arrow;
            if(edge->directed && arrow != UiGraphArrowStyle::None && arrow != UiGraphArrowStyle::Open) {
                PaintGraphGeometryRenderBase(w);
                return;
            }
            paint_edges.Add(q);
        }

    last_paint_node_visit_count_ = paint_nodes.GetCount();
    last_paint_edge_visit_count_ = paint_edges.GetCount();
    last_painted_node_count_ = 0;
    last_painted_edge_count_ = 0;
    last_simplified_edge_count_ = 0;
    last_edge_paint_usecs_ = 0;
    last_node_paint_usecs_ = 0;
    last_node_surface_paint_usecs_ = 0;
    last_node_details_paint_usecs_ = 0;
    last_node_content_paint_usecs_ = 0;

    int64 edge_started = usecs();
    for(int q : paint_edges) {
        const EdgeGeometry& g = edge_geometry_[q];
        const UiGraphEdge* edge = model_->FindEdge(g.ref);
        if(!edge)
            continue;
        UiGraphVisualState state = GetEdgeVisualState(*edge);
        UiGraphEdgeStyle style = ResolveEdgeStyle(*edge, state);
        if(PaintEdgeDirect(w, *edge, g.points, g.simplified, style, state,
                           zoom_, lod_policy_)) {
            last_painted_edge_count_++;
            if(g.simplified)
                last_simplified_edge_count_++;
        }
    }
    last_edge_paint_usecs_ = max<int64>(0, usecs() - edge_started);

    int64 node_started = usecs();
    for(int q : paint_nodes) {
        const NodeGeometry& g = node_geometry_[q];
        const UiGraphNode* node = model_->FindNode(g.ref);
        if(!node)
            continue;
        UiGraphVisualState state = GetNodeVisualState(*node);
        UiGraphNodeStyle style = ResolveNodeStyle(*node, state);
        int si = VisualStateIndex(state);
        StyledMetrics metrics = ScaleNodeMetrics(style.metrics, zoom_);
        Color face = ResolveFace(style.palette.face[si], White());
        Color frame = state == UiGraphVisualState::Selected ? SelectedFrameColor(style)
                                                            : style.palette.frame[si];
        int frame_width = IsNull(frame) ? 0 : max(1, metrics.frame_width);
        if(state == UiGraphVisualState::Selected)
            frame_width = max(frame_width,
                              max(DPI(2), DPI(fround(lod_policy_.selection_outline_width))));

        if(node->shape == UiGraphNodeShape::Circle || node->shape == UiGraphNodeShape::Ellipse)
            w.DrawEllipse(g.surface, face, frame_width, frame);
        else {
            Vector<Point> path = LodIntegerPath(g.hit_path);
            if(path.GetCount() >= 3)
                w.DrawPolygon(path, face, frame_width, frame);
            else
                w.DrawRect(g.surface, face);
        }
        last_painted_node_count_++;
    }
    last_node_surface_paint_usecs_ = max<int64>(0, usecs() - node_started);
    last_node_paint_usecs_ = last_node_surface_paint_usecs_;
}

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

    // Projected size is the authoritative detail boundary. A 20px-wide node is
    // micro regardless of the abstract zoom value; constructing title/content/
    // port-hit geometry for it only creates invisible work and was the dominant
    // intermediate-zoom 10k stall exposed by the P1 live profile.
    const bool overview_geometry = sz.cx < 38 || sz.cy < 26;

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

    // Visible edges still need stable authored-side anchors, but micro geometry
    // deliberately carries no text/content/control/label/hit rectangles.
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
