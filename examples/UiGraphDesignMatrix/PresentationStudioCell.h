#ifndef _UiGraphDesignMatrix_PresentationStudioCell_h_
#define _UiGraphDesignMatrix_PresentationStudioCell_h_

#include "PresentationStudioPolicy.h"

namespace Upp {

enum class StudioChipState : byte { Off, Visible, Suppressed };

Image StudioShapeIcon(UiGraphNodeShape shape, Color ink);

class StudioFeatureChip : public Ctrl {
public:
    Event<> WhenAction;

    void SetTag(const String& text) { tag_ = text; Refresh(); }
    void SetState(StudioChipState state) { state_ = state; Refresh(); }
    StudioChipState GetState() const { return state_; }

    void Paint(Draw& w) override;
    void LeftDown(Point, dword) override { WhenAction(); }
    void MouseMove(Point, dword) override;
    void MouseLeave() override;
    Image CursorImage(Point, dword) override { return Image::Hand(); }

private:
    String tag_;
    StudioChipState state_ = StudioChipState::Off;
    bool hot_ = false;
};

class StudioMatrixCell : public Ctrl {
public:
    Event<int> WhenFeatureToggle;

    StudioMatrixCell();

    void Configure(int shape_index, int lod_index, const StudioFeatureSet& requested,
                   int template_index, Sizef authored, int resolution_px,
                   int port_preset, bool selected);
    void SetSampleResolution(int resolution_px, Sizef authored);
    int SuppressedCount() const { return suppressed_count_; }

    int GetTemplateIndex() const { return template_index_; }
    Sizef GetAuthoredSize() const { return authored_size_; }
    int GetPortInputCount() const { return port_inputs_; }
    int GetPortOutputCount() const { return port_outputs_; }
    bool IsStudioSelected() const { return selected_; }

    void Paint(Draw& w) override;
    void Layout() override;

private:
    void Sync();
    void ConfigureGraphStyle(const StudioTemplateSpec& spec);
    void BuildGraph(Sizef authored, int port_preset);

    UiButton child_;
    UiNodeGraph graph_;
    UiGraphNodeRef main_;
    Array<StudioFeatureChip> chips_;

    StudioFeatureSet requested_;
    Sizef authored_size_;
    int shape_index_ = 0;
    int lod_index_ = 0;
    int template_index_ = 0;
    int resolution_px_ = 0;
    int projected_h_ = 0;
    int port_inputs_ = 0;
    int port_outputs_ = 0;
    double sample_zoom_ = 1.0;
    bool selected_ = false;
    bool fits_ = true;
    UiGraphPresentationLevel actual_level_ = UiGraphPresentationLevel::Lod3;
    int suppressed_count_ = 0;
};

class StudioLodRail : public Ctrl {
public:
    void SetLayout(const Vector<int>& row_y, const Vector<int>& row_h,
                   const StudioThresholdSet& thresholds);
    void Paint(Draw& w) override;

private:
    Vector<int> row_y_;
    Vector<int> row_h_;
    StudioThresholdSet thresholds_;
};

} // namespace Upp

#endif
