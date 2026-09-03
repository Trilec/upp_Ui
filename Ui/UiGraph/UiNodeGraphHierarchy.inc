namespace Upp {

UiGraphScopeRef UiNodeGraph::GetScope() const
{
    if(!model_ || scope_model_ != model_ || !model_->ScopeExists(active_scope_))
        return UiGraphModel::RootScope();
    return active_scope_;
}

bool UiNodeGraph::CanExitScope() const
{
    return model_ && GetScope() != UiGraphModel::RootScope()
        && model_->GetParentScope(GetScope()).IsValid();
}

Vector<UiGraphNodeRef> UiNodeGraph::GetScopePath() const
{
    Vector<UiGraphNodeRef> reverse;
    if(!model_)
        return reverse;
    UiGraphScopeRef scope = GetScope();
    Index<UiGraphId> seen;
    while(scope.IsValid() && scope != UiGraphModel::RootScope()) {
        if(seen.Find(scope.id) >= 0)
            break;
        seen.Add(scope.id);
        UiGraphNodeRef owner = model_->GetOwningGroupNode(scope);
        if(!owner.IsValid())
            break;
        reverse.Add(owner);
        scope = model_->GetParentScope(scope);
    }
    Vector<UiGraphNodeRef> out;
    out.Reserve(reverse.GetCount());
    for(int i = reverse.GetCount() - 1; i >= 0; --i)
        out.Add(reverse[i]);
    return out;
}

UiNodeGraph& UiNodeGraph::SetScope(UiGraphScopeRef scope)
{
    if(!model_ || !model_->ScopeExists(scope))
        return *this;

    UiGraphScopeRef before = GetScope();
    if(scope_model_ == model_ && before == scope)
        return *this;

    if(interaction_ != InteractionMode::None || interaction_capture_owned_)
        CancelMode();

    bool had_selection = !selected_nodes_.IsEmpty() || !selected_edges_.IsEmpty();
    selected_nodes_.Clear();
    selected_edges_.Clear();
    hot_node_ = UiGraphNodeRef();
    hot_edge_ = UiGraphEdgeRef();
    hot_port_ = UiGraphPortRef();
    marquee_preview_nodes_.Clear();
    marquee_preview_deferred_ = false;

    scope_model_ = model_;
    active_scope_ = scope;
    InvalidateSpatialIndex();
    EnsureSpatialIndex();
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    Refresh();

    if(had_selection)
        WhenSelection();
    WhenScopeChanged(scope);
    return *this;
}

bool UiNodeGraph::EnterSubgraph(UiGraphNodeRef group_node)
{
    if(!model_ || !model_->IsSubgraphNode(group_node)
       || model_->GetNodeScope(group_node) != GetScope())
        return false;
    UiGraphScopeRef child = model_->GetChildScope(group_node);
    if(!child.IsValid() || !model_->ScopeExists(child))
        return false;
    SetScope(child);
    FitToGraph(false);
    return GetScope() == child;
}

bool UiNodeGraph::ExitScope()
{
    if(!CanExitScope())
        return false;
    UiGraphScopeRef parent = model_->GetParentScope(GetScope());
    if(!parent.IsValid())
        return false;
    SetScope(parent);
    FitToGraph(false);
    return GetScope() == parent;
}

UiNodeGraph& UiNodeGraph::FitToGraph(bool selection_only)
{
    if(!model_)
        return ResetView();

    bool have = false;
    double minx = 0.0, miny = 0.0, maxx = 0.0, maxy = 0.0;
    auto include_rect = [&](double l, double t, double r, double b) {
        if(!have) {
            minx = l; miny = t; maxx = r; maxy = b; have = true;
        }
        else {
            minx = min(minx, l); miny = min(miny, t);
            maxx = max(maxx, r); maxy = max(maxy, b);
        }
    };

    Vector<UiGraphNodeRef> nodes = model_->GetScopeNodes(GetScope());
    for(UiGraphNodeRef ref : nodes) {
        const UiGraphNode* node = model_->FindNode(ref);
        if(!node || !node->visible || (selection_only && !IsNodeSelected(ref)))
            continue;
        Pointf pos = GetDisplayNodePosition(*node);
        include_rect(pos.x, pos.y, pos.x + node->size.cx, pos.y + node->size.cy);
    }

    if(!selection_only) {
        Vector<UiGraphBackdropRef> backdrops = model_->GetScopeBackdrops(GetScope());
        for(UiGraphBackdropRef ref : backdrops) {
            const UiGraphBackdrop* backdrop = model_->FindBackdrop(ref);
            if(!backdrop || !backdrop->visible)
                continue;
            include_rect(backdrop->position.x, backdrop->position.y,
                         backdrop->position.x + backdrop->size.cx,
                         backdrop->position.y + backdrop->size.cy);
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
    Refresh();
    WhenViewport();
    return *this;
}

UiNodeGraph& UiNodeGraph::CenterOnNode(UiGraphNodeRef ref)
{
    const UiGraphNode* node = model_ ? model_->FindNode(ref) : nullptr;
    if(!node || model_->GetNodeScope(ref) != GetScope())
        return *this;
    Pointf pos = GetDisplayNodePosition(*node);
    Size sz = GetSize();
    pan_ = Pointf(sz.cx * 0.5 - (pos.x + node->size.cx * 0.5) * zoom_,
                  sz.cy * 0.5 - (pos.y + node->size.cy * 0.5) * zoom_);
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
    Refresh();
    WhenViewport();
    return *this;
}

UiNodeGraph& UiNodeGraph::SelectNode(UiGraphNodeRef node, bool additive)
{
    if(!model_ || !model_->Contains(node) || model_->GetNodeScope(node) != GetScope())
        return *this;
    Rect damage = GetSelectionDamage() | GetNodeDamage(node);
    if(!additive || !multi_selection_) {
        selected_nodes_.Clear();
        selected_edges_.Clear();
    }
    int i = selected_nodes_.Find(node.id);
    if(additive && i >= 0)
        selected_nodes_.Remove(i);
    else
        selected_nodes_.FindAdd(node.id);
    InvalidateGeometry();
    PrepareGeometry();
    damage |= GetSelectionDamage() | GetNodeDamage(node);
    RefreshDamage(damage);
    WhenSelection();
    return *this;
}

UiNodeGraph& UiNodeGraph::SelectEdge(UiGraphEdgeRef edge, bool additive)
{
    const UiGraphEdge* e = model_ ? model_->FindEdge(edge) : nullptr;
    if(!e || model_->GetNodeScope(e->source.node) != GetScope())
        return *this;
    Rect damage = GetSelectionDamage() | GetEdgeDamage(edge);
    if(!additive || !multi_selection_) {
        selected_nodes_.Clear();
        selected_edges_.Clear();
    }
    int i = selected_edges_.Find(edge.id);
    if(additive && i >= 0)
        selected_edges_.Remove(i);
    else
        selected_edges_.FindAdd(edge.id);
    InvalidateGeometry();
    PrepareGeometry();
    damage |= GetSelectionDamage() | GetEdgeDamage(edge);
    RefreshDamage(damage);
    WhenSelection();
    return *this;
}

void UiNodeGraph::SetData(const Value& v)
{
    Rect damage = GetSelectionDamage();
    selected_nodes_.Clear();
    selected_edges_.Clear();
    if(!IsNull(v) && model_) {
        ValueArray values;
        if(v.Is<ValueArray>())
            values = v;
        else
            values.Add(v);
        Vector<UiGraphNodeRef> scope_nodes = model_->GetScopeNodes(GetScope());
        for(int k = 0; k < values.GetCount(); k++) {
            const Value& token = values[k];
            UiGraphNodeRef found;
            if(IsNumber(token)) {
                UiGraphNodeRef candidate{(int64)token};
                if(model_->Contains(candidate) && model_->GetNodeScope(candidate) == GetScope())
                    found = candidate;
            }
            if(!found.IsValid())
                for(UiGraphNodeRef ref : scope_nodes) {
                    const UiGraphNode* node = model_->FindNode(ref);
                    if(node && !IsNull(node->data) && node->data == token) {
                        found = ref;
                        break;
                    }
                }
            if(found.IsValid())
                selected_nodes_.FindAdd(found.id);
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
    Vector<UiGraphNodeRef> current;
    for(int i = 0; i < selected_nodes_.GetCount(); i++) {
        UiGraphNodeRef ref{selected_nodes_[i]};
        if(model_ && model_->Contains(ref) && model_->GetNodeScope(ref) == GetScope())
            current.Add(ref);
    }
    if(current.GetCount() == 1)
        return GetSelectionToken(current[0]);
    ValueArray values;
    for(UiGraphNodeRef ref : current)
        values.Add(GetSelectionToken(ref));
    return values;
}

void UiNodeGraph::Layout()
{
    SyncThemeStyle();
    Vector<UiGraphNodeRef> nodes = model_ ? model_->GetScopeNodes(GetScope())
                                          : Vector<UiGraphNodeRef>();
    Vector<UiGraphBackdropRef> backdrops = model_ ? model_->GetScopeBackdrops(GetScope())
                                                  : Vector<UiGraphBackdropRef>();
    if(auto_fit_first_paint_ && !first_paint_done_ && model_
       && (!nodes.IsEmpty() || !backdrops.IsEmpty())) {
        first_paint_done_ = true;
        FitToGraph();
        return;
    }
    InvalidateGeometry();
    PrepareGeometry();
    UpdateAttachedCtrls();
}

void UiNodeGraph::PaintBackdrops(Draw& w)
{
    last_painted_backdrop_count_ = 0;
    if(!model_)
        return;

    Vector<UiGraphBackdropRef> refs = model_->GetScopeBackdrops(GetScope());
    Sort(refs, [&](UiGraphBackdropRef a, UiGraphBackdropRef b) {
        const UiGraphBackdrop* aa = model_->FindBackdrop(a);
        const UiGraphBackdrop* bb = model_->FindBackdrop(b);
        if(!aa || !bb)
            return a.id < b.id;
        return aa->z_order != bb->z_order ? aa->z_order < bb->z_order : a.id < b.id;
    });

    Rect paint = w.GetPaintRect() & Rect(Point(0, 0), GetSize());
    const Style& style = GetEffectiveStyle();
    Color paper = ResolveFace(style.canvas_palette.face[ST_NORMAL], SColorPaper());
    Color fill = BlendGraphColor(paper, style.grid_major, 0.11);
    Color frame = BlendGraphColor(paper, style.grid_major, 0.72);
    Color ink = BlendGraphColor(SColorText(), paper, 0.18);
    Font font = ScaleGraphFont(StdFont().Bold(), zoom_);

    for(UiGraphBackdropRef ref : refs) {
        const UiGraphBackdrop* backdrop = model_->FindBackdrop(ref);
        if(!backdrop || !backdrop->visible)
            continue;
        Point p = WorldToScreen(backdrop->position);
        Size sz(max(1, fround(backdrop->size.cx * zoom_)),
                max(1, fround(backdrop->size.cy * zoom_)));
        Rect r = RectC(p.x, p.y, sz.cx, sz.cy);
        if((r & paint).IsEmpty())
            continue;

        w.DrawRect(r, fill);
        Vector<Point> outline;
        outline << r.TopLeft() << r.TopRight() << r.BottomRight()
                << r.BottomLeft() << r.TopLeft();
        DrawPatternedPolyline(w, outline, max(1, DPI(1)), frame, DASHED,
                              max(3.0, 7.0 * min(1.0, zoom_)),
                              max(2.0, 5.0 * min(1.0, zoom_)));
        if(!backdrop->title.IsEmpty() && r.GetWidth() > DPI(24) && r.GetHeight() > DPI(18)) {
            int x = r.left + DPI(8);
            int y = r.top + DPI(5);
            w.DrawText(x, y, backdrop->title, font, ink);
        }
        last_painted_backdrop_count_++;
    }
}

} // namespace Upp
