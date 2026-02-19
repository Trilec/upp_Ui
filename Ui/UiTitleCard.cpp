#include <Ui/UiTitleCard.h>

namespace Upp {

const UiTitleCard::Style& UiTitleCard::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color face  = Blend(SColorFace(), White(), 18);
        Color frame = Blend(SColorShadow(), Black(), 20);
        Color ink   = SColorText();

        for(int i = 0; i < 4; i++) {
            s.palette.face[i]  = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i]   = ink;
        }
        s.palette.face[ST_HOT]      = UiFill::Solid(LtColor(face, 4));
        s.palette.face[ST_PRESSED]  = UiFill::Solid(DkColor(face, 3));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorDisabled(), 50));

        s.metrics.radius        = DPI(10);
        s.metrics.frame_width   = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled  = true;
        s.metrics.content_padding = Rect(DPI(10), DPI(9), DPI(10), DPI(9));
    }
    return s;
}

const UiTitleCard::Style& UiTitleCard::StyleSquare()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();
        s.metrics.radius = 0;
    }
    return s;
}

UiTitleCard::UiTitleCard()
    : style_(StyleDefault())
{
    BackPaint();
    NoWantFocus();
}

UiTitleCard& UiTitleCard::SetStyle(const Style& s)
{
    style_ = s;
    OnStyleChanged();
    return *this;
}

void UiTitleCard::OnStyleChanged()
{
    if(style_.transparent)
        Transparent();
    else
        BackPaint();
    InvalidateTextCache();
    RefreshLayout();
    Refresh();
}

