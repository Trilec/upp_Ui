#include <Ui/Ui.h>

using namespace Upp;

static Image MakeSliceSkin(int w, int h, Color c0, Color c1)
{
    ImageBuffer ib(Size(w, h));
    RGBA* p = ib;
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            int t = (x * 255) / max(1, w - 1);
            RGBA px = Color((t * c1.GetR() + (255 - t) * c0.GetR()) / 255,
                            (t * c1.GetG() + (255 - t) * c0.GetG()) / 255,
                            (t * c1.GetB() + (255 - t) * c0.GetB()) / 255);
            if(x < 2 || y < 2 || x >= w - 2 || y >= h - 2)
                px = Color(40, 40, 40);
            px.a = 255;
            p[y * w + x] = px;
        }
    }
    return Image(ib);
}

class UiCheckBoxDemoWindow : public TopWindow {
public:
    typedef UiCheckBoxDemoWindow CLASSNAME;

    UiCheckBoxDemoWindow()
    {
        Title("UiCheckBox Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(960), DPI(560));

        Add(info);
        Add(c1); Add(c2); Add(c3); Add(c4); Add(c5); Add(c6);

        info.SetText("Cleaner defaults + style variants: classic, switch, chip, list-check, and 9-slice.")
            .SetAlign(UiAlign::LEFT, UiAlign::TOP)
            .SetInkColor(SColorDisabled());

        c1.SetText("Standard Active").SetStyle(UiCheckBox::StyleClassic()).SetChecked(true);

        c2.SetText("Standard Disabled").SetStyle(UiCheckBox::StyleClassic()).SetChecked(false).Disable();

        c3.SetText("Solid Active (Filled)").SetStyle(UiCheckBox::StyleChip()).SetChecked(true);

        c4.SetText("Solid Disabled (Filled)").SetStyle(UiCheckBox::StyleChip()).SetChecked(false).Disable();
        c4.SetPadding(DPI(10), DPI(6), DPI(10), DPI(6));

        c5.SetText("Switch style").SetStyle(UiCheckBox::StyleSwitch()).SetChecked(true);

        c6.SetText("9-slice chip").SetStyle(UiCheckBox::StyleChip()).SetChecked(true);
        Image skin = MakeSliceSkin(24, 24, Color(180, 220, 255), Color(120, 170, 230));
        c6.SetFill9Slice(skin, Rect(6, 6, 6, 6), true).SetInset(DPI(2)).SetPadding(DPI(10), DPI(6), DPI(10), DPI(6));
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(24);
        int y = m;
        info.SetRect(m, y, r.GetWidth() - 2 * m, DPI(42));
        y += DPI(56);

        UiCheckBox* arr[] = { &c1, &c2, &c3, &c4, &c5, &c6 };
        for(UiCheckBox* c : arr) {
            c->SetRect(m, y, r.GetWidth() - 2 * m, DPI(36));
            y += DPI(44);
        }
    }

private:
    UiLabel info;
    UiCheckBox c1, c2, c3, c4, c5, c6;
};

GUI_APP_MAIN
{
    UiCheckBoxDemoWindow().Run();
}
