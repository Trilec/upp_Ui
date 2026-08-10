#include "UiDocCore.h"

namespace Upp {

static void UiDocMergeMap(ValueMap& dst, const ValueMap& src)
{
    for(int i = 0; i < src.GetCount(); i++)
        dst.GetAdd(AsString(src.GetKey(i))) = src.GetValue(i);
}

static bool UiDocValueUsesResource(const Value& value, const String& key)
{
    if(value.Is<ValueMap>()) {
        ValueMap map = value;
        for(int i = 0; i < map.GetCount(); i++) {
            String name = AsString(map.GetKey(i));
            Value child = map.GetValue(i);
            if(name == "resource_key" && AsString(child) == key)
                return true;
            if(UiDocValueUsesResource(child, key))
                return true;
        }
    }
    else if(value.Is<ValueArray>()) {
        ValueArray array = value;
        for(int i = 0; i < array.GetCount(); i++)
            if(UiDocValueUsesResource(array[i], key))
                return true;
    }
    return false;
}

static void UiDocIncludeRange(UiDocRange& dst, UiDocRange src)
{
    src.Normalize();
    if(dst.IsEmpty())
        dst = src;
    else {
        dst.from = min(dst.from, src.from);
        dst.to = max(dst.to, src.to);
    }
}

bool UiDocCore::ApplyOne(const UiDocCoreChange& change, UiDocApplyResult& result, HistoryStep& history)
{
    switch(change.type) {
    case UiDocCoreChange::ReplaceText: {
        UiDocRange range = NormalizeRange(change.range);
        int old_len = range.to - range.from;
        history.kind = HistoryStep::Text;
        history.at = range.from;
        history.before_text = text_.Mid(range.from, old_len);
        history.after_text = change.text;
        history.before_styles <<= styles_;
        history.before_blocks <<= blocks_;
        history.before_annotations <<= annotations_;
        history.before_embeds <<= embeds_;
        history.before_anchors <<= anchors_;

        text_.Remove(range.from, old_len);
        text_.Insert(range.from, change.text);

        UiDocPositionMap map;
        UiDocPositionMapEntry& edit = map.edits.Add();
        edit.at = range.from;
        edit.old_len = old_len;
        edit.new_len = change.text.GetCount();
        MapRanges(map);
        result.positions.edits.Append(map.edits);

        history.after_styles <<= styles_;
        history.after_blocks <<= blocks_;
        history.after_annotations <<= annotations_;
        history.after_embeds <<= embeds_;
        history.after_anchors <<= anchors_;

        UiDocRange changed(range.from, range.from + max(old_len, change.text.GetCount()));
        UiDocIncludeRange(result.changed_range, changed);
        return true;
    }

    case UiDocCoreChange::SetStyle:
        history.kind = HistoryStep::Styles;
        history.before_styles <<= styles_;
        ReplaceStyleRange(change.range, change.style, change.style_mask ? change.style_mask : STYLE_ALL);
        history.after_styles <<= styles_;
        UiDocIncludeRange(result.changed_range, NormalizeRange(change.range));
        return true;

    case UiDocCoreChange::SetMark:
        history.kind = HistoryStep::Styles;
        history.before_styles <<= styles_;
        ReplaceMarkRange(change.range, change.mark, change.enabled);
        history.after_styles <<= styles_;
        UiDocIncludeRange(result.changed_range, NormalizeRange(change.range));
        return true;

    case UiDocCoreChange::AddBlock:
    case UiDocCoreChange::RemoveBlock:
    case UiDocCoreChange::UpdateBlock: {
        history.kind = HistoryStep::Blocks;
        history.before_blocks <<= blocks_;
        bool ok = true;
        if(change.type == UiDocCoreChange::AddBlock) {
            UiDocBlock block = change.block;
            block.range = NormalizeRange(block.range);
            block.indent = max(0, block.indent);
            if(block.id.IsEmpty())
                block.id = Format("block_%d", next_block_id_++);
            if(FindBlock(block.id) >= 0)
                ok = false;
            else
                blocks_.Add(pick(block));
        }
        else {
            String id = change.block_id.IsEmpty() ? change.block.id : change.block_id;
            int q = FindBlock(id);
            if(q < 0)
                ok = false;
            else if(change.type == UiDocCoreChange::RemoveBlock)
                blocks_.Remove(q);
            else {
                UiDocBlock block = change.block;
                block.id = id;
                block.range = NormalizeRange(block.range);
                block.indent = max(0, block.indent);
                blocks_[q] = pick(block);
            }
        }
        if(ok)
            history.after_blocks <<= blocks_;
        return ok;
    }

    case UiDocCoreChange::AddAnnotation:
    case UiDocCoreChange::RemoveAnnotation:
    case UiDocCoreChange::UpdateAnnotation:
    case UiDocCoreChange::SetAnnotationFlags: {
        history.kind = HistoryStep::Annotations;
        history.before_annotations <<= annotations_;
        bool ok = true;
        if(change.type == UiDocCoreChange::AddAnnotation) {
            UiDocAnnotation annotation = change.annotation;
            annotation.range = NormalizeRange(annotation.range);
            if(annotation.type.IsEmpty())
                ok = false;
            if(ok && annotation.id.IsEmpty())
                annotation.id = Format("ann_%d", next_annotation_id_++);
            if(ok && FindAnnotation(annotation.id) >= 0)
                ok = false;
            if(ok)
                annotations_.Add(pick(annotation));
        }
        else {
            int q = FindAnnotation(change.annotation_id);
            if(q < 0)
                ok = false;
            else if(change.type == UiDocCoreChange::RemoveAnnotation)
                annotations_.Remove(q);
            else if(change.type == UiDocCoreChange::UpdateAnnotation)
                UiDocMergeMap(annotations_[q].payload, change.values);
            else {
                if(change.flag_mask & ANNOT_EXPANDED)  annotations_[q].expanded = change.expanded;
                if(change.flag_mask & ANNOT_PRINTABLE) annotations_[q].printable = change.printable;
                if(change.flag_mask & ANNOT_RESOLVED)  annotations_[q].resolved = change.resolved;
            }
        }
        if(ok)
            history.after_annotations <<= annotations_;
        return ok;
    }

    case UiDocCoreChange::AddResource:
    case UiDocCoreChange::RemoveResource: {
        history.kind = HistoryStep::Resources;
        history.before_resources <<= resources_;
        bool ok = true;
        if(change.type == UiDocCoreChange::AddResource) {
            UiDocResource resource = change.resource;
            if(resource.resource_type.IsEmpty())
                ok = false;
            if(ok && resource.key.IsEmpty())
                resource.key = Format("res_%d", next_resource_id_++);
            if(ok && FindResource(resource.key) >= 0)
                ok = false;
            if(ok)
                resources_.Add(pick(resource));
        }
        else {
            int q = FindResource(change.resource_key);
            if(q < 0)
                ok = false;
            if(ok) {
                for(const UiDocBlock& block : blocks_)
                    if(UiDocValueUsesResource(block.meta, change.resource_key))
                        ok = false;
                for(const UiDocAnnotation& annotation : annotations_)
                    if(UiDocValueUsesResource(annotation.payload, change.resource_key) ||
                       UiDocValueUsesResource(annotation.meta, change.resource_key))
                        ok = false;
                for(const UiDocEmbedBlock& embed : embeds_)
                    if(UiDocValueUsesResource(embed.payload, change.resource_key) ||
                       UiDocValueUsesResource(embed.layout, change.resource_key) ||
                       UiDocValueUsesResource(embed.meta, change.resource_key))
                        ok = false;
                if(UiDocValueUsesResource(meta_, change.resource_key))
                    ok = false;
            }
            if(ok)
                resources_.Remove(q);
        }
        if(ok)
            history.after_resources <<= resources_;
        return ok;
    }

    case UiDocCoreChange::AddEmbed:
    case UiDocCoreChange::RemoveEmbed:
    case UiDocCoreChange::UpdateEmbed: {
        history.kind = HistoryStep::Embeds;
        history.before_embeds <<= embeds_;
        bool ok = true;
        if(change.type == UiDocCoreChange::AddEmbed) {
            UiDocEmbedBlock embed = change.embed;
            embed.range = NormalizeRange(embed.range);
            if(embed.type.IsEmpty())
                ok = false;
            if(ok && embed.id.IsEmpty())
                embed.id = Format("embed_%d", next_embed_id_++);
            if(ok && FindEmbed(embed.id) >= 0)
                ok = false;
            if(ok)
                embeds_.Add(pick(embed));
        }
        else {
            String id = change.embed_id.IsEmpty() ? change.embed.id : change.embed_id;
            int q = FindEmbed(id);
            if(q < 0)
                ok = false;
            else if(change.type == UiDocCoreChange::RemoveEmbed)
                embeds_.Remove(q);
            else {
                UiDocEmbedBlock embed = change.embed;
                embed.id = id;
                embed.range = NormalizeRange(embed.range);
                if(embed.type.IsEmpty())
                    ok = false;
                else
                    embeds_[q] = pick(embed);
            }
        }
        if(ok)
            history.after_embeds <<= embeds_;
        return ok;
    }

    case UiDocCoreChange::SetDocumentMeta:
        if(change.key.IsEmpty())
            return false;
        history.kind = HistoryStep::Meta;
        history.before_meta = clone(meta_);
        if(IsNull(change.value)) {
            int q = meta_.Find(change.key);
            if(q >= 0)
                meta_.Remove(q);
        }
        else
            meta_.GetAdd(change.key) = change.value;
        history.after_meta = clone(meta_);
        return true;

    case UiDocCoreChange::SetAnchor:
    case UiDocCoreChange::RemoveAnchor:
        history.kind = HistoryStep::Anchors;
        history.before_anchors <<= anchors_;
        if(change.type == UiDocCoreChange::SetAnchor) {
            if(change.key.IsEmpty())
                return false;
            anchors_.GetAdd(change.key) = ClampPos(change.pos);
        }
        else {
            int q = anchors_.Find(change.key);
            if(q < 0)
                return false;
            anchors_.Remove(q);
        }
        history.after_anchors <<= anchors_;
        return true;
    }
    return false;
}

UiDocApplyResult UiDocCore::Apply(const UiDocCoreTransaction& tx)
{
    UiDocApplyResult result;
    result.revision_before = revision_;
    result.revision_after = revision_;

    if(tx.base_revision && tx.base_revision != revision_) {
        result.error = Format("revision mismatch: expected %llu, current %llu",
                              (unsigned long long)tx.base_revision,
                              (unsigned long long)revision_);
        return result;
    }
    if(tx.changes.IsEmpty()) {
        result.ok = true;
        return result;
    }

    HistoryRecord record;
    for(const UiDocCoreChange& change : tx.changes) {
        HistoryStep& step = record.steps.Add();
        if(!ApplyOne(change, result, step)) {
            record.steps.Drop();
            ApplyHistoryRecord(record, true);
            RebuildIds();
            result.positions.Clear();
            result.changed_range = UiDocRange();
            result.error = "transaction refused";
            return result;
        }
    }

    if(tx.add_to_history && history_limit_ > 0) {
        undo_.Add(pick(record));
        while(undo_.GetCount() > history_limit_)
            undo_.Remove(0);
        redo_.Clear();
    }

    Touch();
    result.ok = true;
    result.revision_after = revision_;
    WhenChange(result);
    return result;
}

UiDocApplyResult UiDocCore::Replace(UiDocRange range, const WString& text, uint64 base_revision)
{
    UiDocCoreChange change;
    change.type = UiDocCoreChange::ReplaceText;
    change.range = range;
    change.text = text;
    UiDocCoreTransaction tx;
    tx.base_revision = base_revision;
    tx.changes.Add(pick(change));
    return Apply(tx);
}

UiDocApplyResult UiDocCore::SetStyle(UiDocRange range, const UiDocTextStyle& style,
                                     dword mask, uint64 base_revision)
{
    UiDocCoreChange change;
    change.type = UiDocCoreChange::SetStyle;
    change.range = range;
    change.style = style;
    change.style_mask = mask;
    UiDocCoreTransaction tx;
    tx.base_revision = base_revision;
    tx.changes.Add(pick(change));
    return Apply(tx);
}

UiDocApplyResult UiDocCore::SetMark(UiDocRange range, UiDocTextStyle::Mark mark,
                                    bool enabled, uint64 base_revision)
{
    UiDocCoreChange change;
    change.type = UiDocCoreChange::SetMark;
    change.range = range;
    change.mark = (byte)mark;
    change.enabled = enabled;
    UiDocCoreTransaction tx;
    tx.base_revision = base_revision;
    tx.changes.Add(pick(change));
    return Apply(tx);
}

UiDocApplyResult UiDocCore::SetInk(UiDocRange range, Color ink, uint64 base_revision)
{
    UiDocTextStyle style;
    style.ink = ink;
    return SetStyle(range, style, STYLE_INK, base_revision);
}

UiDocApplyResult UiDocCore::SetFont(UiDocRange range, const String& face, int height,
                                    uint64 base_revision)
{
    UiDocTextStyle style;
    style.font_face = face;
    style.font_height = max(0, height);
    dword mask = STYLE_FONT_FACE;
    if(height >= 0)
        mask |= STYLE_FONT_HEIGHT;
    return SetStyle(range, style, mask, base_revision);
}

}
