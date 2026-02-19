#include <Ui/Ui.h>
#include <Painter/Painter.h>

using namespace Upp;

static void BuildUiIconsGallery(Vector<Image>& images, Vector<String>& names)
{
    images.Clear();
    names.Clear();

    images
        << ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48()
        << ICON_NAVIGATION_OUTLINED_MENU_48()
        << ICON_NAVIGATION_OUTLINED_MORE_VERT_48()
        << ICON_NAVIGATION_OUTLINED_ARROW_RIGHT_48()
        << ICON_NAVIGATION_OUTLINED_ARROW_LEFT_48()
        << ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48()
        << ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48()
        << ICON_NAVIGATION_OUTLINED_APPS_48()
        << ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_RIGHT_48()
        << ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_LEFT_48()
        << ICON_CONTENT_OUTLINED_ADD_CIRCLE_OUTLINE_48()
        << ICON_CONTENT_OUTLINED_REMOVE_CIRCLE_OUTLINE_48()
        << ICON_CONTENT_OUTLINED_ADD_48()
        << ICON_CONTENT_OUTLINED_REMOVE_48()
        << ICON_ACTION_OUTLINED_VISIBILITY_48()
        << ICON_ACTION_OUTLINED_VISIBILITY_OFF_48()
        << ICON_DESIGN_DRAG_INDICATOR_48();

    names
        << "more_horiz"
        << "menu"
        << "more_vert"
        << "arrow_right"
        << "arrow_left"
        << "arrow_drop_up"
        << "arrow_drop_down"
        << "apps"
        << "kbd_arrow_right"
        << "kbd_arrow_left"
        << "add_circle_outline"
        << "remove_circle_outline"
        << "add"
        << "remove"
        << "visibility"
        << "visibility_off"
        << "drag_indicator";
}

static Image MakeNeoSkin30()
{
    const int sz = DPI(36);
    ImageBuffer ib(sz, sz);
    Fill(~ib, RGBAZero(), ib.GetLength());

    const double m = DPI(3);
    const double w = sz - DPI(6);
    const double h = sz - DPI(6);
    const double r = DPI(9);

    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(m + 2, m + 2, w, h, r);
        p.Fill(Color(160, 171, 189));
        p.End();
    }
    FastBlur(ib, 3);

    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(m - 1, m - 1, w, h, r);
        p.Fill(Color(255, 255, 255));
        p.End();
    }
    FastBlur(ib, 2);

    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(m, m, w, h, r);
        p.Fill(Color(242, 246, 252));
        p.RoundedRectangle(m, m, w, h, r);
        p.Stroke(1.1, Color(186, 198, 215));
        p.End();
    }

    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(m + 2, m + 2, w - 4, h - 4, max(1.0, r - 2));
        p.Stroke(0.8, Color(255, 255, 255));
        p.End();
    }
    return ib;
}

static Image MakeBrutalistPanelSkin()
{
    const int sz = DPI(40);
    ImageBuffer ib(sz, sz);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    const double m = DPI(2);
    const double w = sz - DPI(8);
    const double h = sz - DPI(8);
    const double r = DPI(10);
    const double sx = DPI(4);
    const double sy = DPI(4);

    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(m + sx, m + sy, w, h, r);
        p.Fill(Black());
        p.End();
    }

    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(m, m, w, h, r);
        p.Fill(Color(255, 228, 0));
        p.RoundedRectangle(m, m, w, h, r);
        p.Stroke(2.0, Black());
        p.End();
    }

    return ib;
}

class UiAccordionDemoWindow : public TopWindow {
public:
    typedef UiAccordionDemoWindow CLASSNAME;

