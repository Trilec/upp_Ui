#include "UiDesignerGeometrySnapshot.h"
namespace Upp {
const UiDesignerGeometryRecord* UiDesignerGeometrySnapshot::Find(UiDesignerNodeId node) const {
    for(const auto& record : records_)
        if(record.node == node) return &record;
    return nullptr;
}

const UiDesignerDropRegion* UiDesignerGeometrySnapshot::FindDropRegion(int region_id) const {
    for(const auto& region : drop_regions_)
        if(region.paint_order == region_id)
            return &region;
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

static int DropRegionPriority(UiDesignerDropRegionKind kind)
{
    switch(kind) {
    case UiDesignerDropRegionKind::GridCell: return 50;
    case UiDesignerDropRegionKind::TitleCardContent: return 70;
    case UiDesignerDropRegionKind::TabPageContent: return 65;
    case UiDesignerDropRegionKind::AccordionSectionContent: return 60;
    case UiDesignerDropRegionKind::PanelBody: return 40;
    case UiDesignerDropRegionKind::GroupPanelBody: return 42;
    case UiDesignerDropRegionKind::BoxBody: return 38;
    case UiDesignerDropRegionKind::BoxFrame: return 36;
    case UiDesignerDropRegionKind::BoxGap: return 35;
    case UiDesignerDropRegionKind::BoxBeforeItem:
    case UiDesignerDropRegionKind::BoxAfterItem: return 30;
    case UiDesignerDropRegionKind::BoxEmptyBody: return 20;
    case UiDesignerDropRegionKind::WindowContent:
    default: return 10;
    }
}

const UiDesignerDropRegion* UiDesignerGeometrySnapshot::HitDropRegion(Point p) const {
    const UiDesignerDropRegion* best = nullptr;
    for(const auto& region : drop_regions_) {
        Rect r = region.visual_rect.IsEmpty() ? region.rect : region.visual_rect;
        if(r.IsEmpty() || !r.Contains(p))
            continue;
        if(!best || region.depth > best->depth ||
           (region.depth == best->depth &&
            DropRegionPriority(region.kind) > DropRegionPriority(best->kind)) ||
           (region.depth == best->depth &&
            DropRegionPriority(region.kind) == DropRegionPriority(best->kind) &&
            region.paint_order > best->paint_order))
            best = &region;
    }
    return best;
}

UiDesignerNodeId UiDesignerGeometrySnapshot::HitDropTarget(Point p) const {
    const UiDesignerDropRegion* region = HitDropRegion(p);
    if(region)
        return region->owner;
    const UiDesignerGeometryRecord* best = nullptr;
    for(const auto& record : records_)
        if(record.drop_target && record.rect.Contains(p) &&
           (!best || record.depth > best->depth ||
            (record.depth == best->depth && record.order > best->order)))
            best = &record;
    return best ? best->node : 0;
}
}
