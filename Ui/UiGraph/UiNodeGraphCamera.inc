// Synchronous camera setters and coalesced view transactions. Programmatic
// changes prepare exact geometry; live input projection belongs in Projection.

namespace Upp {

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
    const bool had_live_projection = CancelLiveViewProjection();
    if(abs(zoom - zoom_) < 1e-9 && !had_live_projection)
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
    const bool had_live_projection = CancelLiveViewProjection();
    if(pan_ == pan && !had_live_projection)
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
    const bool had_live_projection = CancelLiveViewProjection();
    if(abs(zoom_ - 1.0) < 1e-9 && pan_ == Pointf(0, 0) && !had_live_projection)
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
