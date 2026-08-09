/*
    UiRangeSliderDemo
    -----------------

    Purpose
    - Active UiRangeSlider demo and visual smoke test using the shared builder shell.

    Changelog
    - 2026-08: initial range-slider builder demo.
*/

#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

struct RangeSliderConfig {
    int orientation = 0;
    int lower = 20;
    int upper = 80;
    int step = 1;
    bool ticks = true;
    int major_ticks = 11;
};

class UiRangeSliderBuilder : public BuilderWindowBase {
public:
    typedef UiRangeSliderBuilder CLASSNAME;

    UiRangeSliderBuilder()
        : BuilderWindowBase("UiRangeSliderDemo",
                            "U++ UiRangeSlider Builder",
                            "Inspect a two-handle interval selector and its direct-entry UiRangeSliderEdit composition, using the standard UiSlider theme.")
    {
        Preview().Add(range_);
        Preview().Add(range_edit_);

        AddStateRow(StateBox(), state_domain_row_, state_domain_label_, state_domain_value_, "Domain");
        AddStateRow(StateBox(), state_values_row_, state_values_label_, state_values_value_, "Selection");
        AddStateRow(StateBox(), state_alias_row_, state_alias_label_, state_alias_value_, "Start / End");
        AddStateRow(StateBox(), state_active_row_, state_active_label_, state_active_value_, "Active");

        AddDropdownRow(PropsBox(), orientation_row_, orientation_label_, orientation_drop_, "Orientation");
        AddSliderRow(PropsBox(), lower_row_, "Lower", "20");
        AddSliderRow(PropsBox(), upper_row_, "Upper", "80");
        AddSliderRow(PropsBox(), step_row_, "Step", "1");
        AddToggleRow(PropsBox(), ticks_row_, "Ticks");
        AddSliderRow(PropsBox(), tick_count_row_, "Major Ticks", "11");

        orientation_drop_.UseInternalModel();
        orientation_drop_.Clear();
        orientation_drop_.Add("Horizontal", 0);
        orientation_drop_.Add("Vertical", 1);

        lower_row_.Slider().SetRange(0, 100).SetStep(1).SetValue(cfg_.lower);
        upper_row_.Slider().SetRange(0, 100).SetStep(1).SetValue(cfg_.upper);
        step_row_.Slider().SetRange(1, 10).SetStep(1).SetValue(cfg_.step);
        tick_count_row_.Slider().SetRange(2, 21).SetStep(1).SetValue(cfg_.major_ticks);
        ticks_row_.Toggle().SetOn(cfg_.ticks);

        orientation_drop_.WhenSelect = [=](int) {
            cfg_.orientation = (int)orientation_drop_.GetSelectedData();
            RefreshFromConfig();
        };

        auto lower_apply = [=] {
            cfg_.lower = (int)lower_row_.Slider().GetValue();
            cfg_.lower = min(cfg_.lower, cfg_.upper);
            RefreshFromConfig();
        };
        lower_row_.WhenChanging = lower_apply;
        lower_row_.WhenAction = lower_apply;

        auto upper_apply = [=] {
            cfg_.upper = (int)upper_row_.Slider().GetValue();
            cfg_.upper = max(cfg_.upper, cfg_.lower);
            RefreshFromConfig();
        };
        upper_row_.WhenChanging = upper_apply;
        upper_row_.WhenAction = upper_apply;

        auto step_apply = [=] {
            cfg_.step = (int)step_row_.Slider().GetValue();
            RefreshFromConfig();
        };
        step_row_.WhenChanging = step_apply;
        step_row_.WhenAction = step_apply;

        ticks_row_.Toggle().WhenAction = [=] {
            cfg_.ticks = ticks_row_.Toggle().IsOn();
            RefreshFromConfig();
        };

        auto ticks_apply = [=] {
            cfg_.major_ticks = (int)tick_count_row_.Slider().GetValue();
            RefreshFromConfig();
        };
        tick_count_row_.WhenChanging = ticks_apply;
        tick_count_row_.WhenAction = ticks_apply;

        range_.WhenChanging = [=] { SyncFromSlider(); };
        range_.WhenAction = [=] { SyncFromSlider(); };
        range_edit_.WhenChanging = [=] { SyncFromEdit(); };
        range_edit_.WhenAction = [=] { SyncFromEdit(); };

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
        Rect c = Preview().GetCanvasRect();
        if(cfg_.orientation == 0) {
            int w = max(DPI(280), c.GetWidth() - DPI(60));
            int mid = c.CenterPoint().y;
            int block = DPI(78) + DPI(14) + DPI(46);
            int top = mid - block / 2;
            range_.SetRect(c.left + (c.GetWidth() - w) / 2,
                           top,
                           w, DPI(78));
            range_edit_.SetRect(c.left + (c.GetWidth() - w) / 2,
                                top + DPI(78) + DPI(14),
                                w, DPI(46));
        }
        else {
            int h = max(DPI(300), c.GetHeight() - DPI(100));
            int mid = c.CenterPoint().y;
            int top = mid - h / 2;
            int slider_w = DPI(84);
            int edit_w = max(DPI(260), c.GetWidth() - DPI(120));
            int left = c.left + (c.GetWidth() - (slider_w + DPI(24) + edit_w)) / 2;
            range_.SetRect(left, top, slider_w, h);
            range_edit_.SetRect(left + slider_w + DPI(24), top, edit_w, h);
        }
    }

private:
    void RefreshFromConfig()
    {
        range_.ClearCustomStyle();
        range_.SetDirection(cfg_.orientation == 0 ? UiDirection::H : UiDirection::V)
              .SetRange(0, 100)
              .SetStep(cfg_.step)
              .SetStartEnd(cfg_.lower, cfg_.upper)
              .SetTicks(cfg_.ticks, cfg_.major_ticks, 0);

        range_edit_.SetDirection(cfg_.orientation == 0 ? UiDirection::H : UiDirection::V)
                   .SetRange(0, 100)
                   .SetStep(cfg_.step)
                   .SetFieldWidth(DPI(92))
                   .SetGap(DPI(14))
                   .SetStartEnd(cfg_.lower, cfg_.upper);

        cfg_.lower = (int)range_.GetLowerValue();
        cfg_.upper = (int)range_.GetUpperValue();

        orientation_drop_.SelectByData(cfg_.orientation);
        lower_row_.Slider().SetValue(cfg_.lower);
        upper_row_.Slider().SetValue(cfg_.upper);
        step_row_.Slider().SetValue(cfg_.step);
        ticks_row_.Toggle().SetOn(cfg_.ticks);
        tick_count_row_.Slider().SetValue(cfg_.major_ticks);

        lower_row_.SetValueText(AsString(cfg_.lower));
        upper_row_.SetValueText(AsString(cfg_.upper));
        step_row_.SetValueText(AsString(cfg_.step));
        tick_count_row_.SetValueText(AsString(cfg_.major_ticks));

        SyncStateAndCode();
        LayoutPreviewContent();
        Preview().Refresh();
    }

