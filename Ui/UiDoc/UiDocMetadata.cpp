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

UiDocAnnotation UiDocMetadataCopy(const UiDocAnnotation& source)
{
    UiDocAnnotation out;
    out.id = source.id;
    out.range = source.range;
    out.type = source.type;
    out.payload = clone(source.payload);
    out.meta = clone(source.meta);
    out.expanded = source.expanded;
    out.printable = source.printable;
    out.resolved = source.resolved;
    return out;
}

ValueMap UiDocMetadataMergedPayload(const ValueMap& existing, const ValueMap& changes)
{
    ValueMap merged = clone(existing);
    for(int i = 0; i < changes.GetCount(); i++)
        merged.GetAdd(changes.GetKey(i)) = clone(changes[i]);
    return merged;
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
    annotation.id = UiDocMetadataNextId(Model());
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
    return Model().Apply(tx).ok ? id : String();
}

bool UiDoc::UpdateMetadata(const String& id, const String& title, const String& text,
                           const ValueMap& payload)
{
    const UiDocAnnotation* annotation = UiDocMetadataById(Model(), id);
    if(!annotation || !UiDocIsMetadataAnnotation(*annotation))
        return false;
    ValueMap merged = UiDocMetadataMergedPayload(annotation->payload, payload);
    return UpdateMetadata(id, annotation->type, title, text, merged);
}

bool UiDoc::UpdateMetadata(const String& id, const String& type,
                           const String& title, const String& text,
                           const ValueMap& payload)
{
    const UiDocAnnotation* annotation = UiDocMetadataById(Model(), id);
    if(!annotation || !UiDocIsMetadataAnnotation(*annotation))
        return false;

    UiDocAnnotation replacement = UiDocMetadataCopy(*annotation);
    replacement.type = UiDocNormalizeMetadataType(type);
    replacement.payload = clone(payload);
    replacement.payload.GetAdd("title") = title;
    replacement.payload.GetAdd("text") = text;

    UiDocCoreChange remove;
    remove.type = UiDocCoreChange::RemoveAnnotation;
    remove.annotation_id = id;

    UiDocCoreChange add;
    add.type = UiDocCoreChange::AddAnnotation;
    add.annotation = pick(replacement);

    UiDocCoreTransaction tx;
    tx.label = "Update metadata";
    tx.changes.Add(pick(remove));
    tx.changes.Add(pick(add));
    if(!Model().Apply(tx).ok)
        return false;

    // Core change observers run synchronously. Rebuild once more after they
    // return so an expanded card cannot retain pre-edit geometry or paint data.
    InvalidateAllLayout();
    EnsureLayout();
    SyncScrollBar();
    RefreshLayout();
    Refresh();
    return true;
}

bool UiDoc::RemoveMetadata(const String& id)
{
    const UiDocAnnotation* annotation = UiDocMetadataById(Model(), id);
    return annotation && UiDocIsMetadataAnnotation(*annotation) && Model().RemoveAnnotation(id);
}

bool UiDoc::SetMetadataExpanded(const String& id, bool expanded)
{
    const UiDocAnnotation* annotation = UiDocMetadataById(Model(), id);
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
    return Model().Apply(tx).ok;
}

bool UiDoc::ToggleMetadataExpanded(const String& id)
{
    const UiDocAnnotation* annotation = UiDocMetadataById(Model(), id);
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

    for(const UiDocAnnotation& annotation : Model().GetAnnotations()) {
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

void UiDoc::SetActiveAnnotation(const String& id)
{
    if(id.IsEmpty()) {
        if(!active_annotation_id_.IsEmpty()) {
            active_annotation_id_.Clear();
            Refresh();
        }
        return;
    }

    if(!UiDocMetadataById(Model(), id))
        return;
    if(active_annotation_id_ != id) {
        active_annotation_id_ = id;
        Refresh();
    }
}

bool UiDoc::RevealAnnotation(const String& id, bool select_range)
{
    const UiDocAnnotation* annotation = UiDocMetadataById(Model(), id);
    if(!annotation)
        return false;

    UiDocRange range = annotation->range;
    SetActiveAnnotation(id);
    if(select_range && !range.IsEmpty())
        SetSelection(range);
    else
        SetSelection(UiDocRange(range.from, range.from));
    ScrollCaretIntoView();
    Refresh();
    return true;
}

}
