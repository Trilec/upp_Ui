/*
    UiMatrixSelectorDemo
    --------------------

    Purpose
    - Interactive builder for UiMatrixSelector presets, generic ordered pairs,
      default indication, sizing, readout, and cell/surface styling.
*/

#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

struct MatrixConfig {
    int preset = 0;
    int mode = 0;
    int width = 500;
    int height = 300;
    int cell_gap = 0;
    int cell_radius = 4;
    int outer_radius = 8;
    int glyph_inset = 9;
    int pair_line_width = 2;
    int pair_arrow_size = 7;
    int font_size = 11;
    int readout_gap = 12;
    int readout_width = 132;
    int default_index = 4;
    int default_dash = 4;
    int default_gap = 3;
    bool default_marker = true;
    bool readout = true;
    bool cell_face = true;
    bool cell_frame = true;
    bool readout_face = true;
    bool readout_frame = true;
    bool surface = false;
    bool frame = false;
    bool shadow = false;
};

class UiMatrixSelectorBuilder : public BuilderWindowBase {
public:
    typedef UiMatrixSelectorBuilder CLASSNAME;

    UiMatrixSelectorBuilder()
        : BuilderWindowBase("UiMatrixSelectorDemo",
                            "U++ UiMatrixSelector Builder",
                            "Position, compass, region, generic ordered quad pair, and suggested-default indication.",
                            1280, 900)
    {
        Preview().Add(matrix_);

        AddStateRow(StateBox(), state_value_row_, state_value_label_, state_value_, "Value");
        AddStateRow(StateBox(), state_label_row_, state_label_label_, state_label_, "Readout");
        AddStateRow(StateBox(), state_index_row_, state_index_label_, state_index_, "Selection");
        AddStateRow(StateBox(), state_pair_row_, state_pair_label_, state_pair_, "Pair");
        AddStateRow(StateBox(), state_default_row_, state_default_label_, state_default_, "Default");

        AddDropdownRow(PropsBox(), preset_row_, preset_label_, preset_drop_, "Preset");
        AddDropdownRow(PropsBox(), mode_row_, mode_label_, mode_drop_, "Input Mode");
        AddSliderRow(PropsBox(), width_row_, "Width", "500");
        AddSliderRow(PropsBox(), height_row_, "Height", "300");
        AddSliderRow(PropsBox(), gap_row_, "Cell Gap", "0");
        AddSliderRow(PropsBox(), cell_radius_row_, "Cell Radius", "4");
        AddSliderRow(PropsBox(), outer_radius_row_, "Outer Radius", "8");
        AddSliderRow(PropsBox(), glyph_inset_row_, "Glyph Inset", "9");
        AddSliderRow(PropsBox(), pair_width_row_, "Pair Line", "2");
        AddSliderRow(PropsBox(), pair_arrow_row_, "Pair Arrow", "7");
        AddSliderRow(PropsBox(), font_size_row_, "Font Size", "11");
        AddSliderRow(PropsBox(), readout_gap_row_, "Readout Gap", "12");
        AddSliderRow(PropsBox(), readout_width_row_, "Readout Width", "132");
        AddSliderRow(PropsBox(), default_index_row_, "Default Cell", "4");
        AddSliderRow(PropsBox(), default_dash_row_, "Default Dash", "4");
        AddSliderRow(PropsBox(), default_gap_row_, "Default Gap", "3");
        AddToggleRow(PropsBox(), default_marker_row_, "Default Marker");
        AddToggleRow(PropsBox(), readout_row_, "Readout");
        AddToggleRow(PropsBox(), cell_face_row_, "Cell Face");
        AddToggleRow(PropsBox(), cell_frame_row_, "Cell Frame");
        AddToggleRow(PropsBox(), readout_face_row_, "Readout Face");
        AddToggleRow(PropsBox(), readout_frame_row_, "Readout Frame");
        AddToggleRow(PropsBox(), surface_row_, "Outer Face");
        AddToggleRow(PropsBox(), frame_row_, "Outer Frame");
        AddToggleRow(PropsBox(), shadow_row_, "Outer Shadow");

        preset_drop_.UseInternalModel();
        preset_drop_.Clear();
        preset_drop_.Add("Position 9", 0);
        preset_drop_.Add("Compass 8", 1);
        preset_drop_.Add("Side / Center 5", 2);
        preset_drop_.Add("Quad Pair", 3);

        mode_drop_.UseInternalModel();
        mode_drop_.Clear();
        mode_drop_.Add("Single cell", 0);
        mode_drop_.Add("Ordered pair", 1);

        width_row_.Slider().SetRange(40, 700).SetStep(10).SetValue(cfg_.width);
        height_row_.Slider().SetRange(40, 460).SetStep(10).SetValue(cfg_.height);
        gap_row_.Slider().SetRange(0, 18).SetStep(1).SetValue(cfg_.cell_gap);
        cell_radius_row_.Slider().SetRange(0, 20).SetStep(1).SetValue(cfg_.cell_radius);
        outer_radius_row_.Slider().SetRange(0, 24).SetStep(1).SetValue(cfg_.outer_radius);
        glyph_inset_row_.Slider().SetRange(0, 18).SetStep(1).SetValue(cfg_.glyph_inset);
        pair_width_row_.Slider().SetRange(1, 8).SetStep(1).SetValue(cfg_.pair_line_width);
        pair_arrow_row_.Slider().SetRange(3, 16).SetStep(1).SetValue(cfg_.pair_arrow_size);
        font_size_row_.Slider().SetRange(8, 18).SetStep(1).SetValue(cfg_.font_size);
        readout_gap_row_.Slider().SetRange(0, 32).SetStep(1).SetValue(cfg_.readout_gap);
        readout_width_row_.Slider().SetRange(40, 200).SetStep(4).SetValue(cfg_.readout_width);
        default_index_row_.Slider().SetRange(0, 8).SetStep(1).SetValue(cfg_.default_index);
        default_dash_row_.Slider().SetRange(1, 12).SetStep(1).SetValue(cfg_.default_dash);
        default_gap_row_.Slider().SetRange(1, 12).SetStep(1).SetValue(cfg_.default_gap);

        preset_drop_.WhenSelect = [=](int) {
            cfg_.preset = (int)preset_drop_.GetSelectedData();
            cfg_.mode = cfg_.preset == 3 ? 1 : 0;
            cfg_.default_index = cfg_.preset == 3 ? 0 : 4;
            RefreshFromConfig();
        };
        mode_drop_.WhenSelect = [=](int) { cfg_.mode = (int)mode_drop_.GetSelectedData(); RefreshFromConfig(); };

        BindSlider(width_row_, cfg_.width); BindSlider(height_row_, cfg_.height);
        BindSlider(gap_row_, cfg_.cell_gap); BindSlider(cell_radius_row_, cfg_.cell_radius);
        BindSlider(outer_radius_row_, cfg_.outer_radius); BindSlider(glyph_inset_row_, cfg_.glyph_inset);
        BindSlider(pair_width_row_, cfg_.pair_line_width); BindSlider(pair_arrow_row_, cfg_.pair_arrow_size);
        BindSlider(font_size_row_, cfg_.font_size); BindSlider(readout_gap_row_, cfg_.readout_gap);
        BindSlider(readout_width_row_, cfg_.readout_width); BindSlider(default_index_row_, cfg_.default_index);
        BindSlider(default_dash_row_, cfg_.default_dash); BindSlider(default_gap_row_, cfg_.default_gap);

        BindToggle(default_marker_row_, cfg_.default_marker);
        BindToggle(readout_row_, cfg_.readout); BindToggle(cell_face_row_, cfg_.cell_face);
        BindToggle(cell_frame_row_, cfg_.cell_frame); BindToggle(readout_face_row_, cfg_.readout_face);
        BindToggle(readout_frame_row_, cfg_.readout_frame); BindToggle(surface_row_, cfg_.surface);
        BindToggle(frame_row_, cfg_.frame); BindToggle(shadow_row_, cfg_.shadow);

        matrix_.WhenChanging = [=] { SyncStateAndCode(); };
        matrix_.WhenAction = [=] { SyncStateAndCode(); };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override { RefreshFromConfig(); }

    virtual void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        int w = min(cfg_.width, max(DPI(40), canvas.GetWidth() - DPI(40)));
        int h = min(cfg_.height, max(DPI(40), canvas.GetHeight() - DPI(40)));
        matrix_.SetRect(canvas.left + (canvas.GetWidth() - w) / 2,
                        canvas.top + (canvas.GetHeight() - h) / 2, w, h);
    }

private:
    void BindSlider(DemoSliderRow& row, int& value)
    {
        DemoSliderRow* c = &row; int* target = &value;
        auto apply = [=] { *target = (int)c->Slider().GetValue(); RefreshFromConfig(); };
        row.WhenChanging = apply; row.WhenAction = apply;
    }

