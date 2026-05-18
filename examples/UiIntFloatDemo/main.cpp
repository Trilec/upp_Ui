/*
    UiIntFloatDemo
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

struct NumericEditConfig {
    int int_value = 20;
    int int_min = -100;
    int int_max = 100;
    int int_step = 5;
    bool int_spin = true;
    bool int_loop = false;
    double float_value = 1.5;
    double float_min = -10.0;
    double float_max = 10.0;
    double float_step = 0.25;
    int float_precision = 3;
    bool float_spin = true;
};

class UiIntFloatBuilder : public BuilderWindowBase {
public:
    typedef UiIntFloatBuilder CLASSNAME;

    UiIntFloatBuilder()
        : BuilderWindowBase("UiIntFloatDemo", "U++ UiIntEdit / UiFloatEdit Builder",
                            "Validate numeric edit ranges, spin buttons, step size, precision, and data binding.")
    {
        Preview().Add(int_label_);
        Preview().Add(int_edit_);
        Preview().Add(float_label_);
        Preview().Add(float_edit_);
        Preview().Add(sync_btn_);

        AddStateRow(StateBox(), state_int_row_, state_int_label_, state_int_value_, "Integer");
        AddStateRow(StateBox(), state_float_row_, state_float_label_, state_float_value_, "Float");
        AddStateRow(StateBox(), state_range_row_, state_range_label_, state_range_value_, "Ranges");
        AddStateRow(StateBox(), state_spin_row_, state_spin_label_, state_spin_value_, "Spin");

        AddSliderRow(PropsBox(), int_value_row_, "Int value", AsString(cfg_.int_value));
        AddSliderRow(PropsBox(), int_min_row_, "Int min", AsString(cfg_.int_min));
        AddSliderRow(PropsBox(), int_max_row_, "Int max", AsString(cfg_.int_max));
        AddSliderRow(PropsBox(), int_step_row_, "Int step", AsString(cfg_.int_step));
        AddToggleRow(PropsBox(), int_spin_row_, "Int spin");
        AddToggleRow(PropsBox(), int_loop_row_, "Int loop");
        AddSliderRow(PropsBox(), float_value_row_, "Float value", AsString(cfg_.float_value));
        AddSliderRow(PropsBox(), float_step_row_, "Float step", AsString(cfg_.float_step));
        AddSliderRow(PropsBox(), float_precision_row_, "Precision", AsString(cfg_.float_precision));
        AddToggleRow(PropsBox(), float_spin_row_, "Float spin");

        int_value_row_.Slider().SetRange(-100, 100).SetStep(1);
        int_min_row_.Slider().SetRange(-200, 0).SetStep(1);
        int_max_row_.Slider().SetRange(0, 200).SetStep(1);
        int_step_row_.Slider().SetRange(1, 25).SetStep(1);
        float_value_row_.Slider().SetRange(-100, 100).SetStep(1);
        float_step_row_.Slider().SetRange(1, 100).SetStep(1);
        float_precision_row_.Slider().SetRange(0, 6).SetStep(1);

        int_value_row_.WhenAction = [=] { cfg_.int_value = (int)int_value_row_.Slider().GetValue(); RefreshFromConfig(); };
        int_min_row_.WhenAction = [=] { cfg_.int_min = (int)int_min_row_.Slider().GetValue(); RefreshFromConfig(); };
        int_max_row_.WhenAction = [=] { cfg_.int_max = (int)int_max_row_.Slider().GetValue(); RefreshFromConfig(); };
        int_step_row_.WhenAction = [=] { cfg_.int_step = (int)int_step_row_.Slider().GetValue(); RefreshFromConfig(); };
        int_spin_row_.Toggle().WhenAction = [=] { cfg_.int_spin = int_spin_row_.Toggle().IsOn(); RefreshFromConfig(); };
        int_loop_row_.Toggle().WhenAction = [=] { cfg_.int_loop = int_loop_row_.Toggle().IsOn(); RefreshFromConfig(); };
        float_value_row_.WhenAction = [=] { cfg_.float_value = float_value_row_.Slider().GetValue() / 10.0; RefreshFromConfig(); };
        float_step_row_.WhenAction = [=] { cfg_.float_step = max(0.1, float_step_row_.Slider().GetValue() / 20.0); RefreshFromConfig(); };
        float_precision_row_.WhenAction = [=] { cfg_.float_precision = (int)float_precision_row_.Slider().GetValue(); RefreshFromConfig(); };
        float_spin_row_.Toggle().WhenAction = [=] { cfg_.float_spin = float_spin_row_.Toggle().IsOn(); RefreshFromConfig(); };

        int_edit_.WhenChange = [=] { cfg_.int_value = int_edit_.GetValue(); RefreshState(); };
        float_edit_.WhenChange = [=] { cfg_.float_value = float_edit_.GetValue(); RefreshState(); };
        sync_btn_.SetText("Copy float to int");
        sync_btn_.WhenAction = [=] {
            cfg_.int_value = (int)float_edit_.GetValue();
            RefreshFromConfig();
        };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        int_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        float_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        sync_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    }

    virtual void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        int label_w = DPI(110);
        int edit_w = DPI(260);
        int h = DPI(32);
        int gap = DPI(14);
        int row_w = label_w + gap + edit_w;
        int x = canvas.left + (canvas.GetWidth() - row_w) / 2;
        int y = canvas.top + max(0, canvas.GetHeight() - DPI(150)) / 2;

        int_label_.SetRect(x, y, label_w, h);
        int_edit_.SetRect(x + label_w + gap, y, edit_w, h);
        y += DPI(48);
        float_label_.SetRect(x, y, label_w, h);
        float_edit_.SetRect(x + label_w + gap, y, edit_w, h);
        y += DPI(50);
        sync_btn_.SetRect(x + label_w + gap, y, DPI(170), DPI(30));
    }

private:
    void NormalizeRanges()
    {
        if(cfg_.int_min >= cfg_.int_max)
            cfg_.int_max = cfg_.int_min + 1;
        cfg_.int_value = minmax(cfg_.int_value, cfg_.int_min, cfg_.int_max);
    }

    void RefreshState()
    {
        state_int_value_.SetText(AsString(int_edit_.GetValue()));
        state_float_value_.SetText(FormatDoubleFix(float_edit_.GetValue(), cfg_.float_precision));
        state_range_value_.SetText(Format("%d..%d / %.1f..%.1f", cfg_.int_min, cfg_.int_max, cfg_.float_min, cfg_.float_max));
        state_spin_value_.SetText(Format("int %s, float %s", cfg_.int_spin ? "on" : "off", cfg_.float_spin ? "on" : "off"));
    }

    void RefreshFromConfig()
    {
        NormalizeRanges();

        int_label_.SetText("Integer");
        float_label_.SetText("Float");

        int_edit_.MinMax(cfg_.int_min, cfg_.int_max).Step(cfg_.int_step).ShowSpin(cfg_.int_spin).Loop(cfg_.int_loop);
        int_edit_.SetValue(cfg_.int_value);
        int_edit_.SetPlaceholder("int");

        float_edit_.MinMax(cfg_.float_min, cfg_.float_max).Step(cfg_.float_step).Precision(cfg_.float_precision).ShowSpin(cfg_.float_spin);
        float_edit_.SetValue(cfg_.float_value);
        float_edit_.SetPlaceholder("float");

        int_value_row_.Slider().SetValue(cfg_.int_value);
        int_min_row_.Slider().SetValue(cfg_.int_min);
        int_max_row_.Slider().SetValue(cfg_.int_max);
        int_step_row_.Slider().SetValue(cfg_.int_step);
        int_spin_row_.Toggle().SetOn(cfg_.int_spin);
        int_loop_row_.Toggle().SetOn(cfg_.int_loop);
        float_value_row_.Slider().SetValue(cfg_.float_value * 10.0);
        float_step_row_.Slider().SetValue(cfg_.float_step * 20.0);
        float_precision_row_.Slider().SetValue(cfg_.float_precision);
        float_spin_row_.Toggle().SetOn(cfg_.float_spin);

        int_value_row_.SetValueText(AsString(cfg_.int_value));
        int_min_row_.SetValueText(AsString(cfg_.int_min));
        int_max_row_.SetValueText(AsString(cfg_.int_max));
        int_step_row_.SetValueText(AsString(cfg_.int_step));
        float_value_row_.SetValueText(FormatDoubleFix(cfg_.float_value, cfg_.float_precision));
        float_step_row_.SetValueText(FormatDoubleFix(cfg_.float_step, 2));
        float_precision_row_.SetValueText(AsString(cfg_.float_precision));

        SetUsageCode(BuildUsageCode());
        RefreshState();
        Preview().Refresh();
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiIntEdit int_edit;\n";
        code << "int_edit.MinMax(" << cfg_.int_min << ", " << cfg_.int_max << ")\n";
        code << "        .Step(" << cfg_.int_step << ")\n";
        code << "        .ShowSpin(" << (cfg_.int_spin ? "true" : "false") << ")\n";
        code << "        .Loop(" << (cfg_.int_loop ? "true" : "false") << ");\n";
        code << "int_edit.SetValue(" << cfg_.int_value << ");\n\n";
        code << "UiFloatEdit float_edit;\n";
        code << "float_edit.MinMax(" << cfg_.float_min << ", " << cfg_.float_max << ")\n";
        code << "          .Step(" << cfg_.float_step << ")\n";
        code << "          .Precision(" << cfg_.float_precision << ")\n";
        code << "          .ShowSpin(" << (cfg_.float_spin ? "true" : "false") << ");\n";
        code << "float_edit.SetValue(" << cfg_.float_value << ");\n";
        return code;
    }

    NumericEditConfig cfg_;
    UiLabel int_label_, float_label_;
    UiIntEdit int_edit_;
    UiFloatEdit float_edit_;
    UiButton sync_btn_;

    UiBoxLayout state_int_row_ { UiBoxLayout::Direction::H }, state_float_row_ { UiBoxLayout::Direction::H }, state_range_row_ { UiBoxLayout::Direction::H }, state_spin_row_ { UiBoxLayout::Direction::H };
    UiLabel state_int_label_, state_int_value_, state_float_label_, state_float_value_, state_range_label_, state_range_value_, state_spin_label_, state_spin_value_;

    UiCompositeSlider int_value_row_, int_min_row_, int_max_row_, int_step_row_, float_value_row_, float_step_row_, float_precision_row_;
    UiCompositeToggle int_spin_row_, int_loop_row_, float_spin_row_;
};

}

GUI_APP_MAIN
{
    UiIntFloatBuilder demo;
    demo.Run();
}
