#include <Ui/UiItemRender.h>
#include <Ui/UiList.h>
#include <Ui/UiTheme.h>

namespace Upp {

namespace {

UiItemRenderStyle BuildItemRenderStyle(UiRole role)
{
    UiList::Style list = UiTheme::ResolveList(UiTheme::GetContext(), role);
    UiItemRenderStyle out;

    out.palette = list.palette;
    out.palette.face[ST_HOT] = UiFill::Solid(list.hot_face);
    out.palette.frame[ST_HOT] = list.hot_frame;
    out.palette.ink[ST_HOT] = list.hot_ink;
    out.palette.icon[ST_HOT] = list.hot_ink;
    out.palette.face[ST_PRESSED] = UiFill::Solid(list.selected_face);
    out.palette.frame[ST_PRESSED] = list.selected_frame;
    out.palette.ink[ST_PRESSED] = list.selected_ink;
    out.palette.icon[ST_PRESSED] = list.selected_ink;
    out.palette.ink[ST_DISABLED] = list.disabled_ink;
    out.palette.icon[ST_DISABLED] = list.disabled_ink;

    out.metrics = list.metrics;
    out.metrics.radius = list.row_radius;
    out.metrics.content_margin = Rect(list.h_padding, list.v_padding,
                                      list.h_padding, list.v_padding);
    out.metrics.shadow.enabled = false;
    out.metrics.focus_enabled = false;
    out.skin = list.skin;

    out.title_font = list.font;
    out.subtitle_font = list.font;
    out.subtitle_font.Height(max(DPI(9), list.font.GetHeight() - DPI(1)));
    out.description_font = out.subtitle_font;
    out.right_font = list.font;
    out.icon_size = list.icon_size;
    out.check_size = list.check_size;
    out.content_gap = list.content_gap;
    out.text_gap = DPI(2);
    out.metadata_size = list.metadata_size;
    out.metadata_gap = list.metadata_gap;
    out.muted_ink = list.muted_ink;
    out.metadata_default = list.metadata_default;
    out.check_frame = list.check_frame;
    out.check_fill = list.check_fill;
    return out;
}

void DrawAlignedItemText(Draw& w, const Rect& rect, const String& text,
                         Font font, Color ink, int align)
{
    if(rect.IsEmpty() || text.IsEmpty())
        return;
    Size sz = GetTextSize(text, font);
    int x = rect.left;
    if(align == ALIGN_RIGHT)
        x = max(rect.left, rect.right - sz.cx);
    else if(align == ALIGN_CENTER)
        x = max(rect.left, rect.left + (rect.GetWidth() - sz.cx) / 2);
    int y = rect.top + max(0, (rect.GetHeight() - sz.cy) / 2);
    w.Clip(rect);
    w.DrawText(x, y, text, font, ink);
    w.End();
}

void DrawItemCheck(Draw& w, const Rect& rect, const UiItemRenderStyle& style,
                   const UiItemRenderState& state, bool checked)
{
    if(rect.IsEmpty())
        return;

    StyledPalette p;
    StyledMetrics m;
    m.face_enabled = true;
    m.frame_enabled = true;
    m.frame_width = DPI(1);
    m.radius = DPI(3);
    for(int i = 0; i < 4; i++) {
        p.face[i] = UiFill::Solid(White());
        p.frame[i] = style.check_frame;
        p.ink[i] = style.check_fill;
        p.icon[i] = style.check_fill;
    }
    if(state.selected) {
        for(int i = 0; i < 4; i++)
            p.frame[i] = style.palette.frame[ST_PRESSED];
    }
    if(checked) {
        Color fill = state.selected ? style.palette.frame[ST_PRESSED] : style.check_fill;
        for(int i = 0; i < 4; i++)
            p.face[i] = UiFill::Solid(fill);
    }
    UiPaintFaceFrameDash(w, rect, p, m, state.enabled ? ST_NORMAL : ST_DISABLED);

    if(checked) {
        int pad = max(2, rect.GetWidth() / 5);
        int x1 = rect.left + pad;
        int y1 = rect.top + rect.GetHeight() / 2;
        int x2 = rect.left + rect.GetWidth() / 2 - 1;
        int y2 = rect.bottom - pad - 1;
        int x3 = rect.right - pad - 1;
        int y3 = rect.top + pad;
        w.DrawLine(x1, y1, x2, y2, 2, White());
        w.DrawLine(x2, y2, x3, y3, 2, White());
    }
}

Color ResolveItemInk(const UiItemRenderData& data, const UiItemRenderStyle& style,
                     StyledState state)
{
    return IsNull(data.custom_ink_color) ? style.palette.ink[state]
                                         : data.custom_ink_color;
}

Font ResolveItemFont(const UiItemRenderData& data, const UiItemRenderStyle& style)
{
    Font f = data.use_custom_font ? data.custom_font : style.title_font;
    if(data.emphasized && !data.use_custom_font)
        f.Bold();
    return f;
}

Font ShrinkVerticalItemFont(Font font, int steps)
{
    if(steps <= 0)
        return font;
    font.Height(max(DPI(8), font.GetHeight() - DPI(steps)));
    return font;
}

} // namespace

UiItemRenderData::UiItemRenderData()
    : role(UiRole::Standard)
{
}

UiItemRenderData::UiItemRenderData(const UiItemRenderData& src)
{
    *this = src;
}

UiItemRenderData& UiItemRenderData::operator=(const UiItemRenderData& src)
{
    title = src.title;
    subtitle = src.subtitle;
    description = src.description;
    right_text = src.right_text;
    image = src.image;
    icon = src.icon;
    icon_render_mode = src.icon_render_mode;
    value = src.value;
    data = src.data;
    role = src.role;
    enabled = src.enabled;
    has_check = src.has_check;
    checked = src.checked;
    emphasized = src.emphasized;
    has_metadata = src.has_metadata;
    metadata_color = src.metadata_color;
    custom_ink_color = src.custom_ink_color;
    use_custom_font = src.use_custom_font;
    custom_font = src.custom_font;
    underline = src.underline;
    underline_color = src.underline_color;
    text_align = src.text_align;
    right_text_align = src.right_text_align;
    return *this;
}

UiItemRenderData UiMakeItemRenderData(const UiModelItem& item)
{
    UiItemRenderData data;
    data.title = item.text;
    data.description = item.description;
    data.right_text = item.right_text;
    data.image = item.image;
    data.icon = item.icon;
    data.icon_render_mode = item.icon_render_mode;
    data.data = item.data;
    data.enabled = item.enabled;
    data.has_check = item.has_check;
    data.checked = item.checked;
    data.emphasized = item.group_header;
    data.has_metadata = item.has_metadata;
    data.metadata_color = item.metadata_color;
    data.custom_ink_color = item.custom_ink_color;
    data.use_custom_font = item.use_custom_font;
    data.custom_font = item.custom_font;
    data.underline = item.underline;
    data.underline_color = item.underline_color;
    data.text_align = item.text_align;
    data.right_text_align = item.right_text_align;
    return data;
}

UiItemRender::UiItemRender()
{
    themed_style_ = BuildItemRenderStyle(UiRole::Standard);
    custom_style_ = themed_style_;
}

UiItemRender& UiItemRender::SetData(const UiItemRenderData& data)
{
    data_ = data;
    InvalidateLayout();
    return *this;
}

UiItemRender& UiItemRender::SetCustomStyle(const UiItemRenderStyle& style)
{
    custom_style_ = style;
    has_custom_style_ = true;
    InvalidateLayout();
    return *this;
}

UiItemRender& UiItemRender::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
    theme_revision_ = 0;
    themed_role_ = -1;
    InvalidateLayout();
    return *this;
}

