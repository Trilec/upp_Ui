#include <Ui/UiDropdown.h>

namespace Upp {

namespace {

Rect DropdownMarkerRect(const UiDropdown::Style& style, const Rect& inner,
                        bool left_side, int side)
{
    int cy = min(max(DPI(10), side), max(0, inner.GetHeight()));
    int y = inner.top + max(0, (inner.GetHeight() - cy) / 2);
    return left_side ? RectC(inner.left, y, cy, cy)
                     : RectC(inner.right - cy, y, cy, cy);
}

}

void UiDropdown::OpenPopupInternal()
{
    if(popup_open_ || GetCount() <= 0)
        return;

    popup_open_ = true;
    RebuildIndicator();
    UpdatePopupPosition();
    popup_.Layout();
    popup_.PopUp(this, true, true, false);
    popup_.SyncWindowRegion();
    SyncPopupSelection();
    WhenOpen();
    Refresh();
}

void UiDropdown::ClosePopupInternal(bool apply_selection)
{
    if(!popup_open_)
        return;

    if(drag_candidate_)
        EndPopupDrag(true);

    int apply_index = -1;
    if(!multi_select_ && apply_selection && IsSelectableItem(highlight_index_))
        apply_index = highlight_index_;

    bool selection_changed = false;
    String selected_text;
    Value selected_data;

    popup_open_ = false;
    popup_.Close();
    popup_.ResetRenderPool();
    RebuildIndicator();

    if(apply_index >= 0) {
        selection_changed = ApplySelectionInternal(apply_index, false);
        if(selection_changed) {
            selected_text = GetItemText(apply_index);
            selected_data = GetItemData(apply_index);
        }
    }

    Ptr<UiDropdown> self = this;
    if(selection_changed && self) {
        self->WhenSelect(apply_index);
        if(self)
            self->WhenSelectText(selected_text);
        if(self)
            self->WhenSelectData(selected_data);
    }
    if(self)
        self->WhenClose();
    if(self)
        self->Refresh();
}

UiDropdown& UiDropdown::OpenPopup()
{
    OpenPopupInternal();
    return *this;
}

UiDropdown& UiDropdown::ClosePopup()
{
    ClosePopupInternal(true);
    return *this;
}

UiDropdown& UiDropdown::TogglePopup()
{
    if(popup_open_)
        ClosePopupInternal(true);
    else
        OpenPopupInternal();
    return *this;
}

void UiDropdown::UpdatePopupPosition()
{
    if(!popup_open_)
        return;

    const Style& style = GetEffectiveStyle();
    Rect control = GetScreenRect();
    Rect screen = GetVirtualScreenArea();
    int item_h = max(DPI(16), style.popup_item_height);
    int extent = item_h + max(0, style.item_spacing);
    int count = GetCount();
    int64 content64 = count > 0 ? (int64)count * extent - max(0, style.item_spacing) : 0;
    int content_h = content64 >= INT_MAX ? INT_MAX : (int)content64;
    int max_items_h = style.popup_max_items > 0
                    ? min(INT_MAX, style.popup_max_items * extent)
                    : content_h;
    int target_h = min(content_h, min(style.popup_max_height, max_items_h));
    target_h = max(item_h, target_h);
    int popup_w = max(control.GetWidth(), max(DPI(120), style.popup_min_width));
    int space = max(0, style.popup_space);

    int below = max(0, screen.bottom - (control.bottom + space));
    int above = max(0, (control.top - space) - screen.top);
    bool use_below = below >= target_h || below >= above;
    int room = use_below ? below : above;
    int popup_h = min(target_h, max(item_h, room));
    if(room <= 0)
        popup_h = min(target_h, max(item_h, screen.GetHeight() - DPI(8)));

    Point p(control.left, use_below ? control.bottom + space
                                    : control.top - space - popup_h);
    if(p.x + popup_w > screen.right)
        p.x = max(screen.left, screen.right - popup_w);
    p.x = max(screen.left, p.x);
    if(p.y + popup_h > screen.bottom)
        p.y = max(screen.top, screen.bottom - popup_h);
    p.y = max(screen.top, p.y);

    popup_.SetRect(p.x, p.y, popup_w, popup_h);
    popup_.Layout();
    if(popup_.IsOpen())
        popup_.SyncWindowRegion();
}

void UiDropdown::BeginPopupDrag(int row, Point start_screen)
{
    if(!drag_reorder_enabled_ || !model_ || row < 0 || row >= GetCount()
       || GetCount() < 2 || !IsSelectableItem(row)) {
        drag_candidate_ = false;
        return;
    }
    drag_candidate_ = true;
    dragging_ = false;
    drag_moved_ = false;
    drag_from_ = row;
    drag_insert_before_ = row;
    drag_start_screen_ = start_screen;
}

void UiDropdown::ContinuePopupDrag(Point screen)
{
    if(!drag_candidate_ || !popup_.IsOpen())
        return;

    if(!dragging_) {
        int dx = screen.x - drag_start_screen_.x;
        int dy = screen.y - drag_start_screen_.y;
        if(abs(dy) < drag_threshold_px_ || abs(dy) < abs(dx))
            return;
        dragging_ = true;
        drag_moved_ = true;
    }

    Rect sr = popup_.GetScreenRect();
    int y = screen.y - sr.top;
    int before = GetCount();
    for(int i = 0; i < GetCount(); i++) {
        Rect rr = popup_.GetItemRect(i);
        if(y < rr.top + rr.GetHeight() / 2) {
            before = i;
            break;
        }
    }
    if(drag_insert_before_ != before) {
        drag_insert_before_ = before;
        popup_.Refresh();
    }
}

void UiDropdown::EndPopupDrag(bool cancel)
{
    if(!drag_candidate_) {
        dragging_ = drag_moved_ = false;
        hot_drag_ = pressed_drag_ = -1;
        return;
    }

    bool move = !cancel && dragging_ && drag_from_ >= 0;
    int from = drag_from_;
    int before = drag_insert_before_;
    drag_candidate_ = false;
    dragging_ = false;
    drag_moved_ = false;
    drag_from_ = -1;
    drag_insert_before_ = -1;
    hot_drag_ = pressed_drag_ = -1;

    if(move)
        MoveItemTo(from, before);
    if(popup_open_)
        popup_.Refresh();
    Refresh();
}

void UiDropdown::MoveItemTo(int from, int before)
{
    if(!model_ || from < 0 || from >= GetCount() || before < 0 || before > GetCount()
       || before == from || before == from + 1)
        return;

    UiReorderRequest request;
    request.from = from;
    request.before = before;
    if(WhenReorderRequest)
        WhenReorderRequest(request);
    if(!request.accept)
        return;
    if(request.handled) {
        popup_.ResetRenderPool();
        popup_.Layout();
        Refresh();
        return;
    }
    if(!internal_mutation_enabled_)
        return;

    int original_before = before;
    if(model_->Move(from, before) && WhenReordered)
        WhenReordered(from, original_before);
}

int UiDropdown::RemapIndexAfterMove(int index, int from, int before) const
{
    if(index < 0 || from < 0 || before < 0 || before == from || before == from + 1)
        return index;
    int to = before > from ? before - 1 : before;
    if(index == from)
        return to;
    if(from < to && index > from && index <= to)
        return index - 1;
    if(to < from && index >= to && index < from)
        return index + 1;
    return index;
}

bool UiDropdown::PopupWindow::Key(dword key, int)
{
    if(!owner)
        return false;
    switch(key) {
    case K_ESCAPE:
        owner->ClosePopupInternal(false);
        return true;
    case K_ENTER:
        if(owner->IsSelectableItem(owner->highlight_index_)) {
            if(owner->multi_select_) {
                owner->ToggleItemChecked(owner->highlight_index_);
                if(owner->popup_auto_close_)
                    owner->ClosePopupInternal(false);
            }
            else
                owner->ClosePopupInternal(true);
        }
        return true;
    case K_UP: {
        int i = owner->highlight_index_ - 1;
        while(i >= 0 && !owner->IsSelectableItem(i))
            i--;
        if(i >= 0) {
            SetHighlight(i);
            EnsureVisible(i);
        }
        return true;
    }
    case K_DOWN: {
        int i = max(0, owner->highlight_index_ + 1);
        while(i < owner->GetCount() && !owner->IsSelectableItem(i))
            i++;
        if(i < owner->GetCount()) {
            SetHighlight(i);
            EnsureVisible(i);
        }
        return true;
    }
    }
    if((key & K_KEYUP) == 0 && key >= 32 && key < 256)
        return owner->HandleTypeAhead((int)key);
    return false;
}

void UiDropdown::PopupWindow::Init(UiDropdown* dropdown_owner)
{
    owner = dropdown_owner;
    vscroll_.ShowArrows(false);
    vscroll_.EnableThinIdle(false);
    vscroll_.EnableAutoHide(false);
    vscroll_.WhenScroll << [this] {
        scroll_pos_ = vscroll_.GetPos();
        PrepareItemRenders();
        Refresh();
    };
    Add(vscroll_);
}

int UiDropdown::PopupWindow::GetItemExtent() const
{
    return owner ? max(DPI(16), owner->GetEffectiveStyle().popup_item_height)
                 + max(0, owner->GetEffectiveStyle().item_spacing) : DPI(16);
}

int UiDropdown::PopupWindow::GetVisibleStart() const
{
    return max(0, scroll_pos_ / max(1, GetItemExtent()));
}

int UiDropdown::PopupWindow::GetVisibleEnd() const
{
    if(!owner)
        return 0;
    int extent = max(1, GetItemExtent());
    int last = (scroll_pos_ + max(0, GetSize().cy) + extent - 1) / extent + 1;
    return min(owner->GetCount(), max(GetVisibleStart(), last));
}

void UiDropdown::PopupWindow::SyncScrollBarState()
{
    if(!owner)
        return;
    const Style& style = owner->GetEffectiveStyle();
    int extent = GetItemExtent();
    int count = owner->GetCount();
    int64 total64 = count > 0 ? (int64)count * extent - max(0, style.item_spacing) : 0;
    total_height_ = total64 >= INT_MAX ? INT_MAX : (int)total64;
    int view_h = max(1, GetSize().cy);
    scrollbar_visible_ = style.popup_show_scrollbar && total_height_ > view_h;

    if(scrollbar_visible_) {
        int sbw = min(max(DPI(10), scrollbar_width_), max(DPI(10), GetSize().cx / 3));
        scrollbar_width_ = sbw;
        vscroll_.SetRect(max(0, GetSize().cx - sbw), 0, sbw, GetSize().cy);
        vscroll_.Show();
        vscroll_.SetRange(0, max(total_height_, view_h), view_h);
        vscroll_.SetPos(scroll_pos_);
        scroll_pos_ = vscroll_.GetPos();
    }
    else {
        vscroll_.Hide();
        scroll_pos_ = 0;
    }
}

void UiDropdown::PopupWindow::ResetRenderPool()
{
    render_pool_.Clear();
    prepared_first_ = prepared_last_ = -1;
    last_render_layout_count_ = 0;
}

Rect UiDropdown::PopupWindow::GetItemRect(int index) const
{
    if(!owner || index < 0 || index >= owner->GetCount())
        return Rect(0, 0, 0, 0);
    int extent = GetItemExtent();
    int h = max(DPI(16), owner->GetEffectiveStyle().popup_item_height);
    int y = index * extent - scroll_pos_;
    int right = GetSize().cx - (scrollbar_visible_ ? scrollbar_width_ : 0);
    return Rect(0, y, max(0, right), y + h);
}

Rect UiDropdown::PopupWindow::GetItemContentRect(int index) const
{
    Rect inner = GetItemRect(index).Deflated(DPI(8), DPI(3));
    if(!owner || inner.IsEmpty() || index < 0 || index >= owner->GetCount())
        return Rect(0, 0, 0, 0);

    const Style& style = owner->GetEffectiveStyle();
    const UiModelItem& item = owner->GetModel().Get(index);
    int gap = DPI(6);

    bool drag = owner->drag_reorder_enabled_ && style.show_drag_handle
             && owner->GetCount() > 1 && owner->IsSelectableItem(index);
    if(drag) {
        int side = min(max(DPI(10), style.drag_size), max(0, inner.GetHeight()));
        if(style.drag_side == UiAlign::LEFT)
            inner.left = min(inner.right, inner.left + side + style.drag_gap);
        else
            inner.right = max(inner.left, inner.right - side - style.drag_gap);
    }

    bool marker = owner->multi_select_
                ? (!IsNull(style.popup_check_checked_icon) || !IsNull(style.popup_check_unchecked_icon))
                : style.show_popup_selection_marker;
    if(marker) {
        int side = min(DPI(14), max(0, inner.GetHeight()));
        if(style.popup_marker_side == UiAlign::LEFT)
            inner.left = min(inner.right, inner.left + side + gap);
        else
            inner.right = max(inner.left, inner.right - side - gap);
    }

    if(item.group_header)
        inner.left += DPI(2);
    return inner;
}

void UiDropdown::PopupWindow::PrepareItemRenders()
{
    last_render_layout_count_ = 0;
    if(!owner || owner->GetCount() <= 0 || GetSize().IsEmpty()) {
        prepared_first_ = prepared_last_ = -1;
        return;
    }

    owner->EnsureItemRender();
    int first = GetVisibleStart();
    int end = GetVisibleEnd();
    int count = max(0, end - first);
    while(render_pool_.GetCount() < count) {
        PopupRenderSlot& slot = render_pool_.Add();
        slot.render = owner->item_render_->Clone();
    }

    for(int i = 0; i < count; i++) {
        int index = first + i;
        PopupRenderSlot& slot = render_pool_[i];
        if(slot.index != index) {
            slot.render->SetData(owner->MakePopupRenderData(index));
            slot.index = index;
        }
        if(slot.render->PrepareLayout(GetItemContentRect(index), UiDirection::H))
            last_render_layout_count_++;
    }
    for(int i = count; i < render_pool_.GetCount(); i++)
        render_pool_[i].index = -1;
    prepared_first_ = count ? first : -1;
    prepared_last_ = count ? first + count - 1 : -1;
}

const UiItemRender* UiDropdown::PopupWindow::FindPreparedRender(int index) const
{
    if(index < prepared_first_ || index > prepared_last_ || prepared_first_ < 0)
        return nullptr;
    int slot = index - prepared_first_;
    if(slot < 0 || slot >= render_pool_.GetCount() || render_pool_[slot].index != index)
        return nullptr;
    return render_pool_[slot].render.operator->();
}

void UiDropdown::PopupWindow::Layout()
{
    SyncScrollBarState();
    PrepareItemRenders();
}

void UiDropdown::PopupWindow::Deactivate()
{
    if(GetScreenRect().Contains(GetMousePos()))
        return;
    if(owner)
        owner->suppress_next_open_ = owner->GetScreenRect().Contains(GetMousePos());
    if(owner && !owner->popup_pinned_)
        owner->ClosePopupInternal(false);
}

void UiDropdown::PopupWindow::Paint(Draw& target)
{
    last_paint_item_count_ = 0;
    if(!owner)
        return;

    const Style& style = owner->GetEffectiveStyle();
    Rect bounds = GetSize();
    if(bounds.IsEmpty())
        return;

    Color popup_base = style.popup_background_color;
    if(style.popup_use_main_skin && style.palette.face[ST_NORMAL].IsSolid())
        popup_base = style.palette.face[ST_NORMAL].color;
    if(IsNull(popup_base))
        popup_base = SColorPaper();
    Color frame = style.popup_use_main_skin ? style.palette.frame[ST_NORMAL]
                                            : style.popup_frame_color;
    if(IsNull(frame))
        frame = style.palette.frame[ST_NORMAL];
    int frame_w = style.popup_use_main_skin ? max(0, style.metrics.frame_width)
                                            : max(0, style.popup_frame_width);
    int radius = style.popup_use_main_skin ? max(0, style.metrics.radius)
                                           : max(0, style.popup_radius);

    ImageDraw buffer(bounds.GetWidth(), bounds.GetHeight());
    Draw& w = buffer;
    w.DrawRect(bounds, popup_base);

    int first = max(0, prepared_first_);
    int last = min(owner->GetCount() - 1, prepared_last_);
    for(int i = first; i <= last; i++) {
        Rect row = GetItemRect(i);
        if(row.bottom <= 0 || row.top >= bounds.bottom)
            continue;
        const UiModelItem& item = owner->GetModel().Get(i);
        bool hot = i == owner->highlight_index_;
        bool selected = owner->multi_select_ ? item.checked : i == owner->selected_index_;

        if(item.separator_before && i > 0)
            w.DrawRect(row.left + DPI(6), row.top, max(0, row.GetWidth() - DPI(12)), 1,
                       Blend(style.palette.frame[ST_NORMAL], popup_base, 150));

        if(item.group_header) {
            Color header = style.palette.face[ST_HOT].IsSolid()
                         ? Blend(style.palette.face[ST_HOT].color, popup_base, 80)
                         : popup_base;
            w.DrawRect(row, header);
        }
        else if(hot || selected) {
            StyledState state = hot ? ST_HOT : ST_PRESSED;
            Color fill = style.popup_item_style.palette.face[state].IsSolid()
                       ? style.popup_item_style.palette.face[state].color
                       : (hot ? style.palette.face[ST_HOT].IsSolid() ? style.palette.face[ST_HOT].color : popup_base
                              : style.palette.face[ST_PRESSED].IsSolid() ? style.palette.face[ST_PRESSED].color : popup_base);
            w.DrawRect(row, fill);
        }

        UiItemRenderState state;
        state.enabled = item.group_header ? true : item.enabled;
        state.selected = selected;
        state.hot = hot;
        state.pressed = i == owner->pressed_drag_;
        if(const UiItemRender* render = FindPreparedRender(i))
            render->Paint(w, state);

        Rect inner = row.Deflated(DPI(8), DPI(3));
        Color ink = style.popup_item_style.palette.ink[item.enabled ? ST_NORMAL : ST_DISABLED];
        if(IsNull(ink))
            ink = item.enabled ? style.palette.ink[ST_NORMAL] : style.palette.ink[ST_DISABLED];

        bool marker_slot = owner->multi_select_
                         ? (!IsNull(style.popup_check_checked_icon) || !IsNull(style.popup_check_unchecked_icon))
                         : style.show_popup_selection_marker;
        if(marker_slot) {
            Rect marker = DropdownMarkerRect(style, inner,
                                             style.popup_marker_side == UiAlign::LEFT,
                                             DPI(14));
            Image icon;
            if(owner->multi_select_)
                icon = item.checked ? style.popup_check_checked_icon : style.popup_check_unchecked_icon;
            else if(selected || item.checked)
                icon = !IsNull(style.popup_selection_icon) ? style.popup_selection_icon
                                                           : style.popup_check_checked_icon;
            if(!IsNull(icon))
                UiPaintStyledIcon(w, marker, icon, true, false,
                                  style.popup_marker_render_mode, ink, item.enabled);
        }

        bool drag = owner->drag_reorder_enabled_ && style.show_drag_handle
                 && owner->GetCount() > 1 && owner->IsSelectableItem(i);
        if(drag) {
            int side = min(max(DPI(10), style.drag_size), max(0, inner.GetHeight()));
            Rect dr = DropdownMarkerRect(style, inner, style.drag_side == UiAlign::LEFT, side);
            Color drag_ink = owner->dragging_ && i == owner->drag_from_ ? style.drag_marker : ink;
            UiPaintStyledIcon(w, dr,
                              IsNull(style.drag_glyph) ? ICON_DESIGN_DRAG_INDICATOR_48() : style.drag_glyph,
                              true, true, UiIconRenderMode::MonoTint, drag_ink, item.enabled);
        }
        last_paint_item_count_++;
    }

    if(owner->dragging_ && owner->drag_from_ >= 0) {
        int y = 0;
        if(owner->drag_insert_before_ >= 0 && owner->drag_insert_before_ < owner->GetCount())
            y = GetItemRect(owner->drag_insert_before_).top;
        else if(owner->GetCount() > 0)
            y = GetItemRect(owner->GetCount() - 1).bottom;
        int h = max(2, DPI(2));
        w.DrawRect(DPI(8), y - h / 2, max(DPI(24), bounds.GetWidth() - DPI(16)), h,
                   style.drag_marker);
    }

    UiPaintRoundedPopupComposited(target, bounds, buffer, radius, popup_base, frame_w, frame);
}

void UiDropdown::PopupWindow::SyncWindowRegion()
{
#ifdef PLATFORM_WIN32
    if(!owner)
        return;
    HWND hwnd = GetHWND();
    if(!hwnd)
        return;
    Rect r = GetSize();
    int radius = owner->GetEffectiveStyle().popup_use_main_skin
               ? max(0, owner->GetEffectiveStyle().metrics.radius)
               : max(0, owner->GetEffectiveStyle().popup_radius);
    radius = min(radius, min(r.GetWidth(), r.GetHeight()) / 2);
    HRGN region = radius > 0
                ? ::CreateRoundRectRgn(0, 0, r.GetWidth(), r.GetHeight(), radius * 2, radius * 2)
                : ::CreateRectRgn(0, 0, r.GetWidth(), r.GetHeight());
    ::SetWindowRgn(hwnd, region, TRUE);
#endif
}

void UiDropdown::PopupWindow::SetHighlight(int index)
{
    if(!owner || index == owner->highlight_index_)
        return;
    int old = owner->highlight_index_;
    owner->highlight_index_ = index;
    if(old >= 0)
        Refresh(GetItemRect(old));
    if(index >= 0)
        Refresh(GetItemRect(index));
}

int UiDropdown::PopupWindow::HitTest(Point p) const
{
    if(!owner)
        return -1;
    int content_w = GetSize().cx - (scrollbar_visible_ ? scrollbar_width_ : 0);
    if(p.x < 0 || p.x >= content_w || p.y < 0 || p.y >= GetSize().cy)
        return -1;
    int extent = max(1, GetItemExtent());
    int logical = p.y + scroll_pos_;
    int index = logical / extent;
    if(index < 0 || index >= owner->GetCount())
        return -1;
    int within = logical % extent;
    return within < max(DPI(16), owner->GetEffectiveStyle().popup_item_height) ? index : -1;
}

int UiDropdown::PopupWindow::HitTestDrag(Point p) const
{
    int index = HitTest(p);
    if(index < 0 || !owner || !owner->drag_reorder_enabled_
       || !owner->GetEffectiveStyle().show_drag_handle || !owner->IsSelectableItem(index))
        return -1;
    Rect inner = GetItemRect(index).Deflated(DPI(8), DPI(3));
    int side = min(max(DPI(10), owner->GetEffectiveStyle().drag_size), max(0, inner.GetHeight()));
    Rect dr = DropdownMarkerRect(owner->GetEffectiveStyle(), inner,
                                 owner->GetEffectiveStyle().drag_side == UiAlign::LEFT, side);
    return dr.Contains(p) ? index : -1;
}

void UiDropdown::PopupWindow::EnsureVisible(int index)
{
    if(!owner || index < 0 || index >= owner->GetCount())
        return;
    int extent = GetItemExtent();
    int top = index * extent;
    int bottom = top + max(DPI(16), owner->GetEffectiveStyle().popup_item_height);
    int page = max(1, GetSize().cy);
    if(top < scroll_pos_)
        scroll_pos_ = top;
    else if(bottom > scroll_pos_ + page)
        scroll_pos_ = bottom - page;
    int max_scroll = max(0, total_height_ - page);
    scroll_pos_ = clamp(scroll_pos_, 0, max_scroll);
    if(scrollbar_visible_)
        vscroll_.SetPos(scroll_pos_);
    PrepareItemRenders();
    Refresh();
}

void UiDropdown::PopupWindow::LeftDown(Point p, dword)
{
    if(!owner)
        return;
    int drag = HitTestDrag(p);
    if(drag >= 0) {
        owner->highlight_index_ = drag;
        owner->pressed_drag_ = drag;
        SetCapture();
        owner->BeginPopupDrag(drag, GetMousePos());
        Refresh();
        return;
    }

    int index = HitTest(p);
    if(!owner->IsSelectableItem(index))
        return;
    owner->highlight_index_ = index;
    if(owner->multi_select_) {
        owner->ToggleItemChecked(index);
        if(owner->popup_auto_close_)
            owner->ClosePopupInternal(false);
        else {
            PrepareItemRenders();
            Refresh(GetItemRect(index));
        }
    }
    else
        owner->ClosePopupInternal(true);
}

void UiDropdown::PopupWindow::LeftUp(Point, dword)
{
    if(!owner)
        return;
    if(HasCapture())
        ReleaseCapture();
    if(owner->drag_candidate_) {
        owner->EndPopupDrag(false);
        return;
    }
    if(owner->pressed_drag_ >= 0) {
        owner->pressed_drag_ = -1;
        Refresh();
    }
}

void UiDropdown::PopupWindow::MouseMove(Point p, dword)
{
    if(!owner)
        return;
    if(owner->drag_candidate_) {
        owner->ContinuePopupDrag(GetMousePos());
        return;
    }
    int index = HitTest(p);
    int drag = HitTestDrag(p);
    if(index != owner->highlight_index_)
        SetHighlight(index);
    if(drag != owner->hot_drag_) {
        owner->hot_drag_ = drag;
        Refresh();
    }
}

void UiDropdown::PopupWindow::MouseWheel(Point, int zdelta, dword)
{
    if(!owner || !scrollbar_visible_)
        return;
    int step = max(DPI(8), max(DPI(16), owner->GetEffectiveStyle().popup_item_height) / 2);
    int max_scroll = max(0, total_height_ - max(1, GetSize().cy));
    scroll_pos_ = clamp(scroll_pos_ + (zdelta < 0 ? step : zdelta > 0 ? -step : 0),
                        0, max_scroll);
    vscroll_.SetPos(scroll_pos_);
    scroll_pos_ = vscroll_.GetPos();
    PrepareItemRenders();
    Refresh();
}

} // namespace Upp
