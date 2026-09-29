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
          "Contain keeps the complete source");
    Check(contain.target == RectC(0, 25, 100, 50),
          "Contain centers the fitted target");

    const UiMediaFitGeometry cover =
        UiComputeMediaFit(Size(200, 100), square, UiMediaFit::Cover);
    Check(cover.target == square,
          "Cover fills the complete target");
    Check(cover.source == RectC(50, 0, 100, 100),
          "Cover crops the wide source symmetrically");
}

void TestCardLayout()
{
    UiThemeContext context = UiTheme::GetContext();
    context.preset = UiThemePreset::Minimal;
    context.mode = UiThemeMode::Light;
    UiTheme::Set(context);

    UiMediaCard card;
    card.SetRect(0, 0, 180, 220);
    card.SetImage(MakeTestImage(Size(320, 180)))
        .SetTitle("SH030 take 03")
        .SetSubTitle("1536 x 864")
        .SetMetadata("AURORA / SQ020")
        .SetLabelSide(UiAlign::BOTTOM)
        .SetMediaFit(UiMediaFit::Cover);

    UiBadgeData left(
        "IMAGE", UiRole::Subtle, UiBadgeVariant::Filled);
    UiBadgeData right(
        "READY", UiRole::Accent, UiBadgeVariant::Soft);

    card.AddBadge(left, UiMediaBadgeAnchor::TopLeft);
    card.AddBadge(right, UiMediaBadgeAnchor::TopRight);
    card.Layout();

    const UiMediaCardPresentation& p = card.GetPresentation();
    Check(!p.media.IsEmpty(),
          "Media region is prepared");
    Check(!p.text.IsEmpty() && p.text.top > p.media.top,
          "Bottom labels receive a separate text region");
    Check(!p.prepared_title.IsEmpty(),
          "Title is prepared before Paint");
    Check(!p.media_image.IsEmpty(),
          "Scaled/cropped media is prepared before Paint");
    Check(p.badges.GetCount() == 2,
          "Visible semantic badges are prepared");
    Check(p.badges[0].bounds.top >= p.media.top
          && p.badges[0].bounds.left >= p.media.left,
          "Top-left badge stays inside media bounds");
    Check(p.badges[1].bounds.right <= p.media.right,
          "Top-right badge stays inside media bounds");
}

void TestAlternateSidesAndEmptyState()
{
    UiMediaCard card;
    card.SetRect(0, 0, 220, 150);
    card.SetTitle("Camera reference")
        .SetSubTitle("50 mm")
        .SetMetadata("frame 138")
        .SetLabelSide(UiAlign::RIGHT)
        .SetEmptyCue("+");
    card.Layout();

    const UiMediaCardPresentation& p = card.GetPresentation();
    Check(!p.text.IsEmpty() && p.text.left >= p.media.right,
          "Right labels are laid out beside media");
    Check(!p.prepared_empty_text.IsEmpty(),
          "Empty-state cue is prepared without an image");
    Check(card.GetMinSize().cx > 0 && card.GetMinSize().cy > 0,
          "MediaCard has a useful minimum size");
}

void TestBadgeCapacityAndHit()
{
    UiMediaCard card;
    card.SetRect(0, 0, 180, 180);
    card.SetEmptyCue("+");

    UiBadgeData a(
        "IMAGE", UiRole::Subtle, UiBadgeVariant::Filled);
    a.id = "a";
    a.actionable = true;
    a.value = 7;

    UiBadgeData b(
        "READY", UiRole::Accent, UiBadgeVariant::Outline);
    b.id = "b";

    card.AddBadge(a, UiMediaBadgeAnchor::TopLeft)
        .AddBadge(b, UiMediaBadgeAnchor::BottomRight);
    card.Layout();

    const UiMediaCardPresentation& p = card.GetPresentation();
    Check(p.badges.GetCount() == 2,
          "Opposing badge anchors can coexist");

    const Point hit = p.badges[0].bounds.CenterPoint();
    Check(card.HitTestBadge(hit) == 0,
          "Badge hit testing returns source badge identity");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestFitGeometry();
    TestCardLayout();
    TestAlternateSidesAndEmptyState();
    TestBadgeCapacityAndHit();

    Cout() << checks << " checks, "
           << failures << " failures\n";

    if(failures)
        SetExitCode(1);
}
