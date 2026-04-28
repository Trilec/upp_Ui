#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

enum MenuDataset {
    MENU_SIMPLE = 0,
    MENU_RICH,
    MENU_STRESS,
};

struct MenuConfig {
    int dataset = MENU_RICH;
    int row_height = DPI(28);
    int bar_height = DPI(30);
    int icon_size = DPI(16);
    int check_size = DPI(14);
    int arrow_size = DPI(12);
    int left_padding = DPI(10);
    int right_padding = DPI(10);
    int content_gap = DPI(8);
    int item_spacing = 0;
    int right_gap = DPI(16);
    int popup_padding = DPI(6);
    int popup_min_width = DPI(180);
    int popup_max_height = DPI(320);
    int submenu_overlap = DPI(4);
    bool show_icons = true;
    bool show_checks = true;
    bool show_descriptions = false;
    bool show_shortcuts = true;
    bool show_separators = true;
    Color popup_bg = SColorPaper();
    Color bar_bg = SColorFace();
    Color separator_color = Blend(SColorShadow(), SColorPaper(), 210);
    Color item_ink = SColorText();
    Color disabled_ink = SColorDisabled();
    Color right_ink = Color(100, 116, 139);
    Color hot_bg = Color(239, 246, 255);
    Color hot_frame = Color(191, 219, 254);
    Color pressed_bg = Color(219, 234, 254);
    Color pressed_frame = Color(96, 165, 250);
    Color active_bar_bg = Color(232, 242, 255);
    Color check_color = Color(17, 24, 39);
    Color arrow_color = Color(100, 116, 139);
    Color shadow_color = Color(148, 163, 184);
};

String MenuDatasetName(int d)
{
    switch(d) {
    case MENU_SIMPLE: return "Simple";
    case MENU_STRESS: return "Stress";
    default: return "Rich";
    }
}

class UiMenuBuilder : public BuilderWindowBase {
public:
    typedef UiMenuBuilder CLASSNAME;

    UiMenuBuilder()
        : BuilderWindowBase("UiMenuDemo", "U++ UiMenu Builder", "Inspect menu-bar and popup behavior, row geometry, and color lanes from one shell.")
    {
        Preview().Add(menu_bar_);
        Preview().Add(open_popup_button_);
        open_popup_button_.SetText("Open Popup");
        open_popup_button_.WhenAction = [=] { popup_menu_.PopUp(&open_popup_button_, open_popup_button_.GetScreenRect().BottomLeft()); };

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_dataset_row_, state_dataset_label_, state_dataset_value_, "Dataset");
        AddStateRow(StateBox(), state_items_row_, state_items_label_, state_items_value_, "Items");
        AddStateRow(StateBox(), state_action_row_, state_action_label_, state_action_value_, "Last Action");

        AddDropdownRow(PropsBox(), dataset_row_box_, dataset_label_, dataset_drop_, "Dataset");
        AddSliderRow(PropsBox(), row_height_row_, "Row Height", "28px");
        AddSliderRow(PropsBox(), bar_height_row_, "Bar Height", "30px");
        AddSliderRow(PropsBox(), icon_size_row_, "Icon Size", "16px");
        AddSliderRow(PropsBox(), check_size_row_, "Check Size", "14px");
        AddSliderRow(PropsBox(), arrow_size_row_, "Arrow Size", "12px");
        AddSliderRow(PropsBox(), left_padding_row_, "Left Pad", "10px");
        AddSliderRow(PropsBox(), right_padding_row_, "Right Pad", "10px");
        AddSliderRow(PropsBox(), content_gap_row_, "Content Gap", "8px");
        AddSliderRow(PropsBox(), item_spacing_row_, "Item Spacing", "0px");
        AddSliderRow(PropsBox(), right_gap_row_, "Right Gap", "16px");
        AddSliderRow(PropsBox(), popup_padding_row_, "Popup Pad", "6px");
        AddSliderRow(PropsBox(), popup_min_width_row_, "Popup Min", "180px");
        AddSliderRow(PropsBox(), popup_max_height_row_, "Popup Max", "320px");
        AddSliderRow(PropsBox(), submenu_overlap_row_, "Submenu Ov", "4px");
        AddToggleRow(PropsBox(), show_icons_row_, "Show Icons");
        AddToggleRow(PropsBox(), show_checks_row_, "Show Checks");
        AddToggleRow(PropsBox(), show_descriptions_row_, "Descriptions");
        AddToggleRow(PropsBox(), show_shortcuts_row_, "Shortcuts");
        AddToggleRow(PropsBox(), show_separators_row_, "Separators");
        AddColorRow(PropsBox(), popup_bg_row_, "Popup Bg");
        AddColorRow(PropsBox(), bar_bg_row_, "Bar Bg");
        AddColorRow(PropsBox(), separator_row_, "Separator");
        AddColorRow(PropsBox(), item_ink_row_, "Text");
        AddColorRow(PropsBox(), disabled_ink_row_, "Disabled");
        AddColorRow(PropsBox(), right_ink_row_, "Right Text");
        AddColorRow(PropsBox(), hot_bg_row_, "Hot Bg");
        AddColorRow(PropsBox(), hot_frame_row_, "Hot Frame");
        AddColorRow(PropsBox(), pressed_bg_row_, "Pressed Bg");
        AddColorRow(PropsBox(), pressed_frame_row_, "Pressed Frame");
        AddColorRow(PropsBox(), active_bar_bg_row_, "Active Bar");
        AddColorRow(PropsBox(), check_color_row_, "Check");
        AddColorRow(PropsBox(), arrow_color_row_, "Arrow");
        AddColorRow(PropsBox(), shadow_color_row_, "Shadow");

