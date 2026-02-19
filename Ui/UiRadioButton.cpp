#include <Ui/UiRadioButton.h>

namespace Upp {

static StyledState UiRadioToStyledState_(bool enabled, bool pressed, bool hover)
{
    if(!enabled) return ST_DISABLED;
    if(pressed)  return ST_PRESSED;
    if(hover)    return ST_HOT;
    return ST_NORMAL;
}

const UiRadioButton::Style& UiRadioButton::StyleDefault() { return StyleClassic(); }

const UiRadioButton::Style& UiRadioButton::StyleClassic()
{
    static Style s;
    ONCELOCK {
        for(int st = 0; st < 4; st++) {
            s.palette.face[st] = UiFill::Solid(Null);
            s.palette.frame[st] = Null;
            s.palette.ink[st] = SColorText();

            s.indicator_palette.face[st] = UiFill::Solid(Blend(SColorPaper(), SColorFace(), 240));
            s.indicator_palette.frame[st] = SColorShadow();
            s.indicator_palette.ink[st] = SColorHighlight();
        }
        s.indicator_metrics.radius = DPI(999);
        s.indicator_metrics.frame_enabled = true;
        s.indicator_metrics.frame_width = DPI(1);
        s.visual = UIRADIOVIS_CLASSIC;
    }
    return s;
}

const UiRadioButton::Style& UiRadioButton::StylePills()
{
    static Style s;
    ONCELOCK {
        s = StyleClassic();
        s.visual = UIRADIOVIS_PILLS;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(999);
        for(int st = 0; st < 4; st++) {
            s.palette.face[st] = UiFill::Solid(Blend(SColorFace(), SColorPaper(), st == ST_PRESSED ? 170 : 210));
            s.palette.frame[st] = Blend(SColorShadow(), SColorPaper(), 120);
            s.palette.ink[st] = SColorText();
        }
    }
    return s;
}

const UiRadioButton::Style& UiRadioButton::StyleList()
{
    static Style s;
    ONCELOCK {
        s = StyleClassic();
        s.visual = UIRADIOVIS_LIST;
        s.indicator_metrics.frame_enabled = false;
        s.indicator_metrics.face_enabled = false;
        s.metrics.content_padding = Rect(DPI(4), DPI(0), DPI(0), DPI(0));
    }
    return s;
}

UiRadioButton::UiRadioButton()
{
    SetStyle(StyleDefault());
    BackPaint();
    WantFocus();
}

UiRadioButton& UiRadioButton::SetStyle(const Style& s)
{
    style_ = s;
    text_size_cache_ = GetTextSize(text_, style_.font);
    text_size_dirty_ = false;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

void UiRadioButton::OnStyleChanged()
{
    text_size_cache_ = GetTextSize(text_, style_.font);
    text_size_dirty_ = false;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
}

UiRadioButton& UiRadioButton::SetText(const String& s)
{
    text_ = s;
    text_size_cache_ = GetTextSize(text_, style_.font);
    text_size_dirty_ = false;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

Size UiRadioButton::GetTextSizeCached() const
{
    if(text_size_dirty_) {
        text_size_cache_ = GetTextSize(text_, style_.font);
        text_size_dirty_ = false;
    }
    return text_size_cache_;
}

UiRadioButton& UiRadioButton::SetVisual(UiRadioVisual vis)
{
    style_.visual = vis;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiRadioButton& UiRadioButton::SetIndicatorSide(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT)
        side = UiAlign::LEFT;
    style_.indicator_side = side;
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiRadioButton& UiRadioButton::SetIndicatorRadius(int px)
{
    style_.indicator_metrics.radius = max(0, px);
    Refresh();
    return *this;
}

UiRadioButton& UiRadioButton::SetIndicatorRoundness(int percent)
{
    percent = clamp(percent, 0, 100);
    int side = max(DPI(10), style_.indicator_size);
    int r = (side * percent) / 2 / 100;
    return SetIndicatorRadius(r);
}

Rect UiRadioButton::GetIndicatorRect(const Rect& r) const
{
    int side = max(DPI(10), style_.indicator_size);
    int y = r.top + (r.GetHeight() - side) / 2;
    int x = style_.indicator_side == UiAlign::RIGHT ? (r.right - side) : r.left;
    return RectC(x, y, side, side);
}

Rect UiRadioButton::GetTextRect(const Rect& r, const Rect& ind) const
{
    Rect t = r;
    if(style_.visual == UIRADIOVIS_PILLS)
        return t;
    if(style_.indicator_side == UiAlign::RIGHT)
        t.right = max(t.left, ind.left - style_.indicator_gap);
    else
        t.left = min(t.right, ind.right + style_.indicator_gap);
    return t;
}

void UiRadioButton::Paint(Draw& w)
{
    Rect r = GetSize();
    if(r.IsEmpty())
        return;

    StyledState st = UiRadioToStyledState_(IsEnabled() && IsShowEnabled(), pressed_, hover_);
    bool has_focus = HasFocus();

    if(WhenPaintBackground)
        WhenPaintBackground(w, r, style_.palette, style_.metrics, style_.skin, st, has_focus);
    else if(style_.visual == UIRADIOVIS_PILLS)
        UiPaintStyledBackground(w, r, style_.palette, style_.metrics, style_.skin, checked_ ? ST_PRESSED : st, has_focus);

    Rect ind = layout_cache_.support;
    Rect text_r = layout_cache_.main;

    auto PaintSelectionMark = [&](const Rect& outer_ind) {
        Rect mark_area = outer_ind;
        int inset = max(DPI(2), style_.indicator_metrics.frame_width + DPI(1));
        if(style_.visual == UIRADIOVIS_LIST)
            inset = max(inset, DPI(3));
        mark_area = mark_area.Deflated(inset, inset);
        if(mark_area.IsEmpty())
            return;

        int mark_side = min(mark_area.GetWidth(), mark_area.GetHeight());
        int dot = max(DPI(8), (mark_side * 76) / 100);
        dot = min(dot, mark_side);

        double cx = mark_area.left + mark_area.GetWidth() * 0.5;
        double cy = mark_area.top + mark_area.GetHeight() * 0.5;
        double x = cx - dot * 0.5;
        double y = cy - dot * 0.5;

        Color c = style_.indicator_palette.ink[st];
        if(IsNull(c)) c = SColorHighlight();
        RGBA rc = c;

        int outer_side = max(1, min(outer_ind.GetWidth(), outer_ind.GetHeight()));
        int outer_half = max(1, outer_side / 2);
        int outer_radius = max(0, style_.indicator_metrics.radius);
        int pct = min(100, (outer_radius * 100) / outer_half);

        ImageBuffer ib(max(1, dot + 4), max(1, dot + 4));
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());

        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Clear(RGBAZero());
        if(pct >= 95) {
            p.Circle(ib.GetWidth() * 0.5, ib.GetHeight() * 0.5, dot * 0.5);
        }
        else {
            double rr = max(0.0, dot * pct / 200.0);
            p.RoundedRectangle((ib.GetWidth() - dot) * 0.5,
                               (ib.GetHeight() - dot) * 0.5,
                               dot,
                               dot,
                               rr);
        }
        p.Fill(rc);

        int dx = fround(x) - 2;
        int dy = fround(y) - 2;
        w.DrawImage(dx, dy, Image(ib));
    };

    if(style_.visual == UIRADIOVIS_LIST) {
        if(checked_) {
            PaintSelectionMark(ind);
        }
    }
    else {
        if(style_.indicator_skin.enabled) {
            UiDraw9Slice(w, ind, style_.indicator_skin.base, style_.indicator_skin.slice);
            StyledMetrics mm = style_.indicator_metrics;
            mm.face_enabled = false;
            UiPaintFaceFrameDash(w, ind, style_.indicator_palette, mm, st);
        }
        else {
            UiPaintFaceFrameDash(w, ind, style_.indicator_palette, style_.indicator_metrics, st);
        }
        if(checked_)
            PaintSelectionMark(ind);
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

void UiRadioButton::RebuildLayoutCache(const Rect& content) const
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
                                          UiAlign::LEFT,
                                          UiAlign::CENTER,
                                          style_.indicator_side,
                                          Rect(0, 0, 0, 0),
                                          main_margin,
                                          max(DPI(10), style_.indicator_size));
    layout_content_cache_ = content;
    layout_dirty_ = false;
}

void UiRadioButton::Layout()
{
    Rect content = UiStyledInnerRect(GetSize(), style_.metrics, style_.skin);
    RebuildLayoutCache(content);
}

Size UiRadioButton::GetMinSize() const
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

void UiRadioButton::SetMinSize(Size sz)
{
    user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    layout_dirty_ = true;
    RefreshLayout();
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
    pressed_ = true;
    SetFocus();
    SetChecked(true);
    pressed_ = false;
}

bool UiRadioButton::Key(dword key, int)
{
    if(key == K_SPACE || key == K_ENTER) {
        SetChecked(true);
        return true;
    }
    return false;
}

void UiRadioButton::GotFocus() { has_focus_ = true; Refresh(); }
void UiRadioButton::LostFocus() { has_focus_ = false; Refresh(); }
void UiRadioButton::MouseEnter(Point, dword) { hover_ = true; Refresh(); }
void UiRadioButton::MouseLeave() { hover_ = false; Refresh(); }

}
