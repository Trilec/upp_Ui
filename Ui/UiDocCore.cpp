#include "UiDocCore.h"

namespace Upp {

static bool UiDocStyleEqual(const UiDocTextStyle& a, const UiDocTextStyle& b)
{
    return a.flags == b.flags && a.ink == b.ink && a.font_face == b.font_face &&
           a.font_height == b.font_height && a.size_delta == b.size_delta &&
           a.leading_delta == b.leading_delta && a.tracking_delta == b.tracking_delta;
}

static void UiDocApplyStyleMask(UiDocTextStyle& dst, const UiDocTextStyle& src, dword mask)
{
    if(mask & UiDocCore::STYLE_FLAGS)          dst.flags = src.flags;
    if(mask & UiDocCore::STYLE_INK)            dst.ink = src.ink;
    if(mask & UiDocCore::STYLE_FONT_FACE)      dst.font_face = src.font_face;
    if(mask & UiDocCore::STYLE_FONT_HEIGHT)    dst.font_height = src.font_height;
    if(mask & UiDocCore::STYLE_SIZE_DELTA)     dst.size_delta = src.size_delta;
    if(mask & UiDocCore::STYLE_LEADING_DELTA)  dst.leading_delta = src.leading_delta;
    if(mask & UiDocCore::STYLE_TRACKING_DELTA) dst.tracking_delta = src.tracking_delta;
}

static void UiDocMapRange(UiDocRange& range, const UiDocPositionMap& map)
{
    if(range.IsEmpty()) {
        int pos = map.Map(range.from, UiDocPositionMap::Right);
        range.from = range.to = pos;
    }
    else {
        range.from = map.Map(range.from, UiDocPositionMap::Left);
        range.to = map.Map(range.to, UiDocPositionMap::Right);
    }
}

static bool UiDocResourceRefsValid(const Value& value, const Vector<UiDocResource>& resources)
{
    if(value.Is<ValueMap>()) {
        ValueMap map = value;
        for(int i = 0; i < map.GetCount(); i++) {
            String key = AsString(map.GetKey(i));
            Value child = map.GetValue(i);
            if(key == "resource_key") {
                String resource_key = AsString(child);
                bool found = false;
                for(const UiDocResource& resource : resources)
                    if(resource.key == resource_key) {
                        found = true;
                        break;
                    }
                if(!found)
                    return false;
            }
            if(!UiDocResourceRefsValid(child, resources))
                return false;
        }
    }
    else if(value.Is<ValueArray>()) {
        ValueArray array = value;
        for(int i = 0; i < array.GetCount(); i++)
            if(!UiDocResourceRefsValid(array[i], resources))
                return false;
    }
    return true;
}

static int UiDocNumericSuffix(const String& id, const char *prefix)
{
    String p(prefix);
    if(!id.StartsWith(p))
        return 0;
    return max(0, StrInt(id.Mid(p.GetCount())));
}

int UiDocPositionMap::Map(int pos, Bias bias) const
{
    int p = pos;
    for(const UiDocPositionMapEntry& e : edits) {
        if(e.old_len == 0) {
            if(p < e.at)
                continue;
            if(p == e.at) {
                if(bias == Right)
                    p = e.at + e.new_len;
                continue;
            }
            p += e.new_len;
            continue;
        }

        int old_to = e.at + e.old_len;
        if(p < e.at)
            continue;
        if(p > old_to) {
            p += e.new_len - e.old_len;
            continue;
        }
        p = bias == Left ? e.at : e.at + e.new_len;
    }
    return p;
}

UiDocCore::UiDocCore()
{
    Clear();
}

int UiDocCore::ClampPos(int pos) const
{
    return clamp(pos, 0, text_.GetCount());
}

UiDocRange UiDocCore::NormalizeRange(UiDocRange range) const
{
    range.Normalize();
    range.from = ClampPos(range.from);
    range.to = ClampPos(range.to);
    return range;
}

void UiDocCore::ApplyHistoryStep(const HistoryStep& step, bool before)
{
    switch(step.kind) {
    case HistoryStep::Text: {
        const WString& remove = before ? step.after_text : step.before_text;
        const WString& insert = before ? step.before_text : step.after_text;
        text_.Remove(step.at, remove.GetCount());
        text_.Insert(step.at, insert);
        styles_ <<= (before ? step.before_styles : step.after_styles);
        blocks_ <<= (before ? step.before_blocks : step.after_blocks);
        annotations_ <<= (before ? step.before_annotations : step.after_annotations);
        embeds_ <<= (before ? step.before_embeds : step.after_embeds);
        anchors_ <<= (before ? step.before_anchors : step.after_anchors);
        break;
    }
    case HistoryStep::Styles:
        styles_ <<= (before ? step.before_styles : step.after_styles);
        break;
    case HistoryStep::Blocks:
        blocks_ <<= (before ? step.before_blocks : step.after_blocks);
        break;
    case HistoryStep::Annotations:
        annotations_ <<= (before ? step.before_annotations : step.after_annotations);
        break;
    case HistoryStep::Resources:
        resources_ <<= (before ? step.before_resources : step.after_resources);
        break;
    case HistoryStep::Embeds:
        embeds_ <<= (before ? step.before_embeds : step.after_embeds);
        break;
    case HistoryStep::Meta:
        meta_ = clone(before ? step.before_meta : step.after_meta);
        break;
    case HistoryStep::Anchors:
        anchors_ <<= (before ? step.before_anchors : step.after_anchors);
        break;
    }
}

void UiDocCore::ApplyHistoryRecord(const HistoryRecord& record, bool before)
{
    if(before) {
        for(int i = record.steps.GetCount() - 1; i >= 0; --i)
            ApplyHistoryStep(record.steps[i], true);
    }
    else {
        for(const HistoryStep& step : record.steps)
            ApplyHistoryStep(step, false);
    }
}

void UiDocCore::Touch()
{
    revision_ = revision_ == std::numeric_limits<uint64>::max() ? 1 : revision_ + 1;
}

void UiDocCore::RebuildIds()
{
    next_block_id_ = 1;
    next_annotation_id_ = 1;
    next_resource_id_ = 1;
    next_embed_id_ = 1;

    for(const UiDocBlock& block : blocks_)
        next_block_id_ = max(next_block_id_, UiDocNumericSuffix(block.id, "block_") + 1);
    for(const UiDocAnnotation& annotation : annotations_)
        next_annotation_id_ = max(next_annotation_id_, UiDocNumericSuffix(annotation.id, "ann_") + 1);
    for(const UiDocResource& resource : resources_)
        next_resource_id_ = max(next_resource_id_, UiDocNumericSuffix(resource.key, "res_") + 1);
    for(const UiDocEmbedBlock& embed : embeds_)
        next_embed_id_ = max(next_embed_id_, UiDocNumericSuffix(embed.id, "embed_") + 1);
}

void UiDocCore::Clear()
{
    text_.Clear();
    styles_.Clear();
    blocks_.Clear();
    annotations_.Clear();
    resources_.Clear();
    embeds_.Clear();
    anchors_.Clear();
    meta_.Clear();
    undo_.Clear();
    redo_.Clear();
    RebuildIds();
    Touch();
}

bool UiDocCore::Validate(String* error) const
{
    auto Fail = [&](const String& text) {
        if(error)
            *error = text;
        return false;
    };

    int last_to = 0;
    for(const UiDocStyleRun& run : styles_) {
        if(run.from < 0 || run.to > text_.GetCount() || run.from >= run.to)
            return Fail("invalid style range");
        if(run.from < last_to)
            return Fail("overlapping style runs");
        if(run.style.IsDefault())
            return Fail("default style stored as sparse run");
        last_to = run.to;
    }

    for(int i = 0; i < blocks_.GetCount(); i++) {
        const UiDocBlock& block = blocks_[i];
        if(block.id.IsEmpty() || block.range.from < 0 || block.range.to > text_.GetCount() || block.range.from > block.range.to)
            return Fail("invalid block");
        for(int j = i + 1; j < blocks_.GetCount(); j++)
            if(blocks_[j].id == block.id)
                return Fail("duplicate block id");
    }

    for(int i = 0; i < annotations_.GetCount(); i++) {
        const UiDocAnnotation& annotation = annotations_[i];
        if(annotation.id.IsEmpty() || annotation.type.IsEmpty() || annotation.range.from < 0 ||
           annotation.range.to > text_.GetCount() || annotation.range.from > annotation.range.to)
            return Fail("invalid annotation");
        for(int j = i + 1; j < annotations_.GetCount(); j++)
            if(annotations_[j].id == annotation.id)
                return Fail("duplicate annotation id");
    }

    for(int i = 0; i < resources_.GetCount(); i++) {
        const UiDocResource& resource = resources_[i];
        if(resource.key.IsEmpty() || resource.resource_type.IsEmpty())
            return Fail("invalid resource");
        for(int j = i + 1; j < resources_.GetCount(); j++)
            if(resources_[j].key == resource.key)
                return Fail("duplicate resource key");
    }

    for(int i = 0; i < embeds_.GetCount(); i++) {
        const UiDocEmbedBlock& embed = embeds_[i];
        if(embed.id.IsEmpty() || embed.type.IsEmpty() || embed.range.from < 0 ||
           embed.range.to > text_.GetCount() || embed.range.from > embed.range.to)
            return Fail("invalid embed");
        for(int j = i + 1; j < embeds_.GetCount(); j++)
            if(embeds_[j].id == embed.id)
                return Fail("duplicate embed id");
    }

    if(!UiDocResourceRefsValid(meta_, resources_))
        return Fail("dangling document resource reference");
    for(const UiDocBlock& block : blocks_)
        if(!UiDocResourceRefsValid(block.meta, resources_))
            return Fail("dangling block resource reference");
    for(const UiDocAnnotation& annotation : annotations_)
        if(!UiDocResourceRefsValid(annotation.payload, resources_) ||
           !UiDocResourceRefsValid(annotation.meta, resources_))
            return Fail("dangling annotation resource reference");
    for(const UiDocEmbedBlock& embed : embeds_) {
        if(!UiDocResourceRefsValid(embed.payload, resources_) ||
           !UiDocResourceRefsValid(embed.layout, resources_) ||
           !UiDocResourceRefsValid(embed.meta, resources_))
            return Fail("dangling embed resource reference");
        if(embed.type == "table") {
            UiDocTable table;
            if(!GetTable(embed.id, table))
                return Fail("invalid table payload");
        }
    }

    for(int i = 0; i < anchors_.GetCount(); i++)
        if(anchors_[i] < 0 || anchors_[i] > text_.GetCount())
            return Fail("invalid anchor");

    if(error)
        error->Clear();
    return true;
}

void UiDocCore::SetHistoryLimit(int count)
{
    history_limit_ = max(0, count);
    while(undo_.GetCount() > history_limit_)
        undo_.Remove(0);
    while(redo_.GetCount() > history_limit_)
        redo_.Remove(0);
}

WString UiDocCore::GetSlice(UiDocRange range) const
{
    range = NormalizeRange(range);
    return text_.Mid(range.from, range.to - range.from);
}

Value UiDocCore::GetMeta(const String& key) const
{
    int q = meta_.Find(key);
    return q >= 0 ? meta_.GetValue(q) : Value();
}

bool UiDocCore::SetMeta(const String& key, const Value& value)
{
    if(key.IsEmpty())
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::SetDocumentMeta;
    change.key = key;
    change.value = value;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

bool UiDocCore::RemoveMeta(const String& key)
{
    if(key.IsEmpty() || meta_.Find(key) < 0)
        return false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::SetDocumentMeta;
    change.key = key;
    change.value = Null;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(change));
    return Apply(tx).ok;
}

void UiDocCore::NormalizeStyles()
{
    Sort(styles_, [](const UiDocStyleRun& a, const UiDocStyleRun& b) {
        return a.from < b.from || (a.from == b.from && a.to < b.to);
    });

    Vector<UiDocStyleRun> out;
    for(UiDocStyleRun run : styles_) {
        run.from = clamp(run.from, 0, text_.GetCount());
        run.to = clamp(run.to, run.from, text_.GetCount());
        if(run.from == run.to || run.style.IsDefault())
            continue;
        if(!out.IsEmpty() && out.Top().to == run.from && UiDocStyleEqual(out.Top().style, run.style))
            out.Top().to = run.to;
        else
            out.Add(pick(run));
    }
    styles_ = pick(out);
}

void UiDocCore::ReplaceStyleRange(UiDocRange range, const UiDocTextStyle& style, dword mask)
{
    range = NormalizeRange(range);
    if(range.IsEmpty())
        return;

    Vector<UiDocStyleRun> out;
    int cursor = range.from;

    for(const UiDocStyleRun& old : styles_) {
        if(old.to <= range.from || old.from >= range.to) {
            out.Add(old);
            continue;
        }

        if(old.from < range.from) {
            UiDocStyleRun left = old;
            left.to = range.from;
            out.Add(left);
        }

        int from = max(old.from, range.from);
        int to = min(old.to, range.to);
        if(cursor < from) {
            UiDocStyleRun gap;
            gap.from = cursor;
            gap.to = from;
            UiDocApplyStyleMask(gap.style, style, mask);
            out.Add(pick(gap));
        }

        UiDocStyleRun mid = old;
        mid.from = from;
        mid.to = to;
        UiDocApplyStyleMask(mid.style, style, mask);
        out.Add(pick(mid));
        cursor = to;

        if(old.to > range.to) {
            UiDocStyleRun right = old;
            right.from = range.to;
            out.Add(right);
        }
    }

    if(cursor < range.to) {
        UiDocStyleRun tail;
        tail.from = cursor;
        tail.to = range.to;
        UiDocApplyStyleMask(tail.style, style, mask);
        out.Add(pick(tail));
    }

    styles_ = pick(out);
    NormalizeStyles();
}

void UiDocCore::ReplaceMarkRange(UiDocRange range, byte mark, bool enabled)
{
    range = NormalizeRange(range);
    if(range.IsEmpty() || !mark)
        return;

    Vector<UiDocStyleRun> out;
    int cursor = range.from;

    for(const UiDocStyleRun& old : styles_) {
        if(old.to <= range.from || old.from >= range.to) {
            out.Add(old);
            continue;
        }

        if(old.from < range.from) {
            UiDocStyleRun left = old;
            left.to = range.from;
            out.Add(left);
        }

        int from = max(old.from, range.from);
        int to = min(old.to, range.to);
        if(cursor < from && enabled) {
            UiDocStyleRun gap;
            gap.from = cursor;
            gap.to = from;
            gap.style.flags = mark;
            out.Add(pick(gap));
        }

        UiDocStyleRun mid = old;
        mid.from = from;
        mid.to = to;
        if(enabled)
            mid.style.flags |= mark;
        else
            mid.style.flags &= ~mark;
        out.Add(pick(mid));
        cursor = to;

        if(old.to > range.to) {
            UiDocStyleRun right = old;
            right.from = range.to;
            out.Add(right);
        }
    }

    if(cursor < range.to && enabled) {
        UiDocStyleRun tail;
        tail.from = cursor;
        tail.to = range.to;
        tail.style.flags = mark;
        out.Add(pick(tail));
    }

    styles_ = pick(out);
    NormalizeStyles();
}

void UiDocCore::MapRanges(const UiDocPositionMap& map)
{
    for(UiDocStyleRun& style : styles_) {
        style.from = map.Map(style.from, UiDocPositionMap::Left);
        style.to = map.Map(style.to, UiDocPositionMap::Right);
    }
    NormalizeStyles();

    for(UiDocBlock& block : blocks_)
        UiDocMapRange(block.range, map);
    for(UiDocAnnotation& annotation : annotations_)
        UiDocMapRange(annotation.range, map);
    for(UiDocEmbedBlock& embed : embeds_)
        UiDocMapRange(embed.range, map);
    for(int i = 0; i < anchors_.GetCount(); i++)
        anchors_[i] = map.Map(anchors_[i], UiDocPositionMap::Right);
}

int UiDocCore::FindBlock(const String& id) const
{
    for(int i = 0; i < blocks_.GetCount(); i++)
        if(blocks_[i].id == id)
            return i;
    return -1;
}

int UiDocCore::FindAnnotation(const String& id) const
{
    for(int i = 0; i < annotations_.GetCount(); i++)
        if(annotations_[i].id == id)
            return i;
    return -1;
}

int UiDocCore::FindResource(const String& key) const
{
    for(int i = 0; i < resources_.GetCount(); i++)
        if(resources_[i].key == key)
            return i;
    return -1;
}

int UiDocCore::FindEmbed(const String& id) const
{
    for(int i = 0; i < embeds_.GetCount(); i++)
        if(embeds_[i].id == id)
            return i;
    return -1;
}

}
