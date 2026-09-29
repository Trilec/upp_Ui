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
            style.palette.frame[st] = ink;
        }
        return style;
    }

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

    for(int st = 0; st < 4; st++)
        if(!IsNull(label.palette.ink[st]))
            out.palette.ink[st] = label.palette.ink[st];

    return out;
}

Size UiMeasureTag(const UiTagData& data, const UiTagStyle& style, int max_width)
{
    if(!data.visible || (data.text.IsEmpty() && IsNull(data.icon)))
        return Size(0, 0);

    const int icon = IsNull(data.icon) ? 0 : max(0, style.icon_size);
    const int text = data.text.IsEmpty() ? 0
                   : GetTextSize(TagOneLine(data.text), style.font).cx;
    const int gap = icon && text ? max(0, style.content_gap) : 0;
    const int height = max(icon, style.font.GetCy());

    Size natural = UiStyledOuterSizeFromContent(
        Size(icon + gap + text, height), style.metrics, style.skin);
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
    out.icon_image = data.icon;
    out.id = data.id;
    out.value = data.value;
    out.actionable = data.actionable;

    const Rect content =
        UiStyledInnerRect(bounds, out.style.metrics, out.style.skin);
    if(content.IsEmpty()) {
        out.visible = false;
        return out;
    }

    int left = content.left;
    if(!IsNull(data.icon) && out.style.icon_size > 0) {
        const int side = min(out.style.icon_size,
                             min(content.GetWidth(), content.GetHeight()));
        out.icon = RectC(left,
                         content.top + (content.GetHeight() - side) / 2,
                         side, side);
        left = out.icon.right;
        if(!data.text.IsEmpty())
            left += max(0, out.style.content_gap);
    }

    if(!data.text.IsEmpty() && left < content.right) {
        out.prepared_text =
            TagEllipsize(data.text, out.style.font, content.right - left);
        if(!out.prepared_text.IsEmpty()) {
            const Size ts = GetTextSize(out.prepared_text, out.style.font);
            out.text = RectC(left,
                             content.top + max(0, (content.GetHeight() - ts.cy) / 2),
                             min(ts.cx, content.right - left),
                             min(ts.cy, content.GetHeight()));
        }
    }
    return out;
}

void UiPaintTag(Draw& w, const UiTagPresentation& tag, StyledState state)
{
    if(!tag.visible || tag.bounds.IsEmpty())
        return;

    // Tags can be prepared into very small host regions. Clip the complete
    // presentation so text/icons never bleed into adjacent media/header/footer.
    w.Clip(tag.bounds);

    const StyledState st = tag.enabled ? state : ST_DISABLED;
    UiPaintStyledBackground(w, tag.bounds, tag.style.palette,
                            tag.style.metrics, tag.style.skin, st, false);

    const Color ink = TagInk(tag.style.palette, st, SColorText());
    if(!tag.icon.IsEmpty() && !IsNull(tag.icon_image))
        w.DrawImage(tag.icon, tag.icon_image);
    if(!tag.prepared_text.IsEmpty() && !tag.text.IsEmpty())
        w.DrawText(tag.text.left, tag.text.top,
                   tag.prepared_text, tag.style.font, ink);

    w.End();
}

} // namespace Upp
