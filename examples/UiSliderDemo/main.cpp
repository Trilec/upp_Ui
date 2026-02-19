#include <Ui/Ui.h>

using namespace Upp;

class UiSliderDemoWindow : public TopWindow {
public:
    typedef UiSliderDemoWindow CLASSNAME;

    UiSliderDemoWindow()
    {
        Title("UiSlider / UiSliderEdit Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(980), DPI(620));

        Add(h_slider);
        Add(v_slider);
        Add(edit_right);
        Add(edit_top);
        Add(info);

        h_slider.SetRange(0, 100).SetStep(1).SetValue(35).SetTicks(true, 11, 4);
        h_slider.SetTickSide(UiAlign::BOTTOM);

        v_slider.SetDirection(UiDirection::V).SetRange(-50, 50).SetStep(5).SetValue(10).SetTicks(true, 11, 1);
        v_slider.SetTickSide(UiAlign::RIGHT);

        edit_right.SetRange(0, 1).SetStep(0.01).SetValue(0.4).SetFieldAlign(UiAlign::RIGHT);
        edit_right.Field().Precision(3);
        edit_right.Slider().SetTicks(true, 6, 4);

        edit_top.SetRange(100, 1000).SetStep(25).SetValue(350).SetFieldAlign(UiAlign::TOP);
        edit_top.Field().Precision(0);
        edit_top.Slider().SetTicks(true, 10, 1);

        info.SetAlign(UiAlign::LEFT, UiAlign::TOP).SetInkColor(SColorDisabled());
        info.SetText("UiSlider: horizontal/vertical, ticks, keyboard + wheel.\n"
                     "UiSliderEdit: field placement via SetFieldAlign(LEFT/RIGHT/TOP/BOTTOM).");

        h_slider.WhenChanging = [=] {
            info.SetText(Format("H: %.2f   V: %.2f   R: %.3f   T: %.0f",
                               h_slider.GetValue(), v_slider.GetValue(),
                               edit_right.GetValue(), edit_top.GetValue()));
        };
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(24);

        h_slider.SetRect(m, m, r.GetWidth() - DPI(220), DPI(72));
        v_slider.SetRect(r.right - DPI(120), m, DPI(72), r.GetHeight() - DPI(200));

        edit_right.SetRect(m, DPI(130), r.GetWidth() - DPI(260), DPI(48));
        edit_top.SetRect(m, DPI(210), r.GetWidth() - DPI(260), DPI(120));

        info.SetRect(m, r.bottom - DPI(80), r.GetWidth() - 2 * m, DPI(56));
    }

private:
    UiSlider h_slider;
    UiSlider v_slider;
    UiSliderEdit edit_right;
    UiSliderEdit edit_top;
    UiLabel info;
};

GUI_APP_MAIN
{
    UiSliderDemoWindow().Run();
}
