#include <Core/Core.h>
#include <Ui/Ui.h>

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

static void TestValueContract(TestCtx& t)
{
    t.Section("Value contract");

    UiProgressBar p;
    t.Expect(p.Get() == 0 && p.GetTotal() == 100, "default value is 0 / 100");

    p.Set(50, 100);
    t.Expect(p.Get() == 50, "Set(actual,total) stores actual");
    t.Expect(p.GetTotal() == 100, "Set(actual,total) stores total");
    t.Expect(p.GetPercent() == 50, "50 / 100 reports 50 percent");

    p.Set(150, 100);
    t.Expect(p.Get() == 100, "actual clamps to total");

    p.Set(-4, 100);
    t.Expect(p.Get() == 0, "actual clamps to zero");

    p.SetTotal(0);
    t.Expect(p.IsIndeterminate(), "total zero is indeterminate");
    t.Expect(p.GetPercent() == 0, "indeterminate has no percentage");

    p.Set(5, -10);
    t.Expect(p.IsIndeterminate(), "negative total is indeterminate");
    t.Expect(p.Get() == 5, "indeterminate keeps non-negative actual");

    p.Set(1, 4);
    ++p;
    t.Expect(p.Get() == 2, "prefix increment advances");
    int old = p++;
    t.Expect(old == 2 && p.Get() == 3, "postfix increment returns old value");
    p += 10;
    t.Expect(p.Get() == 4, "operator += clamps to total");
}

static void TestGeometry(TestCtx& t)
{
    t.Section("Geometry");

    UiProgressBar p;
    p.Set(0, 100);
    UiProgressBar::Geometry g0 = p.GetGeometry(Size(200, 20));
    t.Expect(!g0.vertical, "wide auto geometry is horizontal");
    t.Expect(g0.fill.GetWidth() == 0, "0 percent fill width is zero");

    p.Set(50, 100);
    UiProgressBar::Geometry g50 = p.GetGeometry(Size(200, 20));
    t.Expect(g50.fill.GetWidth() == 100, "50 percent fill width is half");

    p.Set(100, 100);
    UiProgressBar::Geometry g100 = p.GetGeometry(Size(200, 20));
    t.Expect(g100.fill.GetWidth() == 200, "100 percent fill width is full");

    p.SetOrientation(UiProgressBar::Orientation::Vertical).Set(50, 100);
    UiProgressBar::Geometry gv = p.GetGeometry(Size(20, 200));
    t.Expect(gv.vertical, "explicit vertical geometry is vertical");
    t.Expect(gv.fill.GetHeight() == 100, "vertical 50 percent fill height is half");
    t.Expect(gv.fill.bottom == gv.track.bottom, "vertical fill grows bottom-to-top");

    p.SetOrientation(UiProgressBar::Orientation::Auto);
    UiProgressBar::Geometry ga = p.GetGeometry(Size(20, 200));
    t.Expect(ga.vertical, "tall auto geometry is vertical");
}

static void TestTextAndIndeterminate(TestCtx& t)
{
    t.Section("Text and indeterminate");

    UiProgressBar p;
    p.Percent();
    p.Set(25, 100);
    t.Expect(p.IsPercentShown(), "Percent enables percentage text");

    p.SetIndeterminate(true);
    t.Expect(p.IsIndeterminate(), "SetIndeterminate enters indeterminate state");
    t.Expect(p.GetGeometry(Size(160, 16)).indeterminate, "geometry marks indeterminate");
    t.Expect(p.GetGeometry(Size(160, 16)).fill.GetWidth() > 0, "indeterminate geometry has moving chunk");

    p.SetIndeterminate(false);
    t.Expect(!p.IsIndeterminate(), "SetIndeterminate(false) returns to determinate");

    p.SetText("Working");
    t.Expect(p.GetText() == "Working", "custom text is stored");
    p.ClearText();
    t.Expect(p.GetText().IsEmpty(), "custom text clears");
}

static void TestStyleLifecycle(TestCtx& t)
{
    t.Section("Style lifecycle");

    UiProgressBar p;
    UiProgressBar::Style s = UiTheme::ResolveProgressBar(UiRole::Alert);
    p.SetCustomStyle(s);
    t.Expect(p.HasCustomStyle(), "custom style flag set");

    p.SetColor(Red());
    t.Expect(p.HasCustomStyle(), "SetColor keeps custom style");

    p.ClearCustomStyle();
    t.Expect(!p.HasCustomStyle(), "ClearCustomStyle restores theme style");

    p.SetIndeterminate(true);
    p.Hide();
    p.Show();
    p.SetIndeterminate(false);
    t.Expect(!p.IsAnimationRunning(), "determinate state stops animation");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestValueContract(t);
    TestGeometry(t);
    TestTextAndIndeterminate(t);
    TestStyleLifecycle(t);

    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << "\n";
    SetExitCode(t.fails ? 1 : 0);
}
