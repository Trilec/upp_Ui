#include <Ui/Ui.h>

using namespace Upp;

class UiToggleDemoWindow : public TopWindow {
public:
    typedef UiToggleDemoWindow CLASSNAME;

    UiToggleDemoWindow()
    {
        Title("UiToggle Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(900), DPI(520));

        Add(info);
        Add(t1);
        Add(t2);
        Add(t3);
        Add(t4);

        info.SetText("UiToggle is a standalone boolean control with separate track and thumb styling.")
            .SetAlign(UiAlign::LEFT, UiAlign::TOP)
            .SetInkColor(SColorDisabled());

        t1.SetText("Wi-Fi");
        t1.SetOn(true);
        t2.SetText("Bluetooth");
        t2.SetOn(false);

        t3.SetText("Airplane mode");
        t3.SetOn(false);
        t3.SetPadding(DPI(8), DPI(4), DPI(8), DPI(4));

        t4.SetText("9-slice switch");
        t4.SetOn(true);
        ImageBuffer ib(Size(24, 24));
        RGBA* p = ib;
        for(int i = 0; i < ib.GetLength(); i++) {
            p[i] = Color(120, 170, 230);
            p[i].a = 255;
        }
        Image skin = Image(ib);
        t4.SetFill9Slice(skin, Rect(6, 6, 6, 6), true).SetInset(DPI(2));

        auto bind = [=](UiToggle& t) {
            t.WhenAction = [=] {
                info.SetText(Format("Wi-Fi=%d  Bluetooth=%d  Airplane=%d  9-slice=%d",
                                    t1.IsOn(), t2.IsOn(), t3.IsOn(), t4.IsOn()));
            };
        };
        bind(t1); bind(t2); bind(t3); bind(t4);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(24);
        int y = m;

        info.SetRect(m, y, r.GetWidth() - 2 * m, DPI(44));
        y += DPI(58);

        UiToggle* arr[] = { &t1, &t2, &t3, &t4 };
        for(UiToggle* t : arr) {
            t->SetRect(m, y, r.GetWidth() - 2 * m, DPI(36));
            y += DPI(46);
        }
    }

private:
    UiLabel info;
    UiToggle t1, t2, t3, t4;
};

GUI_APP_MAIN
{
    UiToggleDemoWindow().Run();
}

