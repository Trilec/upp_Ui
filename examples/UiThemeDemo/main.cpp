#include <Ui/Ui.h>

using namespace Upp;

static void PaintVerticalGradient(Draw& w, const Rect& r, Color top, Color bottom)
{
    if(r.IsEmpty())
        return;
    int h = max(1, r.GetHeight());
    for(int i = 0; i < h; i++) {
        int t = (255 * i) / max(1, h - 1);
        w.DrawRect(r.left, r.top + i, r.GetWidth(), 1, Blend(bottom, top, t));
    }
}

class ThemeSpecimen : public ParentCtrl {
public:
    typedef ThemeSpecimen CLASSNAME;

    ThemeSpecimen(UiThemePreset preset, const String& title)
        : preset_(preset), title_text_(title)
    {
        Add(card_title_);
        Add(shell_);

        shell_.Add(menu_file_);
        shell_.Add(menu_edit_);
        shell_.Add(menu_view_);
        shell_.Add(menu_help_);

        shell_.Add(tool_list_);
        shell_.Add(tool_clarify_);
        shell_.Add(tool_table_);
        shell_.Add(tool_settings_);

        shell_.Add(crumb_);
        shell_.Add(tabs_);
        shell_.Add(status_left_);
        shell_.Add(status_right_);

        page_general_.Add(sidebar_);
        page_general_.Add(form_panel_);
        page_empty_a_.Add(empty_a_);
        page_empty_b_.Add(empty_b_);

        sidebar_.Add(tree_box_);
        sidebar_.Add(list_box_);
        tree_box_.Add(tree_label_);
        tree_box_.Add(tree_);
        list_box_.Add(list_label_);
        list_box_.Add(list_);

        form_panel_.Add(group_title_);
        form_panel_.Add(search_);
        form_panel_.Add(config_);
        form_panel_.Add(toggle_);
        form_panel_.Add(toggle_label_);
        form_panel_.Add(option_a_);
        form_panel_.Add(option_b_);
        form_panel_.Add(enabled_);
        form_panel_.Add(primary_action_);
        form_panel_.Add(secondary_action_);

        search_.AddToSide(search_icon_, UiAlign::RIGHT, Size(DPI(18), DPI(18))).Overlay(true);

        menu_file_.SetText("File");
        menu_edit_.SetText("Edit");
        menu_view_.SetText("View");
        menu_help_.SetText("Help");

        crumb_.EnableRich();

        tabs_.SetPlacement(UiAlign::TOP);
        tabs_.Add(page_general_, "General");
        tabs_.Add(page_empty_a_, "Advanced");
        tabs_.Add(page_empty_b_, "Security");

        tree_label_.SetText("UITREE");
        BuildTreeModel();
        list_label_.SetText("UILIST");
        BuildListModel();
        empty_a_.SetText("Single-page specimen. Additional tab pages intentionally left quiet.").SetAlign(UiAlign::LEFT, UiAlign::TOP);
        empty_b_.SetText("Single-page specimen. Additional tab pages intentionally left quiet.").SetAlign(UiAlign::LEFT, UiAlign::TOP);

        group_title_.SetText("CONFIGURATION");
        search_.SetPlaceholder("Search presets");
        config_.Add("Standard Configuration", 0)
               .Add("Custom Setup", 1)
               .Add("Audit Mode", 2)
               .SetData(0);
        toggle_.SetText("").SetOn(true);
        toggle_label_.SetText("Notifications");
        option_a_.SetText("Option A").SetGroup(1).SetChecked(true);
        option_b_.SetText("Option B").SetGroup(1);
        enabled_.SetText("Enabled").SetChecked(true);
        primary_action_.SetText("Apply");
        secondary_action_.SetText("Cancel");
        status_left_.SetText("Ready");
        status_right_.SetText("Ln 12, Col 4");

        search_icon_.NoWantFocus();
        card_title_.SetText(title_text_);
        ApplyTheme(UiThemeMode::Light);
    }

