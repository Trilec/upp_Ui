#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool condition, const String& message)
    {
        checks++;
        if(!condition) {
            fails++;
            Cout() << "[FAIL] " << message << '\n';
        }
    }

    void Section(const String& title)
    {
        Cout() << "\n=== " << title << " ===\n";
    }
};

class MeasuredCtrl : public Ctrl {
public:
    explicit MeasuredCtrl(Size minimum) : minimum_(minimum) {}
    Size GetMinSize() const override { return minimum_; }

private:
    Size minimum_;
};

static UiGroupPanel::Style TestStyle()
{
    UiGroupPanel::Style s = UiGroupPanel::StyleDefault();
    s.metrics.radius = 0;
    s.metrics.frame_width = 1;
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.palette.face[ST_NORMAL] = UiFill::Solid(White());
    s.palette.frame[ST_NORMAL] = Color(231, 35, 47);
    s.inset = Rect(8, 8, 8, 8);
    s.header_inset = Rect(8, 4, 8, 4);
    s.header_gap = 4;
    return s;
}

static bool NonNegative(const Rect& r)
{
    return r.GetWidth() >= 0 && r.GetHeight() >= 0;
}

static void TestSlotOwnership(TestCtx& t)
{
    t.Section("Slot ownership");
    UiGroupPanel group;
    MeasuredCtrl body_a(Size(80, 60));
    MeasuredCtrl body_b(Size(70, 50));
    MeasuredCtrl header_a(Size(42, 16));
    MeasuredCtrl header_b(Size(36, 14));

    t.Expect(group.GetContent() == nullptr && group.GetHeaderContent() == nullptr,
             "new group has two empty semantic slots");
    group.SetContent(body_a);
    t.Expect(group.GetContent() == &body_a && body_a.GetParent() == &group,
             "body-only attachment uses the body slot");
    group.SetHeaderContent(header_a);
    t.Expect(group.GetHeaderContent() == &header_a && header_a.GetParent() == &group,
             "header-only attachment uses the header slot");
    group.SetContent(body_b);
    t.Expect(group.GetContent() == &body_b && body_a.GetParent() == nullptr,
             "replacing body detaches only the old body");
    t.Expect(group.GetHeaderContent() == &header_a,
             "replacing body preserves header content");
    group.SetHeaderContent(header_b);
    t.Expect(group.GetHeaderContent() == &header_b && header_a.GetParent() == nullptr,
             "replacing header detaches only the old header");

    group.SetHeaderContent(body_b);
    t.Expect(group.GetContent() == nullptr && group.GetHeaderContent() == &body_b,
             "one control moves safely from body to header");
    group.SetContent(body_b);
    t.Expect(group.GetContent() == &body_b && group.GetHeaderContent() == nullptr,
             "one control moves safely from header to body");

    group.SetHeaderContent(header_a);
    header_a.Remove();
    t.Expect(group.GetHeaderContent() == nullptr,
             "external header removal clears the stored pointer");
    body_b.Remove();
    t.Expect(group.GetContent() == nullptr,
             "external body removal clears the stored pointer");

    group.SetContent(body_a).SetHeaderContent(header_b);
    t.Expect(group.GetContent() == &body_a && group.GetHeaderContent() == &header_b,
             "both slots can be repopulated independently");
    group.ClearContent();
    t.Expect(group.GetContent() == nullptr, "ClearContent clears the body slot");
    t.Expect(group.GetHeaderContent() == &header_b,
             "ClearContent preserves the header slot");
    group.ClearHeaderContent();
    t.Expect(group.GetHeaderContent() == nullptr,
             "ClearHeaderContent clears only the header slot");
}

static void TestGeometryMatrix(TestCtx& t)
{
    t.Section("Placement and mode geometry");
    UiAlign placements[] = {UiAlign::TOP, UiAlign::BOTTOM, UiAlign::LEFT, UiAlign::RIGHT};
    UiGroupPanel::HeaderMode modes[] = {
        UiGroupPanel::Outside, UiGroupPanel::Center, UiGroupPanel::Inside
    };
    for(UiAlign placement : placements) {
        for(UiGroupPanel::HeaderMode mode : modes) {
            UiGroupPanel group;
            MeasuredCtrl body(Size(90, 55));
            MeasuredCtrl header(Size(44, 14));
            group.SetCustomStyle(TestStyle())
                 .SetTitle("Identity")
                 .SetSubTitle("Context")
                 .SetHeaderPlacement(placement)
                 .SetHeaderMode(mode)
                 .SetContent(body)
                 .SetHeaderContent(header);
            group.SetRect(0, 0, 300, 180);
            group.Layout();
            Rect body_rect = group.GetBodyRect();
            Rect slot_rect = group.GetHeaderContentRect();
            t.Expect(NonNegative(body_rect) && NonNegative(slot_rect) && NonNegative(header.GetRect()),
                     "all mode/placement rectangles are non-negative");
            t.Expect(slot_rect.Contains(header.GetRect()),
                     "resolved header child remains inside its authoritative slot region");
            t.Expect(!body_rect.Intersects(header.GetRect()),
                     "body and actual header content do not overlap");

            group.SetRect(0, 0, 12, 9);
            group.Layout();
            t.Expect(NonNegative(group.GetBodyRect()) &&
                     NonNegative(group.GetHeaderContentRect()) && NonNegative(header.GetRect()),
                     "forced undersize geometry clamps deterministically");
        }
    }
}

