#include <Ui/UiScrollPanel.h>
#include <Ui/UiTheme.h>

namespace Upp {

const UiScrollPanel::Style& UiScrollPanel::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color face = Color(248, 250, 252);
        Color frame = Color(226, 232, 240);
        Color ink = Color(15, 23, 42);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i] = ink;
        }
        s.palette.face[ST_HOT] = UiFill::Solid(Color(241, 245, 249));
        s.palette.face[ST_PRESSED] = UiFill::Solid(Color(226, 232, 240));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.palette.ink[ST_DISABLED] = Color(148, 163, 184);

        s.metrics.radius = DPI(12);
        s.metrics.frame_width = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled = true;
        s.metrics.content_padding = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
    }
    return s;
}

UiScrollPanel::UiScrollPanel()
    : style_(StyleDefault())
{
    AddFrame(sb_);
    Add(content_);

    content_.Transparent();

    sb_.WhenScroll << [=] {
        origin_ = sb_.Get();
        ApplyScroll();
    };
    sb_.WhenLeftClick << [=] { SetFocus(); };

    SyncThemeStyle();
    BackPaint();
}

void UiScrollPanel::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiScrollPanel::Style& UiScrollPanel::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiScrollPanel::SyncThemeStyle()
{
    if(has_style_override_)
        return;
    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    Style resolved = StyleDefault();
    UiPanel::Style panel = UiTheme::ResolvePanel(UiPanelRole::Surface);
    resolved.palette = panel.palette;
    resolved.metrics.radius = max(DPI(10), panel.metrics.radius);
    resolved.metrics.frame_width = max(1, panel.metrics.frame_width);
    resolved.metrics.frame_enabled = panel.metrics.frame_enabled;
    resolved.metrics.face_enabled = panel.metrics.face_enabled;
    resolved.metrics.content_padding = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
    style_ = resolved;
    theme_revision_ = revision;
}

const UiScrollPanel::Style& UiScrollPanel::GetEffectiveStyle() const
{
    const_cast<UiScrollPanel*>(this)->SyncThemeStyle();
    return style_;
}

UiScrollPanel& UiScrollPanel::SetStyle(const Style& s)
{
    style_ = s;
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiScrollPanel& UiScrollPanel::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;
    has_style_override_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiScrollPanel::OnStyleChanged()
{
    const Style& style = GetEffectiveStyle();
    if(style.transparent)
        Transparent();
    else
        BackPaint();
    RefreshLayout();
    Refresh();
}

UiScrollPanel& UiScrollPanel::SetScrollMode(UiScrollPanelMode m)
{
    mode_ = m;
    RefreshLayout();
    Refresh();
    return *this;
}

UiScrollPanel& UiScrollPanel::SetScrollPos(Point p)
{
    origin_ = p;
    UpdateScrollbars();
    ApplyScroll();
    Refresh();
    return *this;
}

Rect UiScrollPanel::MeasureContentBounds() const
{
    Rect b(0, 0, 0, 0);
    bool first = true;
    for(Ctrl* q = content_.GetFirstChild(); q; q = q->GetNext()) {
        if(!q->IsShown())
            continue;
        Rect r = q->GetRect();
        if(first) {
            b = r;
            first = false;
        }
        else
            b |= r;
    }
    return first ? Rect(0, 0, 0, 0) : b;
}

void UiScrollPanel::UpdateScrollbars()
{
    if(updating_sb_)
        return;
    updating_sb_ = true;

    auto Decide = [&](const Size& page, bool& showx, bool& showy) {
        switch(mode_) {
        case UIPANELSCROLL_NONE:
            showx = false;
            showy = false;
            break;
        case UIPANELSCROLL_VERTICAL:
            showx = false;
            showy = true;
            break;
        case UIPANELSCROLL_HORIZONTAL:
            showx = true;
            showy = false;
            break;
        case UIPANELSCROLL_AUTO:
        default:
            showx = (content_size_.cx > page.cx);
            showy = (content_size_.cy > page.cy);
            break;
        }
    };

    for(int i = 0; i < 2; i++) {
        Size page = GetView().GetSize();
        bool sx = false, sy = false;
        Decide(page, sx, sy);
        sb_.ShowX(sx);
        sb_.ShowY(sy);
    }

    Size page = GetView().GetSize();
    Point p = origin_;
    p.x = minmax(p.x, 0, max(0, content_size_.cx - page.cx));
    p.y = minmax(p.y, 0, max(0, content_size_.cy - page.cy));
    origin_ = p;

    if(mode_ == UIPANELSCROLL_NONE) {
        sb_.HideX();
        sb_.HideY();
        origin_ = Point(0, 0);
        updating_sb_ = false;
        return;
    }

    sb_.Set(origin_, page, content_size_);
    updating_sb_ = false;
}

void UiScrollPanel::ApplyScroll()
{
    Rect view = GetView();

    int minx = content_bounds_.left;
    int miny = content_bounds_.top;

    content_.SetRect(view.left - origin_.x - minx,
                     view.top - origin_.y - miny,
                     content_size_.cx,
                     content_size_.cy);
}

void UiScrollPanel::Layout()
{
    const Style& style = GetEffectiveStyle();
    Rect content_area = UiStyledInnerRect(GetSize(), style.metrics, style.skin);
    content_bounds_ = MeasureContentBounds();

    int logical_w = max(content_area.GetWidth(), content_bounds_.GetWidth());
    int logical_h = max(content_area.GetHeight(), content_bounds_.GetHeight());
    content_size_ = Size(max(0, logical_w), max(0, logical_h));

    UpdateScrollbars();
    ApplyScroll();
}

Size UiScrollPanel::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    Size base = UiStyledOuterSizeFromContent(Size(DPI(80), DPI(60)), style.metrics, style.skin);
    if(user_min_size_.cx > 0)
        base.cx = max(base.cx, user_min_size_.cx);
    if(user_min_size_.cy > 0)
        base.cy = max(base.cy, user_min_size_.cy);
    return base;
}

void UiScrollPanel::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    StyledState st = IsEnabled() ? ST_NORMAL : ST_DISABLED;
    bool has_focus = HasFocus();

    bool bg_handled = false;
    bool fg_handled = false;

    if(WhenPaintBackground) {
        WhenPaintBackground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);
        bg_handled = true;
    }

    if(WhenPaintForeground) {
        WhenPaintForeground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);
        fg_handled = true;
    }

    UiPaintStyledSurface(w, outer, style.palette, style.metrics, style.skin, st, has_focus, bg_handled, fg_handled);
}

}

