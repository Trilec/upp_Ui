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

static bool Near(double a, double b)
{
    return std::fabs(a - b) < 1e-12;
}

static void TestValueContract(TestCtx& t)
{
    t.Section("Value contract");

    UiRangeSlider s;
    t.Expect(Near(s.GetMin(), 0.0) && Near(s.GetMax(), 100.0), "default domain is 0..100");
    t.Expect(Near(s.GetLowerValue(), 0.0) && Near(s.GetUpperValue(), 100.0),
             "default selection spans the full domain");

    s.SetValues(20, 80);
    t.Expect(Near(s.GetLowerValue(), 20) && Near(s.GetUpperValue(), 80),
             "SetValues stores lower and upper values");

    s.SetValues(90, 10);
    t.Expect(Near(s.GetLowerValue(), 10) && Near(s.GetUpperValue(), 90),
             "reversed pair is normalized into ordered values");

    s.SetLowerValue(95);
    t.Expect(Near(s.GetLowerValue(), 90), "lower value cannot cross upper value");

    s.SetUpperValue(5);
    t.Expect(Near(s.GetUpperValue(), 90), "upper value cannot cross lower value");

    s.SetRange(60, 40);
    t.Expect(Near(s.GetMin(), 40) && Near(s.GetMax(), 60), "reversed domain is normalized");
    t.Expect(Near(s.GetLowerValue(), 60) && Near(s.GetUpperValue(), 60),
             "selection clamps when the domain contracts");
}

static void TestStepAndAliases(TestCtx& t)
{
    t.Section("Step and aliases");

    UiRangeSlider s;
    s.SetRange(0, 10).SetStep(2).SetValues(3.1, 7.2);
    t.Expect(Near(s.GetLowerValue(), 4) && Near(s.GetUpperValue(), 8),
             "both handles honor step quantization");

    s.SetStartEnd(2, 6);
    t.Expect(Near(s.GetStart(), 2) && Near(s.GetEnd(), 6),
             "start/end aliases use the same authoritative values");

    s.SetStart(4).SetEnd(8);
    t.Expect(Near(s.GetLowerValue(), 4) && Near(s.GetUpperValue(), 8),
             "individual start/end aliases update lower/upper");
}

static void TestDataBinding(TestCtx& t)
{
    t.Section("Data binding");

    UiRangeSlider s;
    ValueArray input;
    input.Add(15.0);
    input.Add(75.0);
    s.SetData(input);

    t.Expect(Near(s.GetLowerValue(), 15) && Near(s.GetUpperValue(), 75),
             "SetData accepts a two-value ValueArray");

    Value value = s.GetData();
    t.Expect(value.Is<ValueArray>(), "GetData returns a ValueArray");
    ValueArray output = value;
    t.Expect(output.GetCount() == 2, "GetData returns exactly two values");
    t.Expect(Near((double)output[0], 15) && Near((double)output[1], 75),
             "GetData preserves lower/upper ordering");

    s.SetData(42);
    t.Expect(Near(s.GetLowerValue(), 15) && Near(s.GetUpperValue(), 75),
             "non-array SetData input is ignored");
}

static void TestInteractionContract(TestCtx& t)
{
    t.Section("Interaction contract");

    UiRangeSlider s;
    s.SetRange(0, 10).SetStep(1).SetValues(2, 8);

    int changing = 0;
    int actions = 0;
    s.WhenChanging = [&] { changing++; };
    s.WhenAction = [&] { actions++; };

    s.SetActiveHandle(UiRangeSlider::Handle::Lower);
    t.Expect(s.Key(K_RIGHT, 1), "right key is handled for horizontal range slider");
    t.Expect(Near(s.GetLowerValue(), 3) && Near(s.GetUpperValue(), 8),
             "keyboard changes only the active lower handle");
    t.Expect(changing == 1 && actions == 1, "keyboard emits changing and action once");

    s.SetActiveHandle(UiRangeSlider::Handle::Upper);
    s.MouseWheel(Point(0, 0), -120, 0);
    t.Expect(Near(s.GetLowerValue(), 3) && Near(s.GetUpperValue(), 7),
             "mouse wheel changes only the active upper handle");
    t.Expect(changing == 2 && actions == 2, "wheel emits changing and action once");

    s.SetValues(5, 5).SetActiveHandle(UiRangeSlider::Handle::Lower);
    changing = actions = 0;
    s.Key(K_RIGHT, 1);
    t.Expect(Near(s.GetLowerValue(), 5) && Near(s.GetUpperValue(), 5),
             "active lower handle cannot cross a coincident upper handle");
    t.Expect(changing == 0 && actions == 0, "blocked movement emits no value events");

    s.Disable();
    t.Expect(!s.Key(K_LEFT, 1), "disabled range slider rejects keyboard editing");
    s.MouseWheel(Point(0, 0), 120, 0);
    t.Expect(Near(s.GetLowerValue(), 5) && Near(s.GetUpperValue(), 5),
             "disabled range slider rejects wheel editing");
}

