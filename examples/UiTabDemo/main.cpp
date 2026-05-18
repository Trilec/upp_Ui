/*
    UiTabDemo
    ------------

    Purpose
    - Active Ui control demo used as a build smoke test and visual styling reference.

    Demo hygiene header
    - Keep this package compiling in the active demo sweep.
    - Prefer BuilderDemoSupport/shared shell and UiComposite inspector rows where practical.
    - Prefer UiTheme defaults; add local styling only when the demo intentionally showcases that variation.

    Changelog
    - 2026-05: active demo sweep verified; header added during demo cleanup pass.
*/
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
    int icon_size = DPI(16);
    int affordance_gap = DPI(4);
    int min_tab_main = DPI(72);
    int indicator_thickness = DPI(3);
    int active_frame_width = DPI(2);
    int tab_radius = DPI(8);
    int open_radius = DPI(6);
    bool expand_tabs = false;
    bool active_uses_body_face = true;
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

String TabVisualCode(UiTabVisual v)
{
    switch(v) {
    case UITAB_UNDERLINE: return "UITAB_UNDERLINE";
    case UITAB_SEGMENTED: return "UITAB_SEGMENTED";
    case UITAB_RAIL: return "UITAB_RAIL";
    case UITAB_DOCUMENT: return "UITAB_DOCUMENT";
    default: return "UITAB_CLASSIC";
    }
}

String TabPlacementCode(UiAlign a)
{
    switch(a) {
    case UiAlign::BOTTOM: return "UiAlign::BOTTOM";
    case UiAlign::LEFT: return "UiAlign::LEFT";
    case UiAlign::RIGHT: return "UiAlign::RIGHT";
    default: return "UiAlign::TOP";
    }
}

String SpanCode(UiSpan span)
{
    switch(span) {
    case MEDIUM: return "MEDIUM";
    case SMALL: return "SMALL";
    default: return "LARGE";
    }
}

