#include <Ui/UiMediaCard.h>
#include <Ui/UiTheme.h>
#include <cmath>

namespace Upp {
namespace {

Color MediaInk(const Color colors[4], StyledState state, Color fallback)
{
    const Color c = colors[(int)state];
    return IsNull(c) ? fallback : c;
}

Color PaletteInk(const StyledPalette& palette,
                 StyledState state, Color fallback)
{
    const Color c = palette.ink[(int)state];
    return IsNull(c) ? fallback : c;
}

String OneLine(const String& text)
{
    String out = text;
    out.Replace("\r", " ");
    out.Replace("\n", " ");
    return out;
}

WString Ellipsize(const String& source, Font font, int width)
{
    if(source.IsEmpty() || width <= 0)
        return WString();

    WString text = OneLine(source).ToWString();
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

int RoleIndex(UiRole role)
{
    return clamp((int)role, 0, 3);
}

UiAlign NormalTagAlign(UiAlign align)
{
    return align == UiAlign::CENTER || align == UiAlign::RIGHT
         ? align : UiAlign::LEFT;
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

Size BandContentSize(const UiMediaBandData& data,
                     const UiMediaBandStyle& style)
{
    if(!data.HasContent())
        return Size(0, 0);

    int width = 0;
    int height = 0;
    int lines = 0;

    auto add = [&](const String& text, Font font) {
        if(text.IsEmpty())
            return;
        const Size size = GetTextSize(OneLine(text), font);
        width = max(width, size.cx);
        if(lines++)
            height += max(0, style.text_gap);
        height += size.cy;
    };

    add(data.title, style.title_font);
    add(data.subtitle, style.subtitle_font);
    add(data.metadata, style.metadata_font);
    return Size(width, height);
}

Size MeasureBand(const UiMediaBandData& data,
                 const UiMediaBandStyle& style)
{
    const Size content = BandContentSize(data, style);
    if(content.cx <= 0 && content.cy <= 0)
        return Size(0, 0);
    return UiStyledOuterSizeFromContent(content, style.metrics, style.skin);
}

UiMediaBandPresentation PrepareBand(const UiMediaBandData& data,
                                    const UiMediaBandStyle& style,
                                    const Rect& bounds)
{
    UiMediaBandPresentation out;
    if(!data.HasContent() || bounds.IsEmpty())
        return out;

    out.bounds = bounds;
    out.visible = true;

    const Rect content =
        UiStyledInnerRect(bounds, style.metrics, style.skin);
    if(content.IsEmpty()) {
        out.visible = false;
        return out;
    }

    struct Line {
        const String *source;
        Font font;
        Rect *rect;
        WString *prepared;
    };

    Line lines[] = {
        { &data.title, style.title_font,
          &out.title, &out.prepared_title },
        { &data.subtitle, style.subtitle_font,
          &out.subtitle, &out.prepared_subtitle },
        { &data.metadata, style.metadata_font,
          &out.metadata, &out.prepared_metadata },
    };

    int total_height = 0;
    int count = 0;
    for(const Line& line : lines) {
        if(line.source->IsEmpty())
            continue;
        if(count++)
            total_height += max(0, style.text_gap);
        total_height += line.font.GetCy();
    }

    int y = content.top
          + max(0, (content.GetHeight() - total_height) / 2);
    bool first = true;

    for(const Line& line : lines) {
        if(line.source->IsEmpty())
            continue;

        if(!first)
            y += max(0, style.text_gap);
        first = false;

        const int height =
            min(line.font.GetCy(), max(0, content.bottom - y));
        if(height <= 0)
            break;

        *line.rect = RectC(content.left, y, content.GetWidth(), height);
        *line.prepared =
            Ellipsize(*line.source, line.font, content.GetWidth());
        y += height;
    }

    return out;
}

void PaintBand(Draw& w,
               const UiMediaBandPresentation& p,
               const UiMediaBandStyle& style,
               StyledState state)
{
    if(!p.visible || p.bounds.IsEmpty())
        return;

    // Header/Footer are bounded structural regions. Their text must not escape
    // if a card is forced smaller than its natural minimum size.
    w.Clip(p.bounds);

    UiPaintStyledBackground(w, p.bounds,
                            style.palette, style.metrics, style.skin,
                            state, false);

    if(!p.prepared_title.IsEmpty() && !p.title.IsEmpty())
        w.DrawText(p.title.left, p.title.top,
                   p.prepared_title, style.title_font,
                   MediaInk(style.title_ink, state,
                            PaletteInk(style.palette, state, SColorText())));

    if(!p.prepared_subtitle.IsEmpty() && !p.subtitle.IsEmpty())
        w.DrawText(p.subtitle.left, p.subtitle.top,
                   p.prepared_subtitle, style.subtitle_font,
                   MediaInk(style.subtitle_ink, state,
                            PaletteInk(style.palette, state, SColorText())));

    if(!p.prepared_metadata.IsEmpty() && !p.metadata.IsEmpty())
        w.DrawText(p.metadata.left, p.metadata.top,
                   p.prepared_metadata, style.metadata_font,
                   MediaInk(style.metadata_ink, state,
                            PaletteInk(style.palette, state, SColorText())));

    w.End();
}

UiMediaBandStyle ResolveBandStyle(UiRole role)
{
    UiMediaBandStyle out;

    const UiPanel::Style panel = UiTheme::ResolvePanel(role);
    const UiLabel::Style title =
        UiTheme::ResolveLabel(role, UiTextSize::H3);
    const UiLabel::Style subtitle =
        UiTheme::ResolveLabel(UiRole::Subtle);
    const UiLabel::Style metadata =
        UiTheme::ResolveLabel(UiRole::Subtle, UiTextSize::H3);

    out.palette = panel.palette;
    out.metrics = panel.metrics;
    out.skin = panel.skin;

    // Bands are structural by default: text can float around a separately
    // bordered media region without creating another box.
    out.metrics.face_enabled = false;
    out.metrics.frame_enabled = false;
    out.metrics.focus_enabled = false;
    out.metrics.shadow.enabled = false;
    out.metrics.highlight.enabled = false;
    out.metrics.content_margin =
        Rect(DPI(2), DPI(2), DPI(2), DPI(2));

    out.title_font = title.font;
    out.title_font.Bold();
    out.subtitle_font = subtitle.font;
    out.metadata_font = metadata.font;

    for(int st = 0; st < 4; st++) {
        out.title_ink[st] =
            !IsNull(title.palette.ink[st])
            ? title.palette.ink[st] : panel.palette.ink[st];
        out.subtitle_ink[st] =
            !IsNull(subtitle.palette.ink[st])
            ? subtitle.palette.ink[st] : panel.palette.ink[st];
        out.metadata_ink[st] =
            !IsNull(metadata.palette.ink[st])
            ? metadata.palette.ink[st] : out.subtitle_ink[st];
    }

    return out;
}

Rect AlignInside(Rect area, Size size, UiAlign horizontal, UiAlign vertical)
{
    if(area.IsEmpty() || size.cx <= 0 || size.cy <= 0)
        return Rect();

    size.cx = min(size.cx, area.GetWidth());
    size.cy = min(size.cy, area.GetHeight());

    int x = area.left;
    if(horizontal == UiAlign::RIGHT)
        x = area.right - size.cx;
    else if(horizontal == UiAlign::CENTER)
        x = area.left + (area.GetWidth() - size.cx) / 2;

    int y = area.top;
    if(vertical == UiAlign::BOTTOM)
        y = area.bottom - size.cy;
    else if(vertical == UiAlign::CENTER)
        y = area.top + (area.GetHeight() - size.cy) / 2;

    return RectC(x, y, size.cx, size.cy);
}

void PrepareTagBand(const Vector<UiMediaCardTag>& source,
                    const UiMediaCard::Style& style,
                    const Rect& media,
                    bool top,
                    Vector<UiTagPresentation>& out)
{
    out.Clear();
    if(source.IsEmpty() || media.IsEmpty())
        return;

    const int inset = max(0, style.tag_inset);
    const int gap = max(0, style.tag_gap);
    const int top_y = media.top + inset;
    const int bottom_y = media.bottom - inset;

    int left = media.left + inset;
    int right = media.right - inset;

    Vector<int> centers;

    auto prepare_at = [&](int index, bool from_right) {
        const UiMediaCardTag& item = source[index];
        const int role = RoleIndex(item.tag.role);
        const int available = max(0, right - left);
        const Size size =
            UiMeasureTag(item.tag, style.tag_style[role], available);
        if(size.cx <= 0 || size.cy <= 0 || size.cx > available)
            return;

        const int x = from_right ? right - size.cx : left;
        const int y = top ? top_y : bottom_y - size.cy;
        const Rect box = RectC(x, y, size.cx, size.cy);

        UiTagPresentation prepared =
            UiPrepareTag(item.tag, style.tag_style[role],
                         box, &style.media_palette);
        if(!prepared.visible)
            return;

        out.Add(prepared);
        if(from_right)
            right = box.left - gap;
        else
            left = box.right + gap;
    };

    for(int i = 0; i < source.GetCount(); i++) {
        const UiAlign align = NormalTagAlign(source[i].align);
        if(align == UiAlign::LEFT)
            prepare_at(i, false);
        else if(align == UiAlign::CENTER)
            centers.Add(i);
    }

    for(int i = source.GetCount() - 1; i >= 0; i--)
        if(NormalTagAlign(source[i].align) == UiAlign::RIGHT)
            prepare_at(i, true);

    int total = 0;
    Vector<Size> center_sizes;
    center_sizes.SetCount(centers.GetCount());

    for(int i = 0; i < centers.GetCount(); i++) {
        const UiMediaCardTag& item = source[centers[i]];
        const int role = RoleIndex(item.tag.role);
        center_sizes[i] =
            UiMeasureTag(item.tag, style.tag_style[role],
                         max(0, right - left));
        if(center_sizes[i].cx <= 0 || center_sizes[i].cy <= 0) {
            total = INT_MAX;
            break;
        }
        if(i)
            total += gap;
        if(total < INT_MAX - center_sizes[i].cx)
            total += center_sizes[i].cx;
    }

    if(total <= right - left) {
        int x = left + ((right - left) - total) / 2;
        for(int i = 0; i < centers.GetCount(); i++) {
            const UiMediaCardTag& item = source[centers[i]];
            const int role = RoleIndex(item.tag.role);
            const Size size = center_sizes[i];
            const int y = top ? top_y : bottom_y - size.cy;
            const Rect box = RectC(x, y, size.cx, size.cy);
            UiTagPresentation prepared =
                UiPrepareTag(item.tag, style.tag_style[role],
                             box, &style.media_palette);
            if(prepared.visible)
                out.Add(prepared);
            x += size.cx + gap;
        }
    }
}

UiTagPresentation PrepareOverlay(const UiMediaOverlayData& overlay,
                                 const UiMediaCard::Style& style,
                                 const Rect& media)
{
    if(!overlay.visible || !overlay.content.visible || media.IsEmpty())
        return UiTagPresentation();

    const int role = RoleIndex(overlay.content.role);
    Rect area = media.Deflated(max(0, style.overlay_inset));
    if(area.IsEmpty())
        return UiTagPresentation();

    const Size size =
        UiMeasureTag(overlay.content, style.overlay_style[role],
                     max(0, area.GetWidth()));
    const Rect box =
        AlignInside(area, size, overlay.align_h, overlay.align_v);
    return UiPrepareTag(overlay.content,
                        style.overlay_style[role],
                        box, &style.media_palette);
}

int MediaContentRadius(const Rect& media,
                       const Rect& content,
                       int outer_radius)
{
    if(media.IsEmpty() || content.IsEmpty() || outer_radius <= 0)
        return 0;

    const int inset = max(
        max(max(0, content.left - media.left),
            max(0, media.right - content.right)),
        max(max(0, content.top - media.top),
            max(0, media.bottom - content.bottom)));

    return max(0, outer_radius - inset);
}

Image CachedRoundedMediaClip(const Image& image,
                             const Rect& image_target,
                             const Rect& clip_absolute,
                             int radius)
{
    if(IsNull(image) || image_target.IsEmpty() || clip_absolute.IsEmpty()
       || radius <= 0)
        return image;

    const Rect clip = RectC(clip_absolute.left - image_target.left,
                            clip_absolute.top - image_target.top,
                            clip_absolute.GetWidth(),
                            clip_absolute.GetHeight());

    return MakeImage(
        [&] {
            String key = "UiMediaCardRoundedClip";
            RawCat(key, image.GetSerialId());
            RawCat(key, clip.left);
            RawCat(key, clip.top);
            RawCat(key, clip.right);
            RawCat(key, clip.bottom);
            RawCat(key, radius);
            return key;
        },
        [&] {
            Image copy = image;
            ImageBuffer out(copy);
            const int w = out.GetWidth();
            const int h = out.GetHeight();
            const double r = min((double)radius,
                                 min(clip.GetWidth(), clip.GetHeight()) / 2.0);

            if(r <= 0.0)
                return Image(out);

            const double inner_left = clip.left + r;
            const double inner_right = clip.right - r;
            const double inner_top = clip.top + r;
            const double inner_bottom = clip.bottom - r;

            for(int y = 0; y < h; y++) {
                RGBA *row = out[y];
                const double py = y + 0.5;

                for(int x = 0; x < w; x++) {
                    const double px = x + 0.5;

                    if(px < clip.left || px >= clip.right
                       || py < clip.top || py >= clip.bottom) {
                        row[x].a = 0;
                        continue;
                    }

                    const double dx =
                        px < inner_left ? inner_left - px
                      : px > inner_right ? px - inner_right : 0.0;
                    const double dy =
                        py < inner_top ? inner_top - py
                      : py > inner_bottom ? py - inner_bottom : 0.0;

                    if(dx <= 0.0 && dy <= 0.0)
                        continue;

                    const double distance = std::sqrt(dx * dx + dy * dy);
                    const double coverage =
                        minmax(r + 0.5 - distance, 0.0, 1.0);

                    if(coverage <= 0.0)
                        row[x].a = 0;
                    else if(coverage < 1.0)
                        row[x].a = (byte)clamp(
                            (int)std::floor(row[x].a * coverage + 0.5),
                            0, 255);
                }
            }
            return Image(out);
        });
}

void PrepareMedia(const UiMediaCardData& data,
                  const UiMediaCard::Style& style,
                  UiMediaCardPresentation& out)
{
    out.media_image = Image();
    out.media_image_rect = Rect();
    out.empty_icon_rect = Rect();
    out.empty_text_rect = Rect();
    out.prepared_empty_text.Clear();

    if(out.media_content.IsEmpty())
        return;

    if(!IsNull(data.image)) {
        const UiMediaFitGeometry fit =
            UiComputeMediaFit(data.image.GetSize(),
                              out.media_content, style.media_fit);
        if(fit.IsValid()
           && fit.target.GetWidth() <= 4096
           && fit.target.GetHeight() <= 4096) {
            // Preserve the original Image identity in the cache key. Cropping
            // first creates a fresh Image on each layout pass and defeats
            // CachedRescale reuse.
            Image prepared =
                CachedRescale(data.image, fit.target.GetSize(), fit.source);

            const int radius =
                MediaContentRadius(out.media, out.media_content,
                                   style.media_metrics.radius);
            out.media_image =
                CachedRoundedMediaClip(prepared, fit.target,
                                       out.media_content, radius);
            out.media_image_rect = fit.target;
        }
        return;
    }

    const Image icon =
        !IsNull(data.empty_icon) ? data.empty_icon : data.fallback_icon;

    if(!IsNull(icon)) {
        const int side =
            min(style.empty_icon_size,
                min(out.media_content.GetWidth(),
                    out.media_content.GetHeight()));
        if(side > 0) {
            out.empty_icon_rect = RectC(
                out.media_content.left
                    + (out.media_content.GetWidth() - side) / 2,
                out.media_content.top
                    + (out.media_content.GetHeight() - side) / 2,
                side, side);
        }
    }

    if(!data.empty_text.IsEmpty()) {
        const Font font = style.footer_style.title_font;
        out.prepared_empty_text =
            Ellipsize(data.empty_text, font,
                      max(0, out.media_content.GetWidth() - DPI(12)));
        const Size ts = GetTextSize(out.prepared_empty_text, font);

        if(!out.empty_icon_rect.IsEmpty()) {
            const int y =
                min(out.media_content.bottom - ts.cy,
                    out.empty_icon_rect.bottom + DPI(5));
            out.empty_text_rect = RectC(
                out.media_content.left
                    + max(0, (out.media_content.GetWidth() - ts.cx) / 2),
                y, min(ts.cx, out.media_content.GetWidth()), ts.cy);
        }
        else {
            out.empty_text_rect = RectC(
                out.media_content.left
                    + max(0, (out.media_content.GetWidth() - ts.cx) / 2),
                out.media_content.top
                    + max(0, (out.media_content.GetHeight() - ts.cy) / 2),
                min(ts.cx, out.media_content.GetWidth()),
                min(ts.cy, out.media_content.GetHeight()));
        }
    }
}

void PaintMediaFrameOnTop(Draw& w,
                          const Rect& rect,
                          const UiMediaCard::Style& style,
                          StyledState state)
{
    if(rect.IsEmpty() || !style.media_metrics.frame_enabled)
        return;

    StyledMetrics metrics = style.media_metrics;
    metrics.face_enabled = false;
    metrics.focus_enabled = false;
    UiPaintStyledBackground(w, rect,
                            style.media_palette, metrics,
                            style.media_skin, state, false);
}

} // namespace

const UiMediaCard::Style& UiMediaCard::StyleDefault()
{
    static Style style;
    ONCELOCK {
        for(int st = 0; st < 4; st++) {
            style.palette.face[st] = UiFill::Solid(Color(250, 250, 251));
            style.palette.frame[st] = Color(214, 219, 226);
            style.palette.ink[st] = Color(24, 32, 43);

            style.media_palette.face[st] =
                UiFill::Solid(Color(234, 237, 241));
            style.media_palette.frame[st] = Color(190, 198, 208);
            style.media_palette.ink[st] = Color(91, 100, 112);
        }

        style.metrics.content_margin =
            Rect(DPI(3), DPI(3), DPI(3), DPI(3));
        style.metrics.face_enabled = false;
        style.metrics.frame_enabled = false;
        style.metrics.focus_enabled = true;
        style.metrics.radius = DPI(9);
        style.metrics.shadow.enabled = false;

        style.media_metrics.content_margin =
            Rect(DPI(1), DPI(1), DPI(1), DPI(1));
        style.media_metrics.face_enabled = true;
        style.media_metrics.frame_enabled = true;
        style.media_metrics.frame_width = DPI(1);
        style.media_metrics.radius = DPI(7);
        style.media_metrics.focus_enabled = false;
        style.media_metrics.shadow.enabled = false;

        style.header_style.metrics.face_enabled = false;
        style.header_style.metrics.frame_enabled = false;
        style.footer_style.metrics.face_enabled = false;
        style.footer_style.metrics.frame_enabled = false;

        for(int role = 0; role < 4; role++) {
            style.tag_style[role] = UiTagStyle();
            style.overlay_style[role] = UiTagStyle();
        }
    }
    return style;
}

UiMediaCard::Style UiResolveMediaCardStyle(UiRole role)
{
    UiMediaCard::Style out = UiMediaCard::StyleDefault();

    const UiPanel::Style card = UiTheme::ResolvePanel(role);
    const UiPanel::Style media = UiTheme::ResolvePanel(UiRole::Subtle);

    out.palette = card.palette;
    out.metrics = card.metrics;
    out.skin = card.skin;

    // Default card is intentionally transparent/frameless so a caller can have
    // only the media well boxed while header/footer text floats around it.
    out.metrics.content_margin =
        Rect(DPI(3), DPI(3), DPI(3), DPI(3));
    out.metrics.face_enabled = false;
    out.metrics.frame_enabled = false;
    out.metrics.focus_enabled = true;
    out.metrics.shadow.enabled = false;
    out.metrics.radius = max(DPI(8), out.metrics.radius);

    out.media_palette = media.palette;
    out.media_metrics = media.metrics;
    out.media_skin = media.skin;
    out.media_metrics.content_margin =
        Rect(DPI(1), DPI(1), DPI(1), DPI(1));
    out.media_metrics.face_enabled = true;
    out.media_metrics.frame_enabled = true;
    out.media_metrics.frame_width =
        max(DPI(1), out.media_metrics.frame_width);
    out.media_metrics.radius = DPI(7);
    out.media_metrics.focus_enabled = false;
    out.media_metrics.shadow.enabled = false;

    out.header_style = ResolveBandStyle(role);
    out.footer_style = ResolveBandStyle(role);

    for(int r = 0; r < 4; r++) {
        out.tag_style[r] = UiResolveTagStyle((UiRole)r);
        out.overlay_style[r] = UiResolveTagStyle((UiRole)r);
        out.overlay_style[r].metrics.content_margin =
            Rect(DPI(8), DPI(4), DPI(8), DPI(4));
        out.overlay_style[r].metrics.radius = DPI(6);
        Font f = UiTheme::ResolveLabel((UiRole)r, UiTextSize::H3).font;
        f.Bold();
        out.overlay_style[r].font = f;
    }

    const UiThemeContext context = UiTheme::GetContext();
    if(context.preset == UiThemePreset::Compact) {
        out.metrics.content_margin =
            Rect(DPI(2), DPI(2), DPI(2), DPI(2));
        out.section_gap = DPI(4);
        out.tag_gap = DPI(3);
        out.tag_inset = DPI(4);
        out.overlay_inset = DPI(6);
        out.empty_icon_size = DPI(24);
        out.min_media_extent = DPI(44);
    }

    return out;
}

Size UiMeasureMediaCard(const UiMediaCardData& data,
                        const UiMediaCard::Style& style)
{
    const Size header = MeasureBand(data.header, style.header_style);
    const Size footer = MeasureBand(data.footer, style.footer_style);

    int media_w = max(DPI(40), style.min_media_extent);
    int media_h = max(DPI(40), style.min_media_extent);
    if(style.media_aspect.cx > 0 && style.media_aspect.cy > 0)
        media_h = max(DPI(40),
            media_w * style.media_aspect.cy / max(1, style.media_aspect.cx));

    int width = max(media_w, max(header.cx, footer.cx));
    int height = media_h;
    if(header.cy > 0)
        height += header.cy + max(0, style.section_gap);
    if(footer.cy > 0)
        height += footer.cy + max(0, style.section_gap);

    return UiStyledOuterSizeFromContent(
        Size(width, height), style.metrics, style.skin);
}

UiMediaCardPresentation UiPrepareMediaCard(
    const UiMediaCardData& data,
    const UiMediaCard::Style& style,
    const Rect& bounds)
{
    UiMediaCardPresentation out;
    out.outer = bounds;
    out.content =
        UiStyledInnerRect(bounds, style.metrics, style.skin);

    if(out.content.IsEmpty())
        return out;

    Rect body = out.content;
    const Size header_size =
        MeasureBand(data.header, style.header_style);
    const Size footer_size =
        MeasureBand(data.footer, style.footer_style);

    if(header_size.cy > 0 && body.GetHeight() > 0) {
        const int h = min(header_size.cy, body.GetHeight());
        const Rect rect = RectC(body.left, body.top, body.GetWidth(), h);
        out.header = PrepareBand(data.header, style.header_style, rect);
        body.top += h;
        if(body.top < body.bottom)
            body.top = min(body.bottom,
                           body.top + max(0, style.section_gap));
    }

    if(footer_size.cy > 0 && body.GetHeight() > 0) {
        const int h = min(footer_size.cy, body.GetHeight());
        const Rect rect =
            RectC(body.left, body.bottom - h, body.GetWidth(), h);
        out.footer = PrepareBand(data.footer, style.footer_style, rect);
        body.bottom -= h;
        if(body.top < body.bottom)
            body.bottom = max(body.top,
                              body.bottom - max(0, style.section_gap));
    }

    out.media = FitAuthoredAspect(body, style.media_aspect);
    out.media_content =
        UiStyledInnerRect(out.media,
                          style.media_metrics, style.media_skin);

    PrepareMedia(data, style, out);
    PrepareTagBand(data.top_tags, style,
                   out.media_content, true, out.top_tags);
    PrepareTagBand(data.bottom_tags, style,
                   out.media_content, false, out.bottom_tags);
    out.overlay =
        PrepareOverlay(data.overlay, style, out.media_content);

    return out;
}

void UiPaintMediaCard(Draw& w,
                      const UiMediaCardData& data,
                      const UiMediaCard::Style& style,
                      const UiMediaCardPresentation& p,
                      StyledState state,
                      bool focused)
{
    if(p.outer.IsEmpty())
        return;

    // Paint the card surface first so authored shadows remain available, then
    // contain every structural/content layer to the actual card allocation.
    UiPaintStyledSurface(w, p.outer,
                         style.palette, style.metrics, style.skin,
                         state, focused, false, false);

    w.Clip(p.outer);

    PaintBand(w, p.header, style.header_style, state);
    PaintBand(w, p.footer, style.footer_style, state);

    if(!p.media.IsEmpty())
        UiPaintStyledBackground(w, p.media,
                                style.media_palette,
                                style.media_metrics,
                                style.media_skin,
                                state, false);

    // Rectangular Draw clipping contains all media children. Rounded image
    // containment is prepared into media_image alpha before Paint.
    if(!p.media.IsEmpty())
        w.Clip(p.media);

    if(!p.media_image.IsEmpty() && !p.media_image_rect.IsEmpty())
        w.DrawImage(p.media_image_rect.left,
                    p.media_image_rect.top,
                    p.media_image);
    else {
        const Image icon =
            !IsNull(data.empty_icon) ? data.empty_icon : data.fallback_icon;

        if(!IsNull(icon) && !p.empty_icon_rect.IsEmpty())
            w.DrawImage(p.empty_icon_rect, icon);

        if(!p.prepared_empty_text.IsEmpty()
           && !p.empty_text_rect.IsEmpty()) {
            const Color ink =
                PaletteInk(style.media_palette, state, SColorText());
            w.DrawText(p.empty_text_rect.left,
                       p.empty_text_rect.top,
                       p.prepared_empty_text,
                       style.footer_style.title_font,
                       ink);
        }
    }

    for(const UiTagPresentation& tag : p.top_tags)
        UiPaintTag(w, tag, state);
    for(const UiTagPresentation& tag : p.bottom_tags)
        UiPaintTag(w, tag, state);

    // Overlay is the final media-content layer and never consumes media space.
    UiPaintTag(w, p.overlay, state);

    if(!p.media.IsEmpty())
        w.End();

    PaintMediaFrameOnTop(w, p.media, style, state);
    w.End();
}

UiMediaCardData UiMakeMediaCardData(const UiItemRenderData& item)
{
    UiMediaCardData out;
    out.image = !IsNull(item.image) ? item.image : item.icon;
    out.footer.title = item.title;
    out.footer.subtitle =
        !item.subtitle.IsEmpty() ? item.subtitle : item.description;
    out.footer.metadata = item.right_text;
    out.enabled = item.enabled;
    out.value = item.value;
    out.data = item.data;
    return out;
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

void UiMediaCard::SyncThemeStyle()
{
    if(has_custom_style_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiResolveMediaCardStyle(role_);
    theme_revision_ = revision;
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

UiMediaCard& UiMediaCard::SetEmptyCue(const String& text, const Image& icon)
{
    data_.empty_text = text;
    data_.empty_icon = icon;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetHeader(const String& title,
                                    const String& subtitle,
                                    const String& metadata)
{
    data_.header.title = title;
    data_.header.subtitle = subtitle;
    data_.header.metadata = metadata;
    data_.header.visible = true;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::ClearHeader()
{
    data_.header.Clear();
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::ShowHeader(bool show)
{
    data_.header.visible = show;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetFooter(const String& title,
                                    const String& subtitle,
                                    const String& metadata)
{
    data_.footer.title = title;
    data_.footer.subtitle = subtitle;
    data_.footer.metadata = metadata;
    data_.footer.visible = true;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::ClearFooter()
{
    data_.footer.Clear();
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::ShowFooter(bool show)
{
    data_.footer.visible = show;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetTitle(const String& text)
{
    data_.footer.title = text;
    data_.footer.visible = true;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetSubTitle(const String& text)
{
    data_.footer.subtitle = text;
    data_.footer.visible = true;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetMetadata(const String& text)
{
    data_.footer.metadata = text;
    data_.footer.visible = true;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::ClearTopTags()
{
    data_.top_tags.Clear();
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::ClearBottomTags()
{
    data_.bottom_tags.Clear();
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::ClearTags()
{
    data_.top_tags.Clear();
    data_.bottom_tags.Clear();
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::AddTopTag(const UiTagData& tag, UiAlign align)
{
    UiMediaCardTag& item = data_.top_tags.Add();
    item.tag = tag;
    item.align = NormalTagAlign(align);
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::AddBottomTag(const UiTagData& tag, UiAlign align)
{
    UiMediaCardTag& item = data_.bottom_tags.Add();
    item.tag = tag;
    item.align = NormalTagAlign(align);
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::SetOverlay(const UiTagData& content,
                                     UiAlign horizontal,
                                     UiAlign vertical)
{
    data_.overlay.content = content;
    data_.overlay.align_h = horizontal;
    data_.overlay.align_v = vertical;
    data_.overlay.visible = true;
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::ClearOverlay()
{
    data_.overlay = UiMediaOverlayData();
    InvalidatePresentation();
    return *this;
}

UiMediaCard& UiMediaCard::ShowOverlay(bool show)
{
    data_.overlay.visible = show;
    InvalidatePresentation();
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
    RefreshLayout();
    Refresh();
}

Size UiMediaCard::GetMinSize() const
{
    return UiMeasureMediaCard(data_, GetEffectiveStyle());
}

void UiMediaCard::Layout()
{
    presentation_ =
        UiPrepareMediaCard(data_, GetEffectiveStyle(),
                           Rect(Point(0, 0), GetSize()));
}

StyledState UiMediaCard::ResolveState() const
{
    if(!IsEnabled() || !data_.enabled)
        return ST_DISABLED;
    if(pressed_ || selected_)
        return ST_PRESSED;
    if(hot_ || drop_hot_)
        return ST_HOT;
    return ST_NORMAL;
}

void UiMediaCard::Paint(Draw& w)
{
    UiPaintMediaCard(w, data_, GetEffectiveStyle(),
                     presentation_, ResolveState(),
                     HasFocus() && selectable_);
}

const UiTagPresentation* UiMediaCard::FindTagAt(Point p) const
{
    if(presentation_.overlay.visible
       && presentation_.overlay.bounds.Contains(p))
        return &presentation_.overlay;

    for(int i = presentation_.top_tags.GetCount() - 1; i >= 0; i--)
        if(presentation_.top_tags[i].bounds.Contains(p))
            return &presentation_.top_tags[i];

    for(int i = presentation_.bottom_tags.GetCount() - 1; i >= 0; i--)
        if(presentation_.bottom_tags[i].bounds.Contains(p))
            return &presentation_.bottom_tags[i];

    return nullptr;
}

void UiMediaCard::MouseEnter(Point p, dword flags)
{
    hot_ = IsEnabled() && data_.enabled;
    Refresh();
    Ctrl::MouseEnter(p, flags);
}

void UiMediaCard::MouseLeave()
{
    if(!pressed_ && !drop_hot_) {
        hot_ = false;
        Refresh();
    }
    Ctrl::MouseLeave();
}

void UiMediaCard::DragEnter()
{
    drop_hot_ = IsEnabled() && data_.enabled && (bool)WhenDrop;
    Refresh();
    Ctrl::DragEnter();
}

void UiMediaCard::DragAndDrop(Point p, PasteClip& d)
{
    if(!IsEnabled() || !data_.enabled || !WhenDrop) {
        d.Reject();
        drop_hot_ = false;
        Refresh();
        return;
    }

    drop_hot_ = true;
    WhenDrop(d);

    if(d.IsPaste())
        drop_hot_ = false;

    Refresh();
    Ctrl::DragAndDrop(p, d);
}

void UiMediaCard::DragLeave()
{
    if(drop_hot_) {
        drop_hot_ = false;
        Refresh();
    }
    Ctrl::DragLeave();
}

void UiMediaCard::LeftDown(Point p, dword flags)
{
    if(!IsEnabled() || !data_.enabled)
        return;

    String actionable_tag;
    if(const UiTagPresentation *tag = FindTagAt(p))
        if(tag->enabled && tag->actionable && !tag->id.IsEmpty())
            actionable_tag = tag->id;

    // A non-selectable card has no body activation/focus contract. Actionable
    // tags remain independently usable.
    if(!selectable_ && actionable_tag.IsEmpty())
        return;

    pressed_ = true;
    pressed_tag_id_ = actionable_tag;

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
    const String pressed_tag = pressed_tag_id_;
    pressed_ = false;
    pressed_tag_id_.Clear();

    if(HasCapture())
        ReleaseCapture();

    const bool can_activate =
        inside && IsEnabled() && data_.enabled;
    hot_ = can_activate;
    Refresh();

    // Enabled state is rechecked after capture teardown: a host can disable
    // the card during the press without receiving a stale activation.
    if(!can_activate) {
        Ctrl::LeftUp(p, flags);
        return;
    }

    if(!pressed_tag.IsEmpty()) {
        if(const UiTagPresentation *tag = FindTagAt(p)) {
            if(tag->enabled && tag->actionable
               && tag->id == pressed_tag) {
                const String id = tag->id;
                const Value value = tag->value;
                WhenTagAction(id, value);
                return;
            }
        }
    }

    if(selectable_ && pressed_tag.IsEmpty())
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
    pressed_tag_id_.Clear();
    hot_ = false;
    drop_hot_ = false;

    // Capture teardown owns ReleaseCapture(). Calling ReleaseCapture() here
    // recursively re-enters CancelMode() on Win32/U++.
    Refresh();
    Ctrl::CancelMode();
}

UiMediaCardRender::UiMediaCardRender()
{
}

UiMediaCardRender& UiMediaCardRender::SetCardStyle(
    const UiMediaCard::Style& style)
{
    custom_card_style_ = style;
    has_custom_card_style_ = true;
    InvalidateLayout();
    return *this;
}

UiMediaCardRender& UiMediaCardRender::ClearCardStyle()
{
    if(!has_custom_card_style_)
        return *this;

    has_custom_card_style_ = false;
    InvalidateLayout();
    return *this;
}

UiMediaCardRender& UiMediaCardRender::SetResolver(
    Function<void(const UiItemRenderData&, UiMediaCardData&)> resolver)
{
    resolver_ = resolver;
    InvalidateLayout();
    return *this;
}

UiMediaCardData UiMediaCardRender::ResolveCardData() const
{
    UiMediaCardData out = UiMakeMediaCardData(Data());
    if(resolver_)
        resolver_(Data(), out);
    return out;
}

UiMediaCard::Style UiMediaCardRender::ResolveCardStyle() const
{
    return has_custom_card_style_
         ? custom_card_style_
         : UiResolveMediaCardStyle(Data().role);
}

One<UiItemRender> UiMediaCardRender::Clone() const
{
    UiMediaCardRender *render = new UiMediaCardRender;
    One<UiItemRender> out = render;

    CopyConfigurationTo(*render);
    render->resolver_ = resolver_;

    if(has_custom_card_style_)
        render->SetCardStyle(custom_card_style_);

    return out;
}

Size UiMediaCardRender::GetContentSize() const
{
    const UiMediaCardData data = ResolveCardData();
    const UiMediaCard::Style style = ResolveCardStyle();
    return UiMeasureMediaCard(data, style);
}

Size UiMediaCardRender::GetMinSize() const
{
    const UiMediaCardData data = ResolveCardData();
    const UiMediaCard::Style style = ResolveCardStyle();
    return UiMeasureMediaCard(data, style);
}

void UiMediaCardRender::Layout()
{
    card_data_ = ResolveCardData();
    resolved_card_style_ = ResolveCardStyle();
    presentation_ =
        UiPrepareMediaCard(card_data_, resolved_card_style_, Bounds());
}

void UiMediaCardRender::Paint(
    Draw& w, const UiItemRenderState& state) const
{
    UiPaintMediaCard(w, card_data_, resolved_card_style_,
                     presentation_, ResolveStyledState(state),
                     state.focused);
}

UiItemRenderHit UiMediaCardRender::HitTest(Point p) const
{
    UiItemRenderHit hit;
    if(Bounds().Contains(p))
        hit.part = UIITEMPART_BODY;
    return hit;
}

} // namespace Upp