        const EnumOption sets[] = { { "Simple", MENU_SIMPLE }, { "Rich", MENU_RICH }, { "Stress", MENU_STRESS } };
        PopulateDropdown(dataset_drop_, sets, 3);

        InitSlider(row_height_row_, cfg_.row_height, DPI(22), DPI(40));
        InitSlider(bar_height_row_, cfg_.bar_height, DPI(24), DPI(40));
        InitSlider(icon_size_row_, cfg_.icon_size, DPI(12), DPI(24));
        InitSlider(check_size_row_, cfg_.check_size, DPI(10), DPI(22));
        InitSlider(arrow_size_row_, cfg_.arrow_size, DPI(8), DPI(20));
        InitSlider(left_padding_row_, cfg_.left_padding, 0, DPI(24));
        InitSlider(right_padding_row_, cfg_.right_padding, 0, DPI(24));
        InitSlider(content_gap_row_, cfg_.content_gap, 0, DPI(20));
        InitSlider(item_spacing_row_, cfg_.item_spacing, 0, DPI(12));
        InitSlider(right_gap_row_, cfg_.right_gap, 0, DPI(24));
        InitSlider(popup_padding_row_, cfg_.popup_padding, 0, DPI(12));
        InitSlider(popup_min_width_row_, cfg_.popup_min_width, DPI(120), DPI(320));
        InitSlider(popup_max_height_row_, cfg_.popup_max_height, DPI(160), DPI(520));
        InitSlider(submenu_overlap_row_, cfg_.submenu_overlap, 0, DPI(12));

        InitColorRow(popup_bg_row_, cfg_.popup_bg); InitColorRow(bar_bg_row_, cfg_.bar_bg); InitColorRow(separator_row_, cfg_.separator_color);
        InitColorRow(item_ink_row_, cfg_.item_ink); InitColorRow(disabled_ink_row_, cfg_.disabled_ink); InitColorRow(right_ink_row_, cfg_.right_ink);
        InitColorRow(hot_bg_row_, cfg_.hot_bg); InitColorRow(hot_frame_row_, cfg_.hot_frame); InitColorRow(pressed_bg_row_, cfg_.pressed_bg);
        InitColorRow(pressed_frame_row_, cfg_.pressed_frame); InitColorRow(active_bar_bg_row_, cfg_.active_bar_bg); InitColorRow(check_color_row_, cfg_.check_color);
        InitColorRow(arrow_color_row_, cfg_.arrow_color); InitColorRow(shadow_color_row_, cfg_.shadow_color);

