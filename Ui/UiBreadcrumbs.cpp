#include <Ui/UiBreadcrumbs.h>
#include <Ui/UiTheme.h>

namespace Upp {

const UiBreadcrumbs::Style& UiBreadcrumbs::StyleDefault()
{
    static Style s;
    ONCELOCK {
        UiThemeContext ctx = UiTheme::GetContext();
        UiThemeDetail::MinimalRoleColors c = UiThemeDetail::MinimalRole(ctx.mode, UiRole::Subtle);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(c.face);
            s.palette.frame[i] = c.frame;
            s.palette.ink[i] = c.ink;
            s.palette.icon[i] = c.ink;
        }
        s.palette.face[ST_HOT] = UiFill::Solid(c.face_hot);
        s.metrics.radius = DPI(8);
        s.metrics.frame_width = DPI(1);
        s.metrics.frame_enabled = false;
        s.metrics.face_enabled = false;
        s.metrics.content_margin = Rect(DPI(10), DPI(5), DPI(10), DPI(5));
        s.metrics.shadow.enabled = false;
        s.divider_ink = c.ink_disabled;
        s.current_underline = c.accent;
    }
    return s;
}

UiBreadcrumbs::UiBreadcrumbs()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
{
    Transparent();
    NoWantFocus();
}

void UiBreadcrumbs::InvalidateStyleCache()
{
    theme_revision_ = 0;
    layout_dirty_ = true;
}

UiBreadcrumbs::Style& UiBreadcrumbs::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiBreadcrumbs::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 rev = UiTheme::GetRevision();
    if(theme_revision_ == rev)
        return;

    UiThemeContext ctx = UiTheme::GetContext();
    themed_style_ = StyleDefault();
    UiThemeDetail::MinimalRoleColors shell = UiThemeDetail::MinimalRole(ctx.mode, UiRole::Subtle);
    for(int i = 0; i < 4; i++) {
        themed_style_.palette.face[i] = UiFill::Solid(shell.face);
        themed_style_.palette.frame[i] = shell.frame;
    }
    ResolveRoleStyle(themed_style_, ctx.mode);
    theme_revision_ = rev;
    layout_dirty_ = true;
}

void UiBreadcrumbs::ResolveRoleStyle(Style& s, UiThemeMode mode)
{
    UiThemeContext ctx = UiTheme::GetContext();
    ctx.mode = mode;
    UiThemeDetail::MinimalRoleColors shell = UiThemeDetail::MinimalRole(ctx.mode, UiRole::Subtle);
    UiLabel::Style text = UiTheme::ResolveLabel(ctx, s.text_role, s.text_size);
    UiLabel::Style current = UiTheme::ResolveLabel(ctx, s.current_role, s.current_size);
    for(int i = 0; i < 4; i++) {
        s.palette.ink[i] = text.palette.ink[i];
        s.palette.icon[i] = text.palette.icon[i];
    }
    s.text_ink = text.palette.ink[ST_NORMAL];
    s.current_ink = current.palette.ink[ST_NORMAL];
    if(IsNull(s.current_underline))
        s.current_underline = current.palette.ink[ST_NORMAL];
    s.divider_ink = shell.ink_disabled;
}

const UiBreadcrumbs::Style& UiBreadcrumbs::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiBreadcrumbs *>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiBreadcrumbs::OnStyleChanged()
{
    if(GetEffectiveStyle().metrics.face_enabled)
        BackPaint();
    else
        Transparent();
    InvalidateStyleCache();
    RefreshLayout();
    Refresh();
}

