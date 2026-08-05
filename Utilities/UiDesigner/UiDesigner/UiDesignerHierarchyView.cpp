#include "UiDesignerWidgets.h"
#include <Ui/UiIcons.h>

namespace Upp {

UiDesignerHierarchyView::UiDesignerHierarchyView()
{
    WantFocus();
}

UiDesignerHierarchyView::~UiDesignerHierarchyView()
{
    ResetNodeDrag();
}

void UiDesignerHierarchyView::SetDocument(const UiDesignerDocument *document)
{
    document_ = document;
    Rebuild();
}

void UiDesignerHierarchyView::SetCatalog(const UiDesignerCatalog *catalog)
{
    catalog_ = catalog;
    Refresh();
}

void UiDesignerHierarchyView::SetSelection(const UiDesignerSelection *selection)
{
    selection_ = selection;
    Refresh();
}

void UiDesignerHierarchyView::AddRows(UiDesignerNodeId node, int depth)
{
    if(!document_)
        return;
    const UiDesignerNode* n = document_->Find(node);
    if(!n)
        return;
    Row& row = rows_.Add();
    row.node = node;
    row.depth = depth;
    for(UiDesignerNodeId child : n->children)
        AddRows(child, depth + 1);
}

void UiDesignerHierarchyView::BuildRows()
{
    rows_.Clear();
    if(document_)
        AddRows(document_->GetRootId(), 0);
}

void UiDesignerHierarchyView::Rebuild()
{
    BuildRows();
    Refresh();
}

Rect UiDesignerHierarchyView::RowRect(int index) const
{
    return RectC(0, GetHeaderRect().Height() + index * DPI(30) - scroll_,
                 GetSize().cx, DPI(30));
}

int UiDesignerHierarchyView::RowAt(Point p) const
{
    if(GetHeaderRect().Contains(p) || p.x < 0 || p.x >= GetSize().cx ||
       p.y < GetHeaderRect().bottom || p.y >= GetSize().cy)
        return -1;
    const int index = (p.y - GetHeaderRect().Height() + scroll_) / DPI(30);
    return index >= 0 && index < rows_.GetCount() ? index : -1;
}

Rect UiDesignerHierarchyView::GetHeaderRect() const
{
    return RectC(0, 0, GetSize().cx, DPI(24));
}

Rect UiDesignerHierarchyView::GetNameRect(int index) const
{
    Rect row = RowRect(index);
    const int type_width = DPI(94);
    const int mode_width = DPI(24);
    const int gap = DPI(4);
    return Rect(row.left, row.top,
                max(0, row.Width() - type_width - 2 * mode_width - gap),
                row.Height());
}

Rect UiDesignerHierarchyView::GetTypeRect(int index) const
{
    Rect row = RowRect(index);
    const int type_width = DPI(94);
    const int mode_width = DPI(24);
    const int gap = DPI(4);
    return Rect(row.right - 2 * mode_width - type_width - gap, row.top,
                type_width, row.Height());
}

Rect UiDesignerHierarchyView::ModeRect(int index, bool height) const
{
    Rect row = RowRect(index);
    const int mode_width = DPI(24);
    const int gap = DPI(4);
    const int x = row.right - mode_width - (height ? 0 : mode_width + gap);
    return Rect(x, row.top, mode_width, row.Height());
}

Rect UiDesignerHierarchyView::GetWidthModeRect(int index) const
{
    return ModeRect(index, false);
}

Rect UiDesignerHierarchyView::GetHeightModeRect(int index) const
{
    return ModeRect(index, true);
}

bool UiDesignerHierarchyView::HasSizingMode(const UiDesignerNode& node) const
{
    return node.type != "UiTabPage" && node.type != "UiAccordionSection" &&
           node.properties.Find("width_mode") >= 0 &&
           node.properties.Find("height_mode") >= 0;
}

String UiDesignerHierarchyView::FriendlyType(const UiDesignerNode& node) const
{
    if(catalog_) {
        const UiDesignerControlSpec *spec = catalog_->Find(node.type);
        if(spec && !spec->display_name.IsEmpty())
            return spec->display_name;
    }
    return node.type;
}

Image UiDesignerHierarchyView::SizingIcon(const String& mode, bool) const
{
    if(mode == "Fixed")
        return ICON_DESIGN_ASPECT_RATIO_48();
    if(mode == "Expand")
        return ICON_DESIGN_ARROWS_OUTPUT_48();
    return ICON_DESIGN_FIT_PAGE_48();
}

void UiDesignerHierarchyView::UpdateSizingTip(int index, bool height)
{
    if(index < 0 || index >= rows_.GetCount() || !document_)
        return;
    const UiDesignerNode *node = document_->Find(rows_[index].node);
    if(!node || !HasSizingMode(*node))
        return;
    const String property = height ? "height_mode" : "width_mode";
    const String mode = node->GetProperty(property, "Fit");
    const String axis = height ? "Height" : "Width";
    Tip(Format("%s mode: %s. Click to change to the next mode.", axis, mode));
}

void UiDesignerHierarchyView::Paint(Draw& w)
{
    w.DrawRect(GetSize(), SColorPaper());
    const Rect header = GetHeaderRect();
    w.DrawRect(header, Blend(SColorFace(), SColorPaper(), 70));
    w.DrawText(DPI(8), header.top + DPI(5), "Name",
               SansSerifZ(9).Bold(), SColorText());
    w.DrawText(GetTypeRect(0).left + DPI(4), header.top + DPI(5), "Type",
               SansSerifZ(9), SColorText());
    w.DrawText(GetWidthModeRect(0).left + DPI(7), header.top + DPI(5), "W",
               SansSerifZ(9).Bold(), SColorText());
    w.DrawText(GetHeightModeRect(0).left + DPI(7), header.top + DPI(5), "H",
               SansSerifZ(9).Bold(), SColorText());
    for(int i = 0; i < rows_.GetCount(); i++) {
        Rect r = RowRect(i);
        if(r.bottom < 0 || r.top > GetSize().cy)
            continue;
        const UiDesignerNode* node = document_ ? document_->Find(rows_[i].node) : nullptr;
        if(!node)
            continue;
        const bool selected = selection_ && selection_->Contains(node->id);
        if(selected)
            w.DrawRect(r, Blend(SColorHighlight(), SColorPaper(), 80));
        const int x = DPI(8) + rows_[i].depth * DPI(16);
        w.DrawText(x, r.top + DPI(7), node->name,
                   SansSerifZ(10), SColorText());
        w.DrawText(GetTypeRect(i).left + DPI(4), r.top + DPI(8),
                   FriendlyType(*node), SansSerifZ(9),
                   Blend(SColorText(), SColorPaper(), 55));
        if(HasSizingMode(*node)) {
            const String width_mode = node->GetProperty("width_mode", "Fit");
            const String height_mode = node->GetProperty("height_mode", "Fit");
            w.DrawImage(GetWidthModeRect(i).left + DPI(4), r.top + DPI(7),
                        DPI(16), DPI(16), SizingIcon(width_mode, false));
            w.DrawImage(GetHeightModeRect(i).left + DPI(4), r.top + DPI(7),
                        DPI(16), DPI(16), SizingIcon(height_mode, true));
        }
    }
    if(drop_row_ >= 0 && drop_row_ < rows_.GetCount()) {
        Rect r = RowRect(drop_row_);
        const Color color = drop_plan_.valid
            ? Color(34, 197, 94) : Color(220, 38, 38);
        if(drop_edge_ < 0)
            w.DrawRect(r.left, r.top, r.Width(), DPI(2), color);
        else if(drop_edge_ > 0)
            w.DrawRect(r.left, r.bottom - DPI(2), r.Width(), DPI(2), color);
        else {
            w.DrawRect(r.left, r.top, r.Width(), DPI(2), color);
            w.DrawRect(r.left, r.bottom - DPI(2), r.Width(), DPI(2), color);
            w.DrawRect(r.left, r.top, DPI(2), r.Height(), color);
            w.DrawRect(r.right - DPI(2), r.top, DPI(2), r.Height(), color);
        }
    }
}

void UiDesignerHierarchyView::LeftDown(Point p, dword flags)
{
    pressed_ = RowAt(p);
    if(pressed_ >= 0) {
        WhenSelectNode(rows_[pressed_].node, (flags & K_CTRL) != 0);
        const UiDesignerNodeId node = rows_[pressed_].node;
        const UiDesignerNode *item = document_ ? document_->Find(node) : nullptr;
        if(item && HasSizingMode(*item)) {
            const bool height = GetHeightModeRect(pressed_).Contains(p);
            const bool width = GetWidthModeRect(pressed_).Contains(p);
            if(width || height) {
                UpdateSizingTip(pressed_, height);
                if(CycleSizingMode)
                    CycleSizingMode(node, height);
                pressed_ = -1;
                return;
            }
        }
        if(document_ && node != document_->GetRootId()) {
            node_drag_nodes_.Clear();
            if(selection_ && selection_->Contains(node))
                node_drag_nodes_ = clone(selection_->nodes);
            else
                node_drag_nodes_.Add(node);
            node_drag_start_ = GetMousePos();
            node_dragging_ = false;
            SetCapture();
            ArmNodeDragPoll();
        }
    }
    SetFocus();
}

void UiDesignerHierarchyView::LeftUp(Point, dword)
{
    if(node_dragging_)
        FinishNodeDrop(GetMousePos());
    else
        ResetNodeDrag();
}

void UiDesignerHierarchyView::LeftDrag(Point, dword)
{
    PollNodeDrag();
}

void UiDesignerHierarchyView::MouseMove(Point, dword flags)
{
    if(node_drag_nodes_.IsEmpty())
        return;
    if(!(flags & K_MOUSELEFT) && !GetMouseLeft()) {
        ResetNodeDrag();
        if(HasCapture())
            ReleaseCapture();
        return;
    }
    const Point screen = GetMousePos();
    if(!node_dragging_ && Length(screen - node_drag_start_) >= DPI(5))
        node_dragging_ = true;
    if(!node_dragging_)
        return;
    if(GetScreenRect().Contains(screen))
        UpdateDrop(screen - GetScreenRect().TopLeft(),
                   UiDesignerNodesDragText(node_drag_nodes_));
    else
        ClearDrop();
}

Image UiDesignerHierarchyView::CursorImage(Point p, dword flags)
{
    return node_dragging_ ? Image::SizeAll()
                          : ParentCtrl::CursorImage(p, flags);
}

void UiDesignerHierarchyView::CancelMode()
{
    ResetNodeDrag();
    ParentCtrl::CancelMode();
}

void UiDesignerHierarchyView::MouseWheel(Point, int zdelta, dword)
{
    const int maximum = max(0,
        rows_.GetCount() * DPI(30) + GetHeaderRect().Height() - GetSize().cy);
    scroll_ = minmax(scroll_ - zdelta / 4, 0, maximum);
    Refresh();
}

void UiDesignerHierarchyView::ClearDrop()
{
    drop_row_ = -1;
    drop_edge_ = 0;
    drag_payload_.Clear();
    drop_plan_ = UiDesignerDropPlan();
    catalog_drag_ = false;
    Refresh();
}

void UiDesignerHierarchyView::UpdateDrop(Point p, const String& payload)
{
    if(!document_ || (!PlanDrop && !PlanCatalogDrop))
        return;
    Vector<UiDesignerNodeId> nodes;
    String catalog_type;
    const bool node_drag = UiDesignerParseNodesDragText(payload, nodes);
    const bool catalog_drag = UiDesignerParseCatalogDragText(payload, catalog_type);
    if(!node_drag && !catalog_drag) {
        ClearDrop();
        return;
    }

    // The heading is never a selectable row. During a catalog drag it is an
    // explicit document-root drop affordance, preserving the original
    // hierarchy workflow without reintroducing row-zero click aliasing.
    if(catalog_drag && GetHeaderRect().Contains(p)) {
        drop_plan_ = PlanCatalogDrop
            ? PlanCatalogDrop(catalog_type, document_->GetRootId(), -1)
            : UiDesignerDropPlan();
        drop_row_ = rows_.IsEmpty() ? -1 : 0;
        drop_edge_ = 0;
        drag_payload_ = payload;
        catalog_drag_ = true;
        if(WhenDropStatus)
            WhenDropStatus(drop_plan_.valid ? drop_plan_.label
                                            : drop_plan_.reason);
        Refresh();
        return;
    }

    const int row = RowAt(p);
    if(row < 0) {
        ClearDrop();
        return;
    }
    const UiDesignerNode* target = document_->Find(rows_[row].node);
    if(!target) {
        ClearDrop();
        return;
    }
    Rect rr = RowRect(row);
    const int third = max(1, rr.Height() / 3);
    drop_edge_ = p.y < rr.top + third ? -1
               : p.y >= rr.bottom - third ? 1 : 0;
    UiDesignerNodeId parent = target->id;
    int index = -1;
    if(drop_edge_ == 0) {
        drop_plan_ = node_drag && PlanDrop
            ? PlanDrop(nodes, target->id, -1)
            : PlanCatalogDrop ? PlanCatalogDrop(catalog_type, target->id, -1)
                              : UiDesignerDropPlan();
        if(!drop_plan_.valid && (!IsContentHost || !IsContentHost(target->id)))
            drop_edge_ = 1;
    }
    if(drop_edge_ != 0) {
        parent = target->parent;
        const UiDesignerNode* parent_node = document_->Find(parent);
        index = parent_node ? FindIndex(parent_node->children, target->id) : -1;
        if(drop_edge_ >= 0 && index >= 0)
            index++;
        drop_edge_ = drop_edge_ == 0 ? 1 : drop_edge_;
    }
    if(drop_edge_ != 0)
        drop_plan_ = node_drag && PlanDrop
            ? PlanDrop(nodes, parent, index)
            : PlanCatalogDrop ? PlanCatalogDrop(catalog_type, parent, index)
                              : UiDesignerDropPlan();
    drop_row_ = row;
    drag_payload_ = payload;
    catalog_drag_ = catalog_drag;
    if(WhenDropStatus)
        WhenDropStatus(drop_plan_.valid ? drop_plan_.label : drop_plan_.reason);
    Refresh();
}

void UiDesignerHierarchyView::PollNodeDrag()
{
    node_drag_poll_armed_ = false;
    if(!GetMouseLeft()) {
        ResetNodeDrag();
        return;
    }
    if(node_drag_nodes_.IsEmpty())
        return;
    const Point screen = GetMousePos();
    MouseMove(screen - GetScreenRect().TopLeft(), 0);
    if(!node_drag_nodes_.IsEmpty())
        ArmNodeDragPoll();
}

bool UiDesignerHierarchyView::FinishNodeDrop(Point screen)
{
    if(node_drag_cleanup_)
        return false;
    UiDesignerDropPlan plan;
    String status;
    bool execute = false;
    if(GetScreenRect().Contains(screen)) {
        UpdateDrop(screen - GetScreenRect().TopLeft(),
                   UiDesignerNodesDragText(node_drag_nodes_));
        execute = drop_plan_.valid;
        status = execute ? drop_plan_.label : drop_plan_.reason;
        if(execute)
            plan = pick(drop_plan_);
    }

    node_drag_cleanup_ = true;
    node_drag_poll_.Kill();
    node_drag_poll_armed_ = false;
    pressed_ = -1;
    node_drag_nodes_.Clear();
    node_dragging_ = false;
    ClearDrop();
    if(HasCapture())
        ReleaseCapture();
    node_drag_cleanup_ = false;

    String error;
    const bool ok = execute && ExecuteDrop && ExecuteDrop(plan, error);
    if(WhenDropStatus)
        WhenDropStatus(ok ? "Move completed"
                          : (error.IsEmpty() ? status : error));
    return ok;
}

void UiDesignerHierarchyView::ResetNodeDrag()
{
    if(node_drag_cleanup_)
        return;
    node_drag_cleanup_ = true;
    node_drag_poll_.Kill();
    node_drag_poll_armed_ = false;
    pressed_ = -1;
    node_drag_nodes_.Clear();
    node_dragging_ = false;
    ClearDrop();
    if(HasCapture())
        ReleaseCapture();
    node_drag_cleanup_ = false;
}

void UiDesignerHierarchyView::ArmNodeDragPoll()
{
    if(node_drag_poll_armed_ || node_drag_nodes_.IsEmpty())
        return;
    node_drag_poll_armed_ = true;
    node_drag_poll_arm_count_++;
    node_drag_poll_.KillSet(16, [=] { PollNodeDrag(); });
}

void UiDesignerHierarchyView::DragEnter()
{
    Refresh();
}

void UiDesignerHierarchyView::TrackCatalogDrop(const String& type_id,
                                               Point screen)
{
    UpdateDrop(screen - GetScreenRect().TopLeft(),
               UiDesignerCatalogDragText(type_id));
}

bool UiDesignerHierarchyView::FinishCatalogDrop(const String& type_id,
                                                Point screen)
{
    UpdateDrop(screen - GetScreenRect().TopLeft(),
               UiDesignerCatalogDragText(type_id));
    if(!drop_plan_.valid) {
        if(WhenDropStatus)
            WhenDropStatus(drop_plan_.reason);
        ClearDrop();
        return false;
    }
    String error;
    const bool ok = ExecuteDrop && ExecuteDrop(drop_plan_, error);
    if(WhenDropStatus)
        WhenDropStatus(ok ? "Control added" : error);
    ClearDrop();
    return ok;
}

void UiDesignerHierarchyView::CancelCatalogDrop()
{
    ClearDrop();
}

void UiDesignerHierarchyView::DragAndDrop(Point p, PasteClip& d)
{
    String payload;
    if(!UiDesignerReadDragText(d, payload)) {
        ClearDrop();
        return;
    }
    UpdateDrop(p, payload);
    if(!drop_plan_.valid) {
        ClearDrop();
        return;
    }
    if(!AcceptText(d)) {
        ClearDrop();
        return;
    }
    d.SetAction(catalog_drag_ ? DND_COPY : DND_MOVE);
    if(d.IsPaste()) {
        String error;
        const bool ok = ExecuteDrop && ExecuteDrop(drop_plan_, error);
        if(WhenDropStatus)
            WhenDropStatus(ok
                ? String(catalog_drag_ ? "Control added" : "Move completed")
                : error);
        ClearDrop();
    }
}

void UiDesignerHierarchyView::DragRepeat(Point p)
{
    if(!drag_payload_.IsEmpty())
        UpdateDrop(p, drag_payload_);
}

void UiDesignerHierarchyView::DragLeave()
{
    ClearDrop();
}

bool UiDesignerHierarchyView::Key(dword key, int)
{
    if(key == K_DELETE) {
        if(WhenDelete)
            WhenDelete();
        return true;
    }
    return ParentCtrl::Key(key, 1);
}

}
