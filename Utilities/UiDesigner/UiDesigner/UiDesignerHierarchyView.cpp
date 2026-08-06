#include "UiDesignerWidgets.h"
#include <Ui/UiIcons.h>

namespace Upp {

static UiTree::Style UiDesignerHierarchyTreeStyle()
{
    UiTree::Style style = UiTheme::ResolveTree();
    style.row_height = DPI(30);
    style.indent_px = DPI(16);
    style.glyph_size = DPI(10);
    style.icon_size = DPI(16);
    style.content_gap = DPI(5);
    style.h_padding = DPI(8);
    style.v_padding = DPI(2);
    style.row_radius = 0;
    style.accessory_gap = DPI(4);
    style.show_icons = true;
    style.show_connector_lines = false;
    style.show_metadata_marker = false;
    style.glyph_style = UITREEGLYPH_CHEVRON;
    style.metrics.frame_enabled = false;
    style.metrics.shadow.enabled = false;
    style.selected_face = Blend(SColorHighlight(), SColorPaper(), 80);
    style.selected_frame = style.selected_face;
    style.hot_face = Blend(SColorFace(), SColorPaper(), 55);
    style.hot_frame = style.hot_face;
    return style;
}

bool UiDesignerHierarchyView::HierarchyTree::Key(dword key, int count)
{
    if((key & ~(K_SHIFT | K_CTRL | K_ALT)) == K_ESCAPE) {
        CancelMode();
        return true;
    }
    if(key == K_DELETE) {
        if(WhenDelete)
            WhenDelete();
        return true;
    }
    return UiTree::Key(key, count);
}

UiDesignerHierarchyView::HierarchyTree::~HierarchyTree()
{
    ResetManualDrag(false);
}

void UiDesignerHierarchyView::HierarchyTree::LeftDown(Point p, dword flags)
{
    ResetManualDrag(false);
    const UiTreeNodeRef pressed = GetNodeAt(p);
    const bool accessory = p.x >= GetSize().cx - DPI(56);
    UiTree::LeftDown(p, flags);

    if(!pressed.IsValid() || accessory)
        return;
    drag_nodes_ = GetSelection();
    if(drag_nodes_.IsEmpty())
        return;
    drag_start_screen_ = GetMousePos();
    SetCapture();
    ArmManualDragPoll();
}

void UiDesignerHierarchyView::HierarchyTree::LeftUp(Point p, dword flags)
{
    const bool was_dragging = dragging_;
    const Vector<UiTreeNodeRef> nodes = clone(drag_nodes_);
    UiTree::DropInfo target;
    if(was_dragging && GetScreenRect().Contains(GetMousePos())) {
        TrackDropTarget(GetMousePos() - GetScreenRect().TopLeft());
        target = GetDropInfo();
    }

    ResetManualDrag(false);
    ReleaseManualCapture();

    if(was_dragging && target.valid && WhenManualDrop)
        WhenManualDrop(nodes, target);
    else if(was_dragging && WhenManualCancel)
        WhenManualCancel();
    else
        UiTree::LeftUp(p, flags);
}

void UiDesignerHierarchyView::HierarchyTree::LeftDrag(Point, dword)
{
    UpdateManualDrag();
}

void UiDesignerHierarchyView::HierarchyTree::MouseMove(Point p, dword flags)
{
    if(!drag_nodes_.IsEmpty()) {
        UpdateManualDrag();
        return;
    }
    UiTree::MouseMove(p, flags);
}

Image UiDesignerHierarchyView::HierarchyTree::CursorImage(Point p, dword flags)
{
    return dragging_ ? Image::SizeAll() : UiTree::CursorImage(p, flags);
}

void UiDesignerHierarchyView::HierarchyTree::CancelMode()
{
    if(cancelling_manual_drag_)
        return;
    cancelling_manual_drag_ = true;
    ResetManualDrag(true);
    ReleaseManualCapture();
    cancelling_manual_drag_ = false;
    UiTree::CancelMode();
}

void UiDesignerHierarchyView::HierarchyTree::UpdateManualDrag()
{
    if(drag_nodes_.IsEmpty())
        return;
    if(!GetMouseLeft()) {
        ResetManualDrag(true);
        ReleaseManualCapture();
        return;
    }

    const Point screen = GetMousePos();
    if(!dragging_ && Length(screen - drag_start_screen_) < DPI(5))
        return;
    dragging_ = true;

    if(GetScreenRect().Contains(screen)) {
        TrackDropTarget(screen - GetScreenRect().TopLeft());
        if(WhenManualDrag)
            WhenManualDrag(drag_nodes_, GetDropInfo());
    }
    else
        ClearTrackedDropTarget();
}

void UiDesignerHierarchyView::HierarchyTree::PollManualDrag()
{
    drag_poll_armed_ = false;
    UpdateManualDrag();
    if(!drag_nodes_.IsEmpty())
        ArmManualDragPoll();
}

void UiDesignerHierarchyView::HierarchyTree::ResetManualDrag(bool notify_cancel)
{
    if(resetting_manual_drag_)
        return;
    resetting_manual_drag_ = true;

    const bool active = !drag_nodes_.IsEmpty() || dragging_;
    drag_poll_.Kill();
    drag_poll_armed_ = false;
    drag_nodes_.Clear();
    dragging_ = false;
    ClearTrackedDropTarget();
    if(active && notify_cancel && WhenManualCancel)
        WhenManualCancel();

    resetting_manual_drag_ = false;
}

void UiDesignerHierarchyView::HierarchyTree::ReleaseManualCapture()
{
    if(releasing_manual_capture_)
        return;
    releasing_manual_capture_ = true;
    if(HasCapture())
        ReleaseCapture();
    releasing_manual_capture_ = false;
}

void UiDesignerHierarchyView::HierarchyTree::ArmManualDragPoll()
{
    if(drag_poll_armed_ || drag_nodes_.IsEmpty())
        return;
    drag_poll_armed_ = true;
    drag_poll_arm_count_++;
    drag_poll_.KillSet(16, [=] { PollManualDrag(); });
}

UiDesignerHierarchyView::UiDesignerHierarchyView()
{
    Add(tree_);
    tree_.SetSelectionMode(UITREESEL_MULTI)
         .SetRootVisible(false)
         .EnableInternalMutation(false)
         .EnableDragDrop(false)
         .EnableRenameOnDblClick(true)
         .ShowConnectorLines(false)
         .ShowMetadataMarker(false)
         .SetCustomStyle(UiDesignerHierarchyTreeStyle());

    Vector<int> widths;
    widths << DPI(94) << DPI(24) << DPI(24);
    tree_.SetColumnWidths(widths);

    tree_.WhenSelection = [=] { ForwardTreeSelection(); };
    tree_.WhenColumnAction = [=](UiTreeNodeRef node, int column) {
        const UiDesignerNodeId id = model_.FindDesignerNode(node);
        if(!id || column == 0 || !CycleSizingMode)
            return;
        if(column == 1 || column == 2)
            CycleSizingMode(id, column == 2);
    };
    const auto ExecuteHierarchyMove = [=](const Vector<UiTreeNodeRef>& refs,
                                          UiTree::DropInfo target) {
        if(!PlanDrop || !ExecuteDrop)
            return;
        Vector<UiDesignerNodeId> nodes;
        for(const UiTreeNodeRef& node : refs) {
            const UiDesignerNodeId id = model_.FindDesignerNode(node);
            if(id)
                nodes.Add(id);
        }
        UiDesignerNodeId parent = model_.FindDesignerNode(target.parent);
        if(!parent && document_)
            parent = document_->GetRootId();
        UiDesignerDropPlan plan = PlanDrop(nodes, parent, target.insert_pos);
        String error;
        const bool ok = plan.valid && ExecuteDrop(plan, error);
        if(WhenDropStatus)
            WhenDropStatus(ok ? "Move completed"
                              : (error.IsEmpty() ? plan.reason : error));
    };
    tree_.WhenManualDrag = [=](const Vector<UiTreeNodeRef>& refs,
                               UiTree::DropInfo target) {
        if(!PlanDrop)
            return;
        Vector<UiDesignerNodeId> nodes;
        for(const UiTreeNodeRef& node : refs) {
            const UiDesignerNodeId id = model_.FindDesignerNode(node);
            if(id)
                nodes.Add(id);
        }
        UiDesignerNodeId parent = model_.FindDesignerNode(target.parent);
        if(!parent && document_)
            parent = document_->GetRootId();
        const UiDesignerDropPlan plan = PlanDrop(nodes, parent, target.insert_pos);
        if(WhenDropStatus)
            WhenDropStatus(plan.valid ? plan.label : plan.reason);
    };
    tree_.WhenManualDrop = ExecuteHierarchyMove;
    tree_.WhenManualCancel = [=] {
        if(WhenDropStatus)
            WhenDropStatus("Move cancelled");
    };
    tree_.WhenRename = [=](UiTreeNodeRef node, const String& name) {
        const UiDesignerNodeId id = model_.FindDesignerNode(node);
        if(id && RenameNode)
            RenameNode(id, name);
    };
    tree_.WhenDelete = [=] {
        if(WhenDelete)
            WhenDelete();
    };
}

UiDesignerHierarchyView::~UiDesignerHierarchyView()
{
    tree_.ClearTrackedDropTarget();
}

void UiDesignerHierarchyView::CancelMode()
{
    tree_.CancelMode();
    CancelCatalogDrop();
    ParentCtrl::CancelMode();
}

void UiDesignerHierarchyView::SetDocument(const UiDesignerDocument *document)
{
    document_ = document;
    Rebuild();
}

void UiDesignerHierarchyView::SetCatalog(const UiDesignerCatalog *catalog)
{
    catalog_ = catalog;
    model_.SetCatalog(catalog);
    model_.SetIconResolver([](const String& key) {
        return UiDesignerResolveCatalogIcon(key);
    });
    Rebuild();
}

void UiDesignerHierarchyView::SetSelection(const UiDesignerSelection *selection)
{
    selection_ = selection;
    SyncSelectionFromDesigner();
}

void UiDesignerHierarchyView::Rebuild()
{
    if(!document_)
        return;

    Vector<UiDesignerNodeId> expanded;
    for(UiTreeNodeRef node : tree_.GetExpandedNodes()) {
        UiDesignerNodeId id = model_.FindDesignerNode(node);
        if(id)
            expanded.Add(id);
    }

    model_.Rebuild(*document_);
    tree_.SetModel(model_.GetModel());

    for(UiDesignerNodeId id : expanded) {
        UiTreeNodeRef node = model_.FindTreeNode(id);
        if(node.IsValid())
            tree_.Expand(node);
    }
    UiTreeNodeRef root = model_.FindTreeNode(document_->GetRootId());
    if(root.IsValid())
        tree_.Expand(root);

    SyncSelectionFromDesigner();
    Refresh();
}

void UiDesignerHierarchyView::SyncSelectionFromDesigner()
{
    if(!selection_)
        return;
    ValueArray selected;
    for(UiDesignerNodeId id : selection_->nodes)
        selected.Add((int64)id);
    syncing_selection_ = true;
    tree_.SetData(selected);
    syncing_selection_ = false;

    if(selection_->primary) {
        UiTreeNodeRef primary = model_.FindTreeNode(selection_->primary);
        if(primary.IsValid()) {
            UiTreeNodeRef p = primary;
            while(model_.GetModel().IsValid(p)) {
                UiTreeNodeRef parent = model_.GetModel().GetParent(p);
                if(!model_.GetModel().IsValid(parent))
                    break;
                tree_.Expand(parent);
                p = parent;
            }
            tree_.SetCursor(primary);
            tree_.ScrollTo(primary);
        }
    }
}

void UiDesignerHierarchyView::ForwardTreeSelection()
{
    if(syncing_selection_ || !WhenSelectNode)
        return;

    Vector<UiTreeNodeRef> selected = tree_.GetSelection();
    if(selected.IsEmpty())
        return;

    bool first = true;
    for(UiTreeNodeRef node : selected) {
        UiDesignerNodeId id = model_.FindDesignerNode(node);
        if(!id)
            continue;
        WhenSelectNode(id, !first);
        first = false;
    }
}

void UiDesignerHierarchyView::Layout()
{
    const Rect header = GetHeaderRect();
    tree_.SetRect(0, header.bottom, GetSize().cx,
                  max(0, GetSize().cy - header.bottom));
}

void UiDesignerHierarchyView::Paint(Draw& w)
{
    w.DrawRect(GetHeaderRect(), Blend(SColorFace(), SColorPaper(), 70));
    const Font normal = SansSerifZ(9);
    const Font bold = SansSerifZ(9).Bold();
    w.DrawText(DPI(8), DPI(5), "Name", bold, SColorText());
    w.DrawText(GetTypeRect(0).left + DPI(4), DPI(5), "Type", normal, SColorText());
    w.DrawText(GetWidthModeRect(0).left + DPI(7), DPI(5), "W", bold, SColorText());
    w.DrawText(GetHeightModeRect(0).left + DPI(7), DPI(5), "H", bold, SColorText());

    if(header_drop_) {
        const Color color = header_plan_.valid ? Color(34, 197, 94)
                                                : Color(220, 38, 38);
        Rect r = GetHeaderRect();
        w.DrawRect(r.left, r.bottom - DPI(2), r.Width(), DPI(2), color);
    }
}

Rect UiDesignerHierarchyView::GetHeaderRect() const
{
    return RectC(0, 0, GetSize().cx, DPI(24));
}

Rect UiDesignerHierarchyView::GetNameRect(int index) const
{
    Rect row = RectC(0, GetHeaderRect().bottom + index * DPI(30),
                     GetSize().cx, DPI(30));
    return Rect(row.left, row.top, GetTypeRect(index).left, row.bottom);
}

Rect UiDesignerHierarchyView::GetTypeRect(int index) const
{
    Rect row = RectC(0, GetHeaderRect().bottom + index * DPI(30),
                     GetSize().cx, DPI(30));
    return Rect(row.right - DPI(150), row.top, row.right - DPI(56), row.bottom);
}

Rect UiDesignerHierarchyView::GetWidthModeRect(int index) const
{
    Rect row = RectC(0, GetHeaderRect().bottom + index * DPI(30),
                     GetSize().cx, DPI(30));
    return RectC(row.right - DPI(52), row.top, DPI(24), row.Height());
}

Rect UiDesignerHierarchyView::GetHeightModeRect(int index) const
{
    Rect row = RectC(0, GetHeaderRect().bottom + index * DPI(30),
                     GetSize().cx, DPI(30));
    return RectC(row.right - DPI(24), row.top, DPI(24), row.Height());
}

void UiDesignerHierarchyView::UpdateCatalogDrop(const String& type_id, Point screen)
{
    if(!document_ || !PlanCatalogDrop)
        return;
    Point local = screen - GetScreenRect().TopLeft();
    if(GetHeaderRect().Contains(local)) {
        header_drop_ = true;
        header_plan_ = PlanCatalogDrop(type_id, document_->GetRootId(), -1);
        tree_.ClearTrackedDropTarget();
    }
    else {
        header_drop_ = false;
        UiTree::DropInfo info =
            tree_.TrackDropTarget(local - Point(0, GetHeaderRect().bottom));
        UiDesignerNodeId parent = model_.FindDesignerNode(info.parent);
        if(!parent)
            parent = document_->GetRootId();
        header_plan_ = info.valid
                     ? PlanCatalogDrop(type_id, parent, info.insert_pos)
                     : UiDesignerDropPlan();
    }
    if(WhenDropStatus)
        WhenDropStatus(header_plan_.valid ? header_plan_.label : header_plan_.reason);
    Refresh();
}

void UiDesignerHierarchyView::TrackCatalogDrop(const String& type_id, Point screen)
{
    UpdateCatalogDrop(type_id, screen);
}

bool UiDesignerHierarchyView::FinishCatalogDrop(const String& type_id, Point screen)
{
    UpdateCatalogDrop(type_id, screen);
    UiDesignerDropPlan plan = header_plan_;
    CancelCatalogDrop();
    if(!plan.valid || !ExecuteDrop)
        return false;
    String error;
    const bool ok = ExecuteDrop(plan, error);
    if(WhenDropStatus)
        WhenDropStatus(ok ? "Control added" : error);
    return ok;
}

void UiDesignerHierarchyView::CancelCatalogDrop()
{
    header_drop_ = false;
    header_plan_ = UiDesignerDropPlan();
    tree_.ClearTrackedDropTarget();
    Refresh();
}

}
