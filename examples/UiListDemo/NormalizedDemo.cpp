/*
    UiList normalized ownership-reference demo.
    The previous large builder remains in main.cpp for historical reference and
    is intentionally not compiled by UiListDemo.upp.
*/
#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

class UiListDemoWindow : public BuilderWindowBase {
public:
    typedef UiListDemoWindow CLASSNAME;

    UiListDemoWindow()
        : BuilderWindowBase("UiList Demo", "UiList",
                            "Viewport chrome and List-owned Rows/*, Content, Badge and Drag reference")
    {
        Preview().Add(list_);
        BuildSections();
        BuildRows();
        Populate();
        Connect();
        FinishInit();
    }

protected:
    void LayoutPreviewContent() override
    {
        Rect rc = Preview().GetCanvasRect().Deflated(DPI(28), DPI(28));
        list_.SetRect(rc);
    }

    void ApplyDemoTheme() override
    {
        ApplyConfig();
    }

private:
    struct Config {
        bool enabled = true;
        bool rename = true;
        bool multi = false;
        bool face_enabled = true;
        Color face = White();
        bool frame_enabled = true;
        int frame_width = 1;
        Color frame = Color(215, 219, 226);
        Color ink = Color(17, 24, 39);
        Color icon = Color(75, 85, 99);
        int icon_size = 16;
        int font_height = 11;
        int margin_x = 0;
        int margin_y = 0;
        bool focus = true;
        bool shadow = false;
        bool highlight = false;
        int row_height = 30;
        int row_radius = 5;
        int row_pad_x = 8;
        int row_pad_y = 5;
        Color hot_face = Color(245, 247, 250);
        Color selected_face = Color(232, 242, 255);
        bool striped = false;
        Color even_face = White();
        Color odd_face = Color(248, 250, 252);
        bool show_checks = true;
        bool show_metadata = true;
        bool show_separator = true;
        bool badge = true;
        Color badge_face = Color(241, 245, 249);
        Color badge_ink = Color(51, 65, 85);
        bool drag_reorder = true;
        bool drag_handle = true;
        UiAlign drag_side = UiAlign::RIGHT;
        int drag_size = 14;
        int drag_gap = 6;
    } cfg_;

    UiList list_;
    UiBoxLayout face_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout frame_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout ink_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout icon_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout typography_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout margin_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout focus_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout shadow_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout highlight_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout rows_layout_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout rows_state_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout content_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout badge_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout drag_box_ { UiBoxLayout::Direction::V };

    DemoToggleRow enabled_row_, rename_row_, multi_row_;
    DemoToggleRow face_enabled_row_, frame_enabled_row_, focus_row_, shadow_row_, highlight_row_;
    DemoToggleRow striped_row_, checks_row_, metadata_row_, separator_row_, badge_row_, drag_reorder_row_, drag_handle_row_;
    DemoColorRow face_row_, frame_row_, ink_row_, icon_row_, hot_row_, selected_row_, even_row_, odd_row_, badge_face_row_, badge_ink_row_;
    DemoSliderRow frame_width_row_, icon_size_row_, font_row_, margin_x_row_, margin_y_row_;
    DemoSliderRow row_height_row_, row_radius_row_, row_pad_x_row_, row_pad_y_row_, drag_size_row_, drag_gap_row_;
    DemoDropdownRow drag_side_row_;

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

    void BuildSections()
    {
        AddSection("FACE", face_box_);
        AddSection("FRAME", frame_box_);
        AddSection("INK", ink_box_);
        AddSection("ICON", icon_box_);
        AddSection("TYPOGRAPHY", typography_box_);
        AddSection("CONTENT MARGIN", margin_box_);
        AddSection("FOCUS", focus_box_);
        AddSection("SHADOW", shadow_box_);
        AddSection("HIGHLIGHT", highlight_box_);
        AddSection("ROWS/LAYOUT", rows_layout_box_);
        AddSection("ROWS/STATE", rows_state_box_);
        AddSection("CONTENT", content_box_);
        AddSection("BADGE", badge_box_);
        AddSection("DRAG", drag_box_);
    }

