#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerInteractionOverlay_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerInteractionOverlay_h_

#include <Utilities/UiDesigner/Preview/UiDesignerPreview.h>

namespace Upp {

class UiDesignerWindow;

class UiDesignerInteractionOverlay : public Ctrl {
public:
    typedef UiDesignerInteractionOverlay CLASSNAME;

    explicit UiDesignerInteractionOverlay(UiDesignerWindow& owner);

    void SetDragStatus(const String& status);
    void TrackCatalogDrag(const String& type_id, Point screen);
    bool FinishCatalogDrag(const String& type_id, Point screen);
    void CancelCatalogDrag();

    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void MouseMove(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual Image CursorImage(Point p, dword keyflags) override;

private:
    UiDesignerWindow *owner_ = nullptr;
    bool resizing_ = false;
    int resize_edge_ = 0;
    Point resize_start_;
    Rect resize_start_rect_;
    Rect resize_pending_rect_;
    String drag_type_id_;
    UiDesignerDropPlan drop_plan_;
    Rect drop_indicator_;
    String drag_status_;

    Rect WorkspaceRootRect() const;
    UiDesignerNodeId HitNode(Point p) const;
    int HitDocumentResizeEdge(Point p) const;
    Rect ResizeDocumentTo(Point p) const;
    void ClearDropPlan();
    void UpdateDropPlan(const String& type_id, Point screen, bool allow_invalid_feedback = true);
};

}

#endif
