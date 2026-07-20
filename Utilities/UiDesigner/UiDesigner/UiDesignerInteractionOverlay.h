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

struct UiDesignerCatalogDragDiagnostics {
    int tracking_calls = 0;
    int tracking_depth = 0;
    int max_tracking_depth = 0;
    int target_resolutions = 0;
    int capture_acquisitions = 0;
    int capture_releases = 0;
    int terminal_cancellations = 0;
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
    int resize_edge_ = 0;
    Point resize_start_;
    Rect resize_start_rect_;
    Rect resize_pending_rect_;
    String drag_type_id_;
    UiDesignerCatalogDragState drag_state_ = UiDesignerCatalogDragState::Idle;
    UiDesignerCatalogDragDiagnostics drag_diagnostics_;
    bool cleaning_drag_ = false;
    UiDesignerDropPlan drop_plan_;
    Rect drop_indicator_;
    String drag_status_;
    bool decorations_visible_ = true;

    Rect WorkspaceRootRect() const;
    UiDesignerNodeId HitNode(Point p) const;
    int HitDocumentResizeEdge(Point p) const;
    Rect ResizeDocumentTo(Point p) const;
    void ClearDropPlan();
    void EndCatalogDrag(UiDesignerCatalogDragState terminal);
    void UpdateDropPlan(const String& type_id, Point screen, bool allow_invalid_feedback = true);
};

}

#endif