    void ApplyTheme(UiThemeMode mode)
    {
        mode_ = mode;

        UiLabel::Style label_body = UiTheme::ResolveLabel(preset_, mode_, UiLabelRole::Body);
        UiLabel::Style label_caption = UiTheme::ResolveLabel(preset_, mode_, UiLabelRole::Caption);
        UiLabel::Style heading_style = UiTheme::ResolveLabel(UiThemePreset::Minimal, mode_, UiLabelRole::Caption);
        heading_style.font = SansSerifZ(11).Bold();
        heading_style.metrics.text_font = heading_style.font;
        heading_style.metrics.use_text_font = true;

        UiPanel::Style shell = UiTheme::ResolvePanel(preset_, mode_, UiPanelRole::Surface);
        UiPanel::Style subtle_panel = UiTheme::ResolvePanel(preset_, mode_, UiPanelRole::Subtle);
        UiPanel::Style strong_panel = UiTheme::ResolvePanel(preset_, mode_, UiPanelRole::Strong);
        UiToolButton::Style icon_button = UiTheme::ResolveToolButton(preset_, mode_);
        UiButton::Style primary_button = UiTheme::ResolveButton(preset_, mode_, UiButtonRole::Accent);
        UiButton::Style subtle_button = UiTheme::ResolveButton(preset_, mode_, UiButtonRole::Subtle);
        UiBaseEdit::Style edit_style = UiTheme::ResolveEdit(preset_, mode_, UiEditRole::Field);
        UiDropdown::Style dropdown_style = UiTheme::ResolveDropdown(preset_, mode_);
        UiTab::Style tab_style = UiTheme::ResolveTab(preset_, mode_, preset_ == UiThemePreset::Minimal ? UITAB_SEGMENTED : UITAB_CLASSIC);
        UiToggle::Style toggle_style = UiTheme::ResolveToggle(preset_, mode_);
        UiCheckBox::Style check_style = UiTheme::ResolveCheckBox(preset_, mode_, UICHECKVIS_CLASSIC);
        UiRadioButton::Style radio_style = UiTheme::ResolveRadioButton(preset_, mode_, preset_ == UiThemePreset::Rounded ? UIRADIOVIS_PILLS : UIRADIOVIS_CLASSIC);

        card_title_.SetStyle(heading_style).SetInkColor(mode_ == UiThemeMode::Dark ? Color(255, 255, 255) : Color(22, 32, 51));

        shell_.SetStyle(shell);
        sidebar_.SetStyle(subtle_panel);
        tree_box_.SetStyle(strong_panel);
        list_box_.SetStyle(strong_panel);
        form_panel_.SetStyle(strong_panel);
        tabs_.SetStyle(tab_style);

        tool_list_.SetStyle(icon_button).SetIcon(ICON_EDITOR_FORMAT_LIST_BULLETED_48()).SetIconScale(true).SetIconTintMono(true);
        tool_clarify_.SetStyle(icon_button).SetIcon(ICON_EDITOR_CLARIFY_48()).SetIconScale(true).SetIconTintMono(true);
        tool_table_.SetStyle(icon_button).SetIcon(ICON_EDITOR_TABLE_48()).SetIconScale(true).SetIconTintMono(true);
        tool_settings_.SetStyle(icon_button).SetIcon(ICON_DESIGN_SETTINGS_48()).SetIconScale(true).SetIconTintMono(true);
        search_icon_.SetStyle(icon_button).SetIcon(ICON_ACTION_SEARCH_48()).SetIconScale(true).SetIconTintMono(true);
        primary_action_.SetStyle(primary_button);
        secondary_action_.SetStyle(subtle_button);

        search_.SetStyle(edit_style);
        config_.SetStyle(dropdown_style);
        toggle_.SetStyle(toggle_style);
        enabled_.SetStyle(check_style);
        option_a_.SetStyle(radio_style);
        option_b_.SetStyle(radio_style);

        menu_file_.SetStyle(label_caption).SetInkColor(label_caption.palette.ink[ST_NORMAL]);
        menu_edit_.SetStyle(label_caption).SetInkColor(label_caption.palette.ink[ST_NORMAL]);
        menu_view_.SetStyle(label_caption).SetInkColor(label_caption.palette.ink[ST_NORMAL]);
        menu_help_.SetStyle(label_caption).SetInkColor(label_caption.palette.ink[ST_NORMAL]);

        crumb_.SetStyle(label_caption);
        crumb_.ClearSpans();
        crumb_.AddTextSpan("PROJECT", label_caption.palette.ink[ST_NORMAL], true);
        crumb_.AddTextSpan("  >  ", label_caption.palette.ink[ST_NORMAL], true);
        crumb_.AddTextSpan("SETTINGS", mode_ == UiThemeMode::Dark ? Color(105, 165, 255) : Color(65, 167, 248), true);

        tree_label_.SetStyle(label_caption);
        tree_.SetStyle(UiTheme::ResolveTree(preset_, mode_));
        list_label_.SetStyle(label_caption);
        list_.SetStyle(UiTheme::ResolveList(preset_, mode_));
        empty_a_.SetStyle(label_body);
        empty_b_.SetStyle(label_body);
        group_title_.SetStyle(label_caption);
        toggle_label_.SetStyle(label_body);
        status_left_.SetStyle(label_caption);
        status_right_.SetStyle(label_caption).SetAlign(UiAlign::RIGHT, UiAlign::CENTER);

        Refresh();
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int title_h = DPI(20);
        card_title_.SetRect(0, 0, r.GetWidth(), title_h);
        shell_.SetRect(0, title_h + DPI(8), r.GetWidth(), max(0, r.GetHeight() - title_h - DPI(8)));

        Rect inner = shell_.GetSize();
        int x = DPI(18);
        int y = DPI(18);
        int menu_gap = DPI(18);
        int menu_w = DPI(40);
        menu_file_.SetRect(x + (menu_w + menu_gap) * 0, y, menu_w, DPI(18));
        menu_edit_.SetRect(x + (menu_w + menu_gap) * 1, y, menu_w, DPI(18));
        menu_view_.SetRect(x + (menu_w + menu_gap) * 2, y, menu_w, DPI(18));
        menu_help_.SetRect(x + (menu_w + menu_gap) * 3, y, menu_w, DPI(18));

        y += DPI(32);
        int icon = DPI(34);
        int icon_gap = DPI(10);
        tool_list_.SetRect(x + (icon + icon_gap) * 0, y, icon, icon);
        tool_clarify_.SetRect(x + (icon + icon_gap) * 1, y, icon, icon);
        tool_table_.SetRect(x + (icon + icon_gap) * 2, y, icon, icon);
        tool_settings_.SetRect(x + (icon + icon_gap) * 3, y, icon, icon);

        int crumb_y = y + DPI(6);
        crumb_.SetRect(inner.GetWidth() - x - DPI(188), crumb_y, DPI(188), DPI(18));

        y += DPI(54);
        int footer_h = DPI(26);
        tabs_.SetRect(x, y, inner.GetWidth() - x * 2, max(DPI(300), inner.GetHeight() - y - footer_h - DPI(18)));
        int footer_y = inner.GetHeight() - footer_h - DPI(10);
        status_left_.SetRect(x, footer_y, DPI(180), footer_h);
        status_right_.SetRect(inner.GetWidth() - x - DPI(120), footer_y, DPI(120), footer_h);

        LayoutGeneral();
    }

private:
    void BuildTreeModel()
    {
        UiTreeNodeRef root = tree_model_.Root();
        UiTreeNodeRef project = tree_model_.AddChild(root, UiModelItem("Project"));
        UiTreeNodeRef settings = tree_model_.AddChild(project, UiModelItem("Settings"));
        tree_model_.AddChild(settings, UiModelItem("General"));
        tree_model_.AddChild(settings, UiModelItem("Appearance"));
        UiModelItem modules("Modules");
        modules.lazy_children = true;
        modules.right_text = "lazy";
        modules.has_metadata = true;
        modules.metadata_color = Color(65, 167, 248);
        UiTreeNodeRef lazy = tree_model_.AddChild(project, modules);
        tree_.SetModel(tree_model_);
        tree_.SetRootVisible(false);
        tree_.SetGlyphStyle(UITREEGLYPH_PLUSMINUS);
        tree_.ShowConnectorLines(true);
        tree_.WhenLazyLoad = [=](UiTreeNodeRef node) {
            if(tree_model_.GetChildCount(node) == 0) {
                tree_model_.AddChild(node, UiModelItem("Deferred A"));
                tree_model_.AddChild(node, UiModelItem("Deferred B"));
                tree_model_.AddChild(node, UiModelItem("Deferred C"));
            }
            tree_.MarkNodeChildrenLoaded(node, true);
            tree_.Expand(node, true, false);
        };
        tree_.Expand(tree_model_.Root(), true, true);
        tree_.SetCursor(lazy);
    }

