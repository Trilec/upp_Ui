#include <Ui/UiSplitButton.h>
#include <Ui/UiTheme.h>

namespace Upp {

UiSplitButton::UiSplitButton()
{
    popup_.Init(this);
    popup_.NoSizeable();
    popup_.SetFrame(NullFrame());
}

UiSplitButton::Style UiSplitButton::ResolveThemeStyle() const
{
    return UiTheme::ResolveButton();
}

Rect UiSplitButton::GetContentLayoutRect(const Rect& outer, const Style& style) const
{
    Rect main = outer;
    Rect split = GetSplitRect();
    main.right = max(main.left, split.left - DPI(4));
    return UiStyledInnerRect(main, style.metrics, style.skin);
}

Rect UiSplitButton::GetSplitRect() const
{
    Rect r = GetSize();
    if(r.IsEmpty())
        return r;
    int w = min(max(DPI(18), split_width_), max(0, r.GetWidth()));
    return Rect(r.right - w, r.top, r.right, r.bottom);
}

Rect UiSplitButton::GetMainRect() const
{
    Rect r = GetSize();
    Rect split = GetSplitRect();
    r.right = max(r.left, split.left);
    return r;
}

UiSplitButton& UiSplitButton::Add(const String& text, const Value& data, bool enabled)
{
    items_.Add(Item(text, data, enabled));
    Refresh();
    return *this;
}

UiSplitButton& UiSplitButton::Add(const Item& item)
{
    items_.Add(item);
    Refresh();
    return *this;
}

UiSplitButton& UiSplitButton::ClearItems()
{
    items_.Clear();
    hot_item_ = -1;
    ClosePopupInternal();
    Refresh();
    return *this;
}

UiSplitButton& UiSplitButton::SetItemDescription(int index, const String& desc)
{
    if(index >= 0 && index < items_.GetCount()) {
        items_[index].description = desc;
        if(popup_open_)
            popup_.Refresh();
    }
    return *this;
}

UiSplitButton& UiSplitButton::SetItemIcon(int index, const Image& icon, UiIconRenderMode mode)
{
    if(index >= 0 && index < items_.GetCount()) {
        items_[index].icon = icon;
        items_[index].icon_render_mode = mode;
        if(popup_open_)
            popup_.Refresh();
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

UiSplitButton& UiSplitButton::SetSplitWidth(int width)
{
    split_width_ = max(DPI(18), width);
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
    if(popup_open_ || items_.IsEmpty() || !IsEnabled())
        return;

    // The popup is intentionally owned by the split button rather than by a
    // hidden UiDropdown, so the closed control can stay one painted surface.
    popup_open_ = true;
    split_pressed_ = true;
    hot_item_ = -1;
    UpdatePopupPosition();
    popup_.PopUp(this, true, true, false);
    WhenOpen();
    Refresh();
}

void UiSplitButton::ClosePopupInternal()
{
    if(!popup_open_)
        return;

    popup_open_ = false;
    split_pressed_ = false;
    hot_item_ = -1;
    popup_.Close();
    WhenClose();
    Refresh();
}

void UiSplitButton::UpdatePopupPosition()
{
    if(!popup_open_)
        return;

    // Popup width is independent from the closed button width. This is what
    // makes compact recent/history buttons usable with long paths.
    Rect outer = GetScreenRect();
    Rect screen = GetVirtualScreenArea();
    int item_h = max(DPI(18), popup_item_height_);
    int visible_items = min(items_.GetCount(), max(1, popup_max_items_));
    int popup_h = max(item_h, visible_items * item_h);
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
    if(index < 0 || index >= items_.GetCount() || !items_[index].enabled)
        return;
    Value data = items_[index].data;
    ClosePopupInternal();
    WhenSelect(index, data);
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
    w.DrawRect(split.left, r.top + DPI(5), divider_w, max(0, r.GetHeight() - DPI(10)), line);

    int side = min(DPI(16), max(DPI(8), split.GetWidth() - DPI(10)));
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
    if(!IsEnabled())
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
    if(GetSplitRect().Contains(p) || popup_open_)
        return;
    UiButton::LeftUp(p, keyflags);
}

void UiSplitButton::MouseMove(Point p, dword keyflags)
{
    bool next_split_hot = GetSplitRect().Contains(p);
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
    int h = max(DPI(18), owner->popup_item_height_);
    return Rect(0, index * h, GetSize().cx, (index + 1) * h);
}

int UiSplitButton::PopupWindow::HitTest(Point p) const
{
    if(!owner)
        return -1;
    int h = max(DPI(18), owner->popup_item_height_);
    if(h <= 0)
        return -1;
    int q = p.y / h;
    return q >= 0 && q < owner->items_.GetCount() && GetItemRect(q).Contains(p) ? q : -1;
}

void UiSplitButton::PopupWindow::Paint(Draw& w)
{
    Rect r = GetSize();
    w.DrawRect(r, SColorPaper());
    if(!owner)
        return;

    UiButton::Style bs = owner->GetEffectiveStyle();
    Color frame = bs.palette.frame[ST_NORMAL];
    if(IsNull(frame))
        frame = Blend(SColorShadow(), SColorPaper(), 120);
    w.DrawRect(r.left, r.top, r.GetWidth(), 1, frame);
    w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, frame);
    w.DrawRect(r.left, r.top, 1, r.GetHeight(), frame);
    w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), frame);

    Font font = bs.metrics.use_text_font ? bs.metrics.text_font : bs.font;
    if(IsNull(font))
        font = StdFont();
    Font desc_font = font;
    desc_font.Height(max(DPI(7), font.GetHeight() - DPI(1)));

    // Rows are deliberately simple: text, optional description, optional icon,
    // disabled state, and hot tracking. Nested menus belong in UiMenu.
    for(int i = 0; i < owner->items_.GetCount(); i++) {
        const Item& it = owner->items_[i];
        Rect row = GetItemRect(i);
        if(row.top >= r.bottom || row.bottom <= r.top)
            continue;

        StyledState state = !it.enabled ? ST_DISABLED : owner->hot_item_ == i ? ST_HOT : ST_NORMAL;
        if(state == ST_HOT) {
            UiFill face = bs.palette.face[ST_HOT];
            if(face.IsNone())
                w.DrawRect(row.Deflated(1, 0), Blend(SColorHighlight(), SColorPaper(), 220));
            else if(face.IsSolid())
                w.DrawRect(row.Deflated(1, 0), face.color);
            else
                w.DrawRect(row.Deflated(1, 0), Blend(SColorHighlight(), SColorPaper(), 220));
        }

        Rect text = row.Deflated(DPI(10), DPI(3));
        if(!IsNull(it.icon)) {
            int side = min(DPI(18), max(DPI(12), text.GetHeight() - DPI(2)));
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
               : row.top + DPI(4);
        w.Clip(text);
        w.DrawText(text.left, ty, it.text, font, ink);
        if(!it.description.IsEmpty())
            w.DrawText(text.left, ty + title_sz.cy + DPI(1), it.description, desc_font, DisabledColor(ink));
        w.End();
    }
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
        int count = owner->items_.GetCount();
        if(count <= 0)
            return true;
        int next = owner->hot_item_;
        for(int step = 0; step < count; step++) {
            next = key == K_DOWN ? (next + 1 + count) % count
                                 : (next - 1 + count) % count;
            if(owner->items_[next].enabled) {
                SetHot(next);
                break;
            }
        }
        return true;
    }
    return false;
}

void UiSplitButton::PopupWindow::Deactivate()
{
    if(owner)
        owner->ClosePopupInternal();
}

} // namespace Upp
