/*
    UiSplitterDemo
    --------------

    Purpose
    - Active Ui control demo used as a build smoke test and visual styling reference.

    Demo hygiene header
    - Uses the shared builder shell with header/version/theme/exit and copyable usage code.
    - Exposes track/thumb splitter styling so the result can be lifted into a theme.
    - Keeps pane controls real and separate, matching the U++ Splitter usage model.

    Changelog
    - 2026-05: initial builder-shell UiSplitter demo.
*/

#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

enum SplitterOrientation {
    SPLIT_HORZ = 0,
    SPLIT_VERT = 1
};

struct SplitterConfig {
    int orientation = SPLIT_HORZ;
    int split_percent = 42;
    int min_a = DPI(140);
    int min_b = DPI(140);

    int hit_width = DPI(14);
    int track_thickness = DPI(2);
    int track_inset = 0;
    int thumb_width = DPI(14);
    int thumb_height = DPI(84);
    int thumb_inset = 0;
    int thumb_radius = DPI(8);
    int thumb_frame_width = DPI(1);
    bool thumb_face = true;
    bool thumb_frame = true;
    bool show_grip = true;
    int grip_count = 6;
    int grip_dot = DPI(2);
    int grip_gap = DPI(3);

    Color track = Color(148, 163, 184);
    Color thumb_face_color = Color(241, 245, 249);
    Color thumb_frame_color = Color(148, 163, 184);
    Color thumb_ink = Color(71, 85, 105);
    Color pane_a = Color(248, 250, 252);
    Color pane_b = Color(241, 245, 249);
};

struct Option {
    const char* label;
    int value;
};

Image SplitterIcon(int orientation)
{
    if(orientation == SPLIT_VERT)
        return ICON_NAVIGATION_OUTLINED_MORE_VERT_48();
    return ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48();
}

String OrientationCode(int mode)
{
    return mode == SPLIT_VERT ? "Vert" : "Horz";
}

String IconCode(int orientation)
{
    if(orientation == SPLIT_VERT)
        return "ICON_NAVIGATION_OUTLINED_MORE_VERT_48()";
    return "ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48()";
}

class UiSplitterBuilder : public BuilderWindowBase {
public:
    typedef UiSplitterBuilder CLASSNAME;

