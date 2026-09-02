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
    void Section(const String& title) { Cout() << "\n=== " << title << " ===\n"; }
};

static bool Near(double a, double b, double eps = 0.0001)
{
    return fabs(a - b) <= eps;
}

static Color FaceColor(const StyledPalette& palette, StyledState state)
{
    const UiFill& fill = palette.face[state];
    return fill.IsSolid() ? fill.color : Color();
}

static void TestDataContract(TestCtx& t)
{
    t.Section("Data contract");
    UiRingChart chart;
    t.Expect(chart.GetSegmentCount() == 0, "chart starts empty");
    t.Expect(Near(chart.GetDataSum(), 0.0) && Near(chart.GetTotal(), 0.0), "empty chart has zero sum and total");

    chart.AddSegment(25, "Design")
         .AddSegment(35, "Build")
         .AddSegment(40, "Test");
    t.Expect(chart.GetSegmentCount() == 3, "segments are appended in authored order");
    t.Expect(Near(chart.GetDataSum(), 100.0), "automatic chart sum is exact");
    t.Expect(Near(chart.GetTotal(), 100.0) && !chart.HasExplicitTotal(), "automatic total normalizes against the data sum");
    t.Expect(chart.GetSegment(1).label == "Build", "segment labels are retained");

    chart.SetTotal(120.0);
    t.Expect(chart.HasExplicitTotal() && Near(chart.GetExplicitTotal(), 120.0), "explicit total is retained");
    t.Expect(Near(chart.GetTotal(), 120.0), "larger explicit total becomes the resolved total");

    chart.SetTotal(50.0);
    t.Expect(Near(chart.GetTotal(), 100.0), "explicit total never clips authored segment data");

    chart.ClearTotal();
    t.Expect(!chart.HasExplicitTotal() && Near(chart.GetTotal(), 100.0), "clearing total restores automatic normalization");

    chart.ClearSegments();
    t.Expect(chart.GetSegmentCount() == 0, "ClearSegments removes all data");
    chart.AddSegment(-10, "Invalid");
    t.Expect(Near(chart.GetSegment(0).value, 0.0), "negative segment values clamp to zero");
}

static void TestGeometryAndGaps(TestCtx& t)
{
    t.Section("Geometry and visible gaps");
    UiRingChart chart;
    chart.SetThickness(20).SetRingInset(4).SetCapRoundness(0).SetSegmentGap(0);
    chart.AddSegment(25).AddSegment(35).AddSegment(40);

    UiRingChart::Geometry g = chart.GetGeometry(Size(180, 120));
    t.Expect(g.square == RectC(30, 0, 120, 120), "non-square allocation centers a square chart viewport");
    t.Expect(g.radius > 0.0 && g.thickness == 20, "ring geometry resolves radius and thickness");
    t.Expect(Near(g.data_sum, 100.0) && Near(g.total, 100.0) && Near(g.remainder, 0.0), "geometry exposes normalized data totals");
    t.Expect(g.segments.GetCount() == 3, "geometry preserves authored segment count");
    t.Expect(g.segments[0].visible && g.segments[1].visible && g.segments[2].visible, "positive segments are visible");

    double end0 = g.segments[0].start_angle + g.segments[0].sweep_angle;
    t.Expect(Near(end0, g.segments[1].start_angle), "flat zero-gap segments meet exactly at their centerline boundary");
    double sweep_sum = 0.0;
    for(const UiRingChart::SegmentGeometry& sg : g.segments)
        sweep_sum += sg.sweep_angle;
    t.Expect(Near(sweep_sum, 2.0 * M_PI), "flat zero-gap full composition covers the complete circle");

    chart.SetCapRoundness(100);
    UiRingChart::Geometry rounded = chart.GetGeometry(Size(120, 120));
    double rounded_end0 = rounded.segments[0].start_angle + rounded.segments[0].sweep_angle;
    double centerline_gap = rounded.segments[1].start_angle - rounded_end0;
    t.Expect(centerline_gap > 0.0, "round zero-gap segments reserve centerline space for their cap extension");
    t.Expect(Near(centerline_gap * rounded.radius, rounded.thickness, 0.2),
             "round zero-gap centerline separation compensates two half-thickness caps");

    chart.SetSegmentGap(6);
    UiRingChart::Geometry spaced = chart.GetGeometry(Size(120, 120));
    double spaced_end0 = spaced.segments[0].start_angle + spaced.segments[0].sweep_angle;
    double spaced_centerline = spaced.segments[1].start_angle - spaced_end0;
    t.Expect(spaced_centerline > centerline_gap, "positive segment gap increases visual separation");
    t.Expect(Near(spaced_centerline * spaced.radius - spaced.thickness, 6.0, 0.3),
             "authored gap is measured between painted rounded ends, not centerlines");

    chart.SetTotal(125.0);
    UiRingChart::Geometry remainder = chart.GetGeometry(Size(120, 120));
    t.Expect(Near(remainder.total, 125.0) && Near(remainder.remainder, 25.0), "explicit larger total leaves themed remainder track");
}