const UiItemRenderStyle& UiItemRender::GetStyle() const
{
    const_cast<UiItemRender*>(this)->SyncThemeStyle();
    return EffectiveStyle();
}

void UiItemRender::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    int role = (int)data_.role;
    if(theme_revision_ == revision && themed_role_ == role)
        return;
    themed_style_ = BuildItemRenderStyle(data_.role);
    theme_revision_ = revision;
    themed_role_ = role;
    InvalidateLayout();
}

const UiItemRenderStyle& UiItemRender::EffectiveStyle() const
{
    return has_custom_style_ ? custom_style_ : themed_style_;
}

bool UiItemRender::PrepareLayout(const Rect& rect, UiDirection direction)
{
    SyncThemeStyle();
    if(bounds_ != rect || direction_ != direction) {
        bounds_ = rect;
        direction_ = direction;
        layout_dirty_ = true;
    }
    if(!layout_dirty_)
        return false;
    Layout();
    layout_dirty_ = false;
    layout_serial_++;
    return true;
}

void UiItemRender::InvalidateLayout()
{
    layout_dirty_ = true;
}

UiItemRenderHit UiItemRender::HitTest(Point p) const
{
    UiItemRenderHit hit;
    if(bounds_.Contains(p))
        hit.part = UIITEMPART_BODY;
    return hit;
}