    UiSplitterBuilder()
        : BuilderWindowBase("UiSplitterDemo",
                            "U++ UiSplitter Builder",
                            "Inspect splitter track/thumb geometry, panes, labels, icons, and theme-ready style code.")
    {
        Preview().Add(splitter_);
        left_.Add(left_label_.SizePos());
        right_.Add(right_label_.SizePos());
        left_label_.SetText("Pane A").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        right_label_.SetText("Pane B").SetAlign(UiAlign::CENTER, UiAlign::CENTER);

    AddStateRow(StateBox(), state_orientation_row_, state_orientation_label_, state_orientation_value_, "Orientation");
        AddStateRow(StateBox(), state_position_row_, state_position_label_, state_position_value_, "Split");
        AddStateRow(StateBox(), state_panes_row_, state_panes_label_, state_panes_value_, "Pane Sizes");
        AddStateRow(StateBox(), state_track_row_, state_track_label_, state_track_value_, "Track");
        AddStateRow(StateBox(), state_thumb_row_, state_thumb_label_, state_thumb_value_, "Thumb");
        ApplyStateTextStyle();

        AddDropdownRow(PropsBox(), orientation_row_, orientation_label_, orientation_drop_, "Orientation");
        AddSliderRow(PropsBox(), position_row_, "Split", "42%");
        AddSliderRow(PropsBox(), min_a_row_, "Pane A Min", "140px");
        AddSliderRow(PropsBox(), min_b_row_, "Pane B Min", "140px");
        AddSliderRow(PropsBox(), hit_width_row_, "Hit Width", "14px");
        AddSliderRow(PropsBox(), track_thickness_row_, "Track Thick", "2px");
        AddSliderRow(PropsBox(), track_inset_row_, "Track Inset", "0px");
        AddSliderRow(PropsBox(), thumb_width_row_, "Thumb Width", "14px");
        AddSliderRow(PropsBox(), thumb_height_row_, "Thumb Height", "84px");
        AddSliderRow(PropsBox(), thumb_inset_row_, "Thumb Inset", "0px");
        AddSliderRow(PropsBox(), thumb_radius_row_, "Thumb Radius", "8px");
        AddSliderRow(PropsBox(), thumb_frame_width_row_, "Frame Width", "1px");
        AddToggleRow(PropsBox(), thumb_face_row_, "Thumb Face");
        AddToggleRow(PropsBox(), thumb_frame_row_, "Thumb Frame");
        AddToggleRow(PropsBox(), show_grip_row_, "Grip Dots");
        AddSliderRow(PropsBox(), grip_count_row_, "Grip Count", "6");
        AddSliderRow(PropsBox(), grip_dot_row_, "Grip Dot", "2px");
        AddSliderRow(PropsBox(), grip_gap_row_, "Grip Gap", "3px");
        AddColorRow(PropsBox(), track_color_row_, "Track");
        AddColorRow(PropsBox(), thumb_face_color_row_, "Thumb Face");
        AddColorRow(PropsBox(), thumb_frame_color_row_, "Thumb Frame");
        AddColorRow(PropsBox(), thumb_ink_color_row_, "Thumb Ink");
        AddColorRow(PropsBox(), pane_a_color_row_, "Pane A");
        AddColorRow(PropsBox(), pane_b_color_row_, "Pane B");

        const Option orientations[] = {
            { "Left / Right", SPLIT_HORZ },
            { "Top / Bottom", SPLIT_VERT }
        };
        Populate(orientation_drop_, orientations, __countof(orientations));

        InitSlider(position_row_, cfg_.split_percent, 5, 95);
        InitSlider(min_a_row_, cfg_.min_a, DPI(40), DPI(340));
        InitSlider(min_b_row_, cfg_.min_b, DPI(40), DPI(340));
        InitSlider(hit_width_row_, cfg_.hit_width, DPI(4), DPI(40));
        InitSlider(track_thickness_row_, cfg_.track_thickness, DPI(1), DPI(18));
        InitSlider(track_inset_row_, cfg_.track_inset, 0, DPI(32));
        InitSlider(thumb_width_row_, cfg_.thumb_width, DPI(4), DPI(80));
        InitSlider(thumb_height_row_, cfg_.thumb_height, DPI(12), DPI(180));
        InitSlider(thumb_inset_row_, cfg_.thumb_inset, 0, DPI(16));
        InitSlider(thumb_radius_row_, cfg_.thumb_radius, 0, DPI(24));
        InitSlider(thumb_frame_width_row_, cfg_.thumb_frame_width, 0, DPI(6));
        InitSlider(grip_count_row_, cfg_.grip_count, 0, 14);
        InitSlider(grip_dot_row_, cfg_.grip_dot, DPI(1), DPI(5));
        InitSlider(grip_gap_row_, cfg_.grip_gap, DPI(1), DPI(10));
        InitColor(track_color_row_, cfg_.track);
        InitColor(thumb_face_color_row_, cfg_.thumb_face_color);
        InitColor(thumb_frame_color_row_, cfg_.thumb_frame_color);
        InitColor(thumb_ink_color_row_, cfg_.thumb_ink);
        InitColor(pane_a_color_row_, cfg_.pane_a);
        InitColor(pane_b_color_row_, cfg_.pane_b);

        orientation_drop_.WhenSelect = [=](int) { cfg_.orientation = (int)orientation_drop_.GetSelectedData(); RefreshFromConfig(); };
        WirePositionSlider();
        WireSlider(min_a_row_, cfg_.min_a);
        WireSlider(min_b_row_, cfg_.min_b);
        WireSlider(hit_width_row_, cfg_.hit_width);
        WireSlider(track_thickness_row_, cfg_.track_thickness);
        WireSlider(track_inset_row_, cfg_.track_inset);
        WireSlider(thumb_width_row_, cfg_.thumb_width);
        WireSlider(thumb_height_row_, cfg_.thumb_height);
        WireSlider(thumb_inset_row_, cfg_.thumb_inset);
        WireSlider(thumb_radius_row_, cfg_.thumb_radius);
        WireSlider(thumb_frame_width_row_, cfg_.thumb_frame_width);
        WireSlider(grip_count_row_, cfg_.grip_count);
        WireSlider(grip_dot_row_, cfg_.grip_dot);
        WireSlider(grip_gap_row_, cfg_.grip_gap);
        WireToggle(thumb_face_row_, cfg_.thumb_face);
        WireToggle(thumb_frame_row_, cfg_.thumb_frame);
        WireToggle(show_grip_row_, cfg_.show_grip);
        WireColor(track_color_row_, cfg_.track);
        WireColor(thumb_face_color_row_, cfg_.thumb_face_color);
        WireColor(thumb_frame_color_row_, cfg_.thumb_frame_color);
        WireColor(thumb_ink_color_row_, cfg_.thumb_ink);
        WireColor(pane_a_color_row_, cfg_.pane_a);
        WireColor(pane_b_color_row_, cfg_.pane_b);

        splitter_.WhenAction = [=] {
            cfg_.split_percent = (int)(splitter_.GetSplitPercent() + 0.5);
            SyncRows();
            SyncState();
            SyncCode();
        };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        UiSplitter::Style s = UiTheme::ResolveSplitter();
        cfg_.track = s.track_palette.face[ST_NORMAL].IsSolid() ? s.track_palette.face[ST_NORMAL].color : cfg_.track;
        cfg_.thumb_face_color = s.thumb_palette.face[ST_NORMAL].IsSolid() ? s.thumb_palette.face[ST_NORMAL].color : cfg_.thumb_face_color;
        cfg_.thumb_frame_color = s.thumb_palette.frame[ST_NORMAL];
        cfg_.thumb_ink = s.thumb_palette.ink[ST_NORMAL];
        ApplyStateTextStyle();
        RefreshFromConfig();
    }

