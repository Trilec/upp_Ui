#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerInteractionOverlay_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerInteractionOverlay_h_

#include <Utilities/UiDesigner/Preview/UiDesignerPreview.h>

namespace Upp {

class UiDesignerWindow;

enum class UiDesignerCatalogDragState : byte {
    Idle,
    Tracking,
    Completing,
    Cancelling,
};

enum class UiDesignerPointerGesture : byte {
    None,
    RootResize,
    CatalogDrag,
};

struct UiDesignerCatalogDragDiagnostics {
    int tracking_calls = 0;
    int tracking_depth = 0;
    int max_tracking_depth = 0;
    int target_resolutions = 0;
    int capture_acquisitions = 0;
    int capture_releases = 0;
    int terminal_cancellations = 0;
};

struct UiDesignerResolvedDrop : Moveable<UiDesignerResolvedDrop> {
    int region_id = -1;
    UiDesignerDropRegion region;
    Rect exact_rect;
    Rect visual_rect;
    int insertion_index = -1;
    int grid_row = -1;
    int grid_column = -1;
    bool valid = false;
    String label;
    String reason;
    UiDesignerDropPlan plan;
};

class UiDesignerInteractionOverlay : public Ctrl {
public:
    typedef UiDesignerInteractionOverlay CLASSNAME;

    explicit UiDesignerInteractionOverlay(UiDesignerWindow& owner);

    void SetDragStatus(const String& status);
    void TrackCatalogDrag(const String& type_id, Point screen);
    bool FinishCatalogDrag(const String& type_id, Point screen);
    void CancelCatalogDrag();
    void InvalidateCatalogDrag();
    UiDesignerCatalogDragState GetCatalogDragState() const { return drag_state_; }
    const UiDesignerCatalogDragDiagnostics& GetDragDiagnostics() const { return drag_diagnostics_; }
    void SetDecorationsVisible(bool on) { decorations_visible_ = on; Refresh(); }

    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void MouseMove(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual Image CursorImage(Point p, dword keyflags) override;
    virtual bool Key(dword key, int count) override;
    virtual void CancelMode() override;

private:
    UiDesignerWindow *owner_ = nullptr;
    bool resizing_ = false;
    bool capture_owned_ = false;
    bool drag_cleanup_in_progress_ = false;
    bool capture_release_in_progress_ = false;
    int resize_edge_ = 0;
    Point resize_start_;
    Rect resize_start_rect_;
    Rect resize_pending_rect_;
    String drag_type_id_;
    UiDesignerCatalogDragState drag_state_ = UiDesignerCatalogDragState::Idle;
    UiDesignerPointerGesture pointer_gesture_ = UiDesignerPointerGesture::None;
    UiDesignerCatalogDragDiagnostics drag_diagnostics_;
    bool cleaning_drag_ = false;
    UiDesignerResolvedDrop resolved_drop_;
    UiDesignerResizeSample resize_sample_;
    uint64 resize_sample_sequence_ = 0;
    bool resize_sample_valid_ = false;
    String drag_status_;
    bool decorations_visible_ = true;

    Rect WorkspaceRootRect() const;
    Point ScreenToWorkspace(Point screen) const;
    Point WorkspaceToCanvas(Point workspace) const;
    Point ScreenToCanvas(Point screen) const;
    Rect CanvasToWorkspace(Rect canvas) const;
    UiDesignerNodeId HitNode(Point p) const;
    int HitDocumentResizeEdge(Point p) const;
    Rect ResizeDocumentTo(Point p) const;
    void ApplyRootResize(Point p);
    void FinishRootResize(Point p);
    void CancelRootResize();
    void RecordRootResizeSample(const UiDesignerPreviewStats& before,
                                const UiDesignerPreviewStats& after,
                                double sync_ms, double preview_ms,
                                const Rect& final_rect);
    void FinalizeRootResizePaint(double overlay_ms);
    void ClearDropPlan();
    void EndCatalogDrag(UiDesignerCatalogDragState terminal);
    void ReleaseOwnedCaptureSafely();
    void UpdateDropPlan(const String& type_id, Point screen, bool allow_invalid_feedback = true);
};

}

#endif