void UiItemRender::CopyConfigurationTo(UiItemRender& target) const
{
    if(has_custom_style_)
        target.SetCustomStyle(custom_style_);
    else
        target.ClearCustomStyle();
}

StyledState UiItemRender::ResolveStyledState(const UiItemRenderState& state) const
{
    if(!state.enabled || !data_.enabled)
        return ST_DISABLED;
    if(state.selected || state.pressed)
        return ST_PRESSED;
    if(state.hot)
        return ST_HOT;
    return ST_NORMAL;
}

UiItemRenderBasic::UiItemRenderBasic()
{
}

One<UiItemRender> UiItemRenderBasic::Clone() const
{
    One<UiItemRender> out = new UiItemRenderBasic;
    CopyConfigurationTo(*out);
    return out;
}

Size UiItemRenderBasic::GetContentSize() const
{
    const UiItemRenderData& data = Data();
    const UiItemRenderStyle& style = GetStyle();
    Font title_font = ResolveItemFont(data, style);
    Size title = GetTextSize(data.title, title_font);
    Size subtitle = data.subtitle.IsEmpty() ? Size(0, 0) : GetTextSize(data.subtitle, style.subtitle_font);
    Size description = data.description.IsEmpty() ? Size(0, 0) : GetTextSize(data.description, style.description_font);
    Size right = data.right_text.IsEmpty() ? Size(0, 0) : GetTextSize(data.right_text, style.right_font);
    int media = (!IsNull(data.icon) && style.show_icon) ? style.icon_size : 0;
    if(data.has_check)
        media += style.check_size + (media ? style.content_gap : 0);
    if(data.has_metadata)
        media += style.metadata_size + (media ? style.metadata_gap : 0);

    if(Direction() == UiDirection::V) {
        int w = max(max(title.cx, subtitle.cx), max(description.cx, right.cx));
        int h = title.cy;
        if(subtitle.cy) h += style.text_gap + subtitle.cy;
        if(description.cy) h += style.text_gap + description.cy;
        if(right.cy) h += style.text_gap + right.cy;
        if(media) h += media + style.content_gap;
        return UiStyledOuterSizeFromContent(Size(max(w, media), h), style.metrics, style.skin);
    }

    int text_h = title.cy + (subtitle.cy ? style.text_gap + subtitle.cy : 0)
                           + (description.cy ? style.text_gap + description.cy : 0);
    int w = media + (media && title.cx ? style.content_gap : 0) + title.cx;
    if(right.cx)
        w += style.content_gap + right.cx;
    return UiStyledOuterSizeFromContent(Size(w, max(media, text_h)), style.metrics, style.skin);
}

Size UiItemRenderBasic::GetMinSize() const
{
    const UiItemRenderStyle& style = GetStyle();
    int h = max(style.title_font.GetHeight() + DPI(4), max(style.icon_size, style.check_size));
    return UiStyledOuterSizeFromContent(Size(DPI(48), h), style.metrics, style.skin);
}

