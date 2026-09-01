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
    bool zoom_approximate = false;
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
    const double title_floor = max(0.16, lod.title_zoom - 0.16);
    const double secondary_floor = max(title_floor, lod.secondary_text_zoom - 0.12);
    const double thresholds[] = {
        lod.edge_hide_zoom,
        lod.minimal_edge_zoom,
        lod.edge_simplify_zoom,
        lod.edge_label_zoom,
        lod.arrow_zoom,
        lod.shadow_zoom,
        lod.icon_zoom,
        title_floor,
        secondary_floor,
        lod.title_zoom,
        lod.secondary_text_zoom,
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

    auto pointf = [&](Pointf p) {
        return Pointf((p.x - old_pan.x) * scale + new_pan.x,
                      (p.y - old_pan.y) * scale + new_pan.y);
    };
    auto point = [&](Point p) {
        Pointf q = pointf(Pointf(p.x, p.y));
        return Point(fround(q.x), fround(q.y));
    };
    auto rect = [&](Rect r) {
        if(r.IsEmpty())
            return RectC(0, 0, 0, 0);
        Pointf a = pointf(Pointf(r.left, r.top));
        Pointf b = pointf(Pointf(r.right, r.bottom));
        int l = fround(min(a.x, b.x));
        int t = fround(min(a.y, b.y));
        int rr = fround(max(a.x, b.x));
        int bb = fround(max(a.y, b.y));
        return Rect(l, t, max(l, rr), max(t, bb));
    };

    for(int i = 0; i < nodes.GetCount(); i++) {
        auto& g = nodes[i];
        g.rect = rect(g.rect);
        g.surface = rect(g.surface);
        g.content = rect(g.content);
        g.paint_bounds = rect(g.paint_bounds);
        g.header = rect(g.header);
        g.icon = rect(g.icon);
        g.title = rect(g.title);
        g.subtitle = rect(g.subtitle);
        g.description = rect(g.description);
        g.control = rect(g.control);
        for(Pointf& p : g.hit_path)
            p = pointf(p);
        for(int q = 0; q < g.anchors.GetCount(); q++)
            g.anchors[q] = point(g.anchors[q]);
        for(int q = 0; q < g.port_hits.GetCount(); q++)
            g.port_hits[q] = rect(g.port_hits[q]);
        for(int q = 0; q < g.port_labels.GetCount(); q++)
            g.port_labels[q] = rect(g.port_labels[q]);
        g.compact = g.rect.GetWidth() < 120 || g.rect.GetHeight() < 72;
        g.micro = g.rect.GetWidth() < 38 || g.rect.GetHeight() < 26;
    }

    for(int i = 0; i < edges.GetCount(); i++) {
        auto& g = edges[i];
        for(Point& p : g.points)
            p = point(p);
        g.bounds = rect(g.bounds);
        g.label_point = point(g.label_point);
        g.route_handle = point(g.route_handle);
        g.route_handle_hit = rect(g.route_handle_hit);
    }
}

} // namespace

void UiNodeGraph::InvalidateGeometry()
{
    // Middle-button pan changes only the camera. Preserve the already-prepared
    // scene long enough for PrepareGeometry() below to translate it. Every other
    // caller retains the canonical invalidation semantics.
    if(interaction_ == InteractionMode::Pan && !geometry_dirty_)
        return;
    InvalidateGeometryLegacy();
}

void UiNodeGraph::PrepareGeometry()
{
    if(interaction_ != InteractionMode::Pan) {
        PrepareGeometryLegacy();
        return;
    }

    GraphViewState& state = GraphViewStateFor(this);
    auto sync_exact = [&] {
        if(state.exact_serial == geometry_build_serial_)
            return;
        state.settle.Kill();
        state.exact_serial = geometry_build_serial_;
        state.exact_zoom = geometry_zoom_;
        state.exact_pan = geometry_pan_;
        state.exact_size = geometry_size_;
        state.coverage = GraphViewWorldBounds(geometry_zoom_, geometry_pan_, geometry_size_,
                                              GraphViewPaintMargin(lod_policy_, geometry_zoom_));
        state.zoom_approximate = false;
    };

    if(geometry_dirty_ || !model_ || model_revision_ != model_->GetRevision()
       || geometry_size_ != GetSize()) {
        PrepareGeometryLegacy();
        sync_exact();
        return;
    }

    sync_exact();
    GraphViewBounds needed = GraphViewWorldBounds(zoom_, pan_, GetSize(),
                                                  GraphViewPaintMargin(lod_policy_, zoom_));
    const bool full_scene = node_geometry_.GetCount() == model_->GetNodeCount()
                         && edge_geometry_.GetCount() == model_->GetEdgeCount();
    if(!full_scene && !GraphViewContains(state.coverage, needed)) {
        InvalidateGeometryLegacy();
        PrepareGeometryLegacy();
        sync_exact();
        return;
    }

    if(geometry_pan_ != pan_ || abs(geometry_zoom_ - zoom_) > 1e-12) {
        ProjectPreparedGraphGeometry(node_geometry_, edge_geometry_,
                                     geometry_zoom_, geometry_pan_, zoom_, pan_);
        geometry_zoom_ = zoom_;
        geometry_pan_ = pan_;
        geometry_size_ = GetSize();
        geometry_dirty_ = false;
        last_geometry_prepare_usecs_ = 0;
    }

    if(state.zoom_approximate) {
        Ptr<UiNodeGraph> weak = this;
        const int serial = geometry_build_serial_;
        state.settle.KillSet(140, [weak, serial] {
            if(!weak)
                return;
            UiNodeGraph *graph = ~weak;
            GraphViewState& current = GraphViewStateFor(graph);
            if(!current.zoom_approximate || graph->geometry_build_serial_ != serial)
                return;
            graph->InvalidateGeometryLegacy();
            graph->PrepareGeometryLegacy();
            graph->UpdateAttachedCtrls();
            graph->Refresh();
            current.exact_serial = -1;
            current.zoom_approximate = false;
            graph->WhenViewport();
        });
    }
}

