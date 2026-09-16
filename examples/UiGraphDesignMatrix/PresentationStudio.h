#ifndef _UiGraphDesignMatrix_PresentationStudio_h_
#define _UiGraphDesignMatrix_PresentationStudio_h_

#include "PresentationStudioCell.h"

namespace Upp {

class StudioGuideBar : public Ctrl {
public:
    void SetDiagnostics(const String& text) { diagnostics_ = text; Refresh(); }
    void Paint(Draw& w) override;

private:
    String diagnostics_;
};

class UiGraphPresentationStudio : public TopWindow {
public:
    UiGraphPresentationStudio();

    void Paint(Draw& w) override;
    void Layout() override;

private:
    void BuildShell();
    void ConnectEvents();
    void ApplyShellStyles();
    void LayoutToolbar();
    void ArrangeMatrix();
    void Scroll();

    int ActiveTemplateIndex() const;
    StudioTemplatePolicy& ActivePolicy();
    const StudioTemplatePolicy& ActivePolicy() const;
    StudioThresholdSet EffectiveThresholds(int shape) const;
    StudioThresholdSet EditorThresholds() const;
    Sizef CurrentAuthoredSize() const;
    int CurrentPortPreset() const;
    int SampleResolution(int shape, int lod) const;

    void ConfigureFromDocument();
    void ConfigureCells();
    void PreviewThresholdChange();
    void SyncThresholdEditor();
    void ApplyThresholdsFromControl();
    void UpdateRangeScope();
    void UpdateDiagnostics();
    void ToggleFeature(int shape, int lod, int feature);
    void SelectThresholdShape(int shape);
    void SetShapeFilter(int dropdown_index);
    void UseGlobalThresholds();
    void ResetThresholds();
    void ToggleTheme();
    void ExportJson();
    void ImportJson();

private:
    StudioDocument document_;
    int selected_shape_index_ = -1; // -1 means global threshold editing.
    int shape_filter_index_ = 0;    // 0 means all shapes; 1..8 select one.
    bool syncing_range_ = false;
    int column_width_ = DPI(252);
    int sheet_height_ = 0;
    int sheet_width_ = 0;

    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiLabel intent_;
    UiButton btn_copy_json_, btn_import_, btn_export_;
    UiToolButton btn_theme_;

    UiPanel toolbar_;
    UiLabel lbl_template_, lbl_shape_, lbl_size_, lbl_ports_, lbl_range_, range_scope_;
    UiDropdown template_, shape_filter_, authored_, ports_;
    UiButton btn_global_, btn_use_global_, btn_reset_range_;
    UiRangeSegments range_;

    StudioGuideBar guide_;
    Ctrl lod_view_, viewport_, sheet_;
    StudioLodRail lod_rail_;
    ScrollBar horizontal_, vertical_;
    Array<UiButton> shape_headers_;
    Array<StudioMatrixCell> cells_;
};

} // namespace Upp

#endif
