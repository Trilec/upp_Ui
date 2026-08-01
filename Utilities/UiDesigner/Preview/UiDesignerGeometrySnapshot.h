#ifndef _Utilities_UiDesigner_Preview_UiDesignerGeometrySnapshot_h_
#define _Utilities_UiDesigner_Preview_UiDesignerGeometrySnapshot_h_
#include <Utilities/UiDesigner/Core/UiDesignerCore.h>
namespace Upp {
enum class UiDesignerDropRegionKind : byte {
    WindowContent = 0,
    PanelBody,
    GroupPanelBody,
    BoxBody,
    BoxFrame,
    BoxEmptyBody,
    BoxBeforeItem,
    BoxGap,
    BoxAfterItem,
    GridCell,
    TitleCardContent,
    AccordionSectionContent,
};

enum class UiDesignerCueKind : byte {
    None = 0,
    ControlBounds,
    ContainerBounds,
    LayoutBounds,
    SemanticItemBounds,
};

struct UiDesignerDropRegion : Moveable<UiDesignerDropRegion> {
    UiDesignerNodeId owner = 0;
    UiDesignerDropRegionKind kind = UiDesignerDropRegionKind::WindowContent;
    Rect rect;
    Rect visual_rect;
    int insertion_index = -1;
    int grid_row = -1;
    int grid_column = -1;
    bool occupied = false;
    int depth = 0;
    int paint_order = 0;
    String label;
};

struct UiDesignerGeometryRecord : Moveable<UiDesignerGeometryRecord> {
    UiDesignerNodeId node = 0, parent = 0;
    Rect rect, body;
    int depth = 0, order = 0, inset = 0, gap = 0;
    bool selectable = false, drop_target = false;
    UiDesignerCueKind cue_kind = UiDesignerCueKind::None;
    bool debug_layout = false;
    Color debug_color = Null;
    Vector<Rect> item_rects, cell_rects, gap_rects, inset_rects;
};
class UiDesignerGeometrySnapshot {
public:
    int GetCount() const { return records_.GetCount(); }
    int GetDropRegionCount() const { return drop_regions_.GetCount(); }
    const UiDesignerGeometryRecord* Find(UiDesignerNodeId node) const;
    const UiDesignerDropRegion* FindDropRegion(int region_id) const;
    UiDesignerNodeId Hit(Point p) const;
    const UiDesignerDropRegion* HitDropRegion(Point p) const;
    UiDesignerNodeId HitDropTarget(Point p) const;
    const Array<UiDesignerDropRegion>& GetDropRegions() const { return drop_regions_; }
private:
    friend class UiDesignerGeometrySnapshotBuilder;
    Array<UiDesignerGeometryRecord> records_;
    Array<UiDesignerDropRegion> drop_regions_;
};
class UiDesignerGeometrySnapshotBuilder {
public:
    void Add(UiDesignerGeometryRecord record) { records_.Add(pick(record)); }
    void AddRegion(UiDesignerDropRegion region) { drop_regions_.Add(pick(region)); }
    UiDesignerGeometrySnapshot Publish() {
        UiDesignerGeometrySnapshot result;
        result.records_ = pick(records_);
        result.drop_regions_ = pick(drop_regions_);
        return result;
    }
private:
    Array<UiDesignerGeometryRecord> records_;
    Array<UiDesignerDropRegion> drop_regions_;
};
}
#endif
