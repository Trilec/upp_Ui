#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

enum BoxPreset {
    BOX_PRESET_STACK = 0,
    BOX_PRESET_TOOLBAR,
    BOX_PRESET_MIXED,
    BOX_PRESET_WRAP,
    BOX_PRESET_STRIP,
};

enum BoxItemMode {
    BOXITEM_FIT = 0,
    BOXITEM_FIXED,
    BOXITEM_EXPAND,
    BOXITEM_BREAK,
    BOXITEM_SPACER,
};

struct BoxItemSpec : Moveable<BoxItemSpec> {
    String text;
    int mode = BOXITEM_FIT;
    int fixed_main = DPI(120);
    int weight = 1;
    int min_cross = DPI(40);
    bool visible = true;
    bool enabled = true;
    UiCrossAlign align = UiCrossAlign::Auto;
    Color face = Color(236, 241, 248);
    Color frame = Color(211, 221, 237);
    Color ink = Color(28, 47, 78);
};

struct BoxConfig {
    int preset = BOX_PRESET_MIXED;
    int direction = (int)UiDirection::V;
    int item_spacing = DPI(8);
    int content_margin = DPI(10);
    int fixed_column = DPI(132);
    int fixed_row = DPI(56);
    int align_items = (int)UiCrossAlign::Stretch;
    bool wrap = false;
    bool debug = false;
    bool wrap_auto_resize = true;
    bool wrap_rows_expand = false;
};

String BoxPresetName(int p)
{
    switch(p) {
    case BOX_PRESET_STACK: return "Vertical Stack";
    case BOX_PRESET_TOOLBAR: return "Toolbar";
    case BOX_PRESET_WRAP: return "Wrapped Chips";
    case BOX_PRESET_STRIP: return "Card Strip";
    default: return "Mixed Sizes";
    }
}

String BoxModeName(int m)
{
    switch(m) {
    case BOXITEM_FIXED: return "Fixed";
    case BOXITEM_EXPAND: return "Expand";
    case BOXITEM_BREAK: return "Break";
    case BOXITEM_SPACER: return "Spacer";
    default: return "Fit";
    }
}

String DirectionName(UiDirection d)
{
    return d == UiDirection::V ? "Vertical" : "Horizontal";
}
String CrossAlignName(UiCrossAlign a)
{
    switch(a) {
    case UiCrossAlign::Stretch: return "Stretch";
    case UiCrossAlign::Start: return "Start";
    case UiCrossAlign::End: return "End";
    case UiCrossAlign::Center: return "Center";
    default: return "Auto";
    }
}

class BoxItemCard : public UiPanel {
public:
    typedef BoxItemCard CLASSNAME;

    BoxItemCard()
    {
        Add(label_);
        label_.NoWantFocus();
        label_.IgnoreMouse();
    }

    void SetIndex(int i) { index_ = i; }
    void SetPicked(bool b) { picked_ = b; label_.Refresh(); Refresh(); }

    void Configure(const BoxItemSpec& spec)
    {
        spec_ = spec;
        UiPanel::Style panel = UiTheme::ResolvePanel(UiPanelRole::Surface);
        Color face = spec.face;
        Color frame = spec.frame;
        Color ink = spec.ink;
        if(spec.mode == BOXITEM_BREAK) {
            face = Color(246, 248, 252);
            frame = Color(200, 208, 220);
            ink = Color(104, 118, 143);
        }
        else if(spec.mode == BOXITEM_SPACER) {
            face = Color(243, 246, 250);
            frame = Color(206, 214, 226);
            ink = Color(112, 124, 148);
        }
        for(int i = 0; i < 4; i++) {
            panel.palette.face[i] = UiFill::Solid(face);
            panel.palette.frame[i] = frame;
            panel.palette.ink[i] = ink;
        }
        panel.metrics.face_enabled = true;
        panel.metrics.frame_enabled = true;
        panel.metrics.frame_width = 1;
        panel.metrics.radius = DPI(8);
        panel.metrics.focus_enabled = false;
        panel.metrics.content_margin = Rect(DPI(8), DPI(6), DPI(8), DPI(6));
        SetStyle(panel);
        ink_ = ink;
        UiLabel::Style ls = UiTheme::ResolveLabel(UiLabelRole::Body);
        for(int i = 0; i < 4; i++) {
            ls.palette.face[i] = UiFill::None();
            ls.palette.frame[i] = Null;
            ls.palette.ink[i] = ink;
        }
        ls.transparent = true;
        ls.font = DemoSans(10, true);
        ls.align_h = UiAlign::CENTER;
        ls.align_v = UiAlign::CENTER;
        label_.SetStyle(ls);
        label_.SetText(spec.text);
        Enable(spec.enabled);
        Show(spec.visible);
        label_.Refresh();
        Refresh();
    }