    void BuildListModel()
    {
        model_list_.Clear();

        UiModelItem presets("Theme Presets");
        presets.group_header = true;
        model_list_.Add(presets);

        UiModelItem minimal("Minimal");
        minimal.checked = true;
        minimal.right_text = "active";
        minimal.icon = ICON_EDITOR_FORMAT_LIST_BULLETED_48();
        minimal.mono_icon = true;
        minimal.has_metadata = true;
        minimal.metadata_color = Color(65, 167, 248);
        model_list_.Add(minimal);

        UiModelItem rounded("Rounded");
        rounded.right_text = "preview";
        rounded.icon = ICON_DESIGN_FOLDER_48();
        rounded.mono_icon = true;
        model_list_.Add(rounded);

        UiModelItem runtime("Runtime");
        runtime.group_header = true;
        runtime.separator_before = true;
        model_list_.Add(runtime);

        UiModelItem notify("Notifications");
        notify.checked = true;
        notify.right_text = "enabled";
        notify.has_metadata = true;
        notify.metadata_color = Color(22, 163, 74);
        model_list_.Add(notify);

        UiModelItem output("Output Folder");
        output.right_text = "build";
        output.underline = true;
        output.underline_color = Color(65, 167, 248);
        model_list_.Add(output);

        list_.SetModel(model_list_);
        list_.SetSelectionMode(UILISTSEL_SINGLE);
        if(model_list_.GetCount() > 0)
            list_.SetCursor(1);
    }
    void LayoutGeneral()
    {
        Rect r = page_general_.GetSize();
        int m = DPI(8);
        int gap = DPI(14);
        int sidebar_w = max(DPI(180), (r.GetWidth() * 34) / 100);
        sidebar_.SetRect(m, m, sidebar_w, max(0, r.GetHeight() - m * 2));
        form_panel_.SetRect(sidebar_w + gap, m, max(0, r.GetWidth() - sidebar_w - gap - m), max(0, r.GetHeight() - m * 2));

        Rect s = sidebar_.GetSize();
        int inner = DPI(6);
        int box_h = max(DPI(120), (s.GetHeight() - inner * 3) / 2);
        tree_box_.SetRect(inner, inner, s.GetWidth() - inner * 2, box_h);
        list_box_.SetRect(inner, inner * 2 + box_h, s.GetWidth() - inner * 2, max(DPI(100), s.GetHeight() - (inner * 3 + box_h)));

        Rect tree = tree_box_.GetSize();
        tree_label_.SetRect(DPI(14), DPI(14), tree.GetWidth() - DPI(28), DPI(16));
        tree_.SetRect(DPI(10), DPI(36), tree.GetWidth() - DPI(20), max(DPI(72), tree.GetHeight() - DPI(46)));

        Rect list = list_box_.GetSize();
        list_label_.SetRect(DPI(14), DPI(14), list.GetWidth() - DPI(28), DPI(16));
        list_.SetRect(DPI(10), DPI(36), list.GetWidth() - DPI(20), max(DPI(72), list.GetHeight() - DPI(46)));

        Rect f = form_panel_.GetSize();
        group_title_.SetRect(DPI(16), DPI(16), f.GetWidth() - DPI(32), DPI(16));
        search_.SetRect(DPI(16), DPI(42), f.GetWidth() - DPI(32), DPI(36));
        config_.SetRect(DPI(16), DPI(90), f.GetWidth() - DPI(32), DPI(36));
        toggle_.SetRect(DPI(16), DPI(144), DPI(42), DPI(24));
        toggle_label_.SetRect(DPI(72), DPI(144), f.GetWidth() - DPI(88), DPI(24));
        option_a_.SetRect(DPI(16), DPI(190), max(DPI(130), (f.GetWidth() - DPI(48)) / 2), DPI(28));
        option_b_.SetRect(DPI(16 + max(DPI(130), (f.GetWidth() - DPI(48)) / 2) + DPI(8)), DPI(190), max(DPI(130), (f.GetWidth() - DPI(48)) / 2), DPI(28));
        enabled_.SetRect(DPI(16), DPI(228), f.GetWidth() - DPI(32), DPI(28));
        int btn_y = max(DPI(282), f.GetHeight() - DPI(50));
        primary_action_.SetRect(DPI(16), btn_y, DPI(106), DPI(34));
        secondary_action_.SetRect(DPI(132), btn_y, DPI(106), DPI(34));

        empty_a_.SetRect(DPI(16), DPI(18), page_empty_a_.GetSize().cx - DPI(32), page_empty_a_.GetSize().cy - DPI(36));
        empty_b_.SetRect(DPI(16), DPI(18), page_empty_b_.GetSize().cx - DPI(32), page_empty_b_.GetSize().cy - DPI(36));
    }

private:
    UiThemePreset preset_;
    UiThemeMode mode_ = UiThemeMode::Light;
    String title_text_;

