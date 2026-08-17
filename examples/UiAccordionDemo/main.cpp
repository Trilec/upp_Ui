/*
    UiAccordionDemo -- canonical Header/*, Body/* and Section ownership reference.
*/
#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

class UiAccordionDemoWindow : public BuilderWindowBase {
public:
    typedef UiAccordionDemoWindow CLASSNAME;

    UiAccordionDemoWindow()
        : BuilderWindowBase("UiAccordion Demo", "UiAccordion",
                            "Outer chrome with distinct Header/*, Body/* and Section style domains")
    {
        Preview().Add(accordion_);
        BuildPreviewSections();
        BuildSections();
        BuildRows();
        Connect();
        FinishInit();
    }

protected:
    void LayoutPreviewContent() override
    {
        Rect rc = Preview().GetCanvasRect().Deflated(DPI(24), DPI(20));
        accordion_.SetRect(rc);
    }

    void ApplyDemoTheme() override
    {
        ApplyConfig();
    }

private:
    struct Config {
        bool face_enabled = false;
        Color face = White();
        bool frame_enabled = false;
        int frame_width = 0;
        Color frame = Color(215, 219, 226);
        Color ink = Color(17, 24, 39);
        int margin_x = 0;
        int margin_y = 0;
        bool shadow = false;
        bool highlight = false;

        int header_height = 34;
        int item_spacing = 6;
        int header_body_gap = 4;
        int body_min_height = 72;

        bool unified_frame = false;
        int unified_radius = 7;
        int unified_frame_width = 1;

        bool header_face_enabled = true;
        Color header_face = Color(247, 248, 250);
        bool header_frame_enabled = true;
        int header_frame_width = 1;
        Color header_frame = Color(215, 219, 226);
        Color header_ink = Color(17, 24, 39);
        int header_font_height = 11;
        int header_radius = 6;
        int header_margin_x = 10;
        int header_margin_y = 6;

        bool show_chevron = true;
        UiAlign chevron_side = UiAlign::RIGHT;
        int chevron_size = 12;
        int chevron_gap = 8;

        bool drag_reorder = true;
        bool show_drag = true;
        UiAlign drag_side = UiAlign::RIGHT;
        int drag_size = 14;
        int drag_gap = 8;

        bool body_transparent = false;
        bool body_face_enabled = true;
        Color body_face = White();
        bool body_frame_enabled = false;
        int body_frame_width = 0;
        Color body_frame = Color(215, 219, 226);
        int body_radius = 0;
        int body_margin_x = 10;
        int body_margin_y = 8;
        UiSpan body_line_extent = NONE;
        UiLineStyle body_line_style = SOLID;
        int body_line_thickness = 1;
        Color body_line_color = Color(215, 219, 226);

        bool single_open = false;
        bool enforce_one = false;
        bool animation = true;
        int anim_open_ms = 120;
        int anim_close_ms = 0;
    } cfg_;

    UiAccordion accordion_;
    UiLabel body_a_, body_b_, body_c_;

    UiBoxLayout face_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout frame_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout ink_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout margin_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout shadow_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout highlight_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout layout_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout section_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout header_face_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout header_frame_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout header_ink_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout header_type_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout header_margin_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout chevron_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout drag_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout body_face_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout body_frame_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout body_margin_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout body_line_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout behaviour_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout animation_box_ { UiBoxLayout::Direction::V };