static void TestAdjustableBounds(TestCtx& t)
{
    t.Section("Adjustable bounds");

    UiRangeSlider s;
    t.Expect(!s.HasAdjustableBounds(), "adjustable bounds are opt-in");

    s.SetActiveHandle(UiRangeSlider::Handle::LowerBound);
    t.Expect(s.GetActiveHandle() == UiRangeSlider::Handle::Lower,
             "bound handle requests normalize to lower selection while bounds are disabled");
    s.SetActiveHandle(UiRangeSlider::Handle::UpperBound);
    t.Expect(s.GetActiveHandle() == UiRangeSlider::Handle::Upper,
             "upper bound requests normalize to upper selection while bounds are disabled");

    s.SetRange(0, 1000).EnableAdjustableBounds().SetBounds(50, 900).SetValues(250, 680);
    t.Expect(s.HasAdjustableBounds(), "adjustable bounds can be enabled without changing default instances");
    t.Expect(Near(s.GetLowerBound(), 50) && Near(s.GetUpperBound(), 900),
             "inner bounds are independent from the hard domain");
    t.Expect(Near(s.GetLowerValue(), 250) && Near(s.GetUpperValue(), 680),
             "selection remains independent inside adjustable bounds");

    s.SetActiveHandle(UiRangeSlider::Handle::LowerBound);
    s.Key(K_RIGHT, 1);
    t.Expect(Near(s.GetLowerBound(), 51), "keyboard interaction moves the lower bound handle");
    s.SetActiveHandle(UiRangeSlider::Handle::UpperBound);
    s.Key(K_LEFT, 1);
    t.Expect(Near(s.GetUpperBound(), 899), "keyboard interaction moves the upper bound handle");

    s.SetActiveHandle(UiRangeSlider::Handle::LowerBound);
    s.EnableAdjustableBounds(false);
    t.Expect(s.GetActiveHandle() == UiRangeSlider::Handle::Lower,
             "disabling bounds returns a lower-bound active handle to the lower selection");
    const double lower_before = s.GetLowerValue();
    s.Key(K_RIGHT, 1);
    t.Expect(s.GetLowerValue() > lower_before,
             "keyboard editing remains live after adjustable bounds are disabled");

    s.EnableAdjustableBounds().SetBounds(400, 600).SetValues(400, 600);
    t.Expect(Near(s.GetLowerValue(), 400) && Near(s.GetUpperValue(), 600),
             "contracting bounds clamps the selected interval");
}

static void TestStyleAndSizing(TestCtx& t)
{
    t.Section("Style and sizing");

    UiRangeSlider s;
    t.Expect(!s.HasCustomStyle(), "range slider starts theme-driven");
    t.Expect(s.AreEndpointMarkersShown(), "range endpoints are visible by default");
    s.ShowEndpointMarkers(false);
    t.Expect(!s.AreEndpointMarkersShown(), "endpoint marker visibility is configurable");
    s.ShowEndpointMarkers();
    t.Expect(s.AreEndpointMarkersShown(), "endpoint markers can be restored fluently");

    UiRangeSlider::Style style = UiTheme::ResolveSlider();
    s.SetCustomStyle(style);
    t.Expect(s.HasCustomStyle(), "custom slider style is accepted");

    s.ClearCustomStyle();
    t.Expect(!s.HasCustomStyle(), "ClearCustomStyle restores theme-driven styling");

    s.SetTicks(true, 6, 1);
    t.Expect(s.HasCustomStyle(), "tick configuration edits the shared slider style");
    t.Expect(s.GetStyle().show_ticks && s.GetStyle().major_ticks == 6
             && s.GetStyle().minor_ticks_per_major == 1,
             "tick configuration is retained");

    Size horizontal = s.GetMinSize();
    s.SetDirection(UiDirection::V);
    Size vertical = s.GetMinSize();
    t.Expect(horizontal.cx > horizontal.cy, "horizontal natural size is wider than tall");
    t.Expect(vertical.cy > vertical.cx, "vertical natural size is taller than wide");
}

static void TestAllocatedTrackExpansion(TestCtx& t)
{
    t.Section("Allocated track expansion");

    UiRangeSlider s;
    s.SetDirection(UiDirection::H);
    s.SetRect(0, 0, 220, 50);
    Rect narrow = s.GetTrackRect();
    s.SetRect(0, 0, 520, 50);
    Rect wide = s.GetTrackRect();
    t.Expect(wide.GetWidth() > narrow.GetWidth() + 250,
             "horizontal painted track expands with allocated width");
    t.Expect(wide.left < 40 && 520 - wide.right < 40,
             "horizontal painted track stays close to allocated edges");

    s.SetDirection(UiDirection::V);
    s.SetRect(0, 0, 50, 220);
    Rect short_track = s.GetTrackRect();
    s.SetRect(0, 0, 50, 520);
    Rect tall_track = s.GetTrackRect();
    t.Expect(tall_track.GetHeight() > short_track.GetHeight() + 250,
             "vertical painted track expands with allocated height");
    t.Expect(tall_track.top < 40 && 520 - tall_track.bottom < 40,
             "vertical painted track stays close to allocated edges");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestValueContract(t);
    TestStepAndAliases(t);
    TestDataBinding(t);
    TestInteractionContract(t);
    TestAdjustableBounds(t);
    TestStyleAndSizing(t);
    TestAllocatedTrackExpansion(t);

    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << "\n";
    SetExitCode(t.fails ? 1 : 0);
}
