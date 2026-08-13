#include <Ui/UiList.h>
#include <Ui/UiTheme.h>

namespace Upp {

void UiList::LeftDown(Point p, dword flags)
{
    SetFocus();
    CommitRenameIfNeeded(p);
    SyncModel();
    int row = HitTestRow(p);
    pressed_ = row;
    pressed_drag_ = -1;
    if(row < 0) {
        Refresh();
        return;
    }
    if(!IsSelectableIndex(row)) {
        pressed_ = -1;
        Refresh();
        return;
    }

    int drag_row = HitTestDrag(p);
    if(drag_row >= 0) {
        pressed_drag_ = drag_row;
        if(!IsSelected(drag_row))
            SelectSingle(drag_row);
        cursor_ = drag_row;
        anchor_ = drag_row;
        BeginRowDrag(drag_row, GetMousePos());
        Refresh();
        return;
    }

    bool shift = (flags & K_SHIFT) != 0;
    bool ctrl = (flags & K_CTRL) != 0;

    if(selection_mode_ == UILISTSEL_MULTI) {
        if(shift)
            SelectRangeTo(row, ctrl);
        else if(ctrl)
            ToggleSelection(row);
        else
            SelectSingle(row);
    }
    else
        SelectSingle(row);

    cursor_ = row;
    anchor_ = row;
    ScrollTo(row);
    Refresh();
}

void UiList::LeftDrag(Point, dword)
{
    if(drag_candidate_)
        ContinueRowDrag(GetMousePos());
}

void UiList::LeftUp(Point, dword)
{
    if(drag_candidate_) {
        EndRowDrag(false);
        return;
    }

    if(pressed_ >= 0 || pressed_drag_ >= 0) {
        pressed_ = -1;
        pressed_drag_ = -1;
        Refresh();
    }
}

void UiList::LeftDouble(Point p, dword)
{
    CommitRenameIfNeeded(p);
    int row = HitTestRow(p);
    if(row < 0 || !model_)
        return;
    const UiModelItem& item = model_->Get(row);
    if(rename_on_dblclick_ && item.editable && item.enabled && !item.group_header)
        BeginRename(row);
    else if(WhenAction)
        WhenAction();
}

void UiList::MouseMove(Point p, dword)
{
    if(drag_candidate_) {
        ContinueRowDrag(GetMousePos());
        return;
    }

    int row = HitTestRow(p);
    int drag_row = HitTestDrag(p);
    if(hot_ != row || hot_drag_ != drag_row) {
        int old_hot = hot_;
        int old_drag = hot_drag_;
        hot_ = row;
        hot_drag_ = drag_row;
        if(old_hot >= 0)
            Refresh(GetRowRect(old_hot));
        if(hot_ >= 0)
            Refresh(GetRowRect(hot_));
        if(old_drag >= 0 && old_drag != old_hot)
            Refresh(GetRowRect(old_drag));
        if(hot_drag_ >= 0 && hot_drag_ != hot_)
            Refresh(GetRowRect(hot_drag_));
    }
}

void UiList::MouseLeave()
{
    if(drag_candidate_)
        return;
    if(hot_ >= 0 || hot_drag_ >= 0 || pressed_ >= 0 || pressed_drag_ >= 0) {
        hot_ = -1;
        hot_drag_ = -1;
        pressed_ = -1;
        pressed_drag_ = -1;
        Refresh();
    }
}

void UiList::MouseWheel(Point, int zdelta, dword)
{
    Rect vp = GetViewportRect();
    int extent = max(DPI(18), GetEffectiveStyle().row_height) + max(0, GetEffectiveStyle().item_spacing);
    int rows = max(1, vp.GetHeight() / max(1, extent));
    int step = max(1, rows / 2) * extent;
    scroll_y_ -= sgn(zdelta) * step;
    ClampScroll();
    Layout();
    Refresh();
}

bool UiList::Key(dword key, int)
{
    SyncModel();
    if(!model_ || model_->IsEmpty())
        return false;

    switch(key) {
    case K_UP: MoveCursorBy(-1); return true;
    case K_DOWN: MoveCursorBy(1); return true;
    case K_HOME: MoveCursorToEdge(false); return true;
    case K_END: MoveCursorToEdge(true); return true;
    case K_PAGEUP: {
        int extent = max(DPI(18), GetEffectiveStyle().row_height) + max(0, GetEffectiveStyle().item_spacing);
        int rows = max(1, GetViewportRect().GetHeight() / max(1, extent));
        MoveCursorBy(-rows);
        return true;
    }
    case K_PAGEDOWN: {
        int extent = max(DPI(18), GetEffectiveStyle().row_height) + max(0, GetEffectiveStyle().item_spacing);
        int rows = max(1, GetViewportRect().GetHeight() / max(1, extent));
        MoveCursorBy(rows);
        return true;
    }
    case K_ENTER:
    case K_SPACE:
        if(WhenAction)
            WhenAction();
        return true;
    case K_F2:
        if(cursor_ >= 0 && cursor_ < model_->GetCount() && model_->Get(cursor_).editable) {
            BeginRename(cursor_);
            return true;
        }
        break;
    case K_CTRL_A:
        if(selection_mode_ == UILISTSEL_MULTI) {
            SelectAll();
            return true;
        }
        break;
    }
    return false;
}

void UiList::GotFocus()
{
    Refresh();
}

void UiList::LostFocus()
{
    if(editing_ && !HasFocusDeep())
        CommitRename();
    Refresh();
}

void UiList::ScrollTo(int index)
{
    if(!model_ || index < 0 || index >= model_->GetCount())
        return;
    Rect vp = GetViewportRect();
    Rect row = GetRowRect(index);
    if(row.top < vp.top)
        scroll_y_ -= vp.top - row.top;
    else if(row.bottom > vp.bottom)
        scroll_y_ += row.bottom - vp.bottom;
    ClampScroll();
    Layout();
    Refresh();
}

bool UiList::IsSelectableIndex(int index) const
{
    if(!model_ || index < 0 || index >= model_->GetCount())
        return false;
    const UiModelItem& item = model_->Get(index);
    return item.enabled && !item.group_header;
}

void UiList::ScrollToSelection()
{
    if(cursor_ >= 0)
        ScrollTo(cursor_);
}

void UiList::MoveCursorBy(int delta)
{
    SyncModel();
    if(!model_ || model_->IsEmpty())
        return;
    int next = cursor_ >= 0 ? cursor_ + delta : (delta >= 0 ? 0 : model_->GetCount() - 1);
    next = clamp(next, 0, model_->GetCount() - 1);
    while(next >= 0 && next < model_->GetCount() && !IsSelectableIndex(next))
        next += delta >= 0 ? 1 : -1;
    if(IsSelectableIndex(next)) {
        SelectSingle(next);
        ScrollTo(next);
    }
}

void UiList::MoveCursorToEdge(bool end)
{
    SyncModel();
    if(!model_ || model_->IsEmpty())
        return;
    int index = end ? model_->GetCount() - 1 : 0;
    while(index >= 0 && index < model_->GetCount() && !IsSelectableIndex(index))
        index += end ? -1 : 1;
    if(IsSelectableIndex(index)) {
        SelectSingle(index);
        ScrollTo(index);
    }
}

void UiList::SelectSingle(int index)
{
    if(!IsSelectableIndex(index))
        return;
    selected_.Clear();
    selected_.FindAdd(index);
    cursor_ = index;
    anchor_ = index;
    NotifySelectionChange();
}

void UiList::ToggleSelection(int index)
{
    if(!IsSelectableIndex(index))
        return;
    int fi = selected_.Find(index);
    if(fi >= 0)
        selected_.Remove(fi);
    else
        selected_.FindAdd(index);
    cursor_ = index;
    anchor_ = index;
    NotifySelectionChange();
}

void UiList::SelectRangeTo(int index, bool additive)
{
    if(!IsSelectableIndex(index))
        return;
    int start = anchor_ >= 0 ? anchor_ : (cursor_ >= 0 ? cursor_ : index);
    if(!additive)
        selected_.Clear();
    int a = min(start, index);
    int b = max(start, index);
    for(int i = a; i <= b; i++)
        if(IsSelectableIndex(i))
            selected_.FindAdd(i);
    cursor_ = index;
    anchor_ = start;
    NotifySelectionChange();
}

Value UiList::GetSelectionToken(int index) const
{
    if(!model_ || index < 0 || index >= model_->GetCount())
        return Value();

    const UiModelItem& item = model_->Get(index);
    return IsNull(item.data) ? Value(index) : item.data;
}

int UiList::ResolveSelectionIndex(const Value& token) const
{
    if(!model_)
        return -1;

    // Preserve the existing token contract: explicit item data takes priority
    // over the row-index fallback even when the token is numeric.
    for(int i = 0; i < model_->GetCount(); i++) {
        const UiModelItem& item = model_->Get(i);
        if(!IsNull(item.data) && item.data == token && IsSelectableIndex(i))
            return i;
    }

    if(token.Is<int>()) {
        int index = token;
        return IsSelectableIndex(index) ? index : -1;
    }
    if(token.Is<int64>()) {
        int64 index = token;
        return index >= 0 && index <= INT_MAX && IsSelectableIndex((int)index) ? (int)index : -1;
    }
    return -1;
}

void UiList::NotifySelectionChange()
{
    Refresh();
    if(WhenSelection)
        WhenSelection();
}

bool UiList::CommitRenameIfNeeded(Point p)
{
    if(!editing_)
        return false;
    if(inline_editor_.IsShown() && inline_editor_.GetRect().Contains(p))
        return false;
    CommitRename();
    return true;
}

void UiList::BeginRename(int index)
{
    if(!model_ || index < 0 || index >= model_->GetCount())
        return;
    const UiModelItem& item = model_->Get(index);
    if(!item.editable)
        return;
    editing_ = true;
    editing_index_ = index;
    inline_editor_.SetData(item.text);
    inline_editor_.Show();
    inline_editor_.SetFocus();
    Layout();
    inline_editor_.SelectAll();
}

void UiList::CommitRename()
{
    if(!editing_ || !model_ || editing_index_ < 0 || editing_index_ >= model_->GetCount()) {
        CancelRename();
        return;
    }
    String text = AsString(inline_editor_.GetData());
    UiModelItem item = model_->Get(editing_index_);
    item.text = text;
    model_->Set(editing_index_, item);
    if(WhenRename)
        WhenRename(editing_index_, text);
    editing_ = false;
    editing_index_ = -1;
    inline_editor_.Hide();
    Refresh();
}

void UiList::CancelRename()
{
    editing_ = false;
    editing_index_ = -1;
    inline_editor_.Hide();
    Refresh();
}

int UiList::HitTestDrag(Point p) const
{
    int row = HitTestRow(p);
    if(row < 0 || !model_)
        return -1;
    Rect rr = GetRowRect(row).Deflated(DPI(2), DPI(1));
    return GetDragRect(rr).Contains(p) ? row : -1;
}

void UiList::BeginRowDrag(int row, Point start_screen)
{
    if(!drag_reorder_enabled_ || !model_ || row < 0 || row >= model_->GetCount() || model_->GetCount() < 2) {
        drag_candidate_ = false;
        return;
    }

    drag_candidate_ = true;
    dragging_ = false;
    drag_moved_ = false;
    drag_from_ = row;
    drag_insert_before_ = row;
    drag_start_screen_ = start_screen;
    drag_marker_.Hide();
}

int UiList::ComputeDragInsertBefore(int local_y) const
{
    if(!model_ || model_->IsEmpty())
        return 0;
    Rect vp = GetViewportRect();
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int sp = max(0, GetEffectiveStyle().item_spacing);
    int64 logical = (int64)local_y - vp.top + scroll_y_;
    return UiComputeUniformInsertBefore(model_->GetCount(), logical, rh, sp);
}

void UiList::ContinueRowDrag(Point p_screen)
{
    if(!drag_candidate_)
        return;

    if(!dragging_) {
        int dx = p_screen.x - drag_start_screen_.x;
        int dy = p_screen.y - drag_start_screen_.y;
        if(abs(dy) < drag_threshold_px_ || abs(dy) < abs(dx))
            return;
        dragging_ = true;
        drag_moved_ = true;
        drag_marker_.Show();
        drag_marker_.Remove();
        Add(drag_marker_);
    }

    Rect self = GetScreenRect();
    int local_y = p_screen.y - self.top;
    drag_insert_before_ = ComputeDragInsertBefore(local_y);
    UpdateDragMarker();
    Refresh();
}

void UiList::EndRowDrag(bool cancel)
{
    if(!drag_candidate_) {
        dragging_ = false;
        drag_moved_ = false;
        return;
    }

    if(!cancel && dragging_ && drag_from_ >= 0)
        MoveRowTo(drag_from_, drag_insert_before_);

    drag_candidate_ = false;
    dragging_ = false;
    drag_moved_ = false;
    drag_from_ = -1;
    drag_insert_before_ = -1;
    pressed_drag_ = -1;
    drag_marker_.Hide();
    Refresh();
}

void UiList::MoveRowTo(int from, int before)
{
    if(!model_ || from < 0 || from >= model_->GetCount())
        return;
    if(before < 0 || before > model_->GetCount())
        return;
    if(before == from || before == from + 1)
        return;

    const int original_before = before;
    UiReorderRequest request;
    request.from = from;
    request.before = before;
    if(WhenReorderRequest)
        WhenReorderRequest(request);
    if(!request.accept)
        return;
    if(request.handled) {
        SyncModel();
        Layout();
        Refresh();
        return;
    }
    if(!internal_mutation_enabled_)
        return;

    if(!model_->Move(from, before))
        return;

    Index<int> remapped;
    for(int i = 0; i < selected_.GetCount(); i++)
        remapped.FindAdd(RemapIndexAfterMove(selected_[i], from, before));
    selected_ = pick(remapped);
    cursor_ = RemapIndexAfterMove(cursor_, from, before);
    anchor_ = RemapIndexAfterMove(anchor_, from, before);
    hot_ = RemapIndexAfterMove(hot_, from, before);
    pressed_ = RemapIndexAfterMove(pressed_, from, before);
    hot_drag_ = RemapIndexAfterMove(hot_drag_, from, before);
    pressed_drag_ = RemapIndexAfterMove(pressed_drag_, from, before);
    editing_index_ = RemapIndexAfterMove(editing_index_, from, before);
    model_revision_ = model_->GetRevision();

    if(WhenReordered)
        WhenReordered(from, original_before);

    Layout();
    Refresh();
}

void UiList::UpdateDragMarker()
{
    if(!dragging_ || !model_ || drag_from_ < 0 || drag_from_ >= model_->GetCount()) {
        drag_marker_.Hide();
        return;
    }

    Rect vp = GetViewportRect();
    int line_y = vp.top;
    if(drag_insert_before_ >= 0 && drag_insert_before_ < model_->GetCount())
        line_y = GetRowRect(drag_insert_before_).top;
    else if(model_->GetCount() > 0)
        line_y = GetRowRect(model_->GetCount() - 1).bottom;

    int cy = DPI(2);
    int x = vp.left + GetEffectiveStyle().h_padding;
    int cx = max(DPI(24), vp.GetWidth() - GetEffectiveStyle().h_padding * 2);
    drag_marker_.Color(GetEffectiveStyle().drag_marker);
    drag_marker_.SetRect(x, line_y - cy / 2, cx, cy);
    drag_marker_.Show();
}

int UiList::RemapIndexAfterMove(int index, int from, int before) const
{
    if(index < 0)
        return index;
    if(before < from) {
        if(index == from)
            return before;
        if(index >= before && index < from)
            return index + 1;
        return index;
    }
    if(before > from + 1) {
        if(index == from)
            return before - 1;
        if(index > from && index < before)
            return index - 1;
        return index;
    }
    return index;
}

} // namespace Upp
