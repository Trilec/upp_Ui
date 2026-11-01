#include <Core/Core.h>
#include <Ui/Ui.h>
#include <Ui/UiMeasure.h>

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
};

class FixedBlock : public Ctrl {
public:
    FixedBlock(int cx, int cy) : min_size(cx, cy) {}
    Size GetMinSize() const override { return min_size; }

private:
    Size min_size;
};

static void ConfigurePage(UiBoxLayout& page)
{
    page.SetDirection(UiDirection::H)
        .SetGap(DPI(4))
        .SetInset(DPI(4))
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetAlignItems(UiCrossAlign::Start);
}

int RunStackSuite()
{
    TestCtx t;

    UiBoxLayout home;
    UiBoxLayout review;
    ConfigurePage(home);
    ConfigurePage(review);

    FixedBlock a(DPI(180), DPI(44));
    FixedBlock b(DPI(180), DPI(44));
    FixedBlock c(DPI(180), DPI(44));
    FixedBlock d(DPI(180), DPI(44));
    FixedBlock e(DPI(180), DPI(44));
    FixedBlock f(DPI(180), DPI(44));

    home.Add(a).Fit();
    home.Add(b).Fit();
    home.Add(c).Fit();
    home.Add(d).Fit();
    review.Add(e).Fit();
    review.Add(f).Fit();

    UiStack stack;
    stack.AddPage(home, "home");
    stack.AddPage(review, "review");
    stack.SetActiveKey("home");

    const int wide = DPI(820);
    const int narrow = DPI(420);

    UiLayoutMeasureResult wide_measure = UiMeasureLayout(stack, { wide });
    UiLayoutMeasureResult narrow_measure = UiMeasureLayout(stack, { narrow });

    t.Expect(wide_measure.width_dependent,
             "UiStack reports width dependence from active wrapped page");
    t.Expect(narrow_measure.measured.cy > wide_measure.measured.cy,
             "narrow UiStack grows when active page wraps");
    t.Expect(narrow_measure.measured.cx == narrow,
             "UiStack preserves the supplied width");

    UiBoxLayout root;
    Ctrl workspace;
    root.SetDirection(UiDirection::V).SetGap(0).SetInset(0);
    root.Add(stack).Fit();
    root.Add(workspace).Expand(1);

    root.SetRect(0, 0, wide, DPI(500));
    root.Layout();
    Rect wide_stack = stack.GetRect();
    Rect wide_workspace = workspace.GetRect();

    root.SetRect(0, 0, narrow, DPI(500));
    root.Layout();
    Rect narrow_stack = stack.GetRect();
    Rect narrow_workspace = workspace.GetRect();

    t.Expect(narrow_stack.GetHeight() > wide_stack.GetHeight(),
             "Fit stack grows when the parent narrows");
    t.Expect(narrow_workspace.top > wide_workspace.top,
             "workspace moves down for wrapped active page");
    t.Expect(narrow_stack.bottom <= narrow_workspace.top,
             "stack and workspace do not overlap");

    stack.SetActiveKey("review");
    UiLayoutMeasureResult short_measure = UiMeasureLayout(stack, { narrow });
    t.Expect(short_measure.measured.cy < narrow_measure.measured.cy,
             "active shorter page reduces measured stack height");

    root.Layout();
    Rect review_stack = stack.GetRect();
    Rect review_workspace = workspace.GetRect();
    t.Expect(review_stack.GetHeight() < narrow_stack.GetHeight(),
             "Fit stack follows the active page height");
    t.Expect(review_workspace.top < narrow_workspace.top,
             "workspace moves upward for shorter active page");

    stack.SetActiveKey("home");
    root.SetRect(0, 0, wide, DPI(500));
    root.Layout();
    t.Expect(stack.GetRect().GetHeight() == wide_stack.GetHeight(),
             "widening restores original active-page height");
    t.Expect(workspace.GetRect().top == wide_workspace.top,
             "widening restores workspace position");

    Cout() << Format("UISTACK_MEASURE_SUMMARY checks=%d failed=%d\n", t.checks, t.fails);
    return t.fails == 0 ? 0 : 1;
}
