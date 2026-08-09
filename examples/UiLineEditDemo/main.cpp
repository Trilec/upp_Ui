/*
    UiLineEditDemo
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

struct LineEditConfig {
    String text = "Morgan";
    String placeholder = "Type here...";
    UiAlign align = UiAlign::LEFT;
    int radius = DPI(8);
    int frame_width = 1;
    int margin_x = DPI(10);
    int margin_y = DPI(5);
    bool enabled = true;
    bool readonly = false;
};

class UiLineEditBuilder : public BuilderWindowBase {
public:
    typedef UiLineEditBuilder CLASSNAME;

    UiLineEditBuilder()
        : BuilderWindowBase("UiLineEditDemo", "U++ UiLineEdit Builder", "Inspect single-line text input styling, alignment, placeholder, and edit state from one shell.")
    {
        Preview().Add(edit_);

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_text_row_, state_text_label_, state_text_value_, "Text");
        AddStateRow(StateBox(), state_align_row_, state_align_label_, state_align_value_, "Align");
        AddStateRow(StateBox(), state_mode_row_, state_mode_label_, state_mode_value_, "Mode");

        AddEditRow(PropsBox(), text_row_box_, text_label_, text_edit_, "Text");
        AddEditRow(PropsBox(), placeholder_row_box_, placeholder_label_, placeholder_edit_, "Placeholder");
        AddDropdownRow(PropsBox(), align_row_box_, align_label_, align_drop_, "Align H");
        AddSliderRow(PropsBox(), radius_row_, "Radius", "8px");
        AddSliderRow(PropsBox(), frame_width_row_, "Frame W", "1px");
        AddSliderRow(PropsBox(), margin_x_row_, "Margin X", "10px");
        AddSliderRow(PropsBox(), margin_y_row_, "Margin Y", "5px");
        AddToggleRow(PropsBox(), enabled_row_, "Enabled");
        AddToggleRow(PropsBox(), readonly_row_, "Read Only");

        const EnumOption aligns[] = { { "Left", (int)UiAlign::LEFT }, { "Center", (int)UiAlign::CENTER }, { "Right", (int)UiAlign::RIGHT } };
        PopulateDropdown(align_drop_, aligns, 3);

        text_edit_.SetData(cfg_.text);
        placeholder_edit_.SetData(cfg_.placeholder);
        radius_row_.Slider().SetRange(0, DPI(18)).SetStep(1).SetValue(cfg_.radius);
        frame_width_row_.Slider().SetRange(0, 4).SetStep(1).SetValue(cfg_.frame_width);
        margin_x_row_.Slider().SetRange(0, DPI(20)).SetStep(1).SetValue(cfg_.margin_x);
        margin_y_row_.Slider().SetRange(0, DPI(12)).SetStep(1).SetValue(cfg_.margin_y);

        text_edit_.WhenChange = [=] { cfg_.text = text_edit_.GetData().ToString(); RefreshFromConfig(); };
        placeholder_edit_.WhenChange = [=] { cfg_.placeholder = placeholder_edit_.GetData().ToString(); RefreshFromConfig(); };
        align_drop_.WhenSelect = [=](int) { cfg_.align = (UiAlign)(int)align_drop_.GetSelectedData(); RefreshFromConfig(); };
        radius_row_.WhenAction = [=] { cfg_.radius = (int)radius_row_.Slider().GetValue(); RefreshFromConfig(); };
        frame_width_row_.WhenAction = [=] { cfg_.frame_width = (int)frame_width_row_.Slider().GetValue(); RefreshFromConfig(); };
        margin_x_row_.WhenAction = [=] { cfg_.margin_x = (int)margin_x_row_.Slider().GetValue(); RefreshFromConfig(); };
        margin_y_row_.WhenAction = [=] { cfg_.margin_y = (int)margin_y_row_.Slider().GetValue(); RefreshFromConfig(); };
        enabled_row_.Toggle().WhenAction = [=] { cfg_.enabled = enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
        readonly_row_.Toggle().WhenAction = [=] { cfg_.readonly = readonly_row_.Toggle().IsOn(); RefreshFromConfig(); };
        edit_.WhenAction = [=] { cfg_.text = edit_.GetData().ToString(); RefreshFromConfig(); };
        edit_.WhenChange = [=] { cfg_.text = edit_.GetData().ToString(); RefreshState(); };

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
        int w = min(DPI(340), canvas.GetWidth() - DPI(40));
        int h = DPI(36);
        int x = canvas.left + (canvas.GetWidth() - w) / 2;
        int y = canvas.top + (canvas.GetHeight() - h) / 2;
        edit_.SetRect(x, y, w, h);
    }

private:
    struct EnumOption { const char* label; int value; };

    void PopulateDropdown(UiDropdown& drop, const EnumOption* opts, int count)
    {
        drop.UseInternalModel();
        drop.Clear();
        for(int i = 0; i < count; i++)
            drop.Add(opts[i].label, opts[i].value);
    }

    void RefreshState()
    {
        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_text_value_.SetText(edit_.GetData().ToString());
        state_align_value_.SetText(cfg_.align == UiAlign::CENTER ? "Center" : cfg_.align == UiAlign::RIGHT ? "Right" : "Left");
        state_mode_value_.SetText(!cfg_.enabled ? "Disabled" : (cfg_.readonly ? "ReadOnly" : "Editable"));
    }

    void RefreshFromConfig()
    {
        UiBaseEdit::Style style = MakeEditStyle(Palette());
        style.metrics.radius = cfg_.radius;
        style.metrics.frame_width = cfg_.frame_width;
        style.metrics.content_margin = Rect(cfg_.margin_x, cfg_.margin_y, cfg_.margin_x, cfg_.margin_y);

        edit_.SetCustomStyle(style);
        edit_.SetData(cfg_.text);
        edit_.SetPlaceholder(cfg_.placeholder);
        edit_.SetTextAlign(cfg_.align);
        edit_.SetEditable(!cfg_.readonly);
        edit_.Enable(cfg_.enabled);

        align_drop_.SelectByData((int)cfg_.align);
        radius_row_.Slider().SetValue(cfg_.radius);
        frame_width_row_.Slider().SetValue(cfg_.frame_width);
        margin_x_row_.Slider().SetValue(cfg_.margin_x);
        margin_y_row_.Slider().SetValue(cfg_.margin_y);
        enabled_row_.Toggle().SetOn(cfg_.enabled);
        readonly_row_.Toggle().SetOn(cfg_.readonly);

        radius_row_.SetValueText(AsString(cfg_.radius) + "px");
        frame_width_row_.SetValueText(AsString(cfg_.frame_width) + "px");
        margin_x_row_.SetValueText(AsString(cfg_.margin_x) + "px");
        margin_y_row_.SetValueText(AsString(cfg_.margin_y) + "px");

        SetUsageCode(BuildUsageCode());
        RefreshState();
        Preview().Refresh();
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiLineEdit edit;\n";
        code << "UiBaseEdit::Style style = UiTheme::ResolveEdit();\n";
        code << "style.metrics.radius = " << cfg_.radius << ";\n";
        code << "style.metrics.frame_width = " << cfg_.frame_width << ";\n";
        code << "style.metrics.content_margin = Rect(" << cfg_.margin_x << ", " << cfg_.margin_y << ", " << cfg_.margin_x << ", " << cfg_.margin_y << ");\n";
        code << "edit.SetCustomStyle(style)\n";
        code << "    .SetData(" << QuoteCpp(cfg_.text) << ")\n";
        code << "    .SetPlaceholder(" << QuoteCpp(cfg_.placeholder) << ")\n";
        code << "    .SetTextAlign(UiAlign::" << (cfg_.align == UiAlign::CENTER ? "CENTER" : cfg_.align == UiAlign::RIGHT ? "RIGHT" : "LEFT") << ")";
        if(cfg_.readonly)
            code << "\n    .SetReadOnly()";
        code << ";\n";
        if(!cfg_.enabled)
            code << "edit.Enable(false);\n";
        return code;
    }

    LineEditConfig cfg_;
    UiLineEdit edit_;

    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_text_row_ { UiBoxLayout::Direction::H }, state_align_row_ { UiBoxLayout::Direction::H }, state_mode_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_text_label_, state_text_value_, state_align_label_, state_align_value_, state_mode_label_, state_mode_value_;

    UiBoxLayout text_row_box_ { UiBoxLayout::Direction::H }, placeholder_row_box_ { UiBoxLayout::Direction::H }, align_row_box_ { UiBoxLayout::Direction::H };
    UiLabel text_label_, placeholder_label_, align_label_;
    UiLineEdit text_edit_, placeholder_edit_;
    UiDropdown align_drop_;
    DemoSliderRow radius_row_, frame_width_row_, margin_x_row_, margin_y_row_;
    DemoToggleRow enabled_row_, readonly_row_;
};

}

GUI_APP_MAIN
{
    UiLineEditBuilder demo;
    demo.Run();
}

