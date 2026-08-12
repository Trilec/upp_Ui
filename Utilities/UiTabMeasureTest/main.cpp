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
    virtual Size GetMinSize() const override { return min_size; }

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

CONSOLE_APP_MAIN
{
    TestCtx t;

    UiBoxLayout page_tall;
    UiBoxLayout page_short;
    ConfigurePage(page_tall);
    ConfigurePage(page_short);

    FixedBlock a(DPI(170), DPI(44));
    FixedBlock b(DPI(170), DPI(44));
    FixedBlock c(DPI(170), DPI(44));
    FixedBlock d(DPI(170), DPI(44));
    FixedBlock e(DPI(170), DPI(44));
    FixedBlock f(DPI(170), DPI(44));

    page_tall.Add(a).Fit();
    page_tall.Add(b).Fit();
    page_tall.Add(c).Fit();
    page_tall.Add(d).Fit();

    page_short.Add(e).Fit();
    page_short.Add(f).Fit();

    UiTab tab;
    tab.SetPlacement(UiAlign::TOP)
       .SetVisual(UITAB_UNDERLINE)
       .SetExpandTabs(false);
    tab.Add(page_tall, "Tall");
    tab.Add(page_short, "Short");
    tab.SetActiveTab(0);

    const int wide = DPI(760);
    const int narrow = DPI(390);

    UiLayoutMeasureResult wide_measure = UiMeasureLayout(tab, { wide });
    UiLayoutMeasureResult narrow_measure = UiMeasureLayout(tab, { narrow });

    t.Expect(wide_measure.width_dependent,
             "UiTab reports width dependence from active wrapped page");
    t.Expect(narrow_measure.measured.cy > wide_measure.measured.cy,
             "narrow UiTab measurement grows when active page wraps");
    t.Expect(narrow_measure.measured.cx == narrow,
             "measured UiTab preserves supplied outer width");

    UiBoxLayout root;
    Ctrl workspace;
    root.SetDirection(UiDirection::V)
        .SetGap(0)
        .SetInset(0);
    root.Add(tab).Fit();
    root.Add(workspace).Expand(1);

    root.SetRect(0, 0, wide, DPI(500));
    root.Layout();
    Rect wide_tab = tab.GetRect();
    Rect wide_workspace = workspace.GetRect();

    root.SetRect(0, 0, narrow, DPI(500));
    root.Layout();
    Rect narrow_tab = tab.GetRect();
    Rect narrow_workspace = workspace.GetRect();

    t.Expect(narrow_tab.GetHeight() > wide_tab.GetHeight(),
             "Fit ribbon rect grows when parent width narrows");
    t.Expect(narrow_workspace.top > wide_workspace.top,
             "expanding workspace is pushed down by wrapped ribbon");
    t.Expect(narrow_tab.bottom <= narrow_workspace.top,
             "wrapped ribbon and workspace do not overlap");

    root.SetRect(0, 0, wide, DPI(500));
    root.Layout();
    Rect wide_again_tab = tab.GetRect();
    Rect wide_again_workspace = workspace.GetRect();
    t.Expect(wide_again_tab.GetHeight() == wide_tab.GetHeight(),
             "widening restores original ribbon height");
    t.Expect(wide_again_workspace.top == wide_workspace.top,
             "widening restores workspace position");

    tab.SetActiveTab(1);
    UiLayoutMeasureResult short_measure = UiMeasureLayout(tab, { narrow });
    t.Expect(short_measure.measured.cy < narrow_measure.measured.cy,
             "active shorter tab page reduces measured ribbon height");

    root.SetRect(0, 0, narrow, DPI(500));
    root.Layout();
    Rect short_tab = tab.GetRect();
    Rect short_workspace = workspace.GetRect();
    t.Expect(short_tab.GetHeight() < narrow_tab.GetHeight(),
             "Fit ribbon follows active tab page height");
    t.Expect(short_workspace.top < narrow_workspace.top,
             "workspace moves upward when active ribbon page is shorter");

    Cout() << Format("UITAB_MEASURE_SUMMARY checks=%d failed=%d\n", t.checks, t.fails);
    SetExitCode(t.fails == 0 ? 0 : 1);
}