UiTitleCard& UiTitleCard::SetTitle(const String& s)
{
    title_ = s;
    InvalidateTextCache();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetSubTitle(const String& s)
{
    subtitle_ = s;
    InvalidateTextCache();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetCopyText(const String& s)
{
    copy_ = s;
    InvalidateTextCache();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetMedia(const Image& img, Size preferred)
{
    media_ = img;
    media_pref_ = preferred;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::ClearMedia()
{
    media_ = Image();
    media_pref_ = Size(0, 0);
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetMediaSide(UiAlign side)
{
    if(side == UiAlign::LEFT || side == UiAlign::RIGHT || side == UiAlign::TOP || side == UiAlign::BOTTOM)
        style_.media_side = side;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetMediaAlign(UiAlign h, UiAlign v)
{
    style_.media_align_h = h;
    style_.media_align_v = v;
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetMediaReserve(int px)
{
    style_.media_reserve = max(DPI(16), px);
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetMediaSharePercent(int pct)
{
    style_.media_share_percent = clamp(pct, 0, 90);
    if(style_.media_share_percent > 0 && style_.media_share_percent < 10)
        style_.media_share_percent = 10;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetRuleStyle(UiLineStyle st)
{
    style_.rule_style = st;
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetRuleExtent(UiSpan ex)
{
    style_.rule_extent = ex;
    InvalidateTextCache();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetBottomLine(UiSpan ex, int thickness, UiLineStyle style, Color c)
{
    style_.bottom_line_extent = ex;
    style_.bottom_line_thickness = max(1, thickness);
    style_.bottom_line_style = style;
    style_.bottom_line_color = c;
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::ShowRule(bool on)
{
    style_.show_rule = on;
    InvalidateTextCache();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::ShowBottomLine(bool on)
{
    style_.show_bottom_line = on;
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::EnableHover(bool on)
{
    style_.hover_enabled = on;
    if(!on)
        hot_ = down_ = false;
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::EnableFocusRing(bool on)
{
    style_.show_focus = on;
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetSelectable(bool on)
{
    if(on)
        WantFocus();
    else
        NoWantFocus();
    Refresh();
    return *this;
}

Size UiTitleCard::GetTextBlockSize() const
{
    return text_block_size_;
}

void UiTitleCard::InvalidateTextCache()
{
    text_metrics_dirty_ = true;
    RebuildTextCache();
}

void UiTitleCard::RebuildTextCache()
{
    if(!text_metrics_dirty_)
        return;

    title_size_ = title_.IsEmpty() ? Size(0, 0) : GetTextSize(title_, style_.title_font);
    subtitle_size_ = subtitle_.IsEmpty() ? Size(0, 0) : GetTextSize(subtitle_, style_.subtitle_font);
    copy_size_ = copy_.IsEmpty() ? Size(0, 0) : GetTextSize(copy_, style_.copy_font);

    int w = 0;
    int h = 0;

    if(!title_.IsEmpty()) {
        w = max(w, title_size_.cx);
        h += title_size_.cy;
    }

    if(style_.show_rule && style_.rule_extent != NONE && !title_.IsEmpty())
        h += style_.rule_gap_above + style_.rule_thickness + style_.rule_gap_below;

    if(!subtitle_.IsEmpty()) {
        w = max(w, subtitle_size_.cx);
        if(h > 0)
            h += style_.title_subtitle_gap;
        h += subtitle_size_.cy;
    }

    if(!copy_.IsEmpty()) {
        w = max(w, copy_size_.cx);
        if(h > 0)
            h += style_.subtitle_copy_gap;
        h += copy_size_.cy;
    }

    text_block_size_ = Size(w, h);
    text_metrics_dirty_ = false;
}

int UiTitleCard::GetRuleWidth(int title_cx, int text_cx) const
{
    switch(style_.rule_extent) {
    case NONE:
        return 0;
    case SMALL:
        return min(text_cx, DPI(40));
    case MEDIUM:
        return min(text_cx, max(1, title_cx));
    case LARGE:
    default:
        return max(0, text_cx);
    }
}

Rect UiTitleCard::GetMediaRect(const Rect& content) const
{
    if(IsNull(media_))
        return RectC(0, 0, 0, 0);

    Rect r = content;
    int reserve = max(style_.media_min, style_.media_reserve);
    if(style_.media_share_percent > 0) {
        bool h = style_.media_side == UiAlign::LEFT || style_.media_side == UiAlign::RIGHT;
        int axis = h ? content.GetWidth() : content.GetHeight();
        reserve = max(style_.media_min, (axis * style_.media_share_percent) / 100);
    }

    switch(style_.media_side) {
    case UiAlign::LEFT:
        r.right = min(content.right, content.left + reserve);
        break;
    case UiAlign::RIGHT:
        r.left = max(content.left, content.right - reserve);
        break;
    case UiAlign::TOP:
        r.bottom = min(content.bottom, content.top + reserve);
        break;
    case UiAlign::BOTTOM:
        r.top = max(content.top, content.bottom - reserve);
        break;
    default:
        break;
    }
    return r;
}

Size UiTitleCard::GetMinSize() const
{
    Size text = GetTextBlockSize();
    Size media = IsNull(media_) ? Size(0, 0)
                                : (media_pref_.cx > 0 && media_pref_.cy > 0 ? media_pref_ : media_.GetSize());

    bool media_h = !IsNull(media_) && (style_.media_side == UiAlign::LEFT || style_.media_side == UiAlign::RIGHT);

    int cw = text.cx;
    int ch = text.cy;
    if(!IsNull(media_)) {
        if(media_h) {
            cw += max(style_.media_min, style_.media_reserve) + style_.media_gap;
            ch = max(ch, max(style_.media_min, media.cy));
        } else {
            cw = max(cw, max(style_.media_min, media.cx));
            ch += max(style_.media_min, style_.media_reserve) + style_.media_gap;
        }
    }

    Size out = UiStyledOuterSizeFromContent(Size(cw, ch), style_.metrics, style_.skin);
    if(user_min_size_.cx > 0) out.cx = max(out.cx, user_min_size_.cx);
    if(user_min_size_.cy > 0) out.cy = max(out.cy, user_min_size_.cy);
    return out;
}

void UiTitleCard::DrawRule(Draw& w, int x, int y, int cx, Color c, int thickness, UiLineStyle style) const
{
    int th = max(1, thickness);
    if(style == SOLID) {
        w.DrawRect(x, y, cx, th, c);
        return;
    }

    int dot = (style == DOTTED) ? th : max(DPI(6), th * 3);
    int gap = (style == DOTTED) ? max(DPI(3), th * 2) : max(DPI(4), th * 2);
    int cur = x;
    while(cur < x + cx) {
        int seg = min(dot, x + cx - cur);
        w.DrawRect(cur, y, seg, th, c);
        cur += dot + gap;
    }
}

void UiTitleCard::Paint(Draw& w)
{
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    StyledState st = ST_DISABLED;
    if(IsEnabled()) {
        if(style_.hover_enabled && down_)
            st = ST_PRESSED;
        else if(style_.hover_enabled && hot_)
            st = ST_HOT;
        else
            st = ST_NORMAL;
    }
    bool has_focus = HasFocus();

    if(WhenPaintBackground)
        WhenPaintBackground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);
    else
        UiPaintStyledBackground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);

    Rect content = UiStyledInnerRect(outer, style_.metrics, style_.skin);
    if(content.IsEmpty()) {
        if(WhenPaintForeground)
            WhenPaintForeground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);
        else if(style_.show_focus)
            UiPaintStyledForeground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);
        return;
    }

    Rect media_r = GetMediaRect(content);
    Rect text_r = content;

    if(!media_r.IsEmpty()) {
        switch(style_.media_side) {
        case UiAlign::LEFT:   text_r.left = min(text_r.right, media_r.right + style_.media_gap); break;
        case UiAlign::RIGHT:  text_r.right = max(text_r.left, media_r.left - style_.media_gap); break;
        case UiAlign::TOP:    text_r.top = min(text_r.bottom, media_r.bottom + style_.media_gap); break;
        case UiAlign::BOTTOM: text_r.bottom = max(text_r.top, media_r.top - style_.media_gap); break;
        default: break;
        }
    }

    if(!media_r.IsEmpty() && !IsNull(media_)) {
        Rect draw_r = media_r;
        if(style_.preserve_media_aspect) {
            Size isz = media_pref_.cx > 0 && media_pref_.cy > 0 ? media_pref_ : media_.GetSize();
            if(isz.cx > 0 && isz.cy > 0) {
                double sx = double(media_r.GetWidth()) / double(isz.cx);
                double sy = double(media_r.GetHeight()) / double(isz.cy);
                double s = min(sx, sy);
                if(media_pref_.cx > 0 && media_pref_.cy > 0)
                    s = min(s, 1.0);
                int dw = max(1, int(isz.cx * s));
                int dh = max(1, int(isz.cy * s));
                int dx = media_r.left;
                if(style_.media_align_h == UiAlign::CENTER)
                    dx = media_r.left + (media_r.GetWidth() - dw) / 2;
                else if(style_.media_align_h == UiAlign::RIGHT)
                    dx = media_r.right - dw;

                int dy = media_r.top;
                if(style_.media_align_v == UiAlign::CENTER)
                    dy = media_r.top + (media_r.GetHeight() - dh) / 2;
                else if(style_.media_align_v == UiAlign::BOTTOM)
                    dy = media_r.bottom - dh;

                draw_r = RectC(dx,
                               dy,
                               dw, dh);
            }
        }
        w.DrawImage(draw_r.left, draw_r.top, draw_r.GetWidth(), draw_r.GetHeight(), media_);
    }

    Color ink = style_.palette.ink[st];
    if(IsNull(ink)) ink = SColorText();

    int y = text_r.top;

    if(!title_.IsEmpty()) {
        Size ts = title_size_;
        int tx = text_r.left;
        if(style_.text_align_h == UiAlign::CENTER)
            tx = text_r.left + (text_r.GetWidth() - ts.cx) / 2;
        else if(style_.text_align_h == UiAlign::RIGHT)
            tx = text_r.right - ts.cx;

        w.DrawText(tx, y, title_, style_.title_font, ink);
        y += ts.cy;

        if(style_.show_rule && style_.rule_extent != NONE) {
            y += style_.rule_gap_above;
            int rcx = GetRuleWidth(ts.cx, max(0, text_r.GetWidth()));
            if(rcx > 0) {
                int rx = text_r.left;
                if(style_.text_align_h == UiAlign::CENTER)
                    rx = text_r.left + (text_r.GetWidth() - rcx) / 2;
                else if(style_.text_align_h == UiAlign::RIGHT)
                    rx = text_r.right - rcx;
                DrawRule(w, rx, y, rcx, Blend(ink, SColorShadow(), 55), style_.rule_thickness, style_.rule_style);
            }
            y += max(1, style_.rule_thickness) + style_.rule_gap_below;
        }
    }

    if(!subtitle_.IsEmpty()) {
        if(y > text_r.top)
            y += style_.title_subtitle_gap;

        Size ts = subtitle_size_;
        int tx = text_r.left;
        if(style_.text_align_h == UiAlign::CENTER)
            tx = text_r.left + (text_r.GetWidth() - ts.cx) / 2;
        else if(style_.text_align_h == UiAlign::RIGHT)
            tx = text_r.right - ts.cx;

        w.DrawText(tx, y, subtitle_, style_.subtitle_font, Blend(ink, SColorPaper(), 40));
        y += ts.cy;
    }

    if(!copy_.IsEmpty()) {
        if(y > text_r.top)
            y += style_.subtitle_copy_gap;

        Size ts = copy_size_;
        int tx = text_r.left;
        if(style_.text_align_h == UiAlign::CENTER)
            tx = text_r.left + (text_r.GetWidth() - ts.cx) / 2;
        else if(style_.text_align_h == UiAlign::RIGHT)
            tx = text_r.right - ts.cx;

        w.DrawText(tx, y, copy_, style_.copy_font, Blend(ink, SColorPaper(), 35));
    }

    if(style_.show_bottom_line && style_.bottom_line_extent != NONE) {
        int cx = max(0, content.GetWidth() - DPI(8));
        if(style_.bottom_line_extent == SMALL)
            cx = min(cx, DPI(40));
        else if(style_.bottom_line_extent == MEDIUM)
            cx = min(cx, (content.GetWidth() * 60) / 100);

        if(cx > 0) {
            int x = content.left + (content.GetWidth() - cx) / 2;
            int y = content.bottom - max(1, style_.bottom_line_thickness);
            Color lc = IsNull(style_.bottom_line_color)
                           ? Blend(ink, SColorShadow(), 80)
                           : style_.bottom_line_color;

            DrawRule(w, x, y, cx, lc, style_.bottom_line_thickness, style_.bottom_line_style);
        }
    }

    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);
    else if(style_.show_focus)
        UiPaintStyledForeground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);

}

void UiTitleCard::MouseEnter(Point, dword)
{
    if(style_.hover_enabled && IsEnabled()) {
        hot_ = true;
        Refresh();
    }
}

void UiTitleCard::MouseLeave()
{
    if(style_.hover_enabled) {
        hot_ = false;
        down_ = false;
        Refresh();
    }
}

void UiTitleCard::LeftDown(Point, dword)
{
    if(!IsEnabled())
        return;
    SetFocus();
    if(style_.hover_enabled) {
        down_ = true;
        Refresh();
    }
}

void UiTitleCard::LeftUp(Point, dword)
{
    if(style_.hover_enabled) {
        down_ = false;
        Refresh();
    }
}

}
