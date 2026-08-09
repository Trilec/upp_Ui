#include <Core/Core.h>
#include <Ui/Ui.h>
#include <Ui/ColorPicker/UiColorPicker.h>

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

    void Section(const char *title)
    {
        Cout() << "\n=== " << title << " ===\n";
    }
};

static Vector<Color> EightColors()
{
    Vector<Color> colors;
    colors << Color(255, 0, 0) << Color(255, 128, 0)
           << Color(255, 255, 0) << Color(0, 190, 80)
           << Color(0, 150, 255) << Color(70, 70, 255)
           << Color(160, 80, 220) << Color(255, 80, 160);
    return colors;
}

static void TestValueContract(TestCtx& t)
{
    t.Section("UiColorMatrix value contract");
    UiColorMatrix matrix;
    t.Expect(matrix.GetColorCount() == 1, "default has one colour");

    Vector<Color> colors = EightColors();
    matrix.SetColors(colors);
    t.Expect(matrix.GetColorCount() == 8, "accepts eight colours");
    for(int i = 0; i < 8; i++)
        t.Expect(matrix.GetColor(i) == colors[i], Format("colour %d preserved", i));

    matrix.SetColorLabel(3, "Accent");
    t.Expect(matrix.GetColorLabel(3) == "Accent", "labels are independent of values");

    matrix.SetColorCount(99);
    t.Expect(matrix.GetColorCount() == UiColorMatrix::MAX_COLORS, "count clamps to maximum");
    matrix.SetColorCount(0);
    t.Expect(matrix.GetColorCount() == 1, "count clamps to one");

    matrix.SetData(Color(12, 34, 56));
    t.Expect(matrix.GetColorCount() == 1 && matrix.GetColor(0) == Color(12, 34, 56),
             "single Color SetData round-trips");

    ValueArray array;
    for(const Color& color : colors)
        array.Add(color);
    matrix.SetData(array);
    t.Expect(matrix.GetColorCount() == 8, "ValueArray restores eight colours");
    Value value = matrix.GetData();
    t.Expect(value.Is<ValueArray>(), "multi-colour GetData returns ValueArray");
    ValueArray roundtrip = value;
    t.Expect(roundtrip.GetCount() == 8, "ValueArray round-trip count");
    for(int i = 0; i < 8; i++)
        t.Expect(roundtrip[i] == Value(colors[i]), Format("ValueArray colour %d round-trips", i));
}

static void TestAdaptiveGeometry(TestCtx& t)
{
    t.Section("adaptive geometry");
    UiColorMatrix matrix;
    matrix.SetColors(EightColors()).SetSlotGap(DPI(4));

    matrix.SetRect(0, 0, DPI(430), DPI(70));
    Rect first = matrix.GetSlotRect(0);
    Rect last = matrix.GetSlotRect(7);
    t.Expect(!first.IsEmpty() && !last.IsEmpty(), "wide geometry produces visible slots");
    t.Expect(first.top == last.top, "eight colours fit on one row when width permits");
    for(int i = 0; i < 8; i++)
        t.Expect(matrix.HitTest(matrix.GetSlotRect(i).CenterPoint()) == i,
                 Format("wide hit test %d", i));

    matrix.SetRect(0, 0, DPI(130), DPI(130));
    first = matrix.GetSlotRect(0);
    last = matrix.GetSlotRect(7);
    t.Expect(first.top != last.top, "narrow geometry wraps to multiple rows");
    t.Expect(first.GetWidth() == first.GetHeight(), "slots remain square");
    for(int i = 0; i < 8; i++) {
        Rect r = matrix.GetSlotRect(i);
        t.Expect(!r.IsEmpty() && r.GetWidth() == r.GetHeight(),
                 Format("wrapped slot %d is square", i));
        t.Expect(matrix.HitTest(r.CenterPoint()) == i,
                 Format("wrapped hit test %d", i));
    }
}

static void TestStyleAndSelection(TestCtx& t)
{
    t.Section("style and selection");
    UiColorMatrix matrix;
    matrix.SetColors(EightColors());
    int selected = -1;
    matrix.WhenSelect = [&](int index) { selected = index; };

    matrix.SetActiveIndex(5, true);
    t.Expect(matrix.GetActiveIndex() == 5 && selected == 5, "active selection event is post-commit");

    matrix.SetSlotGap(DPI(7))
          .SetSlotRadius(DPI(9))
          .SetSlotFrameWidth(DPI(2))
          .ShowSlotFrame(true)
          .SetSlotShadow(true)
          .SetSurfaceRadius(DPI(8))
          .ShowSurface(true)
          .ShowSurfaceFrame(true)
          .SetSurfaceShadow(true);
    t.Expect(matrix.HasCustomStyle(), "style setters materialize an explicit custom style");
    t.Expect(matrix.GetCustomStyle().slot_gap == DPI(7), "custom gap stored");
    t.Expect(matrix.GetCustomStyle().slot_metrics.radius == DPI(9), "custom slot radius stored");
    t.Expect(matrix.GetCustomStyle().slot_metrics.shadow.enabled, "slot shadow uses standard StyledShadow");
    t.Expect(matrix.GetCustomStyle().surface_metrics.shadow.enabled, "surface shadow uses standard StyledShadow");
    matrix.ClearCustomStyle();
    t.Expect(!matrix.HasCustomStyle(), "custom style clears back to theme");
}

static void TestPickerEightSlots(TestCtx& t)
{
    t.Section("UiColorPicker eight-slot contract");
    UiColorPicker picker;
    picker.EnableSessionPersistence(false);
    picker.SetSlotCount(8);
    t.Expect(picker.GetSlotCount() == 8, "picker accepts eight active slots");

    Vector<Color> colors = EightColors();
    ValueArray input;
    for(const Color& color : colors)
        input.Add(color);
    picker.SetData(input);
    t.Expect(picker.GetSlotCount() == 8, "picker SetData keeps eight slots");
    for(int i = 0; i < 8; i++)
        t.Expect(picker.GetSlotColor(i) == colors[i], Format("picker slot %d round-trips", i));

    Value data = picker.GetData();
    t.Expect(data.Is<ValueArray>(), "eight-slot picker GetData returns ValueArray");
    ValueArray output = data;
    t.Expect(output.GetCount() == 8, "picker returns all eight slots");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestValueContract(t);
    TestAdaptiveGeometry(t);
    TestStyleAndSelection(t);
    TestPickerEightSlots(t);

    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << "\n";
    SetExitCode(t.fails ? 1 : 0);
}
