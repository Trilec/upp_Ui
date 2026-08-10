#include <Core/Core.h>
#include <Ui/Ui.h>
#include <cmath>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool condition, const String& message)
    {
        checks++;
        if(!condition) {
            fails++;
            Cout() << "[FAIL] " << message << "\n";
        }
    }
};

static bool Near(double a, double b)
{
    return std::fabs(a - b) < 1e-12;
}

CONSOLE_APP_MAIN
{
    TestCtx t;

    UiRangeSliderEdit edit;
    t.Expect(Near(edit.GetLowerValue(), 25.0) && Near(edit.GetUpperValue(), 75.0),
             "default interval is 25..75");
    t.Expect(Near(edit.GetMin(), 0.0) && Near(edit.GetMax(), 100.0),
             "default domain is 0..100");

    edit.SetRange(-10, 10).SetStep(0.5).SetValues(-3.2, 6.3);
    t.Expect(Near(edit.GetMin(), -10.0) && Near(edit.GetMax(), 10.0),
             "domain is forwarded to the range slider");
    t.Expect(Near(edit.GetLowerValue(), -3.0) && Near(edit.GetUpperValue(), 6.5),
             "both values use range-slider quantization");
    t.Expect(Near(edit.LowerField().GetValue(), -3.0) &&
             Near(edit.UpperField().GetValue(), 6.5),
             "programmatic range values synchronize both fields");

    edit.SetValues(8, -4);
    t.Expect(Near(edit.GetLowerValue(), -4.0) && Near(edit.GetUpperValue(), 8.0),
             "reversed pair is normalized by the authoritative range slider");
    t.Expect(Near(edit.GetStart(), -4.0) && Near(edit.GetEnd(), 8.0),
             "start/end aliases expose the same interval");

    ValueArray pair;
    pair.Add(-2.0);
    pair.Add(5.0);
    edit.SetData(pair);
    Value data = edit.GetData();
    t.Expect(data.Is<ValueArray>(), "GetData returns a ValueArray");
    ValueArray out = data;
    t.Expect(out.GetCount() == 2 && Near((double)out[0], -2.0) && Near((double)out[1], 5.0),
             "ValueArray binding preserves lower/upper ordering");

    edit.SetData(42);
    t.Expect(Near(edit.GetLowerValue(), -2.0) && Near(edit.GetUpperValue(), 5.0),
             "non-array SetData is ignored");

    int changing = 0;
    int actions = 0;
    edit.WhenChanging = [&] { changing++; };
    edit.WhenAction = [&] { actions++; };

    edit.LowerField().SetValue(1.0);
    edit.LowerField().WhenChange();
    t.Expect(Near(edit.GetLowerValue(), 1.0) && Near(edit.LowerField().GetValue(), 1.0),
             "lower field live change synchronizes the slider");
    t.Expect(changing == 1 && actions == 0,
             "field preview emits changing without premature commit");
    edit.LowerField().WhenAction();
    t.Expect(changing == 1 && actions == 1,
             "field commit emits one action after preview");

    edit.UpperField().SetValue(0.0);
    edit.UpperField().WhenChange();
    t.Expect(Near(edit.GetLowerValue(), 0.0) && Near(edit.GetUpperValue(), 1.0),
             "crossing field input is normalized and reflected back into both fields");
    edit.UpperField().WhenAction();
    t.Expect(actions == 2, "second field edit commits once");

    edit.SetDirection(UiDirection::H);
    Size horizontal = edit.GetMinSize();
    edit.SetDirection(UiDirection::V);
    Size vertical = edit.GetMinSize();
    t.Expect(horizontal.cx > horizontal.cy, "horizontal composition is naturally wider than tall");
    t.Expect(vertical.cy > vertical.cx, "vertical composition is naturally taller than wide");

    edit.SetRect(0, 0, 120, 320);
    edit.Layout();
    Rect upper = edit.UpperField().GetRect();
    Rect lower = edit.LowerField().GetRect();
    Rect slider = edit.Slider().GetRect();
    t.Expect(upper.top < slider.top && slider.bottom < lower.bottom,
             "vertical layout places upper field above slider and lower field below");

    edit.SetDirection(UiDirection::H);
    edit.SetFieldWidth(DPI(72)).SetGap(DPI(8)).SetInset(DPI(5));
    edit.SetRect(0, 0, 620, 60);
    edit.Layout();
    Rect left = edit.LowerField().GetRect();
    Rect right = edit.UpperField().GetRect();
    slider = edit.Slider().GetRect();
    t.Expect(left.right + DPI(8) == slider.left && slider.right + DPI(8) == right.left,
             "horizontal layout uses exactly the configured gap around the expanding slider");
    t.Expect(left.left == DPI(5) && right.right == 620 - DPI(5),
             "inset bounds the complete field/slider composition");
    t.Expect(slider.GetWidth() == 620 - 2 * DPI(5) - 2 * left.GetWidth() - 2 * DPI(8),
             "slider receives all remaining horizontal space after fixed fields and gaps");
    Rect painted = edit.Slider().GetTrackRect();
    t.Expect(painted.GetWidth() > slider.GetWidth() - DPI(50),
             "painted range track expands across the allocated slider rectangle");

    edit.SetDirection(UiDirection::V);
    edit.SetRect(0, 0, 120, 420);
    edit.Layout();
    upper = edit.UpperField().GetRect();
    lower = edit.LowerField().GetRect();
    slider = edit.Slider().GetRect();
    t.Expect(upper.bottom + DPI(8) == slider.top && slider.bottom + DPI(8) == lower.top,
             "vertical layout uses exactly the configured gap around the expanding slider");
    painted = edit.Slider().GetTrackRect();
    t.Expect(painted.GetHeight() > slider.GetHeight() - DPI(50),
             "painted range track expands across the allocated vertical slider rectangle");

    edit.SetPrecision(2);
    t.Expect(edit.GetFieldWidth() == DPI(72) && edit.GetGap() == DPI(8) && edit.GetInset() == DPI(5),
             "field width, gap, and inset remain queryable composition settings");
    t.Expect(&edit.Slider() != nullptr && &edit.LowerField() != nullptr &&
             &edit.UpperField() != nullptr,
             "child controls remain available for normal Ui styling and configuration");

    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << "\n";
    SetExitCode(t.fails ? 1 : 0);
}
