/*
    UiMultiEditDemo
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

enum TextPreset {
    MULTI_NOTES = 0,
    MULTI_CODE,
    MULTI_LOG,
};

struct MultiEditConfig {
    int preset = MULTI_NOTES;
    String placeholder = "Enter content...";
    UiAlign text_align = UiAlign::LEFT;
    int radius = DPI(8);
    int frame_width = 1;
    int margin_x = DPI(10);
    int margin_y = DPI(8);
    int caret_width = DPI(1);
    int font_px = 10;
    int tab_size = 4;
    bool read_only = false;
    bool accepts_tabs = true;
    bool accepts_drop = true;
    bool overwrite = false;
    bool show_tabs = false;
    bool show_spaces = false;
    bool show_line_endings = false;
    bool show_readonly_bg = true;
    bool left_icon = false;
    bool right_action = false;
    Color face = Color(250, 252, 255);
    Color frame = Color(211, 221, 237);
    Color text = Color(28, 47, 78);
    Color placeholder_ink = Color(106, 128, 164);
    Color selection = Color(44, 99, 212);
    Color selection_ink = White();
    Color caret = Color(28, 47, 78);
};

String MultiPresetText(int preset)
{
    switch(preset) {
    case MULTI_CODE:
        return String()
            << "void BuildTheme(UiThemeContext& ctx) {\n"
            << "    ctx.mode = UiThemeMode::Dark;\n"
            << "    ctx.preset = UiThemePreset::Minimal;\n"
            << "}\n\n"
            << "int main() {\n"
            << "    return RunDemo();\n"
            << "}";
    case MULTI_LOG:
        return String()
            << "[INFO] Session started\n"
            << "[INFO] Loading workspace cache\n"
            << "[WARN] Theme asset missing, using fallback\n"
            << "[INFO] Ready";
    case MULTI_NOTES:
    default:
        return String()
            << "Sprint Notes\n\n"
            << "- tighten item spacing contract\n"
            << "- verify dark theme paint path\n"
            << "- finish builder demos";
    }
}

String AlignName(UiAlign a)
{
    if(a == UiAlign::CENTER) return "Center";
    if(a == UiAlign::RIGHT) return "Right";
    return "Left";
}

class UiMultiEditBuilder : public BuilderWindowBase {
public:
    typedef UiMultiEditBuilder CLASSNAME;

    UiMultiEditBuilder()
        : BuilderWindowBase("UiMultiEditDemo", "U++ UiMultiEdit Builder", "Inspect multiline editing, scrolling, side-items, and the shared edit style contract from one shell.")
    {
        Preview().Add(editor_);
        Preview().Add(left_icon_btn_);
        Preview().Add(clear_btn_);

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_preset_row_, state_preset_label_, state_preset_value_, "Preset");
        AddStateRow(StateBox(), state_mode_row_, state_mode_label_, state_mode_value_, "Mode");
        AddStateRow(StateBox(), state_length_row_, state_length_label_, state_length_value_, "Length");

        AddDropdownRow(PropsBox(), preset_row_box_, preset_label_, preset_drop_, "Preset");
        AddEditRow(PropsBox(), placeholder_row_box_, placeholder_label_, placeholder_edit_, "Placeholder");
        AddDropdownRow(PropsBox(), align_row_box_, align_label_, align_drop_, "Text Align");
        AddSliderRow(PropsBox(), radius_row_, "Radius", "8px");
        AddSliderRow(PropsBox(), frame_width_row_, "Frame W", "1px");
        AddSliderRow(PropsBox(), margin_x_row_, "Margin X", "10px");
        AddSliderRow(PropsBox(), margin_y_row_, "Margin Y", "8px");
        AddSliderRow(PropsBox(), caret_width_row_, "Caret W", "1px");
        AddSliderRow(PropsBox(), font_px_row_, "Font Size", "10px");
        AddSliderRow(PropsBox(), tab_size_row_, "Tab Size", "4");
        AddToggleRow(PropsBox(), read_only_row_, "Read Only");
        AddToggleRow(PropsBox(), accepts_tabs_row_, "Accept Tabs");
        AddToggleRow(PropsBox(), accepts_drop_row_, "Accept Drop");
        AddToggleRow(PropsBox(), overwrite_row_, "Overwrite");
        AddToggleRow(PropsBox(), show_tabs_row_, "Show Tabs");
        AddToggleRow(PropsBox(), show_spaces_row_, "Show Spaces");
        AddToggleRow(PropsBox(), show_line_endings_row_, "Show Endings");
        AddToggleRow(PropsBox(), show_readonly_bg_row_, "Readonly Bg");
        AddToggleRow(PropsBox(), left_icon_row_, "Left Icon");
        AddToggleRow(PropsBox(), right_action_row_, "Right Action");
        AddColorRow(PropsBox(), face_row_, "Face");
        AddColorRow(PropsBox(), frame_row_, "Frame");
        AddColorRow(PropsBox(), text_row_, "Text");
        AddColorRow(PropsBox(), placeholder_ink_row_, "Placeholder");
        AddColorRow(PropsBox(), selection_row_, "Selection");
        AddColorRow(PropsBox(), selection_ink_row_, "Sel Text");
        AddColorRow(PropsBox(), caret_row_, "Caret");

        const EnumOption presets[] = {
            { "Notes", MULTI_NOTES }, { "Code", MULTI_CODE }, { "Log", MULTI_LOG }
        };
        const EnumOption aligns[] = {
            { "Left", (int)UiAlign::LEFT }, { "Center", (int)UiAlign::CENTER }, { "Right", (int)UiAlign::RIGHT }
        };
        PopulateDropdown(preset_drop_, presets, 3);
        PopulateDropdown(align_drop_, aligns, 3);

        radius_row_.Slider().SetRange(0, DPI(24)).SetStep(1).SetValue(cfg_.radius);
        frame_width_row_.Slider().SetRange(0, 4).SetStep(1).SetValue(cfg_.frame_width);
        margin_x_row_.Slider().SetRange(0, DPI(24)).SetStep(1).SetValue(cfg_.margin_x);
        margin_y_row_.Slider().SetRange(0, DPI(18)).SetStep(1).SetValue(cfg_.margin_y);
        caret_width_row_.Slider().SetRange(1, DPI(4)).SetStep(1).SetValue(cfg_.caret_width);
        font_px_row_.Slider().SetRange(8, 16).SetStep(1).SetValue(cfg_.font_px);
        tab_size_row_.Slider().SetRange(2, 8).SetStep(1).SetValue(cfg_.tab_size);

        placeholder_edit_.SetData(cfg_.placeholder);
        InitColorRow(face_row_, cfg_.face);
        InitColorRow(frame_row_, cfg_.frame);
        InitColorRow(text_row_, cfg_.text);
        InitColorRow(placeholder_ink_row_, cfg_.placeholder_ink);
        InitColorRow(selection_row_, cfg_.selection);
        InitColorRow(selection_ink_row_, cfg_.selection_ink);
        InitColorRow(caret_row_, cfg_.caret);

        left_icon_btn_.SetIcon(ICON_CONTENT_CONTENT_COPY_48());
        clear_btn_.SetIcon(ICON_DESIGN_DELETE_48());
        clear_btn_.WhenAction = [=] { editor_.Clear(); SyncState(); };

        preset_drop_.WhenSelect = [=](int) { cfg_.preset = (int)preset_drop_.GetSelectedData(); RefreshFromConfig(); };
        placeholder_edit_.WhenAction = [=] { cfg_.placeholder = placeholder_edit_.GetText().ToString(); RefreshFromConfig(); };
        placeholder_edit_.WhenChange = [=] { cfg_.placeholder = placeholder_edit_.GetText().ToString(); RefreshFromConfig(); };
        align_drop_.WhenSelect = [=](int) { cfg_.text_align = (UiAlign)(int)align_drop_.GetSelectedData(); RefreshFromConfig(); };
        radius_row_.WhenAction = [=] { cfg_.radius = (int)radius_row_.Slider().GetValue(); RefreshFromConfig(); };
        frame_width_row_.WhenAction = [=] { cfg_.frame_width = (int)frame_width_row_.Slider().GetValue(); RefreshFromConfig(); };
        margin_x_row_.WhenAction = [=] { cfg_.margin_x = (int)margin_x_row_.Slider().GetValue(); RefreshFromConfig(); };
        margin_y_row_.WhenAction = [=] { cfg_.margin_y = (int)margin_y_row_.Slider().GetValue(); RefreshFromConfig(); };
        caret_width_row_.WhenAction = [=] { cfg_.caret_width = (int)caret_width_row_.Slider().GetValue(); RefreshFromConfig(); };
        font_px_row_.WhenAction = [=] { cfg_.font_px = (int)font_px_row_.Slider().GetValue(); RefreshFromConfig(); };
        tab_size_row_.WhenAction = [=] { cfg_.tab_size = (int)tab_size_row_.Slider().GetValue(); RefreshFromConfig(); };
        read_only_row_.Toggle().WhenAction = [=] { cfg_.read_only = read_only_row_.Toggle().IsOn(); RefreshFromConfig(); };
        accepts_tabs_row_.Toggle().WhenAction = [=] { cfg_.accepts_tabs = accepts_tabs_row_.Toggle().IsOn(); RefreshFromConfig(); };
        accepts_drop_row_.Toggle().WhenAction = [=] { cfg_.accepts_drop = accepts_drop_row_.Toggle().IsOn(); RefreshFromConfig(); };
        overwrite_row_.Toggle().WhenAction = [=] { cfg_.overwrite = overwrite_row_.Toggle().IsOn(); RefreshFromConfig(); };
        show_tabs_row_.Toggle().WhenAction = [=] { cfg_.show_tabs = show_tabs_row_.Toggle().IsOn(); RefreshFromConfig(); };
        show_spaces_row_.Toggle().WhenAction = [=] { cfg_.show_spaces = show_spaces_row_.Toggle().IsOn(); RefreshFromConfig(); };
        show_line_endings_row_.Toggle().WhenAction = [=] { cfg_.show_line_endings = show_line_endings_row_.Toggle().IsOn(); RefreshFromConfig(); };
        show_readonly_bg_row_.Toggle().WhenAction = [=] { cfg_.show_readonly_bg = show_readonly_bg_row_.Toggle().IsOn(); RefreshFromConfig(); };
        left_icon_row_.Toggle().WhenAction = [=] { cfg_.left_icon = left_icon_row_.Toggle().IsOn(); RefreshFromConfig(); };
        right_action_row_.Toggle().WhenAction = [=] { cfg_.right_action = right_action_row_.Toggle().IsOn(); RefreshFromConfig(); };
        face_row_.WhenAction = [=] { cfg_.face = face_row_.GetColor(0); RefreshFromConfig(); };
        frame_row_.WhenAction = [=] { cfg_.frame = frame_row_.GetColor(0); RefreshFromConfig(); };
        text_row_.WhenAction = [=] { cfg_.text = text_row_.GetColor(0); RefreshFromConfig(); };
        placeholder_ink_row_.WhenAction = [=] { cfg_.placeholder_ink = placeholder_ink_row_.GetColor(0); RefreshFromConfig(); };
        selection_row_.WhenAction = [=] { cfg_.selection = selection_row_.GetColor(0); RefreshFromConfig(); };
        selection_ink_row_.WhenAction = [=] { cfg_.selection_ink = selection_ink_row_.GetColor(0); RefreshFromConfig(); };
        caret_row_.WhenAction = [=] { cfg_.caret = caret_row_.GetColor(0); RefreshFromConfig(); };
        editor_.WhenAction = [=] { SyncState(); };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        RefreshFromConfig();
    }

    virtual void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        editor_.SetRect(canvas.left + DPI(36), canvas.top + DPI(40), max(DPI(280), canvas.GetWidth() - DPI(72)), max(DPI(220), canvas.GetHeight() - DPI(80)));
    }

private:
    struct EnumOption { const char* label; int value; };

    void AddColorRow(UiBoxLayout& target, UiCompositeColor& row, const char* name)
    {
        row.SetLabel(name).SetColorCount(1).ShowValue(false);
        target.Add(row).Fit();
    }

    void InitColorRow(UiCompositeColor& row, Color c)
    {
        row.SetColor(0, c);
    }

    void PopulateDropdown(UiDropdown& drop, const EnumOption* opts, int count)
    {
        drop.UseInternalModel();
        drop.Clear();
        for(int i = 0; i < count; i++)
            drop.Add(opts[i].label, opts[i].value);
    }

    void RefreshFromConfig()
    {
        preset_drop_.SelectByData(cfg_.preset);
        placeholder_edit_.SetData(cfg_.placeholder);
        align_drop_.SelectByData((int)cfg_.text_align);
        radius_row_.Slider().SetValue(cfg_.radius);
        frame_width_row_.Slider().SetValue(cfg_.frame_width);
        margin_x_row_.Slider().SetValue(cfg_.margin_x);
        margin_y_row_.Slider().SetValue(cfg_.margin_y);
        caret_width_row_.Slider().SetValue(cfg_.caret_width);
        font_px_row_.Slider().SetValue(cfg_.font_px);
        tab_size_row_.Slider().SetValue(cfg_.tab_size);
        read_only_row_.Toggle().SetOn(cfg_.read_only);
        accepts_tabs_row_.Toggle().SetOn(cfg_.accepts_tabs);
        accepts_drop_row_.Toggle().SetOn(cfg_.accepts_drop);
        overwrite_row_.Toggle().SetOn(cfg_.overwrite);
        show_tabs_row_.Toggle().SetOn(cfg_.show_tabs);
        show_spaces_row_.Toggle().SetOn(cfg_.show_spaces);
        show_line_endings_row_.Toggle().SetOn(cfg_.show_line_endings);
        show_readonly_bg_row_.Toggle().SetOn(cfg_.show_readonly_bg);
        left_icon_row_.Toggle().SetOn(cfg_.left_icon);
        right_action_row_.Toggle().SetOn(cfg_.right_action);
        face_row_.SetColor(0, cfg_.face);
        frame_row_.SetColor(0, cfg_.frame);
        text_row_.SetColor(0, cfg_.text);
        placeholder_ink_row_.SetColor(0, cfg_.placeholder_ink);
        selection_row_.SetColor(0, cfg_.selection);
        selection_ink_row_.SetColor(0, cfg_.selection_ink);
        caret_row_.SetColor(0, cfg_.caret);

        UiBaseEdit::Style s = MakeEditStyle(Palette());
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(cfg_.face);
            s.palette.frame[i] = cfg_.frame;
            s.palette.ink[i] = cfg_.text;
        }
        s.placeholder_ink = cfg_.placeholder_ink;
        s.selection_color = cfg_.selection;
        s.selection_ink = cfg_.selection_ink;
        s.caret_color = cfg_.caret;
        s.caret_width = cfg_.caret_width;
        s.font = DemoMono(cfg_.preset == MULTI_CODE ? cfg_.font_px : cfg_.font_px, false);
        if(cfg_.preset != MULTI_CODE)
            s.font = DemoSans(cfg_.font_px);
        s.text_align = cfg_.text_align;
        s.metrics.radius = cfg_.radius;
        s.metrics.frame_width = cfg_.frame_width;
        s.metrics.content_margin = Rect(cfg_.margin_x, cfg_.margin_y, cfg_.margin_x, cfg_.margin_y);
        s.tab_size = cfg_.tab_size;
        s.show_tabs = cfg_.show_tabs;
        s.show_spaces = cfg_.show_spaces;
        s.show_line_endings = cfg_.show_line_endings;
        s.show_readonly_bg = cfg_.show_readonly_bg;
        editor_.SetCustomStyle(s);
        editor_.SetPlaceholder(cfg_.placeholder);
        editor_.SetAcceptsTabs(cfg_.accepts_tabs);
        editor_.SetAcceptsDrop(cfg_.accepts_drop);
        editor_.SetOverwriteMode(cfg_.overwrite);
        editor_.SetTextAlign(cfg_.text_align);
        editor_.SetText(MultiPresetText(cfg_.preset).ToWString());
        if(cfg_.read_only) editor_.SetReadOnly(); else editor_.SetEditable(true);
        editor_.ClearSides();
        if(cfg_.left_icon)
            editor_.AddToSide(left_icon_btn_, UiAlign::LEFT, Size(DPI(24), DPI(24))).Overlay(false);
        if(cfg_.right_action)
            editor_.AddToSide(clear_btn_, UiAlign::RIGHT, Size(DPI(24), DPI(24))).Overlay(false);

        SyncState();
        SyncCode();
        LayoutPreviewContent();
        Preview().Refresh();
    }

    void SyncState()
    {
        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_preset_value_.SetText(cfg_.preset == MULTI_CODE ? "Code" : cfg_.preset == MULTI_LOG ? "Log" : "Notes");
        state_mode_value_.SetText(cfg_.read_only ? "Read Only" : "Editable");
        state_length_value_.SetText(AsString(editor_.GetText().GetCount()) + " chars");
    }

    void SyncCode()
    {
        String code;
        code << "UiMultiEdit edit;\n";
        code << "edit.SetCustomStyle(UiTheme::ResolveEdit());\n";
        code << "edit.SetPlaceholder(" << QuoteCpp(cfg_.placeholder) << ");\n";
        code << "edit.SetTextAlign(UiAlign::" << (cfg_.text_align == UiAlign::CENTER ? "CENTER" : cfg_.text_align == UiAlign::RIGHT ? "RIGHT" : "LEFT") << ");\n";
        code << "edit.SetText(" << QuoteCpp(MultiPresetText(cfg_.preset)) << ");\n";
        if(cfg_.read_only) code << "edit.SetReadOnly();\n";
        if(cfg_.left_icon) code << "edit.AddToSide(icon_button, UiAlign::LEFT, Size(24, 24));\n";
        if(cfg_.right_action) code << "edit.AddToSide(action_button, UiAlign::RIGHT, Size(24, 24));\n";
        code << "// style: radius=" << cfg_.radius << ", frame_width=" << cfg_.frame_width << ", content_margin=Rect(" << cfg_.margin_x << ", " << cfg_.margin_y << ", " << cfg_.margin_x << ", " << cfg_.margin_y << ")\n";
        SetUsageCode(code);
    }

    MultiEditConfig cfg_;
    UiMultiEdit editor_;
    UiButton left_icon_btn_, clear_btn_;

    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_preset_row_ { UiBoxLayout::Direction::H }, state_mode_row_ { UiBoxLayout::Direction::H }, state_length_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_preset_label_, state_preset_value_, state_mode_label_, state_mode_value_, state_length_label_, state_length_value_;

    UiBoxLayout preset_row_box_ { UiBoxLayout::Direction::H }, placeholder_row_box_ { UiBoxLayout::Direction::H }, align_row_box_ { UiBoxLayout::Direction::H };
    UiLabel preset_label_, placeholder_label_, align_label_;
    UiDropdown preset_drop_, align_drop_;
    UiLineEdit placeholder_edit_;
    UiCompositeSlider radius_row_, frame_width_row_, margin_x_row_, margin_y_row_, caret_width_row_, font_px_row_, tab_size_row_;
    UiCompositeToggle read_only_row_, accepts_tabs_row_, accepts_drop_row_, overwrite_row_, show_tabs_row_, show_spaces_row_, show_line_endings_row_, show_readonly_bg_row_, left_icon_row_, right_action_row_;
    UiCompositeColor face_row_, frame_row_, text_row_, placeholder_ink_row_, selection_row_, selection_ink_row_, caret_row_;
};

}

GUI_APP_MAIN
{
    UiMultiEditBuilder().Run();
}