    void BindToggle(DemoToggleRow& row, bool& value)
    {
        DemoToggleRow* c = &row; bool* target = &value;
        row.Toggle().SetOn(value);
        row.Toggle().WhenAction = [=] { *target = c->Toggle().IsOn(); RefreshFromConfig(); };
    }

    UiMatrixPreset ResolvePreset() const
    {
        switch(cfg_.preset) {
        case 1: return UiMatrixPreset::Compass8;
        case 2: return UiMatrixPreset::Region5;
        case 3: return UiMatrixPreset::QuadPair;
        default: return UiMatrixPreset::Position9;
        }
    }

    void ApplyStyle()
    {
        matrix_.SetCellGap(DPI(cfg_.cell_gap))
               .SetCellRadius(DPI(cfg_.cell_radius))
               .SetOuterRadius(DPI(cfg_.outer_radius))
               .SetGlyphInset(DPI(cfg_.glyph_inset))
               .SetPairLineWidth(DPI(cfg_.pair_line_width))
               .SetPairArrowSize(DPI(cfg_.pair_arrow_size))
               .SetCellFont(DemoSans(cfg_.font_size))
               .SetReadoutFont(DemoSans(cfg_.font_size))
               .SetReadoutRadius(DPI(max(0, cfg_.cell_radius)))
               .SetReadoutGap(DPI(cfg_.readout_gap))
               .SetReadoutWidth(DPI(cfg_.readout_width))
               .SetDefaultDash(DPI(cfg_.default_dash), DPI(cfg_.default_gap))
               .ShowDefault(cfg_.default_marker)
               .ShowReadout(cfg_.readout)
               .ShowCellFace(cfg_.cell_face).ShowCellFrame(cfg_.cell_frame)
               .ShowReadoutFace(cfg_.readout_face).ShowReadoutFrame(cfg_.readout_frame)
               .ShowSurface(cfg_.surface).ShowSurfaceFrame(cfg_.frame)
               .SetSurfaceShadow(cfg_.shadow);
    }

