namespace Upp {

UiNodeGraph& UiNodeGraph::SetModel(UiGraphModel& model)
{
    if(model_ == &model)
        return *this;
    ASSERT(batch_update_depth_ == 0);
    if(batch_update_depth_ > 0)
        return *this;

    UiGraphScopeRef before_scope = GetScope();

    if(interaction_ != InteractionMode::None || interaction_capture_owned_)
        CancelMode();

    // Clear attached controls without invoking the retained ClearNodeCtrls()
    // geometry refresh. A model switch immediately invalidates that geometry
    // anyway, and in a view transaction it must not materialise an intermediate
    // old-model frame.
    Vector<Ptr<Ctrl>> controls;
    for(int i = node_ctrls_.GetCount() - 1; i >= 0; --i)
        if(node_ctrls_[i])
            controls.Add(node_ctrls_[i]);
    node_ctrls_.Clear();
    for(Ptr<Ctrl> ctrl : controls)
        if(ctrl && ctrl->GetParent() == this)
            ctrl->Remove();

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

    scope_model_ = model_;
    active_scope_ = UiGraphModel::RootScope();

    InvalidateSpatialIndex();
    EnsureSpatialIndex();

    if(!DeferViewUpdate(false, true)) {
        PrepareViewGeometry();
        UpdateAttachedCtrls();
        Refresh();
        WhenSelection();
    }

    // Preserve the validated SetModel contract: when first-paint auto-fit is
    // enabled, binding a new model schedules Layout so the pending fit occurs.
    // Normal performance/demo paths keep auto-fit disabled and pay no layout cost.
    if(auto_fit_first_paint_)
        RefreshLayout();

    if(before_scope != active_scope_)
        QueueScopeChanged(active_scope_);
    return *this;
}

UiNodeGraph& UiNodeGraph::UseInternalModel()
{
    return SetModel(internal_model_);
}

} // namespace Upp
