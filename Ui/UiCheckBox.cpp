#include <Ui/UiCheckBox.h>
#include <Ui/UiIcons.h>
#include <Ui/UiTheme.h>

namespace Upp {

static StyledState UiCheckToStyledState_(bool enabled, bool pressed, bool hover)
{
    if(!enabled) return ST_DISABLED;
    if(pressed)  return ST_PRESSED;
    if(hover)    return ST_HOT;
    return ST_NORMAL;
}

static Size UiCheckIndicatorExtent(const UiCheckBox::Style& style)
{
    Size extent = style.indicator_extent;
    if(extent.cx <= 0)
        extent.cx = style.indicator_size;
    if(extent.cy <= 0)
        extent.cy = style.indicator_size;
    extent.cx = max(DPI(10), extent.cx);
    extent.cy = max(DPI(10), extent.cy);
    return extent;
}

const UiCheckBox::Style& UiCheckBox::StyleDefault()
{
    static Style s;
    ONCELOCK {
        const Color text_primary = Color(17, 24, 39);
        const Color text_muted   = Color(148, 163, 184);
        const Color indicator_face = Color(255, 255, 255);
        const Color indicator_hot = Color(248, 250, 252);
        const Color indicator_down = Color(241, 245, 249);
        const Color indicator_frame = Color(203, 213, 225);
        const Color indicator_hot_frame = Color(148, 163, 184);
        const Color indicator_press_frame = Color(100, 116, 139);
        const Color accent = Color(37, 99, 235);

        for(int st = 0; st < 4; st++) {
            s.palette.face[st] = UiFill::None();
            s.palette.frame[st] = Null;
            s.palette.ink[st] = text_primary;
            s.indicator_palette.face[st] = UiFill::Solid(indicator_face);
            s.indicator_palette.frame[st] = indicator_frame;
            s.indicator_palette.ink[st] = accent;
        }

        s.palette.ink[ST_DISABLED] = text_muted;
        s.indicator_palette.face[ST_HOT] = UiFill::Solid(indicator_hot);
        s.indicator_palette.face[ST_PRESSED] = UiFill::Solid(indicator_down);
        s.indicator_palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.indicator_palette.frame[ST_HOT] = indicator_hot_frame;
        s.indicator_palette.frame[ST_PRESSED] = indicator_press_frame;
        s.indicator_palette.frame[ST_DISABLED] = Color(226, 232, 240);
        s.indicator_palette.ink[ST_DISABLED] = text_muted;

        s.metrics = StyledMetrics();
        s.metrics.face_enabled = false;
        s.metrics.frame_enabled = false;
        s.metrics.content_padding = Rect(0, 0, 0, 0);

        s.indicator_metrics = StyledMetrics();
        s.indicator_metrics.face_enabled = true;
        s.indicator_metrics.frame_enabled = true;
        s.indicator_metrics.frame_width = DPI(1);
        s.indicator_metrics.radius = DPI(4);

        s.skin = StyledSkin();
        s.indicator_skin = StyledSkin();
        s.font = StdFont();
        s.align_h = UiAlign::LEFT;
        s.align_v = UiAlign::CENTER;
        s.indicator_side = UiAlign::LEFT;
        s.indicator_size = DPI(18);
        s.indicator_gap = DPI(10);
        s.mark_thickness = DPI(2);
    }
    return s;
}

UiCheckBox::UiCheckBox()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
{
    BackPaint();
    WantFocus();
    SyncThemeStyle();
}

void UiCheckBox::InvalidateStyleCache()
{
    theme_revision_ = 0;
    text_size_dirty_ = true;
    layout_dirty_ = true;
    layout_content_cache_ = Rect(0, 0, 0, 0);
}

UiCheckBox::Style& UiCheckBox::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiCheckBox::SyncThemeStyle()
{
    if(has_style_override_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveCheckBox(visual_);
    theme_revision_ = revision;
    text_size_dirty_ = true;
    layout_dirty_ = true;
    layout_content_cache_ = Rect(0, 0, 0, 0);
}

const UiCheckBox::Style& UiCheckBox::GetEffectiveStyle() const
{
    if(has_style_override_)
        return style_;

    const_cast<UiCheckBox*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiCheckBox& UiCheckBox::SetStyle(const Style& s)
{
    style_ = s;
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiCheckBox& UiCheckBox::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;

    has_style_override_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiCheckBox::OnStyleChanged()
{
    const Style& style = GetEffectiveStyle();
    text_size_cache_ = GetTextSize(text_, style.font);
    text_size_dirty_ = false;
    layout_dirty_ = true;
    layout_content_cache_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
}

UiCheckBox& UiCheckBox::SetText(const String& s)
{
    const Style& style = GetEffectiveStyle();
    text_ = s;
    text_size_cache_ = GetTextSize(text_, style.font);
    text_size_dirty_ = false;
    layout_dirty_ = true;
    layout_content_cache_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

Size UiCheckBox::GetTextSizeCached() const
{
    const Style& style = GetEffectiveStyle();
    if(text_size_dirty_) {
        text_size_cache_ = GetTextSize(text_, style.font);
        text_size_dirty_ = false;
    }
    return text_size_cache_;
}

UiCheckBox& UiCheckBox::SetTriState(bool on)
{
    tri_state_ = on;
    if(!tri_state_ && state_ == UICHECK_INDETERMINATE)
        state_ = UICHECK_UNCHECKED;
    Refresh();
    return *this;
}

UiCheckBox& UiCheckBox::SetState(UiCheckState st)
{
    return SetStateInternal(st, true);
}

UiCheckBox& UiCheckBox::SetStateInternal(UiCheckState st, bool fire_action)
{
    if(!tri_state_ && st == UICHECK_INDETERMINATE)
        st = UICHECK_UNCHECKED;
    if(state_ == st)
        return *this;
    state_ = st;
    Refresh();
    if(fire_action && WhenAction)
        WhenAction();
    return *this;
}

UiCheckBox& UiCheckBox::SetVisual(UiCheckVisual vis)
{
    if(vis < UICHECKVIS_CLASSIC || vis > UICHECKVIS_LIST)
        vis = UICHECKVIS_CLASSIC;
    if(visual_ == vis)
        return *this;

    visual_ = vis;
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

UiCheckBox& UiCheckBox::SetIndicatorSide(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT)
        side = UiAlign::LEFT;
    StyleEdit().indicator_side = side;
    OnStyleChanged();
    return *this;
}

UiCheckBox& UiCheckBox::SetIndicatorRadius(int px)
{
    StyleEdit().indicator_metrics.radius = max(0, px);
    OnStyleChanged();
    return *this;
}

UiCheckBox& UiCheckBox::SetIndicatorRoundness(int percent)
{
    percent = clamp(percent, 0, 100);
    int side = max(DPI(10), GetEffectiveStyle().indicator_size);
    int r = (side * percent) / 2 / 100;
    return SetIndicatorRadius(r);
}

Rect UiCheckBox::GetIndicatorRect(const Rect& r) const
{
    const Style& style = GetEffectiveStyle();
    Size extent = UiCheckIndicatorExtent(style);
    int y = r.top + (r.GetHeight() - extent.cy) / 2;
    int x = style.indicator_side == UiAlign::RIGHT ? (r.right - extent.cx) : r.left;
    return RectC(x, y, extent.cx, extent.cy);
}

Rect UiCheckBox::GetTextRect(const Rect& r, const Rect& ind) const
{
    const Style& style = GetEffectiveStyle();
    Rect t = r;
    if(visual_ == UICHECKVIS_CHIP)
        return t;
    if(style.indicator_side == UiAlign::RIGHT)
        t.right = max(t.left, ind.left - style.indicator_gap);
    else
        t.left = min(t.right, ind.right + style.indicator_gap);
    return t;
}

void UiCheckBox::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Rect r = GetSize();
    if(r.IsEmpty())
        return;

    StyledState st = UiCheckToStyledState_(IsEnabled() && IsShowEnabled(), pressed_, hover_);
    bool has_focus = HasFocus();

    if(WhenPaintBackground)
        WhenPaintBackground(w, r, style.palette, style.metrics, style.skin, st, has_focus);
    else if(visual_ == UICHECKVIS_CHIP)
        UiPaintStyledBackground(w, r, style.palette, style.metrics, style.skin, st, has_focus);

    Rect ind = layout_cache_.support;
    Rect text_r = layout_cache_.main;

    auto DrawCheckSmall = [&](const Rect& rc) {
        Image mk = ICON_DESIGN_CHECK_SMALL_48();
        if(IsNull(mk))
            return false;

        Rect rr = rc;
        rr.Deflate(DPI(3), DPI(3));
        if(rr.GetWidth() <= 0 || rr.GetHeight() <= 0)
            return false;

        Size isz = mk.GetSize();
        if(isz.cx <= 0 || isz.cy <= 0)
            return false;

        double sx = (double)rr.GetWidth() / isz.cx;
        double sy = (double)rr.GetHeight() / isz.cy;
        double s = min(sx, sy);
        int dw = max(1, (int)floor(isz.cx * s + 0.5));
        int dh = max(1, (int)floor(isz.cy * s + 0.5));
        Image scaled = CachedRescale(mk, Size(dw, dh));
        int x = rr.left + (rr.GetWidth() - dw) / 2;
        int y = rr.top + (rr.GetHeight() - dh) / 2;
        w.DrawImage(x, y, scaled);
        return true;
    };

    if(visual_ == UICHECKVIS_SWITCH) {
        if(style.indicator_skin.enabled) {
            UiDraw9Slice(w, ind, style.indicator_skin.base, style.indicator_skin.slice);
            StyledMetrics mm = style.indicator_metrics;
            mm.face_enabled = false;
            UiPaintFaceFrameDash(w, ind, style.indicator_palette, mm, st);
        }
        else {
            UiPaintFaceFrameDash(w, ind, style.indicator_palette, style.indicator_metrics, st);
        }
        int thumb = max(DPI(8), ind.GetHeight() - DPI(4));
        int x = ind.left + DPI(2);
        if(state_ == UICHECK_CHECKED)
            x = ind.right - DPI(2) - thumb;
        else if(state_ == UICHECK_INDETERMINATE)
            x = ind.left + (ind.GetWidth() - thumb) / 2;
        Rect tr = RectC(x, ind.top + (ind.GetHeight() - thumb) / 2, thumb, thumb);
        StyledPalette tp = style.indicator_palette;
        for(int i = 0; i < 4; i++)
            tp.face[i] = UiFill::Solid(i == ST_DISABLED ? SColorDisabled() : SColorPaper());
        StyledMetrics tm = style.indicator_metrics;
        tm.frame_enabled = false;
        UiPaintFaceFrameDash(w, tr, tp, tm, st);
    }
    else if(visual_ == UICHECKVIS_LIST) {
        if(state_ != UICHECK_UNCHECKED) {
            Color mk = style.indicator_palette.ink[st];
            if(IsNull(mk)) mk = SColorHighlight();
            int t = max(1, style.mark_thickness);
            int cx = ind.left + ind.GetWidth() / 2;
            int cy = ind.top + ind.GetHeight() / 2;
            if(state_ == UICHECK_INDETERMINATE)
                w.DrawRect(ind.left + DPI(2), cy, max(1, ind.GetWidth() - DPI(4)), t, mk);
            else if(!DrawCheckSmall(ind)) {
                w.DrawLine(ind.left + DPI(2), cy, cx - DPI(1), ind.bottom - DPI(3), t, mk);
                w.DrawLine(cx - DPI(1), ind.bottom - DPI(3), ind.right - DPI(2), ind.top + DPI(3), t, mk);
            }
        }
    }
    else {
        UiPaintFaceFrameDash(w, ind, style.indicator_palette, style.indicator_metrics, st);
        if(state_ != UICHECK_UNCHECKED) {
            Color mk = style.indicator_palette.ink[st];
            if(IsNull(mk)) mk = SColorHighlight();
            int t = max(1, style.mark_thickness);
            int cx = ind.left + ind.GetWidth() / 2;
            int cy = ind.top + ind.GetHeight() / 2;
            if(state_ == UICHECK_INDETERMINATE)
                w.DrawRect(ind.left + DPI(3), cy, max(1, ind.GetWidth() - DPI(6)), t, mk);
            else if(!DrawCheckSmall(ind)) {
                w.DrawLine(ind.left + DPI(3), cy, cx - DPI(1), ind.bottom - DPI(4), t, mk);
                w.DrawLine(cx - DPI(1), ind.bottom - DPI(4), ind.right - DPI(3), ind.top + DPI(4), t, mk);
            }
        }
    }

    Color ink = style.palette.ink[st];
    if(IsNull(ink)) ink = SColorText();
    Font f = style.font;
    int ty = text_r.top + (text_r.GetHeight() - f.GetHeight()) / 2;
    DrawSmartText(w, text_r.left, ty, max(0, text_r.GetWidth()), text_, f, ink, 0);

    if(WhenPaintForeground)
        WhenPaintForeground(w, r, style.palette, style.metrics, style.skin, st, has_focus);
    else
        UiPaintStyledForeground(w, r, style.palette, style.metrics, style.skin, st, has_focus);
}

void UiCheckBox::RebuildLayoutCache(const Rect& content) const
{
    const Style& style = GetEffectiveStyle();
    if(!layout_dirty_ && layout_content_cache_ == content)
        return;

    Size support_natural = UiCheckIndicatorExtent(style);
    Size main_natural = GetTextSizeCached();
    Rect main_margin = style.indicator_side == UiAlign::RIGHT
                       ? Rect(0, 0, style.indicator_gap, 0)
                       : Rect(style.indicator_gap, 0, 0, 0);

    layout_cache_ = UiComputeBlocksLayout(content,
                                          support_natural,
                                          main_natural,
                                          style.align_h,
                                          style.align_v,
                                          style.indicator_side,
                                          Rect(0, 0, 0, 0),
                                          main_margin,
                                          max(DPI(10), max(support_natural.cx, support_natural.cy)));
    layout_content_cache_ = content;
    layout_dirty_ = false;
}

void UiCheckBox::Layout()
{
    const Style& style = GetEffectiveStyle();
    Rect content = UiStyledInnerRect(GetSize(), style.metrics, style.skin);
    RebuildLayoutCache(content);
}

Size UiCheckBox::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    if(user_min_size_.cx > 0 && user_min_size_.cy > 0)
        return user_min_size_;

    Size support_natural = UiCheckIndicatorExtent(style);
    Size main_natural = GetTextSizeCached();
    Rect main_margin = style.indicator_side == UiAlign::RIGHT
                       ? Rect(0, 0, style.indicator_gap, 0)
                       : Rect(style.indicator_gap, 0, 0, 0);

    Size content = UiMeasureBlocksContent(support_natural,
                                          main_natural,
                                          Rect(0, 0, 0, 0),
                                          main_margin,
                                          style.indicator_side,
                                          true,
                                          !text_.IsEmpty(),
                                          0,
                                          max(GetTextSize("A", style.font).cy, support_natural.cy),
                                          max(DPI(10), max(support_natural.cx, support_natural.cy)));
    return UiStyledOuterSizeFromContent(content, style.metrics, style.skin);
}

void UiCheckBox::SetMinSize(Size sz)
{
    user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    layout_dirty_ = true;
    RefreshLayout();
}

void UiCheckBox::Toggle_()
{
    UiCheckState next;
    if(tri_state_) {
        if(state_ == UICHECK_UNCHECKED) next = UICHECK_CHECKED;
        else if(state_ == UICHECK_CHECKED) next = UICHECK_INDETERMINATE;
        else next = UICHECK_UNCHECKED;
    }
    else
        next = (state_ == UICHECK_CHECKED) ? UICHECK_UNCHECKED : UICHECK_CHECKED;

    SetState(next);
}

void UiCheckBox::LeftDown(Point, dword)
{
    if(!IsEnabled() || !IsShowEnabled())
        return;
    pressed_ = true;
    SetFocus();
    Toggle_();
    pressed_ = false;
}

bool UiCheckBox::Key(dword key, int)
{
    if(key == K_SPACE || key == K_ENTER) {
        Toggle_();
        return true;
    }
    return false;
}

void UiCheckBox::GotFocus() { has_focus_ = true; Refresh(); }
void UiCheckBox::LostFocus() { has_focus_ = false; Refresh(); }
void UiCheckBox::MouseEnter(Point, dword) { hover_ = true; Refresh(); }
void UiCheckBox::MouseLeave() { hover_ = false; Refresh(); }

void UiCheckBox::SetData(const Value& v)
{
    if(IsNull(v)) {
        SetStateInternal(UICHECK_UNCHECKED, false);
        return;
    }
    if(v.Is<int>())
        SetStateInternal((UiCheckState)minmax((int)v, 0, 2), false);
    else
        SetStateInternal((bool)v ? UICHECK_CHECKED : UICHECK_UNCHECKED, false);
}

Value UiCheckBox::GetData() const
{
    return (int)state_;
}

}



