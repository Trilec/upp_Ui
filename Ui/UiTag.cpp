#include <Ui/UiTag.h>
#include <Ui/UiTheme.h>

namespace Upp {
namespace {

Color TagInk(const StyledPalette& palette, int state, Color fallback)
{
    const Color c = palette.ink[state];
    return IsNull(c) ? fallback : c;
}

Color TagIconInk(const StyledPalette& palette, int state, Color fallback)
{
    const Color c = UiResolveIconColor(palette, (StyledState)state);
    return IsNull(c) ? fallback : c;
}

String TagOneLine(const String& text)
{
    String out = text;
    out.Replace("\r", " ");
    out.Replace("\n", " ");
    out.Replace("\t", " ");
    return out;
}

WString TagEllipsize(const String& source, Font font, int width)
{
    if(width <= 0 || source.IsEmpty())
        return WString();

    WString text = TagOneLine(source).ToWString();
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

UiTagStyle ResolveTagVariant(UiTagStyle style, UiTagVariant variant)
{
    // A tag never owns focus/capture and deliberately avoids independent
    // shadow/highlight machinery in the high-scale primitive.
    style.metrics.focus_enabled = false;
    style.metrics.shadow.enabled = false;
    style.metrics.highlight.enabled = false;

    for(int st = 0; st < 4; st++) {
        style.face_alpha[st] = clamp(style.face_alpha[st], 0, 255);
        style.frame_alpha[st] = clamp(style.frame_alpha[st], 0, 255);
    }
    style.soft_face_alpha = clamp(style.soft_face_alpha, 0, 255);

    if(variant == UiTagVariant::Soft) {
        style.metrics.frame_enabled = false;
        for(int st = 0; st < 4; st++)
            style.face_alpha[st] =
                min(style.face_alpha[st], style.soft_face_alpha);
    }
    else if(variant == UiTagVariant::Outline) {
        style.metrics.face_enabled = false;
        style.metrics.frame_enabled = true;
        style.metrics.frame_width = max(DPI(1), style.metrics.frame_width);
        for(int st = 0; st < 4; st++) {
            style.palette.face[st] = UiFill::None();
            if(IsNull(style.palette.frame[st]))
                style.palette.frame[st] =
                    TagInk(style.palette, st, SColorText());
        }
    }

    // UiTag deliberately stops at the common palette/metrics contract.
    // Nine-slice StyledSkin belongs to full controls; dense tags use UiFill
    // backgrounds (including image fill) prepared into the shared cache.
    return style;
}

UiAlign TagIconSide(UiAlign side)
{
    return side == UiAlign::RIGHT ? UiAlign::RIGHT : UiAlign::LEFT;
}

Size FitTagIcon(const Image& icon, Size box)
{
    if(IsNull(icon))
        return Size(0, 0);

    const Size source = icon.GetSize();
    if(source.cx <= 0 || source.cy <= 0)
        return Size(0, 0);

    if(box.cx <= 0 || box.cy <= 0)
        return source;

    const double scale =
        min((double)box.cx / source.cx, (double)box.cy / source.cy);
    return Size(max(1, fround(source.cx * scale)),
                max(1, fround(source.cy * scale)));
}

Image PrepareTagIcon(const Image& icon, Size target)
{
    if(IsNull(icon) || target.cx <= 0 || target.cy <= 0)
        return Image();

    if(icon.GetSize() == target)
        return icon;

    // Preserve source Image identity; do not create a throw-away crop before
    // entering the image cache.
    return CachedRescale(icon, target);
}

String TagImageIdentity(const Image& image)
{
    return IsNull(image) ? String()
                         : AsString((int64)image.GetSerialId());
}

Image PrepareTagDecoration(const UiTagStyle& style, Size size,
                           StyledState state)
{
    if(size.cx <= 0 || size.cy <= 0)
        return Image();

    StyledMetrics metrics = style.metrics;
    metrics.focus_enabled = false;
    metrics.shadow.enabled = false;
    metrics.highlight.enabled = false;

    const UiFill& fill = style.palette.face[state];
    const bool draw_face =
        metrics.face_enabled
        && style.face_alpha[state] > 0
        && !fill.IsNone()
        && (fill.IsSolid() || (fill.IsImage() && !IsNull(fill.image)));
    const bool draw_frame =
        metrics.frame_enabled
        && metrics.frame_width > 0
        && style.frame_alpha[state] > 0
        && !IsNull(style.palette.frame[state]);

    if(!draw_face && !draw_frame)
        return Image();

    UiRasterCachePolicy policy = UiRasterPolicyAA("tag/decoration");
    policy.allow_scale_from_bucket = false;
    policy.max_axis = 1024;
    policy.max_single_image_bytes = 4 * 1024 * 1024;

    UiRasterCacheKeyBuilder key("tag/decoration");
    key.Add(size)
       .Add((int)state)
       .Add(metrics.radius)
       .Add(metrics.frame_width)
       .Add(metrics.frame_enabled)
       .Add(metrics.face_enabled)
       .Add(metrics.dashed)
       .Add(metrics.dash_pattern)
       .Add(style.face_alpha[state])
       .Add(style.frame_alpha[state])
       .Add((int)fill.kind);

    if(fill.IsSolid())
        key.Add(fill.color);
    else if(fill.IsImage())
        key.Add(TagImageIdentity(fill.image));

    key.Add(style.palette.frame[state]);

    auto factory = [=] {
        ImageBuffer buffer(size);
        buffer.SetKind(IMAGE_ALPHA);
        Fill(~buffer, RGBAZero(), buffer.GetLength());

        BufferPainter painter(buffer, MODE_ANTIALIASED);

        Rect surface = UiStyledSurfaceRect(
            RectC(0, 0, size.cx, size.cy), metrics);
        if(surface.IsEmpty())
            return Image(buffer);

        const int frame_width =
            draw_frame ? max(0, metrics.frame_width) : 0;
        const double inset =
            frame_width > 0 ? max(0.5, frame_width * 0.5) : 0.5;
        const double x = surface.left + inset;
        const double y = surface.top + inset;
        const double width = surface.GetWidth() - 2 * inset;
        const double height = surface.GetHeight() - 2 * inset;
        const double radius =
            (double)min(max(0, metrics.radius),
                        min(surface.GetWidth(), surface.GetHeight()) / 2);

        if(width <= 0.0 || height <= 0.0)
            return Image(buffer);

        painter.Begin();
        if(radius > 0.0)
            painter.RoundedRectangle(x, y, width, height, radius);
        else
            painter.Rectangle(x, y, width, height);

        if(draw_face) {
            if(fill.IsSolid()) {
                painter.Fill(style.face_alpha[state] * fill.color);
            }
            else if(fill.IsImage()) {
                Size source = fill.image.GetSize();
                if(source.cx > 0 && source.cy > 0) {
                    painter.Opacity(style.face_alpha[state] / 255.0);
                    Xform2D transform =
                        Xform2D::Scale(width / source.cx,
                                      height / source.cy)
                        * Xform2D::Translation(x, y);
                    painter.Fill(fill.image, transform, FILL_FAST);
                    painter.Opacity(1.0);
                }
            }
        }

        if(draw_frame) {
            if(metrics.dashed && !metrics.dash_pattern.IsEmpty())
                painter.Dash(metrics.dash_pattern, 0.0);
            painter.Stroke(frame_width,
                           style.frame_alpha[state]
                           * style.palette.frame[state]);
        }

        painter.End();
        painter.Finish();
        return Image(buffer);
    };

    Size cached_size = UiQuantizeRasterSize(size, policy);
    if(cached_size.IsEmpty())
        return factory();

    return UiRasterCache::Get(key.Build(), policy, factory);
}

} // namespace

UiTagData::UiTagData()
    : role(UiRole::Standard)
{
}

UiTagData::UiTagData(const String& text_, UiRole role_, UiTagVariant variant_)
    : text(text_), role(role_), variant(variant_)
{
}

UiTagStyle UiResolveTagStyle(UiRole role)
{
    UiTagStyle out;
    const UiPanel::Style panel = UiTheme::ResolvePanel(role);
    const UiLabel::Style label =
        UiTheme::ResolveLabel(role, UiTextSize::H3);

    out.palette = panel.palette;
    out.metrics = panel.metrics;
    out.font = label.font;
    out.metrics.content_margin =
        Rect(DPI(5), DPI(2), DPI(5), DPI(2));
    out.metrics.radius = DPI(5);
    out.metrics.frame_width =
        max(DPI(1), out.metrics.frame_width);
    out.metrics.shadow.enabled = false;
    out.metrics.highlight.enabled = false;
    out.metrics.focus_enabled = false;

    for(int st = 0; st < 4; st++) {
        if(!IsNull(label.palette.ink[st]))
            out.palette.ink[st] = label.palette.ink[st];
        if(!IsNull(label.palette.icon[st]))
            out.palette.icon[st] = label.palette.icon[st];
        out.face_alpha[st] = 255;
        out.frame_alpha[st] = 255;
    }

    out.soft_face_alpha = 104;
    return out;
}

Size UiMeasureTag(const UiTagData& data, const UiTagStyle& source_style,
                  int max_width)
{
    if(!data.visible || (data.text.IsEmpty() && IsNull(data.icon)))
        return Size(0, 0);

    const UiTagStyle style =
        ResolveTagVariant(source_style, data.variant);

    const Size icon_size =
        IsNull(data.icon) ? Size(0, 0)
                          : FitTagIcon(data.icon, style.icon_size);
    const int text_width = data.text.IsEmpty() ? 0
                         : GetTextSize(TagOneLine(data.text), style.font).cx;
    const int gap = icon_size.cx && text_width
                  ? max(0, style.content_gap) : 0;
    const int height = max(icon_size.cy, style.font.GetCy());

    Size natural = UiStyledOuterSizeFromContent(
        Size(icon_size.cx + gap + text_width, height),
        style.metrics, StyledSkin());
    if(max_width != INT_MAX)
        natural.cx = min(natural.cx, max(0, max_width));
    return natural;
}

UiTagPresentation UiPrepareTag(const UiTagData& data,
                               const UiTagStyle& source_style,
                               const Rect& bounds,
                               const StyledPalette *parent_palette)
{
    (void)parent_palette; // retained only for source compatibility

    UiTagPresentation out;
    if(!data.visible || bounds.IsEmpty()
       || (data.text.IsEmpty() && IsNull(data.icon)))
        return out;

    const UiTagStyle style =
        ResolveTagVariant(source_style, data.variant);

    out.bounds = bounds;
    out.enabled = data.enabled;
    out.visible = true;
    out.id = data.id;
    out.value = data.value;
    out.interactive = data.IsInteractive();
    out.font = style.font;

    for(int st = 0; st < 4; st++) {
        out.ink[st] =
            TagInk(style.palette, st, SColorText());
        out.icon_ink[st] =
            TagIconInk(style.palette, st, out.ink[st]);
    }

    // Only prepare visual states this presentation can actually enter.
    // Passive enabled tags share their one normal decoration across all states;
    // disabled tags need only Disabled; interactive enabled tags need the three
    // pointer states. This matters when Graph/Gallery retains many tags.
    if(!out.enabled) {
        Image disabled = PrepareTagDecoration(
            style, bounds.GetSize(), ST_DISABLED);
        for(int st = 0; st < 4; st++)
            out.decoration[st] = disabled;
    }
    else if(out.interactive) {
        out.decoration[ST_NORMAL] =
            PrepareTagDecoration(style, bounds.GetSize(), ST_NORMAL);
        out.decoration[ST_HOT] =
            PrepareTagDecoration(style, bounds.GetSize(), ST_HOT);
        out.decoration[ST_PRESSED] =
            PrepareTagDecoration(style, bounds.GetSize(), ST_PRESSED);
        out.decoration[ST_DISABLED] = out.decoration[ST_NORMAL];
    }
    else {
        Image normal = PrepareTagDecoration(
            style, bounds.GetSize(), ST_NORMAL);
        for(int st = 0; st < 4; st++)
            out.decoration[st] = normal;
    }

    const Rect local =
        RectC(0, 0, bounds.GetWidth(), bounds.GetHeight());
    const Rect local_content =
        UiStyledInnerRect(local, style.metrics, StyledSkin());
    if(local_content.IsEmpty()) {
        out.visible = false;
        return out;
    }

    Rect content = local_content.Offseted(bounds.TopLeft());
    const bool have_icon = !IsNull(data.icon);
    const bool have_text = !data.text.IsEmpty();
    const UiAlign icon_side = TagIconSide(style.icon_side);
    const int gap =
        have_icon && have_text ? max(0, style.content_gap) : 0;

    int text_left = content.left;
    int text_right = content.right;

    if(have_icon) {
        Size box = style.icon_size;
        if(box.cx <= 0)
            box.cx = data.icon.GetWidth();
        if(box.cy <= 0)
            box.cy = data.icon.GetHeight();
        box.cx = min(max(0, box.cx), content.GetWidth());
        box.cy = min(max(0, box.cy), content.GetHeight());

        const Size actual = FitTagIcon(data.icon, box);
        if(actual.cx > 0 && actual.cy > 0) {
            if(icon_side == UiAlign::RIGHT) {
                out.icon = RectC(
                    content.right - actual.cx,
                    content.top + (content.GetHeight() - actual.cy) / 2,
                    actual.cx, actual.cy);
                text_right = max(content.left, out.icon.left - gap);
            }
            else {
                out.icon = RectC(
                    content.left,
                    content.top + (content.GetHeight() - actual.cy) / 2,
                    actual.cx, actual.cy);
                text_left = min(content.right, out.icon.right + gap);
            }

            out.icon_image =
                PrepareTagIcon(data.icon, out.icon.GetSize());

            UiIconRenderMode mode = style.icon_render_mode;
            if(mode == UiIconRenderMode::Auto)
                mode = UiIconRenderMode::MonoTint;
            out.tint_icon = mode == UiIconRenderMode::MonoTint;
        }
    }

    if(have_text && text_left < text_right) {
        out.prepared_text =
            TagEllipsize(data.text, style.font,
                         text_right - text_left);
        if(!out.prepared_text.IsEmpty()) {
            const Size ts =
                GetTextSize(out.prepared_text, style.font);
            out.text = RectC(
                text_left,
                content.top + max(0, (content.GetHeight() - ts.cy) / 2),
                min(ts.cx, text_right - text_left),
                min(ts.cy, content.GetHeight()));
        }
    }

    return out;
}

void UiPaintTag(Draw& w, const UiTagPresentation& tag, StyledState state)
{
    if(!tag.visible || tag.bounds.IsEmpty())
        return;

    // Authored geometry is also the paint containment boundary.
    w.Clip(tag.bounds);

    const StyledState st =
        tag.enabled ? state : ST_DISABLED;

    if(!IsNull(tag.decoration[st]))
        w.DrawImage(tag.bounds.left, tag.bounds.top,
                    tag.decoration[st]);

    if(!tag.icon.IsEmpty() && !IsNull(tag.icon_image)) {
        if(tag.tint_icon)
            w.DrawImage(tag.icon.left, tag.icon.top,
                        tag.icon_image, tag.icon_ink[st]);
        else
            w.DrawImage(tag.icon.left, tag.icon.top,
                        tag.icon_image);
    }

    if(!tag.prepared_text.IsEmpty() && !tag.text.IsEmpty())
        w.DrawText(tag.text.left, tag.text.top,
                   tag.prepared_text, tag.font, tag.ink[st]);

    w.End();
}

} // namespace Upp
