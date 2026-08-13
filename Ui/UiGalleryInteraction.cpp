#include <Ui/UiGallery.h>
#include <Ui/UiTheme.h>

namespace Upp {

void UiGallery::LeftDown(Point p, dword flags)
{
    SetFocus();
    SyncModel();
    if(!geometry_valid_)
        UpdateGeometry();
    int index = HitTestItem(p);
    pressed_ = index;
    if(!IsSelectableIndex(index)) {
        if(index < 0 && !(flags & K_CTRL) && !(flags & K_SHIFT))
            ClearSelection();
        Refresh();
        return;
    }

    bool shift = (flags & K_SHIFT) != 0;
    bool ctrl = (flags & K_CTRL) != 0;
    if(selection_mode_ == UIGALLERYSEL_MULTI) {
        if(shift)
            SelectRangeTo(index, ctrl);
        else if(ctrl)
            ToggleSelection(index);
        else
            SelectSingle(index);
    }
    else
        SelectSingle(index);

    cursor_ = index;
    if(!shift)
        anchor_ = index;
    ScrollTo(index);
}

void UiGallery::LeftUp(Point, dword)
{
    if(pressed_ >= 0) {
        int old = pressed_;
        pressed_ = -1;
        Refresh(GetItemRect(old));
    }
}

void UiGallery::LeftDouble(Point p, dword)
{
    int index = HitTestItem(p);
    if(IsSelectableIndex(index)) {
        SelectSingle(index);
        if(WhenAction)
            WhenAction();
    }
}

void UiGallery::MouseMove(Point p, dword)
{
    int next = HitTestItem(p);
    if(next == hot_)
        return;
    int old = hot_;
    hot_ = next;
    if(old >= 0)
        Refresh(GetItemRect(old));
    if(hot_ >= 0)
        Refresh(GetItemRect(hot_));
}

void UiGallery::MouseLeave()
{
    if(hot_ >= 0) {
        int old = hot_;
        hot_ = -1;
        Refresh(GetItemRect(old));
    }
    pressed_ = -1;
}

void UiGallery::MouseWheel(Point, int zdelta, dword)
{
    if(!geometry_valid_)
        UpdateGeometry();
    int extent = max(1, item_size_.cy + gap_);
    int rows = max(1, viewport_.GetHeight() / extent);
    int step = max(1, rows / 2) * extent;
    SetScrollPos(scroll_y_ - sgn(zdelta) * step);
}

bool UiGallery::Key(dword key, int)
{
    SyncModel();
    if(!model_ || model_->IsEmpty())
        return false;
    if(!geometry_valid_)
        UpdateGeometry();

    switch(key) {
    case K_LEFT:  MoveCursor(-1); return true;
    case K_RIGHT: MoveCursor(1); return true;
    case K_UP:    MoveCursorRows(-1); return true;
    case K_DOWN:  MoveCursorRows(1); return true;
    case K_HOME:
        SetCursor(0);
        return true;
    case K_END:
        SetCursor(model_->GetCount() - 1);
        return true;
    case K_PAGEUP: {
        int visible_rows = max(1, viewport_.GetHeight() / max(1, item_size_.cy + gap_));
        MoveCursorRows(-visible_rows);
        return true;
    }
    case K_PAGEDOWN: {
        int visible_rows = max(1, viewport_.GetHeight() / max(1, item_size_.cy + gap_));
        MoveCursorRows(visible_rows);
        return true;
    }
    case K_ENTER:
    case K_SPACE:
        if(cursor_ >= 0 && WhenAction)
            WhenAction();
        return true;
    case K_CTRL_A:
        if(selection_mode_ == UIGALLERYSEL_MULTI) {
            SelectAll();
            return true;
        }
        break;
    }
    return false;
}

void UiGallery::GotFocus()
{
    Refresh();
}

void UiGallery::LostFocus()
{
    pressed_ = -1;
    Refresh();
}

bool UiGallery::IsSelectableIndex(int index) const
{
    if(!model_ || index < 0 || index >= model_->GetCount())
        return false;
    const UiModelItem& item = model_->Get(index);
    return item.enabled && !item.group_header;
}

void UiGallery::SelectSingle(int index)
{
    if(!IsSelectableIndex(index))
        return;
    selected_.Clear();
    selected_.FindAdd(index);
    cursor_ = index;
    anchor_ = index;
    NotifySelectionChange();
}

void UiGallery::ToggleSelection(int index)
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

void UiGallery::SelectRangeTo(int index, bool additive)
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

void UiGallery::MoveCursor(int delta)
{
    if(!model_ || model_->IsEmpty())
        return;
    int next = cursor_ >= 0 ? cursor_ + delta : (delta >= 0 ? 0 : model_->GetCount() - 1);
    next = clamp(next, 0, model_->GetCount() - 1);
    int step = delta >= 0 ? 1 : -1;
    while(next >= 0 && next < model_->GetCount() && !IsSelectableIndex(next))
        next += step;
    if(IsSelectableIndex(next))
        SetCursor(next);
}

void UiGallery::MoveCursorRows(int rows)
{
    if(!model_ || model_->IsEmpty())
        return;
    int delta = rows * max(1, columns_);
    int next = cursor_ >= 0 ? cursor_ + delta : (rows >= 0 ? 0 : model_->GetCount() - 1);
    next = clamp(next, 0, model_->GetCount() - 1);
    int step = rows >= 0 ? 1 : -1;
    while(next >= 0 && next < model_->GetCount() && !IsSelectableIndex(next))
        next += step;
    if(IsSelectableIndex(next))
        SetCursor(next);
}

void UiGallery::NotifySelectionChange()
{
    Refresh();
    if(WhenSelection)
        WhenSelection();
}

Value UiGallery::GetSelectionToken(int index) const
{
    if(!model_ || index < 0 || index >= model_->GetCount())
        return Value();
    const UiModelItem& item = model_->Get(index);
    return IsNull(item.data) ? Value(index) : item.data;
}

int UiGallery::ResolveSelectionIndex(const Value& token) const
{
    if(!model_)
        return -1;

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

void UiGallery::SetData(const Value& value)
{
    SyncModel();
    if(IsNull(value)) {
        ClearSelection();
        return;
    }

    if(selection_mode_ == UIGALLERYSEL_MULTI || value.Is<ValueArray>()) {
        selected_.Clear();
        ValueArray values;
        if(value.Is<ValueArray>())
            values = value;
        else
            values.Add(value);
        for(int i = 0; i < values.GetCount(); i++) {
            int index = ResolveSelectionIndex(values[i]);
            if(index >= 0)
                selected_.FindAdd(index);
        }
        Vector<int> selection = GetSelection();
        anchor_ = selection.IsEmpty() ? -1 : selection[0];
        cursor_ = selection.IsEmpty() ? -1 : selection.Top();
        NotifySelectionChange();
        return;
    }

    int index = ResolveSelectionIndex(value);
    if(index >= 0)
        SelectSingle(index);
    else
        ClearSelection();
}

Value UiGallery::GetData() const
{
    if(selection_mode_ == UIGALLERYSEL_MULTI) {
        ValueArray values;
        Vector<int> selection = GetSelection();
        for(int i = 0; i < selection.GetCount(); i++)
            values.Add(GetSelectionToken(selection[i]));
        return values;
    }
    return selected_.GetCount() > 0 ? GetSelectionToken(selected_[0]) : Value();
}

} // namespace Upp