    UiAccordionDemoWindow()
    {
        Title("UiAccordion Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1220), DPI(860));

        Add(title_lbl);
        Add(policy_acc);
        Add(compact_acc);
        Add(gradient_acc);
        Add(neo_acc);

        title_lbl.SetText("UiAccordion showcase (2x2): policy, compact, gradient, neumorphic")
                 .SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        SetupPolicyAccordion();
        SetupCompactAccordion();
        SetupGradientAccordion();
        SetupNeoAccordion();

    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(12);
        int g = DPI(10);

        title_lbl.SetRect(m, m, r.GetWidth() - m * 2, DPI(30));

        int top = m + DPI(30) + g;
        int avail_h = r.GetHeight() - top - m;
        int row_h = (avail_h - g) / 2;
        int col_w = (r.GetWidth() - m * 2 - g) / 2;

        policy_acc.SetRect(m, top, col_w, row_h);
        gradient_acc.SetRect(m + col_w + g, top, col_w, row_h);
        compact_acc.SetRect(m, top + row_h + g, col_w, row_h);
        neo_acc.SetRect(m + col_w + g, top + row_h + g, col_w, row_h);

        if(compact_acc.GetCount() >= 3) {
            Rect h0 = compact_acc.Header(0).GetSize();
            Rect h1 = compact_acc.Header(1).GetSize();
            Rect h2 = compact_acc.Header(2).GetSize();
            int hw = DPI(24), hh = DPI(18);
            compact_drag_a.SetRect(h0.right - hw - DPI(8), (h0.GetHeight() - hh) / 2, hw, hh);
            compact_drag_b.SetRect(h1.right - hw - DPI(8), (h1.GetHeight() - hh) / 2, hw, hh);
            compact_drag_c.SetRect(h2.right - hw - DPI(8), (h2.GetHeight() - hh) / 2, hw, hh);
        }

        if(!grad_icons_panel.GetSize().IsEmpty()) {
            int panel_w = grad_icons_panel.GetSize().cx;
            int inset = DPI(2);
            int cell = DPI(42);
            int gap = DPI(6);
            int inner_w = max(1, panel_w - inset * 2);
            int cols = max(1, (inner_w + gap) / max(1, cell + gap));
            int rows = max(1, (icon_cells.GetCount() + cols - 1) / cols);
            int h = inset * 2 + rows * cell + max(0, rows - 1) * gap;
            icon_grid.SetRect(0, 0, panel_w, h);
        }
    }

private:
    static void FillGradientRect(Draw& w, const Rect& r, Color top, Color bottom)
    {
        if(r.IsEmpty())
            return;
        int h = max(1, r.GetHeight());
        for(int i = 0; i < h; i++) {
            int t = (255 * i) / max(1, h - 1);
            w.DrawRect(r.left, r.top + i, r.GetWidth(), 1, Blend(bottom, top, t));
        }
    }

    void SetupPolicyAccordion()
    {
        UiAccordion::Style st = UiAccordion::StyleDefault();
        st.single_open = true;
        st.enforce_one = false;
        st.header_height = DPI(72);
        st.section_gap = DPI(8);
        st.header_style.hover_enabled = true;
        st.header_style.show_focus = false;
        st.header_style.show_rule = true;
        st.header_style.title_font = SansSerifZ(13).Bold();
        st.header_style.subtitle_font = SansSerifZ(10);
        st.header_style.copy_font = SansSerifZ(9);
        st.header_style.title_subtitle_gap = DPI(1);
        st.header_style.subtitle_copy_gap = DPI(1);
        st.header_style.metrics.content_padding = Rect(DPI(10), DPI(7), DPI(10), DPI(7));
        for(int i = 0; i < 4; i++)
            st.header_style.palette.ink[i] = Color(24, 44, 72);
        policy_acc.SetStyle(st);

        int a = policy_acc.AddSection("Policy / General", "single-open + enforce-one", "lock modes below", true);
        int b = policy_acc.AddSection("Appearance", "compact section", "style toggles", false);
        int c = policy_acc.AddSection("Integrations", "service links", "mixed controls", false);

        policy_acc.Content(a).Add(policy_controls.SizePos());
        policy_controls.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));
        policy_controls.Add(row_policy_a).Fixed(DPI(30));
        policy_controls.Add(row_policy_b).Fixed(DPI(34));
        policy_controls.Add(row_policy_c).Fixed(DPI(34));
        policy_controls.Add(row_policy_d).Fixed(DPI(34));
        policy_controls.Add(policy_status).Fixed(DPI(24));

        row_policy_a.SetDirection(UiDirection::H).SetGap(DPI(8));
        row_policy_a.Add(policy_single).Fixed(DPI(130));
        row_policy_a.Add(policy_enforce).Fixed(DPI(130));
        row_policy_a.Add(policy_open_all).Fixed(DPI(90));
        row_policy_a.Add(policy_close_all).Fixed(DPI(90));

        policy_single.SetText("Single open").SetChecked(true);
        policy_enforce.SetText("Enforce one").SetChecked(false);
        policy_open_all.SetText("Open all").SetSubtleStyle();
        policy_close_all.SetText("Close all").SetSubtleStyle();
        policy_single.SetStyle(UiCheckBox::StyleClassic());
        policy_enforce.SetStyle(UiCheckBox::StyleListCheck());

        row_policy_b.SetDirection(UiDirection::H).SetGap(DPI(8));
        row_policy_b.Add(lock_open_b).Fixed(DPI(120));
        row_policy_b.Add(lock_closed_c).Fixed(DPI(120));
        row_policy_b.Add(lock_none_all).Fixed(DPI(120));
        lock_open_b.SetText("Force B open");
        lock_closed_c.SetText("Force C closed");
        lock_none_all.SetText("Unlock all").SetSubtleStyle();
        lock_open_b.SetStyle(UiCheckBox::StyleSwitch());
        lock_closed_c.SetStyle(UiCheckBox::StyleClassic());

        row_policy_c.SetDirection(UiDirection::H).SetGap(DPI(8));
        row_policy_c.Add(anim_enabled).Fixed(DPI(120));
        row_policy_c.Add(anim_open).Expand(1);
        row_policy_c.Add(anim_close).Expand(1);
        row_policy_c.Add(anim_value).Fixed(DPI(120));
        anim_enabled.SetText("Anim").SetChecked(true);
        anim_enabled.SetStyle(UiCheckBox::StyleChip());
        anim_open.SetRange(0, 250).SetStep(10).SetValue(120).SetTicks(false);
        anim_close.SetRange(0, 250).SetStep(10).SetValue(0).SetTicks(false);
        anim_value.SetText("open 120 / close 0").SetStyle(UiLabel::StyleCaption());

        row_policy_d.SetDirection(UiDirection::H).SetGap(DPI(8));
        row_policy_d.Add(rule_mode_lbl).Fixed(DPI(65));
        row_policy_d.Add(rule_mode).Fixed(DPI(120));
        row_policy_d.Add(rule_thickness).Expand(1);
        row_policy_d.Add(rule_value).Fixed(DPI(120));
        rule_mode_lbl.SetText("Rule line").SetStyle(UiLabel::StyleCaption());
        rule_mode.Add((int)NONE, "None");
        rule_mode.Add((int)SMALL, "Small");
        rule_mode.Add((int)MEDIUM, "Medium");
        rule_mode.Add((int)LARGE, "Large");
        rule_mode.SetData((int)LARGE);
        rule_thickness.SetRange(1, 5).SetStep(1).SetValue(1).SetTicks(true, 4, 1);
        rule_value.SetText("Large / 1px").SetStyle(UiLabel::StyleCaption());

        policy_status.SetText("Open=1 | force-open=0 | force-closed=0").SetStyle(UiLabel::StyleCaption());

