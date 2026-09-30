#include <Ui/UiTag.h>
#include <Ui/UiTheme.h>

namespace Upp {
namespace {

Color TagSolidFace(const StyledPalette& palette, int state, Color fallback)
{
    const UiFill& fill = palette.face[state];
    return fill.IsSolid() && !IsNull(fill.color) ? fill.color : fallback;
}

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

UiTagStyle ResolveTagVariant(UiTagStyle style, UiTagVariant variant,
                             const StyledPalette *parent)
{
    if(variant == UiTagVariant::Filled)
        return style;

    if(variant == UiTagVariant::Outline) {
        style.metrics.face_enabled = false;
        style.metrics.frame_enabled = true;
        style.metrics.frame_width = max(DPI(1), style.metrics.frame_width);
        for(int st = 0; st < 4; st++) {
            const Color ink = TagInk(style.palette, st, SColorText());
            style.palette.face[st] = UiFill::None();
            if(IsNull(style.palette.frame[st]))
                style.palette.frame[st] = ink;
        }
        return style;
    }

    // Compatibility behavior for the current implementation. The hardening
    // pass will replace this parent-colour blend with cached true-alpha face
    // preparation so Soft remains translucent over arbitrary media.
    style.metrics.frame_enabled = false;
    if(parent) {
        for(int st = 0; st < 4; st++) {
            const Color role_face =
                TagSolidFace(style.palette, st, SColorFace());
            const Color parent_face =
                TagSolidFace(*parent, st, SColorPaper());
            style.palette.face[st] =
                UiFill::Solid(Blend(role_face, parent_face, 150));
        }
    }
    return style;
}

UiAlign TagIconSide(UiAlign side)
{
    return side == UiAlign::RIGHT ? UiAlign::RIGHT : UiAlign::LEFT;
}

Size TagIconTarget(const Image& icon, Size box)
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
    return Size(max(1, (int)floor(source.cx * scale + 0.5)),
                max(1, (int)floor(source.cy * scale + 0.5)));
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
    out.skin = panel.skin;
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
    }

    return out;
}

Size UiMeasureTag(const UiTagData& data, const UiTagStyle& style, int max_width)
{
    if(!data.visible || (data.text.IsEmpty() && IsNull(data.icon)))
        return Size(0, 0);

    const Size icon_size =
        IsNull(data.icon) ? Size(0, 0)
                          : TagIconTarget(data.icon, style.icon_size);
    const int text_width = data.text.IsEmpty() ? 0
                         : GetTextSize(TagOneLine(data.text), style.font).cx;
    const int gap = icon_size.cx && text_width ? max(0, style.content_gap) : 0;
    const int height = max(icon_size.cy, style.font.GetCy());

    Size natural = UiStyledOuterSizeFromContent(
        Size(icon_size.cx + gap + text_width, height),
        style.metrics, style.skin);
    if(max_width != INT_MAX)
        natural.cx = min(natural.cx, max(0, max_width));
    return natural;
}

UiTagPresentation UiPrepareTag(const UiTagData& data,
                               const UiTagStyle& source_style,
                               const Rect& bounds,
                               const StyledPalette *parent_palette)
{
    UiTagPresentation out;
    if(!data.visible || bounds.IsEmpty()
       || (data.text.IsEmpty() && IsNull(data.icon)))
        return out;

    out.bounds = bounds;
    out.style = ResolveTagVariant(source_style, data.variant, parent_palette);
    out.enabled = data.enabled;
    out.visible = true;
    out.id = data.id;
    out.value = data.value;
    out.interactive = data.interactive;
    out.actionable = data.actionable;

    const Rect content =
        UiStyledInnerRect(bounds, out.style.metrics, out.style.skin);
    if(content.IsEmpty()) {
        out.visible = false;
        return out;
    }

    const bool have_icon = !IsNull(data.icon);
    const bool have_text = !data.text.IsEmpty();
    const Size desired_icon =
        have_icon ? TagIconTarget(data.icon, out.style.icon_size)
                  : Size(0, 0);
    const UiAlign icon_side = TagIconSide(out.style.icon_side);
    const int gap =
        have_icon && have_text ? max(0, out.style.content_gap) : 0;

    int text_left = content.left;
    int text_right = content.right;

    if(have_icon && desired_icon.cx > 0 && desired_icon.cy > 0) {
        const int side_w = min(desired_icon.cx, content.GetWidth());
        const int side_h = min(desired_icon.cy, content.GetHeight());
        Size actual(side_w, side_h);

        if(icon_side == UiAlign::RIGHT) {
            out.icon = RectC(content.right - actual.cx,
                             content.top + (content.GetHeight() - actual.cy) / 2,
                             actual.cx, actual.cy);
            text_right = max(content.left, out.icon.left - gap);
        }
        else {
            out.icon = RectC(content.left,
                             content.top + (content.GetHeight() - actual.cy) / 2,
                             actual.cx, actual.cy);
            text_left = min(content.right, out.icon.right + gap);
        }

        out.icon_image = PrepareTagIcon(data.icon, out.icon.GetSize());
        UiIconRenderMode mode = out.style.icon_render_mode;
        if(mode == UiIconRenderMode::Auto)
            mode = UiIconRenderMode::MonoTint;
        out.tint_icon = mode == UiIconRenderMode::MonoTint;
    }

    if(have_text && text_left < text_right) {
        out.prepared_text =
            TagEllipsize(data.text, out.style.font, text_right - text_left);
        if(!out.prepared_text.IsEmpty()) {
            const Size ts = GetTextSize(out.prepared_text, out.style.font);
            out.text = RectC(text_left,
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

    const StyledState st = tag.enabled ? state : ST_DISABLED;
    UiPaintStyledBackground(w, tag.bounds, tag.style.palette,
                            tag.style.metrics, tag.style.skin, st, false);

    const Color ink = TagInk(tag.style.palette, st, SColorText());
    if(!tag.icon.IsEmpty() && !IsNull(tag.icon_image)) {
        if(tag.tint_icon)
            w.DrawImage(tag.icon.left, tag.icon.top, tag.icon_image,
                        TagIconInk(tag.style.palette, st, ink));
        else
            w.DrawImage(tag.icon.left, tag.icon.top, tag.icon_image);
    }

    if(!tag.prepared_text.IsEmpty() && !tag.text.IsEmpty())
        w.DrawText(tag.text.left, tag.text.top,
                   tag.prepared_text, tag.style.font, ink);

    w.End();
}

} // namespace Upp
