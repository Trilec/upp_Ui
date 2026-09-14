#include <Core/Core.h>
#include <Ui/Ui.h>
#include <cmath>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool cond, const String& msg)
    {
        checks++;
        if(!cond) {
            fails++;
            Cout() << "[FAIL] " << msg << "\n";
        }
    }

    void Section(const String& name)
    {
        Cout() << "\n=== " << name << " ===\n";
    }
};

static bool Near(double a, double b, double eps = 0.0001)
{
    return fabs(a - b) <= eps;
}

static void TestDataAndNormalization(TestCtx& t)
{
    t.Section("Data and normalization");

    UiRangeSegments r;
    t.Expect(r.GetSegmentCount() == 4, "default control starts with four equal segments");
    t.Expect(Near(r.GetMin(), 0.0) && Near(r.GetMax(), 100.0), "default scalar domain is 0..100");
    t.Expect(r.GetBoundaryCount() == 3, "four segments expose three internal boundaries");
    t.Expect(r.GetSelectedSegment() == -1 && r.GetActiveBoundary() == -1,
             "default control starts visually neutral with no selected segment or boundary");
    t.Expect(Near(r.GetBoundaryValue(0), 25.0) && Near(r.GetBoundaryValue(1), 50.0) && Near(r.GetBoundaryValue(2), 75.0),
             "default boundaries are equal quarters");

    Vector<UiRangeSegment> weighted;
    weighted.Add(UiRangeSegment(1, "LOD 0"));
    weighted.Add(UiRangeSegment(2, "LOD 1"));
    weighted.Add(UiRangeSegment(1, "LOD 2"));
    r.SetSegments(weighted);
    t.Expect(Near(r.GetSegmentSpan(0), 25.0) && Near(r.GetSegmentSpan(1), 50.0) && Near(r.GetSegmentSpan(2), 25.0),
             "SetSegments treats authored spans as proportional weights across the fixed domain");
    t.Expect(r.GetSegment(1).label == "LOD 1", "segment labels survive normalization");

    r.SetRange(-2.0, 2.0);
    t.Expect(Near(r.GetSegmentSpan(0), 1.0) && Near(r.GetSegmentSpan(1), 2.0) && Near(r.GetSegmentSpan(2), 1.0),
             "changing the scalar domain preserves segment ratios");
    t.Expect(Near(r.GetBoundaryValue(0), -1.0) && Near(r.GetBoundaryValue(1), 1.0),
             "boundaries are reported in domain units, not percentages");

    r.SetRange(0, 100).SetSegmentCount(5);
    double sum = 0.0;
    for(int i = 0; i < r.GetSegmentCount(); i++)
        sum += r.GetSegmentSpan(i);
    t.Expect(Near(sum, 100.0), "equal segment count fills the complete fixed range");
    t.Expect(r.GetBoundaryCount() == 4, "N segments always expose N-1 boundaries");
}

static void TestBoundaryEditing(TestCtx& t)
{
    t.Section("Boundary editing");

    UiRangeSegments r;
    r.SetRange(0, 100).SetSegmentCount(4);
    r.SetBoundaryValue(1, 60);
    t.Expect(Near(r.GetSegmentSpan(0), 25.0), "moving a boundary leaves earlier unrelated segments unchanged");
    t.Expect(Near(r.GetSegmentSpan(1), 35.0) && Near(r.GetSegmentSpan(2), 15.0),
             "moving a boundary redistributes only its two neighbouring spans");
    t.Expect(Near(r.GetSegmentSpan(3), 25.0), "moving a boundary leaves later unrelated segments unchanged");
    t.Expect(Near(r.GetBoundaryValue(2), 75.0), "later boundary remains fixed while adjacent spans redistribute");

    r.SetStep(5).SetBoundaryValue(1, 63);
    t.Expect(Near(r.GetBoundaryValue(1), 65.0), "boundary editing snaps to the scalar step");

    r.SetMinimumSegmentSpan(10).SetBoundaryValue(0, 2);
    t.Expect(Near(r.GetBoundaryValue(0), 10.0), "minimum segment span clamps the left side of a boundary");
    r.SetBoundaryValue(0, 62);
    t.Expect(Near(r.GetBoundaryValue(0), 55.0), "minimum segment span clamps against the neighbouring segment on the right");

    Vector<double> values = r.GetBoundaryValues();
    t.Expect(values.GetCount() == r.GetBoundaryCount(), "boundary vector exposes every internal threshold");
    t.Expect(Near(values[0], r.GetBoundaryValue(0)), "boundary vector uses the same authoritative scalar values");

    UiRangeSegments thresholds;
    thresholds.SetRange(0, 100).SetStep(1).SetMinimumSegmentSpan(5);
    Vector<double> authored;
    authored.Add(15);
    authored.Add(45);
    authored.Add(82);
    thresholds.SetBoundaryValues(authored);
    t.Expect(thresholds.GetSegmentCount() == 4, "SetBoundaryValues derives N+1 contiguous segments from N thresholds");
    t.Expect(Near(thresholds.GetBoundaryValue(0), 15.0) && Near(thresholds.GetBoundaryValue(1), 45.0) && Near(thresholds.GetBoundaryValue(2), 82.0),
             "SetBoundaryValues preserves valid authored scalar thresholds");
    t.Expect(Near(thresholds.GetSegmentSpan(0), 15.0) && Near(thresholds.GetSegmentSpan(3), 18.0),
             "threshold-array input resolves segment spans against the fixed domain");
}

