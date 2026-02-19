#include <Ui/Ui.h>

using namespace Upp;

class UiAllControlsDemoWindow : public TopWindow {
public:
    typedef UiAllControlsDemoWindow CLASSNAME;

    UiAllControlsDemoWindow()
    {
        Title("Ui All Controls Demo (Baseline)");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(980), DPI(780));

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
        Add(base_edit);
        Add(card_probe);
        Add(accordion_probe);
        Add(grid_probe);
        Add(scroll_probe);
        Add(box_probe);

        scroll_probe.Content().Add(sp_a);
        scroll_probe.Content().Add(sp_b);
        scroll_probe.Content().Add(sp_c);

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

        base_edit.SetPlaceholder("UiBaseEdit foundation control");

        card_probe.SetTitle("UiTitleCard")
                 .SetSubTitle("header + copy + rule")
                 .SetCopyText("Smoke sample aligned with current style contract.");

        UiAccordion::Style accs = UiAccordion::StyleDefault();
        accs.header_height = DPI(42);
        accs.section_gap = DPI(10);
        accs.header_body_gap = DPI(6);
        accs.body_min_height = DPI(44);
        accs.metrics.content_padding = Rect(DPI(8), DPI(8), DPI(8), DPI(10));
        accs.header_style.title_font = SansSerifZ(13).Bold();
        accs.header_style.subtitle_font = SansSerifZ(11);
        accs.header_style.copy_font = SansSerifZ(1);
        accordion_probe.SetStyle(accs);

        int sec_a = accordion_probe.AddSection("UiAccordion / A", "open by default", String(), true);
        int sec_b = accordion_probe.AddSection("UiAccordion / B", "collapsed", String(), false);
        accordion_probe.SetSectionBodyHeight(sec_a, DPI(58));
        accordion_probe.SetSectionBodyHeight(sec_b, DPI(58));
        acc_body_a.SetText("Body A content").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        acc_body_b.SetText("Body B content").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        accordion_probe.Body(sec_a).Add(acc_body_a.SizePos());
        accordion_probe.Body(sec_b).Add(acc_body_b.SizePos());
        accordion_probe.SetSingleOpen(true);

        grid_probe.SetMode(UiGridLayout::Grid)
                 .SetGap(DPI(6))
                 .SetInset(DPI(6))
                 .SetScrollMode(UiGridLayout::None);
        grid_a.SetText("Grid A").SetSubtleStyle();
        grid_b.SetText("Grid B").SetSubtleStyle();
        grid_c.SetText("Grid C").SetAccentStyle();
        grid_probe.Add(grid_a, -1, true);
        grid_probe.Add(grid_b, -1, true);
        grid_probe.Add(grid_c, -1, true);

        scroll_probe.SetScrollMode(UIPANELSCROLL_AUTO).SetStyle(UiScrollPanel::StyleDefault());
        sp_a.SetText("UiScrollPanel row 01").SetSubtleStyle();
        sp_b.SetText("UiScrollPanel row 02").SetSubtleStyle();
        sp_c.SetText("UiScrollPanel row 03").SetSubtleStyle();

        box_probe.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(DPI(8));
        box_a.SetText("UiBoxLayout A").SetSubtleStyle();
        box_b.SetText("UiBoxLayout B").SetSubtleStyle();
        box_c.SetText("UiBoxLayout C").SetAccentStyle();
        box_probe.Add(box_a).Expand(1).MinHeight(DPI(30));
        box_probe.Add(box_b).Expand(1).MinHeight(DPI(30));
        box_probe.Add(box_c).Expand(1).MinHeight(DPI(30));
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

        y += DPI(18) + g;

        int trip_w = (r.GetWidth() - m * 2 - 2 * g) / 3;
        base_edit.SetRect(m, y, trip_w, DPI(36));
        card_probe.SetRect(m + trip_w + g, y, trip_w, DPI(92));
        accordion_probe.SetRect(m + 2 * (trip_w + g), y, trip_w, DPI(198));

        int y2 = y + DPI(198) + g;
        grid_probe.SetRect(m, y2, trip_w, DPI(150));
        scroll_probe.SetRect(m + trip_w + g, y2, trip_w, DPI(150));
        box_probe.SetRect(m + 2 * (trip_w + g), y2, trip_w, DPI(150));

        int sx = DPI(8);
        int sy = DPI(8);
        int sw = max(DPI(160), scroll_probe.GetSize().cx - DPI(24));
        sp_a.SetRect(sx, sy, sw, DPI(30));
        sp_b.SetRect(sx, sy + DPI(36), sw, DPI(30));
        sp_c.SetRect(sx, sy + DPI(72), sw, DPI(30));
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

    UiBaseEdit   base_edit;
    UiTitleCard  card_probe;

    UiAccordion  accordion_probe;
    UiLabel      acc_body_a;
    UiLabel      acc_body_b;

    UiGridLayout grid_probe;
    UiButton     grid_a;
    UiButton     grid_b;
    UiButton     grid_c;

    UiScrollPanel scroll_probe;
    UiButton      sp_a;
    UiButton      sp_b;
    UiButton      sp_c;

    UiBoxLayout box_probe { UiDirection::H };
    UiButton    box_a;
    UiButton    box_b;
    UiButton    box_c;
};

GUI_APP_MAIN
{
    UiAllControlsDemoWindow().Run();
}
