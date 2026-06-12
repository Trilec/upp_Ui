#include <Ui/UiPanel.h>
#include <Ui/UiMeasure.h>
#include <Ui/UiTheme.h>

namespace Upp {

static Size UiPanelOuterSizeFromChildExtent(Size content, const StyledMetrics& m, const StyledSkin& skin)
{
    Rect ci = UiNonNegativeThickness(skin.content_inset);
    Rect sh = UiStyledShadowMargins(m);
    int fw = UiResolvedFrameWidth(m, skin);
    return Size(max(0, content.cx) + ci.left + ci.right + sh.left + sh.right + 2 * fw,
                max(0, content.cy) + ci.top + ci.bottom + sh.top + sh.bottom + 2 * fw);
}

const UiPanel::Style& UiPanel::StyleDefault()
{
    static Style s;
    ONCELOCK {
        const Color face = Color(255, 255, 255);
        const Color frame = Color(236, 239, 243);
        const Color ink = Color(17, 24, 39);

        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i] = ink;
            s.palette.icon[i] = Null;
        }

        s.palette.face[ST_HOT] = UiFill::Solid(Color(248, 250, 252));
        s.palette.face[ST_PRESSED] = UiFill::Solid(Color(241, 245, 249));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.palette.frame[ST_DISABLED] = Color(241, 245, 249);
        s.palette.ink[ST_DISABLED] = Color(148, 163, 184);

        s.metrics.text_font = StdFont();
        s.metrics.use_text_font = false;
        s.metrics.content_margin = Rect(DPI(12), DPI(12), DPI(12), DPI(12));
        s.metrics.radius = DPI(8);
        s.metrics.frame_width = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled = true;
        s.metrics.dashed = false;
        s.metrics.high_contrast = false;
        s.metrics.shadow = StyledShadow();
        s.metrics.highlight = StyledHighlight();

        s.skin = StyledSkin();
        s.transparent = false;
        s.metrics.focus_enabled = false;
    }
    return s;
}

UiPanel::UiPanel()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
{
    BackPaint();
    SyncThemeStyle();
}

void UiPanel::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiPanel::Style& UiPanel::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiPanel::SyncThemeStyle()
{
    if(has_custom_style_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolvePanel();
    theme_revision_ = revision;
}

UiPanel& UiPanel::SetCustomStyle(const Style& s)
{
    style_ = Style(s);
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiPanel& UiPanel::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;

    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

const UiPanel::Style& UiPanel::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;

    const_cast<UiPanel*>(this)->SyncThemeStyle();
    return themed_style_;
}

void UiPanel::OnStyleChanged()
{
    const Style& style = GetEffectiveStyle();

    if(style.transparent)
        Transparent();
    else
        BackPaint();

    RefreshLayout();
    Refresh();
}

Size UiPanel::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    bool any_child = false;
    Size child_content(0, 0);
    for(Ctrl* q = GetFirstChild(); q; q = q->GetNext()) {
        if(!q->IsShown())
            continue;
        UiLayoutMeasureResult measure = UiMeasureLayout(*q);
        Size sz = measure.min;
        Rect r = q->GetRect();
        int right = r.IsEmpty() ? sz.cx : max(0, r.left) + sz.cx;
        int bottom = r.IsEmpty() ? sz.cy : max(0, r.top) + sz.cy;
        child_content.cx = max(child_content.cx, max(0, right));
        child_content.cy = max(child_content.cy, max(0, bottom));
        any_child = true;
    }
    Size natural_outer = any_child ? UiPanelOuterSizeFromChildExtent(child_content, style.metrics, style.skin)
                                   : Size(0, 0);

    int w = natural_outer.cx;
    int h = natural_outer.cy;

    if(user_min_size_.cx > 0)
        w = max(w, user_min_size_.cx);
    if(user_min_size_.cy > 0)
        h = max(h, user_min_size_.cy);

    return Size(w, h);
}

Size UiPanel::GetContentSize() const
{
    Rect bounds(0, 0, 0, 0);
    bool any = false;
    for(Ctrl* q = GetFirstChild(); q; q = q->GetNext()) {
        if(!q->IsShown())
            continue;
        Rect r = q->GetRect();
        if(r.IsEmpty()) {
            UiLayoutMeasureResult measure = UiMeasureLayout(*q);
            Size sz = measure.min;
            r = RectC(0, 0, sz.cx, sz.cy);
        }
        if(any)
            bounds |= r;
        else {
            bounds = r;
            any = true;
        }
    }

    Size content = any ? Size(max(0, bounds.right), max(0, bounds.bottom)) : Size(0, 0);
    const Style& style = GetEffectiveStyle();
    Size natural = any ? UiPanelOuterSizeFromChildExtent(content, style.metrics, style.skin) : Size(0, 0);
    if(user_min_size_.cx > 0)
        natural.cx = max(natural.cx, user_min_size_.cx);
    if(user_min_size_.cy > 0)
        natural.cy = max(natural.cy, user_min_size_.cy);
    return natural;
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
    const Style& style = GetEffectiveStyle();
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    bool enabled = IsEnabled();
    bool has_focus = HasFocus();
    StyledState st = enabled ? ST_NORMAL : ST_DISABLED;

    const StyledPalette& pal = style.palette;
    const StyledMetrics& met = style.metrics;
    const StyledSkin& skin = style.skin;

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

    UiPaintStyledSurface(w, outer, pal, met, skin, st, has_focus, bg_handled, fg_handled);
}

} // namespace Upp
