#include "UiDesignerInteractionOverlay.h"
#include "UiDesignerWindow.h"
#include <Utilities/UiDesigner/Preview/UiDesignerVisuals.h>

namespace Upp {

namespace {

static String LayoutNodeName(const UiDesignerCatalog *catalog, const UiDesignerNode& node)
{
    if(catalog) {
        const UiDesignerControlSpec* spec = catalog->Find(node.type);
        if(spec && !spec->display_name.IsEmpty())
            return spec->display_name;
    }
    return node.type;
}

static Rect ExpandVisualRect(Rect rect, int amount = DPI(1))
{
    if(amount <= 0)
        return rect;
    return rect.Inflated(amount);
}

static String BoxGapLabel(const UiDesignerCatalog *catalog, const UiDesignerNode& node,
                         const UiDesignerNode* prev, const UiDesignerNode* next,
                         int index)
{
    String base = LayoutNodeName(catalog, node);
    if(prev && next)
        return Format("%s \"%s\" — between %s and %s", base, node.name, prev->name, next->name);
    if(prev)
        return Format("%s \"%s\" — after %s", base, node.name, prev->name);
    if(next)
        return Format("%s \"%s\" — before %s", base, node.name, next->name);
    return Format("%s \"%s\" — slot %d", base, node.name, index);
}

static void AddRegion(UiDesignerGeometrySnapshotBuilder& snapshot,
                      UiDesignerDropRegion region)
{
    if(region.visual_rect.IsEmpty())
        region.visual_rect = region.rect;
    snapshot.AddRegion(pick(region));
}

static void AddWindowDropRegion(UiDesignerGeometrySnapshotBuilder& snapshot,
                                const UiDesignerNode& node,
                                const UiDesignerGeometryRecord& record)
{
    UiDesignerDropRegion region;
    region.owner = node.id;
    region.kind = UiDesignerDropRegionKind::WindowContent;
    region.rect = record.rect;
    region.visual_rect = record.rect;
    region.depth = record.depth;
    region.paint_order = record.order * 100;
    region.label = "Window";
    AddRegion(snapshot, pick(region));
}

static void AddPanelDropRegion(UiDesignerGeometrySnapshotBuilder& snapshot,
                               const UiDesignerNode& node,
                               const UiDesignerGeometryRecord& record)
{
    UiDesignerDropRegion region;
    region.owner = node.id;
    region.kind = UiDesignerDropRegionKind::PanelBody;
    region.rect = record.body;
    region.visual_rect = record.body;
    region.depth = record.depth + 1;
    region.paint_order = record.order * 100;
    region.label = LayoutNodeName(nullptr, node) + " body";
    AddRegion(snapshot, pick(region));
}

static Rect VisualDropRect(const UiDesignerResolvedDrop& drop)
{
    return drop.visual_rect.IsEmpty() ? drop.exact_rect : drop.visual_rect;
}

static void DrawDashedRect(Draw& w, Rect r, Color color, int thickness,
                           int dash, int gap)
{
    if(r.IsEmpty() || thickness <= 0)
        return;
    const int span = max(1, dash + gap);
    for(int x = r.left; x < r.right; x += span) {
        const int len = min(dash, r.right - x);
        if(len > 0) {
            w.DrawRect(x, r.top, len, thickness, color);
            w.DrawRect(x, r.bottom - thickness, len, thickness, color);
        }
    }
    for(int y = r.top; y < r.bottom; y += span) {
        const int len = min(dash, r.bottom - y);
        if(len > 0) {
            w.DrawRect(r.left, y, thickness, len, color);
            w.DrawRect(r.right - thickness, y, thickness, len, color);
        }
    }
}

static void DrawFrame(Draw& w, Rect r, Color color, int thickness)
{
    if(r.IsEmpty() || thickness <= 0)
        return;
    const int half = thickness / 2;
    w.DrawRect(r.left - half, r.top - half, r.Width() + thickness, thickness, color);
    w.DrawRect(r.left - half, r.bottom - half, r.Width() + thickness, thickness, color);
    w.DrawRect(r.left - half, r.top - half, thickness, r.Height() + thickness, color);
    w.DrawRect(r.right - half, r.top - half, thickness, r.Height() + thickness, color);
}

static void DrawResizeHandles(Draw& w, Rect r, Color frame)
{
    const int handle = DPI(12);
    const Color fill = Blend(frame, White(), 170);
    const Point points[] = {
        r.TopLeft(),
        Point(r.CenterPoint().x, r.top),
        Point(r.right, r.top),
        Point(r.left, r.CenterPoint().y),
        Point(r.right, r.CenterPoint().y),
        Point(r.left, r.bottom),
        Point(r.CenterPoint().x, r.bottom),
        r.BottomRight()
    };
    for(const Point& point : points) {
        Rect grip = RectC(point.x - handle / 2, point.y - handle / 2, handle, handle);
        w.DrawRect(grip, fill);
        w.DrawRect(grip.left, grip.top, grip.Width(), 1, frame);
        w.DrawRect(grip.left, grip.bottom - 1, grip.Width(), 1, frame);
        w.DrawRect(grip.left, grip.top, 1, grip.Height(), frame);
        w.DrawRect(grip.right - 1, grip.top, 1, grip.Height(), frame);
    }
}

}

UiDesignerInteractionOverlay::UiDesignerInteractionOverlay(UiDesignerWindow& owner)
    : owner_(&owner)
{
    WantFocus();
}

void UiDesignerInteractionOverlay::SetDragStatus(const String& status)
{
    drag_status_ = status;
    if(owner_)
        owner_->RefreshStatus(status);
    Refresh();
}

void UiDesignerInteractionOverlay::Paint(Draw& w)
{
    const bool measure = owner_ && owner_->preview_canvas_.IsDetailedTimingEnabled() &&
                         !owner_->preview_canvas_.IsCapturePaused();
    const int64 paint_start = measure ? usecs() : 0;
    if(owner_)
        owner_->preview_canvas_.BumpOverlayOnlyRepaint();
    if(!owner_ || !owner_->preview_canvas_.GetParent())
        return;

    const UiDesignerDocument& document = owner_->session_.Document();
    const UiDesignerNode* root = document.Find(document.GetRootId());
    if(!root)
        return;

    const Point canvas_origin = owner_->preview_canvas_.GetRect().TopLeft();
    const UiDesignerGeometryRecord* root_geometry =
        owner_->preview_canvas_.GetGeometrySnapshot().Find(root->id);
    Rect root_rect = root_geometry ? root_geometry->rect.Offseted(canvas_origin) : Rect();
    if(resizing_)
        root_rect = resize_pending_rect_;

    const Color frame = decorations_visible_
        ? Color(103, 232, 249)
        : Color(101, 116, 153);
    DrawFrame(w, root_rect, frame, decorations_visible_ ? DPI(4) : DPI(1));

    if(decorations_visible_) {
        const UiDesignerGeometrySnapshot& geometry = owner_->preview_canvas_.GetGeometrySnapshot();
        const UiDesignerSelection& selection = owner_->session_.State().selection;
        const int dash = DPI(5);
        const int gap = DPI(2);

        for(const UiDesignerNode& node : document.GetNodes()) {
            if(node.id == root->id)
                continue;
            const UiDesignerGeometryRecord* record = geometry.Find(node.id);
            if(!record || record->cue_kind == UiDesignerCueKind::None)
                continue;
            Rect r = record->rect.Offseted(canvas_origin);
            const Color cue = Blend(SColorText(), SColorPaper(), 170);
            DrawDashedRect(w, r, cue, DPI(1), dash, gap);
        }

        for(const UiDesignerNode& node : document.GetNodes()) {
            if(node.type != "UiBoxLayout" && node.type != "UiGridLayout")
                continue;
            const UiDesignerGeometryRecord* geometry_record = geometry.Find(node.id);
            if(!geometry_record || !geometry_record->debug_layout)
                continue;
            const Color outline = IsNull(geometry_record->debug_color)
                ? UiDesignerStableLayoutColor(node.id, geometry_record->depth)
                : geometry_record->debug_color;
            const Color fill = Blend(outline, SColorPaper(), 215);
            for(const Rect& inset : geometry_record->inset_rects) {
                Rect ir = inset.Offseted(canvas_origin);
                w.DrawRect(ir, fill);
            }
            for(const Rect& gap_rect : geometry_record->gap_rects) {
                Rect gr = gap_rect.Offseted(canvas_origin);
                w.DrawRect(gr, Blend(outline, SColorPaper(), 195));
            }
            for(const Rect& cell : geometry_record->cell_rects) {
                Rect cr = cell.Offseted(canvas_origin);
                w.DrawRect(cr.left, cr.top, cr.Width(), 1, outline);
                w.DrawRect(cr.left, cr.bottom - 1, cr.Width(), 1, outline);
                w.DrawRect(cr.left, cr.top, 1, cr.Height(), outline);
                w.DrawRect(cr.right - 1, cr.top, 1, cr.Height(), outline);
            }
            for(const Rect& item : geometry_record->item_rects) {
                Rect ir = item.Offseted(canvas_origin);
                w.DrawRect(ir.left, ir.top, ir.Width(), 2, outline);
                w.DrawRect(ir.left, ir.bottom - 2, ir.Width(), 2, outline);
                w.DrawRect(ir.left, ir.top, 2, ir.Height(), outline);
                w.DrawRect(ir.right - 2, ir.top, 2, ir.Height(), outline);
            }
            Rect body = geometry_record->body.Offseted(canvas_origin);
            w.DrawRect(body.left, body.top, body.Width(), 1, outline);
            w.DrawRect(body.left, body.bottom - 1, body.Width(), 1, outline);
            w.DrawRect(body.left, body.top, 1, body.Height(), outline);
            w.DrawRect(body.right - 1, body.top, 1, body.Height(), outline);
        }

        for(UiDesignerNodeId id : selection.nodes) {
            const UiDesignerGeometryRecord* geometry_record = geometry.Find(id);
            Rect r = geometry_record ? geometry_record->rect : Rect();
            if(r.IsEmpty())
                continue;
            r.Offset(canvas_origin.x, canvas_origin.y);
            if(id == root->id && resizing_)
                r = resize_pending_rect_;
            const Color color = id == selection.primary
                ? Color(245, 158, 11)
                : Blend(Color(245, 158, 11), White(), 110);
            const int thickness = id == selection.primary ? DPI(3) : DPI(2);
            DrawDashedRect(w, r, color, thickness, dash, gap);
        }

        DrawResizeHandles(w, root_rect, frame);
    }

    if(!resolved_drop_.visual_rect.IsEmpty() || !resolved_drop_.exact_rect.IsEmpty()) {
        const Rect indicator = VisualDropRect(resolved_drop_).Offseted(canvas_origin);
        const Color color = resolved_drop_.valid ? Color(34, 197, 94) : Color(220, 38, 38);
        w.DrawRect(indicator.left, indicator.top,
                   indicator.Width(), 3, color);
        w.DrawRect(indicator.left, indicator.bottom - 3,
                   indicator.Width(), 3, color);
        w.DrawRect(indicator.left, indicator.top,
                   3, indicator.Height(), color);
        w.DrawRect(indicator.right - 3, indicator.top,
                   3, indicator.Height(), color);
    }
    if(owner_ && measure) {
        const double overlay_ms = (double)usecs(paint_start) / 1000.0;
        owner_->preview_canvas_.RecordOverlayPaintMs(overlay_ms);
        FinalizeRootResizePaint(overlay_ms);
    }
}

void UiDesignerInteractionOverlay::LeftDown(Point p, dword keyflags)
{
    if(!owner_)
        return;
    const int resize_edge = HitDocumentResizeEdge(p);
    if(resize_edge) {
        resizing_ = true;
        pointer_gesture_ = UiDesignerPointerGesture::RootResize;
        resize_edge_ = resize_edge;
        resize_start_ = p;
        resize_start_rect_ = WorkspaceRootRect();
        resize_pending_rect_ = resize_start_rect_;
        resize_sample_ = UiDesignerResizeSample();
        resize_sample_valid_ = false;
        SetDragStatus(Format("resize root edge=%d %dx%d",
                             resize_edge,
                             resize_start_rect_.Width(),
                             resize_start_rect_.Height()));
        capture_owned_ = SetCapture();
        if(capture_owned_)
            drag_diagnostics_.capture_acquisitions++;
        SetFocus();
        return;
    }

    const UiDesignerNodeId hit = HitNode(p);
    if(!hit)
        return;
    SetDragStatus(Format("select node=%d", (int)hit));
    owner_->session_.Select(hit, (keyflags & K_CTRL) != 0);
    SetFocus();
}

void UiDesignerInteractionOverlay::MouseMove(Point p, dword)
{
    if(!owner_)
        return;
    if(resizing_) {
        ApplyRootResize(p);
    }
}

void UiDesignerInteractionOverlay::LeftUp(Point p, dword)
{
    if(!owner_)
        return;
    if(resizing_) {
        FinishRootResize(p);
    }
}

Image UiDesignerInteractionOverlay::CursorImage(Point, dword)
{
    if(resizing_ || !drag_type_id_.IsEmpty())
        return Image::SizeAll();
    return Ctrl::CursorImage(Point(), 0);
}

bool UiDesignerInteractionOverlay::Key(dword key, int)
{
    if(key == K_ESCAPE && resizing_) {
        CancelRootResize();
        return true;
    }
    if(key == K_ESCAPE && drag_state_ != UiDesignerCatalogDragState::Idle) {
        CancelCatalogDrag();
        return true;
    }
    if(key == K_DELETE) {
        if(owner_ && owner_->session_.RemoveSelection()) {
            SetDragStatus("Selection deleted");
            return true;
        }
        if(owner_)
            owner_->RefreshStatus(owner_->session_.Commands().GetLastError());
        return true;
    }
    return Ctrl::Key(key, 1);
}

void UiDesignerInteractionOverlay::CancelMode()
{
    Ctrl::CancelMode();
    if(capture_release_in_progress_ || drag_cleanup_in_progress_)
        return;
    if(pointer_gesture_ == UiDesignerPointerGesture::RootResize) {
        CancelRootResize();
        return;
    }
    if(pointer_gesture_ == UiDesignerPointerGesture::CatalogDrag ||
       drag_state_ != UiDesignerCatalogDragState::Idle)
        CancelCatalogDrag();
}

void UiDesignerInteractionOverlay::ReleaseOwnedCaptureSafely()
{
    if(!capture_owned_)
        return;
    // HasCapture is the U++ ownership query; never call the instance release
    // path merely because an old gesture flag says capture existed.
    if(HasCapture()) {
        ReleaseCapture();
        drag_diagnostics_.capture_releases++;
    }
    capture_owned_ = false;
}

void UiDesignerInteractionOverlay::RecordRootResizeSample(
    const UiDesignerPreviewStats& before, const UiDesignerPreviewStats& after,
    double sync_ms, double preview_ms, const Rect& final_rect)
{
    if(!owner_)
        return;

    resize_sample_ = UiDesignerResizeSample();
    resize_sample_.sequence = ++resize_sample_sequence_;
    resize_sample_.timing_enabled = owner_->preview_canvas_.IsDetailedTimingEnabled() &&
                                    !owner_->preview_canvas_.IsCapturePaused();
    resize_sample_.complete = true;
    resize_sample_.paint_complete = !resize_sample_.timing_enabled;
    resize_sample_.total_ms = sync_ms;
    resize_sample_.window_resize_ms = sync_ms;
    resize_sample_.immediate_preview_ms = preview_ms;
    resize_sample_.grid_layout_ms = after.grid_layout_passes > before.grid_layout_passes
        ? owner_->preview_canvas_.GetGridLayoutDurationTotalMs() : -1;
    resize_sample_.box_layout_ms = after.box_layout_passes > before.box_layout_passes
        ? owner_->preview_canvas_.GetBoxLayoutDurationTotalMs() : -1;
    resize_sample_.geometry_walk_ms = after.geometry_walk_time_ms;
    resize_sample_.snapshot_ms = after.snapshot_time_ms;
    resize_sample_.overlay_paint_ms = -1;
    resize_sample_.canvas_paint_ms = -1;
    resize_sample_.inspector_ms = -1;
    resize_sample_.code_ms = -1;
    resize_sample_.resize_events = after.resize_events - before.resize_events;
    resize_sample_.immediate_live_rect_updates =
        after.immediate_live_rect_updates - before.immediate_live_rect_updates;
    resize_sample_.grid_layout_passes = after.grid_layout_passes - before.grid_layout_passes;
    resize_sample_.box_layout_passes = after.box_layout_passes - before.box_layout_passes;
    resize_sample_.absolute_layout_updates =
        after.absolute_layout_updates - before.absolute_layout_updates;
    resize_sample_.layout_item_updates = after.layout_item_updates - before.layout_item_updates;
    resize_sample_.preview_layout_calls = after.preview_layout_calls - before.preview_layout_calls;
    resize_sample_.full_geometry_walks = after.full_geometry_walks - before.full_geometry_walks;
    resize_sample_.geometry_snapshot_publications =
        after.snapshot_publications - before.snapshot_publications;
    resize_sample_.drop_region_publications =
        after.drop_region_publications - before.drop_region_publications;
    resize_sample_.overlay_only_repaints =
        after.overlay_only_repaints - before.overlay_only_repaints;
    resize_sample_.full_canvas_repaints =
        after.full_canvas_repaints - before.full_canvas_repaints;
    resize_sample_.property_editor_refreshes =
        after.property_editor_refreshes - before.property_editor_refreshes;
    resize_sample_.hierarchy_refreshes = after.hierarchy_refreshes - before.hierarchy_refreshes;
    resize_sample_.code_refreshes = after.code_refreshes - before.code_refreshes;
    resize_sample_.deferred_batches = after.deferred_batches - before.deferred_batches;
    resize_sample_.subtree_rebuilds = after.subtree_rebuilds - before.subtree_rebuilds;
    resize_sample_.full_document_rebuilds =
        after.full_document_rebuilds - before.full_document_rebuilds;
    resize_sample_.live_instance_creations =
        after.live_instance_creations - before.live_instance_creations;
    resize_sample_.live_instance_destructions =
        after.live_instance_destructions - before.live_instance_destructions;
    resize_sample_.track_size_calculations =
        after.track_size_calculations - before.track_size_calculations;
    resize_sample_.cached_grid_geometry_publications =
        after.cached_grid_geometry_publications - before.cached_grid_geometry_publications;
    resize_sample_.cached_grid_geometry_reads =
        after.cached_grid_geometry_reads - before.cached_grid_geometry_reads;
    resize_sample_.transient_root_size_updates =
        after.transient_root_size_updates - before.transient_root_size_updates;
    resize_sample_.layout_time_ms = after.layout_time_ms;
    resize_sample_.grid_layout_time_ms = after.grid_layout_time_ms;
    resize_sample_.box_layout_time_ms = after.box_layout_time_ms;
    resize_sample_.geometry_walk_time_ms = after.geometry_walk_time_ms;
    resize_sample_.snapshot_time_ms = after.snapshot_time_ms;
    resize_sample_.overlay_paint_time_ms = after.overlay_paint_time_ms;
    resize_sample_.canvas_paint_time_ms = after.canvas_paint_time_ms;
    resize_sample_.decorations_visible = decorations_visible_;
    resize_sample_.document_nodes = owner_->session_.Document().GetNodes().GetCount();
    resize_sample_.live_runtime_controls = owner_->preview_canvas_.GetLiveInstanceCount();
    resize_sample_.selected_node = owner_->session_.State().selection.primary;
    if(const UiDesignerNode* node = owner_->session_.Document().Find(resize_sample_.selected_node)) {
        resize_sample_.authored_type = node->type;
        if(const UiDesignerControlSpec* spec = owner_->session_.Catalog().Find(node->type))
            resize_sample_.runtime_type = spec->runtime_cpp_type;
    }
    resize_sample_.generation = owner_->preview_canvas_.GetInstanceGeneration(resize_sample_.selected_node);
    resize_sample_.rect = final_rect;
    resize_sample_.virtual_size = final_rect.Size();

    if(owner_->preview_canvas_.IsCapturePaused()) {
        resize_sample_valid_ = false;
        return;
    }

    owner_->preview_canvas_.RecordResizeSample(resize_sample_);
    resize_sample_valid_ = resize_sample_.timing_enabled;
    if(!resize_sample_valid_)
        owner_->RequestDiagnosticsRefresh();
}

void UiDesignerInteractionOverlay::FinalizeRootResizePaint(double overlay_ms)
{
    if(!owner_ || !resize_sample_valid_)
        return;

    UiDesignerResizeSample* latest = owner_->preview_canvas_.GetResizeHistory().GetMutableLatest();
    if(!latest || latest->sequence != resize_sample_sequence_)
        return;

    latest->overlay_paint_ms = overlay_ms;
    latest->overlay_paint_time_ms = overlay_ms;
    latest->canvas_paint_ms = owner_->preview_canvas_.GetStats().canvas_paint_time_ms;
    latest->canvas_paint_time_ms = latest->canvas_paint_ms;
    if(latest->canvas_paint_ms >= 0) {
        latest->paint_complete = true;
        resize_sample_valid_ = false;
        owner_->RequestDiagnosticsRefresh();
    }
}

void UiDesignerInteractionOverlay::ApplyRootResize(Point p)
{
    if(!owner_ || !resizing_)
        return;

    resize_pending_rect_ = ResizeDocumentTo(p);
    const Size final_size = resize_pending_rect_.Size();
    if(!resize_batch_timing_) {
        resize_batch_before_ = owner_->preview_canvas_.GetStats();
        resize_batch_timing_ = owner_->preview_canvas_.IsDetailedTimingEnabled() &&
                               !owner_->preview_canvas_.IsCapturePaused();
        resize_batch_start_us_ = resize_batch_timing_ ? usecs() : 0;
    }
    Ptr<UiDesignerInteractionOverlay> self = this;
    owner_->QueuePreviewVirtualSize(final_size, [self] {
        if(!self || !self->owner_)
            return;
        const UiDesignerPreviewStats after = self->owner_->preview_canvas_.GetStats();
        const double sync_ms = self->resize_batch_timing_
            ? (double)usecs(self->resize_batch_start_us_) / 1000.0 : -1;
        self->RecordRootResizeSample(self->resize_batch_before_, after, sync_ms, sync_ms,
                                     self->resize_pending_rect_);
        self->resize_batch_timing_ = false;
    });

    SetDragStatus(Format("resize root %dx%d", final_size.cx, final_size.cy));
    Refresh();
}

void UiDesignerInteractionOverlay::ApplyQueuedRootResize()
{
    if(owner_)
        owner_->FlushPreviewVirtualSize();
}

void UiDesignerInteractionOverlay::FinishRootResize(Point p)
{
    if(!owner_ || !resizing_)
        return;

    resize_pending_rect_ = ResizeDocumentTo(p);
    const Size final_size = resize_pending_rect_.Size();
    resizing_ = false;
    resize_edge_ = 0;
    pointer_gesture_ = UiDesignerPointerGesture::None;
    Ptr<UiDesignerInteractionOverlay> keep_alive = this;

    owner_->FlushPreviewVirtualSize();

    capture_release_in_progress_ = true;
    ReleaseOwnedCaptureSafely();
    capture_release_in_progress_ = false;

    if(!keep_alive)
        return;

    owner_->preview_canvas_.ClearTransientVirtualSize();
    owner_->CancelQueuedPreviewVirtualSize();
    resize_batch_timing_ = false;
    owner_->session_.SetVirtualSize(final_size);
    owner_->ApplyPreviewVirtualSize(final_size);
    owner_->preview_canvas_.Layout();

    SetDragStatus(Format("resize root done %dx%d",
                         final_size.cx, final_size.cy));
    Refresh();
}

void UiDesignerInteractionOverlay::CancelRootResize()
{
    if(!owner_ || !resizing_)
        return;

    resizing_ = false;
    resize_edge_ = 0;
    pointer_gesture_ = UiDesignerPointerGesture::None;
    resize_sample_valid_ = false;
    resize_pending_rect_ = resize_start_rect_;
    owner_->CancelQueuedPreviewVirtualSize();
    resize_batch_timing_ = false;
    capture_release_in_progress_ = true;
    ReleaseOwnedCaptureSafely();
    capture_release_in_progress_ = false;
    owner_->preview_canvas_.ClearTransientVirtualSize();
    owner_->ApplyPreviewVirtualSize(resize_start_rect_.Size());
    owner_->preview_canvas_.Layout();
    SetDragStatus("resize root cancelled");
    Refresh();
}

void UiDesignerInteractionOverlay::TrackCatalogDrag(const String& type_id, Point screen)
{
    if(cleaning_drag_ || drag_state_ == UiDesignerCatalogDragState::Completing)
        return;
    drag_diagnostics_.tracking_calls++;
    drag_diagnostics_.tracking_depth++;
    drag_diagnostics_.max_tracking_depth = max(
        drag_diagnostics_.max_tracking_depth, drag_diagnostics_.tracking_depth);
    ASSERT(drag_diagnostics_.tracking_depth <= 1);
    if(drag_diagnostics_.tracking_depth > 1) {
        drag_diagnostics_.tracking_depth--;
        CancelCatalogDrag();
        return;
    }
    if(type_id.IsEmpty()) {
        EndCatalogDrag(UiDesignerCatalogDragState::Cancelling);
        SetDragStatus("drag invalid catalog payload");
        drag_diagnostics_.tracking_depth--;
        return;
    }
    drag_state_ = UiDesignerCatalogDragState::Tracking;
    pointer_gesture_ = UiDesignerPointerGesture::CatalogDrag;
    drag_type_id_ = type_id;
    UpdateDropPlan(type_id, screen);
    drag_diagnostics_.tracking_depth--;
}

bool UiDesignerInteractionOverlay::FinishCatalogDrag(const String& type_id, Point screen)
{
    if(cleaning_drag_ || drag_state_ == UiDesignerCatalogDragState::Idle)
        return false;
    drag_state_ = UiDesignerCatalogDragState::Completing;
    if(type_id.IsEmpty()) {
        CancelCatalogDrag();
        return false;
    }
    UpdateDropPlan(type_id, screen, false);
    if(!resolved_drop_.valid) {
        CancelCatalogDrag();
        return false;
    }
    String error;
    const bool ok = owner_->session_.ExecuteDrop(resolved_drop_.plan, nullptr, error);
    if(ok)
        SetDragStatus(resolved_drop_.label + " completed");
    else
        SetDragStatus(error.IsEmpty() ? resolved_drop_.reason : error);
    EndCatalogDrag(UiDesignerCatalogDragState::Idle);
    return ok;
}

void UiDesignerInteractionOverlay::CancelCatalogDrag()
{
    EndCatalogDrag(UiDesignerCatalogDragState::Cancelling);
}

void UiDesignerInteractionOverlay::InvalidateCatalogDrag()
{
    if(drag_state_ == UiDesignerCatalogDragState::Idle)
        return;
    resolved_drop_ = UiDesignerResolvedDrop();
    Refresh();
}

Rect UiDesignerInteractionOverlay::WorkspaceRootRect() const
{
    if(!owner_)
        return RectC(0, 0, 0, 0);
    return owner_->preview_canvas_.GetRect();
}

Point UiDesignerInteractionOverlay::ScreenToWorkspace(Point screen) const
{
    return screen - GetScreenRect().TopLeft();
}

Point UiDesignerInteractionOverlay::WorkspaceToCanvas(Point workspace) const
{
    if(!owner_)
        return workspace;
    return workspace - owner_->preview_canvas_.GetRect().TopLeft();
}

Point UiDesignerInteractionOverlay::ScreenToCanvas(Point screen) const
{
    return WorkspaceToCanvas(ScreenToWorkspace(screen));
}

Rect UiDesignerInteractionOverlay::CanvasToWorkspace(Rect canvas) const
{
    if(!owner_)
        return canvas;
    return canvas.Offseted(owner_->preview_canvas_.GetRect().TopLeft());
}

UiDesignerNodeId UiDesignerInteractionOverlay::HitNode(Point p) const
{
    if(!owner_)
        return 0;
    const Rect root = WorkspaceRootRect();
    if(!root.Contains(p))
        return 0;
    const Point local = p - owner_->preview_canvas_.GetRect().TopLeft();
    UiDesignerNodeId node = owner_->preview_canvas_.GetGeometrySnapshot().Hit(local);
    if(node)
        return node;
    const UiDesignerNode* document_root =
        owner_->session_.Document().Find(owner_->session_.Document().GetRootId());
    return document_root ? document_root->id : 0;
}

int UiDesignerInteractionOverlay::HitDocumentResizeEdge(Point p) const
{
    Rect root = WorkspaceRootRect();
    const int grab = DPI(12);
    if(!root.Inflated(grab).Contains(p))
        return 0;
    int edge = 0;
    if(abs(p.x - root.left) <= grab) edge |= 1;
    if(abs(p.x - root.right) <= grab) edge |= 2;
    if(abs(p.y - root.top) <= grab) edge |= 4;
    if(abs(p.y - root.bottom) <= grab) edge |= 8;
    return edge;
}

Rect UiDesignerInteractionOverlay::ResizeDocumentTo(Point p) const
{
    const Point delta = p - resize_start_;
    Rect rect = resize_start_rect_;
    const int min_width = DPI(160);
    const int min_height = DPI(160);

    if(resize_edge_ & 2)
        rect.right = max(rect.left + min_width, resize_start_rect_.right + delta.x);
    if(resize_edge_ & 1)
        rect.left = min(rect.right - min_width, resize_start_rect_.left + delta.x);
    if(resize_edge_ & 8)
        rect.bottom = max(rect.top + min_height, resize_start_rect_.bottom + delta.y);
    if(resize_edge_ & 4)
        rect.top = min(rect.bottom - min_height, resize_start_rect_.top + delta.y);
    return rect;
}

void UiDesignerInteractionOverlay::ClearDropPlan()
{
    resolved_drop_ = UiDesignerResolvedDrop();
    Refresh();
}

void UiDesignerInteractionOverlay::EndCatalogDrag(UiDesignerCatalogDragState terminal)
{
    if(drag_cleanup_in_progress_)
        return;
    drag_cleanup_in_progress_ = true;
    cleaning_drag_ = true;
    const bool release_capture = capture_owned_ && HasCapture();

    // Clear logical state before U++ can synchronously call CancelMode from
    // ReleaseCapture. The callback must observe an already terminal gesture.
    drag_state_ = UiDesignerCatalogDragState::Idle;
    pointer_gesture_ = UiDesignerPointerGesture::None;
    drag_type_id_.Clear();
    resolved_drop_ = UiDesignerResolvedDrop();
    drag_diagnostics_.tracking_depth = 0;

    if(release_capture) {
        capture_release_in_progress_ = true;
        ReleaseOwnedCaptureSafely();
        capture_release_in_progress_ = false;
    }
    drag_diagnostics_.terminal_cancellations++;
    SetDragStatus(String());
    Refresh();
    cleaning_drag_ = false;
    drag_cleanup_in_progress_ = false;
}

void UiDesignerInteractionOverlay::UpdateDropPlan(const String& type_id, Point screen,
                                                  bool allow_invalid_feedback)
{
    if(!owner_)
        return;

    drag_diagnostics_.target_resolutions++;

    const Point workspace_local = ScreenToWorkspace(screen);
    const Rect root = WorkspaceRootRect();
    drag_type_id_ = type_id;
    if(!root.Contains(workspace_local)) {
        resolved_drop_ = UiDesignerResolvedDrop();
        SetDragStatus(Format("drag %s -> outside Window", type_id));
        Refresh();
        return;
    }

    const UiDesignerDocument& document = owner_->session_.Document();
    const Point doc_local = WorkspaceToCanvas(workspace_local);
    const UiDesignerGeometrySnapshot& geometry = owner_->preview_canvas_.GetGeometrySnapshot();
    const UiDesignerDropRegion* region = geometry.HitDropRegion(doc_local);
    if(!region) {
        resolved_drop_ = UiDesignerResolvedDrop();
        SetDragStatus(Format("drag %s -> Window : invalid, no region", type_id));
        Refresh();
        return;
    }

    resolved_drop_.region_id = region->paint_order;
    resolved_drop_.region = *region;
    resolved_drop_.exact_rect = region->rect;
    resolved_drop_.visual_rect = region->visual_rect.IsEmpty() ? region->rect : region->visual_rect;
    resolved_drop_.insertion_index = region->insertion_index;
    resolved_drop_.grid_row = region->grid_row;
    resolved_drop_.grid_column = region->grid_column;
    resolved_drop_.label = region->label;
    resolved_drop_.reason.Clear();
    resolved_drop_.plan = UiDesignerDropPlan();
    resolved_drop_.valid = false;

    const UiDesignerNode* target_node = document.Find(region->owner);
    if(!target_node) {
        resolved_drop_.reason = "Drop target does not exist";
    }
    else {
        Point position = region->rect.CenterPoint();
        resolved_drop_.plan = owner_->session_.PlanAddControl(
            type_id, region->owner, position, true,
            region->insertion_index, region->grid_row, region->grid_column);
        resolved_drop_.valid = resolved_drop_.plan.valid;
        resolved_drop_.reason = resolved_drop_.plan.reason;
        if(resolved_drop_.valid) {
            resolved_drop_.plan.label = region->label;
            const UiDesignerGeometryRecord* target_geometry = geometry.Find(region->owner);
            if(target_geometry && !target_geometry->rect.IsEmpty())
                resolved_drop_.visual_rect = region->visual_rect.IsEmpty()
                    ? target_geometry->rect
                    : region->visual_rect;
            if(!allow_invalid_feedback && !resolved_drop_.valid)
                resolved_drop_.visual_rect = Rect();
        }
    }

    const String target_name = target_node && target_node->id != document.GetRootId()
        ? LayoutNodeName(&owner_->session_.Catalog(), *target_node)
        : "Window";
    if(resolved_drop_.valid) {
        SetDragStatus(resolved_drop_.label + " : valid");
    }
    else {
        SetDragStatus(Format("drag %s -> %s : invalid%s",
                             type_id,
                             target_name,
                             resolved_drop_.reason.IsEmpty() ? String()
                                                             : ", " + resolved_drop_.reason));
        if(!allow_invalid_feedback)
            resolved_drop_.visual_rect = Rect();
    }
    Refresh();
}

}
