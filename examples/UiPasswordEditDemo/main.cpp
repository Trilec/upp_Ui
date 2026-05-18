/*
    UiPasswordEditDemo
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

struct PasswordConfig {
    String text = "hunter2";
    String placeholder = "Password";
    int radius = DPI(8);
    int frame_width = 1;
    bool enabled = true;
    bool readonly = false;
    bool show_visibility = true;
    bool plain_visible = false;
    int mask_mode = 0;
};

class UiPasswordEditBuilder : public BuilderWindowBase {
public:
    typedef UiPasswordEditBuilder CLASSNAME;

    UiPasswordEditBuilder()
        : BuilderWindowBase("UiPasswordEditDemo", "U++ UiPasswordEdit Builder", "Inspect masking, reveal, placeholder, and password field styling from one shell.")
    {
        Preview().Add(edit_);

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_mode_row_, state_mode_label_, state_mode_value_, "Visibility");
        AddStateRow(StateBox(), state_mask_row_, state_mask_label_, state_mask_value_, "Mask");
        AddStateRow(StateBox(), state_text_row_, state_text_label_, state_text_value_, "Value");

        AddEditRow(PropsBox(), text_row_box_, text_label_, text_edit_, "Text");
        AddEditRow(PropsBox(), placeholder_row_box_, placeholder_label_, placeholder_edit_, "Placeholder");
        AddDropdownRow(PropsBox(), mask_row_box_, mask_label_, mask_drop_, "Mask Char");
        AddSliderRow(PropsBox(), radius_row_, "Radius", "8px");
        AddSliderRow(PropsBox(), frame_width_row_, "Frame W", "1px");
        AddToggleRow(PropsBox(), enabled_row_, "Enabled");
        AddToggleRow(PropsBox(), readonly_row_, "Read Only");
        AddToggleRow(PropsBox(), visibility_row_, "Reveal Icon");
        AddToggleRow(PropsBox(), plain_row_, "Plain Visible");

        const EnumOption masks[] = { { "Bullet", 0 }, { "Asterisk", 1 }, { "Hash", 2 } };
        PopulateDropdown(mask_drop_, masks, 3);

        text_edit_.SetData(cfg_.text);
        placeholder_edit_.SetData(cfg_.placeholder);
        radius_row_.Slider().SetRange(0, DPI(18)).SetStep(1).SetValue(cfg_.radius);
        frame_width_row_.Slider().SetRange(0, 4).SetStep(1).SetValue(cfg_.frame_width);

        text_edit_.WhenChange = [=] { cfg_.text = text_edit_.GetData().ToString(); RefreshFromConfig(); };
        placeholder_edit_.WhenChange = [=] { cfg_.placeholder = placeholder_edit_.GetData().ToString(); RefreshFromConfig(); };
        mask_drop_.WhenSelect = [=](int) { cfg_.mask_mode = (int)mask_drop_.GetSelectedData(); RefreshFromConfig(); };
        radius_row_.WhenAction = [=] { cfg_.radius = (int)radius_row_.Slider().GetValue(); RefreshFromConfig(); };
        frame_width_row_.WhenAction = [=] { cfg_.frame_width = (int)frame_width_row_.Slider().GetValue(); RefreshFromConfig(); };
        enabled_row_.Toggle().WhenAction = [=] { cfg_.enabled = enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
        readonly_row_.Toggle().WhenAction = [=] { cfg_.readonly = readonly_row_.Toggle().IsOn(); RefreshFromConfig(); };
        visibility_row_.Toggle().WhenAction = [=] { cfg_.show_visibility = visibility_row_.Toggle().IsOn(); RefreshFromConfig(); };
        plain_row_.Toggle().WhenAction = [=] { cfg_.plain_visible = plain_row_.Toggle().IsOn(); RefreshFromConfig(); };
        edit_.WhenAction = [=] { cfg_.text = edit_.GetText().ToString(); RefreshFromConfig(); };
        edit_.WhenToggleVisible = [=](bool on) { cfg_.plain_visible = on; RefreshState(); };

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

    wchar MaskChar() const
    {
        if(cfg_.mask_mode == 1) return '*';
        if(cfg_.mask_mode == 2) return '#';
        return 0x25CF;
    }

    String MaskLabel() const
    {
        if(cfg_.mask_mode == 1) return "*";
        if(cfg_.mask_mode == 2) return "#";
        return "Bullet";
    }

    void RefreshState()
    {
        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_mode_value_.SetText(cfg_.plain_visible ? "Visible" : "Masked");
        state_mask_value_.SetText(MaskLabel());
        state_text_value_.SetText(cfg_.plain_visible ? cfg_.text : String("������"));
    }

    void RefreshFromConfig()
    {
        UiBaseEdit::Style style = MakeEditStyle(Palette());
        style.metrics.radius = cfg_.radius;
        style.metrics.frame_width = cfg_.frame_width;
        edit_.SetCustomStyle(style);
        edit_.SetText(WString(cfg_.text));
        edit_.SetPlaceholder(cfg_.placeholder);
        edit_.SetPasswordChar(MaskChar());
        edit_.EnableVisibilityIcon(cfg_.show_visibility);
        edit_.SetPlainTextVisible(cfg_.plain_visible);
        edit_.SetEditable(!cfg_.readonly);
        edit_.Enable(cfg_.enabled);

        mask_drop_.SelectByData(cfg_.mask_mode);
        radius_row_.Slider().SetValue(cfg_.radius);
        frame_width_row_.Slider().SetValue(cfg_.frame_width);
        enabled_row_.Toggle().SetOn(cfg_.enabled);
        readonly_row_.Toggle().SetOn(cfg_.readonly);
        visibility_row_.Toggle().SetOn(cfg_.show_visibility);
        plain_row_.Toggle().SetOn(cfg_.plain_visible);
        radius_row_.SetValueText(AsString(cfg_.radius) + "px");
        frame_width_row_.SetValueText(AsString(cfg_.frame_width) + "px");

        SetUsageCode(BuildUsageCode());
        RefreshState();
        Preview().Refresh();
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiPasswordEdit pass;\n";
        code << "UiBaseEdit::Style style = UiTheme::ResolveEdit();\n";
        code << "style.metrics.radius = " << cfg_.radius << ";\n";
        code << "style.metrics.frame_width = " << cfg_.frame_width << ";\n";
        code << "pass.SetCustomStyle(style)\n";
        code << "    .SetPlaceholder(" << QuoteCpp(cfg_.placeholder) << ")\n";
        code << "    .SetPasswordChar(" << (int)MaskChar() << ")\n";
        if(cfg_.show_visibility)
            code << "    .EnableVisibilityIcon(true)\n";
        if(cfg_.plain_visible)
            code << "    .SetPlainTextVisible(true)";
        code << ";\n";
        if(cfg_.readonly)
            code << "pass.SetEditable(false);\n";
        if(!cfg_.enabled)
            code << "pass.Enable(false);\n";
        return code;
    }

    PasswordConfig cfg_;
    UiPasswordEdit edit_;

    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_mode_row_ { UiBoxLayout::Direction::H }, state_mask_row_ { UiBoxLayout::Direction::H }, state_text_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_mode_label_, state_mode_value_, state_mask_label_, state_mask_value_, state_text_label_, state_text_value_;

    UiBoxLayout text_row_box_ { UiBoxLayout::Direction::H }, placeholder_row_box_ { UiBoxLayout::Direction::H }, mask_row_box_ { UiBoxLayout::Direction::H };
    UiLabel text_label_, placeholder_label_, mask_label_;
    UiLineEdit text_edit_, placeholder_edit_;
    UiDropdown mask_drop_;
    UiCompositeSlider radius_row_, frame_width_row_;
    UiCompositeToggle enabled_row_, readonly_row_, visibility_row_, plain_row_;
};

}

GUI_APP_MAIN
{
    UiPasswordEditBuilder demo;
    demo.Run();
}