    virtual void Layout() override
    {
        Rect rc = UiStyledInnerRect(GetSize(), GetStyle().metrics, GetStyle().skin);
        label_.SetRect(rc);
    }

    virtual void LeftDown(Point, dword) override
    {
        if(WhenPick)
            WhenPick(index_);
    }

    virtual void MouseEnter(Point, dword) override
    {
        hot_ = true;
        label_.Refresh();
        Refresh();
    }

    virtual void MouseLeave() override
    {
        hot_ = false;
        label_.Refresh();
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        UiPanel::Paint(w);
        Rect r = Rect(GetSize()).Deflated(2);
        if(picked_)
            w.DrawRect(r.left + 1, r.top + 1, max(0, r.GetWidth() - 2), max(0, r.GetHeight() - 2), Color(232, 240, 252));
        else if(hot_)
            w.DrawRect(r.left + 1, r.top + 1, max(0, r.GetWidth() - 2), max(0, r.GetHeight() - 2), Color(244, 247, 251));
        DrawDashedRect(w, r, picked_ ? Color(44, 99, 212) : hot_ ? Color(152, 163, 182) : Color(176, 186, 202), DPI(5), DPI(3));
    }

    Callback1<int> WhenPick;

private:
    int index_ = -1;
    bool picked_ = false;
    bool hot_ = false;
    BoxItemSpec spec_;
    Color ink_ = Color(28, 47, 78);
    UiLabel label_;
};

class BoxLayoutHost : public ParentCtrl {
public:
    typedef BoxLayoutHost CLASSNAME;

    UiBoxLayout& Reset(UiDirection d)
    {
        if(layout_)
            layout_->Remove();
        layout_.Create<UiBoxLayout>(d);
        Add(*layout_);
        Layout();
        return *layout_;
    }

    virtual void Layout() override
    {
        if(layout_)
            layout_->SetRect(GetSize());
    }

private:
    One<UiBoxLayout> layout_;
};

class DemoModelTree : public UiTree {
public:
    typedef DemoModelTree CLASSNAME;

    Event<> WhenStructureChanged;

    virtual void LeftDown(Point p, dword flags) override
    {
        Size before = GetContentSize();
        UiTree::LeftDown(p, flags);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
    }

    virtual void LeftDouble(Point p, dword flags) override
    {
        Size before = GetContentSize();
        UiTree::LeftDouble(p, flags);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
    }

    virtual bool Key(dword key, int count) override
    {
        Size before = GetContentSize();
        bool out = UiTree::Key(key, count);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
        return out;
    }
};

class UiBoxLayoutBuilder : public BuilderWindowBase {
public:
    typedef UiBoxLayoutBuilder CLASSNAME;

    UiBoxLayoutBuilder()
        : BuilderWindowBase("UiBoxLayoutDemo", "U++ UiBoxLayout Builder", "Inspect direction, sizing modes, wrapping, spacing, and structure changes from one live workspace.")
    {
        Preview().Add(host_);

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_preset_row_, state_preset_label_, state_preset_value_, "Preset");
        AddStateRow(StateBox(), state_direction_row_, state_direction_label_, state_direction_value_, "Direction");
        AddStateRow(StateBox(), state_items_row_, state_items_label_, state_items_value_, "Items");
        AddStateRow(StateBox(), state_pick_row_, state_pick_label_, state_pick_value_, "Selection");
        StateBox().Add(model_acc_).Fit();
        model_section_ = model_acc_.AddSection("MODEL DATA", true);
        model_acc_.GetSectionContent(model_section_).Add(model_scroll_.SizePos());
        model_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
        model_scroll_.Content().Add(model_tree_);
        model_tree_.SetRootVisible(false);
        model_tree_.SetSelectionMode(UITREESEL_SINGLE);
        model_tree_.SetModel(tree_model_);
        model_tree_.WhenStructureChanged = [=] { UpdateModelViewport(); };

        PropsBox().Add(data_head_).Fit();
        PropsBox().Add(data_box_).Fit();
        PropsBox().Add(layout_head_).Fit();
        PropsBox().Add(layout_box_).Fit();
        PropsBox().Add(item_head_).Fit();
        PropsBox().Add(item_box_).Fit();
        PropsBox().Add(structure_head_).Fit();
        PropsBox().Add(structure_box_).Fit();

        data_head_.SetText("DATA").NoWantFocus();
        layout_head_.SetText("LAYOUT").NoWantFocus();
        item_head_.SetText("ITEM").NoWantFocus();
        structure_head_.SetText("STRUCTURE").NoWantFocus();

        

