#include "UiDesignerInteractionOverlay.h"
#include "UiDesignerWindow.h"

namespace Upp {

UiDesignerInteractionOverlay::UiDesignerInteractionOverlay(UiDesignerWindow& owner)
    : owner_(&owner)
{
    NoWantFocus();
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
    if(!owner_ || !owner_->preview_canvas_.GetParent())
        return;

    const UiDesignerDocument& document = owner_->session_.Document();
    const UiDesignerNode* root = document.Find(document.GetRootId());
    if(!root)
        return;

    const Point canvas_origin = owner_->preview_canvas_.GetRect().TopLeft();
    Rect root_rect = owner_->preview_canvas_.GetNodeRect(root->id).Offseted(canvas_origin);
    if(resizing_)
        root_rect = resize_pending_rect_;

    const UiDesignerSelection& selection = owner_->session_.State().selection;
    const int step = DPI(7);
    const int dot = DPI(3);
    for(UiDesignerNodeId id : selection.nodes) {
        Rect r = owner_->preview_canvas_.GetNodeRect(id);
        if(r.IsEmpty())
            continue;
        r.Offset(canvas_origin.x, canvas_origin.y);
        if(id == root->id && resizing_)
            r = resize_pending_rect_;
        const Color color = id == selection.primary
            ? Color(245, 158, 11) : Blend(Color(245, 158, 11), White(), 110);
        const int thickness = id == selection.primary ? DPI(2) : DPI(1);
        for(int x = r.left; x < r.right; x += step) {
            w.DrawRect(x, r.top, min(dot, r.right - x), thickness, color);
            w.DrawRect(x, r.bottom - thickness, min(dot, r.right - x), thickness, color);
        }
        for(int y = r.top; y < r.bottom; y += step) {
            w.DrawRect(r.left, y, thickness, min(dot, r.bottom - y), color);
            w.DrawRect(r.right - thickness, y, thickness, min(dot, r.bottom - y), color);
        }
    }

    const Color frame = Color(103, 232, 249);
    const int thickness = DPI(4);
    const int half = thickness / 2;
    w.DrawRect(root_rect.left - half, root_rect.top - half, root_rect.Width() + thickness, thickness, frame);
    w.DrawRect(root_rect.left - half, root_rect.bottom - half, root_rect.Width() + thickness, thickness, frame);
    w.DrawRect(root_rect.left - half, root_rect.top - half, thickness, root_rect.Height() + thickness, frame);
    w.DrawRect(root_rect.right - half, root_rect.top - half, thickness, root_rect.Height() + thickness, frame);

    const int handle = DPI(12);
    const Color fill = Blend(frame, White(), 170);
    const Point points[] = {
        root_rect.TopLeft(),
        Point(root_rect.CenterPoint().x, root_rect.top),
        Point(root_rect.right, root_rect.top),
        Point(root_rect.left, root_rect.CenterPoint().y),
        Point(root_rect.right, root_rect.CenterPoint().y),
        Point(root_rect.left, root_rect.bottom),
        Point(root_rect.CenterPoint().x, root_rect.bottom),
        root_rect.BottomRight()
    };
    for(const Point& point : points) {
        Rect grip = RectC(point.x - handle / 2, point.y - handle / 2, handle, handle);
        w.DrawRect(grip, fill);
        w.DrawRect(grip.left, grip.top, grip.Width(), 1, frame);
        w.DrawRect(grip.left, grip.bottom - 1, grip.Width(), 1, frame);
        w.DrawRect(grip.left, grip.top, 1, grip.Height(), frame);
        w.DrawRect(grip.right - 1, grip.top, 1, grip.Height(), frame);
    }

    if(!drop_indicator_.IsEmpty()) {
        const Color color = drop_plan_.valid ? Color(34, 197, 94) : Color(220, 38, 38);
        w.DrawRect(drop_indicator_.left, drop_indicator_.top,
                   drop_indicator_.Width(), 2, color);
        w.DrawRect(drop_indicator_.left, drop_indicator_.bottom - 2,
                   drop_indicator_.Width(), 2, color);
        w.DrawRect(drop_indicator_.left, drop_indicator_.top,
                   2, drop_indicator_.Height(), color);
        w.DrawRect(drop_indicator_.right - 2, drop_indicator_.top,
                   2, drop_indicator_.Height(), color);
    }
}

void UiDesignerInteractionOverlay::LeftDown(Point p, dword keyflags)
{
    if(!owner_)
        return;
    const int resize_edge = HitDocumentResizeEdge(p);
    if(resize_edge) {
        resizing_ = true;
        resize_edge_ = resize_edge;
        resize_start_ = p;
        resize_start_rect_ = WorkspaceRootRect();
        resize_pending_rect_ = resize_start_rect_;
        SetDragStatus(Format("resize root edge=%d %dx%d",
                             resize_edge,
                             resize_start_rect_.Width(),
                             resize_start_rect_.Height()));
        SetCapture();
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
        resize_pending_rect_ = ResizeDocumentTo(p);
        SetDragStatus(Format("resize root %dx%d",
                             resize_pending_rect_.Width(),
                             resize_pending_rect_.Height()));
        Refresh();
        return;
    }
    if(!drag_payload_.IsEmpty())
        UpdateDropPlan(p, drag_payload_);
}

void UiDesignerInteractionOverlay::LeftUp(Point p, dword)
{
    if(!owner_)
        return;
    if(resizing_) {
        resize_pending_rect_ = ResizeDocumentTo(p);
        const Size final_size = resize_pending_rect_.Size();
        resizing_ = false;
        resize_edge_ = 0;
        ReleaseCapture();
        if(final_size != resize_start_rect_.Size())
            owner_->session_.SetVirtualSize(final_size);
        SetDragStatus(Format("resize root done %dx%d",
                             final_size.cx, final_size.cy));
        Refresh();
    }
}

void UiDesignerInteractionOverlay::DragEnter()
{
    Refresh();
}

void UiDesignerInteractionOverlay::DragAndDrop(Point p, PasteClip& d)
{
    String payload;
    if(!d.IsAvailable(UiDesignerCatalogDragFormat())) {
        d.Reject();
        ClearDropPlan();
        return;
    }
    payload = d.Get(UiDesignerCatalogDragFormat());
    UpdateDropPlan(p, payload);
    if(!drop_plan_.valid) {
        d.Reject();
        ClearDropPlan(false);
        return;
    }
    d.Accept(UiDesignerCatalogDragFormat());
    d.SetAction(DND_COPY);
    if(d.IsPaste()) {
        String error;
        const bool ok = owner_->session_.ExecuteDrop(drop_plan_, nullptr, error);
        SetDragStatus(ok ? drop_plan_.label + " completed"
                         : (error.IsEmpty() ? drop_plan_.reason : error));
        ClearDropPlan();
    }
}

void UiDesignerInteractionOverlay::DragRepeat(Point p)
{
    if(!drag_payload_.IsEmpty())
        UpdateDropPlan(p, drag_payload_);
}

void UiDesignerInteractionOverlay::DragLeave()
{
    ClearDropPlan();
}

Rect UiDesignerInteractionOverlay::WorkspaceRootRect() const
{
    if(!owner_)
        return RectC(0, 0, 0, 0);
    return owner_->preview_canvas_.GetRect();
}

UiDesignerNodeId UiDesignerInteractionOverlay::HitNode(Point p) const
{
    if(!owner_)
        return 0;
    const Rect root = WorkspaceRootRect();
    if(!root.Contains(p))
        return 0;
    const Point local = p - owner_->preview_canvas_.GetRect().TopLeft();
    UiDesignerNodeId node = owner_->preview_canvas_.HitNode(local);
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

void UiDesignerInteractionOverlay::ClearDropPlan(bool clear_payload)
{
    drop_plan_ = UiDesignerDropPlan();
    drop_indicator_ = Rect(0, 0, 0, 0);
    if(clear_payload)
        drag_payload_.Clear();
    Refresh();
}

void UiDesignerInteractionOverlay::UpdateDropPlan(Point p, const String& payload)
{
    if(!owner_)
        return;
    String type;
    if(!UiDesignerParseCatalogDragText(payload, type)) {
        SetDragStatus("drag invalid catalog payload");
        ClearDropPlan(false);
        return;
    }

    const Rect root = WorkspaceRootRect();
    if(!root.Contains(p)) {
        drop_plan_ = UiDesignerDropPlan();
        drop_indicator_ = Rect();
        SetDragStatus(Format("drag %s -> outside Window", type));
        Refresh();
        return;
    }

    const UiDesignerNodeId target = owner_->session_.Document().GetRootId();
    const Point local = p - root.TopLeft();
    drop_plan_ = owner_->session_.PlanAddControl(type, target, local, true);
    drag_payload_ = payload;
    drop_indicator_ = root;
    SetDragStatus(Format("drag %s -> Window : %s%s",
                         type,
                         drop_plan_.valid ? "valid" : "invalid",
                         drop_plan_.valid ? "" : (", " + drop_plan_.reason)));
    Refresh();
}

}