static void TestStructureMutation(TestCtx& t)
{
    t.Section("Split and remove");

    UiRangeSegments r;
    Vector<UiRangeSegment> pair;
    pair.Add(UiRangeSegment(50, "Near"));
    pair.Add(UiRangeSegment(50, "Far"));
    r.SetRange(0, 100).SetSegments(pair);
    r.SplitSegment(0, 0.25, "Micro");
    t.Expect(r.GetSegmentCount() == 3, "SplitSegment adds one contiguous range");
    t.Expect(Near(r.GetSegmentSpan(0), 12.5) && Near(r.GetSegmentSpan(1), 37.5),
             "SplitSegment divides only the chosen span using the requested ratio");
    t.Expect(r.GetSegment(1).label == "Micro", "split segment accepts a new label");

    double before = 0.0;
    for(int i = 0; i < r.GetSegmentCount(); i++)
        before += r.GetSegmentSpan(i);
    r.RemoveSegment(1);
    double after = 0.0;
    for(int i = 0; i < r.GetSegmentCount(); i++)
        after += r.GetSegmentSpan(i);
    t.Expect(r.GetSegmentCount() == 2, "RemoveSegment removes one authored range");
    t.Expect(Near(before, after) && Near(after, 100.0), "removing a segment merges its span and preserves the fixed total");

    r.ClearSegments();
    t.Expect(r.GetSegmentCount() == 0 && r.GetBoundaryCount() == 0, "ClearSegments removes data and boundaries together");
}

static void TestDataBinding(TestCtx& t)
{
    t.Section("Value binding");

    UiRangeSegments r;
    Vector<UiRangeSegment> authored;
    authored.Add(UiRangeSegment(30, "Normal", Color(1, 2, 3), String("normal")));
    authored.Add(UiRangeSegment(70, "Reduced", Null, String("reduced")));
    r.SetSegments(authored);

    Value data = r.GetData();
    t.Expect(data.Is<ValueArray>(), "GetData exports a serializable ValueArray");
    ValueArray array = data;
    t.Expect(array.GetCount() == 2 && array[0].Is<ValueMap>(), "each exported segment is a ValueMap record");

    UiRangeSegments copy;
    copy.SetData(data);
    t.Expect(copy.GetSegmentCount() == 2, "SetData restores the exported segment collection");
    t.Expect(copy.GetSegment(0).label == "Normal" && copy.GetSegment(1).label == "Reduced", "SetData restores labels");
    t.Expect(copy.GetSegment(0).color == Color(1, 2, 3), "SetData restores explicit segment colours");
    t.Expect(AsString(copy.GetSegment(1).data) == "reduced", "SetData restores application payload values");
    t.Expect(Near(copy.GetSegmentSpan(0), 30.0) && Near(copy.GetSegmentSpan(1), 70.0), "SetData restores proportional spans");
}

static void TestGeometry(TestCtx& t)
{
    t.Section("Geometry and orientation");

    UiRangeSegments r;
    r.SetRange(0, 100).SetSegmentCount(4);
    UiRangeSegments::Geometry h = r.GetGeometry(Size(420, 90));
    t.Expect(!h.track.IsEmpty() && !h.content.IsEmpty(), "horizontal geometry resolves track and content rectangles");
    t.Expect(h.segments.GetCount() == 4 && h.boundaries.GetCount() == 3, "geometry exposes segment and boundary projections");
    t.Expect(h.segments[0].rect.left == h.content.left && h.segments[3].rect.right == h.content.right,
             "horizontal segments cover the complete content extent");
    t.Expect(h.segments[0].rect.right == h.segments[1].rect.left,
             "adjacent horizontal segments remain contiguous without hidden gaps");

    r.SetDirection(UiDirection::V);
    UiRangeSegments::Geometry v = r.GetGeometry(Size(120, 420));
    t.Expect(v.track.GetHeight() > v.track.GetWidth(), "vertical mode rotates the major axis");
    t.Expect(v.segments[0].rect.top == v.content.top && v.segments[3].rect.bottom == v.content.bottom,
             "vertical segments cover the complete content extent");
    t.Expect(v.boundaries[0].y < v.boundaries[1].y && v.boundaries[1].y < v.boundaries[2].y,
             "vertical boundaries preserve scalar ordering from top to bottom");

    r.SetDirection(UiDirection::H).SetReverse(true);
    UiRangeSegments::Geometry reversed = r.GetGeometry(Size(420, 90));
    t.Expect(r.IsReversed(), "reverse presentation is explicit control state");
    t.Expect(reversed.segments[0].rect.right == reversed.content.right &&
             reversed.segments[3].rect.left == reversed.content.left,
             "reverse presentation flips visual segment order without changing semantic indexes");
    t.Expect(reversed.boundaries[0].x > reversed.boundaries[1].x && reversed.boundaries[1].x > reversed.boundaries[2].x,
             "reverse presentation flips boundary projection while scalar values remain ordered");
    t.Expect(Near(r.GetBoundaryValue(0), 25.0) && Near(r.GetBoundaryValue(2), 75.0),
             "reverse presentation never mutates scalar thresholds");
}

