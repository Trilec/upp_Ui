#include <Ui/UiTitleCard.h>
#include <Ui/UiTheme.h>

namespace Upp {

const UiTitleCard::Style& UiTitleCard::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color face = White();
        Color frame = Color(215, 219, 226);
        Color ink = Color(17, 24, 39);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i] = ink;
        }
        s.palette.face[ST_HOT] = UiFill::Solid(Color(241, 245, 249));
        s.palette.face[ST_PRESSED] = UiFill::Solid(Color(226, 232, 240));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.palette.ink[ST_DISABLED] = Color(148, 163, 184);
        s.metrics.radius = DPI(8);
        s.metrics.frame_width = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled = true;
        s.metrics.content_margin = Rect(DPI(12), DPI(10), DPI(12), DPI(10));
        s.title_line_style = SOLID;
        s.title_line_length = LARGE;
        s.title_line_thickness = DPI(1);
        s.title_line_gap_above = DPI(5);
        s.title_line_gap_below = DPI(5);
        s.media_side = UiAlign::LEFT;
        s.media_reserve = DPI(48);
        s.media_gap = DPI(8);
        s.card_line_color = Null;
    }
    return s;
}

UiTitleCard::UiTitleCard()
{
    BackPaint();
    NoWantFocus();
    SyncThemeStyle();
    OnStyleChanged();
}

void UiTitleCard::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiTitleCard::Style& UiTitleCard::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiTitleCard::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    themed_style_ = UiTheme::ResolveTitleCard();
    theme_revision_ = revision;
}