        dataset_drop_.WhenSelect = [=](int) { cfg_.dataset = (int)dataset_drop_.GetSelectedData(); RefreshFromConfig(); };
        WireSlider(row_height_row_, cfg_.row_height); WireSlider(bar_height_row_, cfg_.bar_height); WireSlider(icon_size_row_, cfg_.icon_size); WireSlider(check_size_row_, cfg_.check_size);
        WireSlider(arrow_size_row_, cfg_.arrow_size); WireSlider(left_padding_row_, cfg_.left_padding); WireSlider(right_padding_row_, cfg_.right_padding); WireSlider(content_gap_row_, cfg_.content_gap);
        WireSlider(item_spacing_row_, cfg_.item_spacing); WireSlider(right_gap_row_, cfg_.right_gap); WireSlider(popup_padding_row_, cfg_.popup_padding); WireSlider(popup_min_width_row_, cfg_.popup_min_width);
        WireSlider(popup_max_height_row_, cfg_.popup_max_height); WireSlider(submenu_overlap_row_, cfg_.submenu_overlap);
        WireToggle(show_icons_row_, cfg_.show_icons); WireToggle(show_checks_row_, cfg_.show_checks); WireToggle(show_descriptions_row_, cfg_.show_descriptions);
        WireToggle(show_shortcuts_row_, cfg_.show_shortcuts); WireToggle(show_separators_row_, cfg_.show_separators);
        WireColor(popup_bg_row_, cfg_.popup_bg); WireColor(bar_bg_row_, cfg_.bar_bg); WireColor(separator_row_, cfg_.separator_color);
        WireColor(item_ink_row_, cfg_.item_ink); WireColor(disabled_ink_row_, cfg_.disabled_ink); WireColor(right_ink_row_, cfg_.right_ink);
        WireColor(hot_bg_row_, cfg_.hot_bg); WireColor(hot_frame_row_, cfg_.hot_frame); WireColor(pressed_bg_row_, cfg_.pressed_bg); WireColor(pressed_frame_row_, cfg_.pressed_frame);
        WireColor(active_bar_bg_row_, cfg_.active_bar_bg); WireColor(check_color_row_, cfg_.check_color); WireColor(arrow_color_row_, cfg_.arrow_color); WireColor(shadow_color_row_, cfg_.shadow_color);

        menu_bar_.WhenAction = [=](UiMenuNodeRef, const UiMenuItem& item) { last_action_ = item.text; SyncState(); };
        popup_menu_.WhenAction = [=](UiMenuNodeRef, const UiMenuItem& item) { last_action_ = item.text; SyncState(); };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        UiLabel::Style body = MakeBodyLabelStyle(Palette());
        UiLabel::Style value = MakeValueLabelStyle(Palette());
        UiDropdown::Style dd = MakeDropdownStyle(Palette());
        state_theme_label_.SetStyle(body); state_theme_value_.SetStyle(value);
        state_dataset_label_.SetStyle(body); state_dataset_value_.SetStyle(value);
        state_items_label_.SetStyle(body); state_items_value_.SetStyle(value);
        state_action_label_.SetStyle(body); state_action_value_.SetStyle(value);
        dataset_label_.SetStyle(body); dataset_drop_.SetStyle(dd);
        ApplySliderStyle(body, value); ApplyToggleStyle(body); ApplyColorStyle(body);
        open_popup_button_.SetStyle(MakeSmallButtonStyle(Palette()));
    }

    virtual void LayoutPreviewContent() override
    {
        Rect c = Preview().GetCanvasRect();
        menu_bar_.SetRect(c.left + DPI(24), c.top + DPI(24), max(DPI(320), c.GetWidth() - DPI(48)), cfg_.bar_height + DPI(8));
        open_popup_button_.SetRect(c.left + DPI(24), c.top + DPI(74), DPI(132), DPI(32));
    }

