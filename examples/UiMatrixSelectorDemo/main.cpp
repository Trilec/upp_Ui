/*
    UiMatrixSelectorDemo
    --------------------

    Purpose
    - Larger interactive builder for UiMatrixSelector presets, Dramatica
      overlays, sizing, readout, and cell/surface styling.

    Changelog
    - 2026-08: initial matrix-selector builder demo.
*/

#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

struct MatrixConfig {
    int preset = 0;
    int overlay = 0;
    int width = 440;
    int height = 280;
    int cell_gap = 0;
    int cell_radius = 4;
    int outer_radius = 8;
    int glyph_inset = 9;
    int overlay_width = 2;
    int font_size = 11;
    int readout_gap = 10;
    int readout_width = 112;
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
                            "Position, compass, side/center, and Dramatica quad selection in one lightweight styled control.",
                            1260, 820)
    {
        Preview().Add(matrix_);

        AddStateRow(StateBox(), state_value_row_, state_value_label_, state_value_, "Value");
        AddStateRow(StateBox(), state_label_row_, state_label_label_, state_label_, "Readout");
        AddStateRow(StateBox(), state_index_row_, state_index_label_, state_index_, "Cell");
        AddStateRow(StateBox(), state_geometry_row_, state_geometry_label_, state_geometry_, "Geometry");

        AddDropdownRow(PropsBox(), preset_row_, preset_label_, preset_drop_, "Preset");
        AddDropdownRow(PropsBox(), overlay_row_, overlay_label_, overlay_drop_, "Quad Overlay");
        AddSliderRow(PropsBox(), width_row_, "Width", "440");
        AddSliderRow(PropsBox(), height_row_, "Height", "280");
        AddSliderRow(PropsBox(), gap_row_, "Cell Gap", "0");
        AddSliderRow(PropsBox(), cell_radius_row_, "Cell Radius", "4");
        AddSliderRow(PropsBox(), outer_radius_row_, "Outer Radius", "8");
        AddSliderRow(PropsBox(), glyph_inset_row_, "Glyph Inset", "9");
        AddSliderRow(PropsBox(), overlay_width_row_, "Overlay Width", "2");
        AddSliderRow(PropsBox(), font_size_row_, "Font Size", "11");
        AddSliderRow(PropsBox(), readout_gap_row_, "Readout Gap", "10");
        AddSliderRow(PropsBox(), readout_width_row_, "Readout Width", "112");
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
        preset_drop_.Add("Dramatica Quad", 3);

        overlay_drop_.UseInternalModel();
        overlay_drop_.Clear();
        overlay_drop_.Add("None", 0);
        overlay_drop_.Add("Dynamic pairs", 1);
        overlay_drop_.Add("Companion pairs", 2);
        overlay_drop_.Add("Dependent pairs", 3);
        overlay_drop_.Add("U path", 4);
        overlay_drop_.Add("Z path", 5);
        overlay_drop_.Add("Butterfly path", 6);
        overlay_drop_.Add("Custom path", 7);

        width_row_.Slider().SetRange(280, 600).SetStep(10).SetValue(cfg_.width);
        height_row_.Slider().SetRange(180, 420).SetStep(10).SetValue(cfg_.height);
        gap_row_.Slider().SetRange(0, 16).SetStep(1).SetValue(cfg_.cell_gap);
        cell_radius_row_.Slider().SetRange(0, 18).SetStep(1).SetValue(cfg_.cell_radius);
        outer_radius_row_.Slider().SetRange(0, 24).SetStep(1).SetValue(cfg_.outer_radius);
        glyph_inset_row_.Slider().SetRange(0, 18).SetStep(1).SetValue(cfg_.glyph_inset);
        overlay_width_row_.Slider().SetRange(1, 8).SetStep(1).SetValue(cfg_.overlay_width);
        font_size_row_.Slider().SetRange(8, 18).SetStep(1).SetValue(cfg_.font_size);
        readout_gap_row_.Slider().SetRange(0, 28).SetStep(1).SetValue(cfg_.readout_gap);
        readout_width_row_.Slider().SetRange(72, 180).SetStep(4).SetValue(cfg_.readout_width);
        readout_row_.Toggle().SetOn(cfg_.readout);
        cell_face_row_.Toggle().SetOn(cfg_.cell_face);
        cell_frame_row_.Toggle().SetOn(cfg_.cell_frame);
        readout_face_row_.Toggle().SetOn(cfg_.readout_face);
        readout_frame_row_.Toggle().SetOn(cfg_.readout_frame);
        surface_row_.Toggle().SetOn(cfg_.surface);
        frame_row_.Toggle().SetOn(cfg_.frame);
        shadow_row_.Toggle().SetOn(cfg_.shadow);

        preset_drop_.WhenSelect = [=](int) {
            cfg_.preset = (int)preset_drop_.GetSelectedData();
            if(cfg_.preset == 3 && cfg_.overlay == 0)
                cfg_.overlay = 1;
            RefreshFromConfig();
        };
        overlay_drop_.WhenSelect = [=](int) {
            cfg_.overlay = (int)overlay_drop_.GetSelectedData();
            RefreshFromConfig();
        };

        BindSlider(width_row_, cfg_.width);
        BindSlider(height_row_, cfg_.height);
        BindSlider(gap_row_, cfg_.cell_gap);
        BindSlider(cell_radius_row_, cfg_.cell_radius);
        BindSlider(outer_radius_row_, cfg_.outer_radius);
        BindSlider(glyph_inset_row_, cfg_.glyph_inset);
        BindSlider(overlay_width_row_, cfg_.overlay_width);
        BindSlider(font_size_row_, cfg_.font_size);
        BindSlider(readout_gap_row_, cfg_.readout_gap);
        BindSlider(readout_width_row_, cfg_.readout_width);

        readout_row_.Toggle().WhenAction = [=] { cfg_.readout = readout_row_.Toggle().IsOn(); RefreshFromConfig(); };
        cell_face_row_.Toggle().WhenAction = [=] { cfg_.cell_face = cell_face_row_.Toggle().IsOn(); RefreshFromConfig(); };
        cell_frame_row_.Toggle().WhenAction = [=] { cfg_.cell_frame = cell_frame_row_.Toggle().IsOn(); RefreshFromConfig(); };
        readout_face_row_.Toggle().WhenAction = [=] { cfg_.readout_face = readout_face_row_.Toggle().IsOn(); RefreshFromConfig(); };
        readout_frame_row_.Toggle().WhenAction = [=] { cfg_.readout_frame = readout_frame_row_.Toggle().IsOn(); RefreshFromConfig(); };
        surface_row_.Toggle().WhenAction = [=] { cfg_.surface = surface_row_.Toggle().IsOn(); RefreshFromConfig(); };
        frame_row_.Toggle().WhenAction = [=] { cfg_.frame = frame_row_.Toggle().IsOn(); RefreshFromConfig(); };
        shadow_row_.Toggle().WhenAction = [=] { cfg_.shadow = shadow_row_.Toggle().IsOn(); RefreshFromConfig(); };

        matrix_.WhenChanging = [=] { SyncStateAndCode(); };
        matrix_.WhenAction = [=] { SyncStateAndCode(); };

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
        int w = min(cfg_.width, max(DPI(220), canvas.GetWidth() - DPI(40)));
        int h = min(cfg_.height, max(DPI(160), canvas.GetHeight() - DPI(40)));
        matrix_.SetRect(canvas.left + (canvas.GetWidth() - w) / 2,
                        canvas.top + (canvas.GetHeight() - h) / 2,
                        w, h);
    }