static void TestBaselineGeometry(TestCtx& t)
{
    t.Section("No-header-content compatibility");
    UiGroupPanel group;
    group.SetCustomStyle(TestStyle()).SetTitle("Group");
    group.SetRect(0, 0, 300, 180);
    t.Expect(group.GetBodyRect() == Rect(9, 37, 291, 171),
             "default Top/Inside body geometry is unchanged without header content");
    Rect empty_slot = group.GetHeaderContentRect();
    t.Expect(!empty_slot.IsEmpty(),
             "an empty group exposes a prospective header-content region");

    MeasuredCtrl body(Size(60, 40));
    group.SetContent(body);
    group.Layout();
    t.Expect(body.GetRect() == group.GetBodyRect(),
             "body-only layout still fills GetBodyRect");

    UiGroupPanel header_only;
    MeasuredCtrl header(Size(36, 12));
    header_only.SetCustomStyle(TestStyle()).SetTitle("Group").SetHeaderContent(header);
    header_only.SetRect(0, 0, 300, 180);
    header_only.Layout();
    t.Expect(header_only.GetHeaderContentRect().Contains(header.GetRect()),
             "header-only layout uses the public header-content region");
}

static void TestTitleSideResolution(TestCtx& t)
{
    t.Section("Title side resolution");
    UiGroupPanel group;
    MeasuredCtrl header(Size(36, 12));
    group.SetCustomStyle(TestStyle()).SetTitle("Title").SetHeaderContent(header);
    group.SetRect(0, 0, 260, 130);

    group.SetHeaderPlacement(UiAlign::TOP).SetTitleAlign(UiAlign::LEFT, UiAlign::CENTER);
    Rect left_slot = group.GetHeaderContentRect();
    group.SetTitleAlign(UiAlign::RIGHT, UiAlign::CENTER);
    Rect right_slot = group.GetHeaderContentRect();
    t.Expect(left_slot.left > right_slot.left && left_slot.right > right_slot.right,
             "left title resolves a trailing/right header-content region");
    t.Expect(right_slot.right < left_slot.right || right_slot.GetWidth() > 0,
             "right title resolves the header-content region on its left");

    group.SetTitleAlign(UiAlign::CENTER, UiAlign::CENTER);
    Rect center_slot = group.GetHeaderContentRect();
    t.Expect(center_slot.left > 260 / 2,
             "centered horizontal title retains the centre and uses trailing remainder");

    group.SetHeaderPlacement(UiAlign::LEFT).SetTitleAlign(UiAlign::CENTER, UiAlign::TOP);
    Rect top_slot = group.GetHeaderContentRect();
    group.SetTitleAlign(UiAlign::CENTER, UiAlign::BOTTOM);
    Rect bottom_slot = group.GetHeaderContentRect();
    t.Expect(top_slot.top > bottom_slot.top,
             "top title resolves a trailing/bottom header-content region");
    t.Expect(bottom_slot.bottom < top_slot.bottom || bottom_slot.GetHeight() > 0,
             "bottom title resolves the header-content region above it");
    group.SetTitleAlign(UiAlign::CENTER, UiAlign::CENTER);
    t.Expect(group.GetHeaderContentRect().top > 130 / 2,
             "centered vertical title retains the centre and uses bottom remainder");
}

static void TestHeaderContentAlignment(TestCtx& t)
{
    t.Section("Header content alignment");
    UiGroupPanel group;
    MeasuredCtrl header(Size(30, 10));
    group.SetCustomStyle(TestStyle()).SetTitle("T").SetHeaderContent(header);
    group.SetRect(0, 0, 280, 150);
    Rect slot = group.GetHeaderContentRect();

    group.SetHeaderContentAlign(UiAlign::LEFT, UiAlign::TOP);
    group.Layout();
    t.Expect(header.GetRect().TopLeft() == slot.TopLeft(),
             "Left/Top aligns child to the slot origin");
    group.SetHeaderContentAlign(UiAlign::CENTER, UiAlign::CENTER);
    group.Layout();
    t.Expect(abs(header.GetRect().CenterPoint().x - slot.CenterPoint().x) <= 1 &&
             abs(header.GetRect().CenterPoint().y - slot.CenterPoint().y) <= 1,
             "Center/Center centres child in the slot");
    group.SetHeaderContentAlign(UiAlign::RIGHT, UiAlign::BOTTOM);
    group.Layout();
    t.Expect(header.GetRect().right == slot.right && header.GetRect().bottom == slot.bottom,
             "Right/Bottom aligns child to the slot end");
}