void UiItemRenderBasic::Layout()
{
    const UiItemRenderData& data = Data();
    const UiItemRenderStyle& style = EffectiveStyle();
    content_ = UiStyledInnerRect(Bounds(), style.metrics, style.skin);
    check_ = icon_ = metadata_ = title_ = subtitle_ = description_ = right_text_ = Rect(0, 0, 0, 0);
    if(content_.IsEmpty())
        return;

    Font title_font = ResolveItemFont(data, style);
    int title_h = max(title_font.GetHeight() + DPI(2), DPI(14));
    int subtitle_h = !data.subtitle.IsEmpty() && style.show_subtitle
                   ? max(style.subtitle_font.GetHeight() + DPI(2), DPI(12)) : 0;
    int description_h = !data.description.IsEmpty() && style.show_description
                      ? max(style.description_font.GetHeight() + DPI(2), DPI(12)) : 0;

    if(Direction() == UiDirection::V) {
        int top = content_.top;
        int media = 0;
        if(data.has_check)
            media = max(media, style.check_size);
        if(style.show_icon && !IsNull(data.icon))
            media = max(media, style.icon_size);
        if(style.show_metadata && data.has_metadata)
            media = max(media, style.metadata_size);
        if(media > 0) {
            int x = content_.left + (content_.GetWidth() - media) / 2;
            if(data.has_check)
                check_ = RectC(x, top, media, media);
            else if(style.show_icon && !IsNull(data.icon))
                icon_ = RectC(x, top, media, media);
            else
                metadata_ = RectC(x, top, media, media);
            top += media + style.content_gap;
        }
        title_ = Rect(content_.left, top, content_.right, min(content_.bottom, top + title_h));
        top = title_.bottom;
        if(subtitle_h && top < content_.bottom) {
            top += style.text_gap;
            subtitle_ = Rect(content_.left, top, content_.right, min(content_.bottom, top + subtitle_h));
            top = subtitle_.bottom;
        }
        if(description_h && top < content_.bottom) {
            top += style.text_gap;
            description_ = Rect(content_.left, top, content_.right, min(content_.bottom, top + description_h));
        }
        if(style.show_right_text && !data.right_text.IsEmpty()) {
            int rh = max(style.right_font.GetHeight() + DPI(2), DPI(12));
            right_text_ = Rect(content_.left, max(content_.top, content_.bottom - rh),
                               content_.right, content_.bottom);
        }
        return;
    }

    int left = content_.left;
    int right = content_.right;
    int cy = content_.GetHeight();
    if(data.has_check) {
        int side = min(style.check_size, cy);
        check_ = RectC(left, content_.top + (cy - side) / 2, side, side);
        left = check_.right + style.content_gap;
    }
    if(style.show_icon && !IsNull(data.icon) && left < right) {
        int side = min(style.icon_size, cy);
        icon_ = RectC(left, content_.top + (cy - side) / 2, side, side);
        left = icon_.right + style.content_gap;
    }
    if(style.show_metadata && data.has_metadata && left < right) {
        int side = min(style.metadata_size, cy);
        metadata_ = RectC(left, content_.top + (cy - side) / 2, side, side);
        left = metadata_.right + style.metadata_gap;
    }
    if(style.show_right_text && !data.right_text.IsEmpty()) {
        int rw = min(GetTextSize(data.right_text, style.right_font).cx + DPI(4),
                     max(0, (right - left) / 2));
        right_text_ = Rect(max(left, right - rw), content_.top, right, content_.bottom);
        right = max(left, right_text_.left - style.content_gap);
    }

    int total_text_h = title_h;
    if(subtitle_h) total_text_h += style.text_gap + subtitle_h;
    if(description_h) total_text_h += style.text_gap + description_h;
    int top = content_.top + max(0, (content_.GetHeight() - total_text_h) / 2);
    title_ = Rect(left, top, right, min(content_.bottom, top + title_h));
    top = title_.bottom;
    if(subtitle_h && top < content_.bottom) {
        top += style.text_gap;
        subtitle_ = Rect(left, top, right, min(content_.bottom, top + subtitle_h));
        top = subtitle_.bottom;
    }
    if(description_h && top < content_.bottom) {
        top += style.text_gap;
        description_ = Rect(left, top, right, min(content_.bottom, top + description_h));
    }
}

