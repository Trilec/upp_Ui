#include <Ui/UiGroupPanel.h>
#include <Ui/UiTheme.h>

namespace Upp {

void UiGroupPanel::Style::Serialize(Stream& s)
{
    int hp = (int)header_placement;
    int ha = (int)title_align_h;
    int va = (int)title_align_v;
    int hm = (int)header_mode;
    s % palette % metrics % skin
      % title_font % subtitle_font % side_title_font
      % title_color % subtitle_color % side_title_color
      % hp % ha % va % hm
      % inset % header_inset % header_gap % icon_size % icon_gap
      % title_subtitle_gap % side_title_gap % separator_thickness
      % line_enabled % header_band_enabled % transparent;
    header_placement = (UiAlign)hp;
    title_align_h = (UiAlign)ha;
    title_align_v = (UiAlign)va;
    header_mode = (HeaderMode)hm;
}

const UiGroupPanel::Style& UiGroupPanel::StyleDefault()
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
            s.palette.icon[i] = ink;
        }
        s.palette.face[ST_HOT] = UiFill::Solid(Color(248, 250, 252));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.palette.ink[ST_DISABLED] = Color(148, 163, 184);
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(8);
        s.metrics.content_margin = Rect(0, 0, 0, 0);
        s.metrics.focus_enabled = false;
    }
    return s;
}

UiGroupPanel::UiGroupPanel()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
{
    BackPaint();
    NoWantFocus();
    SyncThemeStyle();
}

void UiGroupPanel::InvalidateStyleCache() { theme_revision_ = 0; }

UiGroupPanel::Style& UiGroupPanel::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiGroupPanel::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 rev = UiTheme::GetRevision();
    if(theme_revision_ == rev)
        return;
    themed_style_ = UiTheme::ResolveGroupPanel();
    theme_revision_ = rev;
}

const UiGroupPanel::Style& UiGroupPanel::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiGroupPanel *>(this)->SyncThemeStyle();
    return themed_style_;
}

UiGroupPanel& UiGroupPanel::SetCustomStyle(const Style& s) { style_ = s; has_custom_style_ = true; OnStyleChanged(); return *this; }
UiGroupPanel& UiGroupPanel::ClearCustomStyle() { has_custom_style_ = false; style_ = StyleDefault(); InvalidateStyleCache(); OnStyleChanged(); return *this; }

void UiGroupPanel::OnStyleChanged()
{
    const Style& s = GetEffectiveStyle();
    if(s.transparent)
        Transparent();
    else
        BackPaint();
    RefreshLayout();
    Refresh();
}

