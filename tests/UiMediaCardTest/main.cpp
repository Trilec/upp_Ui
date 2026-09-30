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

Image MakeTestImage(Size size)
{
    ImageBuffer out(size);
    for(int y = 0; y < size.cy; y++) {
        RGBA *row = out[y];
        for(int x = 0; x < size.cx; x++) {
            row[x] = RGBA(Color(
                x * 255 / max(1, size.cx - 1),
                y * 255 / max(1, size.cy - 1), 120));
            row[x].a = 255;
        }
    }
    return Image(out);
}

void TestFitGeometry()
{
    const Rect square = RectC(0, 0, 100, 100);

    const UiMediaFitGeometry contain =
        UiComputeMediaFit(Size(200, 100), square, UiMediaFit::Contain);
    Check(contain.source == RectC(0, 0, 200, 100),
          "Contain keeps complete source");
    Check(contain.target == RectC(0, 25, 100, 50),
          "Contain centers fitted target");

    const UiMediaFitGeometry cover =
        UiComputeMediaFit(Size(200, 100), square, UiMediaFit::Cover);
    Check(cover.target == square,
          "Cover fills target");
    Check(cover.source == RectC(50, 0, 100, 100),
          "Cover crops wide source symmetrically");
}

void TestDefaultSurfaceContract()
{
    UiMediaCard card;
    const UiMediaCard::Style& style = card.GetStyle();

    Check(!style.metrics.face_enabled,
          "Card background is optional and off by default");
    Check(!style.metrics.frame_enabled,
          "Card border is optional and off by default");
    Check(style.media_metrics.face_enabled,
          "Media background is on by default");
    Check(style.media_metrics.frame_enabled,
          "Media border is independent and on by default");
    Check(!style.header_style.metrics.face_enabled
          && !style.header_style.metrics.frame_enabled,
          "Header surface is transparent and frameless by default");
    Check(!style.footer_style.metrics.face_enabled
          && !style.footer_style.metrics.frame_enabled,
          "Footer surface is transparent and frameless by default");
}

void TestStructureAndLayers()
{
    UiMediaCard card;
    card.SetRect(0, 0, 240, 320);
    card.SetImage(MakeTestImage(Size(320, 180)))
        .SetHeader("Image processing", "Harbour / convert EXR")
        .SetFooter("EXR / JPEG", "v012", "1536 x 864")
        .SetMediaAspect(Size(16, 9));

    UiTagData kind("IMAGE", UiRole::Subtle, UiTagVariant::Filled);
    kind.id = "kind";
    UiTagData ready("READY", UiRole::Accent, UiTagVariant::Soft);
    ready.id = "ready";
    ready.interactive = true;
    ready.value = 17;

    card.AddTopTag(kind, UiAlign::LEFT)
        .AddTopTag(ready, UiAlign::RIGHT);

    UiTagData take("take 03", UiRole::Standard, UiTagVariant::Soft);
    card.AddBottomTag(take, UiAlign::RIGHT);

    UiTagData overlay("LOADING", UiRole::Accent, UiTagVariant::Filled);
    card.SetOverlay(overlay, UiAlign::CENTER, UiAlign::CENTER);

    card.Layout();

    const UiMediaCardPresentation& p = card.GetPresentation();

    Check(p.header.visible && !p.header.bounds.IsEmpty(),
          "Header reserves structural space");
    Check(p.footer.visible && !p.footer.bounds.IsEmpty(),
          "Footer reserves structural space");
    Check(!p.media.IsEmpty()
          && p.header.bounds.bottom <= p.media.top
          && p.media.bottom <= p.footer.bounds.top,
          "Media stays between header and footer");
    Check(!p.media_image.IsEmpty(),
          "Media image is prepared before Paint");
    Check(p.top_tags.GetCount() == 2,
          "Top tag band prepares left and right tags");
    Check(p.bottom_tags.GetCount() == 1,
          "Bottom tag band is independent");
    Check(p.overlay.visible
          && p.media_content.Contains(p.overlay.bounds.CenterPoint()),
          "Overlay is positioned inside media without consuming it");
}

void TestOptionalBandsAndEmptyState()
{
    UiMediaCard card;
    card.SetRect(0, 0, 180, 210);
    card.ClearHeader()
        .SetFooter("Reference 1", "Drop or choose media")
        .SetEmptyCue("+")
        .SetMediaAspect(Size(1, 1));
    card.Layout();

    const UiMediaCardPresentation& p = card.GetPresentation();

    Check(!p.header.visible && p.header.bounds.IsEmpty(),
          "Absent header reserves no geometry");
    Check(p.footer.visible,
          "Footer remains independently available");
    Check(!p.prepared_empty_text.IsEmpty(),
          "Empty media cue is prepared");
    Check(card.GetMinSize().cx > 0 && card.GetMinSize().cy > 0,
          "MediaCard reports useful minimum size");
}