void UiItemRenderBasic::Paint(Draw& w, const UiItemRenderState& state) const
{
    const UiItemRenderData& data = Data();
    const UiItemRenderStyle& style = EffectiveStyle();
    StyledState st = ResolveStyledState(state);
    UiItemRenderState actual = state;
    actual.enabled = actual.enabled && data.enabled;

    if(style.show_face)
        UiPaintStyledSurface(w, Bounds(), style.palette, style.metrics, style.skin,
                             st, state.focused, false, false);

    if(!check_.IsEmpty())
        DrawItemCheck(w, check_, style, actual, data.checked);
    if(!icon_.IsEmpty()) {
        Color ink = ResolveItemInk(data, style, st);
        UiPaintStyledIcon(w, icon_, data.icon, true, true,
                          data.icon_render_mode, ink, actual.enabled);
    }
    if(!metadata_.IsEmpty())
        w.DrawRect(metadata_, IsNull(data.metadata_color) ? style.metadata_default : data.metadata_color);

    Font title_font = ResolveItemFont(data, style);
    Color ink = ResolveItemInk(data, style, st);
    DrawAlignedItemText(w, title_, data.title, title_font, ink, data.text_align);
    if(!subtitle_.IsEmpty())
        DrawAlignedItemText(w, subtitle_, data.subtitle, style.subtitle_font,
                            actual.enabled ? style.muted_ink : style.palette.ink[ST_DISABLED], data.text_align);
    if(!description_.IsEmpty())
        DrawAlignedItemText(w, description_, data.description, style.description_font,
                            actual.enabled ? style.muted_ink : style.palette.ink[ST_DISABLED], data.text_align);
    if(!right_text_.IsEmpty())
        DrawAlignedItemText(w, right_text_, data.right_text, style.right_font,
                            actual.enabled ? style.muted_ink : style.palette.ink[ST_DISABLED], data.right_text_align);

    if(data.underline && !title_.IsEmpty()) {
        Size tsz = GetTextSize(data.title, title_font);
        int x = title_.left;
        if(data.text_align == ALIGN_RIGHT)
            x = max(title_.left, title_.right - tsz.cx);
        else if(data.text_align == ALIGN_CENTER)
            x = max(title_.left, title_.left + (title_.GetWidth() - tsz.cx) / 2);
        int y = min(title_.bottom - 1, title_.top + max(0, (title_.GetHeight() - tsz.cy) / 2) + tsz.cy);
        w.DrawRect(x, y, min(title_.right - x, tsz.cx), 1,
                   IsNull(data.underline_color) ? ink : data.underline_color);
    }
}

UiItemRenderHit UiItemRenderBasic::HitTest(Point p) const
{
    UiItemRenderHit hit;
    if(check_.Contains(p)) hit.part = UIITEMPART_CHECK;
    else if(icon_.Contains(p)) hit.part = UIITEMPART_ICON;
    else if(metadata_.Contains(p)) hit.part = UIITEMPART_METADATA;
    else if(title_.Contains(p)) hit.part = UIITEMPART_TITLE;
    else if(subtitle_.Contains(p)) hit.part = UIITEMPART_SUBTITLE;
    else if(description_.Contains(p)) hit.part = UIITEMPART_DESCRIPTION;
    else if(right_text_.Contains(p)) hit.part = UIITEMPART_RIGHT_TEXT;
    else if(Bounds().Contains(p)) hit.part = UIITEMPART_BODY;
    return hit;
}

UiItemRenderImage::UiItemRenderImage()
{
}

One<UiItemRender> UiItemRenderImage::Clone() const
{
    One<UiItemRender> out = new UiItemRenderImage;
    CopyConfigurationTo(*out);
    return out;
}

