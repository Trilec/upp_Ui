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
    Add(content_);
    Add(sbx_);
    Add(sby_);

    content_.Transparent();

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

    Style resolved = StyleDefault();
    UiPanel::Style panel = UiTheme::ResolvePanel(UiPanelRole::Surface);
    resolved.palette = panel.palette;
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
    for(Ctrl* q = content_.GetFirstChild(); q; q = q->GetNext()) {
        if(!q->IsShown())
            continue;
        Rect r = q->GetRect();
        Size min_size = UiMeasureLayout(*q).min;
        r.right = max(r.right, r.left + min_size.cx);
        r.bottom = max(r.bottom, r.top + min_size.cy);
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
        face.right -= sby_.GetRect().GetWidth();
    if(sbx_.IsShown())
        face.bottom -= sbx_.GetRect().GetHeight();
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
    Rect view = GetFaceRect();
    if(sby_.IsShown())
        view.right -= sby_.GetRect().GetWidth();
    if(sbx_.IsShown())
        view.bottom -= sbx_.GetRect().GetHeight();
    view = UiApplyThicknessRect(view, UiNonNegativeThickness(GetEffectiveStyle().metrics.content_margin));

    int minx = content_bounds_.left;
    int miny = content_bounds_.top;

    content_.SetRect(view.left - origin_.x - minx,
                     view.top - origin_.y - miny,
                     content_size_.cx,
                     content_size_.cy);
}

void UiScrollPanel::MouseWheel(Point, int zdelta, dword)
{
    if(mode_ == UIPANELSCROLL_NONE)
        return;
    if(!sby_.IsShown() && mode_ != UIPANELSCROLL_HORIZONTAL)
        return;

    Rect view = GetViewportRect();
    int rows = max(1, view.GetHeight() / max(DPI(18), sby_.GetMinSize().cy));
    int step = max(DPI(24), rows * DPI(10));
    origin_.y -= sgn(zdelta) * step;
    UpdateScrollbars();
    ApplyScroll();
    Refresh();
}

void UiScrollPanel::Layout()
{
    const Style& style = GetEffectiveStyle();

    Rect view = GetFaceRect();
    Rect seed_view = UiApplyThicknessRect(view, UiNonNegativeThickness(style.metrics.content_margin));
    Size seed_size(max(0, seed_view.GetWidth()), max(0, seed_view.GetHeight()));

    // Seed the content host with the viewport before measuring. This prevents
    // expanding children from feeding a stale, oversized content width back into
    // the next scroll-panel measurement pass.
    content_size_ = seed_size;
    content_bounds_ = Rect(0, 0, seed_size.cx, seed_size.cy);
    content_.SetRect(seed_view.left, seed_view.top, seed_size.cx, seed_size.cy);
    content_.Layout();
    content_bounds_ = MeasureContentBounds();

    content_size_ = Size(max(seed_size.cx, content_bounds_.GetWidth()),
                         max(seed_size.cy, content_bounds_.GetHeight()));
    UpdateScrollbars();

    view = GetFaceRect();
    int vbarw = sby_.IsShown() ? max(DPI(12), sby_.GetMinSize().cx) : 0;
    int hbarh = sbx_.IsShown() ? max(DPI(12), sbx_.GetMinSize().cy) : 0;
    if(vbarw)
        view.right -= vbarw;
    if(hbarh)
        view.bottom -= hbarh;
    Rect content_view = UiApplyThicknessRect(view, UiNonNegativeThickness(style.metrics.content_margin));

    Size page_size(max(0, content_view.GetWidth()), max(0, content_view.GetHeight()));
    content_size_ = Size(max(page_size.cx, content_bounds_.GetWidth()),
                         max(page_size.cy, content_bounds_.GetHeight()));
    UpdateScrollbars();

    view = GetFaceRect();
    vbarw = sby_.IsShown() ? max(DPI(12), sby_.GetMinSize().cx) : 0;
    hbarh = sbx_.IsShown() ? max(DPI(12), sbx_.GetMinSize().cy) : 0;
    if(vbarw)
        view.right -= vbarw;
    if(hbarh)
        view.bottom -= hbarh;

    int vgap = sby_.IsShown() ? DPI(2) : 0;
    int hgap = sbx_.IsShown() ? DPI(2) : 0;
    if(sby_.IsShown())
        sby_.SetRect(view.right + vgap, view.top + hgap, max(0, vbarw - vgap), max(0, view.GetHeight() - hgap));
    if(sbx_.IsShown())
        sbx_.SetRect(view.left, view.bottom + hgap, max(0, view.GetWidth() - vgap), max(0, hbarh - hgap));

    ApplyScroll();
    content_.Layout();

    // Re-measure after children have been laid out against the final content
    // size. If real content is wider/taller, update once more and keep bars in sync.
    Rect final_bounds = MeasureContentBounds();
    Size final_size(max(content_size_.cx, final_bounds.GetWidth()),
                    max(content_size_.cy, final_bounds.GetHeight()));
    if(final_size != content_size_ || final_bounds != content_bounds_) {
        content_bounds_ = final_bounds;
        content_size_ = final_size;
        UpdateScrollbars();
        ApplyScroll();
        content_.Layout();
    }

    // A restore/fullscreen transition can move the bars under a stationary
    // mouse cursor without generating a fresh enter event. Resync hover state
    // explicitly so the painted hot gutter/thumb reflects reality.
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
