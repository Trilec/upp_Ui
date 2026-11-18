#include <Ui/UiScrollPanel.h>
#include <Ui/UiMeasure.h>
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

        s.metrics.radius = 0;
        s.metrics.frame_width = DPI(1);
        s.metrics.frame_enabled = false;
        s.metrics.face_enabled = false;
        s.metrics.content_margin = Rect(0, 0, 0, 0);
        s.transparent = true;
    }
    return s;
}

UiScrollPanel::UiScrollPanel()
    : style_(StyleDefault())
{
    Add(viewport_);
    viewport_.Add(content_);
    Add(sbx_);
    Add(sby_);

    content_.Transparent();
    viewport_.Transparent();

    sbx_.SetDirection(UiDirection::H);
    sby_.SetDirection(UiDirection::V);

    sbx_.WhenScroll << [=] {
        origin_.x = sbx_.GetPos();
        ApplyScroll();
    };
    sby_.WhenScroll << [=] {
        origin_.y = sby_.GetPos();
        ApplyScroll();
    };

    SyncThemeStyle();
    SyncScrollBarStyles();
    BackPaint();
}

void UiScrollPanel::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiScrollPanel::Style& UiScrollPanel::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiScrollPanel::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    Style resolved = UiTheme::ResolveScrollPanel(UiRole::Standard);
    resolved.metrics.radius = 0;
    resolved.metrics.frame_width = 0;
    resolved.metrics.frame_enabled = false;
    resolved.metrics.face_enabled = false;
    resolved.metrics.content_margin = Rect(0, 0, 0, 0);
    resolved.transparent = true;
    style_ = resolved;
    theme_revision_ = revision;
    SyncScrollBarStyles();
}

const UiScrollPanel::Style& UiScrollPanel::GetEffectiveStyle() const
{
    const_cast<UiScrollPanel*>(this)->SyncThemeStyle();
    return style_;
}

UiScrollPanel& UiScrollPanel::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiScrollPanel& UiScrollPanel::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
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
    SyncScrollBarStyles();
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

UiScrollPanel& UiScrollPanel::SetCustomScrollBarStyle(const UiScrollBar::Style& s)
{
    scrollbar_style_ = s;
    has_custom_scrollbar_style_ = true;
    SyncScrollBarStyles();
    RefreshLayout();
    Refresh();
    return *this;
}

UiScrollPanel& UiScrollPanel::ClearCustomScrollBarStyle()
{
    if(!has_custom_scrollbar_style_)
        return *this;
    has_custom_scrollbar_style_ = false;
    SyncScrollBarStyles();
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
    int host_w = content_.GetRect().GetWidth();
    int viewport_w = GetViewportRect().GetWidth();
    for(Ctrl* q = content_.GetFirstChild(); q; q = q->GetNext()) {
        if(!q->IsShown())
            continue;
        Rect r = q->GetRect();
        int available_w = r.GetWidth();
        if(available_w <= 0)
            available_w = host_w;
        if(available_w <= 0)
            available_w = viewport_w;
        UiLayoutMeasureResult measure = UiMeasureLayout(*q, {available_w > 0 ? available_w : -1});
        if(measure.width_dependent) {
            int constrained_w = available_w;
            if(host_w > 0)
                constrained_w = constrained_w > 0 ? min(constrained_w, host_w) : host_w;
            if(viewport_w > 0)
                constrained_w = constrained_w > 0 ? min(constrained_w, viewport_w) : viewport_w;
            if(constrained_w > 0 && constrained_w != available_w)
                measure = UiMeasureLayout(*q, {constrained_w});
        }
        if(measure.width_dependent) {
            r.right = r.left + max(0, measure.measured.cx);
            r.bottom = r.top + max(0, measure.measured.cy);
        }
        else {
            r.right = r.left + max(r.GetWidth(), measure.min.cx);
            r.bottom = r.top + max(r.GetHeight(), measure.min.cy);
        }
        if(first) {
            b = r;
            first = false;
        }
        else
            b |= r;
    }
    return first ? Rect(0, 0, 0, 0) : b;
}

Rect UiScrollPanel::GetViewportRect() const
{
    Rect face = GetFaceRect();
    if(sby_.IsShown())
        face.right -= max(DPI(12), sby_.GetMinSize().cx);
    if(sbx_.IsShown())
        face.bottom -= max(DPI(12), sbx_.GetMinSize().cy);
    const Rect cp = UiNonNegativeThickness(GetEffectiveStyle().metrics.content_margin);
    if(!UiIsZeroThicknessRect(cp))
        face = UiApplyThicknessRect(face, cp);
    return face;
}

