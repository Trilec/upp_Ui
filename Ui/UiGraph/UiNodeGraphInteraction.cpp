#include <Ui/UiGraph/UiNodeGraph.h>

#include <cmath>

namespace Upp {

namespace {

constexpr int64 kMarqueePreviewMaxCells = 256;

bool HasSelectionModifier(dword flags)
{
    return (flags & (K_CTRL | K_SHIFT)) != 0;
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
    // Win32/U++ can synchronously re-enter CancelMode while releasing capture.
    // Clear ownership first so that callback cannot recursively release again.
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

    if(batch_reset_ || spatial_dirty_) {
        InvalidateSpatialIndex();
        EnsureSpatialIndex();
        last_batch_node_update_count_ = model_ ? model_->GetNodeCount() : 0;
        last_batch_edge_update_count_ = model_ ? model_->GetEdgeCount() : 0;
    }
    else {
        // Node geometry changes also move/re-anchor current incident edges. Add
        // their final IDs once here rather than per intermediate model event.
        if(model_)
            for(int i = 0; i < batch_nodes_.GetCount(); i++) {
                UiGraphNodeRef node{batch_nodes_[i]};
                for(UiGraphEdgeRef edge : model_->GetNodeEdges(node))
                    if(batch_removed_edges_.Find(edge.id) < 0)
                        batch_edges_.FindAdd(edge.id);
            }

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

    batch_reset_ = false;
    batch_nodes_.Clear();
    batch_edges_.Clear();
    batch_removed_nodes_.Clear();
    batch_removed_edges_.Clear();
    batch_flush_serial_++;

    model_revision_ = -1;
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    Refresh();
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
    last_point_ = p;
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    Refresh();
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

    // U++ mouse capture is defined for left/right button interactions only.
    // Middle-button panning therefore remains an in-view interaction and is
    // terminated on MiddleUp or MouseLeave rather than taking Ctrl capture.
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

    // Live candidate preview is intentionally opportunistic. Local rectangles
    // query the retained hash every move; a very large/zoomed-out marquee skips
    // live candidate work and resolves once on mouse-up instead.
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
            QuerySpatial(area, nodes, edges);
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

    // Repaint only the old/new border plus the changed translucent-fill strips.
    // The overlapping interior is visually identical and does not need damage.
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

    bool additive = multi_selection_ && HasSelectionModifier(flags);
    if(!additive) {
        selected_nodes_.Clear();
        selected_edges_.Clear();
    }

    // Large rectangles deliberately skip live candidate preview. Resolve that
    // one final selection here; ordinary rectangles commit the cached preview
    // without a second spatial query.
    if(marquee_preview_deferred_ && model_ && !marquee_.IsEmpty()) {
        EnsureSpatialIndex();
        WorldRect area;
        area.Include(ScreenToWorld(marquee_.TopLeft()));
        area.Include(ScreenToWorld(marquee_.BottomRight()));
        Index<UiGraphId> nodes;
        Index<UiGraphId> edges;
        QuerySpatial(area, nodes, edges);
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
            if(node && node->selectable)
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
    UiGraphPortRef port = HitTestPortSpatial(p);
    if(port.IsValid()) {
        const UiGraphPort* pp = model_ ? model_->FindPort(port) : nullptr;
        if(pp && pp->ProvidesOutput() && editable_) {
            BeginConnection(p, port);
            return;
        }
    }

    UiGraphNodeRef node = HitTestNodeSpatial(p);
    bool additive = multi_selection_ && HasSelectionModifier(flags);
    if(node.IsValid()) {
        const UiGraphNode* n = model_ ? model_->FindNode(node) : nullptr;
        if(n && n->selectable && (!IsNodeSelected(node) || additive))
            SelectNode(node, additive);
        if(editable_ && n && n->movable)
            BeginNodeDrag(p, node);
        return;
    }

    UiGraphEdgeRef edge = HitTestEdgeSpatial(p);
    if(edge.IsValid()) {
        SelectEdge(edge, additive);
        return;
    }

    BeginMarquee(p);
}

void UiNodeGraph::LeftUp(Point p, dword flags)
{
    if(interaction_ == InteractionMode::NodeDrag)
        CommitNodeDrag();
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
    // Active interactions own their geometry/update path. Marquee movement may
    // query a bounded set of spatial cells for transient preview IDs, but must
    // never rebuild the spatial index, prepared geometry, or semantic selection.
    if(interaction_ == InteractionMode::NodeDrag) {
        UpdateNodeDrag(p);
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
        // Style resolvers are allowed to change metrics by state, so rebuild
        // geometry rather than assuming hover is paint-only.
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
