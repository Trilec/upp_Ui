#include <Ui/UiGraph/UiNodeGraph.h>

#include <cmath>

namespace Upp {

namespace {

constexpr int64 kMarqueePreviewMaxCells = 256;

enum class SelectionOperation : byte {
    Replace = 0,
    Add,
    Toggle,
    Subtract,
};

SelectionOperation ResolveSelectionOperation(dword flags, bool multi_selection)
{
    if(!multi_selection)
        return SelectionOperation::Replace;
    // Spatial analogue of familiar desktop selection: Shift adds, Ctrl toggles,
    // Alt subtracts. Alt wins over Ctrl/Shift when modifiers are combined.
    if(flags & K_ALT)
        return SelectionOperation::Subtract;
    if(flags & K_CTRL)
        return SelectionOperation::Toggle;
    if(flags & K_SHIFT)
        return SelectionOperation::Add;
    return SelectionOperation::Replace;
}

void RemoveIndexedId(Index<UiGraphId>& ids, UiGraphId id)
{
    int i = ids.Find(id);
    if(i >= 0)
        ids.Remove(i);
}

} // namespace

void UiNodeGraph::AcquireInteractionCapture()
{
    if(interaction_capture_owned_)
        return;
    SetCapture();
    interaction_capture_owned_ = HasCapture();
}

void UiNodeGraph::ReleaseInteractionCapture()
{
    if(!interaction_capture_owned_)
        return;
    interaction_capture_owned_ = false;
    if(HasCapture())
        ReleaseCapture();
}

UiNodeGraph& UiNodeGraph::BeginBatchUpdate()
{
    if(batch_update_depth_ == 0) {
        batch_reset_ = false;
        batch_nodes_.Clear();
        batch_edges_.Clear();
        batch_removed_nodes_.Clear();
        batch_removed_edges_.Clear();
        last_batch_node_update_count_ = 0;
        last_batch_edge_update_count_ = 0;
    }
    batch_update_depth_++;
    return *this;
}

UiNodeGraph& UiNodeGraph::EndBatchUpdate()
{
    ASSERT(batch_update_depth_ > 0);
    if(batch_update_depth_ <= 0)
        return *this;
    batch_update_depth_--;
    if(batch_update_depth_ == 0)
        FlushBatchModelChanges();
    return *this;
}

void UiNodeGraph::RecordBatchModelChange(const UiGraphChange& change)
{
    if(batch_reset_)
        return;

    switch(change.kind) {
    case UiGraphChangeKind::NodeAdded:
    case UiGraphChangeKind::NodeUpdated:
    case UiGraphChangeKind::PortAdded:
    case UiGraphChangeKind::PortUpdated:
    case UiGraphChangeKind::PortRemoved:
        if(change.node.IsValid()) {
            RemoveIndexedId(batch_removed_nodes_, change.node.id);
            batch_nodes_.FindAdd(change.node.id);
        }
        break;
    case UiGraphChangeKind::NodeRemoved:
        if(change.node.IsValid()) {
            RemoveIndexedId(batch_nodes_, change.node.id);
            batch_removed_nodes_.FindAdd(change.node.id);
        }
        break;
    case UiGraphChangeKind::EdgeAdded:
    case UiGraphChangeKind::EdgeUpdated:
        if(change.edge.IsValid()) {
            RemoveIndexedId(batch_removed_edges_, change.edge.id);
            batch_edges_.FindAdd(change.edge.id);
        }
        break;
    case UiGraphChangeKind::EdgeRemoved:
        if(change.edge.IsValid()) {
            RemoveIndexedId(batch_edges_, change.edge.id);
            batch_removed_edges_.FindAdd(change.edge.id);
        }
        break;
    case UiGraphChangeKind::Reset:
    case UiGraphChangeKind::Cleared:
    default:
        batch_reset_ = true;
        batch_nodes_.Clear();
        batch_edges_.Clear();
        batch_removed_nodes_.Clear();
        batch_removed_edges_.Clear();
        break;
    }
}

void UiNodeGraph::FlushBatchModelChanges()
{
    bool have_changes = batch_reset_ || !batch_nodes_.IsEmpty() || !batch_edges_.IsEmpty()
                        || !batch_removed_nodes_.IsEmpty() || !batch_removed_edges_.IsEmpty();
    if(!have_changes)
        return;

    bool reset_like = batch_reset_;
    bool selection_changed = ReconcileModelState(reset_like);
    Rect damage;
    for(int i = 0; i < batch_nodes_.GetCount(); i++)
        damage |= GetNodeDamage(UiGraphNodeRef{batch_nodes_[i]});
    for(int i = 0; i < batch_removed_nodes_.GetCount(); i++)
        damage |= GetNodeDamage(UiGraphNodeRef{batch_removed_nodes_[i]});
    for(int i = 0; i < batch_edges_.GetCount(); i++)
        damage |= GetEdgeDamage(UiGraphEdgeRef{batch_edges_[i]});
    for(int i = 0; i < batch_removed_edges_.GetCount(); i++)
        damage |= GetEdgeDamage(UiGraphEdgeRef{batch_removed_edges_[i]});

    if(model_)
        for(int i = 0; i < batch_nodes_.GetCount(); i++) {
            UiGraphNodeRef node{batch_nodes_[i]};
            for(UiGraphEdgeRef edge : model_->GetNodeEdges(node))
                if(batch_removed_edges_.Find(edge.id) < 0)
                    batch_edges_.FindAdd(edge.id);
        }

    const int touched = batch_nodes_.GetCount() + batch_removed_nodes_.GetCount()
                      + batch_edges_.GetCount() + batch_removed_edges_.GetCount();
    const bool full_prepare = batch_reset_ || spatial_dirty_
                           || zoom_ < lod_policy_.minimal_edge_zoom
                           || touched > 128;

    if(batch_reset_ || spatial_dirty_) {
        InvalidateSpatialIndex();
        EnsureSpatialIndex();
        last_batch_node_update_count_ = model_ ? model_->GetNodeCount() : 0;
        last_batch_edge_update_count_ = model_ ? model_->GetEdgeCount() : 0;
    }
    else {
        last_batch_node_update_count_ = batch_nodes_.GetCount() + batch_removed_nodes_.GetCount();
        last_batch_edge_update_count_ = batch_edges_.GetCount() + batch_removed_edges_.GetCount();

        for(int i = 0; i < batch_removed_edges_.GetCount(); i++)
            RemoveSpatialEdge(UiGraphEdgeRef{batch_removed_edges_[i]});
        for(int i = 0; i < batch_removed_nodes_.GetCount(); i++)
            RemoveSpatialNode(UiGraphNodeRef{batch_removed_nodes_[i]});
        for(int i = 0; i < batch_nodes_.GetCount(); i++)
            UpdateSpatialNode(UiGraphNodeRef{batch_nodes_[i]});
        for(int i = 0; i < batch_edges_.GetCount(); i++)
            UpdateSpatialEdge(UiGraphEdgeRef{batch_edges_[i]});
    }

    model_revision_ = model_ ? model_->GetRevision() : -1;
    if(full_prepare) {
        InvalidateGeometry();
        PrepareGeometry();
        UpdateAttachedCtrls();
        Refresh();
    }
    else {
        for(int i = 0; i < batch_removed_edges_.GetCount(); i++)
            damage |= RebuildEdge(UiGraphEdgeRef{batch_removed_edges_[i]});
        for(int i = 0; i < batch_removed_nodes_.GetCount(); i++)
            damage |= RebuildNodeAndEdges(UiGraphNodeRef{batch_removed_nodes_[i]});
        for(int i = 0; i < batch_nodes_.GetCount(); i++)
            damage |= RebuildNodeAndEdges(UiGraphNodeRef{batch_nodes_[i]});
        for(int i = 0; i < batch_edges_.GetCount(); i++)
            damage |= RebuildEdge(UiGraphEdgeRef{batch_edges_[i]});
        UpdateAttachedCtrls();
        RefreshDamage(damage.Inflated(DPI(4)));
    }

    batch_reset_ = false;
    batch_nodes_.Clear();
    batch_edges_.Clear();
    batch_removed_nodes_.Clear();
    batch_removed_edges_.Clear();
    batch_flush_serial_++;

    if(selection_changed)
        WhenSelection();
}

void UiNodeGraph::BeginNodeDrag(Point p, UiGraphNodeRef primary)
{
    const UiGraphNode* node = model_ ? model_->FindNode(primary) : nullptr;
    if(!editable_ || !node || !node->movable)
        return;
    interaction_ = InteractionMode::NodeDrag;
    press_point_ = last_point_ = p;
    pressed_node_ = primary;
    drag_start_positions_.Clear();
    drag_preview_positions_.Clear();
    Vector<UiGraphNodeRef> selected = GetSelectedNodes();
    if(selected.IsEmpty())
        selected.Add(primary);
    for(UiGraphNodeRef ref : selected) {
        const UiGraphNode* n = model_->FindNode(ref);
        if(n && n->movable) {
            drag_start_positions_.Add(ref.id, n->position);
            drag_preview_positions_.Add(ref.id, n->position);
        }
    }
    AcquireInteractionCapture();
}

void UiNodeGraph::UpdateNodeDrag(Point p)
{
    if(interaction_ != InteractionMode::NodeDrag)
        return;

    Pointf delta((p.x - press_point_.x) / max(zoom_, 1e-9),
                 (p.y - press_point_.y) / max(zoom_, 1e-9));
    const Style& style = GetStyle();
    double grid = style.grid_size > 0 ? style.grid_size : 1;

    bool changed = false;
    for(int i = 0; i < drag_start_positions_.GetCount(); i++) {
        Pointf next = drag_start_positions_[i] + delta;
        if(style.snap_to_grid) {
            next.x = std::round(next.x / grid) * grid;
            next.y = std::round(next.y / grid) * grid;
        }
        int j = drag_preview_positions_.Find(drag_start_positions_.GetKey(i));
        if(j >= 0 && (abs(drag_preview_positions_[j].x - next.x) > 1e-9 ||
                      abs(drag_preview_positions_[j].y - next.y) > 1e-9)) {
            changed = true;
            break;
        }
    }

    last_point_ = p;
    if(!changed)
        return;

    PrepareGeometry();
    Rect damage;
    Index<UiGraphId> affected_edges;
    int damage_pad = DPI(4);

    for(int i = 0; i < drag_start_positions_.GetCount(); i++) {
        UiGraphNodeRef ref{drag_start_positions_.GetKey(i)};
        const NodeGeometry* old_geometry = FindNodeGeometry(ref);
        if(old_geometry)
            damage |= old_geometry->paint_bounds;

        const UiGraphNode* node = model_ ? model_->FindNode(ref) : nullptr;
        if(node) {
            UiGraphNodeStyle node_style = ResolveNodeStyle(*node, GetNodeVisualState(*node));
            const StyledShadow& shadow = node_style.metrics.shadow;
            if(shadow.enabled && !shadow.inset && shadow.alpha > 0) {
                double z = max(0.01, zoom_);
                int extent = max(0, fround(shadow.distance * z));
                int ox = abs(fround(shadow.offset_x * z));
                int oy = abs(fround(shadow.offset_y * z));
                damage_pad = max(damage_pad, extent + max(ox, oy) + DPI(2));
            }
            for(UiGraphEdgeRef edge : model_->GetNodeEdges(ref))
                affected_edges.FindAdd(edge.id);
        }
    }
    damage_pad = max(damage_pad,
                     DPI(fround(lod_policy_.selection_outline_width)) + DPI(2));

    for(int i = 0; i < affected_edges.GetCount(); i++) {
        const EdgeGeometry* old_geometry = FindEdgeGeometry(UiGraphEdgeRef{affected_edges[i]});
        if(old_geometry)
            damage |= old_geometry->bounds;
    }

    for(int i = 0; i < drag_start_positions_.GetCount(); i++) {
        Pointf next = drag_start_positions_[i] + delta;
        if(style.snap_to_grid) {
            next.x = std::round(next.x / grid) * grid;
            next.y = std::round(next.y / grid) * grid;
        }
        int j = drag_preview_positions_.Find(drag_start_positions_.GetKey(i));
        if(j >= 0)
            drag_preview_positions_[j] = next;
    }

    for(int i = 0; i < drag_preview_positions_.GetCount(); i++) {
        UiGraphNodeRef ref{drag_preview_positions_.GetKey(i)};
        const UiGraphNode* node = model_ ? model_->FindNode(ref) : nullptr;
        if(!node || !node->visible)
            continue;
        NodeGeometry fresh;
        BuildViewNodeGeometry(*node, fresh, false);
        damage |= fresh.paint_bounds;
        int q = node_geometry_.Find(ref.id);
        if(q >= 0)
            node_geometry_[q] = pick(fresh);
        else
            node_geometry_.Add(ref.id, pick(fresh));
    }

    for(int i = 0; i < affected_edges.GetCount(); i++) {
        UiGraphEdgeRef ref{affected_edges[i]};
        int q = edge_geometry_.Find(ref.id);
        const UiGraphEdge* edge = model_ ? model_->FindEdge(ref) : nullptr;
        if(!edge || !edge->visible || !model_->FindPort(edge->source) || !model_->FindPort(edge->target)) {
            if(q >= 0)
                edge_geometry_.Remove(q);
            continue;
        }
        EdgeGeometry fresh;
        BuildEdgeGeometry(*edge, fresh);
        damage |= fresh.bounds;
        if(q >= 0)
            edge_geometry_[q] = pick(fresh);
        else
            edge_geometry_.Add(ref.id, pick(fresh));
    }

    prepared_geometry_revision_++;
    UpdateAttachedCtrls();
    RefreshDamage(damage.Inflated(damage_pad));
}

void UiNodeGraph::CommitNodeDrag()
{
    if(interaction_ != InteractionMode::NodeDrag)
        return;
    UiGraphNodeMoveRequest request;
    request.before = clone(drag_start_positions_);
    request.after = clone(drag_preview_positions_);
    WhenNodeMoveRequest(request);

    bool apply_internal = request.accept && internal_mutation_ && !request.handled && model_;
    if(apply_internal) {
        BeginBatchUpdate();
        for(int i = 0; i < request.after.GetCount(); i++)
            model_->SetNodePosition(UiGraphNodeRef{request.after.GetKey(i)}, request.after[i]);
    }

    drag_start_positions_.Clear();
    drag_preview_positions_.Clear();
    interaction_ = InteractionMode::None;
    pressed_node_ = UiGraphNodeRef();
    ReleaseInteractionCapture();

    if(apply_internal)
        EndBatchUpdate();
    else {
        InvalidateGeometry();
        PrepareGeometry();
        UpdateAttachedCtrls();
        Refresh();
    }
}

void UiNodeGraph::CancelNodeDrag()
{
    drag_start_positions_.Clear();
    drag_preview_positions_.Clear();
    pressed_node_ = UiGraphNodeRef();
    if(interaction_ == InteractionMode::NodeDrag)
        interaction_ = InteractionMode::None;
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    Refresh();
}

Pointf UiNodeGraph::NormalizeRouteDragPoint(const UiGraphEdge& edge,
                                            UiGraphRouteStyle route,
                                            const UiGraphEdgeStyle& edge_style,
                                            Pointf next,
                                            Pointf previous) const
{
    UiGraphPortSide source_side = UiGraphPortSide::Right;
    UiGraphPortSide target_side = UiGraphPortSide::Left;
    Point source_px = GetPortAnchor(edge.source, &source_side);
    Point target_px = GetPortAnchor(edge.target, &target_side);
    Pointf source = ScreenToWorld(source_px);
    Pointf target = ScreenToWorld(target_px);
    double distance = std::sqrt((target.x - source.x) * (target.x - source.x)
                              + (target.y - source.y) * (target.y - source.y));

    if(route == UiGraphRouteStyle::Straight) {
        double tolerance = max(2.0 / max(zoom_, 1e-9),
                               min(8.0 / max(zoom_, 1e-9), distance * 0.04));
        if(DistanceToSegment(next, source, target) <= tolerance) {
            Pointf ab = target - source;
            double len2 = ab.x * ab.x + ab.y * ab.y;
            if(len2 > 1e-9) {
                double t = ((next.x - source.x) * ab.x + (next.y - source.y) * ab.y) / len2;
                t = minmax(t, 0.0, 1.0);
                next = source + ab * t;
            }
        }
        return next;
    }

    if(route == UiGraphRouteStyle::Bezier) {
        // The authored two-half cubic keeps minimum endpoint/midpoint controls.
        // Reserve enough *screen-space* room for those controls before the
        // waypoint reaches a facing port; otherwise the controls can cross and
        // the visible curve briefly folds even though the waypoint itself is
        // technically still between the endpoints.
        Pointf source_dir = SideVector(source_side);
        Pointf target_dir = SideVector(target_side);
        auto dot = [](Pointf a, Pointf b) { return a.x * b.x + a.y * b.y; };
        double margin = min(16.0 / max(zoom_, 1e-9), max(0.0, distance * 0.20));
        if(dot(target - source, source_dir) > margin) {
            double forward = dot(next - source, source_dir);
            if(forward < margin)
                next += source_dir * (margin - forward);
        }
        if(dot(source - target, target_dir) > margin) {
            double forward = dot(next - target, target_dir);
            if(forward < margin)
                next += target_dir * (margin - forward);
        }
        return next;
    }

    if(route == UiGraphRouteStyle::Orthogonal) {
        bool source_horizontal = source_side == UiGraphPortSide::Left ||
                                 source_side == UiGraphPortSide::Right;
        bool target_horizontal = target_side == UiGraphPortSide::Left ||
                                 target_side == UiGraphPortSide::Right;
        // The current one-handle orthogonal contract is a corridor editor for
        // equal-orientation endpoints. Mixed orientation already has a unique
        // elbow and does not gain a stable degree of freedom from one midpoint.
        if(source_horizontal != target_horizontal)
            return previous;

        Pointf source_screen(source_px.x, source_px.y);
        Pointf target_screen(target_px.x, target_px.y);
        double lead = max(0.0, edge_style.orthogonal_lead * zoom_);
        Pointf a = source_screen + SideVector(source_side) * lead;
        Pointf b = target_screen + SideVector(target_side) * lead;
        Pointf middle((a.x + b.x) * 0.5, (a.y + b.y) * 0.5);
        Point candidate_px = WorldToScreen(next);
        Point previous_px = WorldToScreen(previous);
        double dx = abs(candidate_px.x - middle.x);
        double dy = abs(candidate_px.y - middle.y);
        const double lock_epsilon = 1.5;
        const double hysteresis = 6.0;
        bool previous_x_control = abs(previous_px.y - middle.y) <= lock_epsilon;
        bool previous_y_control = abs(previous_px.x - middle.x) <= lock_epsilon;
        bool x_control;
        if(previous_x_control && dx + hysteresis >= dy)
            x_control = true;
        else if(previous_y_control && dy + hysteresis >= dx)
            x_control = false;
        else
            x_control = dx >= dy;

        Point canonical = candidate_px;
        if(x_control)
            canonical.y = fround(middle.y);
        else
            canonical.x = fround(middle.x);
        return ScreenToWorld(canonical);
    }

    return next;
}

void UiNodeGraph::BeginEdgeRouteDrag(Point p, UiGraphEdgeRef edge_ref)
{
    const UiGraphEdge* edge = model_ ? model_->FindEdge(edge_ref) : nullptr;
    const EdgeGeometry* geometry = FindEdgeGeometry(edge_ref);
    if(!editable_ || !edge || !edge->visible || !edge->selectable || !geometry ||
       geometry->route_handle_hit.IsEmpty() || zoom_ < lod_policy_.route_edit_zoom)
        return;

    UiGraphEdgeStyle style = ResolveEdgeStyle(*edge, GetEdgeVisualState(*edge));
    UiGraphRouteStyle route = edge->route == UiGraphRouteStyle::Inherit ? style.route : edge->route;
    if(route == UiGraphRouteStyle::Custom)
        return;

    if(route == UiGraphRouteStyle::Orthogonal) {
        UiGraphPortSide source_side = UiGraphPortSide::Right;
        UiGraphPortSide target_side = UiGraphPortSide::Left;
        GetPortAnchor(edge->source, &source_side);
        GetPortAnchor(edge->target, &target_side);
        bool source_horizontal = source_side == UiGraphPortSide::Left ||
                                 source_side == UiGraphPortSide::Right;
        bool target_horizontal = target_side == UiGraphPortSide::Left ||
                                 target_side == UiGraphPortSide::Right;
        if(source_horizontal != target_horizontal)
            return;
    }

    interaction_ = InteractionMode::EdgeRouteDrag;
    route_edge_ = edge_ref;
    route_before_waypoints_ = clone(edge->waypoints);
    route_preview_waypoints_ = clone(edge->waypoints);
    if(route_preview_waypoints_.IsEmpty()) {
        route_preview_waypoints_.Add(ScreenToWorld(geometry->route_handle));
        route_waypoint_index_ = 0;
    }
    else
        route_waypoint_index_ = route_preview_waypoints_.GetCount() / 2;

    if(route_preview_waypoints_.GetCount() == 1) {
        Pointf previous = route_preview_waypoints_[0];
        route_preview_waypoints_[0] = NormalizeRouteDragPoint(*edge, route, style,
                                                              previous, previous);
        if(abs(previous.x - route_preview_waypoints_[0].x) > 1e-9 ||
           abs(previous.y - route_preview_waypoints_[0].y) > 1e-9)
            RebuildEdge(edge_ref);
    }

    press_point_ = last_point_ = p;
    AcquireInteractionCapture();
}

void UiNodeGraph::UpdateEdgeRouteDrag(Point p)
{
    if(interaction_ != InteractionMode::EdgeRouteDrag || !route_edge_.IsValid() ||
       route_waypoint_index_ < 0 || route_waypoint_index_ >= route_preview_waypoints_.GetCount())
        return;

    const UiGraphEdge* edge = model_ ? model_->FindEdge(route_edge_) : nullptr;
    if(!edge)
        return;
    UiGraphEdgeStyle edge_style = ResolveEdgeStyle(*edge, GetEdgeVisualState(*edge));
    UiGraphRouteStyle route = edge->route == UiGraphRouteStyle::Inherit ? edge_style.route : edge->route;

    Pointf next = ScreenToWorld(p);
    const Style& style = GetStyle();
    if(style.snap_to_grid && style.grid_size > 0) {
        double grid = style.grid_size;
        next.x = std::round(next.x / grid) * grid;
        next.y = std::round(next.y / grid) * grid;
    }
    Pointf old = route_preview_waypoints_[route_waypoint_index_];
    if(route_preview_waypoints_.GetCount() == 1)
        next = NormalizeRouteDragPoint(*edge, route, edge_style, next, old);
    last_point_ = p;
    if(abs(old.x - next.x) <= 1e-9 && abs(old.y - next.y) <= 1e-9)
        return;

    Rect damage = GetEdgeDamage(route_edge_);
    route_preview_waypoints_[route_waypoint_index_] = next;
    damage |= RebuildEdge(route_edge_);
    RefreshDamage(damage.Inflated(DPI(9)));
}

void UiNodeGraph::CommitEdgeRouteDrag()
{
    if(interaction_ != InteractionMode::EdgeRouteDrag)
        return;

    UiGraphEdgeRef ref = route_edge_;
    const UiGraphEdge* edge = model_ ? model_->FindEdge(ref) : nullptr;
    if(!edge || last_point_ == press_point_) {
        CancelEdgeRouteDrag();
        return;
    }

    UiGraphEdgeStyle style = ResolveEdgeStyle(*edge, GetEdgeVisualState(*edge));
    UiGraphEdgeRouteRequest request;
    request.edge = ref;
    request.route = edge->route == UiGraphRouteStyle::Inherit ? style.route : edge->route;
    request.before = clone(route_before_waypoints_);
    request.after = clone(route_preview_waypoints_);
    WhenEdgeRouteRequest(request);

    bool apply_internal = request.accept && internal_mutation_ && !request.handled && model_;
    Rect damage = GetEdgeDamage(ref);

    route_edge_ = UiGraphEdgeRef();
    route_waypoint_index_ = -1;
    route_before_waypoints_.Clear();
    route_preview_waypoints_.Clear();
    interaction_ = InteractionMode::None;
    ReleaseInteractionCapture();

    if(apply_internal) {
        const UiGraphEdge* current = model_->FindEdge(ref);
        if(current) {
            UiGraphEdge updated = *current;
            updated.waypoints = clone(request.after);
            model_->UpdateEdge(ref, updated);
        }
    }
    else {
        damage |= RebuildEdge(ref);
        RefreshDamage(damage.Inflated(DPI(9)));
    }
}

void UiNodeGraph::CancelEdgeRouteDrag()
{
    UiGraphEdgeRef ref = route_edge_;
    Rect damage = GetEdgeDamage(ref);
    route_edge_ = UiGraphEdgeRef();
    route_waypoint_index_ = -1;
    route_before_waypoints_.Clear();
    route_preview_waypoints_.Clear();
    if(interaction_ == InteractionMode::EdgeRouteDrag)
        interaction_ = InteractionMode::None;
    ReleaseInteractionCapture();
    if(ref.IsValid()) {
        damage |= RebuildEdge(ref);
        RefreshDamage(damage.Inflated(DPI(9)));
    }
}

void UiNodeGraph::BeginConnection(Point p, const UiGraphPortRef& source)
{
    const UiGraphPort* port = model_ ? model_->FindPort(source) : nullptr;
    if(!editable_ || !port || !port->ProvidesOutput())
        return;
    interaction_ = InteractionMode::Connect;
    connection_source_ = source;
    connection_target_ = UiGraphPortRef();
    connection_decision_ = UiGraphConnectionDecision();
    press_point_ = last_point_ = p;
    AcquireInteractionCapture();
    Refresh();
}

void UiNodeGraph::UpdateConnection(Point p)
{
    if(interaction_ != InteractionMode::Connect)
        return;
    last_point_ = p;
    UiGraphPortRef target = HitTestPortSpatial(p);
    if(target != connection_target_) {
        connection_target_ = target;
        connection_decision_ = target.IsValid() && model_
                               ? model_->ValidateConnection(connection_source_, target)
                               : UiGraphConnectionDecision();
    }
    Refresh();
}

void UiNodeGraph::CommitConnection(Point p)
{
    if(interaction_ != InteractionMode::Connect)
        return;
    UpdateConnection(p);
    if(connection_target_.IsValid()) {
        UiGraphConnectionRequest request;
        request.source = connection_source_;
        request.target = connection_target_;
        request.decision = connection_decision_;
        request.accept = connection_decision_.IsAllowed();
        WhenConnectionRequest(request);
        if(request.accept && internal_mutation_ && !request.handled && model_)
            model_->Connect(request.source, request.target, UiGraphRouteStyle::Inherit);
    }
    CancelConnection();
}

void UiNodeGraph::CancelConnection()
{
    connection_source_ = UiGraphPortRef();
    connection_target_ = UiGraphPortRef();
    connection_decision_ = UiGraphConnectionDecision();
    if(interaction_ == InteractionMode::Connect)
        interaction_ = InteractionMode::None;
    ReleaseInteractionCapture();
    Refresh();
}

void UiNodeGraph::BeginPan(Point p)
{
    interaction_ = InteractionMode::Pan;
    press_point_ = last_point_ = p;
    pan_at_press_ = pan_;
}

void UiNodeGraph::UpdatePan(Point p)
{
    if(interaction_ != InteractionMode::Pan)
        return;
    pan_ = pan_at_press_ + Pointf(p.x - press_point_.x, p.y - press_point_.y);
    last_point_ = p;
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    Refresh();
    WhenViewport();
}

void UiNodeGraph::EndPan()
{
    if(interaction_ == InteractionMode::Pan)
        interaction_ = InteractionMode::None;
}

void UiNodeGraph::BeginMarquee(Point p)
{
    interaction_ = InteractionMode::Marquee;
    press_point_ = last_point_ = p;
    marquee_ = Rect(p, p);
    marquee_preview_nodes_.Clear();
    marquee_preview_deferred_ = false;
    last_marquee_candidate_count_ = 0;
    AcquireInteractionCapture();
    RefreshDamage(RectC(p.x - DPI(3), p.y - DPI(3), DPI(7), DPI(7)));
}

void UiNodeGraph::UpdateMarquee(Point p)
{
    if(interaction_ != InteractionMode::Marquee)
        return;

    Rect old = marquee_;
    last_point_ = p;
    marquee_ = Rect(min(press_point_.x, p.x), min(press_point_.y, p.y),
                    max(press_point_.x, p.x) + 1, max(press_point_.y, p.y) + 1);

    Index<UiGraphId> next_preview;
    last_marquee_candidate_count_ = 0;
    marquee_preview_deferred_ = false;

    if(model_ && !marquee_.IsEmpty()) {
        EnsureSpatialIndex();
        WorldRect area;
        area.Include(ScreenToWorld(marquee_.TopLeft()));
        area.Include(ScreenToWorld(marquee_.BottomRight()));
        int x0, y0, x1, y1;
        SpatialCellRange(area, x0, y0, x1, y1);
        int64 cells = x1 >= x0 && y1 >= y0
                    ? (int64)(x1 - x0 + 1) * (int64)(y1 - y0 + 1)
                    : 0;
        if(cells > 0 && cells <= kMarqueePreviewMaxCells) {
            Index<UiGraphId> nodes;
            Index<UiGraphId> edges;
            QuerySpatial(area, nodes, edges, SPATIAL_QUERY_NODES);
            last_marquee_candidate_count_ = nodes.GetCount();
            for(int i = 0; i < nodes.GetCount(); i++) {
                UiGraphNodeRef ref{nodes[i]};
                const UiGraphNode* node = model_->FindNode(ref);
                if(node && node->selectable)
                    next_preview.FindAdd(ref.id);
            }
        }
        else if(cells > kMarqueePreviewMaxCells)
            marquee_preview_deferred_ = true;
    }

    auto refresh_border = [&](Rect r) {
        if(r.IsEmpty())
            return;
        int b = DPI(3);
        RefreshDamage(Rect(r.left - b, r.top - b, r.right + b, r.top + b + 1));
        RefreshDamage(Rect(r.left - b, r.bottom - b - 1, r.right + b, r.bottom + b));
        RefreshDamage(Rect(r.left - b, r.top, r.left + b + 1, r.bottom));
        RefreshDamage(Rect(r.right - b - 1, r.top, r.right + b, r.bottom));
    };
    auto refresh_difference = [&](Rect a, Rect b) {
        if(a.IsEmpty())
            return;
        Rect i = a & b;
        if(i.IsEmpty()) {
            RefreshDamage(a.Inflated(DPI(1)));
            return;
        }
        if(a.top < i.top) RefreshDamage(Rect(a.left, a.top, a.right, i.top).Inflated(DPI(1)));
        if(i.bottom < a.bottom) RefreshDamage(Rect(a.left, i.bottom, a.right, a.bottom).Inflated(DPI(1)));
        if(a.left < i.left) RefreshDamage(Rect(a.left, i.top, i.left, i.bottom).Inflated(DPI(1)));
        if(i.right < a.right) RefreshDamage(Rect(i.right, i.top, a.right, i.bottom).Inflated(DPI(1)));
    };
    refresh_difference(old, marquee_);
    refresh_difference(marquee_, old);
    refresh_border(old);
    refresh_border(marquee_);

    for(int i = 0; i < marquee_preview_nodes_.GetCount(); i++) {
        UiGraphId id = marquee_preview_nodes_[i];
        if(next_preview.Find(id) < 0) {
            const NodeGeometry* g = FindNodeGeometry(UiGraphNodeRef{id});
            if(g)
                RefreshDamage(g->paint_bounds.Inflated(DPI(3)));
        }
    }
    for(int i = 0; i < next_preview.GetCount(); i++) {
        UiGraphId id = next_preview[i];
        if(marquee_preview_nodes_.Find(id) < 0) {
            const NodeGeometry* g = FindNodeGeometry(UiGraphNodeRef{id});
            if(g)
                RefreshDamage(g->paint_bounds.Inflated(DPI(3)));
        }
    }

    marquee_preview_nodes_.Clear();
    for(int i = 0; i < next_preview.GetCount(); i++)
        marquee_preview_nodes_.Add(next_preview[i]);
}

void UiNodeGraph::CommitMarquee(dword flags)
{
    if(interaction_ != InteractionMode::Marquee)
        return;

    SelectionOperation op = ResolveSelectionOperation(flags, multi_selection_);
    if(op == SelectionOperation::Replace) {
        selected_nodes_.Clear();
        selected_edges_.Clear();
    }

    if(marquee_preview_deferred_ && model_ && !marquee_.IsEmpty()) {
        EnsureSpatialIndex();
        WorldRect area;
        area.Include(ScreenToWorld(marquee_.TopLeft()));
        area.Include(ScreenToWorld(marquee_.BottomRight()));
        Index<UiGraphId> nodes;
        Index<UiGraphId> edges;
        QuerySpatial(area, nodes, edges, SPATIAL_QUERY_NODES);
        marquee_preview_nodes_.Clear();
        for(int i = 0; i < nodes.GetCount(); i++) {
            UiGraphNodeRef ref{nodes[i]};
            const UiGraphNode* node = model_->FindNode(ref);
            if(node && node->selectable)
                marquee_preview_nodes_.FindAdd(ref.id);
        }
    }

    if(model_)
        for(int i = 0; i < marquee_preview_nodes_.GetCount(); i++) {
            UiGraphNodeRef ref{marquee_preview_nodes_[i]};
            const UiGraphNode* node = model_->FindNode(ref);
            if(!node || !node->selectable)
                continue;
            int q = selected_nodes_.Find(ref.id);
            if(op == SelectionOperation::Subtract) {
                if(q >= 0) selected_nodes_.Remove(q);
            }
            else if(op == SelectionOperation::Toggle) {
                if(q >= 0) selected_nodes_.Remove(q);
                else selected_nodes_.Add(ref.id);
            }
            else
                selected_nodes_.FindAdd(ref.id);
        }

    marquee_preview_nodes_.Clear();
    marquee_preview_deferred_ = false;
    last_marquee_candidate_count_ = 0;
    marquee_ = Rect(0, 0, 0, 0);
    interaction_ = InteractionMode::None;
    ReleaseInteractionCapture();
    NotifySelection();
}

void UiNodeGraph::DeleteSelection()
{
    if(!editable_ || !model_)
        return;
    UiGraphDeleteRequest request;
    request.nodes = GetSelectedNodes();
    request.edges = GetSelectedEdges();
    if(request.nodes.IsEmpty() && request.edges.IsEmpty())
        return;
    WhenDeleteRequest(request);

    bool apply_internal = request.accept && internal_mutation_ && !request.handled;
    if(apply_internal) {
        BeginBatchUpdate();
        for(UiGraphEdgeRef edge : request.edges)
            model_->RemoveEdge(edge);
        for(UiGraphNodeRef node : request.nodes)
            model_->RemoveNode(node);
    }

    selected_nodes_.Clear();
    selected_edges_.Clear();
    if(apply_internal) {
        EndBatchUpdate();
        WhenSelection();
    }
    else
        NotifySelection();
}

void UiNodeGraph::LeftDown(Point p, dword flags)
{
    SetFocus();

    UiGraphEdgeRef route_handle = HitTestEdgeRouteHandle(p);
    if(route_handle.IsValid() && editable_) {
        BeginEdgeRouteDrag(p, route_handle);
        if(interaction_ == InteractionMode::EdgeRouteDrag)
            return;
    }

    UiGraphPortRef port = HitTestPortSpatial(p);
    if(port.IsValid()) {
        const UiGraphPort* pp = model_ ? model_->FindPort(port) : nullptr;
        if(pp && pp->ProvidesOutput() && editable_) {
            BeginConnection(p, port);
            return;
        }
    }

    SelectionOperation op = ResolveSelectionOperation(flags, multi_selection_);
    UiGraphNodeRef node = HitTestNodeSpatial(p);
    if(node.IsValid()) {
        const UiGraphNode* n = model_ ? model_->FindNode(node) : nullptr;
        if(n && n->selectable) {
            if(op == SelectionOperation::Replace) {
                if(!IsNodeSelected(node))
                    SelectNode(node, false);
                // When an already-selected member of a multi-selection is
                // movable, preserve the group during mouse-down so a drag can
                // move it as a unit. If no drag is possible there is no reason
                // to defer the familiar desktop replace-selection behavior.
                else if((!editable_ || !n->movable) && selected_nodes_.GetCount() > 1)
                    SelectNode(node, false);
            }
            else if(op == SelectionOperation::Add) {
                if(!IsNodeSelected(node))
                    SelectNode(node, true);
            }
            else if(op == SelectionOperation::Toggle)
                SelectNode(node, true);
            else if(op == SelectionOperation::Subtract && IsNodeSelected(node))
                SelectNode(node, true);
        }
        if(editable_ && n && n->movable && IsNodeSelected(node))
            BeginNodeDrag(p, node);
        return;
    }

    UiGraphEdgeRef edge = HitTestEdgeSpatial(p);
    if(edge.IsValid()) {
        if(op == SelectionOperation::Replace) {
            if(!IsEdgeSelected(edge))
                SelectEdge(edge, false);
        }
        else if(op == SelectionOperation::Add) {
            if(!IsEdgeSelected(edge))
                SelectEdge(edge, true);
        }
        else if(op == SelectionOperation::Toggle)
            SelectEdge(edge, true);
        else if(op == SelectionOperation::Subtract && IsEdgeSelected(edge))
            SelectEdge(edge, true);
        return;
    }

    BeginMarquee(p);
}

void UiNodeGraph::LeftUp(Point p, dword flags)
{
    if(interaction_ == InteractionMode::NodeDrag) {
        // A plain click on one member of an existing multi-selection must
        // collapse to that object, but mouse-down cannot do so because the same
        // gesture might become a group drag. Resolve the distinction here: only
        // an effectively stationary click replaces; any real drag keeps/moves
        // the complete selected group.
        UiGraphNodeRef clicked = pressed_node_;
        int jitter = DPI(3);
        int dx = p.x - press_point_.x;
        int dy = p.y - press_point_.y;
        bool click_only = dx * dx + dy * dy <= jitter * jitter;
        bool replace = ResolveSelectionOperation(flags, multi_selection_) == SelectionOperation::Replace;
        bool collapse = replace && click_only && clicked.IsValid()
                     && selected_nodes_.GetCount() > 1;
        if(collapse) {
            CancelNodeDrag();
            SelectNode(clicked, false);
        }
        else
            CommitNodeDrag();
    }
    else if(interaction_ == InteractionMode::EdgeRouteDrag) {
        UpdateEdgeRouteDrag(p);
        CommitEdgeRouteDrag();
    }
    else if(interaction_ == InteractionMode::Connect)
        CommitConnection(p);
    else if(interaction_ == InteractionMode::Marquee) {
        UpdateMarquee(p);
        CommitMarquee(flags);
    }
}

void UiNodeGraph::LeftDouble(Point p, dword)
{
    UiGraphNodeRef node = HitTestNodeSpatial(p);
    if(node.IsValid()) {
        SelectNode(node, false);
        WhenNodeAction(node);
    }
}

void UiNodeGraph::MiddleDown(Point p, dword)
{
    BeginPan(p);
}

void UiNodeGraph::MiddleUp(Point, dword)
{
    EndPan();
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

void UiNodeGraph::MouseLeave()
{
    if(interaction_ == InteractionMode::Pan) {
        EndPan();
        return;
    }
    if(interaction_ == InteractionMode::None &&
       (hot_node_.IsValid() || hot_edge_.IsValid() || hot_port_.IsValid())) {
        hot_node_ = UiGraphNodeRef();
        hot_edge_ = UiGraphEdgeRef();
        hot_port_ = UiGraphPortRef();
        InvalidateGeometry();
        PrepareGeometry();
        UpdateAttachedCtrls();
        Refresh();
    }
}

void UiNodeGraph::MouseWheel(Point p, int zdelta, dword)
{
    double steps = (double)zdelta / 120.0;
    SetZoom(zoom_ * std::pow(GetStyle().zoom_step, steps), p);
}

bool UiNodeGraph::Key(dword key, int count)
{
    if(key == K_ESCAPE) {
        CancelMode();
        return true;
    }
    if(key == K_DELETE || key == K_BACKSPACE) {
        DeleteSelection();
        return true;
    }
    if(key == (K_CTRL | K_A)) {
        selected_nodes_.Clear();
        selected_edges_.Clear();
        if(model_)
            for(int i = 0; i < model_->GetNodeCount(); i++)
                if(model_->GetNode(i).visible && model_->GetNode(i).selectable)
                    selected_nodes_.FindAdd(model_->GetNode(i).ref.id);
        NotifySelection();
        return true;
    }
    if(key == 'F' || key == 'f') {
        FitToGraph(false);
        return true;
    }
    if(key == '0') {
        ResetView();
        return true;
    }
    if(key == K_ADD || key == '+') {
        SetZoom(zoom_ * GetStyle().zoom_step);
        return true;
    }
    if(key == K_SUBTRACT || key == '-') {
        SetZoom(zoom_ / GetStyle().zoom_step);
        return true;
    }

    Pointf delta;
    if(key == K_LEFT) delta.x = -1;
    else if(key == K_RIGHT) delta.x = 1;
    else if(key == K_UP) delta.y = -1;
    else if(key == K_DOWN) delta.y = 1;
    if((delta.x != 0 || delta.y != 0) && editable_ && model_) {
        double step = GetStyle().snap_to_grid ? max(1, GetStyle().grid_size) : 1;
        delta = delta * (step * max(1, count));
        UiGraphNodeMoveRequest request;
        for(UiGraphNodeRef ref : GetSelectedNodes()) {
            const UiGraphNode* node = model_->FindNode(ref);
            if(node && node->movable) {
                request.before.Add(ref.id, node->position);
                request.after.Add(ref.id, node->position + delta);
            }
        }
        if(!request.after.IsEmpty()) {
            WhenNodeMoveRequest(request);
            bool apply_internal = request.accept && internal_mutation_ && !request.handled;
            if(apply_internal) {
                BeginBatchUpdate();
                for(int i = 0; i < request.after.GetCount(); i++)
                    model_->SetNodePosition(UiGraphNodeRef{request.after.GetKey(i)}, request.after[i]);
                EndBatchUpdate();
            }
            else {
                InvalidateGeometry();
                PrepareGeometry();
                UpdateAttachedCtrls();
                Refresh();
            }
        }
        return true;
    }
    return Ctrl::Key(key, count);
}

void UiNodeGraph::CancelMode()
{
    if(interaction_ == InteractionMode::NodeDrag)
        CancelNodeDrag();
    else if(interaction_ == InteractionMode::EdgeRouteDrag)
        CancelEdgeRouteDrag();
    else if(interaction_ == InteractionMode::Connect)
        CancelConnection();
    else if(interaction_ == InteractionMode::Pan)
        EndPan();
    else if(interaction_ == InteractionMode::Marquee) {
        Rect old = marquee_;
        for(int i = 0; i < marquee_preview_nodes_.GetCount(); i++) {
            const NodeGeometry* g = FindNodeGeometry(UiGraphNodeRef{marquee_preview_nodes_[i]});
            if(g)
                RefreshDamage(g->paint_bounds.Inflated(DPI(3)));
        }
        marquee_preview_nodes_.Clear();
        marquee_preview_deferred_ = false;
        last_marquee_candidate_count_ = 0;
        marquee_ = Rect(0, 0, 0, 0);
        interaction_ = InteractionMode::None;
        RefreshDamage(old.Inflated(DPI(3)));
    }
    ReleaseInteractionCapture();
    Ctrl::CancelMode();
}

void UiNodeGraph::GotFocus()
{
    Refresh();
}

void UiNodeGraph::LostFocus()
{
    if(interaction_ != InteractionMode::None || interaction_capture_owned_)
        CancelMode();
    Refresh();
}

} // namespace Upp