Rect UiScrollPanel::GetFaceRect() const
{
    return UiStyledFaceRect(GetSize(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
}

void UiScrollPanel::SyncScrollBarStyles()
{
    UiScrollBar::Style sb = has_custom_scrollbar_style_ ? scrollbar_style_ : UiTheme::ResolveScrollBar();
    sbx_.SetCustomStyle(sb);
    sby_.SetCustomStyle(sb);
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

    bool showx = false, showy = false;
    int vbarw = max(DPI(12), sby_.GetMinSize().cx);
    int hbarh = max(DPI(12), sbx_.GetMinSize().cy);

    for(int i = 0; i < 2; i++) {
        Rect view = GetFaceRect();
        if(showy)
            view.right -= vbarw;
        if(showx)
            view.bottom -= hbarh;
        Rect page_view = UiApplyThicknessRect(view, UiNonNegativeThickness(GetEffectiveStyle().metrics.content_margin));
        Size page = page_view.GetSize();
        bool sx = false, sy = false;
        Decide(page, sx, sy);
        showx = sx;
        showy = sy;
    }

    Rect view = GetFaceRect();
    if(showy)
        view.right -= vbarw;
    if(showx)
        view.bottom -= hbarh;
    Rect page_view = UiApplyThicknessRect(view, UiNonNegativeThickness(GetEffectiveStyle().metrics.content_margin));
    Size page = page_view.GetSize();
    Point p = origin_;
    p.x = minmax(p.x, 0, max(0, content_size_.cx - page.cx));
    p.y = minmax(p.y, 0, max(0, content_size_.cy - page.cy));
    origin_ = p;

    if(mode_ == UIPANELSCROLL_NONE) {
        sbx_.Hide();
        sby_.Hide();
        origin_ = Point(0, 0);
        updating_sb_ = false;
        return;
    }

    if(showx) {
        sbx_.Show();
        sbx_.SetRange(0, content_size_.cx, page.cx).SetPos(origin_.x);
    }
    else {
        sbx_.Hide();
        origin_.x = 0;
    }

    if(showy) {
        sby_.Show();
        sby_.SetRange(0, content_size_.cy, page.cy).SetPos(origin_.y);
    }
    else {
        sby_.Hide();
        origin_.y = 0;
    }

    updating_sb_ = false;
}

void UiScrollPanel::ApplyScroll()
{
    Rect view = GetViewportRect();

    int minx = content_bounds_.left;
    int miny = content_bounds_.top;

    // Clip scrolled children to the viewport, not the whole decorated control.
    viewport_.SetRect(view);
    content_.SetRect(-origin_.x - minx,
                     -origin_.y - miny,
                     max(content_size_.cx, content_bounds_.right),
                     max(content_size_.cy, content_bounds_.bottom));
    if(origin_ != notified_origin_) {
        notified_origin_ = origin_;
        WhenScroll();
    }
}

UiScrollBar* UiScrollPanel::GetScrollBarAt(Point p)
{
    if(sby_.IsShown() && sby_.GetRect().Contains(p)) return &sby_;
    if(sbx_.IsShown() && sbx_.GetRect().Contains(p)) return &sbx_;
    return nullptr;
}

void UiScrollPanel::MouseWheel(Point, int zdelta, dword keyflags)
{
    if(mode_ == UIPANELSCROLL_NONE)
        return;
    const bool horizontal = mode_ == UIPANELSCROLL_HORIZONTAL ||
                            ((keyflags & K_SHIFT) && sbx_.IsShown()) ||
                            (!sby_.IsShown() && sbx_.IsShown());
    if(horizontal ? !sbx_.IsShown() : !sby_.IsShown())
        return;

    Rect view = GetViewportRect();
    int rows = max(1, view.GetHeight() / max(DPI(18), sby_.GetMinSize().cy));
    int step = max(DPI(24), rows * DPI(10));
    if(horizontal) origin_.x -= sgn(zdelta) * step;
    else origin_.y -= sgn(zdelta) * step;
    UpdateScrollbars();
    ApplyScroll();
    Refresh();
}

void UiScrollPanel::Layout()
{
    // Scrollbars change the width supplied to responsive descendants. Start from
    // the requested mode and add only bars justified by content measured at that
    // actual page size. At most two bars can be added, so three passes suffice.
    // Keeping the initial unbarred width as a minimum used to manufacture a
    // horizontal overflow whenever a vertical scrollbar appeared.
    bool showx = mode_ == UIPANELSCROLL_HORIZONTAL;
    bool showy = mode_ == UIPANELSCROLL_VERTICAL;
    for(int pass = 0; pass < 3; ++pass) {
        sbx_.Show(showx);
        sby_.Show(showy);
        const Rect page = GetViewportRect();
        const Size page_size(max(0, page.GetWidth()), max(0, page.GetHeight()));
        viewport_.SetRect(page);
        content_.SetRect(0, 0, page_size.cx, page_size.cy);
        content_.Layout();
        content_bounds_ = MeasureContentBounds();
        content_size_ = Size(max(page_size.cx, content_bounds_.GetWidth()),
                             max(page_size.cy, content_bounds_.GetHeight()));
        if(mode_ != UIPANELSCROLL_AUTO)
            break;
        const bool nextx = showx || content_bounds_.GetWidth() > page_size.cx;
        const bool nexty = showy || content_bounds_.GetHeight() > page_size.cy;
        if(nextx == showx && nexty == showy)
            break;
        showx = nextx;
        showy = nexty;
    }
    // Measurements are intrinsic and width-dependent, independent of the previous
    // content allocation. This also permits content and scroll ranges to shrink
    // when a window grows, a wrapped row unwraps, or a model removes items.
    UpdateScrollbars();

    Rect view = GetFaceRect();
    const int vbarw = sby_.IsShown() ? max(DPI(12), sby_.GetMinSize().cx) : 0;
    const int hbarh = sbx_.IsShown() ? max(DPI(12), sbx_.GetMinSize().cy) : 0;
    if(vbarw)
        view.right -= vbarw;
    if(hbarh)
        view.bottom -= hbarh;
    const int vgap = sby_.IsShown() ? DPI(2) : 0;
    const int hgap = sbx_.IsShown() ? DPI(2) : 0;
    if(sby_.IsShown())
        sby_.SetRect(view.right + vgap, view.top + hgap, max(0, vbarw - vgap), max(0, view.GetHeight() - hgap));
    if(sbx_.IsShown())
        sbx_.SetRect(view.left, view.bottom + hgap, max(0, view.GetWidth() - vgap), max(0, hbarh - hgap));

    ApplyScroll();
    content_.Layout();

    // A restore/fullscreen transition can move the bars under a stationary
    // cursor without generating a fresh enter event.
    sby_.SyncHoverFromMouse();
    sbx_.SyncHoverFromMouse();
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