        data_box_.SetGap(DPI(5)).SetInset(0);
        layout_box_.SetGap(DPI(5)).SetInset(0);
        item_box_.SetGap(DPI(5)).SetInset(0);
        structure_box_.SetGap(DPI(5)).SetInset(0);

        AddDropdownRow(data_box_, preset_row_box_, preset_label_, preset_drop_, "Preset");
        AddDropdownRow(data_box_, dir_row_box_, dir_label_, dir_drop_, "Direction");
        AddDropdownRow(data_box_, align_row_box_, align_label_, align_drop_, "Align");

        AddSliderRow(layout_box_, item_spacing_row_, "Item Spacing", "8px");
        AddSliderRow(layout_box_, content_margin_row_, "Content Margin", "10px");
        AddSliderRow(layout_box_, fixed_col_row_, "Fixed Col", "132px");
        AddSliderRow(layout_box_, fixed_row_row_, "Fixed Row", "56px");
        AddToggleRow(layout_box_, wrap_row_, "Wrap");
        AddToggleRow(layout_box_, debug_row_, "Debug");
        AddToggleRow(layout_box_, wrap_auto_row_, "Wrap Auto");
        AddToggleRow(layout_box_, wrap_expand_row_, "Rows Expand");

        AddEditRow(item_box_, item_text_row_box_, item_text_label_, item_text_edit_, "Item Text");
        AddDropdownRow(item_box_, item_mode_row_box_, item_mode_label_, item_mode_drop_, "Mode");
        AddDropdownRow(item_box_, item_align_row_box_, item_align_label_, item_align_drop_, "Align Self");
        AddSliderRow(item_box_, fixed_main_row_, "Fixed Main", "120px");
        AddSliderRow(item_box_, weight_row_, "Weight", "1");
        AddSliderRow(item_box_, min_cross_row_, "Min Cross", "40px");
        AddToggleRow(item_box_, item_visible_row_, "Visible");
        AddToggleRow(item_box_, item_enabled_row_, "Enabled");
        AddColorRow(item_box_, item_face_row_, "Face");
        AddColorRow(item_box_, item_frame_row_, "Frame");
        AddColorRow(item_box_, item_text_color_row_, "Text");

        action_row_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        action_row_.Add(add_button_).Expand(1).MinHeight(DPI(28));
        action_row_.Add(remove_button_).Expand(1).MinHeight(DPI(28));
        action_row_.Add(move_prev_button_).Expand(1).MinHeight(DPI(28));
        structure_box_.Add(action_row_).Fit();
        move_next_row_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        move_next_row_.Add(move_next_button_).Expand(1).MinHeight(DPI(28));
        move_next_row_.Add(add_break_button_).Expand(1).MinHeight(DPI(28));
        move_next_row_.Add(add_spacer_button_).Expand(1).MinHeight(DPI(28));
        structure_box_.Add(move_next_row_).Fit();

        const EnumOption presets[] = {
            { "Vertical Stack", BOX_PRESET_STACK },
            { "Toolbar", BOX_PRESET_TOOLBAR },
            { "Mixed Sizes", BOX_PRESET_MIXED },
            { "Wrapped Chips", BOX_PRESET_WRAP },
            { "Card Strip", BOX_PRESET_STRIP },
        };
        const EnumOption dirs[] = {
            { "Horizontal", (int)UiDirection::H },
            { "Vertical", (int)UiDirection::V },
        };
        const EnumOption aligns[] = {
            { "Stretch", (int)UiCrossAlign::Stretch },
            { "Start", (int)UiCrossAlign::Start },
            { "Center", (int)UiCrossAlign::Center },
            { "End", (int)UiCrossAlign::End },
        };
        const EnumOption modes[] = {
            { "Fit", BOXITEM_FIT },
            { "Fixed", BOXITEM_FIXED },
            { "Expand", BOXITEM_EXPAND },
            { "Break", BOXITEM_BREAK },
            { "Spacer", BOXITEM_SPACER },
        };
        const EnumOption self_aligns[] = {
            { "Auto", (int)UiCrossAlign::Auto },
            { "Stretch", (int)UiCrossAlign::Stretch },
            { "Start", (int)UiCrossAlign::Start },
            { "Center", (int)UiCrossAlign::Center },
            { "End", (int)UiCrossAlign::End },
        };
        PopulateDropdown(preset_drop_, presets, 5);
        PopulateDropdown(dir_drop_, dirs, 2);
        PopulateDropdown(align_drop_, aligns, 4);
        PopulateDropdown(item_mode_drop_, modes, 5);
        PopulateDropdown(item_align_drop_, self_aligns, 5);