        policy_acc.Content(b).Add(policy_appearance.SizePos());
        policy_appearance.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));
        policy_appearance.Add(appearance_dark).Fixed(DPI(30));
        policy_appearance.Add(row_appearance_gradient).Fixed(DPI(30));
        policy_appearance.Add(row_appearance_gap).Fixed(DPI(30));
        policy_appearance.Add(appearance_unified).Fixed(DPI(30));
        appearance_dark.SetText("Enable dark accents").SetChecked(true);
        appearance_unified.SetText("Unified header/body frame");
        appearance_dark.SetStyle(UiCheckBox::StyleSwitch());
        appearance_unified.SetStyle(UiCheckBox::StyleListCheck());

        row_appearance_gradient.SetDirection(UiDirection::H).SetGap(DPI(8));
        row_appearance_gradient.Add(appearance_grad_lbl).Fixed(DPI(82));
        row_appearance_gradient.Add(appearance_grad_from).Fixed(DPI(74));
        row_appearance_gradient.Add(appearance_grad_to).Fixed(DPI(74));
        row_appearance_gradient.Add(appearance_grad_value).Expand(1);
        appearance_grad_lbl.SetText("Header grad").SetStyle(UiLabel::StyleCaption());
        appearance_grad_from.SetData(Color(66, 108, 176));
        appearance_grad_to.SetData(Color(35, 60, 102));
        appearance_grad_value.SetText("from -> to").SetStyle(UiLabel::StyleCaption());

        row_appearance_gap.SetDirection(UiDirection::H).SetGap(DPI(8));
        row_appearance_gap.Add(appearance_gap_lbl).Fixed(DPI(82));
        row_appearance_gap.Add(appearance_gap_slider).Expand(1);
        row_appearance_gap.Add(appearance_gap_value).Fixed(DPI(58));
        appearance_gap_lbl.SetText("Section gap").SetStyle(UiLabel::StyleCaption());
        appearance_gap_slider.SetRange(0, 20).SetStep(1).SetValue(8).SetTicks(true, 5, 1);
        appearance_gap_value.SetText("8px").SetStyle(UiLabel::StyleCaption());

        policy_acc.Content(c).Add(policy_notes.SizePos());
        policy_notes.SetText("Policy accordion demo:\n- lock open/closed\n- single-open\n- enforce-one");

        policy_acc.SetSectionBodyHeight(a, DPI(204)).SetSectionBodyHeight(b, DPI(152)).SetSectionBodyHeight(c, DPI(92));

        policy_single.WhenAction = [=] { policy_acc.SetSingleOpen(policy_single.IsChecked()); SyncPolicyStatus(); };
        policy_enforce.WhenAction = [=] { policy_acc.SetEnforceOne(policy_enforce.IsChecked()); SyncPolicyStatus(); };
        policy_open_all.WhenAction = [=] { policy_acc.OpenAll(true); SyncPolicyStatus(); };
        policy_close_all.WhenAction = [=] { policy_acc.OpenAll(false); SyncPolicyStatus(); };

        auto ApplyAnim = [=] {
            int om = (int)anim_open.GetData();
            int cm = (int)anim_close.GetData();
            bool en = anim_enabled.IsChecked();
            policy_acc.SetAnimation(en, om, cm);
            compact_acc.SetAnimation(en, om, cm);
            gradient_acc.SetAnimation(en, om, cm);
            neo_acc.SetAnimation(en, om, cm);
            anim_value.SetText(Format("open %d / close %d", om, cm));
        };
        anim_enabled.WhenAction = ApplyAnim;
        anim_open.WhenAction = ApplyAnim;
        anim_close.WhenAction = ApplyAnim;

        auto ApplyRulePolicy = [=] {
            UiSpan ex = (UiSpan)(int)rule_mode.GetData();
            int th = (int)rule_thickness.GetData();

            policy_acc.SetHeaderRuleExtent(ex);
            compact_acc.SetHeaderRuleExtent(ex);
            gradient_acc.SetHeaderRuleExtent(ex);
            neo_acc.SetHeaderRuleExtent(ex);

            auto PatchRule = [=](UiAccordion& acc) {
                UiAccordion::Style s = acc.GetStyle();
                s.header_style.rule_thickness = max(1, th);
                acc.SetStyle(s);
            };
            PatchRule(policy_acc);
            PatchRule(compact_acc);
            PatchRule(gradient_acc);
            PatchRule(neo_acc);

            String mode = "Large";
            if(ex == NONE) mode = "None";
            else if(ex == SMALL) mode = "Small";
            else if(ex == MEDIUM) mode = "Medium";
            rule_value.SetText(Format("%s / %dpx", mode, th));
        };
        rule_mode.WhenAction = ApplyRulePolicy;
        rule_thickness.WhenAction = ApplyRulePolicy;

        lock_open_b.WhenAction = [=] {
            policy_acc.SetLockMode(b, lock_open_b.IsChecked() ? UiAccordion::Lock::Open : UiAccordion::Lock::None);
            SyncPolicyStatus();
        };
        lock_closed_c.WhenAction = [=] {
            policy_acc.SetLockMode(c, lock_closed_c.IsChecked() ? UiAccordion::Lock::Closed : UiAccordion::Lock::None);
            SyncPolicyStatus();
        };
        lock_none_all.WhenAction = [=] {
            policy_acc.SetLockMode(a, UiAccordion::Lock::None);
            policy_acc.SetLockMode(b, UiAccordion::Lock::None);
            policy_acc.SetLockMode(c, UiAccordion::Lock::None);
            lock_open_b.SetChecked(false);
            lock_closed_c.SetChecked(false);
            SyncPolicyStatus();
        };

        auto ApplyAppearance = [=] {
            UiAccordion::Style s = policy_acc.GetStyle();
            UiAccordion::Style d = UiAccordion::StyleDefault();

            if(appearance_dark.IsChecked()) {
                for(int i = 0; i < 4; i++) {
                    s.header_style.palette.face[i] = UiFill::Solid(Color(42, 64, 98));
                    s.header_style.palette.frame[i] = Color(25, 43, 72);
                    s.header_style.palette.ink[i] = White();

                    s.body_style.palette.face[i] = UiFill::Solid(Color(238, 242, 248));
                    s.body_style.palette.frame[i] = Color(168, 181, 202);
                    s.body_style.palette.ink[i] = Color(22, 35, 57);
                }
                s.header_style.palette.face[ST_HOT] = UiFill::Solid(Color(53, 78, 118));
                s.header_style.palette.face[ST_PRESSED] = UiFill::Solid(Color(34, 55, 86));
            }
            else {
                s.header_style.palette = d.header_style.palette;
                for(int i = 0; i < 4; i++)
                    s.header_style.palette.ink[i] = Color(24, 44, 72);
                s.body_style.palette = d.body_style.palette;
            }

            Color c0 = appearance_grad_from.GetData();
            Color c1 = appearance_grad_to.GetData();
            Image g0 = MakeQuadGradientTile(48, LtColor(c0, 10), c0, DkColor(c1, 6), c1, 0);
            Image g1 = MakeQuadGradientTile(48, LtColor(c0, 16), LtColor(c0, 6), c1, DkColor(c1, 3), 0);
            Image g2 = MakeQuadGradientTile(48, c0, DkColor(c0, 8), DkColor(c1, 12), DkColor(c1, 16), 0);
            s.header_style.palette.face[ST_NORMAL] = UiFill::ImageFill(g0);
            s.header_style.palette.face[ST_HOT] = UiFill::ImageFill(g1);
            s.header_style.palette.face[ST_PRESSED] = UiFill::ImageFill(g2);
            s.header_style.palette.face[ST_DISABLED] = UiFill::Solid(Blend(c0, SColorDisabled(), 62));
            appearance_grad_value.SetText(Format("#%02X%02X%02X -> #%02X%02X%02X",
                                                 c0.GetR(), c0.GetG(), c0.GetB(),
                                                 c1.GetR(), c1.GetG(), c1.GetB()));

            s.header_height = DPI(72);
            s.section_gap = max(0, (int)appearance_gap_slider.GetData());
            s.header_body_gap = DPI(4);
            s.header_style.metrics.content_padding = Rect(DPI(10), DPI(7), DPI(10), DPI(7));
            appearance_gap_value.SetText(Format("%dpx", s.section_gap));

            s.unified_section_frame = appearance_unified.IsChecked();
            s.unified_section_radius = DPI(7);
            s.unified_section_frame_width = 1;
            if(s.unified_section_frame) {
                s.header_style.metrics.frame_enabled = false;
                s.body_style.metrics.frame_enabled = false;
            }
            else {
                s.header_style.metrics.frame_enabled = true;
                s.body_style.metrics.frame_enabled = true;
            }

            policy_acc.SetStyle(s);
        };
        appearance_dark.WhenAction = ApplyAppearance;
        appearance_grad_from.WhenAction = ApplyAppearance;
        appearance_grad_to.WhenAction = ApplyAppearance;
        appearance_gap_slider.WhenAction = ApplyAppearance;
        appearance_unified.WhenAction = ApplyAppearance;

        policy_acc.WhenSectionToggled = [=](int, bool) { SyncPolicyStatus(); };
        ApplyAppearance();
        ApplyRulePolicy();
        SyncPolicyStatus();
    }

    void SyncPolicyStatus()
    {
        int open = 0, f_open = 0, f_closed = 0;
        for(int i = 0; i < policy_acc.GetCount(); i++) {
            if(policy_acc.IsOpen(i)) open++;
            auto lk = policy_acc.GetLockMode(i);
            if(lk == UiAccordion::Lock::Open) f_open++;
            if(lk == UiAccordion::Lock::Closed) f_closed++;
        }
        policy_status.SetText(Format("Open=%d | force-open=%d | force-closed=%d", open, f_open, f_closed));
    }

    void SetupCompactAccordion()
    {
        UiAccordion::Style st = UiAccordion::StyleDefault();
        st.single_open = true;
        st.header_height = DPI(39);
        st.section_gap = 0;
        st.header_body_gap = 0;
        st.header_style.show_rule = false;
        st.chevron_side = UiAlign::LEFT;
        st.header_style.title_font = SansSerifZ(12).Bold();
        st.header_style.subtitle_font = SansSerifZ(8);
        st.header_style.title_subtitle_gap = 0;
        st.header_style.subtitle_copy_gap = 0;
        st.header_style.copy_font = SansSerifZ(1);
        st.header_style.metrics.content_padding = Rect(DPI(8), DPI(4), DPI(38), DPI(6));
        st.header_style.hover_enabled = true;
        st.unified_section_frame = false;
        st.unified_section_radius = DPI(6);
        st.metrics.frame_enabled = false;
        st.metrics.face_enabled = false;
        st.body_style.metrics.radius = 0;
        st.body_style.metrics.frame_width = 1;
        st.body_style.metrics.face_enabled = false;
        st.body_style.metrics.frame_enabled = false;
        st.header_style.metrics.frame_enabled = false;
        st.header_style.metrics.face_enabled = false;
        st.header_style.show_bottom_line = true;
        st.header_style.bottom_line_extent = LARGE;
        st.header_style.bottom_line_style = SOLID;
        st.header_style.bottom_line_thickness = 1;
        st.header_style.bottom_line_color = Color(112, 123, 142);
        st.body_line_extent = LARGE;
        st.body_line_style = SOLID;
        st.body_line_thickness = 1;
        st.body_line_color = Color(112, 123, 142);
        compact_acc.SetStyle(st);
        compact_acc.EnableDragReorder(true);

        int a = compact_acc.AddSection("Compact / Alpha", "drag handle + left chevron", String(), true);
        int b = compact_acc.AddSection("Compact / Beta", "flat section frame", String(), false);
        int c = compact_acc.AddSection("Compact / Gamma", "header drag reorder", String(), false);

        Image drag_icon = ICON_DESIGN_DRAG_INDICATOR_48();
        UiButton::Style drag_style = UiButton::StyleIcon();
        drag_style.metrics.frame_enabled = false;
        drag_style.metrics.face_enabled = false;
        drag_style.focus_margin = 0;
        drag_style.icon_margin = Rect(DPI(1), DPI(1), DPI(1), DPI(1));
        compact_drag_a.SetStyle(drag_style).SetText("").SetIcon(drag_icon).SetIconScale(true).Tip("Drag handle").IgnoreMouse();
        compact_drag_b.SetStyle(drag_style).SetText("").SetIcon(drag_icon).SetIconScale(true).Tip("Drag handle").IgnoreMouse();
        compact_drag_c.SetStyle(drag_style).SetText("").SetIcon(drag_icon).SetIconScale(true).Tip("Drag handle").IgnoreMouse();
        compact_acc.Header(a).Add(compact_drag_a);
        compact_acc.Header(b).Add(compact_drag_b);
        compact_acc.Header(c).Add(compact_drag_c);

        compact_acc.Content(a).Add(compact_a.SizePos());
        compact_a.SetText("Single rectangle feel: no gap, compact header.")
                .SetAlign(UiAlign::LEFT, UiAlign::TOP)
                .SetInset(DPI(8));
        compact_acc.Content(b).Add(compact_b.SizePos());
        compact_b.SetText("Body can still host controls.").SetAlign(UiAlign::LEFT, UiAlign::TOP).SetInset(DPI(8));
        compact_acc.Content(c).Add(compact_c.SizePos());
        compact_c.SetText("Hover + drag reorder active on headers.").SetAlign(UiAlign::LEFT, UiAlign::TOP).SetInset(DPI(8));

        compact_acc.SetSectionBodyHeight(a, DPI(74)).SetSectionBodyHeight(b, DPI(74)).SetSectionBodyHeight(c, DPI(74));

        compact_acc.WhenReordered = [=](int from, int before) {
            compact_a.SetText(Format("Reordered: %d -> before %d", from, before));
        };
    }

    void SetupGradientAccordion()
    {
        UiAccordion::Style st = UiAccordion::StyleDefault();
        st.single_open = false;
        st.header_height = DPI(56);
        st.section_gap = 0;
        st.header_body_gap = 0;
        st.metrics.frame_enabled = false;
        st.metrics.face_enabled = false;
        st.skin.enabled = true;
        st.skin.base = MakeBrutalistPanelSkin();
        st.skin.slice = Rect(DPI(15), DPI(15), DPI(15), DPI(15));
        st.skin.content_inset = Rect(DPI(10), DPI(10), DPI(16), DPI(16));
        for(int i = 0; i < 4; i++) {
            st.palette.face[i] = UiFill::Solid(Color(255, 228, 0));
            st.palette.frame[i] = Black();
            st.palette.ink[i] = Black();
        }

        for(int i = 0; i < 4; i++) {
            st.header_style.palette.face[i] = UiFill::Solid(Color(255, 228, 0));
            st.header_style.palette.frame[i] = Black();
            st.header_style.palette.ink[i] = Black();
            st.body_style.palette.face[i] = UiFill::Solid(Color(255, 228, 0));
            st.body_style.palette.frame[i] = Black();
            st.body_style.palette.ink[i] = Black();
        }
        st.header_style.palette.face[ST_HOT] = UiFill::Solid(Color(255, 236, 64));
        st.header_style.palette.face[ST_PRESSED] = UiFill::Solid(Color(240, 206, 0));
        st.header_style.palette.face[ST_DISABLED] = UiFill::Solid(Color(232, 232, 232));

        st.header_style.metrics.radius = 0;
        st.header_style.metrics.frame_width = 0;
        st.body_style.metrics.radius = 0;
        st.body_style.metrics.frame_width = 0;
        st.unified_section_frame = false;
        st.header_style.metrics.frame_enabled = false;
        st.body_style.metrics.frame_enabled = false;
        st.header_style.metrics.face_enabled = false;
        st.body_style.metrics.face_enabled = false;
        st.header_style.show_bottom_line = true;
        st.header_style.bottom_line_extent = SMALL;
        st.header_style.bottom_line_style = DASHED;
        st.header_style.bottom_line_thickness = 3;
        st.header_style.bottom_line_color = Color(34, 34, 34);
        st.body_line_extent = NONE;
        st.show_chevron = true;
        st.chevron_side = UiAlign::RIGHT;
        st.chevron_scale = true;
        st.glyph_open = ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48();
        st.glyph_closed = ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48();
        st.header_style.show_rule = false;
        st.header_style.metrics.content_padding = Rect(DPI(12), DPI(8), DPI(44), DPI(7));
        st.body_style.metrics.content_padding = Rect(DPI(12), DPI(6), DPI(12), DPI(8));
        gradient_acc.SetStyle(st);

        int a = gradient_acc.AddSection("Brutalist Design", "The Harder Edge to Life", "SYSTEM MONO", true);
        int b = gradient_acc.AddSection("UTILITY BLOCK", "ICON MATRIX", "STRICT FRAMES", false);


        UiPanel::Style brutal_card = UiPanel::StyleFlat();
        brutal_card.metrics.frame_enabled = true;
        brutal_card.metrics.face_enabled = true;
        brutal_card.metrics.frame_width = 3;
        brutal_card.metrics.radius = DPI(12);
        for(int i = 0; i < 4; i++) {
            brutal_card.palette.face[i] = UiFill::Solid(Color(236, 236, 236));
            brutal_card.palette.frame[i] = Black();
            brutal_card.palette.ink[i] = Color(64, 73, 90);
        }

        gradient_acc.Content(a).Add(brutal_top_wrap.SizePos());
        brutal_top_wrap.SetDirection(UiDirection::V).SetGap(0).SetInset(Rect(DPI(40), DPI(10), DPI(40), DPI(12)));
        brutal_top_wrap.Add(brutal_top_panel).Expand(1);
        brutal_top_panel.SetStyle(brutal_card);
        brutal_top_panel.Add(gradient_tools.SizePos());
        gradient_tools.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));
        gradient_tools.Add(gradient_note).Fixed(DPI(22));
        gradient_tools.Add(grad_scale_row).Fixed(DPI(34));
        gradient_tools.Add(grad_icons_panel).Expand(1);
        grad_icons_panel.SetStyle(UiScrollPanel::StyleFlat());
        grad_icons_panel.SetScrollMode(UIPANELSCROLL_NONE);
        grad_icons_panel.Content().Add(icon_grid);

        gradient_note.SetText("Brutalist style via base metrics/palette (no custom paint)").SetStyle(UiLabel::StyleCaption());

        grad_scale_row.SetDirection(UiDirection::H).SetGap(DPI(8));
        grad_scale_row.Add(grad_scale_toggle).Fixed(DPI(120));
        grad_scale_row.Add(grad_scale_slider).Expand(1);
        grad_scale_row.Add(grad_scale_value).Fixed(DPI(64));
        grad_scale_toggle.SetText("Scale chevron").SetChecked(true);
        grad_scale_slider.SetRange(8, 32).SetStep(1).SetValue(14).SetTicks(true, 8, 1);
        grad_scale_value.SetText("14px").SetAlign(UiAlign::RIGHT, UiAlign::CENTER).SetStyle(UiLabel::StyleCaption());

        icon_grid.SetMode(UiGridLayout::Flow)
                 .SetDirection(UiDirection::H)
                 .SetWrap(true)
                 .SetGap(DPI(6))
                 .SetInset(DPI(2));

        Vector<Image> icons;
        Vector<String> icon_names;
        BuildUiIconsGallery(icons, icon_names);
        int icon_count = min(icons.GetCount(), icon_names.GetCount());
        UiButton::Style icon_style = UiButton::StyleIcon();
        icon_style.metrics.frame_enabled = false;
        icon_style.metrics.face_enabled = false;
        icon_style.metrics.radius = 0;
        for(int stt = 0; stt < 4; stt++)
            icon_style.palette.ink[stt] = Black();
        for(int i = 0; i < icon_count; i++) {
            UiButton& b = icon_cells.Add();
            b.SetMinSize(Size(DPI(42), DPI(42)));
            b.SetStyle(icon_style).SetText("").SetIcon(icons[i]).SetIconScale(false).Tip(icon_names[i]);
            icon_grid.Add(b);
        }

        gradient_acc.Content(b).Add(brutal_nested_wrap.SizePos());
        brutal_nested_wrap.SetDirection(UiDirection::V).SetGap(0).SetInset(Rect(DPI(40), DPI(10), DPI(40), DPI(18)));
        brutal_nested_wrap.Add(brutal_nested).Expand(1);

        UiAccordion::Style nested = UiAccordion::StyleDefault();
        nested.header_height = DPI(42);
        nested.section_gap = 0;
        nested.header_body_gap = 0;
        nested.metrics.radius = DPI(12);
        nested.metrics.frame_width = 3;
        nested.metrics.frame_enabled = true;
        nested.metrics.face_enabled = true;
        for(int i = 0; i < 4; i++) {
            nested.palette.face[i] = UiFill::Solid(Color(236, 236, 236));
            nested.palette.frame[i] = Black();
            nested.palette.ink[i] = Color(64, 73, 90);
            nested.header_style.palette.face[i] = UiFill::Solid(Color(236, 236, 236));
            nested.header_style.palette.frame[i] = Color(210, 210, 210);
            nested.header_style.palette.ink[i] = Color(74, 86, 107);
            nested.body_style.palette.face[i] = UiFill::Solid(Color(236, 236, 236));
            nested.body_style.palette.frame[i] = Color(210, 210, 210);
            nested.body_style.palette.ink[i] = Color(74, 86, 107);
        }
        nested.header_style.metrics.frame_enabled = false;
        nested.body_style.metrics.frame_enabled = false;
        nested.header_style.metrics.face_enabled = false;
        nested.body_style.metrics.face_enabled = false;
        nested.header_style.title_font = SansSerifZ(12).Bold();
        nested.header_style.subtitle_font = SansSerifZ(1);
        nested.header_style.copy_font = SansSerifZ(1);
        nested.header_style.show_rule = false;
        nested.show_chevron = true;
        nested.chevron_side = UiAlign::RIGHT;
        nested.chevron_scale = true;
        nested.chevron_size = DPI(14);
        nested.glyph_open = ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48();
        nested.glyph_closed = ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_RIGHT_48();
        nested.unified_section_frame = false;
        brutal_nested.SetStyle(nested);

        int n0 = brutal_nested.AddSection("Configuration Set", String(), String(), true);
        int n1 = brutal_nested.AddSection("Data Lifecycle", String(), String(), false);

        brutal_nested.Content(n0).Add(brutal_cfg_wrap.SizePos());
        brutal_cfg_wrap.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));
        brutal_cfg_wrap.Add(brutal_cfg_row_a).Fixed(DPI(34));
        brutal_cfg_wrap.Add(brutal_cfg_row_b).Fixed(DPI(34));

        brutal_cfg_row_a.SetDirection(UiDirection::H).SetGap(DPI(10));
        brutal_cfg_row_a.Add(brutal_cfg_label_a).Expand(1);
        brutal_cfg_row_a.Add(brutal_cfg_toggle).Fixed(DPI(44));
        brutal_cfg_label_a.SetText("HARDWARE BOOST").SetStyle(UiLabel::StyleCaption());
        brutal_cfg_toggle.SetStyle(UiCheckBox::StyleSwitch()).SetText("").SetChecked(false);

        brutal_cfg_row_b.SetDirection(UiDirection::H).SetGap(DPI(10));
        brutal_cfg_row_b.Add(brutal_cfg_label_b).Expand(1);
        brutal_cfg_row_b.Add(brutal_cfg_icon).Fixed(DPI(28));
        brutal_cfg_label_b.SetText("SOLID PROTOCOL").SetStyle(UiLabel::StyleCaption());
        UiButton::Style tiny_icon = UiButton::StyleIcon();
        tiny_icon.metrics.frame_enabled = true;
        tiny_icon.metrics.frame_width = 2;
        tiny_icon.metrics.radius = DPI(6);
        for(int i = 0; i < 4; i++) {
            tiny_icon.palette.face[i] = UiFill::Solid(Color(255, 228, 0));
            tiny_icon.palette.frame[i] = Black();
            tiny_icon.palette.ink[i] = Black();
        }
        brutal_cfg_icon.SetStyle(tiny_icon)
            .SetText("")
            .SetIcon(ICON_CONTENT_OUTLINED_ADD_48())
            .SetIconScale(true)
            .SetMinSize(Size(DPI(24), DPI(24)));

        brutal_nested.Content(n1).Add(brutal_icon_grid.SizePos());
        brutal_icon_grid.SetMode(UiGridLayout::Flow)
            .SetDirection(UiDirection::H)
            .SetWrap(true)
            .SetGap(DPI(8))
            .SetInset(DPI(8));
        UiButton::Style matrix_icon = UiButton::StyleIcon();
        matrix_icon.metrics.frame_enabled = false;
        matrix_icon.metrics.face_enabled = false;
        matrix_icon.metrics.radius = 0;
        for(int stt = 0; stt < 4; stt++)
            matrix_icon.palette.ink[stt] = Black();
        Vector<Image> nested_icons;
        Vector<String> nested_names;
        BuildUiIconsGallery(nested_icons, nested_names);
        for(int i = 0; i < min(8, nested_icons.GetCount()); i++) {
            UiButton& ib = brutal_icon_cells.Add();
            ib.SetStyle(matrix_icon).SetText("").SetIcon(nested_icons[i]).SetIconScale(false).Tip(nested_names[i]);
            ib.SetMinSize(Size(DPI(34), DPI(34)));
            brutal_icon_grid.Add(ib);
        }

        const int nested_body0 = DPI(92);
        const int nested_body1 = DPI(108);
        brutal_nested.SetSectionBodyHeight(n0, nested_body0).SetSectionBodyHeight(n1, nested_body1);

        auto ClampBodyHeight = [=](int desired) {
            int h = max(DPI(96), desired);
            const UiAccordion::Style gs = gradient_acc.GetStyle();
            int avail = gradient_acc.GetSize().cy;
            if(avail > 0) {
                int reserved = gs.header_height + gs.section_gap + gs.header_height + gs.header_body_gap;
                int max_body = max(DPI(96), avail - reserved - DPI(12));
                h = min(h, max_body);
            }
            return h;
        };

        auto SyncUtilityHeight = [=] {
            int h = brutal_nested.GetMinSize().cy + DPI(10) + DPI(18);
            h = ClampBodyHeight(h);

            gradient_acc.SetSectionBodyHeight(b, h);
            RefreshLayout();
            Refresh();
        };
        brutal_nested.WhenSectionToggled = [=](int, bool) { SyncUtilityHeight(); };

        gradient_acc.SetSectionBodyHeight(a, ClampBodyHeight(DPI(180)));
        SyncUtilityHeight();

        auto ApplyChevronScale = [=] {
            UiAccordion::Style s = gradient_acc.GetStyle();
            s.chevron_scale = grad_scale_toggle.IsChecked();
            s.chevron_size = (int)grad_scale_slider.GetData();
            gradient_acc.SetStyle(s);
            grad_scale_value.SetText(Format("%dpx", s.chevron_size));
            Refresh();
        };
        grad_scale_toggle.WhenAction = ApplyChevronScale;
        grad_scale_slider.WhenAction = ApplyChevronScale;
    }

    void SetupNeoAccordion()
    {
        UiAccordion::Style st = UiAccordion::StyleDefault();
        Image skin = MakeNeoSkin30();

        for(int i = 0; i < 4; i++) {
            st.palette.face[i] = UiFill::Solid(Color(229, 234, 241));
            st.palette.frame[i] = Color(229, 234, 241);
            st.palette.ink[i] = Color(56, 74, 98);
        }
        st.metrics.frame_enabled = false;
        st.metrics.face_enabled = true;

        st.header_style.skin.enabled = true;
        st.header_style.skin.base = skin;
        st.header_style.skin.slice = Rect(DPI(12), DPI(12), DPI(12), DPI(12));
        st.header_style.skin.content_inset = Rect(DPI(12), DPI(10), DPI(12), DPI(10));
        st.header_style.metrics.face_enabled = false;
        st.header_style.metrics.frame_enabled = false;
        st.header_style.hover_enabled = true;
        st.header_style.title_font = SansSerifZ(16).Bold();
        st.header_style.subtitle_font = SansSerifZ(10);
        st.header_style.copy_font = SansSerifZ(10);
        st.header_style.title_subtitle_gap = 1;
        st.header_style.subtitle_copy_gap = 1;
        st.header_style.show_rule = false;

        st.body_style.skin.enabled = true;
        st.body_style.skin.base = skin;
        st.body_style.skin.slice = Rect(DPI(12), DPI(12), DPI(12), DPI(12));
        st.body_style.skin.content_inset = Rect(DPI(12), DPI(10), DPI(12), DPI(10));
        st.body_style.metrics.face_enabled = false;
        st.body_style.metrics.frame_enabled = false;

        st.section_gap = DPI(8);
        st.header_height = DPI(66);
        st.header_body_gap = 0;
        st.unified_section_frame = true;
        st.unified_section_radius = DPI(16);
        st.unified_section_frame_width = 0;
        st.show_chevron = true;
        st.chevron_side = UiAlign::LEFT;
        st.chevron_scale = true;
        st.chevron_size = DPI(18);
        st.glyph_open = ICON_CONTENT_OUTLINED_REMOVE_CIRCLE_OUTLINE_48();
        st.glyph_closed = ICON_CONTENT_OUTLINED_ADD_CIRCLE_OUTLINE_48();
        st.header_style.show_bottom_line = false;
        st.body_line_extent = NONE;
        neo_acc.SetStyle(st);

        int a = neo_acc.AddSection("Profile Settings", "Manage your personal data and visibility preferences in a soft UI environment.", String(), true);
        int b = neo_acc.AddSection("Privacy Policy", "Soft stacked cards with left action glyphs.", String(), false);

        neo_acc.Header(b).ShowRule(false);

        neo_acc.Content(a).Add(neo_layout.SizePos());
        neo_layout.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(DPI(8));
        neo_layout.Add(neo_btn_a).Fixed(DPI(120));
        neo_layout.Add(neo_btn_b).Fixed(DPI(120));
        neo_layout.Add(neo_toggle).Fixed(DPI(110));
        neo_btn_a.SetText("Run").SetSubtleStyle();
        neo_btn_b.SetText("Apply").SetSubtleStyle();
        neo_toggle.SetText("Auto").SetOn(true);

        neo_acc.Content(b).Add(neo_text.SizePos());
        neo_text.SetText("This variant demonstrates nine-slice depth without custom paint hooks.").SetInset(DPI(8));

        neo_acc.SetSectionBodyHeight(a, DPI(78)).SetSectionBodyHeight(b, DPI(82));
    }