    DemoToggleRow face_enabled_row_, frame_enabled_row_, shadow_row_, highlight_row_, unified_row_;
    DemoToggleRow header_face_enabled_row_, header_frame_enabled_row_, chevron_row_, drag_reorder_row_, drag_row_;
    DemoToggleRow body_transparent_row_, body_face_enabled_row_, body_frame_enabled_row_, single_row_, enforce_row_, animation_row_;
    DemoSliderRow frame_width_row_, margin_x_row_, margin_y_row_, header_height_row_, spacing_row_, header_body_gap_row_, body_min_height_row_;
    DemoSliderRow unified_radius_row_, unified_frame_width_row_, header_frame_width_row_, header_font_row_, header_radius_row_, header_margin_x_row_, header_margin_y_row_;
    DemoSliderRow chevron_size_row_, chevron_gap_row_, drag_size_row_, drag_gap_row_, body_frame_width_row_, body_radius_row_, body_margin_x_row_, body_margin_y_row_;
    DemoSliderRow body_line_thickness_row_, anim_open_row_, anim_close_row_;
    DemoColorRow face_row_, frame_row_, ink_row_, header_face_row_, header_frame_row_, header_ink_row_, body_face_row_, body_frame_row_, body_line_color_row_;
    DemoDropdownRow chevron_side_row_, drag_side_row_, body_line_extent_row_, body_line_style_row_;

