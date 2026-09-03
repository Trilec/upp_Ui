namespace Upp {

UiNodeGraph& UiNodeGraph::SetModel(UiGraphModel& model)
{
    UiGraphScopeRef before = GetScope();
    UiGraphModel* before_model = model_;
    SetModelLegacy(model);
    if(before_model != model_) {
        scope_model_ = model_;
        active_scope_ = UiGraphModel::RootScope();
        if(before != active_scope_)
            WhenScopeChanged(active_scope_);
    }
    return *this;
}

UiNodeGraph& UiNodeGraph::UseInternalModel()
{
    UiGraphScopeRef before = GetScope();
    UiGraphModel* before_model = model_;
    UseInternalModelLegacy();
    if(before_model != model_) {
        scope_model_ = model_;
        active_scope_ = UiGraphModel::RootScope();
        if(before != active_scope_)
            WhenScopeChanged(active_scope_);
    }
    return *this;
}

} // namespace Upp
