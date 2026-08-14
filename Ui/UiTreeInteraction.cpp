#include <Ui/UiTree.h>

namespace Upp {

void UiTree::MoveCursorBy(int delta)
{
    if(visible_rows_.IsEmpty()) {
        SetCursor(UiTreeNodeRef{-1});
        return;
    }
    int row = FindVisibleRow(cursor_id_);
    if(row < 0)
        row = 0;
    else
        row = clamp(row + delta, 0, visible_rows_.GetCount() - 1);
    while(row >= 0 && row < visible_rows_.GetCount()
          && (visible_rows_[row].placeholder
              || !IsSelectableNode(UiTreeNodeRef{visible_rows_[row].id})))
        row += delta >= 0 ? 1 : -1;
    if(row >= 0 && row < visible_rows_.GetCount())
        SetCursor(UiTreeNodeRef{visible_rows_[row].id});
}

void UiTree::MoveCursorToEdge(bool end)
{
    if(visible_rows_.IsEmpty()) {
        SetCursor(UiTreeNodeRef{-1});
        return;
    }
    int row = end ? visible_rows_.GetCount() - 1 : 0;
    while(row >= 0 && row < visible_rows_.GetCount()
          && (visible_rows_[row].placeholder
              || !IsSelectableNode(UiTreeNodeRef{visible_rows_[row].id})))
        row += end ? -1 : 1;
    if(row >= 0 && row < visible_rows_.GetCount())
        SetCursor(UiTreeNodeRef{visible_rows_[row].id});
}

bool UiTree::CommitRenameIfNeeded(Point p)
{
    if(!editing_)
        return false;
    if(inline_editor_.IsShown() && inline_editor_.GetRect().Contains(p))
        return false;
    CommitRename();
    return true;
}

void UiTree::BeginRename(UiTreeNodeRef node)
{
    if(!rename_on_dblclick_ || !model_ || !model_->IsValid(node))
        return;
    const UiModelItem& item = model_->Get(node);
    if(!item.editable)
        return;
    editing_ = true;
    editing_id_ = node.id;
    inline_editor_.SetData(item.text);
    inline_editor_.Show();
    UpdateAttachedCtrls();
    inline_editor_.SetFocus();
    inline_editor_.SetSelection(0, item.text.GetCount());
}

void UiTree::CommitRename()
{
    if(!editing_ || !model_ || !model_->IsValid(UiTreeNodeRef{editing_id_})) {
        CancelRename();
        return;
    }
    UiTreeNodeRef node{editing_id_};
    UiModelItem item = model_->Get(node);
    String text = AsString(inline_editor_.GetData());
    bool changed = item.text != text;
    if(changed) {
        item.text = text;
        model_->Set(node, item);
    }
    editing_ = false;
    editing_id_ = -1;
    inline_editor_.Hide();
    Refresh();
    if(changed && WhenRename)
        WhenRename(node, text);
}

void UiTree::CancelRename()
{
    editing_ = false;
    editing_id_ = -1;
    inline_editor_.Hide();
}

void UiTree::LeftDown(Point p, dword flags)
{
    CommitRenameIfNeeded(p);
    SetFocus();
    pressed_ = true;
    drag_id_ = -1;
    int row = HitTestRow(p);
    if(row >= 0) {
        const VisibleRow& vr = visible_rows_[row];
        if(vr.placeholder) {
            Refresh();
            return;
        }
        UiTreeNodeRef node{vr.id};
        Rect rr = GetRowRect(row);
        const UiModelItem& item = model_->Get(node);
        int column_hit = HitTestColumn(rr, item, p);
        bool toggle_hit = vr.has_children
                       && GetToggleHitRect(rr, vr.depth, vr.has_children).Contains(p);
        if(toggle_hit)
            Toggle(node);

        if(!IsSelectableNode(node)) {
            Refresh();
            return;
        }

        if(column_hit >= 0) {
            pressed_ = false;
            Refresh();
            if(WhenColumnAction)
                WhenColumnAction(node, column_hit);
            return;
        }

        bool shift = (flags & K_SHIFT) != 0;
        bool ctrl = (flags & K_CTRL) != 0;
        SetCursor(node);
        if(selection_mode_ == UITREESEL_SINGLE)
            SelectSingle(node);
        else if(shift)
            SelectRangeTo(node, ctrl);
        else if(ctrl)
            ToggleSelection(node);
        else
            SelectSingle(node);

        if(dnd_enabled_ && !toggle_hit) {
            drag_id_ = node.id;
            SetCapture();
        }
    }
    else if((flags & (K_SHIFT | K_CTRL)) == 0)
        ClearSelection();
    Refresh();
}

void UiTree::LeftDouble(Point p, dword flags)
{
    int row = HitTestRow(p);
    if(row < 0)
        return;
    const VisibleRow& vr = visible_rows_[row];
    if(vr.placeholder)
        return;
    UiTreeNodeRef node{vr.id};
    const UiModelItem& item = model_->Get(node);
    Rect rr = GetRowRect(row);
    bool has_icon = GetEffectiveStyle().show_icons && !IsNull(item.icon);
    bool has_metadata = GetEffectiveStyle().show_metadata_marker && item.has_metadata;
    Rect tr = GetTextRect(rr, vr.depth, vr.has_children, has_icon, has_metadata, vr.id);
    LeftDown(p, flags);
    if(item.editable && tr.Contains(p)) {
        BeginRename(node);
        return;
    }
    if(vr.has_children)
        Toggle(node);
    if(WhenAction)
        WhenAction();
}

void UiTree::LeftUp(Point p, dword)
{
    int row = HitTestRow(p);
    pressed_ = false;
    drag_id_ = -1;
    if(HasCapture())
        ReleaseCapture();
    if(row >= 0 && !visible_rows_[row].placeholder)
        hot_id_ = visible_rows_[row].id;
    Refresh();
}

void UiTree::LeftDrag(Point, dword)
{
    if(!dnd_enabled_ || editing_ || drag_id_ < 0)
        return;
    Vector<UiTreeNodeRef> nodes = GetDragNodes(UiTreeNodeRef{drag_id_});
    if(nodes.IsEmpty())
        return;
    pressed_ = false;
    if(HasCapture())
        ReleaseCapture();
    DoDragAndDrop(InternalClip(*this, "uitree-node"), BuildDragSample(nodes), DND_MOVE);
    drag_id_ = -1;
    ClearDropTarget();
}

void UiTree::MouseMove(Point p, dword)
{
    int tip_row = HitTestRow(p);
    if(tip_row >= 0 && tip_row < visible_rows_.GetCount() && !visible_rows_[tip_row].placeholder) {
        UiTreeNodeRef tip_node{visible_rows_[tip_row].id};
        if(model_ && model_->IsValid(tip_node)) {
            Rect rr = GetRowRect(tip_row);
            const UiModelItem& tip_item = model_->Get(tip_node);
            int tip_column = HitTestColumn(rr, tip_item, p);
            if(tip_column >= 0 && tip_column < tip_item.columns.GetCount())
                Tip(tip_item.columns[tip_column].tooltip);
            else
                Tip(tip_item.description);
        }
    }
    int row = HitTestRow(p);
    int id = row >= 0 && !visible_rows_[row].placeholder ? visible_rows_[row].id : -1;
    if(hot_id_ != id) {
        hot_id_ = id;
        Refresh();
    }
}

void UiTree::MouseLeave()
{
    if(hot_id_ >= 0 || pressed_) {
        hot_id_ = -1;
        pressed_ = false;
        Refresh();
    }
}

void UiTree::MouseWheel(Point p, int zdelta, dword)
{
    CommitRenameIfNeeded(p);
    Rect vp = GetViewportRect();
    if(vp.IsEmpty())
        return;
    int rows = max(1, vp.GetHeight() / max(DPI(18), GetEffectiveStyle().row_height));
    int step = max(1, rows / 2) * max(DPI(18), GetEffectiveStyle().row_height);
    if(zdelta > 0)
        scroll_y_ -= step;
    else if(zdelta < 0)
        scroll_y_ += step;
    ClampScroll();
    PrepareItemRenders();
    UpdateAttachedCtrls();
    Refresh();
}

void UiTree::DragEnter()
{
    Refresh();
}

void UiTree::DragAndDrop(Point p, PasteClip& d)
{
    if(!dnd_enabled_ || editing_ || !AcceptInternal<UiTree>(d, "uitree-node")) {
        ClearDropTarget();
        return;
    }
    const UiTree* src = GetInternalPtr<UiTree>(d, "uitree-node");
    if(!src || src != this || drag_id_ < 0) {
        ClearDropTarget();
        return;
    }

    Vector<UiTreeNodeRef> nodes = GetDragNodes(UiTreeNodeRef{drag_id_});
    DropTarget target = GetDropTarget(p);
    if(!target.valid || !CanMoveNodes(nodes, UiTreeNodeRef{target.parent_id}, target.insert_pos)) {
        ClearDropTarget();
        return;
    }

    d.SetAction(DND_MOVE);
    SetDropTarget(target);
    if(d.IsPaste()) {
        MoveNodes(nodes, UiTreeNodeRef{target.parent_id}, target.insert_pos);
        ClearDropTarget();
        drag_id_ = -1;
    }
}

void UiTree::DragRepeat(Point p)
{
    Rect vp = GetViewportRect();
    if(!vp.IsEmpty()) {
        int step = max(DPI(12), GetEffectiveStyle().row_height / 2);
        if(p.y <= vp.top + DPI(12))
            scroll_y_ -= step;
        else if(p.y >= vp.bottom - DPI(12))
            scroll_y_ += step;
        ClampScroll();
        PrepareItemRenders();
        UpdateAttachedCtrls();
    }
    if(drag_id_ >= 0) {
        Vector<UiTreeNodeRef> nodes = GetDragNodes(UiTreeNodeRef{drag_id_});
        DropTarget target = GetDropTarget(p);
        if(target.valid && CanMoveNodes(nodes, UiTreeNodeRef{target.parent_id}, target.insert_pos))
            SetDropTarget(target);
        else
            ClearDropTarget();
    }
}

void UiTree::DragLeave()
{
    ClearDropTarget();
}

bool UiTree::Key(dword key, int)
{
    SyncModel();
    if(editing_ || visible_rows_.IsEmpty())
        return false;
    if(cursor_id_ < 0)
        cursor_id_ = visible_rows_[0].id;

    if(key == K_CTRL_A && selection_mode_ == UITREESEL_MULTI) {
        SelectAllVisible();
        return true;
    }

    bool shift = (key & K_SHIFT) != 0;
    dword base = key & ~(K_SHIFT | K_CTRL | K_ALT);
    int before = cursor_id_;
    UiTreeNodeRef cur{cursor_id_};

    switch(base) {
    case K_UP: MoveCursorBy(-1); break;
    case K_DOWN: MoveCursorBy(1); break;
    case K_HOME: MoveCursorToEdge(false); break;
    case K_END: MoveCursorToEdge(true); break;
    case K_PAGEUP: {
        int rows = max(1, GetViewportRect().GetHeight()
                          / max(DPI(18), GetEffectiveStyle().row_height));
        MoveCursorBy(-rows);
        break;
    }
    case K_PAGEDOWN: {
        int rows = max(1, GetViewportRect().GetHeight()
                          / max(DPI(18), GetEffectiveStyle().row_height));
        MoveCursorBy(rows);
        break;
    }
    case K_LEFT:
        if((model_->GetChildCount(cur) > 0 || model_->Get(cur).lazy_children || IsNodeLoading(cur))
           && IsExpanded(cur))
            Collapse(cur);
        else {
            UiTreeNodeRef parent = model_->GetParent(cur);
            if(model_->IsValid(parent) && (root_visible_ || parent.id != model_->Root().id))
                SetCursor(parent);
        }
        break;
    case K_RIGHT:
        if(model_->GetChildCount(cur) > 0 || model_->Get(cur).lazy_children || IsNodeLoading(cur)) {
            if(!IsExpanded(cur))
                Expand(cur);
            else if(model_->GetChildCount(cur) > 0)
                SetCursor(model_->GetChild(cur, 0));
        }
        break;
    case K_F2:
        BeginRename(cur);
        return true;
    case K_SPACE:
        if(selection_mode_ == UITREESEL_MULTI) {
            ToggleSelection(UiTreeNodeRef{cursor_id_});
            return true;
        }
        if(model_->GetChildCount(cur) > 0 || model_->Get(cur).lazy_children || IsNodeLoading(cur))
            Toggle(cur);
        else if(WhenAction)
            WhenAction();
        return true;
    case K_ENTER:
        if(WhenAction)
            WhenAction();
        return true;
    default:
        return false;
    }

    if(before != cursor_id_) {
        if(selection_mode_ == UITREESEL_MULTI && shift)
            SelectRangeTo(UiTreeNodeRef{cursor_id_}, false);
        else
            SelectSingle(UiTreeNodeRef{cursor_id_});
    }
    return true;
}

void UiTree::SelectSingle(UiTreeNodeRef node)
{
    selected_ids_.Clear();
    if(IsSelectableNode(node)) {
        cursor_id_ = node.id;
        selected_ids_.FindAdd(node.id);
        anchor_id_ = node.id;
    }
    else
        cursor_id_ = anchor_id_ = -1;
    NotifySelectionChange();
}

void UiTree::ToggleSelection(UiTreeNodeRef node)
{
    if(!IsSelectableNode(node))
        return;
    int q = selected_ids_.Find(node.id);
    if(q >= 0)
        selected_ids_.Remove(q);
    else
        selected_ids_.FindAdd(node.id);
    cursor_id_ = anchor_id_ = node.id;
    NotifySelectionChange();
}

void UiTree::SelectRangeTo(UiTreeNodeRef node, bool additive)
{
    if(!IsSelectableNode(node))
        return;
    if(anchor_id_ < 0)
        anchor_id_ = cursor_id_ >= 0 ? cursor_id_ : node.id;
    int a = FindVisibleRow(anchor_id_);
    int b = FindVisibleRow(node.id);
    if(a < 0 || b < 0) {
        SelectSingle(node);
        return;
    }
    if(!additive)
        selected_ids_.Clear();
    for(int i = min(a, b); i <= max(a, b); i++)
        if(!visible_rows_[i].placeholder && IsSelectableNode(UiTreeNodeRef{visible_rows_[i].id}))
            selected_ids_.FindAdd(visible_rows_[i].id);
    cursor_id_ = node.id;
    NotifySelectionChange();
}

bool UiTree::IsSelectableNode(UiTreeNodeRef node) const
{
    if(!model_ || !model_->IsValid(node))
        return false;
    const UiModelItem& item = model_->Get(node);
    return item.enabled && !item.group_header;
}

Value UiTree::GetSelectionToken(UiTreeNodeRef node) const
{
    if(!model_ || !model_->IsValid(node))
        return Value();
    const UiModelItem& item = model_->Get(node);
    return IsNull(item.data) ? Value(node.id) : item.data;
}

UiTreeNodeRef UiTree::ResolveSelectionNode(const Value& token) const
{
    if(!model_ || !model_->IsValid(model_->Root()))
        return UiTreeNodeRef{-1};

    Vector<int> stack;
    stack.Add(model_->Root().id);
    while(!stack.IsEmpty()) {
        int id = stack.Top();
        stack.Drop();
        UiTreeNodeRef node{id};
        const UiModelItem& item = model_->Get(node);
        if(!IsNull(item.data) && item.data == token && IsSelectableNode(node))
            return node;
        for(int i = model_->GetChildCount(node) - 1; i >= 0; i--)
            stack.Add(model_->GetChild(node, i).id);
    }

    if(token.Is<int>()) {
        UiTreeNodeRef node{(int)token};
        return IsSelectableNode(node) ? node : UiTreeNodeRef{-1};
    }
    if(token.Is<int64>()) {
        UiTreeNodeRef node{(int)(int64)token};
        return IsSelectableNode(node) ? node : UiTreeNodeRef{-1};
    }
    return UiTreeNodeRef{-1};
}

void UiTree::NotifySelectionChange()
{
    Refresh();
    if(WhenSelection)
        WhenSelection();
}

Vector<UiTreeNodeRef> UiTree::GetDragNodes(UiTreeNodeRef primary) const
{
    Vector<UiTreeNodeRef> nodes;
    if(!model_ || !model_->IsValid(primary))
        return nodes;
    if(selection_mode_ == UITREESEL_MULTI && IsSelected(primary))
        nodes = GetSelection();
    else
        nodes.Add(primary);

    Sort(nodes, [=](const UiTreeNodeRef& a, const UiTreeNodeRef& b) {
        int ia = FindVisibleRow(a.id);
        int ib = FindVisibleRow(b.id);
        if(ia >= 0 && ib >= 0)
            return ia < ib;
        return a.id < b.id;
    });

    Vector<UiTreeNodeRef> out;
    for(int i = 0; i < nodes.GetCount(); i++) {
        UiTreeNodeRef node = nodes[i];
        bool nested = false;
        UiTreeNodeRef parent = model_->GetParent(node);
        while(model_->IsValid(parent)) {
            for(int j = 0; j < out.GetCount(); j++)
                if(out[j].id == parent.id) {
                    nested = true;
                    break;
                }
            if(nested)
                break;
            parent = model_->GetParent(parent);
        }
        if(!nested)
            out.Add(node);
    }
    return out;
}

bool UiTree::CanMoveNodes(const Vector<UiTreeNodeRef>& nodes, UiTreeNodeRef new_parent, int) const
{
    if(!model_ || !model_->IsValid(new_parent) || nodes.IsEmpty())
        return false;
    for(int i = 0; i < nodes.GetCount(); i++) {
        UiTreeNodeRef node = nodes[i];
        if(!model_->IsValid(node) || node.id == model_->Root().id || node.id == new_parent.id)
            return false;
        UiTreeNodeRef walk = new_parent;
        while(model_->IsValid(walk)) {
            if(walk.id == node.id)
                return false;
            walk = model_->GetParent(walk);
        }
    }
    return true;
}

bool UiTree::MoveNodes(const Vector<UiTreeNodeRef>& nodes, UiTreeNodeRef new_parent, int pos)
{
    if(!CanMoveNodes(nodes, new_parent, pos))
        return false;

    UiTreeMoveRequest request;
    request.nodes = clone(nodes);
    request.new_parent = new_parent;
    request.insert_pos = pos;
    if(WhenMoveRequest)
        WhenMoveRequest(request);
    if(!request.accept)
        return false;
    if(request.handled) {
        SyncModel();
        RefreshLayout();
        Refresh();
        return true;
    }
    if(!internal_mutation_enabled_)
        return false;

    int insert_pos = pos < 0 ? model_->GetChildCount(new_parent)
                             : min(max(pos, 0), model_->GetChildCount(new_parent));
    for(int i = 0; i < nodes.GetCount(); i++) {
        UiTreeNodeRef node = nodes[i];
        UiTreeNodeRef old_parent = model_->GetParent(node);
        int child_index = model_->GetChildIndex(node);
        if(old_parent.id == new_parent.id && child_index >= 0 && child_index < insert_pos)
            insert_pos--;
        if(!model_->Move(node, new_parent, insert_pos))
            return false;
        insert_pos++;
    }

    expanded_ids_.FindAdd(new_parent.id);
    selected_ids_.Clear();
    for(int i = 0; i < nodes.GetCount(); i++)
        if(model_->IsValid(nodes[i]))
            selected_ids_.FindAdd(nodes[i].id);
    anchor_id_ = nodes.IsEmpty() ? -1 : nodes[0].id;
    SetCursor(nodes.IsEmpty() ? UiTreeNodeRef{-1} : nodes[0]);
    NotifySelectionChange();
    return true;
}

UiTree::DropTarget UiTree::GetDropTarget(Point p) const
{
    DropTarget target;
    if(!model_ || !model_->IsValid(model_->Root()))
        return target;

    Rect vp = GetViewportRect();
    int row = HitTestRow(p);
    if(row < 0) {
        if(!vp.IsEmpty() && p.x >= vp.left && p.x <= vp.right) {
            target.parent_id = model_->Root().id;
            target.insert_pos = model_->GetChildCount(model_->Root());
            target.valid = true;
        }
        return target;
    }

    const VisibleRow& vr = visible_rows_[row];
    UiTreeNodeRef node{vr.id};
    if(!model_->IsValid(node))
        return target;
    if(vr.placeholder) {
        target.parent_id = node.id;
        target.insert_pos = model_->GetChildCount(node);
        target.hover_id = node.id;
        target.into = true;
        target.valid = true;
        return target;
    }

    Rect rr = GetRowRect(row);
    int quarter = max(DPI(4), rr.GetHeight() / 4);
    if(p.y <= rr.top + quarter) {
        UiTreeNodeRef parent = model_->GetParent(node);
        if(model_->IsValid(parent)) {
            target.parent_id = parent.id;
            target.insert_pos = model_->GetChildIndex(node);
            target.hover_id = node.id;
            target.valid = target.insert_pos >= 0;
        }
    }
    else if(p.y >= rr.bottom - quarter) {
        UiTreeNodeRef parent = model_->GetParent(node);
        if(model_->IsValid(parent)) {
            target.parent_id = parent.id;
            target.insert_pos = model_->GetChildIndex(node) + 1;
            target.hover_id = node.id;
            target.valid = target.insert_pos > 0;
        }
    }
    else {
        target.parent_id = node.id;
        target.insert_pos = model_->GetChildCount(node);
        target.hover_id = node.id;
        target.into = true;
        target.valid = true;
    }
    return target;
}

void UiTree::SetDropTarget(const DropTarget& target)
{
    if(drop_parent_id_ == target.parent_id && drop_insert_pos_ == target.insert_pos
       && drop_hover_id_ == target.hover_id && drop_into_ == target.into)
        return;
    drop_parent_id_ = target.parent_id;
    drop_insert_pos_ = target.insert_pos;
    drop_hover_id_ = target.hover_id;
    drop_into_ = target.into;
    Refresh();
}

void UiTree::ClearDropTarget()
{
    if(drop_parent_id_ < 0 && drop_insert_pos_ < 0 && drop_hover_id_ < 0 && !drop_into_)
        return;
    drop_parent_id_ = -1;
    drop_insert_pos_ = -1;
    drop_hover_id_ = -1;
    drop_into_ = false;
    Refresh();
}

Image UiTree::BuildDragSample(const Vector<UiTreeNodeRef>& nodes) const
{
    String label;
    Font font = StdFont().Bold();
    if(nodes.GetCount() == 1 && model_ && model_->IsValid(nodes[0]))
        label = model_->Get(nodes[0]).text;
    else
        label = Format("%d items", nodes.GetCount());

    Size tsz = GetTextSize(label, font);
    Size sz(max(DPI(96), tsz.cx + DPI(20)), DPI(28));
    ImageDraw iw(sz);
    iw.DrawRect(sz, Color(17, 24, 39));
    iw.DrawRect(1, 1, sz.cx - 2, sz.cy - 2, White());
    DrawSmartText(iw, DPI(10), (sz.cy - font.GetHeight()) / 2,
                  max(0, sz.cx - DPI(20)), label, font, Color(17, 24, 39), 0);
    return iw;
}

void UiTree::GotFocus()
{
    Refresh();
}

void UiTree::LostFocus()
{
    if(!editing_)
        Refresh();
}

} // namespace Upp