Size UiItemRenderImage::GetContentSize() const
{
    const UiItemRenderData& data = Data();
    const UiItemRenderStyle& style = GetStyle();
    Font title_font = ResolveItemFont(data, style);
    Size title = GetTextSize(data.title, title_font);
    Size desc = data.description.IsEmpty() ? Size(0, 0) : GetTextSize(data.description, style.description_font);
    if(Direction() == UiDirection::H) {
        int h = max(style.image_extent, title.cy + (desc.cy ? style.text_gap + desc.cy : 0));
        return UiStyledOuterSizeFromContent(Size(style.image_extent + style.content_gap + max(title.cx, desc.cx), h),
                                            style.metrics, style.skin);
    }
    int h = style.image_extent + style.content_gap + title.cy + (desc.cy ? style.text_gap + desc.cy : 0);
    return UiStyledOuterSizeFromContent(Size(max(style.image_extent, max(title.cx, desc.cx)), h),
                                        style.metrics, style.skin);
}

Size UiItemRenderImage::GetMinSize() const
{
    const UiItemRenderStyle& style = GetStyle();
    return UiStyledOuterSizeFromContent(Size(DPI(56), DPI(56)), style.metrics, style.skin);
}

void UiItemRenderImage::Layout()
{
    const UiItemRenderData& data = Data();
    const UiItemRenderStyle& style = EffectiveStyle();
    content_ = UiStyledInnerRect(Bounds(), style.metrics, style.skin);
    media_ = title_ = subtitle_ = description_ = right_text_ = metadata_ = Rect(0, 0, 0, 0);

    title_font_ = ResolveItemFont(data, style);
    subtitle_font_ = style.subtitle_font;
    description_font_ = style.description_font;
    right_font_ = style.right_font;

    if(content_.IsEmpty())
        return;

    bool vertical = Direction() == UiDirection::V;
    if(vertical) {
        int shrink = 0;
        if(content_.GetWidth() < DPI(50) || content_.GetHeight() < DPI(52))
            shrink = 3;
        else if(content_.GetWidth() < DPI(64) || content_.GetHeight() < DPI(68))
            shrink = 2;
        else if(content_.GetWidth() < DPI(78) || content_.GetHeight() < DPI(84))
            shrink = 1;

        // Explicit model fonts are semantic caller choices. Theme-provided fonts
        // may step down with compact vertical tiles, while the caller's custom
        // title font remains untouched.
        if(!data.use_custom_font)
            title_font_ = ShrinkVerticalItemFont(title_font_, shrink);
        subtitle_font_ = ShrinkVerticalItemFont(subtitle_font_, shrink);
        description_font_ = ShrinkVerticalItemFont(description_font_, shrink);
        right_font_ = ShrinkVerticalItemFont(right_font_, shrink);
    }

    int title_h = max(title_font_.GetHeight() + DPI(2), vertical ? DPI(10) : DPI(14));
    int subtitle_h = !data.subtitle.IsEmpty() && style.show_subtitle
                   ? max(subtitle_font_.GetHeight() + DPI(2), vertical ? DPI(9) : DPI(12)) : 0;
    int description_h = !data.description.IsEmpty() && style.show_description
                      ? max(description_font_.GetHeight() + DPI(2), vertical ? DPI(9) : DPI(12)) : 0;

    if(!vertical) {
        int side = min(style.image_extent, min(content_.GetHeight(), max(DPI(16), content_.GetWidth() / 3)));
        media_ = RectC(content_.left, content_.top + (content_.GetHeight() - side) / 2, side, side);
        int left = min(content_.right, media_.right + style.content_gap);
        int right = content_.right;
        if(style.show_right_text && !data.right_text.IsEmpty()) {
            int rw = min(GetTextSize(data.right_text, right_font_).cx + DPI(4),
                         max(0, (right - left) / 2));
            right_text_ = Rect(max(left, right - rw), content_.top, right, content_.bottom);
            right = max(left, right_text_.left - style.content_gap);
        }
        int text_h = title_h + (subtitle_h ? style.text_gap + subtitle_h : 0)
                              + (description_h ? style.text_gap + description_h : 0);
        int top = content_.top + max(0, (content_.GetHeight() - text_h) / 2);
        title_ = Rect(left, top, right, min(content_.bottom, top + title_h));
        top = title_.bottom;
        if(subtitle_h && top < content_.bottom) {
            top += style.text_gap;
            subtitle_ = Rect(left, top, right, min(content_.bottom, top + subtitle_h));
            top = subtitle_.bottom;
        }
        if(description_h && top < content_.bottom) {
            top += style.text_gap;
            description_ = Rect(left, top, right, min(content_.bottom, top + description_h));
        }
    }
    else {
        int text_h = title_h + (subtitle_h ? style.text_gap + subtitle_h : 0)
                              + (description_h ? style.text_gap + description_h : 0);
        int media_h = max(0, content_.GetHeight() - text_h - style.content_gap);
        media_h = min(media_h, max(style.image_extent, content_.GetHeight() * 3 / 5));
        media_ = Rect(content_.left, content_.top, content_.right, content_.top + media_h);
        int top = min(content_.bottom, media_.bottom + style.content_gap);
        title_ = Rect(content_.left, top, content_.right, min(content_.bottom, top + title_h));
        top = title_.bottom;
        if(subtitle_h && top < content_.bottom) {
            top += style.text_gap;
            subtitle_ = Rect(content_.left, top, content_.right, min(content_.bottom, top + subtitle_h));
            top = subtitle_.bottom;
        }
        if(description_h && top < content_.bottom) {
            top += style.text_gap;
            description_ = Rect(content_.left, top, content_.right, min(content_.bottom, top + description_h));
        }
    }

    if(style.show_metadata && data.has_metadata) {
        int side = min(style.metadata_size, min(content_.GetWidth(), content_.GetHeight()));
        metadata_ = RectC(content_.right - side, content_.top, side, side);
    }
}