    void BuildRows()
    {
        AddToggleRow(PropsBox(), enabled_row_, "Enabled");
        AddToggleRow(PropsBox(), rename_row_, "Rename on double-click");
        AddToggleRow(PropsBox(), multi_row_, "Multi select");

        AddToggleRow(face_box_, face_enabled_row_, "Enabled");
        AddColor(face_box_, face_row_, "Normal");

        AddToggleRow(frame_box_, frame_enabled_row_, "Enabled");
        AddSliderRow(frame_box_, frame_width_row_, "Width", "1 px");
        frame_width_row_.Slider().SetRange(0, 8).SetStep(1);
        AddColor(frame_box_, frame_row_, "Normal");

        AddColor(ink_box_, ink_row_, "Normal");
        AddColor(icon_box_, icon_row_, "Normal");
        AddSliderRow(icon_box_, icon_size_row_, "Size", "16 px");
        icon_size_row_.Slider().SetRange(8, 40).SetStep(1);

        AddSliderRow(typography_box_, font_row_, "Font height", "11 px");
        font_row_.Slider().SetRange(8, 28).SetStep(1);

        AddSliderRow(margin_box_, margin_x_row_, "Horizontal", "0 px");
        margin_x_row_.Slider().SetRange(0, 24).SetStep(1);
        AddSliderRow(margin_box_, margin_y_row_, "Vertical", "0 px");
        margin_y_row_.Slider().SetRange(0, 24).SetStep(1);

        AddToggleRow(focus_box_, focus_row_, "Enabled");
        AddToggleRow(shadow_box_, shadow_row_, "Enabled");
        AddToggleRow(highlight_box_, highlight_row_, "Enabled");

        AddSliderRow(rows_layout_box_, row_height_row_, "Height", "30 px");
        row_height_row_.Slider().SetRange(18, 64).SetStep(1);
        AddSliderRow(rows_layout_box_, row_radius_row_, "Radius", "5 px");
        row_radius_row_.Slider().SetRange(0, 24).SetStep(1);
        AddSliderRow(rows_layout_box_, row_pad_x_row_, "Horizontal padding", "8 px");
        row_pad_x_row_.Slider().SetRange(0, 32).SetStep(1);
        AddSliderRow(rows_layout_box_, row_pad_y_row_, "Vertical padding", "5 px");
        row_pad_y_row_.Slider().SetRange(0, 24).SetStep(1);

        AddColor(rows_state_box_, hot_row_, "Hot face");
        AddColor(rows_state_box_, selected_row_, "Selected face");
        AddToggleRow(rows_state_box_, striped_row_, "Striped rows");
        AddColor(rows_state_box_, even_row_, "Even face");
        AddColor(rows_state_box_, odd_row_, "Odd face");

        AddToggleRow(content_box_, checks_row_, "Show checks");
        AddToggleRow(content_box_, metadata_row_, "Show metadata");
        AddToggleRow(content_box_, separator_row_, "Show separators");

        AddToggleRow(badge_box_, badge_row_, "Right text as badge");
        AddColor(badge_box_, badge_face_row_, "Face");
        AddColor(badge_box_, badge_ink_row_, "Ink");

        AddToggleRow(drag_box_, drag_reorder_row_, "Enable reorder");
        AddToggleRow(drag_box_, drag_handle_row_, "Show handle");
        drag_side_row_.SetLabel("Side").Add("Left", 0).Add("Right", 1);
        drag_box_.Add(drag_side_row_).Fit();
        AddSliderRow(drag_box_, drag_size_row_, "Handle size", "14 px");
        drag_size_row_.Slider().SetRange(8, 32).SetStep(1);
        AddSliderRow(drag_box_, drag_gap_row_, "Gap", "6 px");
        drag_gap_row_.Slider().SetRange(0, 24).SetStep(1);
    }

    void Populate()
    {
        list_.UseInternalModel();
        list_.ClearModel();
        for(int i = 0; i < 12; i++) {
            UiModelItem item(Format("Item %d", i + 1), i);
            item.description = "Model-backed row";
            item.right_text = i % 4 == 0 ? "NEW" : Format("%d", i + 1);
            item.icon = ICON_CONTENT_CONTENT_COPY_48();
            item.icon_render_mode = UiIconRenderMode::MonoTint;
            item.has_check = true;
            item.checked = (i % 3) == 0;
            item.has_metadata = true;
            item.metadata_color = Color(65, 167, 248);
            item.separator_before = i > 0 && (i % 4) == 0;
            list_.Model().Add(item);
        }
        list_.Select(0);
    }