    void RefreshFromConfig()
    {
        matrix_.ClearCustomStyle();
        matrix_.SetPreset(ResolvePreset());
        matrix_.SetSelectionMode(cfg_.mode == 1 ? UiMatrixSelectionMode::Pair : UiMatrixSelectionMode::SingleCell);
        if(cfg_.preset == 3) {
            matrix_.SetCell(0, "A", "Concept A", "a");
            matrix_.SetCell(1, "B", "Concept B", "b");
            matrix_.SetCell(2, "C", "Concept C", "c");
            matrix_.SetCell(3, "D", "Concept D", "d");
        }
        ApplyStyle();
        int max_index = matrix_.GetCellCount() - 1;
        cfg_.default_index = clamp(cfg_.default_index, 0, max(0, max_index));
        matrix_.ClearDefault().SetDefault(cfg_.default_index).ShowDefault(cfg_.default_marker);

        preset_drop_.SelectByData(cfg_.preset); mode_drop_.SelectByData(cfg_.mode);
        width_row_.Slider().SetValue(cfg_.width); width_row_.SetValueText(AsString(cfg_.width));
        height_row_.Slider().SetValue(cfg_.height); height_row_.SetValueText(AsString(cfg_.height));
        gap_row_.Slider().SetValue(cfg_.cell_gap); gap_row_.SetValueText(AsString(cfg_.cell_gap));
        cell_radius_row_.Slider().SetValue(cfg_.cell_radius); cell_radius_row_.SetValueText(AsString(cfg_.cell_radius));
        outer_radius_row_.Slider().SetValue(cfg_.outer_radius); outer_radius_row_.SetValueText(AsString(cfg_.outer_radius));
        glyph_inset_row_.Slider().SetValue(cfg_.glyph_inset); glyph_inset_row_.SetValueText(AsString(cfg_.glyph_inset));
        pair_width_row_.Slider().SetValue(cfg_.pair_line_width); pair_width_row_.SetValueText(AsString(cfg_.pair_line_width));
        pair_arrow_row_.Slider().SetValue(cfg_.pair_arrow_size); pair_arrow_row_.SetValueText(AsString(cfg_.pair_arrow_size));
        font_size_row_.Slider().SetValue(cfg_.font_size); font_size_row_.SetValueText(AsString(cfg_.font_size));
        readout_gap_row_.Slider().SetValue(cfg_.readout_gap); readout_gap_row_.SetValueText(AsString(cfg_.readout_gap));
        readout_width_row_.Slider().SetValue(cfg_.readout_width); readout_width_row_.SetValueText(AsString(cfg_.readout_width));
        default_index_row_.Slider().SetRange(0, max(0, max_index)).SetValue(cfg_.default_index); default_index_row_.SetValueText(AsString(cfg_.default_index));
        default_dash_row_.Slider().SetValue(cfg_.default_dash); default_dash_row_.SetValueText(AsString(cfg_.default_dash));
        default_gap_row_.Slider().SetValue(cfg_.default_gap); default_gap_row_.SetValueText(AsString(cfg_.default_gap));

        default_marker_row_.Toggle().SetOn(cfg_.default_marker); readout_row_.Toggle().SetOn(cfg_.readout);
        cell_face_row_.Toggle().SetOn(cfg_.cell_face); cell_frame_row_.Toggle().SetOn(cfg_.cell_frame);
        readout_face_row_.Toggle().SetOn(cfg_.readout_face); readout_frame_row_.Toggle().SetOn(cfg_.readout_frame);
        surface_row_.Toggle().SetOn(cfg_.surface); frame_row_.Toggle().SetOn(cfg_.frame); shadow_row_.Toggle().SetOn(cfg_.shadow);

        LayoutPreviewContent(); SyncStateAndCode(); Preview().Refresh();
    }

