/*
    UiRadioButtonDemo
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

enum RadioVisualMode {
    RADIO_CLASSIC = 0,
    RADIO_PILLS,
    RADIO_LIST,
};

struct RadioConfig {
    int visual = RADIO_CLASSIC;
    UiAlign indicator_side = UiAlign::LEFT;
    int indicator_size = DPI(18);
    int indicator_gap = DPI(8);
    int indicator_radius = DPI(9);
    int body_radius = DPI(8);
    int body_frame_width = 0;
    bool body_face = false;
    bool body_frame = false;
    bool enabled = true;
    Color text = Color(28, 47, 78);
    Color body_face_color = Color(236, 241, 248);
    Color body_frame_color = Color(211, 221, 237);
    Color indicator_face = White();
    Color indicator_frame = Color(117, 141, 182);
};

String RadioVisualName(int v)
{
    switch(v) {
    case RADIO_PILLS: return "Pills";
    case RADIO_LIST: return "List";
    default: return "Classic";
    }
}

class UiRadioButtonBuilder : public BuilderWindowBase {
public:
    typedef UiRadioButtonBuilder CLASSNAME;

    UiRadioButtonBuilder()
        : BuilderWindowBase("UiRadioButtonDemo", "U++ UiRadioButton Builder", "Inspect radio visuals, indicator geometry, and grouped exclusive selection from one shell.")
    {
        Preview().Add(r1_); Preview().Add(r2_); Preview().Add(r3_);
        r1_.SetText("Option A").SetGroup(1).SetChecked(true);
        r2_.SetText("Option B").SetGroup(1);
        r3_.SetText("Option C").SetGroup(1);
        r1_.WhenAction = [=] { SyncState(); };
        r2_.WhenAction = [=] { SyncState(); };
        r3_.WhenAction = [=] { SyncState(); };

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_visual_row_, state_visual_label_, state_visual_value_, "Visual");
        AddStateRow(StateBox(), state_checked_row_, state_checked_label_, state_checked_value_, "Checked");
        AddStateRow(StateBox(), state_side_row_, state_side_label_, state_side_value_, "Indicator");

        AddDropdownRow(PropsBox(), visual_row_box_, visual_label_, visual_drop_, "Visual");
        AddDropdownRow(PropsBox(), side_row_box_, side_label_, side_drop_, "Indicator");
        AddSliderRow(PropsBox(), indicator_size_row_, "Indicator Sz", "18px");
        AddSliderRow(PropsBox(), indicator_gap_row_, "Content Gap", "8px");
        AddSliderRow(PropsBox(), indicator_radius_row_, "Ind Radius", "9px");
        AddSliderRow(PropsBox(), body_radius_row_, "Body Radius", "8px");
        AddSliderRow(PropsBox(), body_frame_width_row_, "Body Frame", "0px");
        AddToggleRow(PropsBox(), body_face_row_, "Body Face");
        AddToggleRow(PropsBox(), body_frame_row_, "Body Frame On");
        AddToggleRow(PropsBox(), enabled_row_, "Enabled");
        AddColorRow(PropsBox(), text_row_, "Text");
        AddColorRow(PropsBox(), body_face_color_row_, "Body Face");
        AddColorRow(PropsBox(), body_frame_color_row_, "Body Frame");
        AddColorRow(PropsBox(), indicator_face_row_, "Ind Face");
        AddColorRow(PropsBox(), indicator_frame_row_, "Ind Frame");

        const EnumOption visuals[] = {
            { "Classic", RADIO_CLASSIC }, { "Pills", RADIO_PILLS }, { "List", RADIO_LIST }
        };
        const EnumOption sides[] = {
            { "Left", (int)UiAlign::LEFT }, { "Right", (int)UiAlign::RIGHT }
        };
        PopulateDropdown(visual_drop_, visuals, 3);
        PopulateDropdown(side_drop_, sides, 2);

        indicator_size_row_.Slider().SetRange(DPI(12), DPI(28)).SetStep(1).SetValue(cfg_.indicator_size);
        indicator_gap_row_.Slider().SetRange(0, DPI(20)).SetStep(1).SetValue(cfg_.indicator_gap);
        indicator_radius_row_.Slider().SetRange(0, DPI(16)).SetStep(1).SetValue(cfg_.indicator_radius);
        body_radius_row_.Slider().SetRange(0, DPI(20)).SetStep(1).SetValue(cfg_.body_radius);
        body_frame_width_row_.Slider().SetRange(0, 4).SetStep(1).SetValue(cfg_.body_frame_width);
        InitColorRow(text_row_, cfg_.text);
        InitColorRow(body_face_color_row_, cfg_.body_face_color);
        InitColorRow(body_frame_color_row_, cfg_.body_frame_color);
        InitColorRow(indicator_face_row_, cfg_.indicator_face);
        InitColorRow(indicator_frame_row_, cfg_.indicator_frame);

        visual_drop_.WhenSelect = [=](int) { cfg_.visual = (int)visual_drop_.GetSelectedData(); RefreshFromConfig(); };
        side_drop_.WhenSelect = [=](int) { cfg_.indicator_side = (UiAlign)(int)side_drop_.GetSelectedData(); RefreshFromConfig(); };
        indicator_size_row_.WhenAction = [=] { cfg_.indicator_size = (int)indicator_size_row_.Slider().GetValue(); RefreshFromConfig(); };
        indicator_gap_row_.WhenAction = [=] { cfg_.indicator_gap = (int)indicator_gap_row_.Slider().GetValue(); RefreshFromConfig(); };
        indicator_radius_row_.WhenAction = [=] { cfg_.indicator_radius = (int)indicator_radius_row_.Slider().GetValue(); RefreshFromConfig(); };
        body_radius_row_.WhenAction = [=] { cfg_.body_radius = (int)body_radius_row_.Slider().GetValue(); RefreshFromConfig(); };
        body_frame_width_row_.WhenAction = [=] { cfg_.body_frame_width = (int)body_frame_width_row_.Slider().GetValue(); RefreshFromConfig(); };
        body_face_row_.Toggle().WhenAction = [=] { cfg_.body_face = body_face_row_.Toggle().IsOn(); RefreshFromConfig(); };
        body_frame_row_.Toggle().WhenAction = [=] { cfg_.body_frame = body_frame_row_.Toggle().IsOn(); RefreshFromConfig(); };
        enabled_row_.Toggle().WhenAction = [=] { cfg_.enabled = enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
        text_row_.WhenAction = [=] { cfg_.text = text_row_.GetColor(0); RefreshFromConfig(); };
        body_face_color_row_.WhenAction = [=] { cfg_.body_face_color = body_face_color_row_.GetColor(0); RefreshFromConfig(); };
        body_frame_color_row_.WhenAction = [=] { cfg_.body_frame_color = body_frame_color_row_.GetColor(0); RefreshFromConfig(); };
        indicator_face_row_.WhenAction = [=] { cfg_.indicator_face = indicator_face_row_.GetColor(0); RefreshFromConfig(); };
        indicator_frame_row_.WhenAction = [=] { cfg_.indicator_frame = indicator_frame_row_.GetColor(0); RefreshFromConfig(); };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        UiLabel::Style body = MakeBodyLabelStyle(Palette());
        UiLabel::Style value = MakeValueLabelStyle(Palette());
        UiDropdown::Style dd = MakeDropdownStyle(Palette());
        state_theme_label_.SetCustomStyle(body); state_theme_value_.SetCustomStyle(value);
        state_visual_label_.SetCustomStyle(body); state_visual_value_.SetCustomStyle(value);
        state_checked_label_.SetCustomStyle(body); state_checked_value_.SetCustomStyle(value);
        state_side_label_.SetCustomStyle(body); state_side_value_.SetCustomStyle(value);
        visual_label_.SetCustomStyle(body); side_label_.SetCustomStyle(body);
        visual_drop_.SetCustomStyle(dd); side_drop_.SetCustomStyle(dd);
        indicator_size_row_.SetLabelStyle(body).SetValueStyle(value);
        indicator_gap_row_.SetLabelStyle(body).SetValueStyle(value);
        indicator_radius_row_.SetLabelStyle(body).SetValueStyle(value);
        body_radius_row_.SetLabelStyle(body).SetValueStyle(value);
        body_frame_width_row_.SetLabelStyle(body).SetValueStyle(value);
        body_face_row_.SetLabelStyle(body);
        body_frame_row_.SetLabelStyle(body);
        enabled_row_.SetLabelStyle(body);
        text_row_.SetLabelStyle(body);
        body_face_color_row_.SetLabelStyle(body);
        body_frame_color_row_.SetLabelStyle(body);
        indicator_face_row_.SetLabelStyle(body);
        indicator_frame_row_.SetLabelStyle(body);
    }

    virtual void LayoutPreviewContent() override
    {
        Rect c = Preview().GetCanvasRect();
        int x = c.left + DPI(48);
        int y = c.top + DPI(72);
        int w = max(DPI(220), c.GetWidth() - DPI(96));
        r1_.SetRect(x, y, w, DPI(34));
        r2_.SetRect(x, y + DPI(46), w, DPI(34));
        r3_.SetRect(x, y + DPI(92), w, DPI(34));
    }

private:
    struct EnumOption { const char* label; int value; };
    void AddColorRow(UiBoxLayout& target, UiCompositeColor& row, const char* name)
    {
        row.SetLabel(name).SetColorCount(1).ShowValue(false);
        target.Add(row).Fit();
    }
    void InitColorRow(UiCompositeColor& row, Color c) { row.SetColor(0, c); }
    void PopulateDropdown(UiDropdown& drop, const EnumOption* opts, int count)
    {
        drop.UseInternalModel();
        drop.Clear();
        for(int i = 0; i < count; i++)
            drop.Add(opts[i].label, opts[i].value);
    }
    UiRadioButton::Style BuildStyle() const
    {
        UiRadioButton::Style s = UiRadioButton::StyleDefault();
        for(int i = 0; i < 4; i++) {
            s.palette.ink[i] = cfg_.text;
            s.palette.face[i] = UiFill::Solid(cfg_.body_face_color);
            s.palette.frame[i] = cfg_.body_frame_color;
            s.indicator_palette.face[i] = UiFill::Solid(cfg_.indicator_face);
            s.indicator_palette.frame[i] = cfg_.indicator_frame;
        }
        s.metrics.face_enabled = cfg_.body_face;
        s.metrics.frame_enabled = cfg_.body_frame;
        s.metrics.frame_width = cfg_.body_frame_width;
        s.metrics.radius = cfg_.body_radius;
        s.indicator_metrics.face_enabled = true;
        s.indicator_metrics.frame_enabled = true;
        s.indicator_metrics.radius = cfg_.indicator_radius;
        s.indicator_side = cfg_.indicator_side;
        s.indicator_size = cfg_.indicator_size;
        s.indicator_gap = cfg_.indicator_gap;
        return s;
    }
    void ApplyTo(UiRadioButton& r)
    {
        r.SetCustomStyle(BuildStyle())
         .SetVisual((UiRadioVisual)cfg_.visual)
         .SetIndicatorSide(cfg_.indicator_side)
         .SetIndicatorRadius(cfg_.indicator_radius);
        r.Enable(cfg_.enabled);
    }
    void RefreshFromConfig()
    {
        visual_drop_.SelectByData(cfg_.visual);
        side_drop_.SelectByData((int)cfg_.indicator_side);
        indicator_size_row_.Slider().SetValue(cfg_.indicator_size);
        indicator_gap_row_.Slider().SetValue(cfg_.indicator_gap);
        indicator_radius_row_.Slider().SetValue(cfg_.indicator_radius);
        body_radius_row_.Slider().SetValue(cfg_.body_radius);
        body_frame_width_row_.Slider().SetValue(cfg_.body_frame_width);
        body_face_row_.Toggle().SetOn(cfg_.body_face);
        body_frame_row_.Toggle().SetOn(cfg_.body_frame);
        enabled_row_.Toggle().SetOn(cfg_.enabled);
        text_row_.SetColor(0, cfg_.text);
        body_face_color_row_.SetColor(0, cfg_.body_face_color);
        body_frame_color_row_.SetColor(0, cfg_.body_frame_color);
        indicator_face_row_.SetColor(0, cfg_.indicator_face);
        indicator_frame_row_.SetColor(0, cfg_.indicator_frame);
        ApplyTo(r1_); ApplyTo(r2_); ApplyTo(r3_);
        SyncState();
        SyncCode();
        Preview().Refresh();
    }
    void SyncState()
    {
        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_visual_value_.SetText(RadioVisualName(cfg_.visual));
        state_checked_value_.SetText(r1_.IsChecked() ? "Option A" : r2_.IsChecked() ? "Option B" : "Option C");
        state_side_value_.SetText(cfg_.indicator_side == UiAlign::RIGHT ? "Right" : "Left");
    }
    void SyncCode()
    {
        String code;
        code << "UiRadioButton radio;\n";
        code << "radio.SetText(\"Option A\").SetGroup(1);\n";
        code << "radio.SetVisual(" << (cfg_.visual == RADIO_PILLS ? "UIRADIOVIS_PILLS" : cfg_.visual == RADIO_LIST ? "UIRADIOVIS_LIST" : "UIRADIOVIS_CLASSIC") << ");\n";
        code << "radio.SetIndicatorSide(UiAlign::" << (cfg_.indicator_side == UiAlign::RIGHT ? "RIGHT" : "LEFT") << ");\n";
        code << "// style: indicator_size=" << cfg_.indicator_size << ", indicator_gap=" << cfg_.indicator_gap << ", indicator_radius=" << cfg_.indicator_radius << "\n";
        SetUsageCode(code);
    }

    RadioConfig cfg_;
    UiRadioButton r1_, r2_, r3_;
    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_visual_row_ { UiBoxLayout::Direction::H }, state_checked_row_ { UiBoxLayout::Direction::H }, state_side_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_visual_label_, state_visual_value_, state_checked_label_, state_checked_value_, state_side_label_, state_side_value_;
    UiBoxLayout visual_row_box_ { UiBoxLayout::Direction::H }, side_row_box_ { UiBoxLayout::Direction::H };
    UiLabel visual_label_, side_label_;
    UiDropdown visual_drop_, side_drop_;
    UiCompositeSlider indicator_size_row_, indicator_gap_row_, indicator_radius_row_, body_radius_row_, body_frame_width_row_;
    UiCompositeToggle body_face_row_, body_frame_row_, enabled_row_;
    UiCompositeColor text_row_, body_face_color_row_, body_frame_color_row_, indicator_face_row_, indicator_frame_row_;
};

}

GUI_APP_MAIN
{
    UiRadioButtonBuilder().Run();
}
