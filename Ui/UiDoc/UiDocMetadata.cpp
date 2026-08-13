#include "UiDoc.h"
#include "UiDocMetadataPrivate.h"

namespace Upp {

namespace {

String UiDocMetadataNextId(const UiDocCore& core)
{
    for(int serial = 1;; serial++) {
        String id = Format("metadata_%d", serial);
        bool used = false;
        for(const UiDocAnnotation& annotation : core.GetAnnotations())
            if(annotation.id == id) {
                used = true;
                break;
            }
        if(!used)
            return id;
    }
}

const UiDocAnnotation* UiDocMetadataById(const UiDocCore& core, const String& id)
{
    for(const UiDocAnnotation& annotation : core.GetAnnotations())
        if(annotation.id == id)
            return &annotation;
    return nullptr;
}

bool UiDocMetadataRangesTouch(UiDocRange a, UiDocRange b)
{
    a.Normalize();
    b.Normalize();
    if(a.IsEmpty())
        return b.from <= a.from && a.from <= b.to;
    if(b.IsEmpty())
        return a.from <= b.from && b.from <= a.to;
    return a.from < b.to && b.from < a.to;
}

}

const UiDoc::AnnotationLane* UiDoc::ResolveAnnotationLane(const UiDocAnnotation& annotation) const
{
    const AnnotationLane* fallback = nullptr;
    for(const AnnotationLane& lane : annotation_lanes_) {
        if(!lane.visible)
            continue;
        for(const String& type : lane.annotation_types)
            if(type == annotation.type)
                return &lane;
        if(lane.annotation_types.IsEmpty() && !fallback)
            fallback = &lane;
    }
    return fallback;
}

String UiDoc::AddMetadata(UiDocRange anchor, const String& type,
                          const String& title, const String& text,
                          const ValueMap& payload, const ValueMap& meta)
{
    anchor = NormalizeRange(anchor);

    UiDocAnnotation annotation;
    annotation.id = UiDocMetadataNextId(core_);
    annotation.range = anchor;
    annotation.type = UiDocNormalizeMetadataType(type);
    annotation.payload = clone(payload);
    annotation.payload.GetAdd("title") = title;
    annotation.payload.GetAdd("text") = text;
    annotation.meta = clone(meta);
    annotation.expanded = false;
    annotation.printable = false;
    annotation.resolved = false;

    UiDocCoreChange add;
    add.type = UiDocCoreChange::AddAnnotation;
    add.annotation = pick(annotation);

    UiDocCoreTransaction tx;
    tx.label = "Add metadata";
    tx.changes.Add(pick(add));
    String id = tx.changes[0].annotation.id;
    return core_.Apply(tx).ok ? id : String();
}

bool UiDoc::UpdateMetadata(const String& id, const String& title, const String& text,
                           const ValueMap& payload)
{
    const UiDocAnnotation* annotation = UiDocMetadataById(core_, id);
    if(!annotation || !UiDocIsMetadataAnnotation(*annotation))
        return false;

    ValueMap values = clone(payload);
    values.GetAdd("title") = title;
    values.GetAdd("text") = text;
    return core_.UpdateAnnotation(id, values);
}

bool UiDoc::RemoveMetadata(const String& id)
{
    const UiDocAnnotation* annotation = UiDocMetadataById(core_, id);
    return annotation && UiDocIsMetadataAnnotation(*annotation) && core_.RemoveAnnotation(id);
}

bool UiDoc::SetMetadataExpanded(const String& id, bool expanded)
{
    const UiDocAnnotation* annotation = UiDocMetadataById(core_, id);
    if(!annotation || !UiDocIsMetadataAnnotation(*annotation))
        return false;

    UiDocCoreChange change;
    change.type = UiDocCoreChange::SetAnnotationFlags;
    change.annotation_id = id;
    change.flag_mask = UiDocCore::ANNOT_EXPANDED;
    change.expanded = expanded;

    UiDocCoreTransaction tx;
    tx.label = expanded ? "Expand metadata" : "Collapse metadata";
    tx.add_to_history = false;
    tx.changes.Add(pick(change));
    return core_.Apply(tx).ok;
}

bool UiDoc::ToggleMetadataExpanded(const String& id)
{
    const UiDocAnnotation* annotation = UiDocMetadataById(core_, id);
    return annotation && UiDocIsMetadataAnnotation(*annotation)
         ? SetMetadataExpanded(id, !annotation->expanded)
         : false;
}

Vector<UiDocAnnotation> UiDoc::GetMetadata(UiDocRange* range) const
{
    Vector<UiDocAnnotation> out;
    UiDocRange query;
    if(range)
        query = NormalizeRange(*range);

    for(const UiDocAnnotation& annotation : core_.GetAnnotations()) {
        if(!UiDocIsMetadataAnnotation(annotation))
            continue;
        if(range && !UiDocMetadataRangesTouch(annotation.range, query))
            continue;
        out.Add(annotation);
    }
    return out;
}

UiDoc& UiDoc::ConfigureMetadataType(const String& type, const Image& icon, Color tint)
{
    String normalized = UiDocNormalizeMetadataType(type);
    AnnotationLane lane;
    lane.id = "metadata-type:" + normalized;
    lane.label = normalized.Mid(9);
    lane.annotation_types.Add(normalized);
    lane.color = IsNull(tint) ? style_.marker_annotation : tint;
    lane.icon = icon;
    lane.shape = MARKER_SQUARE;
    lane.side = LANE_AUTO;
    return AddAnnotationLane(lane);
}

}