    void SyncStateAndCode()
    {
        state_value_.SetText(AsString(matrix_.GetData()));
        state_label_.SetText(matrix_.GetReadoutText());
        state_index_.SetText(matrix_.IsPairSelection()
                           ? Format("%d -> %d", matrix_.GetPairStartIndex(), matrix_.GetPairEndIndex())
                           : AsString(matrix_.GetSelectedIndex()));
        state_pair_.SetText(matrix_.HasCompletePair()
                         ? matrix_.GetPairOrientationName() + " · " + matrix_.GetPairDirectionLabel()
                         : String("—"));
        state_default_.SetText(matrix_.HasDefault()
                            ? Format("cell %d%s", matrix_.GetDefaultIndex(), matrix_.IsDefaultSelected() ? " · selected" : " · suggested")
                            : String("None"));
        SetUsageCode(BuildUsageCode());
    }

    String BuildUsageCode() const
    {
        static const char* presets[] = { "Position9", "Compass8", "Region5", "QuadPair" };
        String code;
        code << "UiMatrixSelector selector;\n";
        code << "selector.SetPreset(UiMatrixPreset::" << presets[clamp(cfg_.preset, 0, 3)] << ")\n";
        code << "        .SetSelectionMode(UiMatrixSelectionMode::" << (cfg_.mode ? "Pair" : "SingleCell") << ")\n";
        code << "        .SetDefault(" << cfg_.default_index << ")\n";
        code << "        .ShowDefault(" << (cfg_.default_marker ? "true" : "false") << ")\n";
        code << "        .SetCellGap(DPI(" << cfg_.cell_gap << "))\n";
        code << "        .SetCellRadius(DPI(" << cfg_.cell_radius << "))\n";
        code << "        .SetPairLineWidth(DPI(" << cfg_.pair_line_width << "))\n";
        code << "        .SetPairArrowSize(DPI(" << cfg_.pair_arrow_size << "));\n";
        return code;
    }

    MatrixConfig cfg_;
    UiMatrixSelector matrix_;

    UiBoxLayout state_value_row_{UiDirection::H}; UiLabel state_value_label_, state_value_;
    UiBoxLayout state_label_row_{UiDirection::H}; UiLabel state_label_label_, state_label_;
    UiBoxLayout state_index_row_{UiDirection::H}; UiLabel state_index_label_, state_index_;
    UiBoxLayout state_pair_row_{UiDirection::H}; UiLabel state_pair_label_, state_pair_;
    UiBoxLayout state_default_row_{UiDirection::H}; UiLabel state_default_label_, state_default_;

    UiBoxLayout preset_row_{UiDirection::H}; UiLabel preset_label_; UiDropdown preset_drop_;
    UiBoxLayout mode_row_{UiDirection::H}; UiLabel mode_label_; UiDropdown mode_drop_;
    DemoSliderRow width_row_, height_row_, gap_row_, cell_radius_row_, outer_radius_row_, glyph_inset_row_;
    DemoSliderRow pair_width_row_, pair_arrow_row_, font_size_row_, readout_gap_row_, readout_width_row_;
    DemoSliderRow default_index_row_, default_dash_row_, default_gap_row_;
    DemoToggleRow default_marker_row_, readout_row_, cell_face_row_, cell_frame_row_;
    DemoToggleRow readout_face_row_, readout_frame_row_, surface_row_, frame_row_, shadow_row_;
};

}

GUI_APP_MAIN
{
    UiMatrixSelectorBuilder().Run();
}