private:
    struct EnumOption { const char* label; int value; };
    void AddColorRow(UiBoxLayout& t, UiCompositeColor& r, const char* n) { r.SetLabel(n).SetSwatchCount(1).ShowValue(false); t.Add(r).Fit(); }
    void PopulateDropdown(UiDropdown& d, const EnumOption* o, int n){ d.UseInternalModel(); d.Clear(); for(int i=0;i<n;i++) d.Add(o[i].label,o[i].value);}    
    void InitColorRow(UiCompositeColor& r, Color c){ r.SetSwatchColor(0,c); }
    void InitSlider(UiCompositeSlider& r, int value, int lo, int hi){ r.Slider().SetRange(lo,hi).SetStep(1).SetValue(value); }
    void ApplySliderStyle(const UiLabel::Style& body, const UiLabel::Style& value){ Vector<UiCompositeSlider*> rows = { &row_height_row_, &bar_height_row_, &icon_size_row_, &check_size_row_, &arrow_size_row_, &left_padding_row_, &right_padding_row_, &content_gap_row_, &item_spacing_row_, &right_gap_row_, &popup_padding_row_, &popup_min_width_row_, &popup_max_height_row_, &submenu_overlap_row_ }; for(auto* r : rows) r->SetLabelStyle(body).SetValueStyle(value); }
    void ApplyToggleStyle(const UiLabel::Style& body){ Vector<UiCompositeToggle*> rows = { &show_icons_row_, &show_checks_row_, &show_descriptions_row_, &show_shortcuts_row_, &show_separators_row_ }; for(auto* r : rows) r->SetLabelStyle(body); }
    void ApplyColorStyle(const UiLabel::Style& body){ Vector<UiCompositeColor*> rows = { &popup_bg_row_, &bar_bg_row_, &separator_row_, &item_ink_row_, &disabled_ink_row_, &right_ink_row_, &hot_bg_row_, &hot_frame_row_, &pressed_bg_row_, &pressed_frame_row_, &active_bar_bg_row_, &check_color_row_, &arrow_color_row_, &shadow_color_row_ }; for(auto* r : rows) r->SetLabelStyle(body); }
    void WireSlider(UiCompositeSlider& r, int& field){ r.WhenAction = [this, &r, &field] { field = (int)r.Slider().GetValue(); RefreshFromConfig(); }; }
    void WireToggle(UiCompositeToggle& r, bool& field){ r.Toggle().WhenAction = [this, &r, &field] { field = r.Toggle().IsOn(); RefreshFromConfig(); }; }
    void WireColor(UiCompositeColor& r, Color& field){ r.WhenAction = [this, &r, &field] { field = r.GetSwatchColor(0); RefreshFromConfig(); }; }

    UiMenu::Style BuildStyle() const
    {
        UiMenu::Style s = UiMenu::StyleDefault();
        s.row_height = cfg_.row_height; s.bar_height = cfg_.bar_height; s.icon_size = cfg_.icon_size; s.check_size = cfg_.check_size; s.arrow_size = cfg_.arrow_size;
        s.left_padding = cfg_.left_padding; s.right_padding = cfg_.right_padding; s.content_gap = cfg_.content_gap; s.item_spacing = cfg_.item_spacing; s.right_gap = cfg_.right_gap;
        s.popup_padding = cfg_.popup_padding; s.popup_min_width = cfg_.popup_min_width; s.popup_max_height = cfg_.popup_max_height; s.submenu_overlap = cfg_.submenu_overlap;
        s.show_icons = cfg_.show_icons; s.show_checks = cfg_.show_checks; s.show_descriptions = cfg_.show_descriptions; s.show_shortcuts = cfg_.show_shortcuts; s.show_separators = cfg_.show_separators;
        s.popup_bg = cfg_.popup_bg; s.bar_bg = cfg_.bar_bg; s.separator_color = cfg_.separator_color; s.item_ink = cfg_.item_ink; s.disabled_ink = cfg_.disabled_ink; s.right_ink = cfg_.right_ink;
        s.hot_bg = cfg_.hot_bg; s.hot_frame = cfg_.hot_frame; s.pressed_bg = cfg_.pressed_bg; s.pressed_frame = cfg_.pressed_frame; s.active_bar_bg = cfg_.active_bar_bg; s.check_color = cfg_.check_color; s.arrow_color = cfg_.arrow_color; s.shadow_color = cfg_.shadow_color;
        return s;
    }

    void BuildModels()
    {
        bar_model_.Clear(); popup_model_.Clear();
        if(cfg_.dataset == MENU_SIMPLE) {
            UiMenuNodeRef file = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("File"));
            UiMenuNodeRef view = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("View"));
            bar_model_.AddChild(file, MakeAction("Open", "Ctrl+O", 101));
            bar_model_.AddChild(file, MakeAction("Exit", "Alt+F4", 102));
            bar_model_.AddChild(view, MakeCheck("Status Bar", true, 201));
            popup_model_.AddChild(popup_model_.Root(), MakeAction("Inspect", String(), 301));
            popup_model_.AddChild(popup_model_.Root(), MakeAction("Rename", "F2", 302));
        }
        else if(cfg_.dataset == MENU_STRESS) {
            UiMenuNodeRef root = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("Stress"));
            for(int i = 0; i < 40; i++)
                bar_model_.AddChild(root, MakeAction(Format("Entry %02d", i), String(), 1000 + i));
            for(int i = 0; i < 120; i++) {
                UiMenuItem item(Format("Stress item %03d", i), i);
                item.shortcut_text = Format("Alt+%d", i % 10);
                item.checkable = (i % 7) == 0;
                item.checked = (i % 21) == 0;
                popup_model_.AddChild(popup_model_.Root(), item);
            }
        }
        else {
            UiMenuNodeRef file = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("File"));
            UiMenuNodeRef edit = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("Edit"));
            UiMenuNodeRef view = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("View"));
            bar_model_.AddChild(file, MakeAction("New Project", "Ctrl+N", 101, ICON_DESIGN_FOLDER_48()));
            bar_model_.AddChild(file, MakeAction("Open", "Ctrl+O", 102, ICON_DESIGN_FOLDER_48()));
            bar_model_.AddChild(file, MakeSeparator());
            bar_model_.AddChild(file, MakeAction("Exit", "Alt+F4", 103));
            bar_model_.AddChild(edit, MakeAction("Undo", "Ctrl+Z", 201));
            bar_model_.AddChild(edit, MakeAction("Redo", "Ctrl+Shift+Z", 202));
            UiMenuNodeRef theme = bar_model_.AddChild(view, UiMenuItem("Theme"));
            bar_model_.AddChild(theme, MakeRadio("Minimal", true, 301));
            bar_model_.AddChild(theme, MakeRadio("Rounded", false, 302));
            popup_model_.AddChild(popup_model_.Root(), MakeAction("Inspect", "F1", 501, ICON_DESIGN_SETTINGS_48()));
            popup_model_.AddChild(popup_model_.Root(), MakeAction("Rename", "F2", 502));
            popup_model_.AddChild(popup_model_.Root(), MakeSeparator());
            UiMenuNodeRef state = popup_model_.AddChild(popup_model_.Root(), UiMenuItem("State"));
            popup_model_.AddChild(state, MakeCheck("Enabled", true, 511));
            popup_model_.AddChild(state, MakeCheck("Visible", true, 512));
            popup_model_.AddChild(state, MakeCheck("Pinned", false, 513));
        }
    }

    UiMenuItem MakeAction(const String& text, const String& shortcut, int cmd, const Image& icon = Image())
    {
        UiMenuItem item(text, cmd); item.command_id = cmd; item.shortcut_text = shortcut; item.icon = icon; item.icon_render_mode = !IsNull(icon) ? UiIconRenderMode::MonoTint : UiIconRenderMode::PreserveColor; return item;
    }
    UiMenuItem MakeCheck(const String& text, bool checked, int cmd) { UiMenuItem item = MakeAction(text, String(), cmd); item.checkable = true; item.checked = checked; return item; }
    UiMenuItem MakeRadio(const String& text, bool checked, int cmd) { UiMenuItem item = MakeAction(text, String(), cmd); item.radio = true; item.checkable = true; item.checked = checked; return item; }
    UiMenuItem MakeSeparator() { UiMenuItem item; item.separator = true; item.enabled = false; return item; }

    void RefreshFromConfig()
    {
        dataset_drop_.SelectByData(cfg_.dataset);
        SyncRows();
        BuildModels();
        menu_bar_.SetStyle(BuildStyle()).SetMenuBarMode(true).SetModel(bar_model_);
        popup_menu_.SetStyle(BuildStyle()).SetModel(popup_model_);
        SyncState(); SyncCode(); LayoutPreviewContent(); Preview().Refresh();
    }

    void SyncRows()
    {
        row_height_row_.Slider().SetValue(cfg_.row_height); bar_height_row_.Slider().SetValue(cfg_.bar_height); icon_size_row_.Slider().SetValue(cfg_.icon_size); check_size_row_.Slider().SetValue(cfg_.check_size); arrow_size_row_.Slider().SetValue(cfg_.arrow_size);
        left_padding_row_.Slider().SetValue(cfg_.left_padding); right_padding_row_.Slider().SetValue(cfg_.right_padding); content_gap_row_.Slider().SetValue(cfg_.content_gap); item_spacing_row_.Slider().SetValue(cfg_.item_spacing); right_gap_row_.Slider().SetValue(cfg_.right_gap);
        popup_padding_row_.Slider().SetValue(cfg_.popup_padding); popup_min_width_row_.Slider().SetValue(cfg_.popup_min_width); popup_max_height_row_.Slider().SetValue(cfg_.popup_max_height); submenu_overlap_row_.Slider().SetValue(cfg_.submenu_overlap);
        show_icons_row_.Toggle().SetOn(cfg_.show_icons); show_checks_row_.Toggle().SetOn(cfg_.show_checks); show_descriptions_row_.Toggle().SetOn(cfg_.show_descriptions); show_shortcuts_row_.Toggle().SetOn(cfg_.show_shortcuts); show_separators_row_.Toggle().SetOn(cfg_.show_separators);
        popup_bg_row_.SetSwatchColor(0, cfg_.popup_bg); bar_bg_row_.SetSwatchColor(0, cfg_.bar_bg); separator_row_.SetSwatchColor(0, cfg_.separator_color); item_ink_row_.SetSwatchColor(0, cfg_.item_ink); disabled_ink_row_.SetSwatchColor(0, cfg_.disabled_ink); right_ink_row_.SetSwatchColor(0, cfg_.right_ink);
        hot_bg_row_.SetSwatchColor(0, cfg_.hot_bg); hot_frame_row_.SetSwatchColor(0, cfg_.hot_frame); pressed_bg_row_.SetSwatchColor(0, cfg_.pressed_bg); pressed_frame_row_.SetSwatchColor(0, cfg_.pressed_frame); active_bar_bg_row_.SetSwatchColor(0, cfg_.active_bar_bg); check_color_row_.SetSwatchColor(0, cfg_.check_color); arrow_color_row_.SetSwatchColor(0, cfg_.arrow_color); shadow_color_row_.SetSwatchColor(0, cfg_.shadow_color);
    }

    void SyncState()
    {
        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_dataset_value_.SetText(MenuDatasetName(cfg_.dataset));
        state_items_value_.SetText(AsString(popup_model_.GetChildCount(popup_model_.Root())) + " popup / " + AsString(bar_model_.GetChildCount(bar_model_.Root())) + " top");
        state_action_value_.SetText(last_action_.IsEmpty() ? "None" : last_action_);
    }

    void SyncCode()
    {
        String code;
        code << "UiMenu menu;\n";
        code << "menu.SetStyle(UiMenu::StyleDefault());\n";
        code << "menu.SetMenuBarMode(true).SetModel(model);\n";
        code << "// row_height=" << cfg_.row_height << ", bar_height=" << cfg_.bar_height << ", content_gap=" << cfg_.content_gap << ", item_spacing=" << cfg_.item_spacing << "\n";
        SetUsageCode(code);
    }

    MenuConfig cfg_;
    String last_action_;
    UiMenu menu_bar_, popup_menu_;
    UiMenuModel bar_model_, popup_model_;
    UiButton open_popup_button_;

    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_dataset_row_ { UiBoxLayout::Direction::H }, state_items_row_ { UiBoxLayout::Direction::H }, state_action_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_dataset_label_, state_dataset_value_, state_items_label_, state_items_value_, state_action_label_, state_action_value_;
    UiBoxLayout dataset_row_box_ { UiBoxLayout::Direction::H };
    UiLabel dataset_label_; UiDropdown dataset_drop_;
    UiCompositeSlider row_height_row_, bar_height_row_, icon_size_row_, check_size_row_, arrow_size_row_, left_padding_row_, right_padding_row_, content_gap_row_, item_spacing_row_, right_gap_row_, popup_padding_row_, popup_min_width_row_, popup_max_height_row_, submenu_overlap_row_;
    UiCompositeToggle show_icons_row_, show_checks_row_, show_descriptions_row_, show_shortcuts_row_, show_separators_row_;
    UiCompositeColor popup_bg_row_, bar_bg_row_, separator_row_, item_ink_row_, disabled_ink_row_, right_ink_row_, hot_bg_row_, hot_frame_row_, pressed_bg_row_, pressed_frame_row_, active_bar_bg_row_, check_color_row_, arrow_color_row_, shadow_color_row_;
};

}

GUI_APP_MAIN
{
    UiMenuBuilder().Run();
}