private:
    void BindSlider(UiCompositeSlider& row, int& value)
    {
        UiCompositeSlider* control = &row;
        int* target = &value;
        auto apply = [=] {
            *target = (int)control->Slider().GetValue();
            RefreshFromConfig();
        };
        row.WhenChanging = apply;
        row.WhenAction = apply;
    }

    UiMatrixPreset ResolvePreset() const
    {
        switch(cfg_.preset) {
        case 1: return UiMatrixPreset::Compass8;
        case 2: return UiMatrixPreset::Region5;
        case 3: return UiMatrixPreset::DramaticaQuad;
        default: return UiMatrixPreset::Position9;
        }
    }

    UiMatrixOverlay ResolveOverlay() const
    {
        switch(cfg_.overlay) {
        case 1: return UiMatrixOverlay::DynamicPairs;
        case 2: return UiMatrixOverlay::CompanionPairs;
        case 3: return UiMatrixOverlay::DependentPairs;
        case 4: return UiMatrixOverlay::PathU;
        case 5: return UiMatrixOverlay::PathZ;
        case 6: return UiMatrixOverlay::PathButterfly;
        case 7: return UiMatrixOverlay::CustomPath;
        default: return UiMatrixOverlay::None;
        }
    }

    void ApplyConfigStyle()
    {
        matrix_.SetCellGap(DPI(cfg_.cell_gap))
               .SetCellRadius(DPI(cfg_.cell_radius))
               .SetOuterRadius(DPI(cfg_.outer_radius))
               .SetGlyphInset(DPI(cfg_.glyph_inset))
               .SetOverlayWidth(DPI(cfg_.overlay_width))
               .SetCellFont(DemoSans(cfg_.font_size))
               .SetReadoutFont(DemoSans(cfg_.font_size))
               .SetReadoutRadius(DPI(max(0, cfg_.cell_radius)))
               .SetReadoutGap(DPI(cfg_.readout_gap))
               .SetReadoutWidth(DPI(cfg_.readout_width))
               .ShowReadout(cfg_.readout)
               .ShowCellFace(cfg_.cell_face)
               .ShowCellFrame(cfg_.cell_frame)
               .ShowReadoutFace(cfg_.readout_face)
               .ShowReadoutFrame(cfg_.readout_frame)
               .ShowSurface(cfg_.surface)
               .ShowSurfaceFrame(cfg_.frame)
               .SetSurfaceShadow(cfg_.shadow);
    }

    void RefreshFromConfig()
    {
        Value selected = matrix_.GetData();
        matrix_.ClearCustomStyle();
        matrix_.SetPreset(ResolvePreset());
        if(cfg_.preset == 3) {
            UiMatrixOverlay overlay = ResolveOverlay();
            if(overlay == UiMatrixOverlay::CustomPath) {
                Vector<int> path;
                path << 0 << 2 << 1 << 3;
                matrix_.SetCustomPath(path);
            }
            else
                matrix_.SetOverlay(overlay);
        }
        else
            matrix_.SetOverlay(UiMatrixOverlay::None);
        ApplyConfigStyle();
        if(!IsNull(selected))
            matrix_.SetData(selected);

        preset_drop_.SelectByData(cfg_.preset);
        overlay_drop_.SelectByData(cfg_.overlay);
        overlay_drop_.Enable(cfg_.preset == 3);
        overlay_width_row_.Enable(cfg_.preset == 3);

        width_row_.Slider().SetValue(cfg_.width); width_row_.SetValueText(AsString(cfg_.width));
        height_row_.Slider().SetValue(cfg_.height); height_row_.SetValueText(AsString(cfg_.height));
        gap_row_.Slider().SetValue(cfg_.cell_gap); gap_row_.SetValueText(AsString(cfg_.cell_gap));
        cell_radius_row_.Slider().SetValue(cfg_.cell_radius); cell_radius_row_.SetValueText(AsString(cfg_.cell_radius));
        outer_radius_row_.Slider().SetValue(cfg_.outer_radius); outer_radius_row_.SetValueText(AsString(cfg_.outer_radius));
        glyph_inset_row_.Slider().SetValue(cfg_.glyph_inset); glyph_inset_row_.SetValueText(AsString(cfg_.glyph_inset));
        overlay_width_row_.Slider().SetValue(cfg_.overlay_width); overlay_width_row_.SetValueText(AsString(cfg_.overlay_width));
        font_size_row_.Slider().SetValue(cfg_.font_size); font_size_row_.SetValueText(AsString(cfg_.font_size));
        readout_gap_row_.Slider().SetValue(cfg_.readout_gap); readout_gap_row_.SetValueText(AsString(cfg_.readout_gap));
        readout_width_row_.Slider().SetValue(cfg_.readout_width); readout_width_row_.SetValueText(AsString(cfg_.readout_width));
        readout_row_.Toggle().SetOn(cfg_.readout);
        cell_face_row_.Toggle().SetOn(cfg_.cell_face);
        cell_frame_row_.Toggle().SetOn(cfg_.cell_frame);
        readout_face_row_.Toggle().SetOn(cfg_.readout_face);
        readout_frame_row_.Toggle().SetOn(cfg_.readout_frame);
        surface_row_.Toggle().SetOn(cfg_.surface);
        frame_row_.Toggle().SetOn(cfg_.frame);
        shadow_row_.Toggle().SetOn(cfg_.shadow);

        LayoutPreviewContent();
        SyncStateAndCode();
        Preview().Refresh();
    }

    void SyncStateAndCode()
    {
        state_value_.SetText(AsString(matrix_.GetData()));
        state_label_.SetText(matrix_.GetSelectedLabel());
        state_index_.SetText(AsString(matrix_.GetSelectedIndex()));
        Rect m = matrix_.GetMatrixRect();
        state_geometry_.SetText(Format("%d x %d · matrix %d x %d",
                                      cfg_.width, cfg_.height, m.GetWidth(), m.GetHeight()));
        SetUsageCode(BuildUsageCode());
    }

    String BuildUsageCode() const
    {
        static const char* presets[] = { "Position9", "Compass8", "Region5", "DramaticaQuad" };
        static const char* overlays[] = { "None", "DynamicPairs", "CompanionPairs", "DependentPairs",
                                          "PathU", "PathZ", "PathButterfly", "CustomPath" };
        String code;
        code << "UiMatrixSelector selector;\n";
        code << "selector.SetPreset(UiMatrixPreset::" << presets[clamp(cfg_.preset, 0, 3)] << ")\n";
        code << "        .ShowReadout(" << (cfg_.readout ? "true" : "false") << ")\n";
        code << "        .SetCellGap(DPI(" << cfg_.cell_gap << "))\n";
        code << "        .SetCellRadius(DPI(" << cfg_.cell_radius << "))\n";
        code << "        .SetGlyphInset(DPI(" << cfg_.glyph_inset << "))\n";
        code << "        .SetOverlayWidth(DPI(" << cfg_.overlay_width << "))\n";
        code << "        .SetCellFont(SansSerifZ(" << cfg_.font_size << "))\n";
        code << "        .SetReadoutFont(SansSerifZ(" << cfg_.font_size << "))\n";
        code << "        .ShowCellFace(" << (cfg_.cell_face ? "true" : "false") << ")\n";
        code << "        .ShowCellFrame(" << (cfg_.cell_frame ? "true" : "false") << ")\n";
        code << "        .ShowReadoutFace(" << (cfg_.readout_face ? "true" : "false") << ")\n";
        code << "        .ShowReadoutFrame(" << (cfg_.readout_frame ? "true" : "false") << ")\n";
        code << "        .SetReadoutGap(DPI(" << cfg_.readout_gap << "))\n";
        code << "        .SetReadoutWidth(DPI(" << cfg_.readout_width << "));\n";
        if(cfg_.preset == 3) {
            if(cfg_.overlay == 7)
                code << "Vector<int> path; path << 0 << 2 << 1 << 3;\nselector.SetCustomPath(path);\n";
            else
                code << "selector.SetOverlay(UiMatrixOverlay::" << overlays[clamp(cfg_.overlay, 0, 7)] << ");\n";
        }
        code << "\nselector.WhenAction = [&] {\n";
        code << "    Value selected = selector.GetData();\n";
        code << "    String label = selector.GetSelectedLabel();\n";
        code << "};\n";
        return code;
    }

    MatrixConfig cfg_;
    UiMatrixSelector matrix_;

    UiBoxLayout state_value_row_ { UiDirection::H }; UiLabel state_value_label_, state_value_;
    UiBoxLayout state_label_row_ { UiDirection::H }; UiLabel state_label_label_, state_label_;
    UiBoxLayout state_index_row_ { UiDirection::H }; UiLabel state_index_label_, state_index_;
    UiBoxLayout state_geometry_row_ { UiDirection::H }; UiLabel state_geometry_label_, state_geometry_;

    UiBoxLayout preset_row_ { UiDirection::H }; UiLabel preset_label_; UiDropdown preset_drop_;
    UiBoxLayout overlay_row_ { UiDirection::H }; UiLabel overlay_label_; UiDropdown overlay_drop_;
    UiCompositeSlider width_row_, height_row_, gap_row_, cell_radius_row_, outer_radius_row_;
    UiCompositeSlider glyph_inset_row_, overlay_width_row_, font_size_row_, readout_gap_row_, readout_width_row_;
    UiCompositeToggle readout_row_, cell_face_row_, cell_frame_row_, readout_face_row_, readout_frame_row_;
    UiCompositeToggle surface_row_, frame_row_, shadow_row_;
};

}

GUI_APP_MAIN
{
    UiMatrixSelectorBuilder().Run();
}