const UiTitleCard::Style& UiTitleCard::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiTitleCard*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiTitleCard& UiTitleCard::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiTitleCard& UiTitleCard::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiTitleCard::OnStyleChanged()
{
    const Style& style = GetEffectiveStyle();
    if(style.transparent)
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
        StyleEdit().media_side = side;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetMediaAlign(UiAlign h, UiAlign v)
{
    StyleEdit().media_align_h = h;
    StyleEdit().media_align_v = v;
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetMediaReserve(int px)
{
    Style& style = StyleEdit();
    style.media_reserve = max(DPI(16), px);
    style.media_auto_fit = false;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetMediaSharePercent(int pct)
{
    Style& style = StyleEdit();
    style.media_share_percent = clamp(pct, 0, 90);
    if(style.media_share_percent > 0 && style.media_share_percent < 10)
        style.media_share_percent = 10;
    if(style.media_share_percent > 0)
        style.media_auto_fit = false;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetMediaAutoFit(bool on)
{
    Style& style = StyleEdit();
    style.media_auto_fit = on;
    if(on)
        style.media_share_percent = 0;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetTitleLine(UiSpan ex, int thickness, UiLineStyle style, Color c)
{
    Style& st = StyleEdit();
    st.title_line_length = ex;
    st.title_line_thickness = max(1, thickness);
    st.title_line_style = style;
    st.title_line_color = c;
    InvalidateTextCache();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::SetCardLine(UiSpan ex, int thickness, UiLineStyle style, Color c)
{
    Style& st = StyleEdit();
    st.card_line_length = ex;
    st.card_line_thickness = max(1, thickness);
    st.card_line_style = style;
    st.card_line_color = c;
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::ShowTitleLine(bool on)
{
    StyleEdit().title_line = on;
    InvalidateTextCache();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::ShowCardLine(bool on)
{
    StyleEdit().card_line = on;
    Refresh();
    return *this;
}

UiTitleCard& UiTitleCard::EnableHover(bool on)
{
    StyleEdit().hover_enabled = on;
    if(!on)
        hot_ = down_ = false;
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

    const Style& style = GetEffectiveStyle();
    title_size_ = title_.IsEmpty() ? Size(0, 0) : GetTextSize(title_, style.title_font);
    subtitle_size_ = subtitle_.IsEmpty() ? Size(0, 0) : GetTextSize(subtitle_, style.subtitle_font);
    copy_size_ = copy_.IsEmpty() ? Size(0, 0) : GetTextSize(copy_, style.copy_font);

    int w = 0;
    int h = 0;

    if(!title_.IsEmpty()) {
        w = max(w, title_size_.cx);
        h += title_size_.cy;
    }

    if(style.title_line && style.title_line_length != NONE && !title_.IsEmpty())
        h += style.title_line_gap_above + style.title_line_thickness + style.title_line_gap_below;

    if(!subtitle_.IsEmpty()) {
        w = max(w, subtitle_size_.cx);
        if(h > 0 && !title_.IsEmpty())
            h += style.title_subtitle_gap;
        h += subtitle_size_.cy;
    }

    if(!copy_.IsEmpty()) {
        w = max(w, copy_size_.cx);
        if(h > 0 && (!title_.IsEmpty() || !subtitle_.IsEmpty()))
            h += style.subtitle_copy_gap;
        h += copy_size_.cy;
    }

    text_block_size_ = Size(w, h);
    text_metrics_dirty_ = false;
}

int UiTitleCard::GetLineWidth(UiSpan length, int title_cx, int text_cx) const
{
    switch(length) {
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

    const Style& style = GetEffectiveStyle();
    bool horizontal = style.media_side == UiAlign::LEFT || style.media_side == UiAlign::RIGHT;
    bool auto_fit = style.media_auto_fit && style.media_share_percent <= 0 && media_pref_.cx <= 0 && media_pref_.cy <= 0;

    if(auto_fit) {
        Size isz = media_.GetSize();
        if(isz.cx <= 0 || isz.cy <= 0)
            return RectC(0, 0, 0, 0);

        int dw = style.media_min;
        int dh = style.media_min;
        if(horizontal) {
            int target_h = clamp(text_block_size_.cy > 0 ? text_block_size_.cy : style.media_min,
                                 style.media_min, max(style.media_min, content.GetHeight()));
            dh = target_h;
            dw = max(1, isz.cx * dh / max(1, isz.cy));
            if(dw > content.GetWidth()) {
                dw = content.GetWidth();
                dh = max(1, isz.cy * dw / max(1, isz.cx));
            }
        }
        else {
            int target_w = clamp(text_block_size_.cx > 0 ? text_block_size_.cx : style.media_min,
                                 style.media_min, max(style.media_min, content.GetWidth()));
            dw = target_w;
            dh = max(1, isz.cy * dw / max(1, isz.cx));
            if(dh > content.GetHeight()) {
                dh = content.GetHeight();
                dw = max(1, isz.cx * dh / max(1, isz.cy));
            }
        }

        int x = content.left;
        int y = content.top;
        if(style.media_side == UiAlign::RIGHT)
            x = content.right - dw;
        else if(!horizontal && style.media_align_h == UiAlign::CENTER)
            x = content.left + (content.GetWidth() - dw) / 2;
        else if(!horizontal && style.media_align_h == UiAlign::RIGHT)
            x = content.right - dw;

        if(style.media_side == UiAlign::BOTTOM)
            y = content.bottom - dh;
        else if(style.media_side == UiAlign::TOP)
            y = content.top;
        else if(style.media_align_v == UiAlign::CENTER)
            y = content.top + (content.GetHeight() - dh) / 2;
        else if(style.media_align_v == UiAlign::BOTTOM)
            y = content.bottom - dh;

        return RectC(x, y, max(1, dw), max(1, dh));
    }

    Rect r = content;
    int reserve = max(style.media_min, style.media_reserve);
    if(style.media_share_percent > 0) {
        int axis = horizontal ? content.GetWidth() : content.GetHeight();
        reserve = max(style.media_min, (axis * style.media_share_percent) / 100);
    }

    switch(style.media_side) {
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

    const Style& style = GetEffectiveStyle();
    bool media_h = !IsNull(media_) && (style.media_side == UiAlign::LEFT || style.media_side == UiAlign::RIGHT);
    bool auto_fit = !IsNull(media_) && style.media_auto_fit && style.media_share_percent <= 0 && media_pref_.cx <= 0 && media_pref_.cy <= 0;

    int media_axis = max(style.media_min, style.media_reserve);
    if(style.media_share_percent > 0)
        media_axis = max(media_axis, DPI(48));
    if(auto_fit && media.cx > 0 && media.cy > 0) {
        if(media_h) {
            int h = max(style.media_min, text.cy);
            media_axis = max(1, media.cx * h / max(1, media.cy));
            media.cy = h;
        }
        else {
            int w = max(style.media_min, text.cx);
            media_axis = max(1, media.cy * w / max(1, media.cx));
            media.cx = w;
        }
    }

    int cw = text.cx;
    int ch = text.cy;
    if(!IsNull(media_)) {
        if(media_h) {
            cw += media_axis + style.media_gap;
            ch = max(ch, max(style.media_min, media.cy));
        } else {
            cw = max(cw, max(style.media_min, media.cx));
            ch += media_axis + style.media_gap;
        }
    }

    Size out = UiStyledOuterSizeFromContent(Size(cw, ch), style.metrics, style.skin);
    if(user_min_size_.cx > 0)
        out.cx = max(out.cx, user_min_size_.cx);
    if(user_min_size_.cy > 0)
        out.cy = max(out.cy, user_min_size_.cy);
    return out;
}

void UiTitleCard::DrawLine(Draw& w, int x, int y, int cx, Color c, int thickness, UiLineStyle style) const
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

    const Style& style = GetEffectiveStyle();
    StyledState st = ST_DISABLED;
    if(IsEnabled()) {
        if(style.hover_enabled && down_)
            st = ST_PRESSED;
        else if(style.hover_enabled && hot_)
            st = ST_HOT;
        else
            st = ST_NORMAL;
    }
    bool has_focus = HasFocus();

    if(WhenPaintBackground)
        WhenPaintBackground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);
    else
        UiPaintStyledBackground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);

    w.Clip(outer);

    Rect content = UiStyledInnerRect(outer, style.metrics, style.skin);
    if(content.IsEmpty()) {
        if(WhenPaintForeground)
            WhenPaintForeground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);
        else if(style.metrics.focus_enabled)
            UiPaintStyledForeground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);
        w.End();
        return;
    }

    Rect media_r = GetMediaRect(content);
    Rect text_r = content;

    if(!media_r.IsEmpty()) {
        switch(style.media_side) {
        case UiAlign::LEFT:
            text_r.left = min(text_r.right, media_r.right + style.media_gap);
            break;
        case UiAlign::RIGHT:
            text_r.right = max(text_r.left, media_r.left - style.media_gap);
            break;
        case UiAlign::TOP:
            text_r.top = min(text_r.bottom, media_r.bottom + style.media_gap);
            break;
        case UiAlign::BOTTOM:
            text_r.bottom = max(text_r.top, media_r.top - style.media_gap);
            break;
        default:
            break;
        }
    }

    if(!media_r.IsEmpty() && !IsNull(media_)) {
        Rect draw_r = media_r;
        if(style.preserve_media_aspect) {
            Size isz = media_pref_.cx > 0 && media_pref_.cy > 0 ? media_pref_ : media_.GetSize();
            if(isz.cx > 0 && isz.cy > 0) {
                double sx = double(media_r.GetWidth()) / double(isz.cx);
                double sy = double(media_r.GetHeight()) / double(isz.cy);
                double scale = min(sx, sy);
                if(media_pref_.cx > 0 && media_pref_.cy > 0)
                    scale = min(scale, 1.0);
                int dw = max(1, int(isz.cx * scale));
                int dh = max(1, int(isz.cy * scale));
                int dx = media_r.left;
                if(style.media_align_h == UiAlign::CENTER)
                    dx = media_r.left + (media_r.GetWidth() - dw) / 2;
                else if(style.media_align_h == UiAlign::RIGHT)
                    dx = media_r.right - dw;

                int dy = media_r.top;
                if(style.media_align_v == UiAlign::CENTER)
                    dy = media_r.top + (media_r.GetHeight() - dh) / 2;
                else if(style.media_align_v == UiAlign::BOTTOM)
                    dy = media_r.bottom - dh;

                draw_r = RectC(dx, dy, dw, dh);
            }
        }

        if(style.media_tint_mono) {
            Color media_ink = UiResolveIconColor(style.palette, st);
        UiPaintStyledIcon(w, draw_r, media_, true, true, UiIconRenderMode::MonoTint, media_ink, IsEnabled());
        }
        else {
            w.DrawImage(draw_r.left, draw_r.top, draw_r.GetWidth(), draw_r.GetHeight(), media_);
        }
    }

    Color ink = style.palette.ink[st];
    if(IsNull(ink))
        ink = SColorText();
    Color title_ink = IsNull(style.title_color) ? ink : style.title_color;
    Color subtitle_ink = IsNull(style.subtitle_color) ? Blend(ink, SColorPaper(), 40) : style.subtitle_color;
    Color copy_ink = IsNull(style.copy_color) ? Blend(ink, SColorPaper(), 35) : style.copy_color;

    int y = text_r.top + max(0, (text_r.GetHeight() - text_block_size_.cy) / 2);

    if(!title_.IsEmpty()) {
        Size ts = title_size_;
        int tx = text_r.left;
        if(style.text_align_h == UiAlign::CENTER)
            tx = text_r.left + (text_r.GetWidth() - ts.cx) / 2;
        else if(style.text_align_h == UiAlign::RIGHT)
            tx = text_r.right - ts.cx;

        w.DrawText(tx, y, title_, style.title_font, title_ink);
        y += ts.cy;

        if(style.title_line && style.title_line_length != NONE) {
            y += style.title_line_gap_above;
            int rcx = GetLineWidth(style.title_line_length, ts.cx, max(0, text_r.GetWidth()));
            if(rcx > 0) {
                int rx = text_r.left;
                if(style.text_align_h == UiAlign::CENTER)
                    rx = text_r.left + (text_r.GetWidth() - rcx) / 2;
                else if(style.text_align_h == UiAlign::RIGHT)
                    rx = text_r.right - rcx;
                Color tc = IsNull(style.title_line_color)
                               ? Blend(title_ink, SColorShadow(), 55)
                               : style.title_line_color;
                DrawLine(w, rx, y, rcx, tc, style.title_line_thickness, style.title_line_style);
            }
            y += max(1, style.title_line_thickness) + style.title_line_gap_below;
        }
    }

    if(!subtitle_.IsEmpty()) {
        if(y > text_r.top && !title_.IsEmpty())
            y += style.title_subtitle_gap;

        Size ts = subtitle_size_;
        int tx = text_r.left;
        if(style.text_align_h == UiAlign::CENTER)
            tx = text_r.left + (text_r.GetWidth() - ts.cx) / 2;
        else if(style.text_align_h == UiAlign::RIGHT)
            tx = text_r.right - ts.cx;

        w.DrawText(tx, y, subtitle_, style.subtitle_font, subtitle_ink);
        y += ts.cy;
    }

    if(!copy_.IsEmpty()) {
        if(y > text_r.top && (!title_.IsEmpty() || !subtitle_.IsEmpty()))
            y += style.subtitle_copy_gap;

        Size ts = copy_size_;
        int tx = text_r.left;
        if(style.text_align_h == UiAlign::CENTER)
            tx = text_r.left + (text_r.GetWidth() - ts.cx) / 2;
        else if(style.text_align_h == UiAlign::RIGHT)
            tx = text_r.right - ts.cx;

        w.DrawText(tx, y, copy_, style.copy_font, copy_ink);
    }

    if(style.card_line && style.card_line_length != NONE) {
        int cx = style.card_line_length == LARGE ? outer.GetWidth() : max(0, content.GetWidth() - DPI(8));
        if(style.card_line_length == SMALL)
            cx = min(cx, DPI(40));
        else if(style.card_line_length == MEDIUM)
            cx = min(cx, (content.GetWidth() * 60) / 100);

        if(cx > 0) {
            int x = style.card_line_length == LARGE ? outer.left : content.left + (content.GetWidth() - cx) / 2;
            int line_y = content.bottom - max(1, style.card_line_thickness);
            Color lc = IsNull(style.card_line_color)
                           ? Blend(ink, SColorShadow(), 80)
                           : style.card_line_color;

            DrawLine(w, x, line_y, cx, lc, style.card_line_thickness, style.card_line_style);
        }
    }

    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);
    else if(style.metrics.focus_enabled)
        UiPaintStyledForeground(w, outer, style.palette, style.metrics, style.skin, st, has_focus);

    w.End();
}

void UiTitleCard::MouseEnter(Point, dword)
{
    const Style& style = GetEffectiveStyle();
    if(style.hover_enabled && IsEnabled()) {
        hot_ = true;
        Refresh();
    }
}

void UiTitleCard::MouseLeave()
{
    const Style& style = GetEffectiveStyle();
    if(style.hover_enabled) {
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
    const Style& style = GetEffectiveStyle();
    if(style.hover_enabled) {
        down_ = true;
        Refresh();
    }
}

void UiTitleCard::LeftUp(Point, dword)
{
    const Style& style = GetEffectiveStyle();
    if(style.hover_enabled) {
        down_ = false;
        Refresh();
    }
}

}