        InitSlider(item_spacing_row_, cfg_.item_spacing, 0, DPI(20));
        InitSlider(content_margin_row_, cfg_.content_margin, 0, DPI(24));
        InitSlider(fixed_col_row_, cfg_.fixed_column, DPI(60), DPI(220));
        InitSlider(fixed_row_row_, cfg_.fixed_row, DPI(34), DPI(120));
        InitSlider(fixed_main_row_, DPI(120), DPI(40), DPI(220));
        InitSlider(weight_row_, 1, 1, 8);
        InitSlider(min_cross_row_, DPI(40), DPI(20), DPI(120));

        add_button_.SetText("Add After").SetStyle(MakeSmallButtonStyle(Palette()));
        remove_button_.SetText("Delete").SetStyle(MakeSmallButtonStyle(Palette()));
        move_prev_button_.SetText("Move Prev").SetStyle(MakeSmallButtonStyle(Palette()));
        move_next_button_.SetText("Move Next").SetStyle(MakeSmallButtonStyle(Palette()));
        add_break_button_.SetText("Break After").SetStyle(MakeSmallButtonStyle(Palette()));
        add_spacer_button_.SetText("Spacer After").SetStyle(MakeSmallButtonStyle(Palette()));

        preset_drop_.WhenSelect = [=](int) { cfg_.preset = (int)preset_drop_.GetSelectedData(); ApplyPreset(); RefreshFromConfig(); };
        dir_drop_.WhenSelect = [=](int) { cfg_.direction = (int)dir_drop_.GetSelectedData(); RefreshFromConfig(); };
        align_drop_.WhenSelect = [=](int) { cfg_.align_items = (int)align_drop_.GetSelectedData(); RefreshFromConfig(); };
        WireSlider(item_spacing_row_, cfg_.item_spacing);
        WireSlider(content_margin_row_, cfg_.content_margin);
        WireSlider(fixed_col_row_, cfg_.fixed_column);
        WireSlider(fixed_row_row_, cfg_.fixed_row);
        WireToggle(wrap_row_, cfg_.wrap);
        WireToggle(debug_row_, cfg_.debug);
        WireToggle(wrap_auto_row_, cfg_.wrap_auto_resize);
        WireToggle(wrap_expand_row_, cfg_.wrap_rows_expand);

        item_text_edit_.WhenAction = [=] { SaveSelectedItem(); };
        item_mode_drop_.WhenSelect = [=](int) { SaveSelectedItem(); };
        item_align_drop_.WhenSelect = [=](int) { SaveSelectedItem(); };
        fixed_main_row_.WhenAction = [=] { SaveSelectedItem(); };
        weight_row_.WhenAction = [=] { SaveSelectedItem(); };
        min_cross_row_.WhenAction = [=] { SaveSelectedItem(); };
        item_visible_row_.Toggle().WhenAction = [=] { SaveSelectedItem(); };
        item_enabled_row_.Toggle().WhenAction = [=] { SaveSelectedItem(); };
        item_face_row_.WhenAction = [=] { SaveSelectedItem(); };
        item_frame_row_.WhenAction = [=] { SaveSelectedItem(); };
        item_text_color_row_.WhenAction = [=] { SaveSelectedItem(); };

        add_button_.WhenAction = [=] { AddItem(); };
        remove_button_.WhenAction = [=] { RemoveItem(); };
        move_prev_button_.WhenAction = [=] { MoveItem(-1); };
        move_next_button_.WhenAction = [=] { MoveItem(1); };
        add_break_button_.WhenAction = [=] { AddBreakItem(); };
        add_spacer_button_.WhenAction = [=] { AddSpacerItem(); };
        model_tree_.WhenSelection = THISBACK(OnModelSelection);

        FinishInit();
        ApplyPreset();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        UiLabel::Style body = MakeBodyLabelStyle(Palette());
        UiLabel::Style value = MakeValueLabelStyle(Palette());
        UiLabel::Style section = MakeBodyLabelStyle(Palette());
        section.font = DemoSans(10, true);
        UiDropdown::Style dd = MakeDropdownStyle(Palette());
        UiBaseEdit::Style edit = MakeEditStyle(Palette());
        ApplyRowStyles(Palette(), state_theme_label_, state_theme_value_);
        ApplyRowStyles(Palette(), state_preset_label_, state_preset_value_);
        ApplyRowStyles(Palette(), state_direction_label_, state_direction_value_);
        ApplyRowStyles(Palette(), state_items_label_, state_items_value_);
        ApplyRowStyles(Palette(), state_pick_label_, state_pick_value_);
        preset_label_.SetStyle(body); dir_label_.SetStyle(body); align_label_.SetStyle(body); item_text_label_.SetStyle(body); item_mode_label_.SetStyle(body); item_align_label_.SetStyle(body);
        data_head_.SetStyle(section); layout_head_.SetStyle(section); item_head_.SetStyle(section); structure_head_.SetStyle(section);
        preset_drop_.SetStyle(dd); dir_drop_.SetStyle(dd); align_drop_.SetStyle(dd); item_mode_drop_.SetStyle(dd); item_align_drop_.SetStyle(dd);
        item_text_edit_.SetStyle(edit);
        ApplySliderStyle(body, value);
        ApplyToggleStyle(body);
        ApplyColorStyle(body);
        model_acc_.SetStyle(MakeAccordionStyle(Palette()));
        model_scroll_.SetStyle(MakeScrollBodyStyle());
        model_tree_.SetStyle(MakeTreeStyle());
        add_button_.SetStyle(MakeSmallButtonStyle(Palette()));
        remove_button_.SetStyle(MakeSmallButtonStyle(Palette()));
        move_prev_button_.SetStyle(MakeSmallButtonStyle(Palette()));
        move_next_button_.SetStyle(MakeSmallButtonStyle(Palette()));
        add_break_button_.SetStyle(MakeSmallButtonStyle(Palette()));
        add_spacer_button_.SetStyle(MakeSmallButtonStyle(Palette()));
    }

    virtual void LayoutPreviewContent() override
    {
        host_.SetRect(Preview().GetCanvasRect());
    }

