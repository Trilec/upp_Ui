/*
    UiDropdown normalized ownership-reference demo.
    The previous large builder remains in main.cpp for historical reference and
    is intentionally not compiled by UiDropdownDemo.upp.
*/
#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

class UiDropdownDemoWindow : public BuilderWindowBase {
public:
    typedef UiDropdownDemoWindow CLASSNAME;

    UiDropdownDemoWindow()
        : BuilderWindowBase("UiDropdown Demo", "UiDropdown",
                            "Collapsed control and nested Popup/* ownership reference")
    {
        Preview().Add(dropdown_);
        BuildSections();
        BuildRows();
        Populate();
        Connect();
        FinishInit();
    }

protected:
    void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        int cx = min(DPI(380), max(DPI(180), canvas.GetWidth() - DPI(80)));
        dropdown_.SetRect(canvas.left + (canvas.GetWidth() - cx) / 2,
                          canvas.top + max(0, (canvas.GetHeight() - DPI(36)) / 2),
                          cx, DPI(36));
    }

    void ApplyDemoTheme() override
    {
        ApplyConfig();
    }

private:
    struct Config {
        bool enabled = true;
        bool multi = false;
        bool face_enabled = true;
        Color face = White();
        bool frame_enabled = true;
        int frame_width = 1;
        Color frame = Color(215, 219, 226);
        Color ink = Color(17, 24, 39);
        Color icon = Color(75, 85, 99);
        int font_height = 11;
        int margin_x = 10;
        int margin_y = 5;
        UiAlign align_h = UiAlign::LEFT;
        bool indicator = true;
        UiAlign indicator_side = UiAlign::RIGHT;
        int indicator_size = 16;
        int content_gap = 6;
        bool focus = true;
        bool shadow = false;
        bool highlight = false;

        int popup_max_height = 260;
        int popup_min_width = 180;
        int popup_item_height = 32;
        int popup_spacing = 0;
        bool popup_scrollbar = true;
        int popup_space = 5;
        Color popup_face = White();
        int popup_frame_width = 1;
        int popup_radius = 8;
        Color popup_frame = Color(215, 219, 226);

        bool item_face_enabled = true;
        Color item_face = White();
        Color item_hot = Color(245, 247, 250);
        Color item_selected = Color(232, 242, 255);
        bool item_frame_enabled = false;
        int item_frame_width = 0;
        Color item_frame = Null;
        Color item_ink = Color(17, 24, 39);
        int item_font_height = 11;
        int item_margin_x = 8;
        int item_margin_y = 5;

        bool marker = true;
        UiAlign marker_side = UiAlign::RIGHT;
        bool badge = true;
        int badge_radius = 10;
        Color badge_face = Color(65, 126, 232);
        Color badge_ink = White();

        bool drag_reorder = false;
        bool drag_handle = true;
        UiAlign drag_side = UiAlign::RIGHT;
        int drag_size = 14;
        int drag_gap = 6;
    } cfg_;

    UiDropdown dropdown_;
    UiBoxLayout face_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout frame_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout ink_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout icon_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout typography_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout margin_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout layout_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout indicator_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout focus_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout shadow_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout highlight_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout popup_layout_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout popup_face_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout popup_frame_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout item_face_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout item_frame_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout item_ink_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout item_type_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout item_margin_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout marker_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout badge_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout drag_box_ { UiBoxLayout::Direction::V };

    DemoToggleRow enabled_row_, multi_row_, face_enabled_row_, frame_enabled_row_, indicator_row_, focus_row_, shadow_row_, highlight_row_;
    DemoToggleRow popup_scrollbar_row_, item_face_enabled_row_, item_frame_enabled_row_, marker_row_, badge_row_, drag_reorder_row_, drag_handle_row_;
    DemoColorRow face_row_, frame_row_, ink_row_, icon_row_, popup_face_row_, popup_frame_row_;
    DemoColorRow item_face_row_, item_hot_row_, item_selected_row_, item_frame_row_, item_ink_row_, badge_face_row_, badge_ink_row_;
    DemoSliderRow frame_width_row_, font_row_, margin_x_row_, margin_y_row_, content_gap_row_, indicator_size_row_;
    DemoSliderRow popup_max_height_row_, popup_min_width_row_, popup_item_height_row_, popup_spacing_row_, popup_space_row_, popup_frame_width_row_, popup_radius_row_;
    DemoSliderRow item_frame_width_row_, item_font_row_, item_margin_x_row_, item_margin_y_row_, badge_radius_row_, drag_size_row_, drag_gap_row_;
    DemoDropdownRow align_row_, indicator_side_row_, marker_side_row_, drag_side_row_;

    void AddSection(const char *name, UiBoxLayout& box)
    {
        box.SetGap(DPI(5)).SetInset(0);
        int q = InspectorAccordion().AddSection(name, false);
        InspectorAccordion().GetSectionContent(q).Add(box.SizePos());
    }

    static void AddColor(UiBoxLayout& box, DemoColorRow& row, const char *label)
    {
        row.SetLabel(label).SetColorCount(1).SetValueSelectable(false);
        box.Add(row).Fit();
    }

    static void AddChoice(UiBoxLayout& box, DemoDropdownRow& row, const char *label)
    {
        row.SetLabel(label);
        box.Add(row).Fit();
    }

    void BuildSections()
    {
        AddSection("FACE", face_box_);
        AddSection("FRAME", frame_box_);
        AddSection("INK", ink_box_);
        AddSection("ICON", icon_box_);
        AddSection("TYPOGRAPHY", typography_box_);
        AddSection("CONTENT MARGIN", margin_box_);
        AddSection("LAYOUT", layout_box_);
        AddSection("INDICATOR", indicator_box_);
        AddSection("FOCUS", focus_box_);
        AddSection("SHADOW", shadow_box_);
        AddSection("HIGHLIGHT", highlight_box_);
        AddSection("POPUP/LAYOUT", popup_layout_box_);
        AddSection("POPUP/FACE", popup_face_box_);
        AddSection("POPUP/FRAME", popup_frame_box_);
        AddSection("POPUP/ITEMS/FACE", item_face_box_);
        AddSection("POPUP/ITEMS/FRAME", item_frame_box_);
        AddSection("POPUP/ITEMS/INK", item_ink_box_);
        AddSection("POPUP/ITEMS/TYPOGRAPHY", item_type_box_);
        AddSection("POPUP/ITEMS/CONTENT MARGIN", item_margin_box_);
        AddSection("POPUP/MARKER", marker_box_);
        AddSection("POPUP/BADGE", badge_box_);
        AddSection("DRAG", drag_box_);
    }

    void BuildRows()
    {
        AddToggleRow(PropsBox(), enabled_row_, "Enabled");
        AddToggleRow(PropsBox(), multi_row_, "Multi select");

        AddToggleRow(face_box_, face_enabled_row_, "Enabled");
        AddColor(face_box_, face_row_, "Normal");
        AddToggleRow(frame_box_, frame_enabled_row_, "Enabled");
        AddSliderRow(frame_box_, frame_width_row_, "Width", "1 px"); frame_width_row_.Slider().SetRange(0, 8).SetStep(1);
        AddColor(frame_box_, frame_row_, "Normal");
        AddColor(ink_box_, ink_row_, "Normal");
        AddColor(icon_box_, icon_row_, "Normal");
        AddSliderRow(typography_box_, font_row_, "Font height", "11 px"); font_row_.Slider().SetRange(8, 28).SetStep(1);
        AddSliderRow(margin_box_, margin_x_row_, "Horizontal", "10 px"); margin_x_row_.Slider().SetRange(0, 32).SetStep(1);
        AddSliderRow(margin_box_, margin_y_row_, "Vertical", "5 px"); margin_y_row_.Slider().SetRange(0, 24).SetStep(1);

        AddChoice(layout_box_, align_row_, "Horizontal align"); align_row_.Add("Left", 0).Add("Center", 1).Add("Right", 2);
        AddSliderRow(layout_box_, content_gap_row_, "Content gap", "6 px"); content_gap_row_.Slider().SetRange(0, 24).SetStep(1);
        AddToggleRow(indicator_box_, indicator_row_, "Show indicator");
        AddChoice(indicator_box_, indicator_side_row_, "Side"); indicator_side_row_.Add("Left", 0).Add("Right", 1);
        AddSliderRow(indicator_box_, indicator_size_row_, "Size", "16 px"); indicator_size_row_.Slider().SetRange(8, 40).SetStep(1);
        AddToggleRow(focus_box_, focus_row_, "Enabled");
        AddToggleRow(shadow_box_, shadow_row_, "Enabled");
        AddToggleRow(highlight_box_, highlight_row_, "Enabled");

        AddSliderRow(popup_layout_box_, popup_max_height_row_, "Max height", "260 px"); popup_max_height_row_.Slider().SetRange(80, 800).SetStep(1);
        AddSliderRow(popup_layout_box_, popup_min_width_row_, "Min width", "180 px"); popup_min_width_row_.Slider().SetRange(80, 600).SetStep(1);
        AddSliderRow(popup_layout_box_, popup_item_height_row_, "Item height", "32 px"); popup_item_height_row_.Slider().SetRange(18, 64).SetStep(1);
        AddSliderRow(popup_layout_box_, popup_spacing_row_, "Item spacing", "0 px"); popup_spacing_row_.Slider().SetRange(0, 20).SetStep(1);
        AddSliderRow(popup_layout_box_, popup_space_row_, "Popup gap", "5 px"); popup_space_row_.Slider().SetRange(0, 24).SetStep(1);
        AddToggleRow(popup_layout_box_, popup_scrollbar_row_, "Show scrollbar");

        AddColor(popup_face_box_, popup_face_row_, "Background");
        AddSliderRow(popup_frame_box_, popup_frame_width_row_, "Width", "1 px"); popup_frame_width_row_.Slider().SetRange(0, 8).SetStep(1);
        AddSliderRow(popup_frame_box_, popup_radius_row_, "Radius", "8 px"); popup_radius_row_.Slider().SetRange(0, 32).SetStep(1);
        AddColor(popup_frame_box_, popup_frame_row_, "Colour");

        AddToggleRow(item_face_box_, item_face_enabled_row_, "Enabled");
        AddColor(item_face_box_, item_face_row_, "Normal");
        AddColor(item_face_box_, item_hot_row_, "Hot");
        AddColor(item_face_box_, item_selected_row_, "Selected");
        AddToggleRow(item_frame_box_, item_frame_enabled_row_, "Enabled");
        AddSliderRow(item_frame_box_, item_frame_width_row_, "Width", "0 px"); item_frame_width_row_.Slider().SetRange(0, 8).SetStep(1);
        AddColor(item_frame_box_, item_frame_row_, "Normal");
        AddColor(item_ink_box_, item_ink_row_, "Normal");
        AddSliderRow(item_type_box_, item_font_row_, "Font height", "11 px"); item_font_row_.Slider().SetRange(8, 28).SetStep(1);
        AddSliderRow(item_margin_box_, item_margin_x_row_, "Horizontal", "8 px"); item_margin_x_row_.Slider().SetRange(0, 32).SetStep(1);
        AddSliderRow(item_margin_box_, item_margin_y_row_, "Vertical", "5 px"); item_margin_y_row_.Slider().SetRange(0, 24).SetStep(1);

        AddToggleRow(marker_box_, marker_row_, "Show selection marker");
        AddChoice(marker_box_, marker_side_row_, "Side"); marker_side_row_.Add("Left", 0).Add("Right", 1);
        AddToggleRow(badge_box_, badge_row_, "Show selection badge");
        AddSliderRow(badge_box_, badge_radius_row_, "Radius", "10 px"); badge_radius_row_.Slider().SetRange(0, 40).SetStep(1);
        AddColor(badge_box_, badge_face_row_, "Face");
        AddColor(badge_box_, badge_ink_row_, "Ink");

        AddToggleRow(drag_box_, drag_reorder_row_, "Enable reorder");
        AddToggleRow(drag_box_, drag_handle_row_, "Show handle");
        AddChoice(drag_box_, drag_side_row_, "Side"); drag_side_row_.Add("Left", 0).Add("Right", 1);
        AddSliderRow(drag_box_, drag_size_row_, "Handle size", "14 px"); drag_size_row_.Slider().SetRange(8, 32).SetStep(1);
        AddSliderRow(drag_box_, drag_gap_row_, "Gap", "6 px"); drag_gap_row_.Slider().SetRange(0, 24).SetStep(1);
    }

    void Populate()
    {
        dropdown_.UseInternalModel();
        dropdown_.ClearModel();
        for(int i = 0; i < 14; i++) {
            UiModelItem item(Format("Choice %d", i + 1), i);
            item.description = i % 2 ? "Secondary choice" : "Primary choice";
            item.right_text = i % 5 == 0 ? "NEW" : String();
            item.icon = ICON_CONTENT_CONTENT_COPY_48();
            item.icon_render_mode = UiIconRenderMode::MonoTint;
            item.has_check = true;
            item.checked = (i % 4) == 0;
            dropdown_.Add(item);
        }
        dropdown_.Select(0);
    }

    void Connect()
    {
#define BIND_TOGGLE(row, field) row.WhenAction = [=] { cfg_.field = (bool)row.Toggle().GetData(); ApplyConfig(); }
#define BIND_COLOR(row, field) row.WhenAction = [=] { cfg_.field = row.GetColor(0); ApplyConfig(); }
#define BIND_SLIDER(row, field) row.WhenAction = [=] { cfg_.field = (int)row.GetData(); ApplyConfig(); }
        BIND_TOGGLE(enabled_row_, enabled); BIND_TOGGLE(multi_row_, multi);
        BIND_TOGGLE(face_enabled_row_, face_enabled); BIND_COLOR(face_row_, face);
        BIND_TOGGLE(frame_enabled_row_, frame_enabled); BIND_SLIDER(frame_width_row_, frame_width); BIND_COLOR(frame_row_, frame);
        BIND_COLOR(ink_row_, ink); BIND_COLOR(icon_row_, icon); BIND_SLIDER(font_row_, font_height);
        BIND_SLIDER(margin_x_row_, margin_x); BIND_SLIDER(margin_y_row_, margin_y); BIND_SLIDER(content_gap_row_, content_gap);
        BIND_TOGGLE(indicator_row_, indicator); BIND_SLIDER(indicator_size_row_, indicator_size);
        BIND_TOGGLE(focus_row_, focus); BIND_TOGGLE(shadow_row_, shadow); BIND_TOGGLE(highlight_row_, highlight);
        BIND_SLIDER(popup_max_height_row_, popup_max_height); BIND_SLIDER(popup_min_width_row_, popup_min_width); BIND_SLIDER(popup_item_height_row_, popup_item_height); BIND_SLIDER(popup_spacing_row_, popup_spacing); BIND_SLIDER(popup_space_row_, popup_space); BIND_TOGGLE(popup_scrollbar_row_, popup_scrollbar);
        BIND_COLOR(popup_face_row_, popup_face); BIND_SLIDER(popup_frame_width_row_, popup_frame_width); BIND_SLIDER(popup_radius_row_, popup_radius); BIND_COLOR(popup_frame_row_, popup_frame);
        BIND_TOGGLE(item_face_enabled_row_, item_face_enabled); BIND_COLOR(item_face_row_, item_face); BIND_COLOR(item_hot_row_, item_hot); BIND_COLOR(item_selected_row_, item_selected);
        BIND_TOGGLE(item_frame_enabled_row_, item_frame_enabled); BIND_SLIDER(item_frame_width_row_, item_frame_width); BIND_COLOR(item_frame_row_, item_frame); BIND_COLOR(item_ink_row_, item_ink); BIND_SLIDER(item_font_row_, item_font_height); BIND_SLIDER(item_margin_x_row_, item_margin_x); BIND_SLIDER(item_margin_y_row_, item_margin_y);
        BIND_TOGGLE(marker_row_, marker); BIND_TOGGLE(badge_row_, badge); BIND_SLIDER(badge_radius_row_, badge_radius); BIND_COLOR(badge_face_row_, badge_face); BIND_COLOR(badge_ink_row_, badge_ink);
        BIND_TOGGLE(drag_reorder_row_, drag_reorder); BIND_TOGGLE(drag_handle_row_, drag_handle); BIND_SLIDER(drag_size_row_, drag_size); BIND_SLIDER(drag_gap_row_, drag_gap);
#undef BIND_TOGGLE
#undef BIND_COLOR
#undef BIND_SLIDER
        align_row_.WhenSelect = [=](int) { int q = (int)align_row_.Dropdown().GetSelectedData(); cfg_.align_h = q == 1 ? UiAlign::CENTER : q == 2 ? UiAlign::RIGHT : UiAlign::LEFT; ApplyConfig(); };
        indicator_side_row_.WhenSelect = [=](int) { cfg_.indicator_side = (int)indicator_side_row_.Dropdown().GetSelectedData() == 0 ? UiAlign::LEFT : UiAlign::RIGHT; ApplyConfig(); };
        marker_side_row_.WhenSelect = [=](int) { cfg_.marker_side = (int)marker_side_row_.Dropdown().GetSelectedData() == 0 ? UiAlign::LEFT : UiAlign::RIGHT; ApplyConfig(); };
        drag_side_row_.WhenSelect = [=](int) { cfg_.drag_side = (int)drag_side_row_.Dropdown().GetSelectedData() == 0 ? UiAlign::LEFT : UiAlign::RIGHT; ApplyConfig(); };
    }

    void SyncRows()
    {
        enabled_row_.SetData(cfg_.enabled); multi_row_.SetData(cfg_.multi); face_enabled_row_.SetData(cfg_.face_enabled); face_row_.SetColor(0, cfg_.face);
        frame_enabled_row_.SetData(cfg_.frame_enabled); frame_width_row_.SetData(cfg_.frame_width); frame_row_.SetColor(0, cfg_.frame); ink_row_.SetColor(0, cfg_.ink); icon_row_.SetColor(0, cfg_.icon); font_row_.SetData(cfg_.font_height);
        margin_x_row_.SetData(cfg_.margin_x); margin_y_row_.SetData(cfg_.margin_y); align_row_.SelectByData(cfg_.align_h == UiAlign::CENTER ? 1 : cfg_.align_h == UiAlign::RIGHT ? 2 : 0); content_gap_row_.SetData(cfg_.content_gap);
        indicator_row_.SetData(cfg_.indicator); indicator_side_row_.SelectByData(cfg_.indicator_side == UiAlign::LEFT ? 0 : 1); indicator_size_row_.SetData(cfg_.indicator_size); focus_row_.SetData(cfg_.focus); shadow_row_.SetData(cfg_.shadow); highlight_row_.SetData(cfg_.highlight);
        popup_max_height_row_.SetData(cfg_.popup_max_height); popup_min_width_row_.SetData(cfg_.popup_min_width); popup_item_height_row_.SetData(cfg_.popup_item_height); popup_spacing_row_.SetData(cfg_.popup_spacing); popup_space_row_.SetData(cfg_.popup_space); popup_scrollbar_row_.SetData(cfg_.popup_scrollbar); popup_face_row_.SetColor(0, cfg_.popup_face); popup_frame_width_row_.SetData(cfg_.popup_frame_width); popup_radius_row_.SetData(cfg_.popup_radius); popup_frame_row_.SetColor(0, cfg_.popup_frame);
        item_face_enabled_row_.SetData(cfg_.item_face_enabled); item_face_row_.SetColor(0, cfg_.item_face); item_hot_row_.SetColor(0, cfg_.item_hot); item_selected_row_.SetColor(0, cfg_.item_selected); item_frame_enabled_row_.SetData(cfg_.item_frame_enabled); item_frame_width_row_.SetData(cfg_.item_frame_width); item_frame_row_.SetColor(0, cfg_.item_frame); item_ink_row_.SetColor(0, cfg_.item_ink); item_font_row_.SetData(cfg_.item_font_height); item_margin_x_row_.SetData(cfg_.item_margin_x); item_margin_y_row_.SetData(cfg_.item_margin_y);
        marker_row_.SetData(cfg_.marker); marker_side_row_.SelectByData(cfg_.marker_side == UiAlign::LEFT ? 0 : 1); badge_row_.SetData(cfg_.badge); badge_radius_row_.SetData(cfg_.badge_radius); badge_face_row_.SetColor(0, cfg_.badge_face); badge_ink_row_.SetColor(0, cfg_.badge_ink); drag_reorder_row_.SetData(cfg_.drag_reorder); drag_handle_row_.SetData(cfg_.drag_handle); drag_side_row_.SelectByData(cfg_.drag_side == UiAlign::LEFT ? 0 : 1); drag_size_row_.SetData(cfg_.drag_size); drag_gap_row_.SetData(cfg_.drag_gap);
#define SETPX(row, field) row.SetValueText(Format("%d px", cfg_.field))
        SETPX(frame_width_row_, frame_width); SETPX(font_row_, font_height); SETPX(margin_x_row_, margin_x); SETPX(margin_y_row_, margin_y); SETPX(content_gap_row_, content_gap); SETPX(indicator_size_row_, indicator_size); SETPX(popup_max_height_row_, popup_max_height); SETPX(popup_min_width_row_, popup_min_width); SETPX(popup_item_height_row_, popup_item_height); SETPX(popup_spacing_row_, popup_spacing); SETPX(popup_space_row_, popup_space); SETPX(popup_frame_width_row_, popup_frame_width); SETPX(popup_radius_row_, popup_radius); SETPX(item_frame_width_row_, item_frame_width); SETPX(item_font_row_, item_font_height); SETPX(item_margin_x_row_, item_margin_x); SETPX(item_margin_y_row_, item_margin_y); SETPX(badge_radius_row_, badge_radius); SETPX(drag_size_row_, drag_size); SETPX(drag_gap_row_, drag_gap);
#undef SETPX
    }

    void ApplyConfig()
    {
        SyncRows();
        UiDropdown::Style s = UiTheme::ResolveDropdown(UiRole::Standard);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(cfg_.face);
            s.palette.frame[i] = cfg_.frame;
            s.palette.ink[i] = cfg_.ink;
            s.palette.icon[i] = cfg_.icon;
        }
        s.metrics.face_enabled = cfg_.face_enabled;
        s.metrics.frame_enabled = cfg_.frame_enabled;
        s.metrics.frame_width = cfg_.frame_width;
        s.metrics.content_margin = Rect(cfg_.margin_x, cfg_.margin_y, cfg_.margin_x, cfg_.margin_y);
        s.metrics.focus_enabled = cfg_.focus;
        s.metrics.shadow.enabled = cfg_.shadow;
        s.metrics.highlight.enabled = cfg_.highlight;
        s.font.Height(cfg_.font_height);
        s.align_h = cfg_.align_h;
        s.content_gap = cfg_.content_gap;
        s.show_indicator = cfg_.indicator;
        s.indicator_side = cfg_.indicator_side;
        s.indicator_size = cfg_.indicator_size;

        s.popup_max_height = cfg_.popup_max_height;
        s.popup_min_width = cfg_.popup_min_width;
        s.popup_item_height = cfg_.popup_item_height;
        s.item_spacing = cfg_.popup_spacing;
        s.popup_show_scrollbar = cfg_.popup_scrollbar;
        s.popup_space = cfg_.popup_space;
        s.popup_background_color = cfg_.popup_face;
        s.popup_frame_width = cfg_.popup_frame_width;
        s.popup_radius = cfg_.popup_radius;
        s.popup_frame_color = cfg_.popup_frame;

        s.popup_item_style.metrics.face_enabled = cfg_.item_face_enabled;
        s.popup_item_style.metrics.frame_enabled = cfg_.item_frame_enabled;
        s.popup_item_style.metrics.frame_width = cfg_.item_frame_width;
        s.popup_item_style.metrics.content_margin = Rect(cfg_.item_margin_x, cfg_.item_margin_y, cfg_.item_margin_x, cfg_.item_margin_y);
        s.popup_item_style.font.Height(cfg_.item_font_height);
        for(int i = 0; i < 4; i++) {
            s.popup_item_style.palette.face[i] = UiFill::Solid(cfg_.item_face);
            s.popup_item_style.palette.frame[i] = cfg_.item_frame;
            s.popup_item_style.palette.ink[i] = cfg_.item_ink;
        }
        s.popup_item_style.palette.face[ST_HOT] = UiFill::Solid(cfg_.item_hot);
        s.popup_item_style.palette.face[ST_PRESSED] = UiFill::Solid(cfg_.item_selected);

        s.show_popup_selection_marker = cfg_.marker;
        s.popup_marker_side = cfg_.marker_side;
        s.show_selection_badge = cfg_.badge;
        s.selection_badge_radius = cfg_.badge_radius;
        s.selection_badge_face = cfg_.badge_face;
        s.selection_badge_ink = cfg_.badge_ink;
        s.show_drag_handle = cfg_.drag_handle;
        s.drag_side = cfg_.drag_side;
        s.drag_size = cfg_.drag_size;
        s.drag_gap = cfg_.drag_gap;

        dropdown_.SetCustomStyle(s);
        dropdown_.Enable(cfg_.enabled);
        dropdown_.SetMultiSelect(cfg_.multi);
        dropdown_.EnableDragReorder(cfg_.drag_reorder);
        dropdown_.ShowDragHandle(cfg_.drag_handle);
        dropdown_.SetDragSide(cfg_.drag_side);

        SetUsageCode("UiDropdown::Style style = UiTheme::ResolveDropdown(UiRole::Standard);\n"
                     "// Collapsed control chrome and Popup/* remain distinct ownership domains.\n"
                     "dropdown.SetCustomStyle(style);\n");
        Refresh();
    }
};

} // namespace

GUI_APP_MAIN
{
    UiDropdownDemoWindow().Run();
}
