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

    void Section(const String& title)
    {
        Cout() << "\n=== " << title << " ===\n";
    }
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

static void TestValueContract(TestCtx& t)
{
    t.Section("Value contract");

    UiProgressRing ring;
    t.Expect(ring.Get() == 0 && ring.GetTotal() == 100, "default value is 0 / 100");
    t.Expect(ring.IsPercentShown(), "percentage text is enabled by default");

    ring.Set(68, 100);
    t.Expect(ring.Get() == 68, "Set stores actual value");
    t.Expect(ring.GetTotal() == 100, "Set stores total");
    t.Expect(ring.GetPercent() == 68, "68 / 100 reports 68 percent");
    t.Expect(Near(ring.GetRatio(), 0.68), "ratio preserves sub-percent geometry precision");

    ring.Set(17, 250);
    t.Expect(ring.GetPercent() == 7, "percentage readout rounds arbitrary actual / total values");
    t.Expect(Near(ring.GetRatio(), 17.0 / 250.0), "ring sweep is not limited to integer percent steps");

    ring.Set(400, 250);
    t.Expect(ring.Get() == 250, "actual clamps to total");
    ring.Set(-4, 250);
    t.Expect(ring.Get() == 0, "actual clamps to zero");

    ring.Set(1, 4);
    ++ring;
    t.Expect(ring.Get() == 2, "prefix increment advances");
    int old = ring++;
    t.Expect(old == 2 && ring.Get() == 3, "postfix increment returns old value");
    ring += 10;
    t.Expect(ring.Get() == 4, "operator += clamps to total");

    ring.SetData(2);
    t.Expect((int)ring.GetData() == 2, "SetData/GetData follows the integer progress value");
}

static void TestGeometry(TestCtx& t)
{
    t.Section("Geometry");

    UiProgressRing ring;
    t.Expect(ring.GetCapRoundness() == 100, "cap roundness defaults to fully rounded");
    ring.AnimateOnShow(false).Set(68, 100);
    UiProgressRing::Geometry g = ring.GetGeometry(Size(180, 100));
    t.Expect(g.square == RectC(40, 0, 100, 100), "non-square allocation centers a square ring viewport");
    t.Expect(Near(g.target_ratio, 0.68), "geometry retains target ratio");
    t.Expect(Near(g.display_ratio, 0.68), "non-animated geometry paints target ratio");
    t.Expect(Near(g.sweep_angle, 0.68 * 2.0 * M_PI), "determinate sweep follows the exact ratio");
    t.Expect(g.radius > 0.0, "normal ring has positive path radius");
    t.Expect(g.text_rect.GetWidth() == g.text_rect.GetHeight(), "center text uses square inner geometry");

    ring.SetThickness(40).SetCapRoundness(100).SetRingInset(5);
    g = ring.GetGeometry(Size(120, 120));
    t.Expect(g.thickness == 40, "geometry uses authored thickness");
    t.Expect(g.cap_roundness == 100, "full cap roundness is independent of stroke thickness");
    t.Expect(g.radius < 40.0, "thickness and inset reserve paint room inside bounds");

    ring.SetCapRoundness(50);
    g = ring.GetGeometry(Size(120, 120));
    t.Expect(g.cap_roundness == 50, "intermediate cap roundness is stored as a percentage");

    ring.SetCapRoundness(-20);
    t.Expect(ring.GetCapRoundness() == 0, "cap roundness clamps to zero");
    ring.SetCapRoundness(140);
    t.Expect(ring.GetCapRoundness() == 100, "cap roundness clamps to one hundred");

    UiProgressRing::Geometry tiny = ring.GetGeometry(Size(12, 8));
    t.Expect(tiny.square == RectC(2, 0, 8, 8), "tiny rectangular allocation still centers the square viewport");
    t.Expect(tiny.radius >= 0.0, "tiny allocation never creates negative radius");
}