static void TestPaletteThemeAndOverrides(TestCtx& t)
{
    t.Section("Palette, theme and overrides");

    UiThemeContext saved = UiTheme::GetContext();
    UiThemeContext light = saved;
    light.preset = UiThemePreset::Minimal;
    light.mode = UiThemeMode::Light;
    UiTheme::Set(light);

    UiRangeSegments r;
    r.SetSegmentCount(4);
    t.Expect(r.GetRole() == UiRole::Standard, "default semantic role is Standard");

    Vector<Color> anchors;
    anchors.Add(Color(255, 0, 0));
    anchors.Add(Color(0, 255, 0));
    anchors.Add(Color(0, 0, 255));
    r.SetPalette(anchors).SetPaletteMode(UiRangeSegments::PaletteMode::Gradient);
    UiRangeSegments::Geometry gradient = r.GetGeometry(Size(400, 90));
    t.Expect(gradient.segments[0].color == anchors[0], "gradient palette begins at the first authored anchor");
    t.Expect(gradient.segments[3].color == anchors[2], "gradient palette ends at the last authored anchor");
    t.Expect(gradient.segments[1].color != gradient.segments[0].color && gradient.segments[1].color != gradient.segments[3].color,
             "intermediate segment colours are deterministic interpolated samples");

    UiRangeSegment explicit_segment = r.GetSegment(1);
    explicit_segment.color = Color(7, 8, 9);
    r.SetSegment(1, explicit_segment);
    t.Expect(r.GetGeometry(Size(400, 90)).segments[1].color == Color(7, 8, 9),
             "explicit per-segment colour overrides automatic palette resolution");

    UiRangeSegments themed;
    themed.SetRole(UiRole::Alert).SetSegmentCount(3);
    UiSlider::Style alert = UiTheme::ResolveSlider(UiRole::Alert);
    Color alert_primary = alert.track_palette.ink[ST_NORMAL];
    t.Expect(themed.GetGeometry(Size(400, 90)).segments[0].color == alert_primary,
             "semantic role drives the inherited first series colour");

    UiRangeSegments::Style local = themed.GetStyle();
    local.series[0] = Color(12, 34, 56);
    themed.SetCustomStyle(local);
    UiThemeContext dark = light;
    dark.mode = UiThemeMode::Dark;
    UiTheme::Set(dark);
    t.Expect(themed.GetGeometry(Size(400, 90)).segments[0].color == Color(12, 34, 56),
             "custom style remains explicit across later theme revisions");
    themed.ClearCustomStyle();
    t.Expect(themed.GetGeometry(Size(400, 90)).segments[0].color != Color(12, 34, 56),
             "clearing custom style restores live role/theme inheritance");

    UiTheme::Set(saved);
}

static void TestSelectionAndPaint(TestCtx& t)
{
    t.Section("Selection and paint smoke");

    UiRangeSegments r;
    r.SetSelectedSegment(2).SetActiveBoundary(1);
    t.Expect(r.GetSelectedSegment() == 2, "selected segment is independently addressable");
    t.Expect(r.GetActiveBoundary() == 1, "active boundary is independently addressable");
    r.SetSelectedSegment(99).SetActiveBoundary(99);
    t.Expect(r.GetSelectedSegment() == -1 && r.GetActiveBoundary() == -1,
             "invalid public selection indexes normalize to none");

    r.SetRect(0, 0, 420, 90);
    ImageDraw draw(420, 90);
    r.Paint(draw);
    t.Expect(true, "horizontal paint completes without assertions");

    r.SetDirection(UiDirection::V).SetRect(0, 0, 120, 420);
    ImageDraw draw_vertical(120, 420);
    r.Paint(draw_vertical);
    t.Expect(true, "vertical paint completes without assertions");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestDataAndNormalization(t);
    TestBoundaryEditing(t);
    TestStructureMutation(t);
    TestDataBinding(t);
    TestGeometry(t);
    TestPaletteThemeAndOverrides(t);
    TestSelectionAndPaint(t);

    Cout() << "\nUIRANGESEGMENTS_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << "\n";
    SetExitCode(t.fails ? 1 : 0);
}