private:
    struct EnumOption { const char* label; int value; };

    void AddColorRow(UiBoxLayout& target, UiCompositeColor& row, const char* name)
    {
        row.SetLabel(name).SetSwatchCount(1).ShowValue(false);
        target.Add(row).Fit();
    }

    void PopulateDropdown(UiDropdown& d, const EnumOption* options, int count)
    {
        d.UseInternalModel();
        d.Clear();
        for(int i = 0; i < count; i++)
            d.Add(options[i].label, options[i].value);
    }

    void InitSlider(UiCompositeSlider& row, int value, int lo, int hi)
    {
        row.Slider().SetRange(lo, hi).SetStep(1).SetValue(value);
        row.SetValueWidth(DPI(80));
    }

    void ApplySliderStyle(const UiLabel::Style& body, const UiLabel::Style& value)
    {
        Vector<UiCompositeSlider*> rows = { &item_spacing_row_, &content_margin_row_, &fixed_col_row_, &fixed_row_row_, &fixed_main_row_, &weight_row_, &min_cross_row_ };
        for(UiCompositeSlider* row : rows)
            row->SetLabelStyle(body).SetValueStyle(value);
    }

    void ApplyToggleStyle(const UiLabel::Style& body)
    {
        Vector<UiCompositeToggle*> rows = { &wrap_row_, &debug_row_, &wrap_auto_row_, &wrap_expand_row_, &item_visible_row_, &item_enabled_row_ };
        for(UiCompositeToggle* row : rows)
            row->SetLabelStyle(body);
    }

    void ApplyColorStyle(const UiLabel::Style& body)
    {
        Vector<UiCompositeColor*> rows = { &item_face_row_, &item_frame_row_, &item_text_color_row_ };
        for(UiCompositeColor* row : rows)
            row->SetLabelStyle(body);
    }

    void WireSlider(UiCompositeSlider& row, int& field)
    {
        row.WhenAction = [this, &row, &field] { field = (int)row.Slider().GetValue(); RefreshFromConfig(); };
    }

    void WireToggle(UiCompositeToggle& row, bool& field)
    {
        row.Toggle().WhenAction = [this, &row, &field] { field = row.Toggle().IsOn(); RefreshFromConfig(); };
    }

    UiTree::Style MakeTreeStyle() const
    {
        UiTree::Style s = UiTheme::ResolveTree();
        s.metrics.content_margin = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
        return s;
    }

    void UpdateModelViewport()
    {
        int viewport_h = min(max(model_tree_.GetContentSize().cy, DPI(120)), DPI(240));
        model_acc_.SetSectionBodyHeight(model_section_, viewport_h);
        int width = max(0, model_scroll_.GetViewportRect().GetWidth());
        model_tree_.SetRect(0, 0, width, max(viewport_h, model_tree_.GetContentSize().cy));
        model_scroll_.Layout();
        SyncInspectorLayout(true);
    }

    void ApplyPreset()
    {
        items_.Clear();
        selected_ = -1;
        switch(cfg_.preset) {
        case BOX_PRESET_STACK:
            cfg_.direction = (int)UiDirection::V;
            cfg_.wrap = false;
            for(int i = 0; i < 5; i++) AddNamedItem(Format("Item %d", i + 1), BOXITEM_FIT);
            break;
        case BOX_PRESET_TOOLBAR:
            cfg_.direction = (int)UiDirection::H;
            cfg_.wrap = false;
            AddNamedItem("Back", BOXITEM_FIT);
            AddNamedItem("Search", BOXITEM_EXPAND);
            AddNamedItem("Filter", BOXITEM_FIT);
            AddNamedItem("Share", BOXITEM_FIT);
            break;
        case BOX_PRESET_WRAP:
            cfg_.direction = (int)UiDirection::H;
            cfg_.wrap = true;
            for(int i = 0; i < 10; i++) AddNamedItem(Format("Chip %d", i + 1), BOXITEM_FIT);
            break;
        case BOX_PRESET_STRIP:
            cfg_.direction = (int)UiDirection::H;
            cfg_.wrap = false;
            for(int i = 0; i < 4; i++) { AddNamedItem(Format("Card %d", i + 1), i == 1 ? BOXITEM_EXPAND : BOXITEM_FIXED); if(i != 1) items_.Top().fixed_main = DPI(128); }
            break;
        default:
            cfg_.direction = (int)UiDirection::V;
            cfg_.wrap = false;
            AddNamedItem("Header", BOXITEM_FIXED); items_.Top().fixed_main = DPI(44);
            AddNamedItem("Summary", BOXITEM_FIT);
            AddNamedItem("Content", BOXITEM_EXPAND); items_.Top().weight = 2;
            AddNamedItem("Footer", BOXITEM_FIXED); items_.Top().fixed_main = DPI(38);
            break;
        }
        for(int i = 0; i < items_.GetCount(); i++) {
            if(items_[i].mode != BOXITEM_BREAK && items_[i].mode != BOXITEM_SPACER) {
                selected_ = i;
                break;
            }
        }
    }

    void AddNamedItem(const String& text, int mode)
    {
        BoxItemSpec& item = items_.Add();
        item.text = text;
        item.mode = mode;
        item.fixed_main = DPI(120);
        item.weight = 1;
        item.min_cross = DPI(40);
        item.face = Color(236, 241, 248);
        item.frame = Color(211, 221, 237);
        item.ink = Color(28, 47, 78);
    }

    void RefreshFromConfig()
    {
        if(items_.IsEmpty())
            ApplyPreset();
        if(selected_ < 0 || selected_ >= items_.GetCount()) {
            for(int i = 0; i < items_.GetCount(); i++) {
                if(items_[i].mode != BOXITEM_BREAK && items_[i].mode != BOXITEM_SPACER) {
                    selected_ = i;
                    break;
                }
            }
        }
        syncing_ = true;
        SyncRows();
        BuildPreview();
        BuildModelTree();
        SyncItemEditor();
        syncing_ = false;
        SyncState();
        SyncCode();
        LayoutPreviewContent();
        Preview().Refresh();
    }

    void BuildPreview()
    {
        cards_.Clear();
        UiBoxLayout& layout = host_.Reset((UiDirection)cfg_.direction);
        layout.SetGap(cfg_.item_spacing)
              .SetInset(cfg_.content_margin)
              .SetWrap(cfg_.wrap)
              .SetDebug(cfg_.debug)
              .SetWrapAutoResize(cfg_.wrap_auto_resize)
              .SetWrapRowsExpand(cfg_.wrap_rows_expand)
              .SetAlignItems((UiCrossAlign)cfg_.align_items);
        if((UiDirection)cfg_.direction == UiDirection::H)
            layout.SetFixedColumn(cfg_.fixed_column);
        else
            layout.SetFixedRow(cfg_.fixed_row);

        cards_.SetCount(items_.GetCount());
        for(int i = 0; i < items_.GetCount(); i++) {
            const BoxItemSpec& spec = items_[i];
            if(spec.mode == BOXITEM_BREAK) {
                layout.AddBreak();
                continue;
            }
            if(spec.mode == BOXITEM_SPACER) {
                layout.AddSpacer(max(1, spec.weight));
                continue;
            }
            BoxItemCard& card = cards_[i];
            card.SetIndex(i);
            card.WhenPick = callback(this, &CLASSNAME::PickItem);
            card.Configure(spec);
            UiBoxLayout::ItemRef ref = layout.Add(card);
            if(spec.mode == BOXITEM_FIXED)
                ref.Fixed(spec.fixed_main);
            else if(spec.mode == BOXITEM_EXPAND)
                ref.Expand(spec.weight);
            else
                ref.Fit();
            ref.MinCross(spec.min_cross).AlignSelf(spec.align);
        }
        SyncSelectionStyles();
    }

    void PickItem(int q)
    {
        selected_ = q;
        SyncSelectionStyles();
        SyncItemEditor();
        SyncState();
    }

    void OnModelSelection()
    {
        Value v = model_tree_.GetData();
        if(!IsNull(v) && IsNumber(v)) {
            int idx = (int)v;
            if(idx >= 0 && idx < items_.GetCount())
                PickItem(idx);
        }
    }
    void SyncSelectionStyles()
    {
        for(int i = 0; i < cards_.GetCount(); i++)
            cards_[i].SetPicked(i == selected_);
    }

    void BuildModelTree()
    {
        tree_model_.Clear();
        UiTreeNodeRef root = tree_model_.Root();
        UiTreeNodeRef layout = tree_model_.AddChild(root, UiModelItem("Layout"));
        tree_model_.AddChild(layout, UiModelItem("preset = " + BoxPresetName(cfg_.preset)));
        tree_model_.AddChild(layout, UiModelItem("direction = " + DirectionName((UiDirection)cfg_.direction)));
        tree_model_.AddChild(layout, UiModelItem("item_spacing = " + AsString(cfg_.item_spacing)));
        tree_model_.AddChild(layout, UiModelItem("content_margin = " + AsString(cfg_.content_margin)));
        UiTreeNodeRef items = tree_model_.AddChild(root, UiModelItem("Items"));
        for(int i = 0; i < items_.GetCount(); i++) {
            const BoxItemSpec& item = items_[i];
            UiTreeNodeRef n = tree_model_.AddChild(items, UiModelItem(Format("%d. %s", i + 1, item.text), i));
            tree_model_.AddChild(n, UiModelItem("mode = " + BoxModeName(item.mode)));
            tree_model_.AddChild(n, UiModelItem(String("visible = ") + (item.visible ? "true" : "false")));
            tree_model_.AddChild(n, UiModelItem(String("enabled = ") + (item.enabled ? "true" : "false")));
        }
        model_tree_.Refresh();
        UpdateModelViewport();
    }

    void SyncRows()
    {
        preset_drop_.SelectByData(cfg_.preset);
        dir_drop_.SelectByData(cfg_.direction);
        align_drop_.SelectByData(cfg_.align_items);
        item_spacing_row_.Slider().SetValue(cfg_.item_spacing);
        content_margin_row_.Slider().SetValue(cfg_.content_margin);
        fixed_col_row_.Slider().SetValue(cfg_.fixed_column);
        fixed_row_row_.Slider().SetValue(cfg_.fixed_row);
        wrap_row_.Toggle().SetOn(cfg_.wrap);
        wrap_auto_row_.Toggle().SetOn(cfg_.wrap_auto_resize);
        wrap_expand_row_.Toggle().SetOn(cfg_.wrap_rows_expand);
    }

    void SyncItemEditor()
    {
        if(selected_ < 0 || selected_ >= items_.GetCount()) {
            item_text_edit_.SetText("");
            return;
        }
        const BoxItemSpec& item = items_[selected_];
        item_text_edit_.SetText(item.text.ToWString());
        item_mode_drop_.SelectByData(item.mode);
        item_align_drop_.SelectByData((int)item.align);
        fixed_main_row_.Slider().SetValue(item.fixed_main);
        weight_row_.Slider().SetValue(item.weight);
        min_cross_row_.Slider().SetValue(item.min_cross);
        item_visible_row_.Toggle().SetOn(item.visible);
        item_enabled_row_.Toggle().SetOn(item.enabled);
        item_face_row_.SetSwatchColor(0, item.face);
        item_frame_row_.SetSwatchColor(0, item.frame);
        item_text_color_row_.SetSwatchColor(0, item.ink);
    }

    void SaveSelectedItem()
    {
        if(syncing_)
            return;
        if(selected_ < 0 || selected_ >= items_.GetCount())
            return;
        BoxItemSpec& item = items_[selected_];
        item.text = item_text_edit_.GetText().ToString();
        item.mode = (int)item_mode_drop_.GetSelectedData();
        item.align = (UiCrossAlign)(int)item_align_drop_.GetSelectedData();
        item.fixed_main = (int)fixed_main_row_.Slider().GetValue();
        item.weight = (int)weight_row_.Slider().GetValue();
        item.min_cross = (int)min_cross_row_.Slider().GetValue();
        item.visible = item_visible_row_.Toggle().IsOn();
        item.enabled = item_enabled_row_.Toggle().IsOn();
        item.face = item_face_row_.GetSwatchColor(0);
        item.frame = item_frame_row_.GetSwatchColor(0);
        item.ink = item_text_color_row_.GetSwatchColor(0);
        RefreshFromConfig();
    }

    void AddItem()
    {
        int pos = selected_ >= 0 ? selected_ + 1 : items_.GetCount();
        BoxItemSpec item;
        item.text = Format("Item %d", items_.GetCount() + 1);
        items_.Insert(pos, item);
        selected_ = pos;
        RefreshFromConfig();
    }

    void RemoveItem()
    {
        if(selected_ < 0 || selected_ >= items_.GetCount())
            return;
        items_.Remove(selected_);
        if(selected_ >= items_.GetCount())
            selected_ = items_.GetCount() - 1;
        RefreshFromConfig();
    }

    void MoveItem(int delta)
    {
        if(selected_ < 0 || selected_ >= items_.GetCount())
            return;
        int dst = selected_ + delta;
        if(dst < 0 || dst >= items_.GetCount())
            return;
        Swap(items_[selected_], items_[dst]);
        selected_ = dst;
        RefreshFromConfig();
    }

    void AddBreakItem()
    {
        BoxItemSpec item;
        item.text = "Break";
        item.mode = BOXITEM_BREAK;
        int pos = selected_ >= 0 ? selected_ + 1 : items_.GetCount();
        items_.Insert(pos, item);
        selected_ = pos;
        RefreshFromConfig();
    }

    void AddSpacerItem()
    {
        BoxItemSpec item;
        item.text = "Spacer";
        item.mode = BOXITEM_SPACER;
        item.weight = 1;
        int pos = selected_ >= 0 ? selected_ + 1 : items_.GetCount();
        items_.Insert(pos, item);
        selected_ = pos;
        RefreshFromConfig();
    }

    void SyncState()
    {
        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_preset_value_.SetText(BoxPresetName(cfg_.preset));
        state_direction_value_.SetText(DirectionName((UiDirection)cfg_.direction));
        state_items_value_.SetText(AsString(items_.GetCount()));
        state_pick_value_.SetText(selected_ >= 0 && selected_ < items_.GetCount() ? items_[selected_].text + " (insert after)" : "Append End");
    }

    void SyncCode()
    {
        String code;
        code << "UiBoxLayout layout(UiDirection::" << ((UiDirection)cfg_.direction == UiDirection::V ? "V" : "H") << ");\n";
        code << "layout.SetGap(" << cfg_.item_spacing << ").SetInset(" << cfg_.content_margin << ");\n";
        code << "layout.SetWrap(" << (cfg_.wrap ? "true" : "false") << ");\n";
        for(int i = 0; i < min(items_.GetCount(), 6); i++)
            code << "// item " << i + 1 << ": " << QuoteCpp(items_[i].text) << " -> " << BoxModeName(items_[i].mode) << "\n";
        SetUsageCode(code);
    }

    BoxConfig cfg_;
    Vector<BoxItemSpec> items_;
    int selected_ = -1;
    bool syncing_ = false;

    BoxLayoutHost host_;
    Array<BoxItemCard> cards_;
    UiAccordion model_acc_;
    int model_section_ = -1;
    UiLabel data_head_, layout_head_, item_head_, structure_head_;
    UiScrollPanel model_scroll_;
    DemoModelTree model_tree_;
    UiTreeModel tree_model_;
    UiBoxLayout data_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout layout_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout item_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout structure_box_ { UiBoxLayout::Direction::V };

    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_preset_row_ { UiBoxLayout::Direction::H }, state_direction_row_ { UiBoxLayout::Direction::H }, state_items_row_ { UiBoxLayout::Direction::H }, state_pick_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_preset_label_, state_preset_value_, state_direction_label_, state_direction_value_, state_items_label_, state_items_value_, state_pick_label_, state_pick_value_;

    UiBoxLayout preset_row_box_ { UiBoxLayout::Direction::H }, dir_row_box_ { UiBoxLayout::Direction::H }, align_row_box_ { UiBoxLayout::Direction::H };
    UiLabel preset_label_, dir_label_, align_label_;
    UiDropdown preset_drop_, dir_drop_, align_drop_;

    UiCompositeSlider item_spacing_row_, content_margin_row_, fixed_col_row_, fixed_row_row_;
    UiCompositeToggle wrap_row_, debug_row_, wrap_auto_row_, wrap_expand_row_;

    UiBoxLayout item_text_row_box_ { UiBoxLayout::Direction::H }, item_mode_row_box_ { UiBoxLayout::Direction::H }, item_align_row_box_ { UiBoxLayout::Direction::H };
    UiLabel item_text_label_, item_mode_label_, item_align_label_;
    UiLineEdit item_text_edit_;
    UiDropdown item_mode_drop_, item_align_drop_;
    UiCompositeSlider fixed_main_row_, weight_row_, min_cross_row_;
    UiCompositeToggle item_visible_row_, item_enabled_row_;
    UiCompositeColor item_face_row_, item_frame_row_, item_text_color_row_;

    UiBoxLayout action_row_ { UiBoxLayout::Direction::H }, move_next_row_ { UiBoxLayout::Direction::H };
    UiButton add_button_, remove_button_, move_prev_button_, move_next_button_, add_break_button_, add_spacer_button_;
};

}

GUI_APP_MAIN
{
    UiBoxLayoutBuilder().Run();
}










