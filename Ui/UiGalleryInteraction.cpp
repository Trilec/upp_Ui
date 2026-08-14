#include <Ui/UiGallery.h>
#include <Ui/UiTheme.h>

namespace Upp {

Point UiGallery::ToContentPoint(Point p) const
{
    return Point(p.x - viewport_.left,
                 p.y - viewport_.top + scroll_y_);
}

Rect UiGallery::GetMarqueeContentRect() const
{
    int left = min(marquee_start_content_.x, marquee_current_content_.x);
    int top = min(marquee_start_content_.y, marquee_current_content_.y);
    int right = max(marquee_start_content_.x, marquee_current_content_.x) + 1;
    int bottom = max(marquee_start_content_.y, marquee_current_content_.y) + 1;
    return Rect(left, top, right, bottom);
}

Rect UiGallery::GetMarqueeRect() const
{
    if(!marquee_active_)
        return Rect(0, 0, 0, 0);
    Rect r = GetMarqueeContentRect();
    r.Offset(viewport_.left, viewport_.top - scroll_y_);
    return r;
}

void UiGallery::BeginMarquee(Point p, dword flags)
{
    if(selection_mode_ != UIGALLERYSEL_MULTI || !viewport_.Contains(p))
        return;

    marquee_candidate_ = true;
    marquee_active_ = false;
    marquee_start_content_ = marquee_current_content_ = ToContentPoint(p);
    marquee_flags_ = flags;
    marquee_open_selection_ = GetSelection();
    marquee_open_anchor_ = anchor_;
}

void UiGallery::AutoScrollMarquee(Point p)
{
    if(!marquee_active_ || viewport_.IsEmpty())
        return;
    int margin = min(DPI(28), max(DPI(12), viewport_.GetHeight() / 5));
    int step = max(DPI(8), (item_size_.cy + gap_) / 3);
    if(p.y < viewport_.top + margin)
        SetScrollPos(scroll_y_ - step);
    else if(p.y >= viewport_.bottom - margin)
        SetScrollPos(scroll_y_ + step);
}

void UiGallery::UpdateMarquee(Point p, dword)
{
    if(!marquee_candidate_ && !marquee_active_)
        return;

    if(!marquee_active_) {
        Point current = ToContentPoint(p);
        if(abs(current.x - marquee_start_content_.x) < marquee_threshold_ &&
           abs(current.y - marquee_start_content_.y) < marquee_threshold_)
            return;
        marquee_active_ = true;
        marquee_candidate_ = false;
        SetCapture();
    }

    AutoScrollMarquee(p);
    marquee_current_content_ = ToContentPoint(p);
    UpdateMarqueeSelection();
    Refresh();
}

void UiGallery::UpdateMarqueeSelection()
{
    if(!marquee_active_ || !model_ || model_->IsEmpty())
        return;

    Index<int> next;
    bool ctrl = (marquee_flags_ & K_CTRL) != 0;
    bool shift = (marquee_flags_ & K_SHIFT) != 0;
    if(ctrl || shift)
        for(int i = 0; i < marquee_open_selection_.GetCount(); i++)
            if(IsSelectableIndex(marquee_open_selection_[i]))
                next.FindAdd(marquee_open_selection_[i]);

    Rect band = GetMarqueeContentRect();
    int col_extent = max(1, item_size_.cx + gap_);
    int row_extent = max(1, item_size_.cy + gap_);
    int first_col = max(0, max(0, band.left - inset_.left) / col_extent - 1);
    int last_col = min(columns_ - 1, max(0, band.right - inset_.left) / col_extent + 1);
    int first_row = max(0, max(0, band.top - inset_.top) / row_extent - 1);
    int last_row = min(rows_ - 1, max(0, band.bottom - inset_.top) / row_extent + 1);

    int last_hit = -1;
    for(int row = first_row; row <= last_row; row++) {
        for(int col = first_col; col <= last_col; col++) {
            int index = row * columns_ + col;
            if(!IsSelectableIndex(index))
                continue;
            Rect tile = RectC(inset_.left + col * col_extent,
                              inset_.top + row * row_extent,
                              item_size_.cx, item_size_.cy);
            if(!tile.Intersects(band))
                continue;

            if(ctrl) {
                int fi = next.Find(index);
                if(fi >= 0)
                    next.Remove(fi);
                else
                    next.FindAdd(index);
            }
            else
                next.FindAdd(index);
            last_hit = index;
        }
    }

    selected_ = pick(next);
    if(last_hit >= 0)
        cursor_ = last_hit;
    if(shift)
        anchor_ = marquee_open_anchor_;
    else if(last_hit >= 0)
        anchor_ = last_hit;
    NotifySelectionChange();
}

void UiGallery::EndMarquee(bool cancel)
{
    if(cancel && (marquee_candidate_ || marquee_active_)) {
        selected_.Clear();
        for(int i = 0; i < marquee_open_selection_.GetCount(); i++)
            if(IsSelectableIndex(marquee_open_selection_[i]))
                selected_.FindAdd(marquee_open_selection_[i]);
        anchor_ = marquee_open_anchor_;
        cursor_ = selected_.IsEmpty() ? -1 : selected_.Top();
    }

    marquee_candidate_ = false;
    marquee_active_ = false;
    marquee_open_selection_.Clear();
    if(HasCapture())
        ReleaseCapture();
    Refresh();
    if(cancel && WhenSelection)
        WhenSelection();
}

void UiGallery::LeftDown(Point p, dword flags)
{
    SetFocus();
    SyncModel();
    if(!geometry_valid_)
        UpdateGeometry();

    int index = HitTestItem(p);
    pressed_ = index;
    if(!IsSelectableIndex(index)) {
        pressed_ = -1;
        if(index < 0 && selection_mode_ == UIGALLERYSEL_MULTI)
            BeginMarquee(p, flags);
        else if(index < 0 && !(flags & K_CTRL) && !(flags & K_SHIFT))
            ClearSelection();
        Refresh();
        return;
    }

    EndMarquee(true);
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

void UiGallery::LeftDrag(Point p, dword flags)
{
    UpdateMarquee(p, flags);
}

void UiGallery::LeftUp(Point, dword)
{
    if(marquee_active_) {
        EndMarquee(false);
        return;
    }
    if(marquee_candidate_) {
        bool preserve = (marquee_flags_ & (K_CTRL | K_SHIFT)) != 0;
        EndMarquee(false);
        if(!preserve)
            ClearSelection();
        return;
    }

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

void UiGallery::MouseMove(Point p, dword flags)
{
    if(marquee_candidate_ || marquee_active_) {
        UpdateMarquee(p, flags);
        return;
    }

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
    if(marquee_active_)
        return;
    if(hot_ >= 0) {
        int old = hot_;
        hot_ = -1;
        Refresh(GetItemRect(old));
    }
    pressed_ = -1;
}

void UiGallery::MouseWheel(Point p, int zdelta, dword keyflags)
{
#ifdef PLATFORM_WIN32
    if(keyflags & K_CTRL) {
        if(zdelta > 0)
            ZoomBy(zoom_step_, p);
        else if(zdelta < 0)
            ZoomBy(1.0 / zoom_step_, p);
        return;
    }
#endif
    if(!geometry_valid_)
        UpdateGeometry();
    int extent = max(1, item_size_.cy + gap_);
    int rows = max(1, viewport_.GetHeight() / extent);
    int step = max(1, rows / 2) * extent;
    SetScrollPos(scroll_y_ - sgn(zdelta) * step);
}

bool UiGallery::Key(dword key, int)
{
    if(key == K_ESCAPE && (marquee_candidate_ || marquee_active_)) {
        EndMarquee(true);
        return true;
    }

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
    case K_HOME:  SetCursor(0); return true;
    case K_END:   SetCursor(model_->GetCount() - 1); return true;
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

void UiGallery::CancelMode()
{
    if(marquee_candidate_ || marquee_active_)
        EndMarquee(true);
    if(HasCapture())
        ReleaseCapture();
    Ctrl::CancelMode();
}

void UiGallery::GotFocus()
{
    Refresh();
}

void UiGallery::LostFocus()
{
    if(marquee_candidate_ || marquee_active_)
        EndMarquee(true);
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
