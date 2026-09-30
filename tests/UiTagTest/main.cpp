#include <Ui/Ui.h>

using namespace Upp;

namespace {

int checks = 0;
int failures = 0;

void Check(bool condition, const String& message)
{
    checks++;
    if(condition)
        Cout() << "PASS  " << message << '\n';
    else {
        failures++;
        Cout() << "FAIL  " << message << '\n';
    }
}

Image MakeIcon(Size size)
{
    ImageBuffer out(size);
    out.SetKind(IMAGE_ALPHA);
    for(int y = 0; y < size.cy; y++) {
        RGBA *row = out[y];
        for(int x = 0; x < size.cx; x++) {
            row[x] = RGBA(Color(30 + x * 7, 80 + y * 9, 180));
            row[x].a = 255;
        }
    }
    return Image(out);
}

int AlphaAt(const Image& image, int x, int y)
{
    if(IsNull(image) || x < 0 || y < 0
       || x >= image.GetWidth() || y >= image.GetHeight())
        return -1;
    return image[y][x].a;
}

RGBA CompositeCenter(const Image& overlay, Color background)
{
    SImageDraw canvas(overlay.GetSize());
    canvas.DrawRect(overlay.GetSize(), background);
    canvas.DrawImage(0, 0, overlay);
    Image result = canvas;
    return result[result.GetHeight() / 2][result.GetWidth() / 2];
}

UiTagStyle BareStyle()
{
    UiTagStyle style = UiResolveTagStyle(UiRole::Standard);
    style.metrics.content_margin = Rect(0, 0, 0, 0);
    style.metrics.face_enabled = false;
    style.metrics.frame_enabled = false;
    style.metrics.frame_width = 0;
    style.metrics.radius = 0;
    return style;
}

void TestSuppressionAndForms()
{
    UiTagStyle style = BareStyle();

    UiTagData empty;
    Check(UiMeasureTag(empty, style) == Size(0, 0),
          "Empty tag measures to zero");
    Check(!UiPrepareTag(empty, style, RectC(0, 0, 80, 24)).visible,
          "Empty tag does not prepare a visible presentation");

    UiTagData text("READY", UiRole::Standard, UiTagVariant::Filled);
    UiTagPresentation tp =
        UiPrepareTag(text, style, RectC(0, 0, 80, 24));
    Check(tp.visible && !tp.prepared_text.IsEmpty() && tp.icon.IsEmpty(),
          "Text-only tag prepares without an icon");

    UiTagData icon;
    icon.icon = MakeIcon(Size(20, 10));
    icon.role = UiRole::Standard;
    UiTagPresentation ip =
        UiPrepareTag(icon, style, RectC(0, 0, 40, 24));
    Check(ip.visible && !ip.icon.IsEmpty() && ip.prepared_text.IsEmpty(),
          "Icon-only tag prepares without text");

    UiTagData both("INFO", UiRole::Standard, UiTagVariant::Filled);
    both.icon = icon.icon;
    UiTagPresentation bp =
        UiPrepareTag(both, style, RectC(0, 0, 90, 24));
    Check(bp.visible && !bp.icon.IsEmpty() && !bp.prepared_text.IsEmpty(),
          "Icon+text tag prepares both blocks");
}

void TestIconContract()
{
    Image icon = MakeIcon(Size(20, 10));
    UiTagStyle style = BareStyle();
    style.icon_size = Size(12, 12);
    style.icon_render_mode = UiIconRenderMode::MonoTint;

    UiTagData data;
    data.icon = icon;
    data.role = UiRole::Accent;

    UiTagPresentation mono =
        UiPrepareTag(data, style, RectC(0, 0, 40, 24));
    Check(mono.icon.GetSize() == Size(12, 6),
          "Icon aspect is preserved inside the explicit icon box");
    Check(mono.icon_image.GetSize() == Size(12, 6),
          "Prepared icon raster matches fitted target size");
    Check(mono.tint_icon,
          "MonoTint prepares icon for state tinting");

    style.icon_render_mode = UiIconRenderMode::PreserveColor;
    UiTagPresentation preserve =
        UiPrepareTag(data, style, RectC(0, 0, 40, 24));
    Check(!preserve.tint_icon,
          "PreserveColor keeps source icon colours");

    style.icon_render_mode = UiIconRenderMode::Auto;
    UiTagPresentation automatic =
        UiPrepareTag(data, style, RectC(0, 0, 40, 24));
    Check(automatic.tint_icon,
          "Auto resolves to the compact tag MonoTint default");

    style.icon_render_mode = UiIconRenderMode::MonoTint;
    style.icon_side = UiAlign::RIGHT;
    style.content_gap = 7;
    UiTagData both("META", UiRole::Standard, UiTagVariant::Filled);
    both.icon = icon;
    UiTagPresentation right =
        UiPrepareTag(both, style, RectC(0, 0, 100, 24));
    Check(right.icon.right == 100
          && right.text.right <= right.icon.left - 7,
          "Right-side icon placement preserves the authored content gap");

    UiTagPresentation again =
        UiPrepareTag(data, style, RectC(0, 0, 40, 24));
    UiTagPresentation again2 =
        UiPrepareTag(data, style, RectC(0, 0, 40, 24));
    Check(again.icon_image.GetSerialId() == again2.icon_image.GetSerialId(),
          "Repeated icon preparation reuses CachedRescale output");
}

void TestVariantsAndAlpha()
{
    UiTagStyle style = BareStyle();
    style.metrics.face_enabled = true;
    style.metrics.frame_enabled = true;
    style.metrics.frame_width = 2;
    style.palette.face[ST_NORMAL] = UiFill::Solid(Color(70, 120, 210));
    style.palette.frame[ST_NORMAL] = Color(240, 220, 80);
    style.face_alpha[ST_NORMAL] = 200;
    style.frame_alpha[ST_NORMAL] = 90;
    style.soft_face_alpha = 72;

    UiTagData filled("FILLED", UiRole::Standard, UiTagVariant::Filled);
    UiTagPresentation fp =
        UiPrepareTag(filled, style, RectC(0, 0, 90, 28));
    int filled_center =
        AlphaAt(fp.decoration[ST_NORMAL],
                fp.decoration[ST_NORMAL].GetWidth() / 2,
                fp.decoration[ST_NORMAL].GetHeight() / 2);
    Check(filled_center >= 190 && filled_center <= 205,
          "Filled face opacity is prepared as real alpha");

    UiTagData soft("SOFT", UiRole::Standard, UiTagVariant::Soft);
    UiTagPresentation sp =
        UiPrepareTag(soft, style, RectC(0, 0, 90, 28));
    int soft_center =
        AlphaAt(sp.decoration[ST_NORMAL],
                sp.decoration[ST_NORMAL].GetWidth() / 2,
                sp.decoration[ST_NORMAL].GetHeight() / 2);
    Check(soft_center >= 65 && soft_center <= 80,
          "Soft variant caps face opacity without parent-colour blending");

    const RGBA soft_black =
        CompositeCenter(sp.decoration[ST_NORMAL], Black());
    const RGBA soft_white =
        CompositeCenter(sp.decoration[ST_NORMAL], White());
    Check(soft_black.r < soft_white.r
          && soft_black.g < soft_white.g
          && soft_black.b < soft_white.b,
          "Soft alpha actually composites over arbitrary underlying pixels");

    UiTagData outline("OUTLINE", UiRole::Standard, UiTagVariant::Outline);
    UiTagPresentation op =
        UiPrepareTag(outline, style, RectC(0, 0, 90, 28));
    int outline_center =
        AlphaAt(op.decoration[ST_NORMAL],
                op.decoration[ST_NORMAL].GetWidth() / 2,
                op.decoration[ST_NORMAL].GetHeight() / 2);
    int outline_edge =
        AlphaAt(op.decoration[ST_NORMAL], 1,
                op.decoration[ST_NORMAL].GetHeight() / 2);
    Check(outline_center == 0,
          "Outline variant keeps the face transparent");
    Check(outline_edge > 0 && outline_edge < 180,
          "Outline frame opacity is prepared independently");

    StyledPalette parent_a;
    StyledPalette parent_b;
    for(int st = 0; st < 4; st++) {
        parent_a.face[st] = UiFill::Solid(Black());
        parent_b.face[st] = UiFill::Solid(White());
    }
    UiTagPresentation pa =
        UiPrepareTag(soft, style, RectC(0, 0, 90, 28), &parent_a);
    UiTagPresentation pb =
        UiPrepareTag(soft, style, RectC(0, 0, 90, 28), &parent_b);
    Check(pa.decoration[ST_NORMAL].GetSerialId()
          == pb.decoration[ST_NORMAL].GetSerialId(),
          "Soft decoration is independent of a guessed parent colour");

    UiTagStyle image_style = BareStyle();
    image_style.metrics.face_enabled = true;
    image_style.palette.face[ST_NORMAL] =
        UiFill::ImageFill(MakeIcon(Size(6, 6)));
    image_style.face_alpha[ST_NORMAL] = 128;
    UiTagData image_fill("IMAGE", UiRole::Standard, UiTagVariant::Filled);
    UiTagPresentation image_p =
        UiPrepareTag(image_fill, image_style, RectC(0, 0, 90, 28));
    int image_center =
        AlphaAt(image_p.decoration[ST_NORMAL],
                image_p.decoration[ST_NORMAL].GetWidth() / 2,
                image_p.decoration[ST_NORMAL].GetHeight() / 2);
    Check(image_center >= 120 && image_center <= 136,
          "UiFill image backgrounds retain prepared face alpha");
}

void TestTextGeometry()
{
    UiTagStyle style = BareStyle();
    style.metrics.content_margin = Rect(5, 3, 7, 3);
    style.content_gap = 4;

    UiTagData data(
        "THIS IS A DELIBERATELY LONG TAG LABEL",
        UiRole::Standard, UiTagVariant::Filled);
    data.icon = MakeIcon(Size(16, 16));
    style.icon_size = Size(10, 10);

    Rect bounds = RectC(20, 10, 88, 26);
    UiTagPresentation p =
        UiPrepareTag(data, style, bounds);

    Check(bounds.Contains(p.icon) && bounds.Contains(p.text),
          "Prepared icon/text geometry stays inside tag bounds");
    Check(p.text.left >= p.icon.right + style.content_gap,
          "Padding/gap geometry separates icon and text");
    Check(!p.prepared_text.IsEmpty()
          && p.prepared_text.GetCount() < data.text.GetCount(),
          "Constrained text is ellipsized during preparation");
}

void TestInteractionAndDisabled()
{
    UiTagStyle style = BareStyle();
    style.palette.ink[ST_DISABLED] = Color(31, 41, 55);
    style.palette.icon[ST_DISABLED] = Color(91, 101, 115);

    UiTagData data("ERROR", UiRole::Alert, UiTagVariant::Filled);
    data.id = "error";
    data.value = 42;
    data.interactive = true;
    data.enabled = false;

    UiTagPresentation p =
        UiPrepareTag(data, style, RectC(0, 0, 80, 24));
    Check(p.IsInteractive() && p.id == "error" && (int)p.value == 42,
          "Interactive tag preserves id and opaque payload");
    Check(!p.enabled && p.ink[ST_DISABLED] == Color(31, 41, 55),
          "Disabled presentation preserves disabled visual state");

    UiTagData compatibility("OLD", UiRole::Standard, UiTagVariant::Filled);
    compatibility.actionable = true;
    UiTagPresentation cp =
        UiPrepareTag(compatibility, style, RectC(0, 0, 60, 24));
    Check(cp.IsInteractive(),
          "Short-lived actionable compatibility still routes as interactive");
}

void TestDecorationCacheAndPaintPath()
{
    UiRasterCache::ClearTag("tag/decoration");

    UiTagStyle style = UiResolveTagStyle(UiRole::Accent);
    style.metrics.radius = 6;
    UiTagData data("CACHE", UiRole::Accent, UiTagVariant::Filled);

    UiRasterCacheStats before = UiRasterCache::GetStats();
    UiTagPresentation first =
        UiPrepareTag(data, style, RectC(10, 8, 84, 26));
    UiRasterCacheStats after_first = UiRasterCache::GetStats();
    UiTagPresentation second =
        UiPrepareTag(data, style, RectC(10, 8, 84, 26));
    UiRasterCacheStats after_second = UiRasterCache::GetStats();

    Check(!IsNull(first.decoration[ST_NORMAL])
          && first.decoration[ST_NORMAL].GetSerialId()
             == second.decoration[ST_NORMAL].GetSerialId(),
          "Prepared face/frame decoration is shared from UiRasterCache");
    Check(first.decoration[ST_NORMAL].GetSerialId()
          == first.decoration[ST_HOT].GetSerialId()
          && first.decoration[ST_NORMAL].GetSerialId()
             == first.decoration[ST_PRESSED].GetSerialId(),
          "Passive tag aliases one prepared decoration across pointer states");
    Check(after_first.misses > before.misses
          && after_second.hits > after_first.hits,
          "Repeated preparation records raster-cache hits");

    UiRasterCacheStats before_paint = UiRasterCache::GetStats();
    ImageDraw canvas(120, 48);
    canvas.DrawRect(Size(120, 48), White());
    UiPaintTag(canvas, first, ST_NORMAL);
    Image rendered = canvas;
    UiRasterCacheStats after_paint = UiRasterCache::GetStats();

    Check(before_paint.hits == after_paint.hits
          && before_paint.misses == after_paint.misses
          && before_paint.insertions == after_paint.insertions,
          "Paint consumes prepared rasters without cache/raster preparation");

    const RGBA *outside = rendered[2];
    Check(outside[2].r == 255 && outside[2].g == 255
          && outside[2].b == 255,
          "Tag paint leaves pixels outside authored bounds untouched");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestSuppressionAndForms();
    TestIconContract();
    TestVariantsAndAlpha();
    TestTextGeometry();
    TestInteractionAndDisabled();
    TestDecorationCacheAndPaintPath();

    Cout() << checks << " checks, "
           << failures << " failures\n";

    if(failures)
        SetExitCode(1);
}
