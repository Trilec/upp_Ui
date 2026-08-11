#include <Ui/UiGroupPanel.h>
#include <Ui/UiMeasure.h>
#include <Ui/UiTheme.h>

namespace Upp {

void UiGroupPanel::Style::Serialize(Stream& s)
{
    // Keep the retired SideTitle fields in their original stream positions.
    // They are consumed for compatibility but never affect live styling.
    Font legacy_side_title_font = SansSerifZ(9);
    Color legacy_side_title_color;
    int legacy_side_title_gap = DPI(8);
    int hp = (int)header_placement;
    int ha = (int)title_align_h;
    int va = (int)title_align_v;
    int hm = (int)header_mode;
    s % palette % metrics % skin
      % title_font % subtitle_font % legacy_side_title_font
      % title_color % subtitle_color % legacy_side_title_color
      % hp % ha % va % hm
      % inset % header_inset % header_gap % icon_size % icon_gap
      % title_subtitle_gap % legacy_side_title_gap % separator_thickness
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
UiGroupPanel& UiGroupPanel::SetIcon(const Image& img) { icon_ = img; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::ClearIcon() { icon_ = Image(); RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetHeaderPlacement(UiAlign side) { if(side == UiAlign::TOP || side == UiAlign::BOTTOM || side == UiAlign::LEFT || side == UiAlign::RIGHT) StyleEdit().header_placement = side; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetHeaderMode(HeaderMode mode) { StyleEdit().header_mode = mode; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetTitleAlign(UiAlign horizontal, UiAlign vertical)
{
    if(horizontal == UiAlign::LEFT || horizontal == UiAlign::CENTER || horizontal == UiAlign::RIGHT)
        StyleEdit().title_align_h = horizontal;
    if(vertical == UiAlign::TOP || vertical == UiAlign::CENTER || vertical == UiAlign::BOTTOM)
        StyleEdit().title_align_v = vertical;
    RefreshLayout();
    Refresh();
    return *this;
}

UiGroupPanel& UiGroupPanel::SetHeaderContentAlign(UiAlign horizontal, UiAlign vertical)
{
    if(horizontal == UiAlign::DEFAULT || horizontal == UiAlign::LEFT ||
       horizontal == UiAlign::CENTER || horizontal == UiAlign::RIGHT)
        header_content_align_h_ = horizontal;
    if(vertical == UiAlign::DEFAULT || vertical == UiAlign::TOP ||
       vertical == UiAlign::CENTER || vertical == UiAlign::BOTTOM)
        header_content_align_v_ = vertical;
    RefreshLayout();
    return *this;
}
UiGroupPanel& UiGroupPanel::SetLine(bool on) { StyleEdit().line_enabled = on; Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetHeaderBand(bool on) { StyleEdit().header_band_enabled = on; Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetInset(const Rect& r) { StyleEdit().inset = UiNonNegativeThickness(r); RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetHeaderInset(const Rect& r) { StyleEdit().header_inset = UiNonNegativeThickness(r); RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetIconSize(int px) { StyleEdit().icon_size = max(0, px); RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetLineThickness(int px) { StyleEdit().separator_thickness = max(1, px); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetTitleFont(Font f) { StyleEdit().title_font = f; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetSubTitleFont(Font f) { StyleEdit().subtitle_font = f; RefreshLayout(); Refresh(); return *this; }
UiGroupPanel& UiGroupPanel::SetTitleSubTitleGap(int px) { StyleEdit().title_subtitle_gap = max(0, px); RefreshLayout(); Refresh(); return *this; }

UiGroupPanel& UiGroupPanel::SetContent(Ctrl& ctrl)
{
    if(content_ == &ctrl)
        return *this;

    if(header_content_ == &ctrl) {
        if(content_)
            content_->Remove();
        header_content_ = nullptr;
        content_ = &ctrl;
        RefreshLayout();
        Refresh();
        return *this;
    }
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

UiGroupPanel& UiGroupPanel::SetHeaderContent(Ctrl& ctrl)
{
    if(header_content_ == &ctrl)
        return *this;

    if(content_ == &ctrl) {
        if(header_content_)
            header_content_->Remove();
        content_ = nullptr;
        header_content_ = &ctrl;
        RefreshLayout();
        Refresh();
        return *this;
    }
    if(header_content_)
        header_content_->Remove();
    header_content_ = &ctrl;
    Add(ctrl);
    RefreshLayout();
    Refresh();
    return *this;
}

UiGroupPanel& UiGroupPanel::ClearHeaderContent()
{
    if(header_content_) {
        Ctrl *child = header_content_;
        header_content_ = nullptr;
        child->Remove();
        RefreshLayout();
        Refresh();
    }
    return *this;
}

static Rect UiGroupPanelClampRect(Rect r)
{
    r.right = max(r.left, r.right);
    r.bottom = max(r.top, r.bottom);
    return r;
}

static int UiGroupPanelAlignedOffset(int available, int size, UiAlign align)
{
    if(align == UiAlign::RIGHT || align == UiAlign::BOTTOM)
        return max(0, available - size);
    if(align == UiAlign::CENTER)
        return max(0, (available - size) / 2);
    return 0;
}

Size UiGroupPanel::GetTitleNaturalSize() const
{
    const Style& s = GetEffectiveStyle();
    Size title_sz = title_.IsEmpty() ? Size(0, 0) : GetTextSize(title_, s.title_font);
    Size sub_sz = subtitle_.IsEmpty() ? Size(0, 0) : GetTextSize(subtitle_, s.subtitle_font);
    int text_w = max(title_sz.cx, sub_sz.cx);
    int text_h = title_sz.cy + (title_sz.cy && sub_sz.cy ? s.title_subtitle_gap : 0) + sub_sz.cy;
    int icon_size = !IsNull(icon_) && s.icon_size > 0 ? s.icon_size : 0;
    int icon_w = icon_size ? icon_size + (text_w ? s.icon_gap : 0) : 0;
    return Size(icon_w + text_w, max(text_h, icon_size));
}

Size UiGroupPanel::GetHeaderContentNaturalSize() const
{
    return header_content_ ? UiMeasureLayout(*header_content_).min : Size(0, 0);
}

Size UiGroupPanel::GetHeaderSize() const
{
    const Style& s = GetEffectiveStyle();
    Size title = GetTitleNaturalSize();
    Size child = GetHeaderContentNaturalSize();
    bool have_title = title.cx > 0 && title.cy > 0;
    bool have_child = child.cx > 0 && child.cy > 0;
    int gap = have_title && have_child ? s.header_gap : 0;
    bool horizontal = s.header_placement == UiAlign::TOP || s.header_placement == UiAlign::BOTTOM;
    int w;
    int h;
    if(horizontal) {
        int primary = title.cx + gap + child.cx;
        if(have_title && have_child && s.title_align_h == UiAlign::CENTER)
            primary = title.cx + 2 * (gap + child.cx);
        w = s.header_inset.left + s.header_inset.right + primary;
        h = s.header_inset.top + s.header_inset.bottom + max(title.cy, child.cy);
    }
    else {
        int primary = title.cy + gap + child.cy;
        if(have_title && have_child && s.title_align_v == UiAlign::CENTER)
            primary = title.cy + 2 * (gap + child.cy);
        w = s.header_inset.left + s.header_inset.right + max(title.cx, child.cx);
        h = s.header_inset.top + s.header_inset.bottom + primary;
    }
    return Size(max(DPI(24), w), max(DPI(24), h));
}

Rect UiGroupPanel::GetFrameRect(const Rect& face) const
{
    const Style& s = GetEffectiveStyle();
    Size hs = GetHeaderSize();
    Rect frame = UiGroupPanelClampRect(face.Deflated(s.inset.left, s.inset.top,
                                                    s.inset.right, s.inset.bottom));
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
    Rect area = UiGroupPanelClampRect(face.Deflated(s.inset.left, s.inset.top,
                                                   s.inset.right, s.inset.bottom));
    switch(s.header_placement) {
    case UiAlign::BOTTOM: return Rect(area.left, max(area.top, area.bottom - hs.cy), area.right, area.bottom);
    case UiAlign::LEFT:   return Rect(area.left, area.top, min(area.right, area.left + hs.cx), area.bottom);
    case UiAlign::RIGHT:  return Rect(max(area.left, area.right - hs.cx), area.top, area.right, area.bottom);
    case UiAlign::TOP:
    default:              return Rect(area.left, area.top, area.right, min(area.bottom, area.top + hs.cy));
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
    return UiGroupPanelClampRect(body);
}

UiGroupPanel::HeaderLayout UiGroupPanel::ResolveHeaderLayout(const Rect& face) const
{
    const Style& s = GetEffectiveStyle();
    HeaderLayout out;
    out.header = GetHeaderRect(face);
    Rect inner = UiGroupPanelClampRect(out.header.Deflated(s.header_inset.left, s.header_inset.top,
                                                           s.header_inset.right, s.header_inset.bottom));
    Size title = GetTitleNaturalSize();
    Size child = GetHeaderContentNaturalSize();
    bool horizontal = s.header_placement == UiAlign::TOP || s.header_placement == UiAlign::BOTTOM;
    bool have_title = title.cx > 0 && title.cy > 0;
    bool have_child = header_content_ && child.cx > 0 && child.cy > 0;
    int gap = have_title && have_child ? s.header_gap : 0;

    Rect title_region = inner;
    out.content_region = inner;

    // Reserve the attached content root first. This guarantees that a long
    // title or subtitle is clipped inside its own region instead of consuming
    // the action/content slot that callers explicitly attached.
    if(have_child) {
        if(horizontal) {
            int child_w = min(max(0, child.cx), inner.GetWidth());
            if(s.title_align_h == UiAlign::RIGHT) {
                out.content_region.right = min(inner.right, inner.left + child_w);
                title_region.left = min(inner.right, out.content_region.right + gap);
            }
            else {
                out.content_region.left = max(inner.left, inner.right - child_w);
                title_region.right = max(inner.left, out.content_region.left - gap);
            }
        }
        else {
            int child_h = min(max(0, child.cy), inner.GetHeight());
            if(s.title_align_v == UiAlign::BOTTOM) {
                out.content_region.bottom = min(inner.bottom, inner.top + child_h);
                title_region.top = min(inner.bottom, out.content_region.bottom + gap);
            }
            else {
                out.content_region.top = max(inner.top, inner.bottom - child_h);
                title_region.bottom = max(inner.top, out.content_region.top - gap);
            }
        }
    }

    title_region = UiGroupPanelClampRect(title_region);
    title.cx = min(max(0, title.cx), title_region.GetWidth());
    title.cy = min(max(0, title.cy), title_region.GetHeight());
    int tx = title_region.left + UiGroupPanelAlignedOffset(title_region.GetWidth(), title.cx, s.title_align_h);
    int ty = title_region.top + UiGroupPanelAlignedOffset(title_region.GetHeight(), title.cy, s.title_align_v);
    out.title = RectC(tx, ty, title.cx, title.cy);

    // Preserve the prospective header-content slot contract when no child is
    // attached: the remaining region follows the title as before.
    if(!have_child) {
        out.content_region = inner;
        if(horizontal && have_title) {
            if(s.title_align_h == UiAlign::RIGHT)
                out.content_region.right = max(inner.left, out.title.left - s.header_gap);
            else
                out.content_region.left = min(inner.right, out.title.right + s.header_gap);
        }
        else if(!horizontal && have_title) {
            if(s.title_align_v == UiAlign::BOTTOM)
                out.content_region.bottom = max(inner.top, out.title.top - s.header_gap);
            else
                out.content_region.top = min(inner.bottom, out.title.bottom + s.header_gap);
        }
    }
    out.content_region = UiGroupPanelClampRect(out.content_region);

    if(header_content_) {
        child.cx = min(max(0, child.cx), out.content_region.GetWidth());
        child.cy = min(max(0, child.cy), out.content_region.GetHeight());
        UiAlign ah = header_content_align_h_;
        UiAlign av = header_content_align_v_;
        if(ah == UiAlign::DEFAULT)
            ah = horizontal ? (s.title_align_h == UiAlign::RIGHT ? UiAlign::LEFT : UiAlign::RIGHT)
                            : UiAlign::CENTER;
        if(av == UiAlign::DEFAULT)
            av = horizontal ? UiAlign::CENTER
                            : (s.title_align_v == UiAlign::BOTTOM ? UiAlign::TOP : UiAlign::BOTTOM);
        int x = out.content_region.left + UiGroupPanelAlignedOffset(out.content_region.GetWidth(), child.cx, ah);
        int y = out.content_region.top + UiGroupPanelAlignedOffset(out.content_region.GetHeight(), child.cy, av);
        out.content = RectC(x, y, child.cx, child.cy);
    }
    return out;
}

void UiGroupPanel::Layout()
{
    Rect face = UiStyledFaceRect(GetSize(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
    if(content_)
        content_->SetRect(GetBodyRect(face));
    if(header_content_)
        header_content_->SetRect(ResolveHeaderLayout(face).content);
}

Rect UiGroupPanel::GetBodyRect() const
{
    Rect face = UiStyledFaceRect(GetSize(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
    return GetBodyRect(face);
}

Rect UiGroupPanel::GetHeaderContentRect() const
{
    Rect face = UiStyledFaceRect(GetSize(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
    return ResolveHeaderLayout(face).content_region;
}

Size UiGroupPanel::GetMinSize() const
{
    const Style& s = GetEffectiveStyle();
    Size hs = GetHeaderSize();
    Size cs = content_ ? UiMeasureLayout(*content_).min : Size(DPI(40), DPI(28));
    cs.cx += s.inset.left + s.inset.right;
    cs.cy += s.inset.top + s.inset.bottom;
    bool vertical = s.header_placement == UiAlign::TOP || s.header_placement == UiAlign::BOTTOM;
    Size content = vertical ? Size(max(hs.cx, cs.cx), hs.cy + s.header_gap + cs.cy)
                            : Size(hs.cx + s.header_gap + cs.cx, max(hs.cy, cs.cy));
    return UiStyledOuterSizeFromContent(content, s.metrics, s.skin);
}

void UiGroupPanel::PaintHeader(Draw& w, const HeaderLayout& layout, StyledState st) const
{
    const Style& s = GetEffectiveStyle();
    Size title_sz = title_.IsEmpty() ? Size(0, 0) : GetTextSize(title_, s.title_font);
    Size sub_sz = subtitle_.IsEmpty() ? Size(0, 0) : GetTextSize(subtitle_, s.subtitle_font);
    int text_h = title_sz.cy + (title_sz.cy && sub_sz.cy ? s.title_subtitle_gap : 0) + sub_sz.cy;
    Rect block = layout.title;
    if(block.IsEmpty())
        return;
    w.Clip(block);
    int x = block.left;
    if(!IsNull(icon_) && s.icon_size > 0) {
        int icon_size = min(s.icon_size, min(block.GetWidth(), block.GetHeight()));
        Rect ir = RectC(x, block.top + max(0, (block.GetHeight() - icon_size) / 2), icon_size, icon_size);
        UiPaintStyledIcon(w, ir, icon_, true, true, UiIconRenderMode::MonoTint, s.palette.icon[st], IsEnabled());
        x += icon_size + (text_h ? s.icon_gap : 0);
    }
    Color title_c = IsNull(s.title_color) ? s.palette.ink[st] : s.title_color;
    Color sub_c = IsNull(s.subtitle_color) ? Blend(s.palette.ink[st], SColorPaper(), 80) : s.subtitle_color;
    int ty = block.top + max(0, (block.GetHeight() - text_h) / 2);
    if(!title_.IsEmpty()) {
        w.DrawText(x, ty, title_, s.title_font, title_c);
        ty += title_sz.cy + (subtitle_.IsEmpty() ? 0 : s.title_subtitle_gap);
    }
    if(!subtitle_.IsEmpty())
        w.DrawText(x, ty, subtitle_, s.subtitle_font, sub_c);
    w.End();
}

void UiGroupPanel::PaintGroupFrame(Draw& w, const Rect& frame_rect,
                                   const HeaderLayout& header, StyledState st) const
{
    const Style& s = GetEffectiveStyle();
    if(frame_rect.IsEmpty())
        return;

    StyledMetrics metrics = s.metrics;
    metrics.content_margin = Rect(0, 0, 0, 0);
    StyledMetrics face_metrics = metrics;
    face_metrics.frame_enabled = false;
    UiPaintFaceFrameDash(w, frame_rect, s.palette, face_metrics, st);

    if(!metrics.frame_enabled || metrics.frame_width <= 0 || IsNull(s.palette.frame[st]))
        return;

    StyledMetrics frame_metrics = metrics;
    frame_metrics.face_enabled = false;
    if(s.header_mode != Center) {
        UiPaintFaceFrameDash(w, frame_rect, s.palette, frame_metrics, st);
        return;
    }

    bool horizontal = s.header_placement == UiAlign::TOP || s.header_placement == UiAlign::BOTTOM;
    int begin = horizontal ? frame_rect.left : frame_rect.top;
    int end = horizontal ? frame_rect.right : frame_rect.bottom;
    int gap_begin[2];
    int gap_end[2];
    int gap_count = 0;
    auto AddGap = [&](const Rect& occupied) {
        if(occupied.IsEmpty())
            return;
        int a = horizontal ? occupied.left : occupied.top;
        int b = horizontal ? occupied.right : occupied.bottom;
        a = max(begin, a - DPI(4));
        b = min(end, b + DPI(4));
        if(a < b && gap_count < 2) {
            gap_begin[gap_count] = a;
            gap_end[gap_count] = b;
            gap_count++;
        }
    };
    AddGap(header.title);
    AddGap(header.content);
    if(gap_count == 0) {
        UiPaintFaceFrameDash(w, frame_rect, s.palette, frame_metrics, st);
        return;
    }
    if(gap_count == 2 && gap_begin[1] < gap_begin[0]) {
        Swap(gap_begin[0], gap_begin[1]);
        Swap(gap_end[0], gap_end[1]);
    }
    if(gap_count == 2 && gap_begin[1] <= gap_end[0]) {
        gap_end[0] = max(gap_end[0], gap_end[1]);
        gap_count = 1;
    }

    int depth = max(1, metrics.frame_width) + max(0, metrics.radius) + DPI(2);
    Rect remainder = frame_rect;
    switch(s.header_placement) {
    case UiAlign::BOTTOM: remainder.bottom = max(remainder.top, remainder.bottom - depth); break;
    case UiAlign::LEFT:   remainder.left = min(remainder.right, remainder.left + depth); break;
    case UiAlign::RIGHT:  remainder.right = max(remainder.left, remainder.right - depth); break;
    case UiAlign::TOP:
    default:              remainder.top = min(remainder.bottom, remainder.top + depth); break;
    }
    if(!remainder.IsEmpty()) {
        w.Clip(remainder);
        UiPaintFaceFrameDash(w, frame_rect, s.palette, frame_metrics, st);
        w.End();
    }

    auto PaintEdgeSegment = [&](int a, int b) {
        if(a >= b)
            return;
        Rect clip;
        switch(s.header_placement) {
        case UiAlign::BOTTOM: clip = Rect(a, max(frame_rect.top, frame_rect.bottom - depth), b, frame_rect.bottom); break;
        case UiAlign::LEFT:   clip = Rect(frame_rect.left, a, min(frame_rect.right, frame_rect.left + depth), b); break;
        case UiAlign::RIGHT:  clip = Rect(max(frame_rect.left, frame_rect.right - depth), a, frame_rect.right, b); break;
        case UiAlign::TOP:
        default:              clip = Rect(a, frame_rect.top, b, min(frame_rect.bottom, frame_rect.top + depth)); break;
        }
        if(clip.IsEmpty())
            return;
        w.Clip(clip);
        UiPaintFaceFrameDash(w, frame_rect, s.palette, frame_metrics, st);
        w.End();
    };
    int cursor = begin;
    for(int i = 0; i < gap_count; i++) {
        PaintEdgeSegment(cursor, gap_begin[i]);
        cursor = gap_end[i];
    }
    PaintEdgeSegment(cursor, end);
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
    HeaderLayout header = ResolveHeaderLayout(face);

    Color frame = s.palette.frame[st];
    int th = max(1, s.separator_thickness);
    PaintGroupFrame(w, frame_rect, header, st);
    if(s.line_enabled) {
        if(s.header_placement == UiAlign::LEFT)
            w.DrawRect(header.header.right - th, header.header.top, th, header.header.GetHeight(), frame);
        else if(s.header_placement == UiAlign::RIGHT)
            w.DrawRect(header.header.left, header.header.top, th, header.header.GetHeight(), frame);
        else if(s.header_placement == UiAlign::BOTTOM)
            w.DrawRect(header.header.left, header.header.top, header.header.GetWidth(), th, frame);
        else
            w.DrawRect(header.header.left, header.header.bottom - th, header.header.GetWidth(), th, frame);
    }
    if(s.header_band_enabled) {
        if(s.palette.face[ST_HOT].IsSolid())
            w.DrawRect(header.header, s.palette.face[ST_HOT].color);
    }

    PaintHeader(w, header, st);
}

void UiGroupPanel::ChildRemoved(Ctrl *child)
{
    Ctrl::ChildRemoved(child);
    bool changed = false;
    if(child == content_) {
        content_ = nullptr;
        changed = true;
    }
    if(child == header_content_) {
        header_content_ = nullptr;
        changed = true;
    }
    if(changed) {
        RefreshLayout();
        Refresh();
    }
}

} // namespace Upp