    virtual void LayoutPreviewContent() override
    {
        Rect c = Preview().GetCanvasRect();
        splitter_.SetRect(c.left + DPI(26), c.top + DPI(30),
                          max(DPI(360), c.GetWidth() - DPI(52)),
                          max(DPI(260), c.GetHeight() - DPI(60)));
    }

private:
    void AddColorRow(UiBoxLayout& target, UiCompositeColor& row, const char* name)
    {
        row.SetLabel(name).SetColorCount(1).ShowValue(false);
        target.Add(row).Fit();
    }

    void ApplyStateTextStyle()
    {
        UiLabel::Style label = UiTheme::ResolveLabel(UiRole::Subtle);
        UiLabel::Style value = UiTheme::ResolveLabel(UiRole::Standard);
        label.font = SansSerifZ(11);
        value.font = SansSerifZ(11);
        UiLabel* labels[] = {
            &state_orientation_label_, &state_position_label_, &state_panes_label_, &state_track_label_, &state_thumb_label_
        };
        UiLabel* values[] = {
            &state_orientation_value_, &state_position_value_, &state_panes_value_, &state_track_value_, &state_thumb_value_
        };
        for(int i = 0; i < __countof(labels); i++)
            labels[i]->SetCustomStyle(label);
        for(int i = 0; i < __countof(values); i++)
            values[i]->SetCustomStyle(value);
    }

    void Populate(UiDropdown& drop, const Option* options, int count)
    {
        drop.UseInternalModel();
        drop.Clear();
        for(int i = 0; i < count; i++)
            drop.Add(options[i].label, options[i].value);
    }

    void InitSlider(UiCompositeSlider& row, int value, int lo, int hi)
    {
        row.Slider().SetRange(lo, hi).SetStep(1).SetValue(value);
    }

    void InitColor(UiCompositeColor& row, Color c)
    {
        row.SetColor(0, c);
    }

    void WireSlider(UiCompositeSlider& row, int& field)
    {
        auto apply = [this, &row, &field] {
            field = (int)row.Slider().GetValue();
            RefreshFromConfig();
        };
        row.WhenChanging = apply;
        row.WhenAction = apply;
    }

    void WirePositionSlider()
    {
        auto apply = [this] {
            cfg_.split_percent = (int)position_row_.Slider().GetValue();
            RefreshFromConfig();
        };
        position_row_.WhenChanging = apply;
        position_row_.WhenAction = apply;
    }