    UiLabel card_title_;
    UiPanel shell_;

    UiLabel menu_file_;
    UiLabel menu_edit_;
    UiLabel menu_view_;
    UiLabel menu_help_;

    UiToolButton tool_list_;
    UiToolButton tool_clarify_;
    UiToolButton tool_table_;
    UiToolButton tool_settings_;

    UiLabel crumb_;

    UiTab tabs_;
    UiLabel status_left_;
    UiLabel status_right_;

    ParentCtrl page_general_;
    ParentCtrl page_empty_a_;
    ParentCtrl page_empty_b_;

    UiPanel sidebar_;
    UiPanel tree_box_;
    UiPanel list_box_;
    UiLabel tree_label_;
    UiTree tree_;
    UiTreeModel tree_model_;
    UiLabel list_label_;
    UiList list_;
    UiListModel model_list_;

    UiPanel form_panel_;
    UiLabel group_title_;
    UiLineEdit search_;
    UiToolButton search_icon_;
    UiDropdown config_;
    UiToggle toggle_;
    UiLabel toggle_label_;
    UiRadioButton option_a_;
    UiRadioButton option_b_;
    UiCheckBox enabled_;
    UiButton primary_action_;
    UiButton secondary_action_;

    UiLabel empty_a_;
    UiLabel empty_b_;
};

