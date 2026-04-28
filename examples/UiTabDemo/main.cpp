#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

enum TabPlacementMode { TAB_TOP = 0, TAB_BOTTOM, TAB_LEFT, TAB_RIGHT };

enum IndicatorSpanMode { SPAN_LARGE = 0, SPAN_MEDIUM, SPAN_SMALL };

struct TabConfig {
    UiTabVisual visual = UITAB_CLASSIC;
    UiAlign placement = UiAlign::TOP;
    UiSpan indicator_span = LARGE;
    int tab_extent = DPI(32);
    int item_spacing = DPI(4);
    int body_gap = DPI(4);
    int tab_padding_x = DPI(10);
    int tab_padding_y = DPI(6);
    int strip_inset_x = 0;
    int strip_inset_y = 0;
    int content_gap = DPI(6);
    int affordance_gap = DPI(4);
    int affordance_size = DPI(12);
    int min_tab_main = DPI(72);
    int indicator_thickness = DPI(3);
    bool fill_tabs = false;
    bool close_buttons = true;
    bool drag_handles = true;
    bool drag_reorder = true;
    Color body_face = Color(250, 252, 255);
    Color body_frame = Color(211, 221, 237);
    Color body_text = Color(28, 47, 78);
    Color tab_face = Color(236, 241, 248);
    Color tab_frame = Color(211, 221, 237);
    Color tab_text = Color(28, 47, 78);
};

String TabVisualName(UiTabVisual v)
{
    switch(v) {
    case UITAB_UNDERLINE: return "Underline";
    case UITAB_SEGMENTED: return "Segmented";
    case UITAB_RAIL: return "Rail";
    case UITAB_DOCUMENT: return "Document";
    default: return "Classic";
    }
}

class UiTabBuilder : public BuilderWindowBase {
public:
    typedef UiTabBuilder CLASSNAME;

    UiTabBuilder()
        : BuilderWindowBase("UiTabDemo", "U++ UiTab Builder", "Inspect tab-strip placement, affordances, spacing, and palette lanes from one shell.")
    {
        Preview().Add(tab_);
        page_a_.SetText("Overview page").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        page_b_.SetText("Settings page").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        page_c_.SetText("Logs page").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        tab_.Add(page_a_, "Overview", ICON_DESIGN_HOME_48());
        tab_.Add(page_b_, "Settings", ICON_DESIGN_SETTINGS_48());
        tab_.Add(page_c_, "Logs", ICON_DESIGN_MENU_48());
        tab_.WhenAction = [=] { SyncState(); };

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_visual_row_, state_visual_label_, state_visual_value_, "Visual");
        AddStateRow(StateBox(), state_tab_row_, state_tab_label_, state_tab_value_, "Active Tab");
        AddStateRow(StateBox(), state_place_row_, state_place_label_, state_place_value_, "Placement");

