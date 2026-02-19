#include <Ui/Ui.h>

using namespace Upp;

class UiAllControlsDemoWindow : public TopWindow {
public:
    typedef UiAllControlsDemoWindow CLASSNAME;

    UiAllControlsDemoWindow()
    {
        Title("Ui All Controls Demo (Baseline)");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(900), DPI(520));

        Add(title_lbl);
        Add(line);
        Add(pass);
        Add(btn_primary);
        Add(btn_subtle);
        Add(check);
        Add(radio_a);
        Add(radio_b);
        Add(toggle);
        Add(int_edit);
        Add(float_edit);
        Add(mask);
        Add(slider_edit);
        Add(multi);
        Add(panel_probe);
        panel_probe.Add(panel_probe_label.SizePos());
        Add(slider);
        Add(scroll_h);

        title_lbl.SetText("Baseline Controls (no panel/card/layout wrappers)")
                 .SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        line.SetPlaceholder("UiLineEdit");
        pass.SetPlaceholder("UiPasswordEdit");
        pass.EnableVisibilityIcon(true);

        btn_primary.SetText("Primary").SetAccentStyle();
        btn_subtle.SetText("Subtle").SetSubtleStyle();

        check.SetText("UiCheckBox").SetChecked(true);
        radio_a.SetText("Radio A").SetGroup(1).SetChecked(true);
        radio_b.SetText("Radio B").SetGroup(1);
        toggle.SetText("UiToggle").SetOn(true);

        int_edit.MinMax(0, 5000).Step(1).SetValue(128);
        float_edit.MinMax(0.0, 1000.0).Precision(2).Step(0.25).SetValue(42.75);
        mask.SetMask("##/##/####").SetPlaceholder("UiMaskEdit");

        slider_edit.SetRange(0, 100).SetStep(0.5).SetValue(28.5).SetFieldAlign(UiAlign::RIGHT);

        multi.SetPlaceholder("UiMultiEdit");
        multi.SetText("Multiline baseline section\nwithout layout wrappers.");

        panel_probe.SetStyle(UiPanel::StyleDefault()).SetFaceColor(Blend(SColorFace(), SColorPaper(), 220));
        panel_probe_label.SetText("UiPanel probe (manual SetRect)")
                        .SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        slider.SetRange(0, 100).SetStep(1).SetValue(60).SetTicks(true, 11, 1);
        scroll_h.SetDirection(UiDirection::H).SetRange(0, 100, 25).SetPos(35);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        const int m = DPI(16);
        const int g = DPI(10);
        int y = m;

        title_lbl.SetRect(m, y, r.GetWidth() - m * 2, DPI(28));
        y += DPI(28) + g;

        int col_w = (r.GetWidth() - m * 2 - g) / 2;

        line.SetRect(m, y, col_w, DPI(36));
        pass.SetRect(m + col_w + g, y, col_w, DPI(36));
        y += DPI(36) + g;

        btn_primary.SetRect(m, y, DPI(170), DPI(36));
        btn_subtle.SetRect(m + DPI(182), y, DPI(170), DPI(36));
        y += DPI(36) + g;

        check.SetRect(m, y, DPI(190), DPI(34));
        radio_a.SetRect(m + DPI(198), y, DPI(150), DPI(34));
        radio_b.SetRect(m + DPI(356), y, DPI(150), DPI(34));
        toggle.SetRect(m + DPI(514), y, DPI(150), DPI(34));
        y += DPI(34) + g;

        int_edit.SetRect(m, y, col_w, DPI(36));
        float_edit.SetRect(m + col_w + g, y, col_w, DPI(36));
        y += DPI(36) + g;

        mask.SetRect(m, y, col_w, DPI(36));
        slider_edit.SetRect(m + col_w + g, y, col_w, DPI(42));
        y += DPI(42) + g;

        int half_w = (r.GetWidth() - m * 2 - g) / 2;
        multi.SetRect(m, y, half_w, DPI(110));
        panel_probe.SetRect(m + half_w + g, y, half_w, DPI(110));
        y += DPI(110) + g;

        slider.SetRect(m, y, r.GetWidth() - m * 2, DPI(46));
        y += DPI(46) + g;

        scroll_h.SetRect(m, y, r.GetWidth() - m * 2, DPI(18));
    }

private:
    UiLabel      title_lbl;
    UiLineEdit   line;
    UiPasswordEdit pass;
    UiButton     btn_primary;
    UiButton     btn_subtle;
    UiCheckBox   check;
    UiRadioButton radio_a;
    UiRadioButton radio_b;
    UiToggle     toggle;
    UiIntEdit    int_edit;
    UiFloatEdit  float_edit;
    UiMaskEdit   mask;
    UiSliderEdit slider_edit;
    UiMultiEdit  multi;
    UiPanel      panel_probe;
    UiLabel      panel_probe_label;
    UiSlider     slider;
    UiScrollBar  scroll_h { UiDirection::H };
};

GUI_APP_MAIN
{
    UiAllControlsDemoWindow().Run();
}