String ColorCode(Color c)
{
    return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
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
        AddSliderRow(PropsBox(), item_spacing_row_, "Tab Spacing", "4px");
        AddSliderRow(PropsBox(), body_gap_row_, "Body Gap", "4px");
        AddSliderRow(PropsBox(), padding_x_row_, "Tab Padding X", "10px");
        AddSliderRow(PropsBox(), padding_y_row_, "Tab Padding Y", "6px");
        AddSliderRow(PropsBox(), inset_x_row_, "Strip Inset X", "0px");
        AddSliderRow(PropsBox(), inset_y_row_, "Strip Inset Y", "0px");
        AddSliderRow(PropsBox(), content_gap_row_, "Content Gap", "6px");
        AddSliderRow(PropsBox(), icon_size_row_, "Icon Size", "16px");
        AddSliderRow(PropsBox(), affordance_gap_row_, "Icon Gap", "4px");
        AddSliderRow(PropsBox(), min_main_row_, "Min Main", "72px");
        AddSliderRow(PropsBox(), indicator_thickness_row_, "Ind Thick", "3px");
        AddSliderRow(PropsBox(), active_frame_width_row_, "Active Line", "2px");
        AddSliderRow(PropsBox(), tab_radius_row_, "Tab Radius", "8px");
        AddSliderRow(PropsBox(), open_radius_row_, "Open Radius", "6px");
        AddToggleRow(PropsBox(), fill_tabs_row_, "Expand Tabs");
        AddToggleRow(PropsBox(), active_body_face_row_, "Active Body Face");
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

        InitSlider(tab_extent_row_, cfg_.tab_extent, DPI(26), DPI(96));
        InitSlider(item_spacing_row_, cfg_.item_spacing, 0, DPI(100));
        InitSlider(body_gap_row_, cfg_.body_gap, 0, DPI(12));
        InitSlider(padding_x_row_, cfg_.tab_padding_x, 0, DPI(20));
        InitSlider(padding_y_row_, cfg_.tab_padding_y, 0, DPI(14));
        InitSlider(inset_x_row_, cfg_.strip_inset_x, 0, DPI(14));
        InitSlider(inset_y_row_, cfg_.strip_inset_y, 0, DPI(14));
        InitSlider(content_gap_row_, cfg_.content_gap, 0, DPI(16));
        InitSlider(icon_size_row_, cfg_.icon_size, DPI(10), DPI(32));
        InitSlider(affordance_gap_row_, cfg_.affordance_gap, 0, DPI(12));
        InitSlider(min_main_row_, cfg_.min_tab_main, DPI(48), DPI(140));
        InitSlider(indicator_thickness_row_, cfg_.indicator_thickness, DPI(1), DPI(6));
        InitSlider(active_frame_width_row_, cfg_.active_frame_width, DPI(1), DPI(8));
        InitSlider(tab_radius_row_, cfg_.tab_radius, 0, DPI(18));
        InitSlider(open_radius_row_, cfg_.open_radius, 0, DPI(18));
        InitColorRow(body_face_row_, cfg_.body_face); InitColorRow(body_frame_row_, cfg_.body_frame); InitColorRow(body_text_row_, cfg_.body_text);
        InitColorRow(tab_face_row_, cfg_.tab_face); InitColorRow(tab_frame_row_, cfg_.tab_frame); InitColorRow(tab_text_row_, cfg_.tab_text);

        visual_drop_.WhenSelect = [=](int) { cfg_.visual = (UiTabVisual)(int)visual_drop_.GetSelectedData(); RefreshFromConfig(); };
        placement_drop_.WhenSelect = [=](int) { cfg_.placement = (UiAlign)(int)placement_drop_.GetSelectedData(); RefreshFromConfig(); };
        span_drop_.WhenSelect = [=](int) { cfg_.indicator_span = (UiSpan)(int)span_drop_.GetSelectedData(); RefreshFromConfig(); };
        WireSlider(tab_extent_row_, cfg_.tab_extent); WireSlider(item_spacing_row_, cfg_.item_spacing); WireSlider(body_gap_row_, cfg_.body_gap); WireSlider(padding_x_row_, cfg_.tab_padding_x); WireSlider(padding_y_row_, cfg_.tab_padding_y);
        WireSlider(inset_x_row_, cfg_.strip_inset_x); WireSlider(inset_y_row_, cfg_.strip_inset_y); WireSlider(content_gap_row_, cfg_.content_gap); WireSlider(icon_size_row_, cfg_.icon_size); WireSlider(affordance_gap_row_, cfg_.affordance_gap); WireSlider(min_main_row_, cfg_.min_tab_main); WireSlider(indicator_thickness_row_, cfg_.indicator_thickness); WireSlider(active_frame_width_row_, cfg_.active_frame_width); WireSlider(tab_radius_row_, cfg_.tab_radius); WireSlider(open_radius_row_, cfg_.open_radius);
        WireToggle(fill_tabs_row_, cfg_.expand_tabs); WireToggle(active_body_face_row_, cfg_.active_uses_body_face); WireToggle(close_buttons_row_, cfg_.close_buttons); WireToggle(drag_handles_row_, cfg_.drag_handles); WireToggle(drag_reorder_row_, cfg_.drag_reorder);
        WireColor(body_face_row_, cfg_.body_face); WireColor(body_frame_row_, cfg_.body_frame); WireColor(body_text_row_, cfg_.body_text); WireColor(tab_face_row_, cfg_.tab_face); WireColor(tab_frame_row_, cfg_.tab_frame); WireColor(tab_text_row_, cfg_.tab_text);

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        UiTab::Style base = UiTheme::ResolveTab(cfg_.visual);
        StyledState st = ST_NORMAL;
        cfg_.body_face = base.palette.face[st].IsSolid() ? base.palette.face[st].color : Palette().paper;
        cfg_.body_frame = base.palette.frame[st];
        cfg_.body_text = base.palette.ink[st];
        cfg_.tab_face = base.tab_palette.face[st].IsSolid() ? base.tab_palette.face[st].color : Palette().segment_face;
        cfg_.tab_frame = base.tab_palette.frame[st];
        cfg_.tab_text = base.tab_palette.ink[st];
        RefreshFromConfig();
    }

    virtual void LayoutPreviewContent() override
    {
        Rect c = Preview().GetCanvasRect();
        tab_.SetRect(c.left + DPI(24), c.top + DPI(28), max(DPI(340), c.GetWidth() - DPI(48)), max(DPI(240), c.GetHeight() - DPI(56)));
    }