    void Connect()
    {
#define BIND_TOGGLE(row, field) row.WhenAction = [=] { cfg_.field = (bool)row.Toggle().GetData(); ApplyConfig(); }
#define BIND_COLOR(row, field) row.WhenAction = [=] { cfg_.field = row.GetColor(0); ApplyConfig(); }
#define BIND_SLIDER(row, field) row.WhenAction = [=] { cfg_.field = (int)row.GetData(); ApplyConfig(); }
        BIND_TOGGLE(enabled_row_, enabled);
        BIND_TOGGLE(rename_row_, rename);
        BIND_TOGGLE(multi_row_, multi);
        BIND_TOGGLE(face_enabled_row_, face_enabled);
        BIND_COLOR(face_row_, face);
        BIND_TOGGLE(frame_enabled_row_, frame_enabled);
        BIND_SLIDER(frame_width_row_, frame_width);
        BIND_COLOR(frame_row_, frame);
        BIND_COLOR(ink_row_, ink);
        BIND_COLOR(icon_row_, icon);
        BIND_SLIDER(icon_size_row_, icon_size);
        BIND_SLIDER(font_row_, font_height);
        BIND_SLIDER(margin_x_row_, margin_x);
        BIND_SLIDER(margin_y_row_, margin_y);
        BIND_TOGGLE(focus_row_, focus);
        BIND_TOGGLE(shadow_row_, shadow);
        BIND_TOGGLE(highlight_row_, highlight);
        BIND_SLIDER(row_height_row_, row_height);
        BIND_SLIDER(row_radius_row_, row_radius);
        BIND_SLIDER(row_pad_x_row_, row_pad_x);
        BIND_SLIDER(row_pad_y_row_, row_pad_y);
        BIND_COLOR(hot_row_, hot_face);
        BIND_COLOR(selected_row_, selected_face);
        BIND_TOGGLE(striped_row_, striped);
        BIND_COLOR(even_row_, even_face);
        BIND_COLOR(odd_row_, odd_face);
        BIND_TOGGLE(checks_row_, show_checks);
        BIND_TOGGLE(metadata_row_, show_metadata);
        BIND_TOGGLE(separator_row_, show_separator);
        BIND_TOGGLE(badge_row_, badge);
        BIND_COLOR(badge_face_row_, badge_face);
        BIND_COLOR(badge_ink_row_, badge_ink);
        BIND_TOGGLE(drag_reorder_row_, drag_reorder);
        BIND_TOGGLE(drag_handle_row_, drag_handle);
        BIND_SLIDER(drag_size_row_, drag_size);
        BIND_SLIDER(drag_gap_row_, drag_gap);
#undef BIND_TOGGLE
#undef BIND_COLOR
#undef BIND_SLIDER
        drag_side_row_.WhenSelect = [=](int) {
            cfg_.drag_side = (int)drag_side_row_.Dropdown().GetSelectedData() == 0 ? UiAlign::LEFT : UiAlign::RIGHT;
            ApplyConfig();
        };
    }