    void SyncFromSlider()
    {
        if(syncing_)
            return;
        cfg_.lower = (int)range_.GetLowerValue();
        cfg_.upper = (int)range_.GetUpperValue();
        PushConfig();
    }

    void SyncFromEdit()
    {
        if(syncing_)
            return;
        cfg_.lower = (int)range_edit_.GetLowerValue();
        cfg_.upper = (int)range_edit_.GetUpperValue();
        PushConfig();
    }

    void PushConfig()
    {
        if(syncing_)
            return;
        syncing_ = true;
        range_.SetStartEnd(cfg_.lower, cfg_.upper);
        range_edit_.SetStartEnd(cfg_.lower, cfg_.upper);
        lower_row_.Slider().SetValue(cfg_.lower);
        upper_row_.Slider().SetValue(cfg_.upper);
        lower_row_.SetValueText(AsString(cfg_.lower));
        upper_row_.SetValueText(AsString(cfg_.upper));
        syncing_ = false;
        SyncStateAndCode();
    }

    void SyncStateAndCode()
    {
        state_domain_value_.SetText("0 .. 100");
        state_values_value_.SetText(Format("%d .. %d", cfg_.lower, cfg_.upper));
        state_alias_value_.SetText(Format("%d / %d", (int)range_.GetStart(), (int)range_.GetEnd()));
        state_active_value_.SetText(range_.GetActiveHandle() == UiRangeSlider::Handle::Lower ? "Lower" : "Upper");
        SetUsageCode(BuildUsageCode());
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiRangeSlider range;\n";
        code << "range.SetDirection(UiDirection::" << (cfg_.orientation == 0 ? "H" : "V") << ")\n";
        code << "     .SetRange(0, 100)\n";
        code << "     .SetStep(" << cfg_.step << ")\n";
        code << "     .SetStartEnd(" << cfg_.lower << ", " << cfg_.upper << ")\n";
        code << "     .SetTicks(" << (cfg_.ticks ? "true" : "false") << ", " << cfg_.major_ticks << ", 0);\n\n";
        code << "range.WhenChanging = [&] {\n";
        code << "    double start = range.GetStart();\n";
        code << "    double end = range.GetEnd();\n";
        code << "    // Preview the selected interval.\n";
        code << "};\n\n";
        code << "range.WhenAction = [&] {\n";
        code << "    // Commit the selected interval.\n";
        code << "};\n";
        return code;
    }

    RangeSliderConfig cfg_;
    bool syncing_ = false;
    UiRangeSlider range_;
    UiRangeSliderEdit range_edit_;

    UiBoxLayout state_domain_row_ { UiDirection::H };
    UiLabel state_domain_label_;
    UiLabel state_domain_value_;
    UiBoxLayout state_values_row_ { UiDirection::H };
    UiLabel state_values_label_;
    UiLabel state_values_value_;
    UiBoxLayout state_alias_row_ { UiDirection::H };
    UiLabel state_alias_label_;
    UiLabel state_alias_value_;
    UiBoxLayout state_active_row_ { UiDirection::H };
    UiLabel state_active_label_;
    UiLabel state_active_value_;

    UiBoxLayout orientation_row_ { UiDirection::H };
    UiLabel orientation_label_;
    UiDropdown orientation_drop_;
    DemoSliderRow lower_row_;
    DemoSliderRow upper_row_;
    DemoSliderRow step_row_;
    DemoToggleRow ticks_row_;
    DemoSliderRow tick_count_row_;
};

}

GUI_APP_MAIN
{
    UiRangeSliderBuilder().Run();
}