        AddDropdownRow(PropsBox(), visual_row_box_, visual_label_, visual_drop_, "Visual");
        AddDropdownRow(PropsBox(), placement_row_box_, placement_label_, placement_drop_, "Placement");
        AddDropdownRow(PropsBox(), span_row_box_, span_label_, span_drop_, "Ind Span");
        AddSliderRow(PropsBox(), tab_extent_row_, "Tab Extent", "32px");
        AddSliderRow(PropsBox(), item_spacing_row_, "Item Spacing", "4px");
        AddSliderRow(PropsBox(), body_gap_row_, "Body Gap", "4px");
        AddSliderRow(PropsBox(), padding_x_row_, "Pad X", "10px");
        AddSliderRow(PropsBox(), padding_y_row_, "Pad Y", "6px");
        AddSliderRow(PropsBox(), inset_x_row_, "Inset X", "0px");
        AddSliderRow(PropsBox(), inset_y_row_, "Inset Y", "0px");
        AddSliderRow(PropsBox(), content_gap_row_, "Content Gap", "6px");
        AddSliderRow(PropsBox(), affordance_gap_row_, "Afford Gap", "4px");
        AddSliderRow(PropsBox(), affordance_size_row_, "Afford Sz", "12px");
        AddSliderRow(PropsBox(), min_main_row_, "Min Main", "72px");
        AddSliderRow(PropsBox(), indicator_thickness_row_, "Ind Thick", "3px");
        AddToggleRow(PropsBox(), fill_tabs_row_, "Fill Tabs");
        AddToggleRow(PropsBox(), close_buttons_row_, "Close Buttons");
        AddToggleRow(PropsBox(), drag_handles_row_, "Drag Handles");
        AddToggleRow(PropsBox(), drag_reorder_row_, "Use Drag");
        AddColorRow(PropsBox(), body_face_row_, "Body Face");
        AddColorRow(PropsBox(), body_frame_row_, "Body Frame");
        AddColorRow(PropsBox(), body_text_row_, "Body Text");
        AddColorRow(PropsBox(), tab_face_row_, "Tab Face");
        AddColorRow(PropsBox(), tab_frame_row_, "Tab Frame");
        AddColorRow(PropsBox(), tab_text_row_, "Tab Text");

        const EnumOption visuals[] = {
            { "Classic", (int)UITAB_CLASSIC }, { "Underline", (int)UITAB_UNDERLINE }, { "Segmented", (int)UITAB_SEGMENTED }, { "Rail", (int)UITAB_RAIL }, { "Document", (int)UITAB_DOCUMENT }
        };
        const EnumOption placements[] = {
            { "Top", (int)UiAlign::TOP }, { "Bottom", (int)UiAlign::BOTTOM }, { "Left", (int)UiAlign::LEFT }, { "Right", (int)UiAlign::RIGHT }
        };
        const EnumOption spans[] = {
            { "Large", (int)LARGE }, { "Medium", (int)MEDIUM }, { "Small", (int)SMALL }
        };
        PopulateDropdown(visual_drop_, visuals, 5);
        PopulateDropdown(placement_drop_, placements, 4);
        PopulateDropdown(span_drop_, spans, 3);

        InitSlider(tab_extent_row_, cfg_.tab_extent, DPI(26), DPI(48));
        InitSlider(item_spacing_row_, cfg_.item_spacing, 0, DPI(12));
        InitSlider(body_gap_row_, cfg_.body_gap, 0, DPI(12));
        InitSlider(padding_x_row_, cfg_.tab_padding_x, 0, DPI(20));
        InitSlider(padding_y_row_, cfg_.tab_padding_y, 0, DPI(14));
        InitSlider(inset_x_row_, cfg_.strip_inset_x, 0, DPI(14));
        InitSlider(inset_y_row_, cfg_.strip_inset_y, 0, DPI(14));
        InitSlider(content_gap_row_, cfg_.content_gap, 0, DPI(16));
        InitSlider(affordance_gap_row_, cfg_.affordance_gap, 0, DPI(12));
        InitSlider(affordance_size_row_, cfg_.affordance_size, DPI(8), DPI(18));
        InitSlider(min_main_row_, cfg_.min_tab_main, DPI(48), DPI(140));
        InitSlider(indicator_thickness_row_, cfg_.indicator_thickness, DPI(1), DPI(6));
        InitColorRow(body_face_row_, cfg_.body_face); InitColorRow(body_frame_row_, cfg_.body_frame); InitColorRow(body_text_row_, cfg_.body_text);
        InitColorRow(tab_face_row_, cfg_.tab_face); InitColorRow(tab_frame_row_, cfg_.tab_frame); InitColorRow(tab_text_row_, cfg_.tab_text);