class UiThemeDemoWindow : public TopWindow {
public:
    typedef UiThemeDemoWindow CLASSNAME;

    UiThemeDemoWindow()
        : minimal_(UiThemePreset::Minimal, "MINIMAL"),
          rounded_(UiThemePreset::Rounded, "ROUNDED")
    {
        Title("UiTheme Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1560), DPI(980));
        BackPaint();

        Add(title_);
        Add(copy_);
        Add(mode_shell_);
        mode_shell_.Add(mode_label_);
        mode_shell_.Add(mode_toggle_);
        Add(minimal_);
        Add(rounded_);

        title_.SetText("Preset Theme Directions").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        copy_.SetText("Minimal is being tuned against the HTML preset contract first. Rounded remains live for side-by-side comparison while the fidelity pass continues.")
             .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        mode_label_.SetText("Light").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        mode_toggle_.SetText("").SetOn(false).SetShowFocus(false);
        mode_toggle_.WhenAction = [=] { ApplyMode(mode_toggle_.GetData() ? UiThemeMode::Dark : UiThemeMode::Light); };

        ApplyMode(UiThemeMode::Light);
    }

    void ApplyMode(UiThemeMode mode)
    {
        mode_ = mode;

        UiLabel::Style heading = UiTheme::ResolveLabel(UiThemePreset::Minimal, mode_, UiLabelRole::Headline);
        heading.align_h = UiAlign::CENTER;
        title_.SetStyle(heading);

        UiLabel::Style sub = UiTheme::ResolveLabel(UiThemePreset::Minimal, mode_, UiLabelRole::Subheadline);
        sub.align_h = UiAlign::CENTER;
        copy_.SetStyle(sub);

        UiPanel::Style mode_shell_style = UiTheme::ResolvePanel(UiThemePreset::Minimal, mode_, UiPanelRole::Surface);
        mode_shell_style.metrics.radius = DPI(999);
        mode_shell_style.metrics.frame_width = DPI(1);
        mode_shell_style.metrics.shadow.enabled = true;
        mode_shell_style.metrics.shadow.inset = false;
        mode_shell_style.metrics.shadow.distance = DPI(3);
        mode_shell_style.metrics.shadow.angle = 45;
        mode_shell_style.metrics.shadow.blur = DPI(15);
        mode_shell_style.metrics.shadow.alpha = mode_ == UiThemeMode::Dark ? 40 : 34;
        mode_shell_style.metrics.shadow.color = mode_ == UiThemeMode::Dark ? Color(0, 0, 0) : Color(148, 160, 176);
        mode_shell_.SetStyle(mode_shell_style);
        UiToggle::Style mode_toggle_style = UiTheme::ResolveToggle(UiThemePreset::Minimal, mode_);
        mode_toggle_style.metrics.face_enabled = false;
        mode_toggle_style.metrics.frame_enabled = false;
        mode_toggle_style.metrics.frame_width = 0;
        mode_toggle_style.metrics.radius = DPI(999);
        mode_toggle_style.metrics.content_padding = Rect(0, 0, 0, 0);
        mode_toggle_style.metrics.focus_enabled = false;
        mode_toggle_style.metrics.shadow.enabled = false;
        mode_toggle_style.metrics.shadow.inset = false;
        mode_toggle_style.metrics.shadow.distance = DPI(2);
        mode_toggle_style.metrics.shadow.angle = 45;
        mode_toggle_style.metrics.shadow.blur = DPI(15);
        mode_toggle_style.metrics.shadow.alpha = mode_ == UiThemeMode::Dark ? 32 : 24;
        mode_toggle_style.metrics.shadow.color = mode_ == UiThemeMode::Dark ? Color(0, 0, 0) : Color(148, 160, 176);
        mode_toggle_style.track_extent = Size(DPI(52), DPI(27));
        mode_toggle_style.label_gap = 0;
        mode_toggle_style.thumb_inset = DPI(2);
        mode_toggle_style.track_side = UiAlign::RIGHT;
        mode_toggle_style.font = SansSerifZ(13).Bold();
        mode_toggle_style.track_metrics.face_enabled = true;
        mode_toggle_style.track_metrics.frame_enabled = true;
        mode_toggle_style.track_metrics.frame_width = DPI(1);
        mode_toggle_style.track_metrics.radius = DPI(999);
        mode_toggle_style.thumb_metrics.face_enabled = true;
        mode_toggle_style.thumb_metrics.frame_enabled = true;
        mode_toggle_style.thumb_metrics.frame_width = DPI(1);
        mode_toggle_style.thumb_metrics.radius = DPI(999);
        if(mode_ == UiThemeMode::Dark) {
            UiThemeDetail::SetFace(mode_toggle_style.palette, Color(14, 24, 39), Color(14, 24, 39), Color(14, 24, 39), Color(14, 24, 39));
            UiThemeDetail::SetFrame(mode_toggle_style.palette, Color(45, 61, 87), Color(45, 61, 87), Color(45, 61, 87), Color(45, 61, 87));
            UiThemeDetail::SetInk(mode_toggle_style.palette, White(), White(), White(), Color(146, 160, 181));
            UiThemeDetail::SetFace(mode_toggle_style.track_palette, Color(36, 49, 71), Color(36, 49, 71), Color(23, 35, 54), Color(30, 41, 59));
            UiThemeDetail::SetFrame(mode_toggle_style.track_palette, Color(66, 84, 112), Color(66, 84, 112), Color(66, 84, 112), Color(57, 72, 97));
            UiThemeDetail::SetFace(mode_toggle_style.thumb_palette, Color(241, 245, 249), Color(241, 245, 249), Color(241, 245, 249), Color(196, 205, 219));
            UiThemeDetail::SetFrame(mode_toggle_style.thumb_palette, Color(111, 127, 150), Color(111, 127, 150), Color(111, 127, 150), Color(94, 109, 131));
        }
        else {
            UiThemeDetail::SetFace(mode_toggle_style.palette, White(), White(), White(), White());
            UiThemeDetail::SetFrame(mode_toggle_style.palette, Color(222, 229, 237), Color(222, 229, 237), Color(222, 229, 237), Color(222, 229, 237));
            UiThemeDetail::SetInk(mode_toggle_style.palette, Color(17, 24, 39), Color(17, 24, 39), Color(17, 24, 39), Color(148, 163, 184));
            UiThemeDetail::SetFace(mode_toggle_style.track_palette, Color(230, 236, 242), Color(230, 236, 242), Color(17, 24, 39), Color(238, 242, 247));
            UiThemeDetail::SetFrame(mode_toggle_style.track_palette, Color(198, 208, 220), Color(198, 208, 220), Color(17, 24, 39), Color(214, 222, 231));
            UiThemeDetail::SetFace(mode_toggle_style.thumb_palette, White(), White(), White(), Color(245, 247, 250));
            UiThemeDetail::SetFrame(mode_toggle_style.thumb_palette, Color(198, 208, 220), Color(198, 208, 220), Color(17, 24, 39), Color(214, 222, 231));
        }
        mode_toggle_.SetStyle(mode_toggle_style).SetText("").SetShowFocus(false);

        minimal_.ApplyTheme(mode_);
        rounded_.ApplyTheme(mode_);
        Refresh();
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(20);
        title_.SetRect(m, DPI(18), r.GetWidth() - m * 2, DPI(42));
        copy_.SetRect(r.GetWidth() / 2 - DPI(420), DPI(66), DPI(840), DPI(40));
        mode_shell_.SetRect(r.GetWidth() - DPI(214), DPI(14), DPI(178), DPI(56));
        Rect ms = mode_shell_.GetSize();
        mode_label_.SetRect(DPI(16), 0, DPI(70), ms.GetHeight());
        mode_toggle_.SetRect(ms.GetWidth() - DPI(68), (ms.GetHeight() - DPI(27)) / 2, DPI(52), DPI(27));

        int top = DPI(126);
        int gap = DPI(26);
        int specimen_w = (r.GetWidth() - m * 2 - gap) / 2;
        int specimen_h = r.GetHeight() - top - DPI(26);
        minimal_.SetRect(m, top, specimen_w, specimen_h);
        rounded_.SetRect(m + specimen_w + gap, top, specimen_w, specimen_h);
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        if(mode_ == UiThemeMode::Dark)
            PaintVerticalGradient(w, r, Color(13, 20, 32), Color(19, 29, 43));
        else
            PaintVerticalGradient(w, r, Color(232, 238, 245), Color(212, 222, 232));
    }

private:
    UiThemeMode mode_ = UiThemeMode::Light;
    UiLabel title_;
    UiLabel copy_;
    UiPanel mode_shell_;
    UiLabel mode_label_;
    UiToggle mode_toggle_;
    ThemeSpecimen minimal_;
    ThemeSpecimen rounded_;
};

GUI_APP_MAIN
{
    UiThemeDemoWindow().Run();
}


























