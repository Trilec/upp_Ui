#include <Ui/UiMediaCard.h>
#include <Ui/UiTheme.h>
#include <cmath>

namespace Upp {
namespace {

Color MediaCardInk(const Color colors[4], StyledState state, Color fallback)
{
    const Color c = colors[(int)state];
    return IsNull(c) ? fallback : c;
}

Color MediaCardPaletteInk(const StyledPalette& palette, StyledState state,
                          Color fallback)
{
    const Color c = palette.ink[(int)state];
    return IsNull(c) ? fallback : c;
}

String MediaCardOneLine(const String& text)
{
    String out = text;
    out.Replace("\r", " ");
    out.Replace("\n", " ");
    return out;
}

WString MediaCardEllipsize(const String& source, Font font, int width)
{
    if(source.IsEmpty() || width <= 0)
        return WString();

    WString text = MediaCardOneLine(source).ToWString();
    if(GetTextSize(text, font).cx <= width)
        return text;

    const WString ellipsis = "...";
    if(GetTextSize(ellipsis, font).cx > width)
        return WString();

    int lo = 0;
    int hi = text.GetCount();
    while(lo < hi) {
        const int mid = (lo + hi + 1) / 2;
        if(GetTextSize(text.Left(mid) + ellipsis, font).cx <= width)
            lo = mid;
        else
            hi = mid - 1;
    }
    return text.Left(lo) + ellipsis;
}

bool MediaCardLabelSide(UiAlign side)
{
    return side == UiAlign::TOP || side == UiAlign::BOTTOM
        || side == UiAlign::LEFT || side == UiAlign::RIGHT;
}

int MediaCardRoleIndex(UiRole role)
{
    return clamp((int)role, 0, 3);
}

Rect FitAuthoredAspect(const Rect& area, Size ratio)
{
    if(area.IsEmpty() || ratio.cx <= 0 || ratio.cy <= 0)
        return area;

    const double aspect = (double)ratio.cx / ratio.cy;
    int width = area.GetWidth();
    int height = max(1, (int)std::floor(width / aspect + 0.5));
    if(height > area.GetHeight()) {
        height = area.GetHeight();
        width = max(1, (int)std::floor(height * aspect + 0.5));
    }
    return RectC(area.left + (area.GetWidth() - width) / 2,
                 area.top + (area.GetHeight() - height) / 2,
                 width, height);
}

} // namespace

const UiMediaCard::Style& UiMediaCard::StyleDefault()
{
    static Style s;
    ONCELOCK {
        for(int st = 0; st < 4; st++) {
            s.palette.face[st] = UiFill::Solid(Color(250, 250, 251));
            s.palette.frame[st] = Color(214, 219, 226);
            s.palette.ink[st] = Color(24, 32, 43);
            s.media_palette.face[st] = UiFill::Solid(Color(234, 237, 241));
            s.media_palette.frame[st] = Color(214, 219, 226);
            s.media_palette.ink[st] = Color(91, 100, 112);
            s.title_ink[st] = Color(24, 32, 43);
            s.subtitle_ink[st] = Color(92, 101, 113);
            s.metadata_ink[st] = Color(118, 126, 138);
        }
        s.palette.face[ST_HOT] = UiFill::Solid(Color(246, 248, 251));
        s.palette.face[ST_PRESSED] = UiFill::Solid(Color(237, 242, 248));
        s.palette.face[ST_DISABLED] = UiFill::Solid(Color(247, 248, 250));
        s.palette.ink[ST_DISABLED] = Color(150, 157, 167);
        s.title_ink[ST_DISABLED] = s.subtitle_ink[ST_DISABLED]
                                  = s.metadata_ink[ST_DISABLED]
                                  = Color(150, 157, 167);

        s.metrics.content_margin = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
        s.metrics.radius = DPI(9);
        s.metrics.frame_width = DPI(1);
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.focus_enabled = true;
        s.metrics.shadow.enabled = false;

        s.media_metrics.content_margin = Rect(0, 0, 0, 0);
        s.media_metrics.radius = DPI(6);
        s.media_metrics.frame_width = DPI(1);
        s.media_metrics.face_enabled = true;
        s.media_metrics.frame_enabled = true;
        s.media_metrics.focus_enabled = false;
        s.media_metrics.shadow.enabled = false;

        for(int role = 0; role < 4; role++)
            s.badge_style[role] = UiBadgeStyle();
    }
    return s;
}

UiMediaCard::UiMediaCard()
    : role_(UiRole::Standard)
{
    BackPaint();
    WantFocus();
    SyncThemeStyle();
}

void UiMediaCard::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiMediaCard::Style& UiMediaCard::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

UiMediaCard::Style UiMediaCard::ResolveThemeStyle() const
{
    Style out = StyleDefault();

    const UiPanel::Style card = UiTheme::ResolvePanel(role_);
    const UiPanel::Style media = UiTheme::ResolvePanel(UiRole::Subtle);
    const UiLabel::Style title = UiTheme::ResolveLabel(role_, UiTextSize::H3);
    const UiLabel::Style subtitle = UiTheme::ResolveLabel(UiRole::Subtle);
    const UiLabel::Style metadata =
        UiTheme::ResolveLabel(UiRole::Subtle, UiTextSize::H3);

    out.palette = card.palette;
    out.metrics = card.metrics;
    out.skin = card.skin;
    out.metrics.content_margin = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
    out.metrics.radius = max(DPI(8), out.metrics.radius);
    out.metrics.frame_enabled = true;
    out.metrics.frame_width = max(DPI(1), out.metrics.frame_width);
    out.metrics.focus_enabled = true;
    out.metrics.shadow.enabled = false;

    out.media_palette = media.palette;
    out.media_metrics = media.metrics;
    out.media_skin = media.skin;
    out.media_metrics.content_margin = Rect(0, 0, 0, 0);
    out.media_metrics.radius = DPI(6);
    out.media_metrics.frame_enabled = true;
    out.media_metrics.frame_width = max(DPI(1), out.media_metrics.frame_width);
    out.media_metrics.focus_enabled = false;
    out.media_metrics.shadow.enabled = false;

    out.title_font = title.font;
    out.title_font.Bold();
    out.subtitle_font = subtitle.font;
    out.metadata_font = metadata.font;

    for(int st = 0; st < 4; st++) {
        out.title_ink[st] = !IsNull(title.palette.ink[st])
                          ? title.palette.ink[st] : card.palette.ink[st];
        out.subtitle_ink[st] = !IsNull(subtitle.palette.ink[st])
                             ? subtitle.palette.ink[st] : card.palette.ink[st];
        out.metadata_ink[st] = !IsNull(metadata.palette.ink[st])
                             ? metadata.palette.ink[st] : out.subtitle_ink[st];
    }

    for(int role = 0; role < 4; role++)
        out.badge_style[role] = UiResolveBadgeStyle((UiRole)role);

    const UiThemeContext context = UiTheme::GetContext();
    if(context.preset == UiThemePreset::Compact) {
        out.metrics.content_margin = Rect(DPI(4), DPI(4), DPI(4), DPI(4));
        out.media_text_gap = DPI(5);
        out.text_gap = DPI(1);
        out.badge_gap = DPI(3);
        out.badge_inset = DPI(4);
        out.empty_icon_size = DPI(24);
        out.min_media_extent = DPI(44);
    }
    return out;
}

void UiMediaCard::SyncThemeStyle()
{
    if(has_custom_style_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = ResolveThemeStyle();
    theme_revision_ = revision;
    presentation_dirty_ = true;
    RefreshLayout();
}

const UiMediaCard::Style& UiMediaCard::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiMediaCard *>(this)->SyncThemeStyle();
    return themed_style_;
}

UiMediaCard& UiMediaCard::SetCustomStyle(const Style& style)
{
    style_ = style;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiMediaCard& UiMediaCard::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;

    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiMediaCard::OnStyleChanged()
{
    BackPaint();
    InvalidatePresentation();
}

UiMediaCard& UiMediaCard::SetRole(UiRole role)
{
    if(role_ == role)
        return *this;

    role_ = role;
    if(!has_custom_style_)
        InvalidateStyleCache();
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetCardData(const UiMediaCardData& data)
{
    data_ = data;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetImage(const Image& image)
{
    data_.image = image;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::ClearImage()
{
    return SetImage(Image());
}

UiMediaCard& UiMediaCard::SetFallbackIcon(const Image& image)
{
    data_.fallback_icon = image;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetTitle(const String& text)
{
    data_.title = text;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetSubTitle(const String& text)
{
    data_.subtitle = text;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetMetadata(const String& text)
{
    data_.metadata = text;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetEmptyCue(const String& text, const Image& icon)
{
    data_.empty_text = text;
    data_.empty_icon = icon;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::ClearBadges()
{
    data_.badges.Clear();
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::AddBadge(const UiBadgeData& badge,
                                   UiMediaBadgeAnchor anchor)
{
    UiMediaCardBadge& item = data_.badges.Add();
    item.badge = badge;
    item.anchor = anchor;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetLabelSide(UiAlign side)
{
    if(side != UiAlign::DEFAULT && !MediaCardLabelSide(side))
        return *this;

    StyleEdit().label_side = side;
    OnStyleChanged();
    return *this;
}

UiMediaCard& UiMediaCard::SetMediaFit(UiMediaFit fit)
{
    StyleEdit().media_fit = fit;
    OnStyleChanged();
    return *this;
}

UiMediaCard& UiMediaCard::SetMediaAspect(Size ratio)
{
    if(ratio.cx <= 0 || ratio.cy <= 0)
        return *this;

    StyleEdit().media_aspect = ratio;
    OnStyleChanged();
    return *this;
}

UiMediaCard& UiMediaCard::SetSelected(bool selected)
{
    if(selected_ == selected)
        return *this;

    selected_ = selected;
    Refresh();
    return *this;
}

UiMediaCard& UiMediaCard::SetSelectable(bool selectable)
{
    selectable_ = selectable;
    if(selectable_)
        WantFocus();
    else {
        NoWantFocus();
        selected_ = false;
    }
    Refresh();
    return *this;
}

void UiMediaCard::InvalidatePresentation()
{
    presentation_dirty_ = true;
    RefreshLayout();
    Refresh();
}

Size UiMediaCard::MeasureTextBlock(const Style& style) const
{
    if(style.label_side == UiAlign::DEFAULT)
        return Size(0, 0);

    int width = 0;
    int height = 0;

    auto add = [&](const String& text, Font font) {
        if(text.IsEmpty())
            return;
        const Size ts = GetTextSize(MediaCardOneLine(text), font);
        width = max(width, ts.cx);
        if(height)
            height += max(0, style.text_gap);
        height += ts.cy;
    };

    add(data_.title, style.title_font);
    add(data_.subtitle, style.subtitle_font);
    add(data_.metadata, style.metadata_font);
    return Size(width, height);
}

Size UiMediaCard::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    const Size text = MeasureTextBlock(style);

    int media_width = max(DPI(40), style.min_media_extent);
    int media_height = max(DPI(40), style.min_media_extent);
    if(style.media_aspect.cx > 0 && style.media_aspect.cy > 0)
        media_height = max(DPI(40),
            media_width * style.media_aspect.cy / max(1, style.media_aspect.cx));

    Size content(media_width, media_height);
    if(text.cx > 0 || text.cy > 0) {
        const int gap = max(0, style.media_text_gap);
        if(style.label_side == UiAlign::TOP || style.label_side == UiAlign::BOTTOM) {
            content.cx = max(content.cx, min(text.cx, DPI(220)));
            content.cy += gap + text.cy;
        }
        else if(style.label_side == UiAlign::LEFT || style.label_side == UiAlign::RIGHT) {
            content.cx += gap + min(text.cx, DPI(120));
            content.cy = max(content.cy, text.cy);
        }
    }
    return UiStyledOuterSizeFromContent(content, style.metrics, style.skin);
}

void UiMediaCard::LayoutText(const Rect& rect, const Style& style)
{
    presentation_.title = Rect();
    presentation_.subtitle = Rect();
    presentation_.metadata = Rect();
    presentation_.prepared_title.Clear();
    presentation_.prepared_subtitle.Clear();
    presentation_.prepared_metadata.Clear();

    if(rect.IsEmpty())
        return;

    struct Line {
        const String *source;
        Font font;
        Rect *box;
        WString *prepared;
    };

    Line lines[] = {
        { &data_.title, style.title_font, &presentation_.title, &presentation_.prepared_title },
        { &data_.subtitle, style.subtitle_font, &presentation_.subtitle, &presentation_.prepared_subtitle },
        { &data_.metadata, style.metadata_font, &presentation_.metadata, &presentation_.prepared_metadata },
    };

    int total_height = 0;
    int visible_lines = 0;
    for(const Line& line : lines) {
        if(line.source->IsEmpty())
            continue;
        if(visible_lines++)
            total_height += max(0, style.text_gap);
        total_height += line.font.GetCy();
    }

    int y = rect.top + max(0, (rect.GetHeight() - total_height) / 2);
    bool first = true;
    for(const Line& line : lines) {
        if(line.source->IsEmpty())
            continue;

        if(!first)
            y += max(0, style.text_gap);
        first = false;

        const int height = min(line.font.GetCy(), max(0, rect.bottom - y));
        if(height <= 0)
            break;

        *line.box = RectC(rect.left, y, rect.GetWidth(), height);
        *line.prepared = MediaCardEllipsize(*line.source, line.font, rect.GetWidth());
        y += height;
    }
}

void UiMediaCard::PrepareMedia(const Style& style)
{
    presentation_.media_image = Image();
    presentation_.media_image_rect = Rect();
    presentation_.empty_icon_rect = Rect();
    presentation_.empty_text_rect = Rect();
    presentation_.prepared_empty_text.Clear();

    if(presentation_.media.IsEmpty())
        return;

    if(!IsNull(data_.image)) {
        const UiMediaFitGeometry fit =
            UiComputeMediaFit(data_.image.GetSize(), presentation_.media, style.media_fit);
        if(fit.IsValid() && fit.target.GetWidth() <= 4096 && fit.target.GetHeight() <= 4096) {
            const Rect full_source = RectC(0, 0, data_.image.GetWidth(), data_.image.GetHeight());
            Image source = fit.source == full_source ? data_.image : Crop(data_.image, fit.source);
            presentation_.media_image = CachedRescale(source, fit.target.GetSize());
            presentation_.media_image_rect = fit.target;
        }
        return;
    }

    const Image icon = !IsNull(data_.empty_icon) ? data_.empty_icon : data_.fallback_icon;
    if(!IsNull(icon)) {
        const int side = min(style.empty_icon_size,
                             min(presentation_.media.GetWidth(), presentation_.media.GetHeight()));
        if(side > 0) {
            presentation_.empty_icon_rect = RectC(
                presentation_.media.left + (presentation_.media.GetWidth() - side) / 2,
                presentation_.media.top + (presentation_.media.GetHeight() - side) / 2,
                side, side);
        }
    }

    if(!data_.empty_text.IsEmpty()) {
        presentation_.prepared_empty_text = MediaCardEllipsize(
            data_.empty_text, style.title_font, max(0, presentation_.media.GetWidth() - DPI(12)));

        const Size ts = GetTextSize(presentation_.prepared_empty_text, style.title_font);
        if(!presentation_.empty_icon_rect.IsEmpty()) {
            const int y = min(presentation_.media.bottom - ts.cy,
                              presentation_.empty_icon_rect.bottom + DPI(5));
            presentation_.empty_text_rect = RectC(
                presentation_.media.left + max(0, (presentation_.media.GetWidth() - ts.cx) / 2),
                y, min(ts.cx, presentation_.media.GetWidth()), ts.cy);
        }
        else {
            presentation_.empty_text_rect = RectC(
                presentation_.media.left + max(0, (presentation_.media.GetWidth() - ts.cx) / 2),
                presentation_.media.top + max(0, (presentation_.media.GetHeight() - ts.cy) / 2),
                min(ts.cx, presentation_.media.GetWidth()),
                min(ts.cy, presentation_.media.GetHeight()));
        }
    }
}

void UiMediaCard::PrepareBadges(const Style& style)
{
    presentation_.badges.Clear();
    presentation_.badge_source_indices.Clear();

    if(presentation_.media.IsEmpty())
        return;

    int top_left = presentation_.media.left + max(0, style.badge_inset);
    int top_right = presentation_.media.right - max(0, style.badge_inset);
    int bottom_left = top_left;
    int bottom_right = top_right;
    const int top_y = presentation_.media.top + max(0, style.badge_inset);
    const int bottom_y = presentation_.media.bottom - max(0, style.badge_inset);
    const int gap = max(0, style.badge_gap);

    for(int i = 0; i < data_.badges.GetCount(); i++) {
        const UiMediaCardBadge& entry = data_.badges[i];
        if(!entry.badge.visible)
            continue;

        const int role = MediaCardRoleIndex(entry.badge.role);
        const int max_width = max(0, presentation_.media.GetWidth() - style.badge_inset * 2);
        const Size size = UiMeasureBadge(entry.badge, style.badge_style[role], max_width);
        if(size.cx <= 0 || size.cy <= 0)
            continue;

        Rect box;
        switch(entry.anchor) {
        case UiMediaBadgeAnchor::TopLeft:
            if(top_left + size.cx > top_right) continue;
            box = RectC(top_left, top_y, size.cx, size.cy);
            top_left = box.right + gap;
            break;
        case UiMediaBadgeAnchor::TopRight:
            if(top_right - size.cx < top_left) continue;
            box = RectC(top_right - size.cx, top_y, size.cx, size.cy);
            top_right = box.left - gap;
            break;
        case UiMediaBadgeAnchor::BottomLeft:
            if(bottom_left + size.cx > bottom_right) continue;
            box = RectC(bottom_left, bottom_y - size.cy, size.cx, size.cy);
            bottom_left = box.right + gap;
            break;
        case UiMediaBadgeAnchor::BottomRight:
            if(bottom_right - size.cx < bottom_left) continue;
            box = RectC(bottom_right - size.cx, bottom_y - size.cy, size.cx, size.cy);
            bottom_right = box.left - gap;
            break;
        }

        UiBadgePresentation prepared = UiPrepareBadge(
            entry.badge, style.badge_style[role], box, &style.media_palette);
        if(prepared.visible) {
            presentation_.badges.Add(prepared);
            presentation_.badge_source_indices.Add(i);
        }
    }
}

void UiMediaCard::RebuildPresentation()
{
    presentation_ = UiMediaCardPresentation();

    const Style& style = GetEffectiveStyle();
    const Rect outer(Point(0, 0), GetSize());
    presentation_.outer = outer;
    presentation_.content = UiStyledInnerRect(outer, style.metrics, style.skin);

    if(presentation_.content.IsEmpty()) {
        presentation_dirty_ = false;
        return;
    }

    const Size natural_text = MeasureTextBlock(style);
    Rect media_area = presentation_.content;
    Rect text;
    const bool has_text = (natural_text.cx > 0 || natural_text.cy > 0)
                       && style.label_side != UiAlign::DEFAULT;

    if(has_text) {
        const int gap = max(0, style.media_text_gap);

        if(style.label_side == UiAlign::TOP || style.label_side == UiAlign::BOTTOM) {
            const int available = max(0, presentation_.content.GetHeight()
                - min(style.min_media_extent, presentation_.content.GetHeight()));
            const int reserve = min(natural_text.cy, available);
            if(reserve > 0) {
                if(style.label_side == UiAlign::TOP) {
                    text = RectC(presentation_.content.left, presentation_.content.top,
                                 presentation_.content.GetWidth(), reserve);
                    media_area.top = min(presentation_.content.bottom, text.bottom + gap);
                }
                else {
                    text = RectC(presentation_.content.left, presentation_.content.bottom - reserve,
                                 presentation_.content.GetWidth(), reserve);
                    media_area.bottom = max(presentation_.content.top, text.top - gap);
                }
            }
        }
        else if(style.label_side == UiAlign::LEFT || style.label_side == UiAlign::RIGHT) {
            const int available = max(0, presentation_.content.GetWidth()
                - min(style.min_media_extent, presentation_.content.GetWidth()));
            const int reserve = min(natural_text.cx,
                min(available, presentation_.content.GetWidth() * 45 / 100));
            if(reserve > 0) {
                if(style.label_side == UiAlign::LEFT) {
                    text = RectC(presentation_.content.left, presentation_.content.top,
                                 reserve, presentation_.content.GetHeight());
                    media_area.left = min(presentation_.content.right, text.right + gap);
                }
                else {
                    text = RectC(presentation_.content.right - reserve, presentation_.content.top,
                                 reserve, presentation_.content.GetHeight());
                    media_area.right = max(presentation_.content.left, text.left - gap);
                }
            }
        }
    }

    presentation_.media = FitAuthoredAspect(media_area, style.media_aspect);
    presentation_.text = text;
    LayoutText(text, style);
    PrepareMedia(style);
    PrepareBadges(style);
    presentation_dirty_ = false;
}

void UiMediaCard::Layout()
{
    RebuildPresentation();
}

StyledState UiMediaCard::ResolveState() const
{
    if(!IsEnabled() || !data_.enabled)
        return ST_DISABLED;
    if(pressed_ || selected_)
        return ST_PRESSED;
    if(hot_)
        return ST_HOT;
    return ST_NORMAL;
}

void UiMediaCard::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    const StyledState state = ResolveState();
    const bool focused = HasFocus() && selectable_;

    UiPaintStyledSurface(w, presentation_.outer,
                         style.palette, style.metrics, style.skin,
                         state, focused, false, false);

    if(!presentation_.media.IsEmpty())
        UiPaintStyledBackground(w, presentation_.media,
                                style.media_palette, style.media_metrics,
                                style.media_skin, state, false);

    if(!presentation_.media_image.IsEmpty() && !presentation_.media_image_rect.IsEmpty()) {
        w.DrawImage(presentation_.media_image_rect.left,
                    presentation_.media_image_rect.top,
                    presentation_.media_image);
    }
    else {
        const Image icon = !IsNull(data_.empty_icon) ? data_.empty_icon : data_.fallback_icon;
        if(!IsNull(icon) && !presentation_.empty_icon_rect.IsEmpty())
            w.DrawImage(presentation_.empty_icon_rect, icon);

        if(!presentation_.prepared_empty_text.IsEmpty()
           && !presentation_.empty_text_rect.IsEmpty()) {
            const Color ink = MediaCardPaletteInk(style.media_palette, state, SColorText());
            w.DrawText(presentation_.empty_text_rect.left,
                       presentation_.empty_text_rect.top,
                       presentation_.prepared_empty_text, style.title_font, ink);
        }
    }

    for(const UiBadgePresentation& badge : presentation_.badges)
        UiPaintBadge(w, badge, state);

    if(!presentation_.prepared_title.IsEmpty() && !presentation_.title.IsEmpty())
        w.DrawText(presentation_.title.left, presentation_.title.top,
                   presentation_.prepared_title, style.title_font,
                   MediaCardInk(style.title_ink, state,
                       MediaCardPaletteInk(style.palette, state, SColorText())));

    if(!presentation_.prepared_subtitle.IsEmpty() && !presentation_.subtitle.IsEmpty())
        w.DrawText(presentation_.subtitle.left, presentation_.subtitle.top,
                   presentation_.prepared_subtitle, style.subtitle_font,
                   MediaCardInk(style.subtitle_ink, state,
                       MediaCardPaletteInk(style.palette, state, SColorText())));

    if(!presentation_.prepared_metadata.IsEmpty() && !presentation_.metadata.IsEmpty())
        w.DrawText(presentation_.metadata.left, presentation_.metadata.top,
                   presentation_.prepared_metadata, style.metadata_font,
                   MediaCardInk(style.metadata_ink, state,
                       MediaCardPaletteInk(style.palette, state, SColorText())));
}

int UiMediaCard::HitTestBadge(Point p) const
{
    for(int i = presentation_.badges.GetCount() - 1; i >= 0; i--)
        if(presentation_.badges[i].bounds.Contains(p))
            return presentation_.badge_source_indices[i];
    return -1;
}

void UiMediaCard::MouseEnter(Point p, dword flags)
{
    hot_ = true;
    Refresh();
    Ctrl::MouseEnter(p, flags);
}

void UiMediaCard::MouseLeave()
{
    if(!pressed_) {
        hot_ = false;
        Refresh();
    }
    Ctrl::MouseLeave();
}

void UiMediaCard::LeftDown(Point p, dword flags)
{
    if(!IsEnabled() || !data_.enabled)
        return;

    pressed_ = true;
    pressed_badge_ = HitTestBadge(p);

    if(selectable_)
        SetFocus();

    SetCapture();
    Refresh();
    Ctrl::LeftDown(p, flags);
}

void UiMediaCard::LeftUp(Point p, dword flags)
{
    if(!pressed_)
        return;

    const bool inside = Rect(Point(0, 0), GetSize()).Contains(p);
    const int release_badge = HitTestBadge(p);
    const int pressed_badge = pressed_badge_;

    pressed_ = false;
    pressed_badge_ = -1;
    if(HasCapture())
        ReleaseCapture();
    Refresh();

    if(inside && pressed_badge >= 0
       && pressed_badge == release_badge
       && pressed_badge < data_.badges.GetCount()) {
        const UiBadgeData& badge = data_.badges[pressed_badge].badge;
        if(badge.enabled && badge.actionable) {
            const String id = badge.id;
            const Value value = badge.value;
            WhenBadgeAction(id, value);
            return;
        }
    }

    if(inside && pressed_badge < 0)
        WhenAction();

    Ctrl::LeftUp(p, flags);
}

bool UiMediaCard::Key(dword key, int count)
{
    if(IsEnabled() && data_.enabled && selectable_
       && (key == K_ENTER || key == K_SPACE)) {
        WhenAction();
        return true;
    }
    return Ctrl::Key(key, count);
}

void UiMediaCard::GotFocus()
{
    Refresh();
    Ctrl::GotFocus();
}

void UiMediaCard::LostFocus()
{
    Refresh();
    Ctrl::LostFocus();
}

void UiMediaCard::CancelMode()
{
    pressed_ = false;
    pressed_badge_ = -1;
    if(HasCapture())
        ReleaseCapture();
    Refresh();
    Ctrl::CancelMode();
}

} // namespace Upp
