#include "UiDesignerGeometrySnapshot.h"
namespace Upp {
const UiDesignerGeometryRecord* UiDesignerGeometrySnapshot::Find(UiDesignerNodeId node) const {
    for(const auto& record : records_)
        if(record.node == node) return &record;
    return nullptr;
}
UiDesignerNodeId UiDesignerGeometrySnapshot::Hit(Point p) const {
    const UiDesignerGeometryRecord* best = nullptr;
    for(const auto& record : records_)
        if(record.selectable && record.rect.Contains(p) &&
           (!best || record.depth > best->depth ||
            (record.depth == best->depth && record.order > best->order)))
            best = &record;
    return best ? best->node : 0;
}
UiDesignerNodeId UiDesignerGeometrySnapshot::HitDropTarget(Point p) const {
    const UiDesignerGeometryRecord* best = nullptr;
    for(const auto& record : records_)
        if(record.drop_target && record.rect.Contains(p) &&
           (!best || record.depth > best->depth ||
            (record.depth == best->depth && record.order > best->order)))
            best = &record;
    return best ? best->node : 0;
}
}
