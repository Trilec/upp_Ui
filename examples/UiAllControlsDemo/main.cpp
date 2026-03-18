#include <Ui/Ui.h>

using namespace Upp;

class UiAllControlsDemoWindow : public TopWindow {
public:
    typedef UiAllControlsDemoWindow CLASSNAME;

    UiAllControlsDemoWindow()
    {
        Title("Ui All Controls Demo (Baseline)");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1100), DPI(1080));

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
        Add(style_title);
        Add(h_standard);
        Add(h_minimal);
        Add(h_soft);
        Add(h_strong);
        Add(r_button);
        Add(r_edit);
        Add(r_check);
        Add(r_radio);
        Add(r_slider);
        Add(r_panel);

        Add(b_std);
        Add(b_min);
        Add(b_soft);
        Add(b_strong);
        Add(e_std);
        Add(e_min);
        Add(e_soft);
        Add(e_strong);
        Add(c_std);
        Add(c_min);
        Add(c_soft);
        Add(c_strong);
        Add(rb_std);
        Add(rb_min);
        Add(rb_soft);
        Add(rb_strong);
        Add(s_std);
        Add(s_min);
        Add(s_soft);
        Add(s_strong);
        Add(p_std);
        Add(p_min);
        Add(p_soft);
        Add(p_strong);

        p_std.Add(pl_std.SizePos());
        p_min.Add(pl_min.SizePos());
        p_soft.Add(pl_soft.SizePos());
        p_strong.Add(pl_strong.SizePos());

        scroll_probe.Content().Add(sp_a);
        scroll_probe.Content().Add(sp_b);
        scroll_probe.Content().Add(sp_c);

        title_lbl.SetText("Baseline Controls (no panel/card/layout wrappers)")
                 .SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        line.SetPlaceholder("UiLineEdit");
        pass.SetPlaceholder("UiPasswordEdit");
        pass.EnableVisibilityIcon(true);

        btn_primary.SetText("Primary").SetStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
        btn_subtle.SetText("Subtle").SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));

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
        grid_a.SetText("Grid A").SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        grid_b.SetText("Grid B").SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        grid_c.SetText("Grid C").SetStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
        grid_probe.Add(grid_a, -1, true);
        grid_probe.Add(grid_b, -1, true);
        grid_probe.Add(grid_c, -1, true);

        scroll_probe.SetScrollMode(UIPANELSCROLL_AUTO).SetStyle(UiScrollPanel::StyleDefault());
        sp_a.SetText("UiScrollPanel row 01").SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        sp_b.SetText("UiScrollPanel row 02").SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        sp_c.SetText("UiScrollPanel row 03").SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));

        box_probe.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(DPI(8));
        box_a.SetText("UiBoxLayout A").SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        box_b.SetText("UiBoxLayout B").SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        box_c.SetText("UiBoxLayout C").SetStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
        box_probe.Add(box_a).Expand(1).MinHeight(DPI(30));
        box_probe.Add(box_b).Expand(1).MinHeight(DPI(30));
        box_probe.Add(box_c).Expand(1).MinHeight(DPI(30));

        style_title.SetText("Unified Style Set: Standard / Subtle / Accent / Strong")
                   .SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        h_standard.SetText("Standard").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        h_minimal.SetText("Subtle").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        h_soft.SetText("Accent").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        h_strong.SetText("Strong").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        r_button.SetText("UiButton").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        r_edit.SetText("UiLineEdit").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        r_check.SetText("UiCheckBox").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        r_radio.SetText("UiRadioButton").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        r_slider.SetText("UiSlider").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        r_panel.SetText("UiPanel").SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        b_std.SetText("Button").SetStyle(UiTheme::ResolveButton(UiButtonRole::Standard));
        b_min.SetText("Button").SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        b_soft.SetText("Button").SetStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
        b_strong.SetText("Button").SetStyle(UiTheme::ResolveButton(UiButtonRole::Danger));

        e_std.SetPlaceholder("Field").SetStyle(UiTheme::ResolveEdit(UiEditRole::Field));
        e_min.SetPlaceholder("Subtle").SetStyle(UiTheme::ResolveEdit(UiEditRole::Subtle));
        e_soft.SetPlaceholder("Default").SetStyle(UiBaseEdit::StyleDefault());
        e_strong.SetPlaceholder("Strong").SetStyle(UiTheme::ResolveEdit(UiEditRole::Strong));

        c_std.SetText("Checked").SetChecked(true);
        c_min.SetText("Switch").SetVisual(UICHECKVIS_SWITCH).SetChecked(true);
        c_soft.SetText("Chip").SetVisual(UICHECKVIS_CHIP).SetChecked(true);
        c_strong.SetText("List").SetVisual(UICHECKVIS_LIST).SetChecked(true);

        rb_std.SetText("Selected").SetChecked(true);
        rb_min.SetText("Pill").SetVisual(UIRADIOVIS_PILLS).SetChecked(true);
        rb_soft.SetText("List").SetVisual(UIRADIOVIS_LIST).SetChecked(true);
        rb_strong.SetText("Rounded").SetChecked(true).SetIndicatorRoundness(40);

        s_std.SetRange(0, 100).SetValue(64);
        s_min.SetRange(0, 100).SetValue(64).SetTicks(true, 6, 2);
        s_soft.SetRange(0, 100).SetValue(64).SetTicks(true, 11, 4).SetTickSide(UiAlign::TOP);
        s_strong.SetDirection(UiDirection::V).SetRange(0, 100).SetValue(64);

        p_std.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        p_min.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
        p_soft.SetStyle(UiTheme::ResolvePanel(UiThemePreset::Layered, UiThemeMode::Light, UiPanelRole::Surface));
        p_strong.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Strong));
        pl_std.SetText("Panel").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        pl_min.SetText("Panel").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        pl_soft.SetText("Panel").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        pl_strong.SetText("Panel").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
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

        int y3 = y2 + DPI(150) + DPI(18);
        int row_lbl_w = DPI(110);
        int col_gap = g;
        int cell_w = (r.GetWidth() - m * 2 - row_lbl_w - 4 * col_gap) / 4;
        int x0 = m + row_lbl_w + col_gap;

        style_title.SetRect(m, y3, r.GetWidth() - m * 2, DPI(26));
        y3 += DPI(30);

        h_standard.SetRect(x0 + (cell_w + col_gap) * 0, y3, cell_w, DPI(24));
        h_minimal.SetRect(x0 + (cell_w + col_gap) * 1, y3, cell_w, DPI(24));
        h_soft.SetRect(x0 + (cell_w + col_gap) * 2, y3, cell_w, DPI(24));
        h_strong.SetRect(x0 + (cell_w + col_gap) * 3, y3, cell_w, DPI(24));
        y3 += DPI(30);

        auto Row = [&](UiLabel& lbl, Ctrl& a, Ctrl& b, Ctrl& c, Ctrl& d, int h) {
            lbl.SetRect(m, y3, row_lbl_w, h);
            a.SetRect(x0 + (cell_w + col_gap) * 0, y3, cell_w, h);
            b.SetRect(x0 + (cell_w + col_gap) * 1, y3, cell_w, h);
            c.SetRect(x0 + (cell_w + col_gap) * 2, y3, cell_w, h);
            d.SetRect(x0 + (cell_w + col_gap) * 3, y3, cell_w, h);
            y3 += h + DPI(8);
        };

        Row(r_button, b_std, b_min, b_soft, b_strong, DPI(34));
        Row(r_edit, e_std, e_min, e_soft, e_strong, DPI(34));
        Row(r_check, c_std, c_min, c_soft, c_strong, DPI(34));
        Row(r_radio, rb_std, rb_min, rb_soft, rb_strong, DPI(34));
        Row(r_slider, s_std, s_min, s_soft, s_strong, DPI(36));
        Row(r_panel, p_std, p_min, p_soft, p_strong, DPI(58));
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

    UiLabel style_title;
    UiLabel h_standard;
    UiLabel h_minimal;
    UiLabel h_soft;
    UiLabel h_strong;
    UiLabel r_button;
    UiLabel r_edit;
    UiLabel r_check;
    UiLabel r_radio;
    UiLabel r_slider;
    UiLabel r_panel;

    UiButton b_std;
    UiButton b_min;
    UiButton b_soft;
    UiButton b_strong;

    UiLineEdit e_std;
    UiLineEdit e_min;
    UiLineEdit e_soft;
    UiLineEdit e_strong;

    UiCheckBox c_std;
    UiCheckBox c_min;
    UiCheckBox c_soft;
    UiCheckBox c_strong;

    UiRadioButton rb_std;
    UiRadioButton rb_min;
    UiRadioButton rb_soft;
    UiRadioButton rb_strong;

    UiSlider s_std;
    UiSlider s_min;
    UiSlider s_soft;
    UiSlider s_strong;

    UiPanel p_std;
    UiPanel p_min;
    UiPanel p_soft;
    UiPanel p_strong;
    UiLabel pl_std;
    UiLabel pl_min;
    UiLabel pl_soft;
    UiLabel pl_strong;
};

GUI_APP_MAIN
{
    UiAllControlsDemoWindow().Run();
}






