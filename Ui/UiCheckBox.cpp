#include <Ui/UiCheckBox.h>
#include <Ui/UiIndicatorSupport.h>
#include <Ui/UiIcons.h>
#include <Ui/UiTheme.h>

namespace Upp {

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
    SyncThemeStyle();
}

void UiCheckBox::InvalidateStyleCache()
{
    theme_revision_ = 0;
    InvalidateIndicatorCaches();
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
    InvalidateIndicatorCaches();
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
    OnIndicatorStyleChanged(GetEffectiveStyle().font);
}

UiCheckBox& UiCheckBox::SetText(const String& s)
{
    SetIndicatorTextValue(s, GetEffectiveStyle().font);
    return *this;
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

void UiCheckBox::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Rect r = GetSize();
    if(r.IsEmpty())
        return;

    StyledState st = GetIndicatorStyledState();
    bool has_focus = HasFocus();

    if(WhenPaintBackground)
        WhenPaintBackground(w, r, style.palette, style.metrics, style.skin, st, has_focus);
    else if(visual_ == UICHECKVIS_CHIP)
        UiPaintStyledBackground(w, r, style.palette, style.metrics, style.skin, st, has_focus);

    const UiBlocksLayout& layout = GetIndicatorLayoutCache();
    Rect ind = layout.support;
    Rect text_r = layout.main;

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
            if(state_ == UICHECK_INDETERMINATE)
                UiPaintIndicatorBar(w, ind, mk, t, DPI(2));
            else if(!UiPaintCenteredScaledImage(w, ind, ICON_DESIGN_CHECK_SMALL_48(), DPI(3), DPI(3)))
                UiPaintIndicatorCheckStroke(w, ind, mk, t, DPI(2), 0, DPI(3), DPI(2), DPI(3));
        }
    }
    else {
        UiPaintFaceFrameDash(w, ind, style.indicator_palette, style.indicator_metrics, st);
        if(state_ != UICHECK_UNCHECKED) {
            Color mk = style.indicator_palette.ink[st];
            if(IsNull(mk)) mk = SColorHighlight();
            int t = max(1, style.mark_thickness);
            if(state_ == UICHECK_INDETERMINATE)
                UiPaintIndicatorBar(w, ind, mk, t, DPI(3));
            else if(!UiPaintCenteredScaledImage(w, ind, ICON_DESIGN_CHECK_SMALL_48(), DPI(3), DPI(3)))
                UiPaintIndicatorCheckStroke(w, ind, mk, t, DPI(3), 0, DPI(4), DPI(3), DPI(4));
        }
    }

    Color ink = style.palette.ink[st];
    if(IsNull(ink)) ink = SColorText();
    Font f = style.font;
    int ty = text_r.top + (text_r.GetHeight() - f.GetHeight()) / 2;
    DrawSmartText(w, text_r.left, ty, max(0, text_r.GetWidth()), GetIndicatorTextValue(), f, ink, 0);

    if(WhenPaintForeground)
        WhenPaintForeground(w, r, style.palette, style.metrics, style.skin, st, has_focus);
    else
        UiPaintStyledForeground(w, r, style.palette, style.metrics, style.skin, st, has_focus);
}

void UiCheckBox::Layout()
{
    const Style& style = GetEffectiveStyle();
    Size support_natural = UiCheckIndicatorExtent(style);
    LayoutIndicatorBlocks(style.metrics,
                          style.skin,
                          style.font,
                          support_natural,
                          style.align_h,
                          style.align_v,
                          style.indicator_side,
                          style.indicator_gap,
                          max(DPI(10), max(support_natural.cx, support_natural.cy)));
}

Size UiCheckBox::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    Size support_natural = UiCheckIndicatorExtent(style);
    return GetIndicatorMinSize(style.metrics,
                               style.skin,
                               style.font,
                               support_natural,
                               style.indicator_side,
                               style.indicator_gap,
                               0,
                               max(GetTextSize("A", style.font).cy, support_natural.cy),
                               max(DPI(10), max(support_natural.cx, support_natural.cy)));
}

void UiCheckBox::SetMinSize(Size sz)
{
    SetIndicatorUserMinSize(sz);
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
    BeginIndicatorPress();
    SetFocus();
    Toggle_();
    EndIndicatorPress();
}

bool UiCheckBox::Key(dword key, int)
{
    if(!IsEnabled() || !IsShowEnabled())
        return false;
    if(IsIndicatorActivationKey(key)) {
        Toggle_();
        return true;
    }
    return false;
}

void UiCheckBox::GotFocus() { IndicatorGotFocus(); }
void UiCheckBox::LostFocus() { IndicatorLostFocus(); }
void UiCheckBox::MouseEnter(Point, dword) { IndicatorMouseEnter(); }
void UiCheckBox::MouseLeave() { IndicatorMouseLeave(); }

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