        visual_drop_.WhenSelect = [=](int) { cfg_.visual = (UiTabVisual)(int)visual_drop_.GetSelectedData(); RefreshFromConfig(); };
        placement_drop_.WhenSelect = [=](int) { cfg_.placement = (UiAlign)(int)placement_drop_.GetSelectedData(); RefreshFromConfig(); };
        span_drop_.WhenSelect = [=](int) { cfg_.indicator_span = (UiSpan)(int)span_drop_.GetSelectedData(); RefreshFromConfig(); };
        WireSlider(tab_extent_row_, cfg_.tab_extent); WireSlider(item_spacing_row_, cfg_.item_spacing); WireSlider(body_gap_row_, cfg_.body_gap); WireSlider(padding_x_row_, cfg_.tab_padding_x); WireSlider(padding_y_row_, cfg_.tab_padding_y);
        WireSlider(inset_x_row_, cfg_.strip_inset_x); WireSlider(inset_y_row_, cfg_.strip_inset_y); WireSlider(content_gap_row_, cfg_.content_gap); WireSlider(affordance_gap_row_, cfg_.affordance_gap); WireSlider(affordance_size_row_, cfg_.affordance_size); WireSlider(min_main_row_, cfg_.min_tab_main); WireSlider(indicator_thickness_row_, cfg_.indicator_thickness);
        WireToggle(fill_tabs_row_, cfg_.fill_tabs); WireToggle(close_buttons_row_, cfg_.close_buttons); WireToggle(drag_handles_row_, cfg_.drag_handles); WireToggle(drag_reorder_row_, cfg_.drag_reorder);
        WireColor(body_face_row_, cfg_.body_face); WireColor(body_frame_row_, cfg_.body_frame); WireColor(body_text_row_, cfg_.body_text); WireColor(tab_face_row_, cfg_.tab_face); WireColor(tab_frame_row_, cfg_.tab_frame); WireColor(tab_text_row_, cfg_.tab_text);

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
        state_visual_label_.SetStyle(body); state_visual_value_.SetStyle(value);
        state_tab_label_.SetStyle(body); state_tab_value_.SetStyle(value);
        state_place_label_.SetStyle(body); state_place_value_.SetStyle(value);
        visual_label_.SetStyle(body); placement_label_.SetStyle(body); span_label_.SetStyle(body);
        visual_drop_.SetStyle(dd); placement_drop_.SetStyle(dd); span_drop_.SetStyle(dd);
        ApplySliderStyle(body, value); ApplyToggleStyle(body); ApplyColorStyle(body);
        page_a_.SetStyle(MakeBodyLabelStyle(Palette()));
        page_b_.SetStyle(MakeBodyLabelStyle(Palette()));
        page_c_.SetStyle(MakeBodyLabelStyle(Palette()));
    }

    virtual void LayoutPreviewContent() override
    {
        Rect c = Preview().GetCanvasRect();
        tab_.SetRect(c.left + DPI(24), c.top + DPI(28), max(DPI(340), c.GetWidth() - DPI(48)), max(DPI(240), c.GetHeight() - DPI(56)));
    }

