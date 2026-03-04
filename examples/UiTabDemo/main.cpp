#include <Ui/Ui.h>

using namespace Upp;

class UiTabDemoWindow : public TopWindow {
public:
    typedef UiTabDemoWindow CLASSNAME;

    UiTabDemoWindow()
    {
        Title("UiTab Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1220), DPI(860));

        Add(t00); Add(t01); Add(t02);
        Add(t10); Add(t11); Add(t12);
        Add(t20); Add(t21); Add(t22);

        UiTab::Style classic_notched = UiTab::StyleClassic();
        classic_notched.body_gap = 0;

        UiTab::Style underline_full = UiTab::StyleUnderline();
        underline_full.indicator_span = LARGE;

        UiTab::Style underline_medium = UiTab::StyleUnderline();
        underline_medium.indicator_span = MEDIUM;

        UiTab::Style segmented_inset = UiTab::StyleSegmented();
        segmented_inset.strip_inset = Rect(DPI(5), DPI(5), DPI(5), DPI(5));
        segmented_inset.tab_padding = Rect(DPI(10), DPI(5), DPI(10), DPI(5));

        UiTab::Style segmented_square = segmented_inset;
        segmented_square.tab_metrics.radius = 0;
        segmented_square.metrics.radius = 0;

        UiTab::Style segmented_loose = UiTab::StyleSegmented();
        segmented_loose.strip_inset = Rect(DPI(7), DPI(7), DPI(7), DPI(7));
        segmented_loose.tab_padding = Rect(DPI(9), DPI(4), DPI(9), DPI(4));

        UiTab::Style doc_drag = UiTab::StyleDocument();
        doc_drag.body_gap = 2;

        SetupTab(t00, UiAlign::TOP, classic_notched, "Explorer / Connected Cap");
        SetupTab(t01, UiAlign::TOP, underline_full, "Modern / Underline Full");
        SetupTab(t02, UiAlign::TOP, doc_drag, "Document / Reorder");
        t02.SetTabText(0, "").SetTabText(1, "").SetTabText(2, "");

        SetupTab(t10, UiAlign::TOP, segmented_square, "Modern / Segmented Square");
        SetupTab(t11, UiAlign::TOP, segmented_loose, "Modern / Segmented Loose");
        SetupTab(t12, UiAlign::LEFT, UiTab::StyleRail(), "Vertical / Rail");

        SetupTab(t20, UiAlign::BOTTOM, classic_notched, "Explorer / Bottom Connected");
        SetupTab(t21, UiAlign::TOP, underline_medium, "Modern / Underline Medium");
        SetupTab(t22, UiAlign::TOP, UiTab::StyleSoft(), "Soft / Standardized");

        t02.EnableDragReorder(true)
           .EnableDragHandles(true)
           .EnableCloseButtons(true);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(16);
        int g = DPI(12);
        int cw = (r.GetWidth() - 2 * m - 2 * g) / 3;
        int ch = (r.GetHeight() - 2 * m - 2 * g) / 3;

        t00.SetRect(m + (cw + g) * 0, m + (ch + g) * 0, cw, ch);
        t01.SetRect(m + (cw + g) * 1, m + (ch + g) * 0, cw, ch);
        t02.SetRect(m + (cw + g) * 2, m + (ch + g) * 0, cw, ch);

        t10.SetRect(m + (cw + g) * 0, m + (ch + g) * 1, cw, ch);
        t11.SetRect(m + (cw + g) * 1, m + (ch + g) * 1, cw, ch);
        t12.SetRect(m + (cw + g) * 2, m + (ch + g) * 1, cw, ch);

        t20.SetRect(m + (cw + g) * 0, m + (ch + g) * 2, cw, ch);
        t21.SetRect(m + (cw + g) * 1, m + (ch + g) * 2, cw, ch);
        t22.SetRect(m + (cw + g) * 2, m + (ch + g) * 2, cw, ch);
    }

private:
    struct TabPages {
        UiLabel a;
        UiLabel b;
        UiLabel c;
    };

    void SetupTab(UiTab& t, UiAlign side, const UiTab::Style& st, const String& title)
    {
        TabPages& p = pages.Add();

        p.a.SetText(title + " / Overview").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        p.b.SetText(title + " / Settings").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        p.c.SetText(title + " / Logs").SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        t.SetStyle(st)
         .SetPlacement(side)
         .SetFillTabs(false);
        t.Add(p.a, "Overview", ICON_DESIGN_HOME_48());
        t.Add(p.b, "Settings", ICON_DESIGN_SETTINGS_48());
        t.Add(p.c, "Logs", ICON_DESIGN_MENU_48());
    }

private:
    UiTab t00;
    UiTab t01;
    UiTab t02;
    UiTab t10;
    UiTab t11;
    UiTab t12;
    UiTab t20;
    UiTab t21;
    UiTab t22;
    Array<TabPages> pages;
};

GUI_APP_MAIN
{
    UiTabDemoWindow().Run();
}