static void TestStyleAndGradient(TestCtx& t)
{
    t.Section("Style and gradient");

    UiProgressRing ring;
    ring.SetProgressColor(Color(20, 90, 210));
    t.Expect(ring.HasCustomStyle(), "direct progress colour creates a local style");
    t.Expect(!ring.GetStyle().gradient_enabled, "SetProgressColor produces a solid progress stroke");
    t.Expect(FaceColor(ring.GetStyle().progress_palette, ST_NORMAL) == Color(20, 90, 210), "solid progress colour is stored");

    ring.SetProgressGradient(Color(30, 120, 245), Color(80, 40, 220));
    t.Expect(ring.GetStyle().gradient_enabled, "progress gradient can be enabled");
    t.Expect(FaceColor(ring.GetStyle().progress_palette, ST_NORMAL) == Color(30, 120, 245), "gradient start colour is stored");
    t.Expect(ring.GetStyle().gradient_end[ST_NORMAL] == Color(80, 40, 220), "gradient end colour is stored");

    ring.SetTrackColor(Color(210, 214, 220));
    t.Expect(FaceColor(ring.GetStyle().track_palette, ST_NORMAL) == Color(210, 214, 220), "unused track has an independent colour");

    ring.SetTextColor(Color(10, 20, 30));
    t.Expect(ring.GetStyle().text_palette.ink[ST_NORMAL] == Color(10, 20, 30), "center text colour is independent");

    ring.ClearProgressGradient();
    t.Expect(!ring.GetStyle().gradient_enabled, "gradient can be cleared without changing progress value");

    ring.ClearCustomStyle();
    t.Expect(!ring.HasCustomStyle(), "ClearCustomStyle restores theme-driven style");
}

static void TestTextSizing(TestCtx& t)
{
    t.Section("Center text sizing");

    UiProgressRing ring;
    ring.AnimateOnShow(false).Set(100, 100).SetFontSize(42);
    UiProgressRing::Geometry normal = ring.GetGeometry(Size(90, 90));
    t.Expect(normal.text_visible, "percentage text remains visible when a smaller fitted font can be used");
    t.Expect(normal.text_font_height > 0 && normal.text_font_height <= 42, "fitted font never grows beyond the authored size");

    UiProgressRing::Geometry small = ring.GetGeometry(Size(32, 32));
    t.Expect(small.text_font_height <= normal.text_font_height, "shrinking the ring never increases center font size");

    ring.SetText("Done");
    t.Expect(ring.GetText() == "Done", "custom center text is stored");
    ring.ClearText().NoPercent();
    t.Expect(ring.GetText().IsEmpty() && !ring.IsPercentShown(), "custom and percentage text can both be disabled");
}

static void TestAnimationStates(TestCtx& t)
{
    t.Section("Animation states");

    UiProgressRing ring;
    ring.Set(68, 100);
    double semantic = ring.GetRatio();
    ring.RestartIntroAnimation();
    t.Expect(Near(ring.GetRatio(), semantic), "intro animation never changes semantic value");
    t.Expect(!ring.IsAnimationRunning(), "closed control does not start a timer");

    ring.SetIndeterminate(true);
    t.Expect(ring.IsIndeterminate(), "SetIndeterminate enters unknown-total state");
    UiProgressRing::Geometry gi = ring.GetGeometry(Size(80, 80));
    t.Expect(gi.indeterminate, "geometry marks indeterminate state");
    t.Expect(gi.sweep_angle > 0.0 && gi.sweep_angle < 2.0 * M_PI, "indeterminate state uses a bounded moving arc");

    ring.SetIndeterminate(false);
    t.Expect(!ring.IsIndeterminate(), "SetIndeterminate(false) returns to determinate state");
    t.Expect(!ring.IsAnimationRunning(), "closed determinate control has no active timer");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestValueContract(t);
    TestGeometry(t);
    TestStyleAndGradient(t);
    TestTextSizing(t);
    TestAnimationStates(t);

    Cout() << "\nUIPROGRESSRING_SUMMARY checks=" << t.checks << " failed=" << t.fails << "\n";
    SetExitCode(t.fails ? 1 : 0);
}