private:
    struct EnumOption { const char* label; int value; };
    void AddColorRow(UiBoxLayout& t, UiCompositeColor& r, const char* n) { r.SetLabel(n).SetSwatchCount(1).ShowValue(false); t.Add(r).Fit(); }
    void PopulateDropdown(UiDropdown& d, const EnumOption* o, int n){ d.UseInternalModel(); d.Clear(); for(int i=0;i<n;i++) d.Add(o[i].label,o[i].value);}    
    void InitColorRow(UiCompositeColor& r, Color c){ r.SetSwatchColor(0,c); }
    void InitSlider(UiCompositeSlider& r, int value, int lo, int hi){ r.Slider().SetRange(lo,hi).SetStep(1).SetValue(value); }
    void ApplySliderStyle(const UiLabel::Style& body, const UiLabel::Style& value){ Vector<UiCompositeSlider*> rows = { &tab_extent_row_, &item_spacing_row_, &body_gap_row_, &padding_x_row_, &padding_y_row_, &inset_x_row_, &inset_y_row_, &content_gap_row_, &affordance_gap_row_, &affordance_size_row_, &min_main_row_, &indicator_thickness_row_ }; for(auto* r : rows) r->SetLabelStyle(body).SetValueStyle(value); }
    void ApplyToggleStyle(const UiLabel::Style& body){ Vector<UiCompositeToggle*> rows = { &fill_tabs_row_, &close_buttons_row_, &drag_handles_row_, &drag_reorder_row_ }; for(auto* r : rows) r->SetLabelStyle(body); }
    void ApplyColorStyle(const UiLabel::Style& body){ Vector<UiCompositeColor*> rows = { &body_face_row_, &body_frame_row_, &body_text_row_, &tab_face_row_, &tab_frame_row_, &tab_text_row_ }; for(auto* r : rows) r->SetLabelStyle(body); }
    void WireSlider(UiCompositeSlider& r, int& field){ r.WhenAction = [this, &r, &field] { field = (int)r.Slider().GetValue(); RefreshFromConfig(); }; }
    void WireToggle(UiCompositeToggle& r, bool& field){ r.Toggle().WhenAction = [this, &r, &field] { field = r.Toggle().IsOn(); RefreshFromConfig(); }; }
    void WireColor(UiCompositeColor& r, Color& field){ r.WhenAction = [this, &r, &field] { field = r.GetSwatchColor(0); RefreshFromConfig(); }; }

    UiTab::Style BuildStyle() const
    {
        UiTab::Style s = UiTab::StyleDefault();
        s.visual = cfg_.visual; s.tab_extent = cfg_.tab_extent; s.item_spacing = cfg_.item_spacing; s.body_gap = cfg_.body_gap; s.tab_padding = Rect(cfg_.tab_padding_x, cfg_.tab_padding_y, cfg_.tab_padding_x, cfg_.tab_padding_y);
        s.strip_inset = Rect(cfg_.strip_inset_x, cfg_.strip_inset_y, cfg_.strip_inset_x, cfg_.strip_inset_y); s.content_gap = cfg_.content_gap; s.affordance_gap = cfg_.affordance_gap; s.affordance_size = cfg_.affordance_size; s.min_tab_main = cfg_.min_tab_main; s.indicator_thickness = cfg_.indicator_thickness; s.indicator_span = cfg_.indicator_span; s.fill_tabs = cfg_.fill_tabs;
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(cfg_.body_face); s.palette.frame[i] = cfg_.body_frame; s.palette.ink[i] = cfg_.body_text;
            s.tab_palette.face[i] = UiFill::Solid(cfg_.tab_face); s.tab_palette.frame[i] = cfg_.tab_frame; s.tab_palette.ink[i] = cfg_.tab_text;
        }
        s.metrics.face_enabled = true; s.metrics.frame_enabled = true; s.metrics.frame_width = 1; s.metrics.radius = DPI(8);
        s.tab_metrics.face_enabled = true; s.tab_metrics.frame_enabled = true; s.tab_metrics.frame_width = 1; s.tab_metrics.radius = DPI(8);
        return s;
    }

    void RefreshFromConfig()
    {
        visual_drop_.SelectByData((int)cfg_.visual); placement_drop_.SelectByData((int)cfg_.placement); span_drop_.SelectByData((int)cfg_.indicator_span);
        SyncRows();
        tab_.SetStyle(BuildStyle()).SetPlacement(cfg_.placement).SetVisual(cfg_.visual).SetFillTabs(cfg_.fill_tabs).EnableCloseButtons(cfg_.close_buttons).EnableDragHandles(cfg_.drag_handles).EnableDragReorder(cfg_.drag_reorder);
        SyncState(); SyncCode(); LayoutPreviewContent(); Preview().Refresh();
    }
    void SyncRows()
    {
        tab_extent_row_.Slider().SetValue(cfg_.tab_extent); item_spacing_row_.Slider().SetValue(cfg_.item_spacing); body_gap_row_.Slider().SetValue(cfg_.body_gap); padding_x_row_.Slider().SetValue(cfg_.tab_padding_x); padding_y_row_.Slider().SetValue(cfg_.tab_padding_y); inset_x_row_.Slider().SetValue(cfg_.strip_inset_x); inset_y_row_.Slider().SetValue(cfg_.strip_inset_y); content_gap_row_.Slider().SetValue(cfg_.content_gap); affordance_gap_row_.Slider().SetValue(cfg_.affordance_gap); affordance_size_row_.Slider().SetValue(cfg_.affordance_size); min_main_row_.Slider().SetValue(cfg_.min_tab_main); indicator_thickness_row_.Slider().SetValue(cfg_.indicator_thickness);
        fill_tabs_row_.Toggle().SetOn(cfg_.fill_tabs); close_buttons_row_.Toggle().SetOn(cfg_.close_buttons); drag_handles_row_.Toggle().SetOn(cfg_.drag_handles); drag_reorder_row_.Toggle().SetOn(cfg_.drag_reorder);
        body_face_row_.SetSwatchColor(0, cfg_.body_face); body_frame_row_.SetSwatchColor(0, cfg_.body_frame); body_text_row_.SetSwatchColor(0, cfg_.body_text); tab_face_row_.SetSwatchColor(0, cfg_.tab_face); tab_frame_row_.SetSwatchColor(0, cfg_.tab_frame); tab_text_row_.SetSwatchColor(0, cfg_.tab_text);
    }
    void SyncState()
    {
        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_visual_value_.SetText(TabVisualName(cfg_.visual));
        state_tab_value_.SetText(tab_.GetActiveTab() == 0 ? "Overview" : tab_.GetActiveTab() == 1 ? "Settings" : "Logs");
        state_place_value_.SetText(cfg_.placement == UiAlign::BOTTOM ? "Bottom" : cfg_.placement == UiAlign::LEFT ? "Left" : cfg_.placement == UiAlign::RIGHT ? "Right" : "Top");
    }
    void SyncCode()
    {
        String code;
        code << "UiTab tabs;\n";
        code << "tabs.SetPlacement(UiAlign::" << (cfg_.placement == UiAlign::BOTTOM ? "BOTTOM" : cfg_.placement == UiAlign::LEFT ? "LEFT" : cfg_.placement == UiAlign::RIGHT ? "RIGHT" : "TOP") << ");\n";
        code << "tabs.SetVisual(" << TabVisualName(cfg_.visual) << ");\n";
        code << "// tab_extent=" << cfg_.tab_extent << ", item_spacing=" << cfg_.item_spacing << ", content_gap=" << cfg_.content_gap << "\n";
        SetUsageCode(code);
    }

    TabConfig cfg_;
    UiTab tab_;
    UiLabel page_a_, page_b_, page_c_;
    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_visual_row_ { UiBoxLayout::Direction::H }, state_tab_row_ { UiBoxLayout::Direction::H }, state_place_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_visual_label_, state_visual_value_, state_tab_label_, state_tab_value_, state_place_label_, state_place_value_;
    UiBoxLayout visual_row_box_ { UiBoxLayout::Direction::H }, placement_row_box_ { UiBoxLayout::Direction::H }, span_row_box_ { UiBoxLayout::Direction::H };
    UiLabel visual_label_, placement_label_, span_label_;
    UiDropdown visual_drop_, placement_drop_, span_drop_;
    UiCompositeSlider tab_extent_row_, item_spacing_row_, body_gap_row_, padding_x_row_, padding_y_row_, inset_x_row_, inset_y_row_, content_gap_row_, affordance_gap_row_, affordance_size_row_, min_main_row_, indicator_thickness_row_;
    UiCompositeToggle fill_tabs_row_, close_buttons_row_, drag_handles_row_, drag_reorder_row_;
    UiCompositeColor body_face_row_, body_frame_row_, body_text_row_, tab_face_row_, tab_frame_row_, tab_text_row_;
};

}

GUI_APP_MAIN
{
    UiTabBuilder().Run();
}

