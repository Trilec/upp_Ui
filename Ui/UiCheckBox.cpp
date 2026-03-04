#include <Ui/UiCheckBox.h>
#include <Ui/UiIcons.h>

namespace Upp {

static StyledState UiCheckToStyledState_(bool enabled, bool pressed, bool hover)
{
    if(!enabled) return ST_DISABLED;
    if(pressed)  return ST_PRESSED;
    if(hover)    return ST_HOT;
    return ST_NORMAL;
}

const UiCheckBox::Style& UiCheckBox::StyleDefault() { return StyleStandard(); }

const UiCheckBox::Style& UiCheckBox::StyleStandard() { return StyleClassic(); }

const UiCheckBox::Style& UiCheckBox::StyleMinimal()
{
    static Style s;
    ONCELOCK {
        s = StyleClassic();
        s.metrics.frame_enabled = false;
        s.metrics.face_enabled = false;
        s.indicator_palette.face[ST_NORMAL] = UiFill::Solid(Null);
        s.indicator_palette.face[ST_HOT] = UiFill::Solid(Null);
        s.indicator_palette.face[ST_PRESSED] = UiFill::Solid(Null);
        s.indicator_palette.frame[ST_NORMAL] = Blend(SColorShadow(), SColorPaper(), 130);
        s.indicator_palette.frame[ST_HOT] = DkColor(s.indicator_palette.frame[ST_NORMAL], 10);
        s.indicator_palette.frame[ST_PRESSED] = DkColor(s.indicator_palette.frame[ST_NORMAL], 20);
    }
    return s;
}

const UiCheckBox::Style& UiCheckBox::StyleSoft()
{
    static Style s;
    ONCELOCK {
        s = StyleClassic();
        for(int st = 0; st < 4; st++) {
            s.palette.face[st] = UiFill::Solid(Blend(SColorFace(), SColorPaper(), 220));
            s.palette.frame[st] = Blend(SColorShadow(), SColorPaper(), 140);
        }
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(6);
        s.metrics.content_padding = Rect(DPI(6), DPI(3), DPI(6), DPI(3));
    }
    return s;
}

const UiCheckBox::Style& UiCheckBox::StyleStrong()
{
    static Style s;
    ONCELOCK {
        s = StyleClassic();
        Color base = SColorHighlight();
        Color ink = SColorHighlightText();
        for(int st = 0; st < 4; st++) {
            s.palette.face[st] = UiFill::Solid(base);
            s.palette.frame[st] = DkColor(base, 30);
            s.palette.ink[st] = ink;
            s.indicator_palette.face[st] = UiFill::Solid(Blend(White(), base, 205));
            s.indicator_palette.frame[st] = DkColor(base, 15);
            s.indicator_palette.ink[st] = DkColor(base, 35);
        }
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(6);
        s.metrics.content_padding = Rect(DPI(6), DPI(3), DPI(6), DPI(3));
    }
    return s;
}

const UiCheckBox::Style& UiCheckBox::StyleClassic()
{
    static Style s;
    ONCELOCK {
        for(int st = 0; st < 4; st++) {
            s.palette.face[st] = UiFill::Solid(Null);
            s.palette.frame[st] = Null;
            s.palette.ink[st] = Color(78, 102, 138);

            s.indicator_palette.face[st] = UiFill::Solid(Color(245, 248, 252));
            s.indicator_palette.frame[st] = Color(106, 138, 186);
            s.indicator_palette.ink[st] = Color(46, 112, 235);
        }
        s.palette.ink[ST_DISABLED] = SColorDisabled();
        s.indicator_palette.face[ST_DISABLED] = UiFill::Solid(Color(234, 238, 244));
        s.indicator_palette.frame[ST_DISABLED] = Color(193, 204, 222);
        s.indicator_palette.ink[ST_DISABLED] = Color(165, 178, 198);

        s.indicator_palette.face[ST_HOT] = UiFill::Solid(Color(250, 252, 255));
        s.indicator_palette.face[ST_PRESSED] = UiFill::Solid(Color(233, 241, 252));

        s.font = SansSerifZ(13);
        s.indicator_metrics.radius = DPI(3);
        s.indicator_metrics.frame_enabled = true;
        s.indicator_metrics.frame_width = DPI(1);
        s.visual = UICHECKVIS_CLASSIC;
    }
    return s;
}

const UiCheckBox::Style& UiCheckBox::StyleSwitch()
{
    static Style s;
    ONCELOCK {
        s = StyleClassic();
        s.visual = UICHECKVIS_SWITCH;
        s.indicator_size = DPI(18);
        s.indicator_metrics.radius = DPI(999);
        s.indicator_metrics.frame_width = DPI(1);
        for(int st = 0; st < 4; st++) {
            s.indicator_palette.face[st] = UiFill::Solid(Color(224, 231, 242));
            s.indicator_palette.frame[st] = Color(181, 194, 216);
        }
        s.indicator_palette.face[ST_HOT] = UiFill::Solid(Color(216, 227, 244));
        s.indicator_palette.face[ST_PRESSED] = UiFill::Solid(Color(206, 220, 242));
    }
    return s;
}

const UiCheckBox::Style& UiCheckBox::StyleChip()
{
    static Style s;
    ONCELOCK {
        s = StyleClassic();
        s.visual = UICHECKVIS_CHIP;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(999);
        for(int st = 0; st < 4; st++) {
            s.palette.face[st] = UiFill::Solid(Blend(SColorFace(), SColorPaper(), st == ST_PRESSED ? 170 : 210));
            s.palette.frame[st] = Blend(SColorShadow(), SColorPaper(), 120);
            s.palette.ink[st] = SColorText();
        }
        s.indicator_size = DPI(14);
    }
    return s;
}

const UiCheckBox::Style& UiCheckBox::StyleListCheck()
{
    static Style s;
    ONCELOCK {
        s = StyleClassic();
        s.visual = UICHECKVIS_LIST;
        s.indicator_size = DPI(14);
        s.indicator_metrics.frame_enabled = false;
        s.indicator_metrics.face_enabled = false;
    }
    return s;
}

UiCheckBox::UiCheckBox()
{
    SetStyle(StyleDefault());
    BackPaint();
    WantFocus();
}

UiCheckBox& UiCheckBox::SetStyle(const Style& s)
{
    style_ = s;
    text_size_cache_ = GetTextSize(text_, style_.font);
    text_size_dirty_ = false;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

void UiCheckBox::OnStyleChanged()
{
    text_size_cache_ = GetTextSize(text_, style_.font);
    text_size_dirty_ = false;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
}

UiCheckBox& UiCheckBox::SetText(const String& s)
{
    text_ = s;
    text_size_cache_ = GetTextSize(text_, style_.font);
    text_size_dirty_ = false;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

Size UiCheckBox::GetTextSizeCached() const
{
    if(text_size_dirty_) {
        text_size_cache_ = GetTextSize(text_, style_.font);
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
    if(style_.visual != vis) {
        style_.visual = vis;
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiCheckBox& UiCheckBox::SetIndicatorSide(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT)
        side = UiAlign::LEFT;
    style_.indicator_side = side;
    RefreshLayout();
    Refresh();
    return *this;
}

UiCheckBox& UiCheckBox::SetIndicatorRadius(int px)
{
    style_.indicator_metrics.radius = max(0, px);
    Refresh();
    return *this;
}

UiCheckBox& UiCheckBox::SetIndicatorRoundness(int percent)
{
    percent = clamp(percent, 0, 100);
    int side = max(DPI(10), style_.indicator_size);
    int r = (side * percent) / 2 / 100;
    return SetIndicatorRadius(r);
}

Rect UiCheckBox::GetIndicatorRect(const Rect& r) const
{
    int side = max(DPI(10), style_.indicator_size);
    int y = r.top + (r.GetHeight() - side) / 2;
    int x = style_.indicator_side == UiAlign::RIGHT
            ? (r.right - side)
            : r.left;
    return RectC(x, y, side, side);
}

Rect UiCheckBox::GetTextRect(const Rect& r, const Rect& ind) const
{
    Rect t = r;
    if(style_.visual == UICHECKVIS_CHIP)
        return t;
    if(style_.indicator_side == UiAlign::RIGHT)
        t.right = max(t.left, ind.left - style_.indicator_gap);
    else
        t.left = min(t.right, ind.right + style_.indicator_gap);
    return t;
}

void UiCheckBox::Paint(Draw& w)
{
    Rect r = GetSize();
    if(r.IsEmpty())
        return;

    StyledState st = UiCheckToStyledState_(IsEnabled() && IsShowEnabled(), pressed_, hover_);
    bool has_focus = HasFocus();

    if(WhenPaintBackground)
        WhenPaintBackground(w, r, style_.palette, style_.metrics, style_.skin, st, has_focus);
    else if(style_.visual == UICHECKVIS_CHIP)
        UiPaintStyledBackground(w, r, style_.palette, style_.metrics, style_.skin, st, has_focus);

    Rect ind = layout_cache_.support;
    Rect text_r = layout_cache_.main;

    auto DrawCheckSmall = [&](const Rect& rc) {
        Image mk = ICON_DESIGN_CHECK_SMALL_48();
        if(IsNull(mk))
            return false;

        Rect r = rc;
        r.Deflate(DPI(3), DPI(3));
        if(r.GetWidth() <= 0 || r.GetHeight() <= 0)
            return false;

        Size isz = mk.GetSize();
        if(isz.cx <= 0 || isz.cy <= 0)
            return false;

        double sx = (double)r.GetWidth() / isz.cx;
        double sy = (double)r.GetHeight() / isz.cy;
        double s = min(sx, sy);
        int dw = max(1, (int)floor(isz.cx * s + 0.5));
        int dh = max(1, (int)floor(isz.cy * s + 0.5));
        Image scaled = CachedRescale(mk, Size(dw, dh));
        int x = r.left + (r.GetWidth() - dw) / 2;
        int y = r.top + (r.GetHeight() - dh) / 2;
        w.DrawImage(x, y, scaled);
        return true;
    };

    if(style_.visual == UICHECKVIS_SWITCH) {
        if(style_.indicator_skin.enabled) {
            UiDraw9Slice(w, ind, style_.indicator_skin.base, style_.indicator_skin.slice);
            StyledMetrics mm = style_.indicator_metrics;
            mm.face_enabled = false;
            UiPaintFaceFrameDash(w, ind, style_.indicator_palette, mm, st);
        }
        else {
            UiPaintFaceFrameDash(w, ind, style_.indicator_palette, style_.indicator_metrics, st);
        }
        int thumb = max(DPI(8), ind.GetHeight() - DPI(4));
        int x = ind.left + DPI(2);
        if(state_ == UICHECK_CHECKED)
            x = ind.right - DPI(2) - thumb;
        else if(state_ == UICHECK_INDETERMINATE)
            x = ind.left + (ind.GetWidth() - thumb) / 2;
        Rect tr = RectC(x, ind.top + (ind.GetHeight() - thumb) / 2, thumb, thumb);
        StyledPalette tp = style_.indicator_palette;
        for(int i = 0; i < 4; i++)
            tp.face[i] = UiFill::Solid(i == ST_DISABLED ? SColorDisabled() : SColorPaper());
        StyledMetrics tm = style_.indicator_metrics;
        tm.frame_enabled = false;
        UiPaintFaceFrameDash(w, tr, tp, tm, st);
    }
    else if(style_.visual == UICHECKVIS_LIST) {
        if(state_ != UICHECK_UNCHECKED) {
            Color mk = style_.indicator_palette.ink[st];
            if(IsNull(mk)) mk = SColorHighlight();
            int t = max(1, style_.mark_thickness);
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
        UiPaintFaceFrameDash(w, ind, style_.indicator_palette, style_.indicator_metrics, st);
        if(state_ != UICHECK_UNCHECKED) {
            Color mk = style_.indicator_palette.ink[st];
            if(IsNull(mk)) mk = SColorHighlight();
            int t = max(1, style_.mark_thickness);
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

    Color ink = style_.palette.ink[st];
    if(IsNull(ink)) ink = SColorText();
    Font f = style_.font;
    int ty = text_r.top + (text_r.GetHeight() - f.GetHeight()) / 2;
    DrawSmartText(w, text_r.left, ty, max(0, text_r.GetWidth()), text_, f, ink, 0);

    if(WhenPaintForeground)
        WhenPaintForeground(w, r, style_.palette, style_.metrics, style_.skin, st, has_focus);
    else
        UiPaintStyledForeground(w, r, style_.palette, style_.metrics, style_.skin, st, has_focus);
}

void UiCheckBox::RebuildLayoutCache(const Rect& content) const
{
    if(!layout_dirty_ && layout_content_cache_ == content)
        return;

    Size support_natural(style_.indicator_size, style_.indicator_size);
    Size main_natural = GetTextSizeCached();
    Rect main_margin = style_.indicator_side == UiAlign::RIGHT
                       ? Rect(0, 0, style_.indicator_gap, 0)
                       : Rect(style_.indicator_gap, 0, 0, 0);

    layout_cache_ = UiComputeBlocksLayout(content,
                                          support_natural,
                                          main_natural,
                                          style_.align_h,
                                          style_.align_v,
                                          style_.indicator_side,
                                          Rect(0, 0, 0, 0),
                                          main_margin,
                                          max(DPI(10), style_.indicator_size));
    layout_content_cache_ = content;
    layout_dirty_ = false;
}

void UiCheckBox::Layout()
{
    Rect content = UiStyledInnerRect(GetSize(), style_.metrics, style_.skin);
    RebuildLayoutCache(content);
}

Size UiCheckBox::GetMinSize() const
{
    if(user_min_size_.cx > 0 && user_min_size_.cy > 0)
        return user_min_size_;

    Size support_natural(style_.indicator_size, style_.indicator_size);
    Size main_natural = GetTextSizeCached();
    Rect main_margin = style_.indicator_side == UiAlign::RIGHT
                       ? Rect(0, 0, style_.indicator_gap, 0)
                       : Rect(style_.indicator_gap, 0, 0, 0);

    Size content = UiMeasureBlocksContent(support_natural,
                                         main_natural,
                                         Rect(0, 0, 0, 0),
                                         main_margin,
                                         style_.indicator_side,
                                         true,
                                         !text_.IsEmpty(),
                                         0,
                                         max(GetTextSize("A", style_.font).cy, style_.indicator_size),
                                         max(DPI(10), style_.indicator_size));
    return UiStyledOuterSizeFromContent(content, style_.metrics, style_.skin);
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
        if(tri_state_) SetStateInternal(UICHECK_INDETERMINATE, false);
        else SetStateInternal(UICHECK_UNCHECKED, false);
        return;
    }
    SetStateInternal((bool)v ? UICHECK_CHECKED : UICHECK_UNCHECKED, false);
}

Value UiCheckBox::GetData() const
{
    if(tri_state_ && state_ == UICHECK_INDETERMINATE)
        return Value();
    return state_ == UICHECK_CHECKED;
}

}
