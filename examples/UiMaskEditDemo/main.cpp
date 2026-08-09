/*
    UiMaskEditDemo
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

struct MaskCase {
    const char *name;
    const char *mask;
    const char *sample;
    const char *tip;
};

const MaskCase kMaskCases[] = {
    { "Phone", "(###) ###-####", "(123) 456-7890", "Classic digit mask" },
    { "Date", "##/##/####", "12/31/2026", "Mask plus semantic date validation" },
    { "ZIP", "#####", "90210", "Five digit numeric mask" },
    { "Plate", "UUU-####", "ABC-1234", "Auto-capitalized letters" },
    { "Username", "", "Open_Ui_Designer", "Formatter only, no fixed mask" },
};

class UiMaskEditBuilder : public BuilderWindowBase {
public:
    typedef UiMaskEditBuilder CLASSNAME;

    UiMaskEditBuilder()
        : BuilderWindowBase("UiMaskEditDemo", "U++ UiMaskEdit Builder",
                            "Test masks, semantic validators, formatters, placeholder text, and validation feedback.")
    {
        for(int i = 0; i < 5; i++) {
            Preview().Add(labels_[i]);
            Preview().Add(edits_[i]);
        }

        AddStateRow(StateBox(), state_case_row_, state_case_label_, state_case_value_, "Case");
        AddStateRow(StateBox(), state_value_row_, state_value_label_, state_value_value_, "Value");
        AddStateRow(StateBox(), state_valid_row_, state_valid_label_, state_valid_value_, "Valid");
        AddStateRow(StateBox(), state_mask_row_, state_mask_label_, state_mask_value_, "Mask");

        AddDropdownRow(PropsBox(), case_row_, case_label_, case_drop_, "Case");
        PropsBox().Add(sample_row_).Fit();
        sample_row_.SetLabel("Sample").SetData(kMaskCases[0].sample);
        sample_row_.Edit().WhenChange = [=] {
            sample_override_ = sample_row_.GetData().ToString();
            RefreshFromConfig();
        };
        AddToggleRow(PropsBox(), live_validation_row_, "Live validation");
        AddToggleRow(PropsBox(), success_flash_row_, "Flash on action");

        case_drop_.UseInternalModel();
        for(int i = 0; i < __countof(kMaskCases); i++)
            case_drop_.Add(kMaskCases[i].name, i);
        case_drop_.WhenSelect = [=](int) {
            int selected = (int)case_drop_.GetSelectedData();
            if(selected >= 0 && selected < __countof(kMaskCases)) {
                active_case_ = selected;
                sample_override_.Clear();
                RefreshFromConfig();
            }
        };
        live_validation_row_.Toggle().WhenAction = [=] { live_validation_ = live_validation_row_.Toggle().IsOn(); RefreshFromConfig(); };
        success_flash_row_.Toggle().WhenAction = [=] { success_flash_ = success_flash_row_.Toggle().IsOn(); RefreshFromConfig(); };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        for(int i = 0; i < 5; i++)
            labels_[i].SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
    }

    virtual void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        int label_w = DPI(132);
        int edit_w = min(DPI(330), max(DPI(220), canvas.GetWidth() - label_w - DPI(92)));
        int h = DPI(32);
        int row_gap = DPI(18);
        int row_w = label_w + DPI(12) + edit_w;
        int total_h = 5 * h + 4 * row_gap;
        int x = canvas.left + (canvas.GetWidth() - row_w) / 2;
        int y = canvas.top + (canvas.GetHeight() - total_h) / 2;

        for(int i = 0; i < 5; i++) {
            labels_[i].SetRect(x, y, label_w, h);
            edits_[i].SetRect(x + label_w + DPI(12), y, edit_w, h);
            y += h + row_gap;
        }
    }

private:
    void SetupValidation(UiMaskEdit& edit)
    {
        edit.WhenChange = [=, &edit] {
            if(live_validation_)
                edit.ShowError(!edit.IsValid());
            RefreshState();
        };
        edit.WhenAction = [=, &edit] {
            bool valid = edit.IsValid();
            edit.ShowError(!valid);
            if(success_flash_) {
                if(valid)
                    edit.FlashSuccess();
                else
                    edit.FlashError();
            }
            RefreshState();
        };
    }

    void ConfigureEdit(int i)
    {
        const MaskCase& c = kMaskCases[i];
        labels_[i].SetText(c.name);
        edits_[i].SetTip(c.tip);
        edits_[i].SetPlaceholder(c.mask[0] ? c.mask : "formatter only");
        edits_[i].SetMask(c.mask);
        if(i == 1)
            edits_[i].SetValidator(UiMaskEdit::DateValidator());
        else if(i == 4) {
            edits_[i].SetFormatter(UiMaskEdit::UsernameFormatter());
            edits_[i].SetValidator(UiMaskEdit::AlnumUnderscoreValidator(true));
        }
        SetupValidation(edits_[i]);
    }

    void RefreshState()
    {
        const MaskCase& c = kMaskCases[active_case_];
        state_case_value_.SetText(c.name);
        state_value_value_.SetText(edits_[active_case_].GetData().ToString());
        state_valid_value_.SetText(edits_[active_case_].IsValid() ? "Valid" : "Invalid");
        state_mask_value_.SetText(c.mask[0] ? c.mask : "formatter only");
    }

    void RefreshFromConfig()
    {
        for(int i = 0; i < 5; i++)
            ConfigureEdit(i);

        String sample = sample_override_.IsEmpty() ? String(kMaskCases[active_case_].sample) : sample_override_;
        edits_[active_case_].SetData(sample);

        case_drop_.SelectByData(active_case_);
        sample_row_.SetData(sample);
        live_validation_row_.Toggle().SetOn(live_validation_);
        success_flash_row_.Toggle().SetOn(success_flash_);

        SetUsageCode(BuildUsageCode());
        RefreshState();
        Preview().Refresh();
    }

    String BuildUsageCode() const
    {
        const MaskCase& c = kMaskCases[active_case_];
        String code;
        code << "UiMaskEdit edit;\n";
        if(c.mask[0])
            code << "edit.SetMask(" << QuoteCpp(c.mask) << ");\n";
        if(active_case_ == 1)
            code << "edit.SetValidator(UiMaskEdit::DateValidator());\n";
        if(active_case_ == 4) {
            code << "edit.SetFormatter(UiMaskEdit::UsernameFormatter());\n";
            code << "edit.SetValidator(UiMaskEdit::AlnumUnderscoreValidator(true));\n";
        }
        code << "edit.SetPlaceholder(" << QuoteCpp(c.mask[0] ? c.mask : "formatter only") << ");\n";
        code << "edit.WhenAction = [=] {\n";
        code << "    edit.ShowError(!edit.IsValid());\n";
        code << "};\n";
        return code;
    }

    int active_case_ = 0;
    bool live_validation_ = true;
    bool success_flash_ = true;
    String sample_override_;

    UiLabel labels_[5];
    UiMaskEdit edits_[5];

    UiBoxLayout state_case_row_ { UiBoxLayout::Direction::H }, state_value_row_ { UiBoxLayout::Direction::H }, state_valid_row_ { UiBoxLayout::Direction::H }, state_mask_row_ { UiBoxLayout::Direction::H };
    UiLabel state_case_label_, state_case_value_, state_value_label_, state_value_value_, state_valid_label_, state_valid_value_, state_mask_label_, state_mask_value_;

    UiBoxLayout case_row_ { UiBoxLayout::Direction::H };
    UiLabel case_label_;
    UiDropdown case_drop_;
    DemoEditRow sample_row_;
    DemoToggleRow live_validation_row_, success_flash_row_;
};

}

GUI_APP_MAIN
{
    UiMaskEditBuilder demo;
    demo.Run();
}