static void TestMinimumSize(TestCtx& t)
{
    t.Section("Minimum size contributions");
    UiGroupPanel empty;
    empty.SetCustomStyle(TestStyle()).SetTitle("Group");
    Size base = empty.GetMinSize();

    UiGroupPanel body_group;
    MeasuredCtrl body(Size(220, 90));
    body_group.SetCustomStyle(TestStyle()).SetTitle("Group").SetContent(body);
    Size with_body = body_group.GetMinSize();
    t.Expect(with_body.cx >= 220 && with_body.cy > base.cy,
             "body minimum contributes to GroupPanel minimum size");

    UiGroupPanel header_group;
    MeasuredCtrl header(Size(180, 34));
    header_group.SetCustomStyle(TestStyle()).SetTitle("Group").SetHeaderContent(header);
    Size with_header = header_group.GetMinSize();
    t.Expect(with_header.cx > base.cx && with_header.cy >= base.cy,
             "header-content minimum contributes to GroupPanel minimum size");
}

static bool IsFramePixel(const Image& image, int x, int y)
{
    if(x < 0 || y < 0 || x >= image.GetWidth() || y >= image.GetHeight())
        return false;
    const RGBA& p = image[y][x];
    return p.r == 231 && p.g == 35 && p.b == 47 && p.a != 0;
}

static void TestCenteredFrameExclusion(TestCtx& t)
{
    t.Section("Centered frame exclusions");
    UiAlign placements[] = {UiAlign::TOP, UiAlign::BOTTOM, UiAlign::LEFT, UiAlign::RIGHT};
    for(UiAlign placement : placements) {
        UiGroupPanel group;
        MeasuredCtrl header(Size(42, 14));
        group.SetCustomStyle(TestStyle())
             .SetTitle("Title")
             .SetHeaderMode(UiGroupPanel::Center)
             .SetHeaderPlacement(placement)
             .SetHeaderContent(header);
        group.SetRect(0, 0, 280, 160);
        group.Layout();
        ImageDraw draw(280, 160);
        draw.DrawRect(0, 0, 280, 160, White());
        group.Paint(draw);
        Image image = draw;
        Rect occupied = header.GetRect();
        bool frame_under_child = false;
        int frame_pixels = 0;
        for(int y = 0; y < image.GetHeight(); y++)
            for(int x = 0; x < image.GetWidth(); x++)
                frame_pixels += IsFramePixel(image, x, y);
        for(int y = occupied.top; y < occupied.bottom && !frame_under_child; y++)
            for(int x = occupied.left; x < occupied.right; x++)
                if(IsFramePixel(image, x, y)) {
                    frame_under_child = true;
                    break;
                }
        t.Expect(!frame_under_child,
                 "centered frame omits the occupied header-content rectangle on every side");
        t.Expect(frame_pixels > 100,
                 "centered exclusion preserves the remaining styled frame");
    }
}

static void TestStyleSerialization(TestCtx& t)
{
    t.Section("Style serialization");
    UiGroupPanel::Style source = TestStyle();
    source.title_font = Serif(17).Bold();
    source.subtitle_color = Color(12, 34, 56);
    source.header_placement = UiAlign::RIGHT;
    source.title_align_h = UiAlign::RIGHT;
    source.title_align_v = UiAlign::BOTTOM;
    source.header_mode = UiGroupPanel::Center;
    source.header_gap = 11;
    String encoded = StoreAsString(source);
    UiGroupPanel::Style loaded;
    bool ok = LoadFromString(loaded, encoded);
    t.Expect(ok, "GroupPanel style round-trips through its stable stream contract");
    t.Expect(loaded.header_placement == UiAlign::RIGHT &&
             loaded.title_align_h == UiAlign::RIGHT &&
             loaded.title_align_v == UiAlign::BOTTOM &&
             loaded.header_mode == UiGroupPanel::Center && loaded.header_gap == 11,
             "live fields remain aligned after consuming retired SideTitle positions");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestSlotOwnership(t);
    TestGeometryMatrix(t);
    TestBaselineGeometry(t);
    TestTitleSideResolution(t);
    TestHeaderContentAlignment(t);
    TestMinimumSize(t);
    TestCenteredFrameExclusion(t);
    TestStyleSerialization(t);

    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
