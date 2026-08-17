/*
    UiLineEditDemo -- canonical UiBaseEdit ownership reference.
*/
#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

class UiLineEditDemoWindow : public BuilderWindowBase {
public:
    typedef UiLineEditDemoWindow CLASSNAME;

    UiLineEditDemoWindow()
        : BuilderWindowBase("UiLineEdit Demo", "UiBaseEdit / UiLineEdit",
                            "Face, Editing, Underline and Whitespace ownership reference")
    {
        Preview().Add(edit_);
        edit_.SetTextUtf8("Edit me");
        edit_.SetPlaceholder("Placeholder text");
        BuildSections();
        BuildRows();
        Connect();
        FinishInit();
    }

protected:
    void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        int cx = min(DPI(380), max(DPI(180), canvas.GetWidth() - DPI(80)));
        edit_.SetRect(canvas.left + (canvas.GetWidth() - cx) / 2,
                      canvas.top + max(0, (canvas.GetHeight() - DPI(38)) / 2),
                      cx, DPI(38));
    }

    void ApplyDemoTheme() override
    {
        ApplyConfig();
    }

private:
    struct Config {
        bool enabled = true;
        bool read_only = false;
        bool face_enabled = true;
        Color face = White();
        bool frame_enabled = true;
        int frame_width = 1;
        Color frame = Color(215, 219, 226);
        Color ink = Color(17, 24, 39);
        Color placeholder = Color(148, 163, 184);
        int font_height = 11;
        int margin_x = 10;
        int margin_y = 6;

        Color caret = Color(17, 24, 39);
        int caret_width = 1;
        bool block_caret = false;
        Color selection_face = Color(219, 234, 254);
        Color selection_ink = Color(17, 24, 39);

        bool underline_enabled = false;
        int underline_width = 1;
        Color underline = Color(112, 122, 138);

        int tab_size = 4;
        Color whitespace = Color(148, 163, 184);
        Color tab_char = Color(148, 163, 184);
        bool show_tabs = false;
        bool show_spaces = false;
        bool show_line_endings = false;
        bool readonly_background = true;

        bool focus_enabled = true;
        Color focus = Color(65, 126, 232);
        bool shadow_enabled = false;
        Color shadow = Black();
        bool highlight_enabled = false;
        Color highlight = White();
    } cfg_;

    UiLineEdit edit_;
    UiBoxLayout face_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout frame_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout ink_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout typography_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout margin_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout editing_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout underline_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout whitespace_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout focus_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout shadow_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout highlight_box_ { UiBoxLayout::Direction::V };

    DemoToggleRow enabled_row_, readonly_row_, face_enabled_row_, frame_enabled_row_;
    DemoToggleRow block_caret_row_, underline_enabled_row_, show_tabs_row_, show_spaces_row_, show_line_endings_row_, readonly_background_row_;
    DemoToggleRow focus_enabled_row_, shadow_enabled_row_, highlight_enabled_row_;
    DemoSliderRow frame_width_row_, font_height_row_, margin_x_row_, margin_y_row_, caret_width_row_, underline_width_row_, tab_size_row_;
    DemoColorRow face_row_, frame_row_, ink_row_, placeholder_row_, caret_row_, selection_face_row_, selection_ink_row_;
    DemoColorRow underline_row_, whitespace_row_, tab_char_row_, focus_row_, shadow_row_, highlight_row_;

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
        AddSection("TYPOGRAPHY", typography_box_);
        AddSection("CONTENT MARGIN", margin_box_);
        AddSection("EDITING", editing_box_);
        AddSection("UNDERLINE", underline_box_);
        AddSection("WHITESPACE", whitespace_box_);
        AddSection("FOCUS", focus_box_);
        AddSection("SHADOW", shadow_box_);
        AddSection("HIGHLIGHT", highlight_box_);
    }

    void BuildRows()
    {
        AddToggleRow(PropsBox(), enabled_row_, "Enabled");
        AddToggleRow(PropsBox(), readonly_row_, "Read only");

        AddToggleRow(face_box_, face_enabled_row_, "Enabled");
        AddColor(face_box_, face_row_, "Normal");

        AddToggleRow(frame_box_, frame_enabled_row_, "Enabled");
        AddSliderRow(frame_box_, frame_width_row_, "Width", "1 px"); frame_width_row_.Slider().SetRange(0, 8).SetStep(1);
        AddColor(frame_box_, frame_row_, "Normal");

        AddColor(ink_box_, ink_row_, "Normal");
        AddColor(ink_box_, placeholder_row_, "Placeholder");

        AddSliderRow(typography_box_, font_height_row_, "Font height", "11 px"); font_height_row_.Slider().SetRange(8, 28).SetStep(1);
        AddSliderRow(margin_box_, margin_x_row_, "Horizontal", "10 px"); margin_x_row_.Slider().SetRange(0, 32).SetStep(1);
        AddSliderRow(margin_box_, margin_y_row_, "Vertical", "6 px"); margin_y_row_.Slider().SetRange(0, 24).SetStep(1);

        AddColor(editing_box_, caret_row_, "Caret colour");
        AddSliderRow(editing_box_, caret_width_row_, "Caret width", "1 px"); caret_width_row_.Slider().SetRange(1, 8).SetStep(1);
        AddToggleRow(editing_box_, block_caret_row_, "Block caret");
        AddColor(editing_box_, selection_face_row_, "Selection face");
        AddColor(editing_box_, selection_ink_row_, "Selection ink");

        AddToggleRow(underline_box_, underline_enabled_row_, "Enabled");
        AddSliderRow(underline_box_, underline_width_row_, "Width", "1 px"); underline_width_row_.Slider().SetRange(1, 8).SetStep(1);
        AddColor(underline_box_, underline_row_, "Normal");

        AddSliderRow(whitespace_box_, tab_size_row_, "Tab size", "4"); tab_size_row_.Slider().SetRange(1, 12).SetStep(1);
        AddColor(whitespace_box_, whitespace_row_, "Whitespace colour");
        AddColor(whitespace_box_, tab_char_row_, "Tab character colour");
        AddToggleRow(whitespace_box_, show_tabs_row_, "Show tabs");
        AddToggleRow(whitespace_box_, show_spaces_row_, "Show spaces");
        AddToggleRow(whitespace_box_, show_line_endings_row_, "Show line endings");
        AddToggleRow(whitespace_box_, readonly_background_row_, "Readonly background");

        AddToggleRow(focus_box_, focus_enabled_row_, "Enabled");
        AddColor(focus_box_, focus_row_, "Colour");
        AddToggleRow(shadow_box_, shadow_enabled_row_, "Enabled");
        AddColor(shadow_box_, shadow_row_, "Colour");
        AddToggleRow(highlight_box_, highlight_enabled_row_, "Enabled");
        AddColor(highlight_box_, highlight_row_, "Colour");
    }

    void Connect()
    {
#define BIND_TOGGLE(row, field) row.WhenAction = [=] { cfg_.field = (bool)row.Toggle().GetData(); ApplyConfig(); }
#define BIND_COLOR(row, field) row.WhenAction = [=] { cfg_.field = row.GetColor(0); ApplyConfig(); }
#define BIND_SLIDER(row, field) row.WhenAction = [=] { cfg_.field = (int)row.GetData(); ApplyConfig(); }
        BIND_TOGGLE(enabled_row_, enabled); BIND_TOGGLE(readonly_row_, read_only);
        BIND_TOGGLE(face_enabled_row_, face_enabled); BIND_COLOR(face_row_, face);
        BIND_TOGGLE(frame_enabled_row_, frame_enabled); BIND_SLIDER(frame_width_row_, frame_width); BIND_COLOR(frame_row_, frame);
        BIND_COLOR(ink_row_, ink); BIND_COLOR(placeholder_row_, placeholder); BIND_SLIDER(font_height_row_, font_height);
        BIND_SLIDER(margin_x_row_, margin_x); BIND_SLIDER(margin_y_row_, margin_y);
        BIND_COLOR(caret_row_, caret); BIND_SLIDER(caret_width_row_, caret_width); BIND_TOGGLE(block_caret_row_, block_caret); BIND_COLOR(selection_face_row_, selection_face); BIND_COLOR(selection_ink_row_, selection_ink);
        BIND_TOGGLE(underline_enabled_row_, underline_enabled); BIND_SLIDER(underline_width_row_, underline_width); BIND_COLOR(underline_row_, underline);
        BIND_SLIDER(tab_size_row_, tab_size); BIND_COLOR(whitespace_row_, whitespace); BIND_COLOR(tab_char_row_, tab_char); BIND_TOGGLE(show_tabs_row_, show_tabs); BIND_TOGGLE(show_spaces_row_, show_spaces); BIND_TOGGLE(show_line_endings_row_, show_line_endings); BIND_TOGGLE(readonly_background_row_, readonly_background);
        BIND_TOGGLE(focus_enabled_row_, focus_enabled); BIND_COLOR(focus_row_, focus); BIND_TOGGLE(shadow_enabled_row_, shadow_enabled); BIND_COLOR(shadow_row_, shadow); BIND_TOGGLE(highlight_enabled_row_, highlight_enabled); BIND_COLOR(highlight_row_, highlight);
#undef BIND_TOGGLE
#undef BIND_COLOR
#undef BIND_SLIDER
    }

    void SyncRows()
    {
        enabled_row_.SetData(cfg_.enabled); readonly_row_.SetData(cfg_.read_only);
        face_enabled_row_.SetData(cfg_.face_enabled); face_row_.SetColor(0, cfg_.face); frame_enabled_row_.SetData(cfg_.frame_enabled); frame_width_row_.SetData(cfg_.frame_width); frame_row_.SetColor(0, cfg_.frame);
        ink_row_.SetColor(0, cfg_.ink); placeholder_row_.SetColor(0, cfg_.placeholder); font_height_row_.SetData(cfg_.font_height); margin_x_row_.SetData(cfg_.margin_x); margin_y_row_.SetData(cfg_.margin_y);
        caret_row_.SetColor(0, cfg_.caret); caret_width_row_.SetData(cfg_.caret_width); block_caret_row_.SetData(cfg_.block_caret); selection_face_row_.SetColor(0, cfg_.selection_face); selection_ink_row_.SetColor(0, cfg_.selection_ink);
        underline_enabled_row_.SetData(cfg_.underline_enabled); underline_width_row_.SetData(cfg_.underline_width); underline_row_.SetColor(0, cfg_.underline);
        tab_size_row_.SetData(cfg_.tab_size); whitespace_row_.SetColor(0, cfg_.whitespace); tab_char_row_.SetColor(0, cfg_.tab_char); show_tabs_row_.SetData(cfg_.show_tabs); show_spaces_row_.SetData(cfg_.show_spaces); show_line_endings_row_.SetData(cfg_.show_line_endings); readonly_background_row_.SetData(cfg_.readonly_background);
        focus_enabled_row_.SetData(cfg_.focus_enabled); focus_row_.SetColor(0, cfg_.focus); shadow_enabled_row_.SetData(cfg_.shadow_enabled); shadow_row_.SetColor(0, cfg_.shadow); highlight_enabled_row_.SetData(cfg_.highlight_enabled); highlight_row_.SetColor(0, cfg_.highlight);
#define SETPX(row, field) row.SetValueText(Format("%d px", cfg_.field))
        SETPX(frame_width_row_, frame_width); SETPX(font_height_row_, font_height); SETPX(margin_x_row_, margin_x); SETPX(margin_y_row_, margin_y); SETPX(caret_width_row_, caret_width); SETPX(underline_width_row_, underline_width);
#undef SETPX
        tab_size_row_.SetValueText(AsString(cfg_.tab_size));
    }

    void ApplyConfig()
    {
        SyncRows();
        UiBaseEdit::Style s = UiTheme::ResolveEdit(UiRole::Standard);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(cfg_.face);
            s.palette.frame[i] = cfg_.frame;
            s.palette.ink[i] = cfg_.ink;
            s.underline[i] = cfg_.underline;
        }
        s.metrics.face_enabled = cfg_.face_enabled;
        s.metrics.frame_enabled = cfg_.frame_enabled;
        s.metrics.frame_width = cfg_.frame_width;
        s.metrics.content_margin = Rect(cfg_.margin_x, cfg_.margin_y, cfg_.margin_x, cfg_.margin_y);
        s.font.Height(cfg_.font_height);
        s.placeholder_ink = cfg_.placeholder;
        s.caret_color = cfg_.caret;
        s.caret_width = cfg_.caret_width;
        s.block_caret = cfg_.block_caret;
        s.selection_color = cfg_.selection_face;
        s.selection_ink = cfg_.selection_ink;
        s.underline_enabled = cfg_.underline_enabled;
        s.underline_width = cfg_.underline_width;
        s.tab_size = cfg_.tab_size;
        s.whitespace_color = cfg_.whitespace;
        s.tab_char_color = cfg_.tab_char;
        s.show_tabs = cfg_.show_tabs;
        s.show_spaces = cfg_.show_spaces;
        s.show_line_endings = cfg_.show_line_endings;
        s.show_readonly_bg = cfg_.readonly_background;
        s.metrics.focus_enabled = cfg_.focus_enabled;
        s.metrics.focus_color = cfg_.focus;
        s.metrics.shadow.enabled = cfg_.shadow_enabled;
        s.metrics.shadow.color = cfg_.shadow;
        s.metrics.highlight.enabled = cfg_.highlight_enabled;
        s.metrics.highlight.color = cfg_.highlight;

        edit_.SetCustomStyle(s);
        edit_.SetEditable(!cfg_.read_only);
        edit_.Enable(cfg_.enabled);

        SetUsageCode("UiBaseEdit::Style style = UiTheme::ResolveEdit(UiRole::Standard);\n"
                     "// Editing, Underline and Whitespace remain UiBaseEdit-owned style domains.\n"
                     "edit.SetCustomStyle(style);\n");
        Refresh();
    }
};

} // namespace

GUI_APP_MAIN
{
    UiLineEditDemoWindow().Run();
}