void TestTagHitIdentity()
{
    UiMediaCard card;
    card.SetRect(0, 0, 180, 180);
    card.SetEmptyCue("+");

    UiTagData action("READY", UiRole::Accent, UiTagVariant::Soft);
    action.id = "ready";
    action.interactive = true;
    action.value = 23;

    card.AddTopTag(action, UiAlign::RIGHT);
    card.Layout();

    const UiMediaCardPresentation& p = card.GetPresentation();
    Check(p.top_tags.GetCount() == 1,
          "Action tag is prepared");

    const UiTagPresentation *hit =
        card.FindTagAt(p.top_tags[0].bounds.CenterPoint());

    Check(hit && hit->id == "ready"
          && hit->IsInteractive() && (int)hit->value == 23,
          "Tag hit preserves id, interactivity and payload");
}

void TestRoundedMediaClipAndCacheReuse()
{
    UiMediaCardData data;
    data.image = MakeTestImage(Size(320, 180));

    UiMediaCard card;
    UiMediaCard::Style style = card.GetStyle();
    style.media_fit = UiMediaFit::Cover;
    style.media_aspect = Size(1, 1);
    style.media_metrics.radius = 18;

    const Rect bounds = RectC(0, 0, 180, 180);
    UiMediaCardPresentation first =
        UiPrepareMediaCard(data, style, bounds);
    UiMediaCardPresentation second =
        UiPrepareMediaCard(data, style, bounds);

    Check(!first.media_image.IsEmpty(),
          "Rounded media preparation produces an image");
    Check(first.media_image.GetSerialId() == second.media_image.GetSerialId(),
          "Repeated media preparation reuses the cached prepared image");

    if(!first.media_image.IsEmpty()) {
        const RGBA *top = first.media_image[0];
        const RGBA *middle =
            first.media_image[first.media_image.GetHeight() / 2];

        Check(top[0].a < 255,
              "Rounded media preparation clears/softens corner alpha");
        Check(middle[first.media_image.GetWidth() / 2].a == 255,
              "Rounded media preparation preserves center opacity");
    }
}

void TestInteractionGuards()
{
    UiMediaCard card;
    card.SetRect(0, 0, 180, 180);
    int actions = 0;
    card.WhenAction = [&] { actions++; };

    card.SetSelectable(false);
    card.LeftDown(Point(40, 40), 0);
    card.LeftUp(Point(40, 40), 0);
    Check(actions == 0,
          "Non-selectable card does not mouse-activate its body");

    card.SetSelectable(true);
    card.Enable(false);
    card.LeftDown(Point(40, 40), 0);
    card.LeftUp(Point(40, 40), 0);
    Check(actions == 0,
          "Disabled card does not mouse-activate its body");

    card.Enable(true);
    card.LeftDown(Point(40, 40), 0);
    card.Enable(false);
    card.LeftUp(Point(40, 40), 0);
    Check(actions == 0,
          "Card rechecks enabled state before mouse activation");

    card.Enable(true);
    card.LeftDown(Point(40, 40), 0);
    card.CancelMode();
    card.LeftUp(Point(40, 40), 0);
    Check(actions == 0,
          "CancelMode clears pending mouse activation");
}

void TestRendererSharesPresentation()
{
    UiItemRenderData item;
    item.title = "Take 04";
    item.description = "Image rework";
    item.right_text = "v014";
    item.image = MakeTestImage(Size(320, 180));
    item.role = UiRole::Standard;

    UiMediaCardRender render;
    render.SetResolver(
        [](const UiItemRenderData&, UiMediaCardData& card) {
            UiTagData kind("IMAGE", UiRole::Subtle, UiTagVariant::Filled);
            card.top_tags.Add().tag = kind;
            card.top_tags.Top().align = UiAlign::LEFT;
        });

    render.SetData(item);
    render.PrepareLayout(RectC(0, 0, 180, 200), UiDirection::V);

    const UiMediaCardPresentation& p = render.GetPresentation();

    Check(p.footer.visible && !p.footer.prepared_title.IsEmpty(),
          "Renderer maps item title into shared footer presentation");
    Check(!p.media_image.IsEmpty(),
          "Renderer uses the same prepared media path");
    Check(p.top_tags.GetCount() == 1,
          "Renderer resolver can add semantic tags");

    One<UiItemRender> clone = render.Clone();
    clone->SetData(item);
    clone->PrepareLayout(RectC(0, 0, 180, 200), UiDirection::V);
    Check(clone->GetMinSize().cx > 0,
          "Renderer clone remains usable by pooled model views");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestFitGeometry();
    TestDefaultSurfaceContract();
    TestStructureAndLayers();
    TestOptionalBandsAndEmptyState();
    TestTagHitIdentity();
    TestRoundedMediaClipAndCacheReuse();
    TestInteractionGuards();
    TestRendererSharesPresentation();

    Cout() << checks << " checks, "
           << failures << " failures\n";

    if(failures)
        SetExitCode(1);
}