UiNodeGraph& UiNodeGraph::SetZoom(double next_zoom, Point anchor)
{
    // Programmatic zoom with the default centre anchor keeps the established
    // synchronous/exact contract used by tests, Fit/1:1 and host code. A real
    // pointer anchor is the wheel-zoom path and may reuse prepared geometry.
    if(anchor.x < 0 || anchor.y < 0)
        return SetZoomLegacy(next_zoom, anchor);

    const Style& style = GetEffectiveStyle();
    next_zoom = minmax(next_zoom, max(0.01, style.min_zoom), max(style.min_zoom, style.max_zoom));
    if(abs(next_zoom - zoom_) < 1e-9)
        return *this;

    if(!model_ || batch_update_depth_ > 0 || geometry_dirty_
       || model_revision_ != model_->GetRevision() || geometry_size_ != GetSize())
        return SetZoomLegacy(next_zoom, anchor);

    GraphViewState& state = GraphViewStateFor(this);
    if(state.exact_serial != geometry_build_serial_) {
        state.settle.Kill();
        state.exact_serial = geometry_build_serial_;
        state.exact_zoom = geometry_zoom_;
        state.exact_pan = geometry_pan_;
        state.exact_size = geometry_size_;
        state.coverage = GraphViewWorldBounds(geometry_zoom_, geometry_pan_, geometry_size_,
                                              GraphViewPaintMargin(lod_policy_, geometry_zoom_));
        state.zoom_approximate = false;
    }

    Pointf world = ScreenToWorld(anchor);
    Pointf next_pan(anchor.x - world.x * next_zoom,
                    anchor.y - world.y * next_zoom);
    GraphViewBounds needed = GraphViewWorldBounds(next_zoom, next_pan, GetSize(),
                                                  GraphViewPaintMargin(lod_policy_, next_zoom));
    const bool full_scene = node_geometry_.GetCount() == model_->GetNodeCount()
                         && edge_geometry_.GetCount() == model_->GetEdgeCount();
    bool compatible = full_scene || GraphViewContains(state.coverage, needed);
    compatible = compatible && GraphViewLodCompatible(lod_policy_, state.exact_zoom, next_zoom);

    const double cumulative = next_zoom / max(state.exact_zoom, 1e-9);
    compatible = compatible && cumulative >= 0.65 && cumulative <= 1.55;

    if(compatible) {
        const double step_scale = next_zoom / max(geometry_zoom_, 1e-9);
        for(int i = 0; i < node_geometry_.GetCount() && compatible; i++) {
            const NodeGeometry& g = node_geometry_[i];
            int w = max(1, fround(g.rect.GetWidth() * step_scale));
            int h = max(1, fround(g.rect.GetHeight() * step_scale));
            bool compact = w < 120 || h < 72;
            bool micro = w < 38 || h < 26;
            if(compact != g.compact || micro != g.micro)
                compatible = false;
        }
    }

    if(compatible)
        for(int i = 0; i < node_ctrls_.GetCount(); i++) {
            const UiGraphNode* node = model_->FindNode(UiGraphNodeRef{node_ctrls_.GetKey(i)});
            if(!node)
                continue;
            UiGraphNodeStyle node_style = ResolveNodeStyle(*node, GetNodeVisualState(*node));
            double threshold = max(node_style.content_cell_min_zoom, lod_policy_.secondary_text_zoom);
            if(GraphViewCrosses(geometry_zoom_, next_zoom, threshold)) {
                compatible = false;
                break;
            }
        }

    if(!compatible) {
        state.settle.Kill();
        state.zoom_approximate = false;
        UiNodeGraph& out = SetZoomLegacy(next_zoom, anchor);
        state.exact_serial = -1;
        return out;
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
    state.zoom_approximate = true;

    UpdateAttachedCtrls();
    Refresh();
    WhenViewport();

    Ptr<UiNodeGraph> weak = this;
    const int serial = geometry_build_serial_;
    state.settle.KillSet(140, [weak, serial] {
        if(!weak)
            return;
        UiNodeGraph *graph = ~weak;
        GraphViewState& current = GraphViewStateFor(graph);
        if(!current.zoom_approximate || graph->geometry_build_serial_ != serial)
            return;
        graph->InvalidateGeometryLegacy();
        graph->PrepareGeometryLegacy();
        graph->UpdateAttachedCtrls();
        graph->Refresh();
        current.exact_serial = -1;
        current.zoom_approximate = false;
        graph->WhenViewport();
    });
    return *this;
}

} // namespace Upp
