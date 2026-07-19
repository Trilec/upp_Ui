#ifndef _Utilities_UiDesigner_Preview_UiDesignerGeometrySnapshot_h_
#define _Utilities_UiDesigner_Preview_UiDesignerGeometrySnapshot_h_
#include <Utilities/UiDesigner/Core/UiDesignerCore.h>
namespace Upp {
struct UiDesignerGeometryRecord : Moveable<UiDesignerGeometryRecord> {
    UiDesignerNodeId node = 0, parent = 0;
    Rect rect, body;
    int depth = 0, order = 0, inset = 0, gap = 0;
    bool selectable = false, drop_target = false;
    Vector<Rect> item_rects, gap_rects;
};
class UiDesignerGeometrySnapshot {
public:
    void Clear() { records_.Clear(); }
    void Add(UiDesignerGeometryRecord record) { records_.Add(pick(record)); }
    const UiDesignerGeometryRecord* Find(UiDesignerNodeId node) const;
    UiDesignerNodeId Hit(Point p) const;
private:
    Array<UiDesignerGeometryRecord> records_;
};
}
#endif