    void SyncRows()
    {
        enabled_row_.SetData(cfg_.enabled);
        rename_row_.SetData(cfg_.rename);
        multi_row_.SetData(cfg_.multi);
        face_enabled_row_.SetData(cfg_.face_enabled); face_row_.SetColor(0, cfg_.face);
        frame_enabled_row_.SetData(cfg_.frame_enabled); frame_width_row_.SetData(cfg_.frame_width); frame_row_.SetColor(0, cfg_.frame);
        ink_row_.SetColor(0, cfg_.ink); icon_row_.SetColor(0, cfg_.icon); icon_size_row_.SetData(cfg_.icon_size);
        font_row_.SetData(cfg_.font_height); margin_x_row_.SetData(cfg_.margin_x); margin_y_row_.SetData(cfg_.margin_y);
        focus_row_.SetData(cfg_.focus); shadow_row_.SetData(cfg_.shadow); highlight_row_.SetData(cfg_.highlight);
        row_height_row_.SetData(cfg_.row_height); row_radius_row_.SetData(cfg_.row_radius); row_pad_x_row_.SetData(cfg_.row_pad_x); row_pad_y_row_.SetData(cfg_.row_pad_y);
        hot_row_.SetColor(0, cfg_.hot_face); selected_row_.SetColor(0, cfg_.selected_face); striped_row_.SetData(cfg_.striped); even_row_.SetColor(0, cfg_.even_face); odd_row_.SetColor(0, cfg_.odd_face);
        checks_row_.SetData(cfg_.show_checks); metadata_row_.SetData(cfg_.show_metadata); separator_row_.SetData(cfg_.show_separator);
        badge_row_.SetData(cfg_.badge); badge_face_row_.SetColor(0, cfg_.badge_face); badge_ink_row_.SetColor(0, cfg_.badge_ink);
        drag_reorder_row_.SetData(cfg_.drag_reorder); drag_handle_row_.SetData(cfg_.drag_handle); drag_side_row_.SelectByData(cfg_.drag_side == UiAlign::LEFT ? 0 : 1); drag_size_row_.SetData(cfg_.drag_size); drag_gap_row_.SetData(cfg_.drag_gap);
        frame_width_row_.SetValueText(Format("%d px", cfg_.frame_width)); icon_size_row_.SetValueText(Format("%d px", cfg_.icon_size)); font_row_.SetValueText(Format("%d px", cfg_.font_height));
        margin_x_row_.SetValueText(Format("%d px", cfg_.margin_x)); margin_y_row_.SetValueText(Format("%d px", cfg_.margin_y)); row_height_row_.SetValueText(Format("%d px", cfg_.row_height)); row_radius_row_.SetValueText(Format("%d px", cfg_.row_radius));
        row_pad_x_row_.SetValueText(Format("%d px", cfg_.row_pad_x)); row_pad_y_row_.SetValueText(Format("%d px", cfg_.row_pad_y)); drag_size_row_.SetValueText(Format("%d px", cfg_.drag_size)); drag_gap_row_.SetValueText(Format("%d px", cfg_.drag_gap));
    }

    void ApplyConfig()
    {
        SyncRows();
        UiList::Style s = UiTheme::ResolveList();
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
        s.icon_size = cfg_.icon_size;
        s.row_height = cfg_.row_height;
        s.row_radius = cfg_.row_radius;
        s.h_padding = cfg_.row_pad_x;
        s.v_padding = cfg_.row_pad_y;
        s.hot_face = cfg_.hot_face;
        s.selected_face = cfg_.selected_face;
        s.striped_rows = cfg_.striped;
        s.row_even_face = cfg_.even_face;
        s.row_odd_face = cfg_.odd_face;
        s.show_checks = cfg_.show_checks;
        s.show_metadata_marker = cfg_.show_metadata;
        s.show_row_separator = cfg_.show_separator;
        s.right_text_as_badge = cfg_.badge;
        s.badge_face = cfg_.badge_face;
        s.badge_ink = cfg_.badge_ink;
        s.show_drag_handle = cfg_.drag_handle;
        s.drag_side = cfg_.drag_side;
        s.drag_size = cfg_.drag_size;
        s.drag_gap = cfg_.drag_gap;

        list_.SetCustomStyle(s);
        list_.Enable(cfg_.enabled);
        list_.SetSelectionMode(cfg_.multi ? UILISTSEL_MULTI : UILISTSEL_SINGLE);
        list_.EnableRenameOnDblClick(cfg_.rename);
        list_.EnableDragReorder(cfg_.drag_reorder);
        list_.ShowDragHandle(cfg_.drag_handle);
        list_.SetDragSide(cfg_.drag_side);

        SetUsageCode("UiList::Style style = UiTheme::ResolveList();\n"
                     "// Viewport Face/Frame stays separate from Rows/*, Content, Badge and Drag.\n"
                     "list.SetCustomStyle(style);\n");
        Refresh();
    }
};

} // namespace

GUI_APP_MAIN
{
    UiListDemoWindow().Run();
}