private:
    struct EnumOption { const char* label; int value; };
    void AddColorRow(UiBoxLayout& t, UiCompositeColor& r, const char* n) { r.SetLabel(n).SetColorCount(1).ShowValue(false); t.Add(r).Fit(); }
    void PopulateDropdown(UiDropdown& d, const EnumOption* o, int n){ d.UseInternalModel(); d.Clear(); for(int i=0;i<n;i++) d.Add(o[i].label,o[i].value);}    
    void InitColorRow(UiCompositeColor& r, Color c){ r.SetColor(0,c); }
    void InitSlider(UiCompositeSlider& r, int value, int lo, int hi){ r.Slider().SetRange(lo,hi).SetStep(1).SetValue(value); }
    void WireSlider(UiCompositeSlider& r, int& field)
    {
        auto apply = [this, &r, &field] {
            field = (int)r.Slider().GetValue();
            RefreshFromConfig();
        };
        r.WhenChanging = apply;
        r.WhenAction = apply;
    }
    void WireToggle(UiCompositeToggle& r, bool& field){ r.Toggle().WhenAction = [this, &r, &field] { field = r.Toggle().IsOn(); RefreshFromConfig(); }; }
    void WireColor(UiCompositeColor& r, Color& field){ r.WhenAction = [this, &r, &field] { field = r.GetColor(0); RefreshFromConfig(); }; }

    UiTab::Style BuildStyle() const
    {
        UiTab::Style s = UiTab::StyleDefault();
        s.visual = cfg_.visual; s.tab_extent = cfg_.tab_extent; s.item_spacing = cfg_.item_spacing; s.body_gap = cfg_.body_gap; s.tab_padding = Rect(cfg_.tab_padding_x, cfg_.tab_padding_y, cfg_.tab_padding_x, cfg_.tab_padding_y);
        s.strip_inset = Rect(cfg_.strip_inset_x, cfg_.strip_inset_y, cfg_.strip_inset_x, cfg_.strip_inset_y); s.content_gap = cfg_.content_gap; s.icon_size = cfg_.icon_size; s.affordance_gap = cfg_.affordance_gap; s.min_tab_main = cfg_.min_tab_main; s.indicator_thickness = cfg_.indicator_thickness; s.active_frame_width = cfg_.active_frame_width; s.open_corner_radius = cfg_.open_radius; s.active_frame_color = cfg_.tab_frame; s.indicator_span = cfg_.indicator_span; s.expand_tabs = cfg_.expand_tabs; s.active_tab_uses_body_face = cfg_.active_uses_body_face;
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(cfg_.body_face); s.palette.frame[i] = cfg_.body_frame; s.palette.ink[i] = cfg_.body_text;
            s.tab_palette.face[i] = UiFill::Solid(cfg_.tab_face); s.tab_palette.frame[i] = cfg_.tab_frame; s.tab_palette.ink[i] = cfg_.tab_text;
        }
        s.metrics.face_enabled = true; s.metrics.frame_enabled = true; s.metrics.frame_width = 1; s.metrics.radius = DPI(8);
        s.tab_metrics.face_enabled = true; s.tab_metrics.frame_enabled = true; s.tab_metrics.frame_width = 1; s.tab_metrics.radius = cfg_.tab_radius;
        return s;
    }

    void RefreshFromConfig()
    {
        visual_drop_.SelectByData((int)cfg_.visual); placement_drop_.SelectByData((int)cfg_.placement); span_drop_.SelectByData((int)cfg_.indicator_span);
        SyncRows();
        tab_.SetCustomStyle(BuildStyle()).SetPlacement(cfg_.placement).SetVisual(cfg_.visual).SetExpandTabs(cfg_.expand_tabs).EnableCloseButtons(cfg_.close_buttons).EnableDragHandles(cfg_.drag_handles).EnableDragReorder(cfg_.drag_reorder);
        SyncState(); SyncCode(); LayoutPreviewContent(); Preview().Refresh();
    }
    void SyncRows()
    {
        tab_extent_row_.Slider().SetValue(cfg_.tab_extent); item_spacing_row_.Slider().SetValue(cfg_.item_spacing); body_gap_row_.Slider().SetValue(cfg_.body_gap); padding_x_row_.Slider().SetValue(cfg_.tab_padding_x); padding_y_row_.Slider().SetValue(cfg_.tab_padding_y); inset_x_row_.Slider().SetValue(cfg_.strip_inset_x); inset_y_row_.Slider().SetValue(cfg_.strip_inset_y); content_gap_row_.Slider().SetValue(cfg_.content_gap); icon_size_row_.Slider().SetValue(cfg_.icon_size); affordance_gap_row_.Slider().SetValue(cfg_.affordance_gap); min_main_row_.Slider().SetValue(cfg_.min_tab_main); indicator_thickness_row_.Slider().SetValue(cfg_.indicator_thickness); active_frame_width_row_.Slider().SetValue(cfg_.active_frame_width); tab_radius_row_.Slider().SetValue(cfg_.tab_radius); open_radius_row_.Slider().SetValue(cfg_.open_radius);
        tab_extent_row_.SetValueText(AsString(cfg_.tab_extent) + "px"); item_spacing_row_.SetValueText(AsString(cfg_.item_spacing) + "px"); body_gap_row_.SetValueText(AsString(cfg_.body_gap) + "px"); padding_x_row_.SetValueText(AsString(cfg_.tab_padding_x) + "px"); padding_y_row_.SetValueText(AsString(cfg_.tab_padding_y) + "px"); inset_x_row_.SetValueText(AsString(cfg_.strip_inset_x) + "px"); inset_y_row_.SetValueText(AsString(cfg_.strip_inset_y) + "px"); content_gap_row_.SetValueText(AsString(cfg_.content_gap) + "px"); icon_size_row_.SetValueText(AsString(cfg_.icon_size) + "px"); affordance_gap_row_.SetValueText(AsString(cfg_.affordance_gap) + "px"); min_main_row_.SetValueText(AsString(cfg_.min_tab_main) + "px"); indicator_thickness_row_.SetValueText(AsString(cfg_.indicator_thickness) + "px"); active_frame_width_row_.SetValueText(AsString(cfg_.active_frame_width) + "px"); tab_radius_row_.SetValueText(AsString(cfg_.tab_radius) + "px"); open_radius_row_.SetValueText(AsString(cfg_.open_radius) + "px");
        fill_tabs_row_.Toggle().SetOn(cfg_.expand_tabs); active_body_face_row_.Toggle().SetOn(cfg_.active_uses_body_face); close_buttons_row_.Toggle().SetOn(cfg_.close_buttons); drag_handles_row_.Toggle().SetOn(cfg_.drag_handles); drag_reorder_row_.Toggle().SetOn(cfg_.drag_reorder);
        body_face_row_.SetColor(0, cfg_.body_face); body_frame_row_.SetColor(0, cfg_.body_frame); body_text_row_.SetColor(0, cfg_.body_text); tab_face_row_.SetColor(0, cfg_.tab_face); tab_frame_row_.SetColor(0, cfg_.tab_frame); tab_text_row_.SetColor(0, cfg_.tab_text);
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
        code << "UiTab::Style style = UiTheme::ResolveTab(" << TabVisualCode(cfg_.visual) << ");\n";
        code << "style.visual = " << TabVisualCode(cfg_.visual) << ";\n";
        code << "style.tab_extent = DPI(" << cfg_.tab_extent << ");\n";
        code << "style.item_spacing = DPI(" << cfg_.item_spacing << ");\n";
        code << "style.body_gap = DPI(" << cfg_.body_gap << ");\n";
        code << "style.tab_padding = Rect(DPI(" << cfg_.tab_padding_x << "), DPI(" << cfg_.tab_padding_y << "), DPI(" << cfg_.tab_padding_x << "), DPI(" << cfg_.tab_padding_y << "));\n";
        code << "style.strip_inset = Rect(DPI(" << cfg_.strip_inset_x << "), DPI(" << cfg_.strip_inset_y << "), DPI(" << cfg_.strip_inset_x << "), DPI(" << cfg_.strip_inset_y << "));\n";
        code << "style.content_gap = DPI(" << cfg_.content_gap << ");\n";
        code << "style.icon_size = DPI(" << cfg_.icon_size << ");\n";
        code << "style.affordance_gap = DPI(" << cfg_.affordance_gap << ");\n";
        code << "style.min_tab_main = DPI(" << cfg_.min_tab_main << ");\n";
        code << "style.indicator_thickness = DPI(" << cfg_.indicator_thickness << ");\n";
        code << "style.active_frame_width = DPI(" << cfg_.active_frame_width << ");\n";
        code << "style.open_corner_radius = DPI(" << cfg_.open_radius << ");\n";
        code << "style.indicator_span = " << SpanCode(cfg_.indicator_span) << ";\n";
        code << "style.expand_tabs = " << (cfg_.expand_tabs ? "true" : "false") << ";\n";
        code << "style.active_tab_uses_body_face = " << (cfg_.active_uses_body_face ? "true" : "false") << ";\n";
        code << "style.active_frame_color = " << ColorCode(cfg_.tab_frame) << ";\n";
        code << "style.metrics.radius = DPI(8);\n";
        code << "style.tab_metrics.radius = DPI(" << cfg_.tab_radius << ");\n";
        code << "for(int i = 0; i < 4; i++) {\n";
        code << "    style.palette.face[i] = UiFill::Solid(" << ColorCode(cfg_.body_face) << ");\n";
        code << "    style.palette.frame[i] = " << ColorCode(cfg_.body_frame) << ";\n";
        code << "    style.palette.ink[i] = " << ColorCode(cfg_.body_text) << ";\n";
        code << "    style.tab_palette.face[i] = UiFill::Solid(" << ColorCode(cfg_.tab_face) << ");\n";
        code << "    style.tab_palette.frame[i] = " << ColorCode(cfg_.tab_frame) << ";\n";
        code << "    style.tab_palette.ink[i] = " << ColorCode(cfg_.tab_text) << ";\n";
        code << "}\n";
        code << "tabs.SetCustomStyle(style)\n";
        code << "    .SetPlacement(" << TabPlacementCode(cfg_.placement) << ")\n";
        code << "    .SetVisual(" << TabVisualCode(cfg_.visual) << ")\n";
        code << "    .SetExpandTabs(" << (cfg_.expand_tabs ? "true" : "false") << ")\n";
        code << "    .EnableCloseButtons(" << (cfg_.close_buttons ? "true" : "false") << ")\n";
        code << "    .EnableDragHandles(" << (cfg_.drag_handles ? "true" : "false") << ")\n";
        code << "    .EnableDragReorder(" << (cfg_.drag_reorder ? "true" : "false") << ");\n";
        code << "UiLabel& overview = *new UiLabel;\n";
        code << "UiLabel& settings = *new UiLabel;\n";
        code << "UiLabel& logs = *new UiLabel;\n";
        code << "overview.SetText(\"Overview page\").SetAlign(UiAlign::CENTER, UiAlign::CENTER);\n";
        code << "settings.SetText(\"Settings page\").SetAlign(UiAlign::CENTER, UiAlign::CENTER);\n";
        code << "logs.SetText(\"Logs page\").SetAlign(UiAlign::CENTER, UiAlign::CENTER);\n";
        code << "tabs.Add(overview, \"Overview\", ICON_DESIGN_HOME_48());\n";
        code << "tabs.Add(settings, \"Settings\", ICON_DESIGN_SETTINGS_48());\n";
        code << "tabs.Add(logs, \"Logs\", ICON_DESIGN_MENU_48());\n";
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
    UiCompositeSlider tab_extent_row_, item_spacing_row_, body_gap_row_, padding_x_row_, padding_y_row_, inset_x_row_, inset_y_row_, content_gap_row_, icon_size_row_, affordance_gap_row_, min_main_row_, indicator_thickness_row_, active_frame_width_row_, tab_radius_row_, open_radius_row_;
    UiCompositeToggle fill_tabs_row_, active_body_face_row_, close_buttons_row_, drag_handles_row_, drag_reorder_row_;
    UiCompositeColor body_face_row_, body_frame_row_, body_text_row_, tab_face_row_, tab_frame_row_, tab_text_row_;
};

}

GUI_APP_MAIN
{
    UiTabBuilder().Run();
}

