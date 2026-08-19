#include <Ui/UiSplitButton.h>
#include <Ui/UiTheme.h>

namespace Upp {

UiSplitButton::UiSplitButton()
{
    popup_.Init(this);
    popup_.NoSizeable();
    popup_.SetFrame(NullFrame());
}

int UiSplitButton::GetPopupRowHeight() const
{
    UiButton::Style bs = GetEffectiveStyle();
    Font font = bs.metrics.use_text_font ? bs.metrics.text_font : bs.font;
    if(IsNull(font))
        font = StdFont();
    Font desc_font = font;
    desc_font.Height(max(DPI(7), font.GetHeight() - DPI(1)));

    int pad_y = DPI(6);
    int icon_side = DPI(18);
    int configured_min = max(DPI(18), popup_item_height_);
    int measured = configured_min;
    for(const Item& it : items_) {
        if(it.group_header) {
            measured = max(measured, font.GetCy() + pad_y * 2);
            continue;
        }
        int title_h = GetTextSize(it.text, font).cy;
        int desc_h = it.description.IsEmpty() ? 0 : GetTextSize(it.description, desc_font).cy + DPI(1);
        int text_h = title_h + desc_h;
        int content_h = max(text_h, IsNull(it.icon) ? 0 : icon_side);
        measured = max(measured, content_h + pad_y * 2);
    }
    return measured;
}

int UiSplitButton::GetPopupHeightForVisibleItems() const
{
    int visible_items = min(items_.GetCount(), max(1, popup_max_items_));
    return max(GetPopupRowHeight(), visible_items * GetPopupRowHeight());
}

UiSplitButton::Style UiSplitButton::ResolveThemeStyle() const
{
    return UiTheme::ResolveButton();
}

Rect UiSplitButton::GetContentLayoutRect(const Rect& outer, const Style& style) const
{
    Rect content = UiButton::GetContentLayoutRect(outer, style);
    Rect surface = UiStyledSurfaceRect(outer, style.metrics);
    int w = min(max(DPI(18), split_width_), max(0, surface.GetWidth()));
    int split_left = surface.right - w;
    content.right = max(content.left, min(content.right, split_left - split_content_gap_));
    return content;
}

Rect UiSplitButton::GetSplitRect() const
{
    const Style& style = GetEffectiveStyle();
    Rect r = UiStyledSurfaceRect(Rect(GetSize()), style.metrics);
    if(r.IsEmpty())
        return r;
    int w = min(max(DPI(18), split_width_), max(0, r.GetWidth()));
    return Rect(r.right - w, r.top, r.right, r.bottom);
}

Rect UiSplitButton::GetMainRect() const
{
    const Style& style = GetEffectiveStyle();
    Rect r = UiStyledSurfaceRect(Rect(GetSize()), style.metrics);
    Rect split = GetSplitRect();
    r.right = max(r.left, split.left);
    return r;
}

UiSplitButton& UiSplitButton::Add(const String& text, const Value& data, bool enabled)
{
    Item it(text, data, enabled);
    if(pending_separator_ && items_.GetCount() > 0) {
        it.separator_before = true;
        pending_separator_ = false;
    }
    items_.Add(it);
    Refresh();
    return *this;
}

UiSplitButton& UiSplitButton::Add(const Item& item)
{
    Item it = item;
    if(pending_separator_ && items_.GetCount() > 0) {
        it.separator_before = true;
        pending_separator_ = false;
    }
    items_.Add(it);
    Refresh();
    return *this;
}

UiSplitButton& UiSplitButton::AddSeparator()
{
    pending_separator_ = items_.GetCount() > 0;
    return *this;
}

UiSplitButton& UiSplitButton::AddGroupHeader(const String& text)
{
    Item it(text, Value(), false);
    it.group_header = true;
    it.separator_before = pending_separator_ || items_.GetCount() > 0;
    pending_separator_ = false;
    items_.Add(it);
    Refresh();
    return *this;
}

UiSplitButton& UiSplitButton::ClearItems()
{
    items_.Clear();
    hot_item_ = -1;
    pending_separator_ = false;
    ClosePopupInternal();
    Refresh();
    return *this;
}

UiSplitButton& UiSplitButton::SetItemDescription(int index, const String& desc)
{
    if(index >= 0 && index < items_.GetCount()) {
        items_[index].description = desc;
        if(popup_open_) {
            UpdatePopupPosition();
            popup_.Refresh();
        }
    }
    return *this;
}

UiSplitButton& UiSplitButton::SetItemIcon(int index, const Image& icon, UiIconRenderMode mode)
{
    if(index >= 0 && index < items_.GetCount()) {
        items_[index].icon = icon;
        items_[index].icon_render_mode = mode;
        if(popup_open_) {
            UpdatePopupPosition();
            popup_.Refresh();
        }
    }
    return *this;
}

UiSplitButton& UiSplitButton::SetItemEnabled(int index, bool enabled)
{
    if(index >= 0 && index < items_.GetCount()) {
        items_[index].enabled = enabled;
        if(popup_open_)
            popup_.Refresh();
    }
    return *this;
}

UiSplitButton& UiSplitButton::SetItemSeparatorBefore(int index, bool on)
{
    if(index >= 0 && index < items_.GetCount()) {
        items_[index].separator_before = on;
        if(popup_open_)
            popup_.Refresh();
    }
    return *this;
}

UiSplitButton& UiSplitButton::SetItemGroupHeader(int index, bool on)
{
    if(index >= 0 && index < items_.GetCount()) {
        items_[index].group_header = on;
        if(on)
            items_[index].enabled = false;
        if(popup_open_)
            popup_.Refresh();
    }
    return *this;
}

UiSplitButton& UiSplitButton::SetSplitWidth(int width)
{
    split_width_ = max(DPI(18), width);
    RefreshLayout();
    Refresh();
    return *this;
}

UiSplitButton& UiSplitButton::SetSplitIconSize(int size)
{
    split_icon_size_ = max(DPI(8), size);
    Refresh();
    return *this;
}

UiSplitButton& UiSplitButton::SetSplitContentGap(int gap)
{
    split_content_gap_ = max(0, gap);
    RefreshLayout();
    Refresh();
    return *this;
}

UiSplitButton& UiSplitButton::SetPopupMinWidth(int width)
{
    popup_min_width_ = max(0, width);
    if(popup_open_)
        UpdatePopupPosition();
    return *this;
}

UiSplitButton& UiSplitButton::SetPopupMaxItems(int count)
{
    popup_max_items_ = max(1, count);
    if(popup_open_)
        UpdatePopupPosition();
    return *this;
}

UiSplitButton& UiSplitButton::SetPopupItemHeight(int height)
{
    popup_item_height_ = max(DPI(18), height);
    if(popup_open_)
        UpdatePopupPosition();
    return *this;
}

void UiSplitButton::OpenPopupInternal()
{
    if(popup_open_ || !IsEnabled())
        return;

    // Dynamic menus rebuild their rows from WhenOpen. Fire that preparation
    // callback before popup_open_ is set so ClearItems() cannot close the popup
    // that is still being prepared.
    WhenOpen();
    if(popup_open_ || items_.IsEmpty() || !IsEnabled())
        return;

    // The popup is intentionally owned by the split button rather than by a
    // hidden UiDropdown, so the closed control can stay one painted surface.
    popup_open_ = true;
    split_pressed_ = true;
    hot_item_ = -1;
    UpdatePopupPosition();
    popup_.PopUp(this, true, true, false);
    Refresh();
}

void UiSplitButton::ClosePopupInternal(int select_index, bool fire_select)
{
    if(!popup_open_)
        return;

    Value selected_data;
    if(fire_select && select_index >= 0 && select_index < items_.GetCount())
        selected_data = items_[select_index].data;

    popup_open_ = false;
    split_pressed_ = false;
    hot_item_ = -1;
    popup_.Close();
    Ptr<UiSplitButton> self = this;

    if(fire_select && select_index >= 0 && self)
        self->WhenSelect(select_index, selected_data);

    if(self)
        self->WhenClose();

    if(self)
        self->Refresh();
}

void UiSplitButton::UpdatePopupPosition()
{
    if(!popup_open_)
        return;

    // Popup width is independent from the closed button width. This is what
    // makes compact recent/history buttons usable with long paths.
    Rect outer = GetScreenRect();
    Rect screen = GetVirtualScreenArea();
    int item_h = GetPopupRowHeight();
    int popup_h = GetPopupHeightForVisibleItems();
    int popup_w = max(outer.GetWidth(), max(DPI(120), popup_min_width_));
    int space = max(0, popup_space_);

    int room_below = max(0, screen.bottom - outer.bottom - space);
    int room_above = max(0, outer.top - screen.top - space);
    bool below = room_below >= popup_h || room_below >= room_above;
    int room = below ? room_below : room_above;
    popup_h = min(popup_h, max(item_h, room > 0 ? room : screen.GetHeight() - DPI(8)));

    Point p(outer.left, below ? outer.bottom + space : outer.top - space - popup_h);
    if(p.x + popup_w > screen.right)
        p.x = max(screen.left, screen.right - popup_w);
    if(p.x < screen.left)
        p.x = screen.left;
    if(p.y + popup_h > screen.bottom)
        p.y = max(screen.top, screen.bottom - popup_h);
    if(p.y < screen.top)
        p.y = screen.top;

    popup_.SetRect(p.x, p.y, popup_w, popup_h);
}

void UiSplitButton::SelectPopupItem(int index)
{
    if(!IsSelectableItem(index))
        return;
    ClosePopupInternal(index, true);
}

bool UiSplitButton::IsSelectableItem(int index) const
{
    return index >= 0 && index < items_.GetCount() &&
           items_[index].enabled &&
           !items_[index].group_header;
}

int UiSplitButton::FindNextSelectable(int start, int step) const
{
    int count = items_.GetCount();
    if(count <= 0 || step == 0)
        return -1;
    int index = start;
    for(int probe = 0; probe < count; probe++) {
        index += step;
        if(index < 0)
            index = count - 1;
        else if(index >= count)
            index = 0;
        if(IsSelectableItem(index))
            return index;
    }
    return -1;
}

UiSplitButton& UiSplitButton::OpenPopup()
{
    OpenPopupInternal();
    return *this;
}

UiSplitButton& UiSplitButton::ClosePopup()
{
    ClosePopupInternal();
    return *this;
}

UiSplitButton& UiSplitButton::TogglePopup()
{
    if(popup_open_)
        ClosePopupInternal();
    else
        OpenPopupInternal();
    return *this;
}

void UiSplitButton::DrawSplitAffordance(Draw& w, const Rect& r)
{
    Rect split = GetSplitRect();
    if(split.IsEmpty())
        return;

    const Style& st = GetEffectiveStyle();
    StyledState state = !IsEnabled() ? ST_DISABLED
                         : split_pressed_ || popup_open_ ? ST_PRESSED
                         : split_hot_ ? ST_HOT
                         : visual_state_;

    // Draw only the semantic split affordance here; UiButton owns the shared
    // face/frame/focus paint for the complete rounded button surface.
    Color line = st.palette.frame[state];
    if(IsNull(line))
        line = Blend(SColorShadow(), SColorPaper(), 130);
    int divider_w = max(1, st.metrics.frame_width);
    w.DrawRect(split.left, split.top + DPI(5), divider_w, max(0, split.GetHeight() - DPI(10)), line);

    int side = min(max(DPI(8), split_icon_size_), max(DPI(8), split.GetWidth() - DPI(10)));
    Rect ir(split.left + (split.GetWidth() - side) / 2,
            split.top + (split.GetHeight() - side) / 2,
            split.left + (split.GetWidth() + side) / 2,
            split.top + (split.GetHeight() + side) / 2);
    Image glyph = popup_open_ ? ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48()
                              : ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48();
    Color ink = st.palette.icon[state];
    if(IsNull(ink))
        ink = st.palette.ink[state];
    UiPaintStyledIcon(w, ir, glyph, true, true, UiIconRenderMode::MonoTint, ink, IsEnabled());
}

void UiSplitButton::Paint(Draw& w)
{
    UiButton::Paint(w);
    DrawSplitAffordance(w, GetSize());
}

void UiSplitButton::LeftDown(Point p, dword keyflags)
{
    if(!IsEnabled() || !IsInteractionPoint(p))
        return;
    if(GetSplitRect().Contains(p)) {
        // The arrow region is its own command target. The main region keeps
        // UiButton's normal press/release behavior and action timing.
        SetFocus();
        split_pressed_ = true;
        TogglePopup();
        Refresh();
        return;
    }
    UiButton::LeftDown(p, keyflags);
}

void UiSplitButton::LeftUp(Point p, dword keyflags)
{
    if(split_pressed_ && !popup_open_) {
        split_pressed_ = false;
        Refresh();
        return;
    }
    if((IsInteractionPoint(p) && GetSplitRect().Contains(p)) || popup_open_)
        return;
    UiButton::LeftUp(p, keyflags);
}

void UiSplitButton::MouseMove(Point p, dword keyflags)
{
    bool next_split_hot = IsInteractionPoint(p) && GetSplitRect().Contains(p);
    if(split_hot_ != next_split_hot) {
        split_hot_ = next_split_hot;
        Refresh();
    }
    UiButton::MouseMove(p, keyflags);
}

void UiSplitButton::MouseLeave()
{
    split_hot_ = false;
    UiButton::MouseLeave();
}

void UiSplitButton::CancelMode()
{
    ClosePopupInternal();
    split_pressed_ = false;
    UiButton::CancelMode();
}

bool UiSplitButton::Key(dword key, int count)
{
    if(key == K_DOWN || key == K_ALT_DOWN) {
        OpenPopupInternal();
        return true;
    }
    if(key == K_ESCAPE && popup_open_) {
        ClosePopupInternal();
        return true;
    }
    return UiButton::Key(key, count);
}

Size UiSplitButton::GetMinSize() const
{
    Size sz = UiButton::GetMinSize();
    sz.cx += max(DPI(18), split_width_);
    return sz;
}

String UiSplitButton::GetDesc() const
{
    return "UiSplitButton";
}

void UiSplitButton::PopupWindow::Init(UiSplitButton *button)
{
    owner = button;
    BackPaint();
}

void UiSplitButton::PopupWindow::SetHot(int index)
{
    if(owner && owner->hot_item_ != index) {
        owner->hot_item_ = index;
        Refresh();
    }
}

Rect UiSplitButton::PopupWindow::GetItemRect(int index) const
{
    if(!owner)
        return Rect(0, 0, 0, 0);
    int h = owner->GetPopupRowHeight();
    return Rect(0, index * h, GetSize().cx, (index + 1) * h);
}

int UiSplitButton::PopupWindow::HitTest(Point p) const
{
    if(!owner)
        return -1;
    int h = owner->GetPopupRowHeight();
    if(h <= 0)
        return -1;
    int q = p.y / h;
    return q >= 0 && q < owner->items_.GetCount() && GetItemRect(q).Contains(p) ? q : -1;
}

void UiSplitButton::PopupWindow::Paint(Draw& w)
{
    Rect r = GetSize();
    if(!owner)
        return;

    UiButton::Style bs = owner->GetEffectiveStyle();
    int popup_radius = min(DPI(10), max(DPI(6), bs.metrics.radius));
    Color popup_base = bs.palette.face[ST_NORMAL].IsSolid() ? bs.palette.face[ST_NORMAL].color : SColorPaper();
    if(IsNull(popup_base))
        popup_base = SColorPaper();
    int frame_w = bs.metrics.frame_enabled ? max(0, bs.metrics.frame_width) : 0;
    Color frame = bs.palette.frame[ST_NORMAL];
    if(IsNull(frame))
        frame = Blend(SColorShadow(), popup_base, 120);

    Font font = bs.metrics.use_text_font ? bs.metrics.text_font : bs.font;
    if(IsNull(font))
        font = StdFont();
    Font desc_font = font;
    desc_font.Height(max(DPI(7), font.GetHeight() - DPI(1)));
    Color subtitle_ink = bs.palette.ink[ST_DISABLED];
    if(IsNull(subtitle_ink))
        subtitle_ink = DisabledColor(SColorText());

    ImageDraw popup_buf(r.GetWidth(), r.GetHeight());
    Draw& __popup_draw = popup_buf;
#define w __popup_draw
    w.DrawRect(r, popup_base);

    // Rows are deliberately simple: text, optional description, optional icon,
    // disabled state, and hot tracking. Nested menus belong in UiMenu.
    for(int i = 0; i < owner->items_.GetCount(); i++) {
        const Item& it = owner->items_[i];
        Rect row = GetItemRect(i);
        if(row.top >= r.bottom || row.bottom <= r.top)
            continue;

        if(it.separator_before && i > 0) {
            Color sep = Blend(SColorShadow(), popup_base, 200);
            w.DrawRect(row.left + DPI(8), row.top, max(0, row.GetWidth() - DPI(16)), 1, sep);
        }

        StyledState state = !it.enabled ? ST_DISABLED : owner->hot_item_ == i ? ST_HOT : ST_NORMAL;
        if(it.group_header) {
            Rect text = row.Deflated(DPI(10), DPI(6));
            int hy = text.top + max(0, (text.GetHeight() - desc_font.Bold().GetCy()) / 2);
            Color hdr = Blend(SColorFace(), popup_base, 20);
            w.DrawRect(row.Deflated(1, 0), hdr);
            w.DrawText(text.left, hy, ToUpper(it.text), desc_font.Bold(), Blend(SColorText(), popup_base, 120));
            continue;
        }
        if(state == ST_HOT) {
            UiFill face = bs.palette.face[ST_HOT];
            if(face.IsNone())
                w.DrawRect(row.Deflated(1, 0), Blend(SColorHighlight(), SColorPaper(), 220));
            else if(face.IsSolid())
                w.DrawRect(row.Deflated(1, 0), face.color);
            else
                w.DrawRect(row.Deflated(1, 0), Blend(SColorHighlight(), SColorPaper(), 220));
        }

        Rect text = row.Deflated(DPI(10), DPI(6));
        if(!IsNull(it.icon)) {
            int side = DPI(18);
            Rect ir(text.left, text.top + (text.GetHeight() - side) / 2, text.left + side, text.top + (text.GetHeight() + side) / 2);
            if(it.icon_render_mode == UiIconRenderMode::MonoTint)
                UiPaintStyledIcon(w, ir, it.icon, true, true, UiIconRenderMode::MonoTint, bs.palette.icon[state], it.enabled);
            else
                w.DrawImage(ir.left, ir.top, CachedRescale(it.icon, ir.GetSize()));
            text.left = ir.right + DPI(8);
        }

        Color ink = bs.palette.ink[state];
        if(IsNull(ink))
            ink = SColorText();
        Size title_sz = GetTextSize(it.text, font);
        int ty = it.description.IsEmpty()
               ? row.top + (row.GetHeight() - title_sz.cy) / 2
               : text.top;
        w.Clip(text);
        w.DrawText(text.left, ty, it.text, font, ink);
        if(!it.description.IsEmpty())
            w.DrawText(text.left, ty + title_sz.cy + DPI(1), it.description, desc_font, subtitle_ink);
        w.End();
    }

#undef w
    UiPaintRoundedPopupComposited(w, r, popup_buf, popup_radius, popup_base, frame_w, frame);
}

void UiSplitButton::PopupWindow::LeftDown(Point p, dword)
{
    int row = HitTest(p);
    if(row >= 0)
        owner->SelectPopupItem(row);
    else if(owner)
        owner->ClosePopupInternal();
}

void UiSplitButton::PopupWindow::MouseMove(Point p, dword)
{
    SetHot(HitTest(p));
}

void UiSplitButton::PopupWindow::MouseLeave()
{
    SetHot(-1);
}

bool UiSplitButton::PopupWindow::Key(dword key, int)
{
    if(!owner)
        return false;
    if(key == K_ESCAPE) {
        owner->ClosePopupInternal();
        return true;
    }
    if(key == K_ENTER && owner->hot_item_ >= 0) {
        owner->SelectPopupItem(owner->hot_item_);
        return true;
    }
    if(key == K_DOWN || key == K_UP) {
        if(owner->items_.IsEmpty())
            return true;
        int seed = owner->hot_item_;
        if(seed < 0)
            seed = key == K_DOWN ? owner->items_.GetCount() - 1 : 0;
        int next = owner->FindNextSelectable(seed, key == K_DOWN ? +1 : -1);
        if(next >= 0)
            SetHot(next);
        return true;
    }
    return false;
}

void UiSplitButton::PopupWindow::Deactivate()
{
    if(GetScreenRect().Contains(GetMousePos()))
        return;

    Ptr<UiSplitButton> self = owner;
    if(self && self->IsPopupOpen())
        self->ClosePopup();
}

} // namespace Upp