private:
    UiLabel title_lbl;

    UiAccordion policy_acc;
    UiAccordion compact_acc;
    UiAccordion gradient_acc;
    UiAccordion neo_acc;

    UiBoxLayout policy_controls { UiDirection::V };
    UiBoxLayout row_policy_a { UiDirection::H };
    UiBoxLayout row_policy_b { UiDirection::H };
    UiBoxLayout row_policy_c { UiDirection::H };
    UiBoxLayout row_policy_d { UiDirection::H };
    UiCheckBox policy_single;
    UiCheckBox policy_enforce;
    UiButton policy_open_all;
    UiButton policy_close_all;
    UiCheckBox lock_open_b;
    UiCheckBox lock_closed_c;
    UiButton lock_none_all;
    UiCheckBox anim_enabled;
    UiSlider anim_open;
    UiSlider anim_close;
    UiLabel anim_value;
    UiLabel rule_mode_lbl;
    DropList rule_mode;
    UiSlider rule_thickness;
    UiLabel rule_value;
    UiLabel policy_status;
    UiBoxLayout policy_appearance { UiDirection::V };
    UiCheckBox appearance_dark;
    UiBoxLayout row_appearance_gradient { UiDirection::H };
    UiLabel appearance_grad_lbl;
    ColorPusher appearance_grad_from;
    ColorPusher appearance_grad_to;
    UiLabel appearance_grad_value;
    UiBoxLayout row_appearance_gap { UiDirection::H };
    UiLabel appearance_gap_lbl;
    UiSlider appearance_gap_slider;
    UiLabel appearance_gap_value;
    UiCheckBox appearance_unified;
    UiMultiEdit policy_notes;

    UiLabel compact_a;
    UiLabel compact_b;
    UiLabel compact_c;
    UiButton compact_drag_a;
    UiButton compact_drag_b;
    UiButton compact_drag_c;

    UiLabel gradient_note;
    UiBoxLayout gradient_tools { UiDirection::V };
    UiBoxLayout grad_scale_row { UiDirection::H };
    UiCheckBox grad_scale_toggle;
    UiSlider grad_scale_slider;
    UiLabel grad_scale_value;
    UiBoxLayout brutal_top_wrap { UiDirection::V };
    UiPanel brutal_top_panel;
    UiGridLayout icon_grid;
    UiScrollPanel grad_icons_panel;
    Array<UiButton> icon_cells;
    UiScrollPanel grad_panel;
    UiAccordion brutal_nested;
    UiBoxLayout brutal_nested_wrap { UiDirection::V };
    UiBoxLayout brutal_cfg_wrap { UiDirection::V };
    UiBoxLayout brutal_cfg_row_a { UiDirection::H };
    UiBoxLayout brutal_cfg_row_b { UiDirection::H };
    UiLabel brutal_cfg_label_a;
    UiLabel brutal_cfg_label_b;
    UiToggle brutal_cfg_toggle;
    UiButton brutal_cfg_icon;
    UiGridLayout brutal_icon_grid;
    Array<UiButton> brutal_icon_cells;

    UiBoxLayout neo_layout { UiDirection::H };
    UiButton neo_btn_a;
    UiButton neo_btn_b;
    UiToggle neo_toggle;
    UiLabel neo_text;
};

GUI_APP_MAIN
{
    UiAccordionDemoWindow().Run();
}