static void TestStyleThemeAndText(TestCtx& t)
{
    t.Section("Style, theme and text");
    UiThemeContext saved = UiTheme::GetContext();
    UiThemeContext light = saved;
    light.preset = UiThemePreset::Minimal;
    light.mode = UiThemeMode::Light;
    UiTheme::Set(light);

    UiRingChart chart;
    chart.SetRole(UiRole::Alert).AddSegment(60).AddSegment(40, "Authored", Color(1, 2, 3));
    t.Expect(chart.GetRole() == UiRole::Alert, "chart stores semantic role");

    UiProgressBar::Style expected = UiTheme::ResolveProgressBar(UiRole::Alert);
    UiRingChart::Geometry g = chart.GetGeometry(Size(120, 120));
    t.Expect(g.segments[0].color == FaceColor(expected.fill_palette, ST_NORMAL), "first inherited series follows role primary colour");
    t.Expect(g.segments[1].color == Color(1, 2, 3), "per-segment authored colour overrides the series palette");

    UiThemeContext dark = light;
    dark.mode = UiThemeMode::Dark;
    UiTheme::Set(dark);
    UiProgressBar::Style dark_expected = UiTheme::ResolveProgressBar(UiRole::Alert);
    g = chart.GetGeometry(Size(120, 120));
    t.Expect(g.segments[0].color == FaceColor(dark_expected.fill_palette, ST_NORMAL), "theme-driven chart follows live dark-mode revision");

    chart.SetSeriesColor(0, Color(12, 34, 56));
    UiTheme::Set(light);
    g = chart.GetGeometry(Size(120, 120));
    t.Expect(g.segments[0].color == Color(12, 34, 56), "local series style survives later theme revisions");
    chart.ClearCustomStyle();
    g = chart.GetGeometry(Size(120, 120));
    t.Expect(g.segments[0].color == FaceColor(expected.fill_palette, ST_NORMAL), "clearing local chart style restores role-driven series colour");

    chart.SetCenterText("Team").SetFontSize(40);
    UiRingChart::Geometry text = chart.GetGeometry(Size(90, 90));
    t.Expect(text.text_visible && text.text_font_height > 0 && text.text_font_height <= 40, "center text fits down without growing past authored size");
    chart.ClearCenterText();
    t.Expect(chart.GetCenterText().IsEmpty(), "center text can be cleared independently of data");

    chart.SetCapRoundness(-4).SetSegmentGap(-3).SetRingInset(-2);
    t.Expect(chart.GetCapRoundness() == 0 && chart.GetSegmentGap() == 0 && chart.GetRingInset() == 0,
             "ring style geometry setters clamp invalid negative values");

    UiTheme::Set(saved);
}

static void TestRasterCache(TestCtx& t)
{
    t.Section("Stable raster cache");
    UiRasterCache::ClearTag("aa/ui-ring-chart");

    UiRingChart chart;
    chart.AddSegment(20).AddSegment(30).AddSegment(50).SetCenterText("100");
    chart.SetRect(0, 0, 139, 139);
    ImageDraw draw(139, 139);

    UiRasterCacheStats before = UiRasterCache::GetStats();
    chart.Paint(draw);
    UiRasterCacheStats first = UiRasterCache::GetStats();
    chart.Paint(draw);
    UiRasterCacheStats second = UiRasterCache::GetStats();

    t.Expect(first.misses > before.misses, "first chart paint populates shared exact ring cache");
    t.Expect(second.hits > first.hits, "repeated chart paint reuses cached ring composition");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestDataContract(t);
    TestGeometryAndGaps(t);
    TestStyleThemeAndText(t);
    TestRasterCache(t);

    Cout() << "\nUIRINGCHART_SUMMARY checks=" << t.checks << " failed=" << t.fails << "\n";
    SetExitCode(t.fails ? 1 : 0);
}