UiBreadcrumbs& UiBreadcrumbs::SetCustomStyle(const Style& s)
{
    style_ = s;
    ResolveRoleStyle(style_, UiTheme::GetContext().mode);
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
    OnStyleChanged();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::AddCrumb(const String& text, const Value& data)
{
    Item& it = items_.Add();
    it.text = text;
    it.data = data;
    current_ = items_.GetCount() - 1;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::SetPath(const Vector<String>& path)
{
    items_.Clear();
    for(int i = 0; i < path.GetCount(); i++) {
        Item& it = items_.Add();
        it.text = path[i];
        it.data = i;
    }
    current_ = items_.IsEmpty() ? -1 : items_.GetCount() - 1;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::SetPath(const String& path, const String& separator)
{
    Vector<String> part = Split(path, separator);
    Vector<String> clean;
    for(String s : part)
        if(!s.IsEmpty())
            clean.Add(s);
    return SetPath(clean);
}

UiBreadcrumbs& UiBreadcrumbs::SetItems(const Vector<Item>& items)
{
    items_ <<= items;
    current_ = items_.IsEmpty() ? -1 : items_.GetCount() - 1;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::ClearItems()
{
    items_.Clear();
    current_ = -1;
    hot_ = -1;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::SetCurrentIndex(int i)
{
    current_ = items_.IsEmpty() ? -1 : min(max(0, i), items_.GetCount() - 1);
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

Value UiBreadcrumbs::GetCurrentData() const
{
    return current_ >= 0 && current_ < items_.GetCount() ? items_[current_].data : Value();
}

Value UiBreadcrumbs::GetItemData(int i) const
{
    return i >= 0 && i < items_.GetCount() ? items_[i].data : Value();
}

String UiBreadcrumbs::GetPathText(const String& separator) const
{
    String out;
    for(int i = 0; i < items_.GetCount(); i++) {
        if(i)
            out << separator;
        out << items_[i].text;
    }
    return out;
}

UiBreadcrumbs& UiBreadcrumbs::SetTrimOnSelect(bool b)
{
    trim_on_select_ = b;
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::SetDivider(const String& text)
{
    Style& s = StyleEdit();
    s.divider = text;
    s.divider_icon = Image();
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::SetDividerIcon(const Image& icon, Size size)
{
    Style& s = StyleEdit();
    s.divider_icon = icon;
    s.divider_icon_size = size;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::ClearDividerIcon()
{
    Style& s = StyleEdit();
    s.divider_icon = Image();
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::SetPathIcon(const Image& icon, UiAlign side, Size size)
{
    Style& s = StyleEdit();
    s.path_icon = icon;
    s.path_icon_side = side;
    if(size.cx > 0 && size.cy > 0)
        s.path_icon_size = size;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::ClearPathIcon()
{
    StyleEdit().path_icon = Image();
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::SetPathIconSide(UiAlign side)
{
    StyleEdit().path_icon_side = side;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::SetPathIconSize(Size size)
{
    StyleEdit().path_icon_size = size;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::SetRoles(UiRole text_role, UiRole current_role)
{
    Style& s = StyleEdit();
    s.text_role = text_role;
    s.current_role = current_role;
    layout_dirty_ = true;
    Refresh();
    return *this;
}

UiBreadcrumbs& UiBreadcrumbs::SetTextSizes(UiTextSize text_size, UiTextSize current_size)
{
    Style& s = StyleEdit();
    s.text_size = text_size;
    s.current_size = current_size;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

Size UiBreadcrumbs::GetItemSize(const Item& item, bool current) const
{
    const Style& s = GetEffectiveStyle();
    Font f = current ? s.current_font : s.font;
    return GetTextSize(item.text, IsNull(f) ? StdFont() : f);
}

Size UiBreadcrumbs::GetPathIconSize() const
{
    const Style& s = GetEffectiveStyle();
    if(IsNull(s.path_icon))
        return Size(0, 0);
    Size icon = s.path_icon_size;
    if(icon.cx <= 0 || icon.cy <= 0)
        icon = Size(DPI(18), DPI(18));
    return icon;
}

Size UiBreadcrumbs::GetMinSize() const
{
    const Style& s = GetEffectiveStyle();
    Rect m = s.metrics.content_margin;
    int w = m.left + m.right;
    int h = max(s.min_height, m.top + m.bottom);
    Size path_icon = GetPathIconSize();
    bool icon_vertical = !IsNull(s.path_icon) && (s.path_icon_side == UiAlign::TOP || s.path_icon_side == UiAlign::BOTTOM);
    int path_w = 0;
    int path_h = 0;
    for(int i = 0; i < items_.GetCount(); i++) {
        Size isz = GetItemSize(items_[i], i == current_);
        path_w += isz.cx;
        path_h = max(path_h, isz.cy);
        if(i < items_.GetCount() - 1) {
            Size dsz = IsNull(s.divider_icon)
                ? GetTextSize(s.divider, s.font)
                : (s.divider_icon_size.cx > 0 && s.divider_icon_size.cy > 0 ? s.divider_icon_size : Size(DPI(14), DPI(14)));
            path_w += s.divider_gap * 2 + dsz.cx;
            path_h = max(path_h, dsz.cy);
        }
    }
    if(!IsNull(s.path_icon)) {
        if(icon_vertical) {
            path_w = max(path_w, path_icon.cx);
            path_h += s.content_gap + path_icon.cy;
        }
        else {
            path_w += s.content_gap + path_icon.cx;
            path_h = max(path_h, path_icon.cy);
        }
    }
    w += path_w;
    h = max(h, m.top + path_h + m.bottom);
    return Size(w, h);
}

void UiBreadcrumbs::RebuildLayout(Size sz) const
{
    if(!layout_dirty_ && layout_size_ == sz)
        return;
    pieces_.Clear();
    layout_size_ = sz;
    layout_dirty_ = false;

    const Style& s = GetEffectiveStyle();
    Rect content = UiStyledInnerRect(Rect(sz), s.metrics, s.skin);
    Size path_icon = GetPathIconSize();
    bool has_icon = !IsNull(s.path_icon);
    bool icon_vertical = has_icon && (s.path_icon_side == UiAlign::TOP || s.path_icon_side == UiAlign::BOTTOM);
    int x = content.left;
    int y = content.top;
    if(has_icon && !icon_vertical && s.path_icon_side == UiAlign::LEFT)
        x += path_icon.cx + s.content_gap;
    if(has_icon && icon_vertical && s.path_icon_side == UiAlign::TOP)
        y += path_icon.cy + s.content_gap;
    int cy = content.GetHeight();
    if(has_icon && icon_vertical)
        cy = max(0, cy - path_icon.cy - s.content_gap);
    for(int i = 0; i < items_.GetCount(); i++) {
        Size isz = GetItemSize(items_[i], i == current_);
        Piece p;
        p.item = true;
        p.index = i;
        p.rect = RectC(x, y + (cy - isz.cy) / 2, isz.cx, isz.cy);
        pieces_.Add(p);
        x += isz.cx;
        if(i < items_.GetCount() - 1) {
            Size dsz = IsNull(s.divider_icon)
                ? GetTextSize(s.divider, s.font)
                : (s.divider_icon_size.cx > 0 && s.divider_icon_size.cy > 0 ? s.divider_icon_size : Size(DPI(14), DPI(14)));
            Piece d;
            d.item = false;
            d.index = i;
            d.rect = RectC(x + s.divider_gap, y + (cy - dsz.cy) / 2, dsz.cx, dsz.cy);
            pieces_.Add(d);
            x += s.divider_gap * 2 + dsz.cx;
        }
    }
}

void UiBreadcrumbs::Layout()
{
    RebuildLayout(GetSize());
}

void UiBreadcrumbs::Paint(Draw& w)
{
    const Style& s = GetEffectiveStyle();
    Rect outer = GetSize();
    UiPaintStyledBackground(w, outer, s.palette, s.metrics, s.skin, ST_NORMAL, HasFocus());
    RebuildLayout(outer.GetSize());

    if(!IsNull(s.path_icon)) {
        Rect content = UiStyledInnerRect(Rect(outer.GetSize()), s.metrics, s.skin);
        Size icon = GetPathIconSize();
        Rect ir;
        if(s.path_icon_side == UiAlign::RIGHT)
            ir = RectC(content.right - icon.cx, content.top + (content.GetHeight() - icon.cy) / 2, icon.cx, icon.cy);
        else if(s.path_icon_side == UiAlign::TOP)
            ir = RectC(content.left + (content.GetWidth() - icon.cx) / 2, content.top, icon.cx, icon.cy);
        else if(s.path_icon_side == UiAlign::BOTTOM)
            ir = RectC(content.left + (content.GetWidth() - icon.cx) / 2, content.bottom - icon.cy, icon.cx, icon.cy);
        else
            ir = RectC(content.left, content.top + (content.GetHeight() - icon.cy) / 2, icon.cx, icon.cy);
        UiPaintStyledIcon(w, ir, s.path_icon, true, true, s.path_icon_render, s.palette.icon[ST_NORMAL], IsEnabled());
    }

    for(const Piece& p : pieces_) {
        if(!p.item) {
            Color ink = IsNull(s.divider_ink) ? s.palette.ink[ST_DISABLED] : s.divider_ink;
            if(!IsNull(s.divider_icon))
                UiPaintStyledIcon(w, p.rect, s.divider_icon, true, true, s.divider_icon_render, ink, IsEnabled());
            else
                w.DrawText(p.rect.left, p.rect.top, s.divider, s.font, ink);
            continue;
        }

        const Item& item = items_[p.index];
        bool current = p.index == current_;
        Font f = current ? s.current_font : s.font;
        Color ink = current ? s.current_ink : s.text_ink;
        if(IsNull(ink))
            ink = s.palette.ink[ST_NORMAL];
        if(p.index == hot_ && !current)
            ink = s.palette.ink[ST_HOT];

        Rect r = p.rect;
        Size text = GetTextSize(item.text, f);
        w.DrawText(r.left, r.top + (r.GetHeight() - text.cy) / 2, item.text, f, ink);
        if(current && s.current_underline_enabled) {
            Color line = IsNull(s.current_underline) ? ink : s.current_underline;
            int width = max(1, s.current_underline_width);
            int y = r.top + (r.GetHeight() + text.cy) / 2 + DPI(4);
            w.DrawRect(r.left, y, text.cx, width, line);
        }
    }
}

int UiBreadcrumbs::HitTest(Point p) const
{
    RebuildLayout(GetSize());
    for(const Piece& pc : pieces_)
        if(pc.item && pc.rect.Contains(p))
            return pc.index;
    return -1;
}

void UiBreadcrumbs::LeftDown(Point p, dword)
{
    int i = HitTest(p);
    if(i < 0)
        return;
    current_ = i;
    if(trim_on_select_ && i + 1 < items_.GetCount())
        items_.SetCount(i + 1);
    layout_dirty_ = true;
    RefreshLayout();
    WhenAction(i);
    Refresh();
}

void UiBreadcrumbs::MouseMove(Point p, dword)
{
    int i = HitTest(p);
    if(i != hot_) {
        hot_ = i;
        Refresh();
    }
}

void UiBreadcrumbs::MouseLeave()
{
    if(hot_ >= 0) {
        hot_ = -1;
        Refresh();
    }
}

}
