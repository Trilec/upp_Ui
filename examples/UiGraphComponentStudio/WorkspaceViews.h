#ifndef _UiGraphWorkspaceViews_h_
#define _UiGraphWorkspaceViews_h_
#include <Utilities/UiGraphWorkspace/UiGraphWorkspace.h>

namespace Upp {
namespace GraphWorkspace {
struct Drag {
    const void* owner = nullptr;
    int revision = 0, scope = -1;
    bool move = false;
    UiGraphNodeComponentKind kind = UiGraphNodeComponentKind::Text;
    String id;
};

struct WorkspaceSelection {
    String id;
    int region = 2;
};

void DrawWorkspaceFrame(Draw& w, Rect r, Color c, int width = 1);
String PlacementSummary(const UiGraphNodeSlotRule& r);
Color RegionColor(int region);
Rect RegionRect(const UiGraphNodePresentation& p, int region);
// Native view regressions; no model, template allocator or OS drag simulation.
bool RunWorkspaceViewTests(String& error);

class DragTile : public UiButton {
public:
    Event<> WhenDrag;
    void LeftDrag(Point, dword) override
    {
        if(!IsEnabled()) return;
        // A native drag is not a button click. Disarm before the modal DND loop,
        // including Escape/release over the source, so it cannot also add an item.
        UiButton::CancelMode();
        if(HasCapture()) ReleaseCapture();
        WhenDrag();
    }
};

class RegionView : public Ctrl {
    struct Target : Moveable<Target> { Rect rect; int region = -1; String id; bool empty = false; };
    UiGraphNodePresentation presentation_;
    UiGraphNodeTemplate spec_;
    Vector<Pointf> path_;
    Rect surface_;
    Vector<Target> targets_;
    WorkspaceSelection selected_;
    bool overlay_ = false;
    int hover_ = -1;
    int ShelfColumns() const;
    int ShelfRows() const;
    Rect Board() const;
    bool Active(const Target& target) const;
    Rect Project(Rect r) const;
    Point Project(Pointf p) const;
    int Hit(Point p) const;
public:
    Event<String, int> WhenSelect;
    Event<String> WhenDrag;
    Function<bool(PasteClip&, int, String)> WhenDrop;
    friend bool RunWorkspaceViewTests(String& error);
    RegionView() { BackPaint(); }
    Size GetMinSize() const override { return Size(DPI(160), DPI(170)); }
    void SetOverlay(bool on) { overlay_ = on; }
    void Set(const UiGraphNodeTemplate& spec, const UiGraphNodePresentation& p,
             const Vector<Pointf>& path, Rect surface, const WorkspaceSelection& selection);
    void Layout() override;
    void Paint(Draw& w) override;
    void LeftDown(Point p, dword) override;
    void LeftDrag(Point p, dword) override;
    void DragAndDrop(Point p, PasteClip& d) override;
    void DragLeave() override { hover_ = -1; Refresh(); }
};

class StructureView : public Ctrl {
    struct Row : Moveable<Row> {
        String name, id;
        int depth = 0, region = -1, mask = 0;
    };
    Vector<Row> rows_;
    UiGraphNodeTemplate spec_;
    UiGraphNodePresentation presentation_;
    WorkspaceSelection selected_;
    ScrollBar scroll_, horizontal_;
    int drop_row_ = -1;
    int Hit(Point p) const;
    int CanvasWidth() const { return max(DPI(680), GetSize().cx); }
    int Columns() const { return CanvasWidth() - DPI(252); }
    int RowHeight() const { return DPI(25); }
public:
    Event<String, int> WhenSelect;
    Event<String, int> WhenLod;
    Event<String> WhenDrag;
    Function<bool(PasteClip&, int, String)> WhenDrop;
    friend bool RunWorkspaceViewTests(String& error);
    StructureView();
    void Set(const UiGraphNodeTemplate&, const UiGraphNodePresentation&, const WorkspaceSelection&);
    void Layout() override;
    void Paint(Draw& w) override;
    void LeftDown(Point p, dword) override;
    void LeftDrag(Point p, dword) override;
    void MouseWheel(Point, int z, dword) override;
    void HorzMouseWheel(Point, int z, dword) override;
    void DragAndDrop(Point p, PasteClip& d) override;
    void DragLeave() override { drop_row_ = -1; Refresh(); }
};

class PreviewGraph : public UiNodeGraph {
public:
    UiGraphNodePresentation snapshot;
    WorkspaceSelection selected;
    Event<String, int> WhenComponentSelect;
    void Paint(Draw& w) override;
    void LeftDown(Point p, dword flags) override;
};
} // namespace GraphWorkspace
} // namespace Upp
#endif
