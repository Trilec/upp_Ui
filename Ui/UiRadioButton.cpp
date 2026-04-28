#include <Ui/UiRadioButton.h>
#include <Ui/UiIndicatorSupport.h>
#include <Ui/UiTheme.h>

namespace Upp {

const UiRadioButton::Style& UiRadioButton::StyleDefault()
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
        s.metrics.content_margin = Rect(0, 0, 0, 0);

        s.indicator_metrics = StyledMetrics();
        s.indicator_metrics.face_enabled = true;
        s.indicator_metrics.frame_enabled = true;
        s.indicator_metrics.frame_width = DPI(1);
        s.indicator_metrics.radius = DPI(999);

        s.skin = StyledSkin();
        s.indicator_skin = StyledSkin();
        s.font = StdFont();
        s.indicator_side = UiAlign::LEFT;
        s.indicator_size = DPI(18);
        s.indicator_gap = DPI(10);
    }
    return s;
}

UiRadioButton::UiRadioButton()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
{
    SyncThemeStyle();
}

void UiRadioButton::InvalidateStyleCache()
{
    theme_revision_ = 0;
    InvalidateIndicatorCaches();
}

UiRadioButton::Style& UiRadioButton::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiRadioButton::SyncThemeStyle()
{
    if(has_style_override_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveRadioButton(visual_);
    theme_revision_ = revision;
    InvalidateIndicatorCaches();
}

const UiRadioButton::Style& UiRadioButton::GetEffectiveStyle() const
{
    if(has_style_override_)
        return style_;

    const_cast<UiRadioButton*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiRadioButton& UiRadioButton::SetStyle(const Style& s)
{
    style_ = s;
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiRadioButton& UiRadioButton::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;

    has_style_override_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiRadioButton::OnStyleChanged()
{
    OnIndicatorStyleChanged(GetEffectiveStyle().font);
}

UiRadioButton& UiRadioButton::SetText(const String& s)
{
    SetIndicatorTextValue(s, GetEffectiveStyle().font);
    return *this;
}

UiRadioButton& UiRadioButton::SetVisual(UiRadioVisual vis)
{
    if(vis < UIRADIOVIS_CLASSIC || vis > UIRADIOVIS_LIST)
        vis = UIRADIOVIS_CLASSIC;
    if(visual_ == vis)
        return *this;

    visual_ = vis;
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

UiRadioButton& UiRadioButton::SetIndicatorSide(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT)
        side = UiAlign::LEFT;
    StyleEdit().indicator_side = side;
    OnStyleChanged();
    return *this;
}

UiRadioButton& UiRadioButton::SetIndicatorRadius(int px)
{
    StyleEdit().indicator_metrics.radius = max(0, px);
    OnStyleChanged();
    return *this;
}

UiRadioButton& UiRadioButton::SetIndicatorRoundness(int percent)
{
    percent = clamp(percent, 0, 100);
    int side = max(DPI(10), GetEffectiveStyle().indicator_size);
    int r = (side * percent) / 2 / 100;
    return SetIndicatorRadius(r);
}

void UiRadioButton::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Rect r = GetSize();
    if(r.IsEmpty())
        return;

    StyledState st = GetIndicatorStyledState();
    bool has_focus = HasFocus();

    if(WhenPaintBackground)
        WhenPaintBackground(w, r, style.palette, style.metrics, style.skin, st, has_focus);
    else if(visual_ == UIRADIOVIS_PILLS)
        UiPaintStyledBackground(w, r, style.palette, style.metrics, style.skin, checked_ ? ST_PRESSED : st, has_focus);

    const UiBlocksLayout& layout = GetIndicatorLayoutCache();
    Rect ind = layout.support;
    Rect text_r = layout.main;

    auto PaintSelectionMark = [&](const Rect& outer_ind) {
        int inset = max(DPI(2), style.indicator_metrics.frame_width + DPI(1));
        if(visual_ == UIRADIOVIS_LIST)
            inset = max(inset, DPI(3));

        Color c = style.indicator_palette.ink[st];
        if(IsNull(c)) c = SColorHighlight();

        int outer_side = max(1, min(outer_ind.GetWidth(), outer_ind.GetHeight()));
        int outer_half = max(1, outer_side / 2);
        int outer_radius = max(0, style.indicator_metrics.radius);
        int pct = min(100, (outer_radius * 100) / outer_half);
        UiPaintIndicatorRadioDot(w, outer_ind, c, inset, pct, DPI(8));
    };

    if(visual_ == UIRADIOVIS_LIST) {
        if(checked_)
            PaintSelectionMark(ind);
    }
    else {
        if(style.indicator_skin.enabled) {
            UiDraw9Slice(w, ind, style.indicator_skin.base, style.indicator_skin.slice);
            StyledMetrics mm = style.indicator_metrics;
            mm.face_enabled = false;
            UiPaintFaceFrameDash(w, ind, style.indicator_palette, mm, st);
        }
        else {
            UiPaintFaceFrameDash(w, ind, style.indicator_palette, style.indicator_metrics, st);
        }
        if(checked_)
            PaintSelectionMark(ind);
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

void UiRadioButton::Layout()
{
    const Style& style = GetEffectiveStyle();
    Size support_natural(style.indicator_size, style.indicator_size);
    LayoutIndicatorBlocks(style.metrics,
                          style.skin,
                          style.font,
                          support_natural,
                          UiAlign::LEFT,
                          UiAlign::CENTER,
                          style.indicator_side,
                          style.indicator_gap,
                          max(DPI(10), style.indicator_size));
}

Size UiRadioButton::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    Size support_natural(style.indicator_size, style.indicator_size);
    return GetIndicatorMinSize(style.metrics,
                               style.skin,
                               style.font,
                               support_natural,
                               style.indicator_side,
                               style.indicator_gap,
                               0,
                               max(GetTextSize("A", style.font).cy, style.indicator_size),
                               max(DPI(10), style.indicator_size));
}

void UiRadioButton::SetMinSize(Size sz)
{
    SetIndicatorUserMinSize(sz);
}

void UiRadioButton::UncheckSiblings_()
{
    Ctrl* p = GetParent();
    if(!p)
        return;
    for(Ctrl* c = p->GetFirstChild(); c; c = c->GetNext()) {
        UiRadioButton* rb = dynamic_cast<UiRadioButton*>(c);
        if(!rb || rb == this)
            continue;
        if(rb->group_ == group_ && rb->checked_) {
            rb->checked_ = false;
            rb->Refresh();
        }
    }
}

UiRadioButton& UiRadioButton::SetChecked(bool on)
{
    return SetCheckedInternal(on, true);
}

UiRadioButton& UiRadioButton::SetCheckedInternal(bool on, bool fire_action)
{
    if(checked_ == on)
        return *this;
    if(on)
        UncheckSiblings_();
    checked_ = on;
    Refresh();
    if(fire_action && on && WhenAction)
        WhenAction();
    return *this;
}

void UiRadioButton::SetData(const Value& v)
{
    if(IsNull(v)) {
        SetCheckedInternal(false, false);
        return;
    }
    SetCheckedInternal((bool)v, false);
}

void UiRadioButton::LeftDown(Point, dword)
{
    if(!IsEnabled() || !IsShowEnabled())
        return;
    BeginIndicatorPress();
    SetFocus();
    SetChecked(true);
    EndIndicatorPress();
}

bool UiRadioButton::Key(dword key, int)
{
    if(!IsEnabled() || !IsShowEnabled())
        return false;
    if(IsIndicatorActivationKey(key)) {
        SetChecked(true);
        return true;
    }
    return false;
}

void UiRadioButton::GotFocus() { IndicatorGotFocus(); }
void UiRadioButton::LostFocus() { IndicatorLostFocus(); }
void UiRadioButton::MouseEnter(Point, dword) { IndicatorMouseEnter(); }
void UiRadioButton::MouseLeave() { IndicatorMouseLeave(); }

}