    void BuildPreviewSections()
    {
        body_a_.SetText("Outer Accordion chrome is separate from section Header and Body styles.");
        body_b_.SetText("Header uses UiTitleCard::Style; Body uses UiPanel::Style.");
        body_c_.SetText("Chevron, section line, drag and animation remain Accordion-owned.");

        int a = accordion_.AddSection("Overview", "Outer chrome", "Face / Frame / Section", true);
        int b = accordion_.AddSection("Composition", "Nested styles", "Header / Body", true);
        int c = accordion_.AddSection("Behaviour", "Accordion-owned", "Chevron / Drag / Animation", false);
        accordion_.GetSectionContent(a).Add(body_a_.SizePos());
        accordion_.GetSectionContent(b).Add(body_b_.SizePos());
        accordion_.GetSectionContent(c).Add(body_c_.SizePos());
        accordion_.SetSectionBodyHeight(a, DPI(64));
        accordion_.SetSectionBodyHeight(b, DPI(64));
        accordion_.SetSectionBodyHeight(c, DPI(64));
    }

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
        AddSection("CONTENT MARGIN", margin_box_);
        AddSection("SHADOW", shadow_box_);
        AddSection("HIGHLIGHT", highlight_box_);
        AddSection("LAYOUT", layout_box_);
        AddSection("SECTION", section_box_);
        AddSection("HEADER/FACE", header_face_box_);
        AddSection("HEADER/FRAME", header_frame_box_);
        AddSection("HEADER/INK", header_ink_box_);
        AddSection("HEADER/TYPOGRAPHY", header_type_box_);
        AddSection("HEADER/CONTENT MARGIN", header_margin_box_);
        AddSection("HEADER/CHEVRON", chevron_box_);
        AddSection("HEADER/DRAG", drag_box_);
        AddSection("BODY/FACE", body_face_box_);
        AddSection("BODY/FRAME", body_frame_box_);
        AddSection("BODY/CONTENT MARGIN", body_margin_box_);
        AddSection("BODY/LINE", body_line_box_);
        AddSection("BEHAVIOUR", behaviour_box_);
        AddSection("ANIMATION", animation_box_);
    }

    void BuildRows()
    {
        AddToggleRow(face_box_, face_enabled_row_, "Enabled"); AddColor(face_box_, face_row_, "Normal");
        AddToggleRow(frame_box_, frame_enabled_row_, "Enabled"); AddSliderRow(frame_box_, frame_width_row_, "Width", "0 px"); frame_width_row_.Slider().SetRange(0, 8).SetStep(1); AddColor(frame_box_, frame_row_, "Normal");
        AddColor(ink_box_, ink_row_, "Normal");
        AddSliderRow(margin_box_, margin_x_row_, "Horizontal", "0 px"); margin_x_row_.Slider().SetRange(0, 32).SetStep(1); AddSliderRow(margin_box_, margin_y_row_, "Vertical", "0 px"); margin_y_row_.Slider().SetRange(0, 24).SetStep(1);
        AddToggleRow(shadow_box_, shadow_row_, "Enabled"); AddToggleRow(highlight_box_, highlight_row_, "Enabled");

        AddSliderRow(layout_box_, header_height_row_, "Header height", "34 px"); header_height_row_.Slider().SetRange(22, 72).SetStep(1);
        AddSliderRow(layout_box_, spacing_row_, "Item spacing", "6 px"); spacing_row_.Slider().SetRange(0, 24).SetStep(1);
        AddSliderRow(layout_box_, header_body_gap_row_, "Header/body gap", "4 px"); header_body_gap_row_.Slider().SetRange(0, 24).SetStep(1);
        AddSliderRow(layout_box_, body_min_height_row_, "Body min height", "72 px"); body_min_height_row_.Slider().SetRange(20, 240).SetStep(1);

        AddToggleRow(section_box_, unified_row_, "Unified frame");
        AddSliderRow(section_box_, unified_radius_row_, "Radius", "7 px"); unified_radius_row_.Slider().SetRange(0, 32).SetStep(1);
        AddSliderRow(section_box_, unified_frame_width_row_, "Frame width", "1 px"); unified_frame_width_row_.Slider().SetRange(0, 8).SetStep(1);

        AddToggleRow(header_face_box_, header_face_enabled_row_, "Enabled"); AddColor(header_face_box_, header_face_row_, "Normal");
        AddToggleRow(header_frame_box_, header_frame_enabled_row_, "Enabled"); AddSliderRow(header_frame_box_, header_frame_width_row_, "Width", "1 px"); header_frame_width_row_.Slider().SetRange(0, 8).SetStep(1); AddSliderRow(header_frame_box_, header_radius_row_, "Radius", "6 px"); header_radius_row_.Slider().SetRange(0, 32).SetStep(1); AddColor(header_frame_box_, header_frame_row_, "Normal");
        AddColor(header_ink_box_, header_ink_row_, "Title");
        AddSliderRow(header_type_box_, header_font_row_, "Title font height", "11 px"); header_font_row_.Slider().SetRange(8, 28).SetStep(1);
        AddSliderRow(header_margin_box_, header_margin_x_row_, "Horizontal", "10 px"); header_margin_x_row_.Slider().SetRange(0, 32).SetStep(1); AddSliderRow(header_margin_box_, header_margin_y_row_, "Vertical", "6 px"); header_margin_y_row_.Slider().SetRange(0, 24).SetStep(1);

        AddToggleRow(chevron_box_, chevron_row_, "Show"); AddChoice(chevron_box_, chevron_side_row_, "Side"); chevron_side_row_.Add("Left", 0).Add("Right", 1); AddSliderRow(chevron_box_, chevron_size_row_, "Size", "12 px"); chevron_size_row_.Slider().SetRange(6, 32).SetStep(1); AddSliderRow(chevron_box_, chevron_gap_row_, "Gap", "8 px"); chevron_gap_row_.Slider().SetRange(0, 24).SetStep(1);
        AddToggleRow(drag_box_, drag_reorder_row_, "Enable reorder"); AddToggleRow(drag_box_, drag_row_, "Show handle"); AddChoice(drag_box_, drag_side_row_, "Side"); drag_side_row_.Add("Left", 0).Add("Right", 1); AddSliderRow(drag_box_, drag_size_row_, "Size", "14 px"); drag_size_row_.Slider().SetRange(8, 32).SetStep(1); AddSliderRow(drag_box_, drag_gap_row_, "Gap", "8 px"); drag_gap_row_.Slider().SetRange(0, 24).SetStep(1);

        AddToggleRow(body_face_box_, body_transparent_row_, "Transparent"); AddToggleRow(body_face_box_, body_face_enabled_row_, "Face enabled"); AddColor(body_face_box_, body_face_row_, "Normal");
        AddToggleRow(body_frame_box_, body_frame_enabled_row_, "Enabled"); AddSliderRow(body_frame_box_, body_frame_width_row_, "Width", "0 px"); body_frame_width_row_.Slider().SetRange(0, 8).SetStep(1); AddSliderRow(body_frame_box_, body_radius_row_, "Radius", "0 px"); body_radius_row_.Slider().SetRange(0, 32).SetStep(1); AddColor(body_frame_box_, body_frame_row_, "Normal");
        AddSliderRow(body_margin_box_, body_margin_x_row_, "Horizontal", "10 px"); body_margin_x_row_.Slider().SetRange(0, 32).SetStep(1); AddSliderRow(body_margin_box_, body_margin_y_row_, "Vertical", "8 px"); body_margin_y_row_.Slider().SetRange(0, 24).SetStep(1);
        AddChoice(body_line_box_, body_line_extent_row_, "Extent"); body_line_extent_row_.Add("None", 0).Add("Small", 1).Add("Medium", 2).Add("Large", 3);
        AddChoice(body_line_box_, body_line_style_row_, "Style"); body_line_style_row_.Add("Solid", 0).Add("Dashed", 1).Add("Dotted", 2);
        AddSliderRow(body_line_box_, body_line_thickness_row_, "Thickness", "1 px"); body_line_thickness_row_.Slider().SetRange(1, 8).SetStep(1); AddColor(body_line_box_, body_line_color_row_, "Colour");

        AddToggleRow(behaviour_box_, single_row_, "Single open"); AddToggleRow(behaviour_box_, enforce_row_, "Enforce one");
        AddToggleRow(animation_box_, animation_row_, "Enabled"); AddSliderRow(animation_box_, anim_open_row_, "Open ms", "120"); anim_open_row_.Slider().SetRange(0, 600).SetStep(10); AddSliderRow(animation_box_, anim_close_row_, "Close ms", "0"); anim_close_row_.Slider().SetRange(0, 600).SetStep(10);
    }

    void Connect()
    {
#define BIND_TOGGLE(row, field) row.WhenAction = [=] { cfg_.field = (bool)row.Toggle().GetData(); ApplyConfig(); }
#define BIND_COLOR(row, field) row.WhenAction = [=] { cfg_.field = row.GetColor(0); ApplyConfig(); }
#define BIND_SLIDER(row, field) row.WhenAction = [=] { cfg_.field = (int)row.GetData(); ApplyConfig(); }
        BIND_TOGGLE(face_enabled_row_, face_enabled); BIND_COLOR(face_row_, face); BIND_TOGGLE(frame_enabled_row_, frame_enabled); BIND_SLIDER(frame_width_row_, frame_width); BIND_COLOR(frame_row_, frame); BIND_COLOR(ink_row_, ink); BIND_SLIDER(margin_x_row_, margin_x); BIND_SLIDER(margin_y_row_, margin_y); BIND_TOGGLE(shadow_row_, shadow); BIND_TOGGLE(highlight_row_, highlight);
        BIND_SLIDER(header_height_row_, header_height); BIND_SLIDER(spacing_row_, item_spacing); BIND_SLIDER(header_body_gap_row_, header_body_gap); BIND_SLIDER(body_min_height_row_, body_min_height); BIND_TOGGLE(unified_row_, unified_frame); BIND_SLIDER(unified_radius_row_, unified_radius); BIND_SLIDER(unified_frame_width_row_, unified_frame_width);
        BIND_TOGGLE(header_face_enabled_row_, header_face_enabled); BIND_COLOR(header_face_row_, header_face); BIND_TOGGLE(header_frame_enabled_row_, header_frame_enabled); BIND_SLIDER(header_frame_width_row_, header_frame_width); BIND_SLIDER(header_radius_row_, header_radius); BIND_COLOR(header_frame_row_, header_frame); BIND_COLOR(header_ink_row_, header_ink); BIND_SLIDER(header_font_row_, header_font_height); BIND_SLIDER(header_margin_x_row_, header_margin_x); BIND_SLIDER(header_margin_y_row_, header_margin_y);
        BIND_TOGGLE(chevron_row_, show_chevron); BIND_SLIDER(chevron_size_row_, chevron_size); BIND_SLIDER(chevron_gap_row_, chevron_gap); BIND_TOGGLE(drag_reorder_row_, drag_reorder); BIND_TOGGLE(drag_row_, show_drag); BIND_SLIDER(drag_size_row_, drag_size); BIND_SLIDER(drag_gap_row_, drag_gap);
        BIND_TOGGLE(body_transparent_row_, body_transparent); BIND_TOGGLE(body_face_enabled_row_, body_face_enabled); BIND_COLOR(body_face_row_, body_face); BIND_TOGGLE(body_frame_enabled_row_, body_frame_enabled); BIND_SLIDER(body_frame_width_row_, body_frame_width); BIND_SLIDER(body_radius_row_, body_radius); BIND_COLOR(body_frame_row_, body_frame); BIND_SLIDER(body_margin_x_row_, body_margin_x); BIND_SLIDER(body_margin_y_row_, body_margin_y); BIND_SLIDER(body_line_thickness_row_, body_line_thickness); BIND_COLOR(body_line_color_row_, body_line_color);
        BIND_TOGGLE(single_row_, single_open); BIND_TOGGLE(enforce_row_, enforce_one); BIND_TOGGLE(animation_row_, animation); BIND_SLIDER(anim_open_row_, anim_open_ms); BIND_SLIDER(anim_close_row_, anim_close_ms);
#undef BIND_TOGGLE
#undef BIND_COLOR
#undef BIND_SLIDER
        chevron_side_row_.WhenSelect = [=](int) { cfg_.chevron_side = (int)chevron_side_row_.Dropdown().GetSelectedData() == 0 ? UiAlign::LEFT : UiAlign::RIGHT; ApplyConfig(); };
        drag_side_row_.WhenSelect = [=](int) { cfg_.drag_side = (int)drag_side_row_.Dropdown().GetSelectedData() == 0 ? UiAlign::LEFT : UiAlign::RIGHT; ApplyConfig(); };
        body_line_extent_row_.WhenSelect = [=](int) { int q = (int)body_line_extent_row_.Dropdown().GetSelectedData(); cfg_.body_line_extent = q == 1 ? SMALL : q == 2 ? MEDIUM : q == 3 ? LARGE : NONE; ApplyConfig(); };
        body_line_style_row_.WhenSelect = [=](int) { int q = (int)body_line_style_row_.Dropdown().GetSelectedData(); cfg_.body_line_style = q == 1 ? DASHED : q == 2 ? DOTTED : SOLID; ApplyConfig(); };
    }

    void SyncRows()
    {
        face_enabled_row_.SetData(cfg_.face_enabled); face_row_.SetColor(0, cfg_.face); frame_enabled_row_.SetData(cfg_.frame_enabled); frame_width_row_.SetData(cfg_.frame_width); frame_row_.SetColor(0, cfg_.frame); ink_row_.SetColor(0, cfg_.ink); margin_x_row_.SetData(cfg_.margin_x); margin_y_row_.SetData(cfg_.margin_y); shadow_row_.SetData(cfg_.shadow); highlight_row_.SetData(cfg_.highlight);
        header_height_row_.SetData(cfg_.header_height); spacing_row_.SetData(cfg_.item_spacing); header_body_gap_row_.SetData(cfg_.header_body_gap); body_min_height_row_.SetData(cfg_.body_min_height); unified_row_.SetData(cfg_.unified_frame); unified_radius_row_.SetData(cfg_.unified_radius); unified_frame_width_row_.SetData(cfg_.unified_frame_width);
        header_face_enabled_row_.SetData(cfg_.header_face_enabled); header_face_row_.SetColor(0, cfg_.header_face); header_frame_enabled_row_.SetData(cfg_.header_frame_enabled); header_frame_width_row_.SetData(cfg_.header_frame_width); header_radius_row_.SetData(cfg_.header_radius); header_frame_row_.SetColor(0, cfg_.header_frame); header_ink_row_.SetColor(0, cfg_.header_ink); header_font_row_.SetData(cfg_.header_font_height); header_margin_x_row_.SetData(cfg_.header_margin_x); header_margin_y_row_.SetData(cfg_.header_margin_y);
        chevron_row_.SetData(cfg_.show_chevron); chevron_side_row_.SelectByData(cfg_.chevron_side == UiAlign::LEFT ? 0 : 1); chevron_size_row_.SetData(cfg_.chevron_size); chevron_gap_row_.SetData(cfg_.chevron_gap); drag_reorder_row_.SetData(cfg_.drag_reorder); drag_row_.SetData(cfg_.show_drag); drag_side_row_.SelectByData(cfg_.drag_side == UiAlign::LEFT ? 0 : 1); drag_size_row_.SetData(cfg_.drag_size); drag_gap_row_.SetData(cfg_.drag_gap);
        body_transparent_row_.SetData(cfg_.body_transparent); body_face_enabled_row_.SetData(cfg_.body_face_enabled); body_face_row_.SetColor(0, cfg_.body_face); body_frame_enabled_row_.SetData(cfg_.body_frame_enabled); body_frame_width_row_.SetData(cfg_.body_frame_width); body_radius_row_.SetData(cfg_.body_radius); body_frame_row_.SetColor(0, cfg_.body_frame); body_margin_x_row_.SetData(cfg_.body_margin_x); body_margin_y_row_.SetData(cfg_.body_margin_y); body_line_extent_row_.SelectByData(cfg_.body_line_extent == SMALL ? 1 : cfg_.body_line_extent == MEDIUM ? 2 : cfg_.body_line_extent == LARGE ? 3 : 0); body_line_style_row_.SelectByData(cfg_.body_line_style == DASHED ? 1 : cfg_.body_line_style == DOTTED ? 2 : 0); body_line_thickness_row_.SetData(cfg_.body_line_thickness); body_line_color_row_.SetColor(0, cfg_.body_line_color);
        single_row_.SetData(cfg_.single_open); enforce_row_.SetData(cfg_.enforce_one); animation_row_.SetData(cfg_.animation); anim_open_row_.SetData(cfg_.anim_open_ms); anim_close_row_.SetData(cfg_.anim_close_ms);
#define SETPX(row, field) row.SetValueText(Format("%d px", cfg_.field))
        SETPX(frame_width_row_, frame_width); SETPX(margin_x_row_, margin_x); SETPX(margin_y_row_, margin_y); SETPX(header_height_row_, header_height); SETPX(spacing_row_, item_spacing); SETPX(header_body_gap_row_, header_body_gap); SETPX(body_min_height_row_, body_min_height); SETPX(unified_radius_row_, unified_radius); SETPX(unified_frame_width_row_, unified_frame_width); SETPX(header_frame_width_row_, header_frame_width); SETPX(header_radius_row_, header_radius); SETPX(header_font_row_, header_font_height); SETPX(header_margin_x_row_, header_margin_x); SETPX(header_margin_y_row_, header_margin_y); SETPX(chevron_size_row_, chevron_size); SETPX(chevron_gap_row_, chevron_gap); SETPX(drag_size_row_, drag_size); SETPX(drag_gap_row_, drag_gap); SETPX(body_frame_width_row_, body_frame_width); SETPX(body_radius_row_, body_radius); SETPX(body_margin_x_row_, body_margin_x); SETPX(body_margin_y_row_, body_margin_y); SETPX(body_line_thickness_row_, body_line_thickness);
#undef SETPX
        anim_open_row_.SetValueText(AsString(cfg_.anim_open_ms)); anim_close_row_.SetValueText(AsString(cfg_.anim_close_ms));
    }

    void ApplyConfig()
    {
        SyncRows();
        UiAccordion::Style s = MakeAccordionStyle(Palette());
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(cfg_.face);
            s.palette.frame[i] = cfg_.frame;
            s.palette.ink[i] = cfg_.ink;
            s.header_style.palette.face[i] = UiFill::Solid(cfg_.header_face);
            s.header_style.palette.frame[i] = cfg_.header_frame;
            s.header_style.palette.ink[i] = cfg_.header_ink;
            s.body_style.palette.face[i] = UiFill::Solid(cfg_.body_face);
            s.body_style.palette.frame[i] = cfg_.body_frame;
        }
        s.metrics.face_enabled = cfg_.face_enabled;
        s.metrics.frame_enabled = cfg_.frame_enabled;
        s.metrics.frame_width = cfg_.frame_width;
        s.metrics.content_margin = Rect(cfg_.margin_x, cfg_.margin_y, cfg_.margin_x, cfg_.margin_y);
        s.metrics.shadow.enabled = cfg_.shadow;
        s.metrics.highlight.enabled = cfg_.highlight;
        s.header_height = cfg_.header_height;
        s.item_spacing = cfg_.item_spacing;
        s.header_body_gap = cfg_.header_body_gap;
        s.body_min_height = cfg_.body_min_height;
        s.unified_section_frame = cfg_.unified_frame;
        s.unified_section_radius = cfg_.unified_radius;
        s.unified_section_frame_width = cfg_.unified_frame_width;

        s.header_style.metrics.face_enabled = cfg_.header_face_enabled;
        s.header_style.metrics.frame_enabled = cfg_.header_frame_enabled;
        s.header_style.metrics.frame_width = cfg_.header_frame_width;
        s.header_style.metrics.radius = cfg_.header_radius;
        s.header_style.metrics.content_margin = Rect(cfg_.header_margin_x, cfg_.header_margin_y, cfg_.header_margin_x, cfg_.header_margin_y);
        s.header_style.title_font.Height(cfg_.header_font_height);

        s.show_chevron = cfg_.show_chevron;
        s.chevron_side = cfg_.chevron_side;
        s.chevron_scale = true;
        s.chevron_size = cfg_.chevron_size;
        s.chevron_gap = cfg_.chevron_gap;
        s.show_drag_handle = cfg_.show_drag;
        s.drag_side = cfg_.drag_side;
        s.drag_size = cfg_.drag_size;
        s.drag_gap = cfg_.drag_gap;

        s.body_style.transparent = cfg_.body_transparent;
        s.body_style.metrics.face_enabled = cfg_.body_face_enabled;
        s.body_style.metrics.frame_enabled = cfg_.body_frame_enabled;
        s.body_style.metrics.frame_width = cfg_.body_frame_width;
        s.body_style.metrics.radius = cfg_.body_radius;
        s.body_style.metrics.content_margin = Rect(cfg_.body_margin_x, cfg_.body_margin_y, cfg_.body_margin_x, cfg_.body_margin_y);
        s.body_line_extent = cfg_.body_line_extent;
        s.body_line_style = cfg_.body_line_style;
        s.body_line_thickness = cfg_.body_line_thickness;
        s.body_line_color = cfg_.body_line_color;
        s.single_open = cfg_.single_open;
        s.enforce_one = cfg_.enforce_one;
        s.animation_enabled = cfg_.animation;
        s.anim_open_ms = cfg_.anim_open_ms;
        s.anim_close_ms = cfg_.anim_close_ms;

        accordion_.SetCustomStyle(s);
        accordion_.SetSingleOpen(cfg_.single_open);
        accordion_.SetEnforceOne(cfg_.enforce_one);
        accordion_.ShowChevron(cfg_.show_chevron);
        accordion_.SetChevronSide(cfg_.chevron_side);
        accordion_.SetChevronSize(cfg_.chevron_size);
        accordion_.SetChevronGap(cfg_.chevron_gap);
        accordion_.SetAnimation(cfg_.animation, cfg_.anim_open_ms, cfg_.anim_close_ms);
        accordion_.EnableDragReorder(cfg_.drag_reorder);
        accordion_.ShowDragHandle(cfg_.show_drag);
        accordion_.SetDragSide(cfg_.drag_side);

        SetUsageCode("UiAccordion::Style style = UiAccordion::StyleDefault();\n"
                     "// Header/* and Body/* remain composed style domains; Section and animation stay Accordion-owned.\n"
                     "accordion.SetCustomStyle(style);\n");
        Refresh();
    }
};

} // namespace

GUI_APP_MAIN
{
    UiAccordionDemoWindow().Run();
}