void UiItemRenderImage::Paint(Draw& w, const UiItemRenderState& state) const
{
    const UiItemRenderData& data = Data();
    const UiItemRenderStyle& style = EffectiveStyle();
    StyledState st = ResolveStyledState(state);
    bool enabled = state.enabled && data.enabled;

    if(style.show_face)
        UiPaintStyledSurface(w, Bounds(), style.palette, style.metrics, style.skin,
                             st, state.focused, false, false);

    Image media = !IsNull(data.image) ? data.image : data.icon;
    if(!media_.IsEmpty() && !IsNull(media)) {
        UiIconRenderMode mode = !IsNull(data.image) ? UiIconRenderMode::PreserveColor
                                                    : data.icon_render_mode;
        Color ink = ResolveItemInk(data, style, st);
        UiPaintStyledIcon(w, media_, media, true, true, mode, ink, enabled);
    }
    if(!metadata_.IsEmpty())
        w.DrawRect(metadata_, IsNull(data.metadata_color) ? style.metadata_default : data.metadata_color);

    Color ink = ResolveItemInk(data, style, st);
    int align = Direction() == UiDirection::V ? ALIGN_CENTER : data.text_align;
    DrawAlignedItemText(w, title_, data.title, title_font_, ink, align);
    if(!subtitle_.IsEmpty())
        DrawAlignedItemText(w, subtitle_, data.subtitle, subtitle_font_,
                            enabled ? style.muted_ink : style.palette.ink[ST_DISABLED], align);
    if(!description_.IsEmpty())
        DrawAlignedItemText(w, description_, data.description, description_font_,
                            enabled ? style.muted_ink : style.palette.ink[ST_DISABLED], align);
    if(!right_text_.IsEmpty())
        DrawAlignedItemText(w, right_text_, data.right_text, right_font_,
                            enabled ? style.muted_ink : style.palette.ink[ST_DISABLED], data.right_text_align);
}

UiItemRenderHit UiItemRenderImage::HitTest(Point p) const
{
    UiItemRenderHit hit;
    if(media_.Contains(p)) hit.part = UIITEMPART_IMAGE;
    else if(metadata_.Contains(p)) hit.part = UIITEMPART_METADATA;
    else if(title_.Contains(p)) hit.part = UIITEMPART_TITLE;
    else if(subtitle_.Contains(p)) hit.part = UIITEMPART_SUBTITLE;
    else if(description_.Contains(p)) hit.part = UIITEMPART_DESCRIPTION;
    else if(right_text_.Contains(p)) hit.part = UIITEMPART_RIGHT_TEXT;
    else if(Bounds().Contains(p)) hit.part = UIITEMPART_BODY;
    return hit;
}

} // namespace Upp
