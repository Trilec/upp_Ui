#include <Ui/UiPanel.h>

namespace Upp {

const UiPanel::Style& UiPanel::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color face  = Blend(SColorFace(), White(), 20);
        Color frame = Blend(SColorShadow(), Black(), 30);
        Color ink   = SColorText();

        for(int i = 0; i < 4; i++) {
            s.palette.face[i]  = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i]   = ink;
        }

        s.palette.face[ST_HOT]      = UiFill::Solid(LtColor(face, 5));
        s.palette.face[ST_PRESSED]  = UiFill::Solid(DkColor(face, 5));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorDisabled(), 50));

        s.metrics.radius        = DPI(8);
        s.metrics.frame_width   = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled  = true;
        s.metrics.dashed        = false;

        s.skin.enabled        = false;
        s.skin.content_inset  = Rect(0, 0, 0, 0); // correct field

        s.transparent = false;
        s.show_focus  = false;
    }
    return s;
}

const UiPanel::Style& UiPanel::StyleDark()
{
    static Style s;
    ONCELOCK {
        s = Style(StyleDefault());

        Color face  = Color(40, 40, 50);
        Color frame = Color(70, 70, 90);
        Color ink   = Color(220, 220, 230);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i]  = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i]   = ink;
        }

        s.palette.face[ST_HOT]      = UiFill::Solid(LtColor(face, 5));
        s.palette.face[ST_PRESSED]  = UiFill::Solid(DkColor(face, 5));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorDisabled(), 50));

        s.metrics.radius = DPI(8);
    }
    return s;
}

const UiPanel::Style& UiPanel::StyleFlat()
{
    static Style s;
    ONCELOCK {
        s = Style(StyleDefault());
        s.metrics.radius      = 0;
        s.metrics.frame_width = DPI(1);
        s.metrics.dashed      = false;
    }
    return s;
}

UiPanel::UiPanel()
    : style_(StyleDefault())
{
    BackPaint();
}

UiPanel& UiPanel::SetStyle(const Style& s)
{
    style_ = Style(s);
    OnStyleChanged();
    return *this;
}

void UiPanel::OnStyleChanged()
{
    if(style_.transparent)
        Transparent();
    else
        BackPaint();

    RefreshLayout();
    Refresh();
}

Size UiPanel::GetMinSize() const
{
    Size base_content(DPI(40), DPI(40));
    Size natural_outer = UiStyledOuterSizeFromContent(base_content, style_.metrics, style_.skin);

    int w = natural_outer.cx;
    int h = natural_outer.cy;

    if(user_min_size_.cx > 0)
        w = max(w, user_min_size_.cx);
    if(user_min_size_.cy > 0)
        h = max(h, user_min_size_.cy);

    return Size(w, h);
}

UiPanel& UiPanel::SetSizeMin(Size sz)
{
    user_min_size_ = sz;
    RefreshLayout();
    Refresh();
    return *this;
}

void UiPanel::Paint(Draw& w)
{
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    bool        enabled   = IsEnabled();
    bool        has_focus = HasFocus();
    StyledState st        = enabled ? ST_NORMAL : ST_DISABLED;

    StyledPalette& pal  = style_.palette;
    StyledMetrics& met  = style_.metrics;
    StyledSkin&    skin = style_.skin;

    bool bg_handled = false;
    bool fg_handled = false;

    if(WhenPaintBackground) {
        WhenPaintBackground(w, outer, pal, met, skin, st, has_focus);
        bg_handled = true;
    }

    if(WhenPaintForeground) {
        WhenPaintForeground(w, outer, pal, met, skin, st, has_focus);
        fg_handled = true;
    }

    // See UiDraw.h: UiPaintStyledSurface contract (hook order + fallback).
    UiPaintStyledSurface(w, outer, pal, met, skin, st, has_focus,
                         bg_handled, fg_handled, style_.show_focus);
}

} // namespace Upp
