#include <Ui/UiScrollPanel.h>

namespace Upp {

const UiScrollPanel::Style& UiScrollPanel::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color face  = Blend(SColorFace(), White(), 16);
        Color frame = Blend(SColorShadow(), Black(), 18);
        Color ink   = SColorText();

        for(int i = 0; i < 4; i++) {
            s.palette.face[i]  = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i]   = ink;
        }
        s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorDisabled(), 50));

        s.metrics.radius        = DPI(8);
        s.metrics.frame_width   = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled  = true;
        s.metrics.content_padding = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
    }
    return s;
}

const UiScrollPanel::Style& UiScrollPanel::StyleFlat()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();
        s.metrics.radius = 0;
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

    BackPaint();
}

UiScrollPanel& UiScrollPanel::SetStyle(const Style& s)
{
    style_ = s;
    OnStyleChanged();
    return *this;
}

void UiScrollPanel::OnStyleChanged()
{
    if(style_.transparent)
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
    Rect content_area = UiStyledInnerRect(GetSize(), style_.metrics, style_.skin);
    content_bounds_ = MeasureContentBounds();

    int logical_w = max(content_area.GetWidth(), content_bounds_.GetWidth());
    int logical_h = max(content_area.GetHeight(), content_bounds_.GetHeight());
    content_size_ = Size(max(0, logical_w), max(0, logical_h));

    UpdateScrollbars();
    ApplyScroll();
}

Size UiScrollPanel::GetMinSize() const
{
    Size base = UiStyledOuterSizeFromContent(Size(DPI(80), DPI(60)), style_.metrics, style_.skin);
    if(user_min_size_.cx > 0) base.cx = max(base.cx, user_min_size_.cx);
    if(user_min_size_.cy > 0) base.cy = max(base.cy, user_min_size_.cy);
    return base;
}

void UiScrollPanel::Paint(Draw& w)
{
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    StyledState st = IsEnabled() ? ST_NORMAL : ST_DISABLED;
    bool has_focus = HasFocus();

    bool bg_handled = false;
    bool fg_handled = false;

    if(WhenPaintBackground) {
        WhenPaintBackground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);
        bg_handled = true;
    }

    if(WhenPaintForeground) {
        WhenPaintForeground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);
        fg_handled = true;
    }

    // See UiDraw.h: UiPaintStyledSurface contract (hook order + fallback).
    UiPaintStyledSurface(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus,
                         bg_handled, fg_handled, style_.show_focus);
}

}