    void WireToggle(UiCompositeToggle& row, bool& field)
    {
        row.Toggle().WhenAction = [this, &row, &field] {
            field = row.Toggle().IsOn();
            RefreshFromConfig();
        };
    }

    void WireColor(UiCompositeColor& row, Color& field)
    {
        row.WhenAction = [this, &row, &field] {
            field = row.GetColor(0);
            RefreshFromConfig();
        };
    }

    UiSplitter::Style BuildSplitterStyle() const
    {
        UiSplitter::Style s = UiTheme::ResolveSplitter();
        s.hit_width = cfg_.hit_width;
        s.track_thickness = cfg_.track_thickness;
        s.track_inset = Rect(cfg_.track_inset, cfg_.track_inset, cfg_.track_inset, cfg_.track_inset);
        if(cfg_.orientation == SPLIT_VERT) {
            s.thumb_main = cfg_.thumb_width;
            s.thumb_cross = cfg_.thumb_height;
        }
        else {
            s.thumb_main = cfg_.thumb_height;
            s.thumb_cross = cfg_.thumb_width;
        }
        s.thumb_inset = Rect(cfg_.thumb_inset, cfg_.thumb_inset, cfg_.thumb_inset, cfg_.thumb_inset);
        s.thumb_metrics.radius = cfg_.thumb_radius;
        s.thumb_metrics.frame_width = cfg_.thumb_frame_width;
        s.thumb_metrics.face_enabled = cfg_.thumb_face;
        s.thumb_metrics.frame_enabled = cfg_.thumb_frame;
        s.show_grip = cfg_.show_grip;
        s.grip_dot_count = cfg_.grip_count;
        s.grip_dot_size = cfg_.grip_dot;
        s.grip_dot_gap = cfg_.grip_gap;
        s.label.Clear();
        s.thumb_icon = SplitterIcon(cfg_.orientation);
        s.thumb_icon_size = DPI(14);

        for(int i = 0; i < 4; i++) {
            s.track_palette.face[i] = UiFill::Solid(cfg_.track);
            s.track_palette.frame[i] = Null;
            s.track_palette.ink[i] = cfg_.thumb_ink;
            s.thumb_palette.face[i] = UiFill::Solid(cfg_.thumb_face_color);
            s.thumb_palette.frame[i] = cfg_.thumb_frame_color;
            s.thumb_palette.ink[i] = cfg_.thumb_ink;
        }
        s.thumb_palette.face[ST_HOT] = UiFill::Solid(Blend(cfg_.thumb_face_color, cfg_.track, 42));
        s.thumb_palette.face[ST_PRESSED] = UiFill::Solid(Blend(cfg_.thumb_face_color, cfg_.track, 84));
        s.track_palette.face[ST_HOT] = UiFill::Solid(Blend(cfg_.track, cfg_.thumb_ink, 32));
        s.track_palette.face[ST_PRESSED] = UiFill::Solid(Blend(cfg_.track, cfg_.thumb_ink, 64));
        return s;
    }