UiGroupPanel& UiGroupPanel::SetTitle(const String& s) { title_ = s; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetSubTitle(const String& s) { subtitle_ = s; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetSideTitle(const String& s) { side_title_ = s; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetIcon(const Image& img) { icon_ = img; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::ClearIcon() { icon_ = Image(); RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetHeaderPlacement(UiAlign side) { if(side == UiAlign::TOP || side == UiAlign::BOTTOM || side == UiAlign::LEFT || side == UiAlign::RIGHT) StyleEdit().header_placement = side; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetHeaderMode(HeaderMode mode) { StyleEdit().header_mode = mode; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetLine(bool on) { StyleEdit().line_enabled = on; Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetHeaderBand(bool on) { StyleEdit().header_band_enabled = on; Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetInset(const Rect& r) { StyleEdit().inset = UiNonNegativeThickness(r); RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetHeaderInset(const Rect& r) { StyleEdit().header_inset = UiNonNegativeThickness(r); RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetIconSize(int px) { StyleEdit().icon_size = max(0, px); RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetLineThickness(int px) { StyleEdit().separator_thickness = max(1, px); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetTitleFont(Font f) { StyleEdit().title_font = f; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetSubTitleFont(Font f) { StyleEdit().subtitle_font = f; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetSideTitleFont(Font f) { StyleEdit().side_title_font = f; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetTitleSubTitleGap(int px) { StyleEdit().title_subtitle_gap = max(0, px); RefreshLayout(); Refresh(); return *this; }

UiGroupPanel& UiGroupPanel::SetContent(Ctrl& ctrl)
{
    if(content_ == &ctrl)
        return *this;

    if(content_)
        content_->Remove();
    content_ = &ctrl;
    Add(ctrl);
    RefreshLayout();
    return *this;
}

UiGroupPanel& UiGroupPanel::ClearContent()
{
    if(content_) {
        content_->Remove();
        content_ = nullptr;
        RefreshLayout();
    }
    return *this;
}

Size UiGroupPanel::GetHeaderSize() const
{
    const Style& s = GetEffectiveStyle();
    Size title_sz = title_.IsEmpty() ? Size(0, 0) : GetTextSize(title_, s.title_font);
    Size sub_sz = subtitle_.IsEmpty() ? Size(0, 0) : GetTextSize(subtitle_, s.subtitle_font);
    Size trail_sz = side_title_.IsEmpty() ? Size(0, 0) : GetTextSize(side_title_, s.side_title_font);
    int text_w = max(title_sz.cx, sub_sz.cx);
    int text_h = title_sz.cy + (title_sz.cy && sub_sz.cy ? s.title_subtitle_gap : 0) + sub_sz.cy;
    int icon_w = IsNull(icon_) || s.icon_size <= 0 ? 0 : s.icon_size + s.icon_gap;
    int trailing_w = trail_sz.cx ? trail_sz.cx + s.side_title_gap : 0;
    int w = s.header_inset.left + s.header_inset.right + icon_w + text_w + trailing_w;
    int h = s.header_inset.top + s.header_inset.bottom + max(max(text_h, trail_sz.cy), IsNull(icon_) ? 0 : s.icon_size);
    return Size(max(DPI(24), w), max(DPI(24), h));
}

Rect UiGroupPanel::GetFrameRect(const Rect& face) const
{
    const Style& s = GetEffectiveStyle();
    Size hs = GetHeaderSize();
    Rect frame = face.Deflated(s.inset.left, s.inset.top, s.inset.right, s.inset.bottom);
    if(s.header_mode == Outside) {
        switch(s.header_placement) {
        case UiAlign::BOTTOM: frame.bottom = max(frame.top, face.bottom - hs.cy - s.header_gap); break;
        case UiAlign::LEFT:   frame.left = min(frame.right, face.left + hs.cx + s.header_gap); break;
        case UiAlign::RIGHT:  frame.right = max(frame.left, face.right - hs.cx - s.header_gap); break;
        case UiAlign::TOP:
        default:              frame.top = min(frame.bottom, face.top + hs.cy + s.header_gap); break;
        }
    }
    else if(s.header_mode == Center) {
        switch(s.header_placement) {
        case UiAlign::BOTTOM: frame.bottom = max(frame.top, face.bottom - hs.cy / 2); break;
        case UiAlign::LEFT:   frame.left = min(frame.right, face.left + hs.cx / 2); break;
        case UiAlign::RIGHT:  frame.right = max(frame.left, face.right - hs.cx / 2); break;
        case UiAlign::TOP:
        default:              frame.top = min(frame.bottom, face.top + hs.cy / 2); break;
        }
    }
    return frame;
}

Rect UiGroupPanel::GetHeaderRect(const Rect& face) const
{
    const Style& s = GetEffectiveStyle();
    Size hs = GetHeaderSize();
    Rect area = face.Deflated(s.inset.left, s.inset.top, s.inset.right, s.inset.bottom);
    switch(s.header_placement) {
    case UiAlign::BOTTOM: return Rect(area.left, area.bottom - hs.cy, area.right, area.bottom);
    case UiAlign::LEFT:   return Rect(area.left, area.top, area.left + hs.cx, area.bottom);
    case UiAlign::RIGHT:  return Rect(area.right - hs.cx, area.top, area.right, area.bottom);
    case UiAlign::TOP:
    default:              return Rect(area.left, area.top, area.right, area.top + hs.cy);
    }
}

Rect UiGroupPanel::GetBodyRect(const Rect& face) const
{
    const Style& s = GetEffectiveStyle();
    Rect frame = GetFrameRect(face);
    Rect header = GetHeaderRect(face);
    Rect body = frame;
    if(s.header_mode != Outside) {
        switch(s.header_placement) {
        case UiAlign::BOTTOM: body.bottom = min(body.bottom, header.top - s.header_gap); break;
        case UiAlign::LEFT:   body.left = max(body.left, header.right + s.header_gap); break;
        case UiAlign::RIGHT:  body.right = min(body.right, header.left - s.header_gap); break;
        case UiAlign::TOP:
        default:              body.top = max(body.top, header.bottom + s.header_gap); break;
        }
    }
    return body;
}

Rect UiGroupPanel::GetTitleBlockRect(const Rect& header, Size block) const
{
    const Style& s = GetEffectiveStyle();
    int x = header.left + s.header_inset.left;
    int y = header.top + s.header_inset.top;
    int avail_w = max(0, header.GetWidth() - s.header_inset.left - s.header_inset.right);
    int avail_h = max(0, header.GetHeight() - s.header_inset.top - s.header_inset.bottom);
    if(s.title_align_h == UiAlign::CENTER)
        x += max(0, (avail_w - block.cx) / 2);
    else if(s.title_align_h == UiAlign::RIGHT)
        x += max(0, avail_w - block.cx);
    if(s.title_align_v == UiAlign::CENTER)
        y += max(0, (avail_h - block.cy) / 2);
    else if(s.title_align_v == UiAlign::BOTTOM)
        y += max(0, avail_h - block.cy);
    return RectC(x, y, min(block.cx, avail_w), min(block.cy, avail_h));
}

void UiGroupPanel::Layout()
{
    Rect face = UiStyledFaceRect(GetSize(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
    if(content_)
        content_->SetRect(GetBodyRect(face));
}

Rect UiGroupPanel::GetBodyRect() const
{
    Rect face = UiStyledFaceRect(GetSize(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
    return GetBodyRect(face);
}

Size UiGroupPanel::GetMinSize() const
{
    const Style& s = GetEffectiveStyle();
    Size hs = GetHeaderSize();
    Size cs = content_ ? content_->GetMinSize() : Size(DPI(40), DPI(28));
    cs.cx += s.inset.left + s.inset.right;
    cs.cy += s.inset.top + s.inset.bottom;
    bool vertical = s.header_placement == UiAlign::TOP || s.header_placement == UiAlign::BOTTOM;
    Size content = vertical ? Size(max(hs.cx, cs.cx), hs.cy + s.header_gap + cs.cy)
                            : Size(hs.cx + s.header_gap + cs.cx, max(hs.cy, cs.cy));
    return UiStyledOuterSizeFromContent(content, s.metrics, s.skin);
}

void UiGroupPanel::PaintHeader(Draw& w, const Rect& header, StyledState st) const
{
    const Style& s = GetEffectiveStyle();
    Size title_sz = title_.IsEmpty() ? Size(0, 0) : GetTextSize(title_, s.title_font);
    Size sub_sz = subtitle_.IsEmpty() ? Size(0, 0) : GetTextSize(subtitle_, s.subtitle_font);
    Size trail_sz = side_title_.IsEmpty() ? Size(0, 0) : GetTextSize(side_title_, s.side_title_font);
    int text_w = max(title_sz.cx, sub_sz.cx);
    int text_h = title_sz.cy + (title_sz.cy && sub_sz.cy ? s.title_subtitle_gap : 0) + sub_sz.cy;
    int icon_w = IsNull(icon_) || s.icon_size <= 0 ? 0 : s.icon_size + s.icon_gap;
    int trailing_w = trail_sz.cx ? trail_sz.cx + s.side_title_gap : 0;
    int available_w = max(0, header.GetWidth() - s.header_inset.left - s.header_inset.right);
    int block_w = side_title_.IsEmpty() ? icon_w + text_w : available_w;
    Rect block = GetTitleBlockRect(header, Size(block_w, max(max(text_h, trail_sz.cy), IsNull(icon_) ? 0 : s.icon_size)));
    int x = block.left;
    if(!IsNull(icon_) && s.icon_size > 0) {
        Rect ir = RectC(x, block.top + max(0, (block.GetHeight() - s.icon_size) / 2), s.icon_size, s.icon_size);
        UiPaintStyledIcon(w, ir, icon_, true, true, UiIconRenderMode::MonoTint, s.palette.icon[st], IsEnabled());
        x += s.icon_size + s.icon_gap;
    }
    Color title_c = IsNull(s.title_color) ? s.palette.ink[st] : s.title_color;
    Color sub_c = IsNull(s.subtitle_color) ? Blend(s.palette.ink[st], SColorPaper(), 80) : s.subtitle_color;
    Color trail_c = IsNull(s.side_title_color) ? sub_c : s.side_title_color;
    int ty = block.top + max(0, (block.GetHeight() - text_h) / 2);
    int subtitle_y = ty + (title_.IsEmpty() ? 0 : title_sz.cy + (subtitle_.IsEmpty() ? 0 : s.title_subtitle_gap));
    int side_y = subtitle_.IsEmpty() ? ty : subtitle_y;
    int side_x = block.right - trail_sz.cx;
    if(s.title_align_h == UiAlign::RIGHT) {
        side_x = block.left;
        if(!side_title_.IsEmpty())
            x = max(x, block.left + trail_sz.cx + s.side_title_gap);
    }
    if(!title_.IsEmpty()) {
        w.DrawText(x, ty, title_, s.title_font, title_c);
        ty += title_sz.cy + (subtitle_.IsEmpty() ? 0 : s.title_subtitle_gap);
    }
    if(!subtitle_.IsEmpty())
        w.DrawText(x, ty, subtitle_, s.subtitle_font, sub_c);
    if(!side_title_.IsEmpty())
        w.DrawText(side_x, side_y, side_title_, s.side_title_font, trail_c);
}

void UiGroupPanel::PaintGroupFrame(Draw& w, const Rect& frame_rect, const Rect& title_block, StyledState st) const
{
    const Style& s = GetEffectiveStyle();
    if(frame_rect.IsEmpty())
        return;

    StyledMetrics metrics = s.metrics;
    metrics.content_margin = Rect(0, 0, 0, 0);
    UiPaintFaceFrameDash(w, frame_rect, s.palette, metrics, st);

    if(!s.metrics.frame_enabled)
        return;

    int fw = max(1, metrics.frame_width);
    bool cut = s.header_mode == Center;
    if(cut && s.header_placement == UiAlign::TOP) {
        int gap_l = max(frame_rect.left, title_block.left - DPI(4));
        int gap_r = min(frame_rect.right, title_block.right + DPI(4));
        Color erase = SColorFace();
        if(s.metrics.face_enabled && s.palette.face[st].IsSolid())
            erase = s.palette.face[st].color;
        w.DrawRect(gap_l, frame_rect.top, max(0, gap_r - gap_l), fw, erase);
    }
}

void UiGroupPanel::Paint(Draw& w)
{
    const Style& s = GetEffectiveStyle();
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;
    StyledState st = IsEnabled() ? ST_NORMAL : ST_DISABLED;
    Rect face = UiStyledFaceRect(outer, s.metrics, s.skin);
    Rect frame_rect = GetFrameRect(face);
    Rect header = GetHeaderRect(face);
    Rect title_block = GetTitleBlockRect(header, GetHeaderSize());

    Color frame = s.palette.frame[st];
    int th = max(1, s.separator_thickness);
    PaintGroupFrame(w, frame_rect, title_block, st);
    if(s.line_enabled) {
        if(s.header_placement == UiAlign::LEFT)
            w.DrawRect(header.right - th, header.top, th, header.GetHeight(), frame);
        else if(s.header_placement == UiAlign::RIGHT)
            w.DrawRect(header.left, header.top, th, header.GetHeight(), frame);
        else if(s.header_placement == UiAlign::BOTTOM)
            w.DrawRect(header.left, header.top, header.GetWidth(), th, frame);
        else
            w.DrawRect(header.left, header.bottom - th, header.GetWidth(), th, frame);
    }
    if(s.header_band_enabled) {
        if(s.palette.face[ST_HOT].IsSolid())
            w.DrawRect(header, s.palette.face[ST_HOT].color);
    }

    PaintHeader(w, header, st);
}

} // namespace Upp
