#include "UiDocCore.h"

namespace Upp {

static bool UiDocRangesTouch(const UiDocRange& a, const UiDocRange& b)
{
    if(a.IsEmpty())
        return b.from <= a.from && a.from <= b.to;
    if(b.IsEmpty())
        return a.from <= b.from && b.from <= a.to;
    return a.from < b.to && b.from < a.to;
}

String UiDocCore::AddBlock(UiDocRange range, const String& role, int indent, const ValueMap& meta)
{
    UiDocCoreChange change;
    change.type = UiDocCoreChange::AddBlock;
    change.block.id = Format("block_%d", next_block_id_++);
    change.block.range = range;
    change.block.role = role;
    change.block.indent = max(0, indent);
    change.block.meta = clone(meta);
    String id = change.block.id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok ? id : String();
}

bool UiDocCore::UpdateBlock(const UiDocBlock& block)
{
    if(block.id.IsEmpty())
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::UpdateBlock;
    change.block = block;
    change.block_id = block.id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

bool UiDocCore::RemoveBlock(const String& id)
{
    if(id.IsEmpty())
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::RemoveBlock;
    change.block_id = id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

Vector<UiDocBlock> UiDocCore::QueryBlocks(const UiDocRange* range, const String& role) const
{
    Vector<UiDocBlock> out;
    UiDocRange query;
    if(range)
        query = NormalizeRange(*range);
    for(const UiDocBlock& block : blocks_) {
        if(!role.IsEmpty() && block.role != role)
            continue;
        if(range && !UiDocRangesTouch(block.range, query))
            continue;
        out.Add(block);
    }
    return out;
}

String UiDocCore::AddAnnotation(UiDocRange range, const String& type,
                                const ValueMap& payload, const ValueMap& meta)
{
    if(type.IsEmpty())
        return String();
    UiDocCoreChange change;
    change.type = UiDocCoreChange::AddAnnotation;
    change.annotation.id = Format("ann_%d", next_annotation_id_++);
    change.annotation.range = range;
    change.annotation.type = type;
    change.annotation.payload = clone(payload);
    change.annotation.meta = clone(meta);
    String id = change.annotation.id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok ? id : String();
}

bool UiDocCore::RemoveAnnotation(const String& id)
{
    if(id.IsEmpty())
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::RemoveAnnotation;
    change.annotation_id = id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

bool UiDocCore::UpdateAnnotation(const String& id, const ValueMap& values)
{
    if(id.IsEmpty())
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::UpdateAnnotation;
    change.annotation_id = id;
    change.values = clone(values);
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

bool UiDocCore::SetAnnotationFlags(const String& id, dword mask,
                                   bool expanded, bool printable, bool resolved)
{
    if(id.IsEmpty())
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::SetAnnotationFlags;
    change.annotation_id = id;
    change.flag_mask = mask;
    change.expanded = expanded;
    change.printable = printable;
    change.resolved = resolved;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

Vector<UiDocAnnotation> UiDocCore::QueryAnnotations(const UiDocRange* range, const String& type) const
{
    Vector<UiDocAnnotation> out;
    UiDocRange query;
    if(range)
        query = NormalizeRange(*range);
    for(const UiDocAnnotation& annotation : annotations_) {
        if(!type.IsEmpty() && annotation.type != type)
            continue;
        if(range && !UiDocRangesTouch(annotation.range, query))
            continue;
        out.Add(annotation);
    }
    return out;
}

String UiDocCore::AddResource(const UiDocResource& resource, bool dedupe)
{
    if(resource.resource_type.IsEmpty())
        return String();
    if(dedupe && !resource.content_hash.IsEmpty())
        for(const UiDocResource& existing : resources_)
            if(existing.content_hash == resource.content_hash &&
               existing.resource_type == resource.resource_type)
                return existing.key;

    UiDocCoreChange change;
    change.type = UiDocCoreChange::AddResource;
    change.resource = resource;
    if(change.resource.key.IsEmpty())
        change.resource.key = Format("res_%d", next_resource_id_++);
    String key = change.resource.key;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok ? key : String();
}

bool UiDocCore::RemoveResource(const String& key)
{
    if(key.IsEmpty())
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::RemoveResource;
    change.resource_key = key;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

bool UiDocCore::GetResource(const String& key, UiDocResource& out) const
{
    int q = FindResource(key);
    if(q < 0)
        return false;
    out = resources_[q];
    return true;
}

String UiDocCore::AddEmbed(int pos, const String& type,
                           const ValueMap& payload, const ValueMap& layout, const ValueMap& meta)
{
    if(type.IsEmpty())
        return String();
    UiDocCoreChange change;
    change.type = UiDocCoreChange::AddEmbed;
    change.embed.id = Format("embed_%d", next_embed_id_++);
    change.embed.type = type;
    int at = ClampPos(pos);
    change.embed.range = UiDocRange(at, at);
    change.embed.payload = clone(payload);
    change.embed.layout = clone(layout);
    change.embed.meta = clone(meta);
    String id = change.embed.id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok ? id : String();
}

bool UiDocCore::RemoveEmbed(const String& id)
{
    if(id.IsEmpty())
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::RemoveEmbed;
    change.embed_id = id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

bool UiDocCore::UpdateEmbed(const UiDocEmbedBlock& embed)
{
    if(embed.id.IsEmpty() || embed.type.IsEmpty())
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::UpdateEmbed;
    change.embed = embed;
    change.embed_id = embed.id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

Vector<UiDocEmbedBlock> UiDocCore::QueryEmbeds(const UiDocRange* range, const String& type) const
{
    Vector<UiDocEmbedBlock> out;
    UiDocRange query;
    if(range)
        query = NormalizeRange(*range);
    for(const UiDocEmbedBlock& embed : embeds_) {
        if(!type.IsEmpty() && embed.type != type)
            continue;
        if(range && !UiDocRangesTouch(embed.range, query))
            continue;
        out.Add(embed);
    }
    return out;
}

bool UiDocCore::SetAnchor(const String& id, int pos)
{
    if(id.IsEmpty())
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::SetAnchor;
    change.key = id;
    change.pos = pos;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

bool UiDocCore::RemoveAnchor(const String& id)
{
    if(id.IsEmpty())
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::RemoveAnchor;
    change.key = id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

bool UiDocCore::ResolveAnchor(const String& id, int& pos) const
{
    int q = anchors_.Find(id);
    if(q < 0)
        return false;
    pos = anchors_[q];
    return true;
}

bool UiDocCore::Undo()
{
    if(undo_.IsEmpty())
        return false;
    uint64 before = revision_;
    HistoryRecord record = pick(undo_.Top());
    undo_.Drop();
    ApplyHistoryRecord(record, true);
    RebuildIds();
    redo_.Add(pick(record));
    Touch();

    UiDocApplyResult result;
    result.ok = true;
    result.revision_before = before;
    result.revision_after = revision_;
    WhenChange(result);
    return true;
}

bool UiDocCore::Redo()
{
    if(redo_.IsEmpty())
        return false;
    uint64 before = revision_;
    HistoryRecord record = pick(redo_.Top());
    redo_.Drop();
    ApplyHistoryRecord(record, false);
    RebuildIds();
    undo_.Add(pick(record));
    Touch();

    UiDocApplyResult result;
    result.ok = true;
    result.revision_before = before;
    result.revision_after = revision_;
    WhenChange(result);
    return true;
}

}
