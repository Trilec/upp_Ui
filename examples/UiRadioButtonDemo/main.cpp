#include <Ui/Ui.h>

using namespace Upp;

class UiRadioButtonDemoWindow : public TopWindow {
public:
    typedef UiRadioButtonDemoWindow CLASSNAME;

    UiRadioButtonDemoWindow()
    {
        Title("UiRadioButton Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(920), DPI(560));

        Add(info);
        Add(r1); Add(r2); Add(r3);
        Add(p1); Add(p2); Add(p3);
        Add(l1); Add(l2);
        Add(s1); Add(s2); Add(s3);

        info.SetText("Classic, pill, list, and square radios. Grouped exclusivity by SetGroup().")
            .SetAlign(UiAlign::LEFT, UiAlign::TOP)
            .SetInkColor(SColorDisabled());

        r1.SetText("Classic A").SetGroup(1).SetChecked(true);
        r2.SetText("Classic B").SetGroup(1);
        r3.SetText("Classic C").SetGroup(1);

        p1.SetText("Pill 1").SetStyle(UiRadioButton::StylePills()).SetGroup(2).SetChecked(true);
        p2.SetText("Pill 2").SetStyle(UiRadioButton::StylePills()).SetGroup(2);
        p3.SetText("Pill 3").SetStyle(UiRadioButton::StylePills()).SetGroup(2);
        p1.SetPadding(DPI(10), DPI(6), DPI(10), DPI(6));
        p2.SetPadding(DPI(10), DPI(6), DPI(10), DPI(6));
        p3.SetPadding(DPI(10), DPI(6), DPI(10), DPI(6));

        l1.SetText("List item option 1").SetStyle(UiRadioButton::StyleList()).SetGroup(3).SetChecked(true);
        l2.SetText("List item option 2").SetStyle(UiRadioButton::StyleList()).SetGroup(3);

        UiRadioButton::Style sq = UiRadioButton::StyleClassic();
        sq.indicator_metrics.radius = DPI(4);
        s1.SetText("Square A").SetStyle(sq).SetGroup(4).SetChecked(true);
        s2.SetText("Square B").SetStyle(sq).SetGroup(4);
        s3.SetText("Roundness 40%").SetStyle(sq).SetGroup(4).SetIndicatorRoundness(40);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(24);
        int y = m;
        info.SetRect(m, y, r.GetWidth() - 2 * m, DPI(42));
        y += DPI(54);

        r1.SetRect(m, y, DPI(260), DPI(34));
        r2.SetRect(m + DPI(280), y, DPI(260), DPI(34));
        r3.SetRect(m + DPI(560), y, DPI(260), DPI(34));
        y += DPI(50);

        p1.SetRect(m, y, DPI(200), DPI(36));
        p2.SetRect(m + DPI(220), y, DPI(200), DPI(36));
        p3.SetRect(m + DPI(440), y, DPI(200), DPI(36));
        y += DPI(56);

        l1.SetRect(m, y, r.GetWidth() - 2 * m, DPI(36));
        y += DPI(42);
        l2.SetRect(m, y, r.GetWidth() - 2 * m, DPI(36));

        y += DPI(54);
        s1.SetRect(m, y, DPI(220), DPI(34));
        s2.SetRect(m + DPI(240), y, DPI(220), DPI(34));
        s3.SetRect(m + DPI(480), y, DPI(260), DPI(34));
    }

private:
    UiLabel info;
    UiRadioButton r1, r2, r3;
    UiRadioButton p1, p2, p3;
    UiRadioButton l1, l2;
    UiRadioButton s1, s2, s3;
};

GUI_APP_MAIN
{
    UiRadioButtonDemoWindow().Run();
}