    UiPanel::Style PaneStyle(Color face) const
    {
        UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Surface);
        for(int i = 0; i < 4; i++)
            s.palette.face[i] = UiFill::Solid(face);
        s.metrics.radius = DPI(8);
        return s;
    }

    void RefreshFromConfig()
    {
        orientation_drop_.SelectByData(cfg_.orientation);
        SyncRows();

        left_.SetCustomStyle(PaneStyle(cfg_.pane_a));
        right_.SetCustomStyle(PaneStyle(cfg_.pane_b));
        splitter_.Clear();
        splitter_.SetCustomStyle(BuildSplitterStyle());
        if(cfg_.orientation == SPLIT_VERT)
            splitter_.Vert(left_, right_);
        else
            splitter_.Horz(left_, right_);
        splitter_.SetMinPixels(0, cfg_.min_a).SetMinPixels(1, cfg_.min_b).SetSplitPercent(cfg_.split_percent);

        LayoutPreviewContent();
        splitter_.Layout();
        SyncState();
        SyncCode();
        Preview().Refresh();
    }

    void SyncRows()
    {
        position_row_.Slider().SetValue(cfg_.split_percent);
        min_a_row_.Slider().SetValue(cfg_.min_a);
        min_b_row_.Slider().SetValue(cfg_.min_b);
        hit_width_row_.Slider().SetValue(cfg_.hit_width);
        track_thickness_row_.Slider().SetValue(cfg_.track_thickness);
        track_inset_row_.Slider().SetValue(cfg_.track_inset);
        thumb_width_row_.Slider().SetValue(cfg_.thumb_width);
        thumb_height_row_.Slider().SetValue(cfg_.thumb_height);
        thumb_inset_row_.Slider().SetValue(cfg_.thumb_inset);
        thumb_radius_row_.Slider().SetValue(cfg_.thumb_radius);
        thumb_frame_width_row_.Slider().SetValue(cfg_.thumb_frame_width);
        grip_count_row_.Slider().SetValue(cfg_.grip_count);
        grip_dot_row_.Slider().SetValue(cfg_.grip_dot);
        grip_gap_row_.Slider().SetValue(cfg_.grip_gap);

        position_row_.SetValueText(AsString(cfg_.split_percent) + "%");
        min_a_row_.SetValueText(AsString(cfg_.min_a) + "px");
        min_b_row_.SetValueText(AsString(cfg_.min_b) + "px");
        hit_width_row_.SetValueText(AsString(cfg_.hit_width) + "px");
        track_thickness_row_.SetValueText(AsString(cfg_.track_thickness) + "px");
        track_inset_row_.SetValueText(AsString(cfg_.track_inset) + "px");
        thumb_width_row_.SetValueText(AsString(cfg_.thumb_width) + "px");
        thumb_height_row_.SetValueText(AsString(cfg_.thumb_height) + "px");
        thumb_inset_row_.SetValueText(AsString(cfg_.thumb_inset) + "px");
        thumb_radius_row_.SetValueText(AsString(cfg_.thumb_radius) + "px");
        thumb_frame_width_row_.SetValueText(AsString(cfg_.thumb_frame_width) + "px");
        grip_count_row_.SetValueText(AsString(cfg_.grip_count));
        grip_dot_row_.SetValueText(AsString(cfg_.grip_dot) + "px");
        grip_gap_row_.SetValueText(AsString(cfg_.grip_gap) + "px");

        thumb_face_row_.Toggle().SetOn(cfg_.thumb_face);
        thumb_frame_row_.Toggle().SetOn(cfg_.thumb_frame);
        show_grip_row_.Toggle().SetOn(cfg_.show_grip);
        track_color_row_.SetColor(0, cfg_.track);
        thumb_face_color_row_.SetColor(0, cfg_.thumb_face_color);
        thumb_frame_color_row_.SetColor(0, cfg_.thumb_frame_color);
        thumb_ink_color_row_.SetColor(0, cfg_.thumb_ink);
        pane_a_color_row_.SetColor(0, cfg_.pane_a);
        pane_b_color_row_.SetColor(0, cfg_.pane_b);
    }

    void SyncState()
    {
        state_orientation_value_.SetText(cfg_.orientation == SPLIT_VERT ? "Top / Bottom" : "Left / Right");
        state_position_value_.SetText(AsString(cfg_.split_percent) + "%");
        Size a = left_.GetSize();
        Size b = right_.GetSize();
        state_panes_value_.SetText(Format("%d x %d / %d x %d", a.cx, a.cy, b.cx, b.cy));
        state_track_value_.SetText(Format("%dpx line / %dpx hit", cfg_.track_thickness, cfg_.hit_width));
        state_thumb_value_.SetText(Format("%d x %d", cfg_.thumb_width, cfg_.thumb_height));
    }

    void SyncCode()
    {
        String code;
        code << "UiSplitter splitter;\n";
        code << "UiPanel pane_a, pane_b;\n";
        code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n";
        code << "style.hit_width = DPI(" << cfg_.hit_width << ");\n";
        code << "style.track_thickness = DPI(" << cfg_.track_thickness << ");\n";
        code << "style.track_inset = Rect(DPI(" << cfg_.track_inset << "), DPI(" << cfg_.track_inset << "), DPI(" << cfg_.track_inset << "), DPI(" << cfg_.track_inset << "));\n";
        if(cfg_.orientation == SPLIT_VERT) {
            code << "style.thumb_main = DPI(" << cfg_.thumb_width << "); // visual width\n";
            code << "style.thumb_cross = DPI(" << cfg_.thumb_height << "); // visual height\n";
        }
        else {
            code << "style.thumb_main = DPI(" << cfg_.thumb_height << "); // visual height\n";
            code << "style.thumb_cross = DPI(" << cfg_.thumb_width << "); // visual width\n";
        }
        code << "style.thumb_inset = Rect(DPI(" << cfg_.thumb_inset << "), DPI(" << cfg_.thumb_inset << "), DPI(" << cfg_.thumb_inset << "), DPI(" << cfg_.thumb_inset << "));\n";
        code << "style.thumb_metrics.radius = DPI(" << cfg_.thumb_radius << ");\n";
        code << "style.thumb_metrics.frame_width = DPI(" << cfg_.thumb_frame_width << ");\n";
        code << "style.thumb_metrics.face_enabled = " << (cfg_.thumb_face ? "true" : "false") << ";\n";
        code << "style.thumb_metrics.frame_enabled = " << (cfg_.thumb_frame ? "true" : "false") << ";\n";
        code << "style.show_grip = " << (cfg_.show_grip ? "true" : "false") << ";\n";
        code << "style.grip_dot_count = " << cfg_.grip_count << ";\n";
        code << "style.grip_dot_size = DPI(" << cfg_.grip_dot << ");\n";
        code << "style.grip_dot_gap = DPI(" << cfg_.grip_gap << ");\n";
        code << "style.label.Clear();\n";
        code << "style.thumb_icon = " << IconCode(cfg_.orientation) << ";\n";
        code << "for(int i = 0; i < 4; i++) {\n";
        code << "    style.track_palette.face[i] = UiFill::Solid(" << ColorCpp(cfg_.track) << ");\n";
        code << "    style.thumb_palette.face[i] = UiFill::Solid(" << ColorCpp(cfg_.thumb_face_color) << ");\n";
        code << "    style.thumb_palette.frame[i] = " << ColorCpp(cfg_.thumb_frame_color) << ";\n";
        code << "    style.thumb_palette.ink[i] = " << ColorCpp(cfg_.thumb_ink) << ";\n";
        code << "}\n";
        code << "splitter.SetCustomStyle(style)\n";
        code << "        ." << OrientationCode(cfg_.orientation) << "(pane_a, pane_b)\n";
        code << "        .SetMinPixels(0, DPI(" << cfg_.min_a << "))\n";
        code << "        .SetMinPixels(1, DPI(" << cfg_.min_b << "))\n";
        code << "        .SetSplitPercent(" << cfg_.split_percent << ");\n";
        SetUsageCode(code);
    }

    SplitterConfig cfg_;
    UiSplitter splitter_;
    UiPanel left_, right_;
    UiLabel left_label_, right_label_;

    UiBoxLayout state_orientation_row_ { UiDirection::H }, state_position_row_ { UiDirection::H }, state_panes_row_ { UiDirection::H }, state_track_row_ { UiDirection::H }, state_thumb_row_ { UiDirection::H };
    UiLabel state_orientation_label_, state_orientation_value_, state_position_label_, state_position_value_, state_panes_label_, state_panes_value_, state_track_label_, state_track_value_, state_thumb_label_, state_thumb_value_;

    UiBoxLayout orientation_row_ { UiDirection::H };
    UiLabel orientation_label_;
    UiDropdown orientation_drop_;
    UiCompositeSlider position_row_, min_a_row_, min_b_row_, hit_width_row_, track_thickness_row_, track_inset_row_, thumb_width_row_, thumb_height_row_, thumb_inset_row_, thumb_radius_row_, thumb_frame_width_row_, grip_count_row_, grip_dot_row_, grip_gap_row_;
    UiCompositeToggle thumb_face_row_, thumb_frame_row_, show_grip_row_;
    UiCompositeColor track_color_row_, thumb_face_color_row_, thumb_frame_color_row_, thumb_ink_color_row_, pane_a_color_row_, pane_b_color_row_;
};

}

GUI_APP_MAIN
{
    UiSplitterBuilder().Run();
}
