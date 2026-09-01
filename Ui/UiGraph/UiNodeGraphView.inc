namespace Upp {
namespace {

struct GraphViewBounds {
    double left = 0.0;
    double top = 0.0;
    double right = 0.0;
    double bottom = 0.0;
    bool valid = false;
};

struct GraphViewState {
    Ptr<UiNodeGraph> owner;
    TimeCallback settle;
    int exact_serial = -1;
    double exact_zoom = 1.0;
    Pointf exact_pan;
    Size exact_size;
    GraphViewBounds coverage;
    bool approximate_zoom = false;
};

Array<GraphViewState>& GraphViewStates()
{
    static Array<GraphViewState> states;
    return states;
}

GraphViewState& GraphViewStateFor(UiNodeGraph *owner)
{
    Array<GraphViewState>& states = GraphViewStates();
    for(int i = states.GetCount() - 1; i >= 0; --i)
        if(!states[i].owner)
            states.Remove(i);
    for(GraphViewState& state : states)
        if(state.owner == owner)
            return state;
    GraphViewState& state = states.Add();
    state.owner = owner;
    return state;
}

double GraphViewSmoothUnit(double value, double low, double high)
{
    if(high <= low)
        return value >= high ? 1.0 : 0.0;
    double t = minmax((value - low) / (high - low), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

double GraphViewPaintMargin(const UiNodeGraph::LodPolicy& lod, double zoom)
{
    double t = GraphViewSmoothUnit(zoom, lod.minimal_edge_zoom, lod.full_detail_zoom);
    return lod.paint_query_margin_low
         + (lod.paint_query_margin_full - lod.paint_query_margin_low) * t;
}

GraphViewBounds GraphViewWorldBounds(double zoom, Pointf pan, Size size, double screen_margin)
{
    GraphViewBounds out;
    zoom = max(zoom, 1e-9);
    out.left = (-screen_margin - pan.x) / zoom;
    out.top = (-screen_margin - pan.y) / zoom;
    out.right = (size.cx + screen_margin - pan.x) / zoom;
    out.bottom = (size.cy + screen_margin - pan.y) / zoom;
    out.valid = size.cx > 0 && size.cy > 0;
    return out;
}

bool GraphViewContains(const GraphViewBounds& outer, const GraphViewBounds& inner)
{
    if(!outer.valid || !inner.valid)
        return false;
    const double eps = 1e-6;
    return inner.left >= outer.left - eps && inner.top >= outer.top - eps
        && inner.right <= outer.right + eps && inner.bottom <= outer.bottom + eps;
}

bool GraphViewCrosses(double from, double to, double threshold)
{
    return (from < threshold) != (to < threshold);
}

bool GraphViewLodCompatible(const UiNodeGraph::LodPolicy& lod, double exact_zoom, double next_zoom)
{
    const double title_floor = max(0.24, lod.title_zoom - 0.04);
    const double content_floor = max(0.24, lod.secondary_text_zoom - 0.08);
    const double port_floor = max(0.12, lod.port_zoom - 0.04);
    const double thresholds[] = {
        lod.edge_hide_zoom,
        lod.minimal_edge_zoom,
        lod.edge_simplify_zoom,
        lod.edge_label_zoom,
        lod.arrow_zoom,
        lod.shadow_zoom,
        lod.icon_zoom,
        title_floor,
        content_floor,
        lod.title_zoom,
        lod.secondary_text_zoom,
        port_floor,
        lod.port_zoom,
        lod.port_label_zoom,
        lod.route_edit_zoom,
        lod.full_detail_zoom,
    };
    for(double threshold : thresholds)
        if(GraphViewCrosses(exact_zoom, next_zoom, threshold))
            return false;
    return true;
}

template <class NodeMap, class EdgeMap>
void ProjectPreparedGraphGeometry(NodeMap& nodes, EdgeMap& edges,
                                  double old_zoom, Pointf old_pan,
                                  double new_zoom, Pointf new_pan)
{
    const double scale = new_zoom / max(old_zoom, 1e-9);

    auto project_pointf = [&](Pointf p) {
        return Pointf((p.x - old_pan.x) * scale + new_pan.x,
                      (p.y - old_pan.y) * scale + new_pan.y);
    };
    auto project_point = [&](Point p) {
        Pointf q = project_pointf(Pointf(p.x, p.y));
        return Point(fround(q.x), fround(q.y));
    };
    auto project_rect = [&](Rect r) {
        if(r.IsEmpty())
            return RectC(0, 0, 0, 0);
        Pointf a = project_pointf(Pointf(r.left, r.top));
        Pointf b = project_pointf(Pointf(r.right, r.bottom));
        int l = fround(min(a.x, b.x));
        int t = fround(min(a.y, b.y));
        int rr = fround(max(a.x, b.x));
        int bb = fround(max(a.y, b.y));
        return Rect(l, t, max(l, rr), max(t, bb));
    };

    for(int i = 0; i < nodes.GetCount(); i++) {
        auto& g = nodes[i];
        g.rect = project_rect(g.rect);
        g.surface = project_rect(g.surface);
        g.content = project_rect(g.content);
        g.paint_bounds = project_rect(g.paint_bounds);
        g.header = project_rect(g.header);
        g.icon = project_rect(g.icon);
        g.title = project_rect(g.title);
        g.subtitle = project_rect(g.subtitle);
        g.description = project_rect(g.description);
        g.control = project_rect(g.control);
        for(Pointf& q : g.hit_path)
            q = project_pointf(q);
        for(int q = 0; q < g.anchors.GetCount(); q++)
            g.anchors[q] = project_point(g.anchors[q]);
        for(int q = 0; q < g.port_hits.GetCount(); q++)
            g.port_hits[q] = project_rect(g.port_hits[q]);
        for(int q = 0; q < g.port_labels.GetCount(); q++)
            g.port_labels[q] = project_rect(g.port_labels[q]);
        g.compact = g.rect.GetWidth() < 120 || g.rect.GetHeight() < 72;
        g.micro = g.rect.GetWidth() < 38 || g.rect.GetHeight() < 26;
    }

    for(int i = 0; i < edges.GetCount(); i++) {
        auto& g = edges[i];
        for(Point& q : g.points)
            q = project_point(q);
        g.bounds = project_rect(g.bounds);
        g.label_point = project_point(g.label_point);
        g.route_handle = project_point(g.route_handle);
        g.route_handle_hit = project_rect(g.route_handle_hit);
    }
}

} // namespace

bool UiNodeGraph::ProjectLiveView(double next_zoom, Pointf next_pan, bool approximate_zoom)
{
    GraphViewState& state = GraphViewStateFor(this);

    auto cancel_approximation = [&] {
        state.settle.Kill();
        state.approximate_zoom = false;
    };
    auto sync_exact_state = [&] {
        state.settle.Kill();
        state.exact_serial = geometry_build_serial_;
        state.exact_zoom = geometry_zoom_;
        state.exact_pan = geometry_pan_;
        state.exact_size = geometry_size_;
        // RebuildGeometry already prepares candidates using the Graph paint-query
        // margin. Keep that expanded exact footprint as reusable camera coverage.
        state.coverage = GraphViewWorldBounds(geometry_zoom_, geometry_pan_, geometry_size_,
                                              GraphViewPaintMargin(lod_policy_, geometry_zoom_));
        state.approximate_zoom = false;
    };

    if(!model_ || batch_update_depth_ > 0 || geometry_dirty_
       || model_revision_ != model_->GetRevision() || geometry_size_ != GetSize()) {
        cancel_approximation();
        return false;
    }

    if(state.exact_serial != geometry_build_serial_)
        sync_exact_state();

    // The prepared exact scene already includes overscan. Require only the new
    // visible viewport to remain inside it; demanding another full query margin
    // here would consume the overscan twice and force an exact rebuild on every
    // small pan in a partially prepared large graph.
    GraphViewBounds needed = GraphViewWorldBounds(next_zoom, next_pan, GetSize(), 0.0);
    const bool full_scene = node_geometry_.GetCount() == model_->GetNodeCount()
                         && edge_geometry_.GetCount() == model_->GetEdgeCount();
    if(!full_scene && !GraphViewContains(state.coverage, needed)) {
        cancel_approximation();
        return false;
    }

    if(approximate_zoom) {
        if(!GraphViewLodCompatible(lod_policy_, state.exact_zoom, next_zoom)) {
            cancel_approximation();
            return false;
        }

        const double cumulative = next_zoom / max(state.exact_zoom, 1e-9);
        if(cumulative < 0.65 || cumulative > 1.55) {
            cancel_approximation();
            return false;
        }

        const double step_scale = next_zoom / max(geometry_zoom_, 1e-9);
        for(int i = 0; i < node_geometry_.GetCount(); i++) {
            const NodeGeometry& g = node_geometry_[i];
            int width = max(1, fround(g.rect.GetWidth() * step_scale));
            int height = max(1, fround(g.rect.GetHeight() * step_scale));
            bool compact = width < 120 || height < 72;
            bool micro = width < 38 || height < 26;
            if(compact != g.compact || micro != g.micro) {
                cancel_approximation();
                return false;
            }
        }

        for(int i = 0; i < node_ctrls_.GetCount(); i++) {
            const UiGraphNode* node = model_->FindNode(UiGraphNodeRef{node_ctrls_.GetKey(i)});
            if(!node)
                continue;
            UiGraphNodeStyle node_style = ResolveNodeStyle(*node, GetNodeVisualState(*node));
            double threshold = max(node_style.content_cell_min_zoom, lod_policy_.secondary_text_zoom);
            if(GraphViewCrosses(state.exact_zoom, next_zoom, threshold)) {
                cancel_approximation();
                return false;
            }
        }
    }

    ProjectPreparedGraphGeometry(node_geometry_, edge_geometry_,
                                 geometry_zoom_, geometry_pan_, next_zoom, next_pan);
    zoom_ = next_zoom;
    pan_ = next_pan;
    geometry_zoom_ = next_zoom;
    geometry_pan_ = next_pan;
    geometry_size_ = GetSize();
    geometry_dirty_ = false;
    last_geometry_prepare_usecs_ = 0;

    UpdateAttachedCtrls();
    Refresh();
    WhenViewport();

    if(approximate_zoom) {
        state.approximate_zoom = true;
        Ptr<UiNodeGraph> weak = this;
        const int serial = geometry_build_serial_;
        state.settle.KillSet(140, [weak, serial] {
            if(!weak)
                return;
            UiNodeGraph *graph = ~weak;
            GraphViewState& current = GraphViewStateFor(graph);
            if(!current.approximate_zoom || current.exact_serial != serial
               || graph->geometry_build_serial_ != serial) {
                current.approximate_zoom = false;
                return;
            }
            graph->SettleLiveViewProjection();
        });
    }
    return true;
}

void UiNodeGraph::SettleLiveViewProjection()
{
    GraphViewState& state = GraphViewStateFor(this);
    state.settle.Kill();
    if(!state.approximate_zoom)
        return;
    if(state.exact_serial != geometry_build_serial_) {
        state.approximate_zoom = false;
        return;
    }

    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    Refresh();

    state.exact_serial = geometry_build_serial_;
    state.exact_zoom = geometry_zoom_;
    state.exact_pan = geometry_pan_;
    state.exact_size = geometry_size_;
    state.coverage = GraphViewWorldBounds(geometry_zoom_, geometry_pan_, geometry_size_,
                                          GraphViewPaintMargin(lod_policy_, geometry_zoom_));
    state.approximate_zoom = false;
    WhenViewport();
}

void UiNodeGraph::UpdatePan(Point p)
{
    if(interaction_ != InteractionMode::Pan)
        return;

    Pointf next_pan = pan_at_press_ + Pointf(p.x - press_point_.x, p.y - press_point_.y);
    last_point_ = p;
    if(abs(next_pan.x - pan_.x) <= 1e-9 && abs(next_pan.y - pan_.y) <= 1e-9)
        return;

    if(ProjectLiveView(zoom_, next_pan, false))
        return;

    pan_ = next_pan;
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    Refresh();
    WhenViewport();
}

void UiNodeGraph::MouseMove(Point p, dword)
{
    if(interaction_ == InteractionMode::NodeDrag) {
        UpdateNodeDrag(p);
        return;
    }
    if(interaction_ == InteractionMode::EdgeRouteDrag) {
        UpdateEdgeRouteDrag(p);
        return;
    }
    if(interaction_ == InteractionMode::Connect) {
        UpdateConnection(p);
        return;
    }
    if(interaction_ == InteractionMode::Pan) {
        UpdatePan(p);
        return;
    }
    if(interaction_ == InteractionMode::Marquee) {
        UpdateMarquee(p);
        return;
    }

    UiGraphPortRef port = HitTestPortSpatial(p);
    UiGraphNodeRef node = HitTestNodeSpatial(p);
    UiGraphEdgeRef edge = node.IsValid() ? UiGraphEdgeRef() : HitTestEdgeSpatial(p);
    if(port != hot_port_ || node != hot_node_ || edge != hot_edge_) {
        hot_port_ = port;
        hot_node_ = node;
        hot_edge_ = edge;
        InvalidateGeometry();
        PrepareGeometry();
        UpdateAttachedCtrls();
        Refresh();
    }
}

void UiNodeGraph::MouseWheel(Point p, int zdelta, dword)
{
    const Style& style = GetEffectiveStyle();
    double steps = (double)zdelta / 120.0;
    double next_zoom = zoom_ * std::pow(style.zoom_step, steps);
    next_zoom = minmax(next_zoom, max(0.01, style.min_zoom), max(style.min_zoom, style.max_zoom));
    if(abs(next_zoom - zoom_) < 1e-9)
        return;

    Pointf world = ScreenToWorld(p);
    Pointf next_pan(p.x - world.x * next_zoom,
                    p.y - world.y * next_zoom);
    if(ProjectLiveView(next_zoom, next_pan, true))
        return;

    // Crossing an LOD/coverage boundary falls back to the established exact
    // programmatic path. Public SetZoom semantics are therefore unchanged.
    SetZoom(next_zoom, p);
}

} // namespace Upp
