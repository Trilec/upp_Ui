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

static void UiDocMergeMap(ValueMap& dst, const ValueMap& src)
{
    for(int i = 0; i < src.GetCount(); i++)
        dst.GetAdd(AsString(src.GetKey(i))) = src.GetValue(i);
}

static bool UiDocStyleValueEqual(const UiDocStyleRun& a, const UiDocStyleRun& b)
{
    return a.flags == b.flags && a.ink == b.ink && a.font_face == b.font_face &&
           a.font_height == b.font_height && a.size_delta == b.size_delta &&
           a.leading_delta == b.leading_delta && a.tracking_delta == b.tracking_delta;
}

UiDocCore::UiDocCore()
{
    Clear();
}

int UiDocCore::ClampPos(int pos) const
{
    return clamp(pos, 0, text_.GetCount());
}

UiDocRange UiDocCore::NormalizeRange(UiDocRange r) const
{
    r.Normalize();
    r.from = ClampPos(r.from);
    r.to = ClampPos(r.to);
    return r;
}

void UiDocCore::ApplyHistoryStep(const HistoryStep& s, bool before)
{
    switch(s.kind) {
    case HistoryStep::Text: {
        const WString& remove = before ? s.after_text : s.before_text;
        const WString& insert = before ? s.before_text : s.after_text;
        text_.Remove(s.at, remove.GetCount());
        text_.Insert(s.at, insert);
        styles_ <<= (before ? s.before_styles : s.after_styles);
        annotations_ <<= (before ? s.before_annotations : s.after_annotations);
        embeds_ <<= (before ? s.before_embeds : s.after_embeds);
        anchors_ <<= (before ? s.before_anchors : s.after_anchors);
        break;
    }
    case HistoryStep::Styles:
        styles_ <<= (before ? s.before_styles : s.after_styles);
        break;
    case HistoryStep::Annotations:
        annotations_ <<= (before ? s.before_annotations : s.after_annotations);
        break;
    case HistoryStep::Resources:
        resources_ <<= (before ? s.before_resources : s.after_resources);
        break;
    case HistoryStep::Embeds:
        embeds_ <<= (before ? s.before_embeds : s.after_embeds);
        break;
    case HistoryStep::Meta:
        meta_ = clone(before ? s.before_meta : s.after_meta);
        break;
    case HistoryStep::Anchors:
        anchors_ <<= (before ? s.before_anchors : s.after_anchors);
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
    if(revision_ == std::numeric_limits<uint64>::max())
        revision_ = 1;
    else
        ++revision_;
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
    revision_ = 1;
    next_annotation_id_ = 1;
    next_resource_id_ = 1;
    next_embed_id_ = 1;
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

void UiDocCore::SetMeta(const String& key, const Value& value)
{
    if(key.IsEmpty())
        return;
    UiDocCoreChange c;
    c.type = UiDocCoreChange::SetDocumentMeta;
    c.key = key;
    c.value = value;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
    Apply(tx);
}

void UiDocCore::RemoveMeta(const String& key)
{
    if(meta_.Find(key) < 0)
        return;
    UiDocCoreChange c;
    c.type = UiDocCoreChange::SetDocumentMeta;
    c.key = key;
    c.value = Null;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
    Apply(tx);
}

void UiDocCore::NormalizeStyles()
{
    Vector<UiDocStyleRun> out;
    Sort(styles_, [](const UiDocStyleRun& a, const UiDocStyleRun& b) {
        return a.from < b.from || (a.from == b.from && a.to < b.to);
    });

    for(UiDocStyleRun r : styles_) {
        r.from = clamp(r.from, 0, text_.GetCount());
        r.to = clamp(r.to, r.from, text_.GetCount());
        if(r.from == r.to || r.IsDefault())
            continue;
        if(!out.IsEmpty() && out.Top().to == r.from && UiDocStyleValueEqual(out.Top(), r))
            out.Top().to = r.to;
        else
            out.Add(pick(r));
    }
    styles_ = pick(out);
}

void UiDocCore::ReplaceStyleRange(UiDocRange range, const UiDocStyleRun& style, dword mask)
{
    range = NormalizeRange(range);
    if(range.IsEmpty())
        return;

    Vector<UiDocStyleRun> split;
    int cursor = range.from;

    for(const UiDocStyleRun& old : styles_) {
        if(old.to <= range.from || old.from >= range.to) {
            split.Add(old);
            continue;
        }
        if(old.from < range.from) {
            UiDocStyleRun left = old;
            left.to = range.from;
            split.Add(left);
        }

        int a = max(old.from, range.from);
        int b = min(old.to, range.to);
        if(cursor < a) {
            UiDocStyleRun gap;
            gap.from = cursor;
            gap.to = a;
            split.Add(gap);
        }

        UiDocStyleRun mid = old;
        mid.from = a;
        mid.to = b;
        if(mask & STYLE_FLAGS)          mid.flags = style.flags;
        if(mask & STYLE_INK)            mid.ink = style.ink;
        if(mask & STYLE_FONT_FACE)      mid.font_face = style.font_face;
        if(mask & STYLE_FONT_HEIGHT)    mid.font_height = style.font_height;
        if(mask & STYLE_SIZE_DELTA)     mid.size_delta = style.size_delta;
        if(mask & STYLE_LEADING_DELTA)  mid.leading_delta = style.leading_delta;
        if(mask & STYLE_TRACKING_DELTA) mid.tracking_delta = style.tracking_delta;
        split.Add(mid);
        cursor = b;

        if(old.to > range.to) {
            UiDocStyleRun right = old;
            right.from = range.to;
            split.Add(right);
        }
    }

    if(styles_.IsEmpty() || cursor < range.to) {
        UiDocStyleRun mid;
        mid.from = cursor;
        mid.to = range.to;
        if(mask & STYLE_FLAGS)          mid.flags = style.flags;
        if(mask & STYLE_INK)            mid.ink = style.ink;
        if(mask & STYLE_FONT_FACE)      mid.font_face = style.font_face;
        if(mask & STYLE_FONT_HEIGHT)    mid.font_height = style.font_height;
        if(mask & STYLE_SIZE_DELTA)     mid.size_delta = style.size_delta;
        if(mask & STYLE_LEADING_DELTA)  mid.leading_delta = style.leading_delta;
        if(mask & STYLE_TRACKING_DELTA) mid.tracking_delta = style.tracking_delta;
        split.Add(mid);
    }

    styles_ = pick(split);
    NormalizeStyles();
}

void UiDocCore::MapRanges(const UiDocPositionMap& map)
{
    for(UiDocStyleRun& s : styles_) {
        s.from = map.Map(s.from, UiDocPositionMap::Left);
        s.to = map.Map(s.to, UiDocPositionMap::Right);
    }
    NormalizeStyles();

    for(UiDocAnnotation& a : annotations_) {
        a.range.from = map.Map(a.range.from, UiDocPositionMap::Left);
        a.range.to = map.Map(a.range.to, UiDocPositionMap::Right);
    }
    for(UiDocEmbedBlock& e : embeds_) {
        e.range.from = map.Map(e.range.from, UiDocPositionMap::Left);
        e.range.to = map.Map(e.range.to, UiDocPositionMap::Right);
    }
    for(int i = 0; i < anchors_.GetCount(); i++)
        anchors_[i] = map.Map(anchors_[i], UiDocPositionMap::Right);
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
        if(embeds_[i].embed_id == id)
            return i;
    return -1;
}

bool UiDocCore::ApplyOne(const UiDocCoreChange& c, UiDocApplyResult& result, HistoryStep* h)
{
    switch(c.type) {
    case UiDocCoreChange::ReplaceText: {
        UiDocRange r = NormalizeRange(c.range);
        int old_len = r.to - r.from;
        if(h) {
            h->kind = HistoryStep::Text;
            h->at = r.from;
            h->before_text = text_.Mid(r.from, old_len);
            h->after_text = c.text;
            h->before_styles <<= styles_;
            h->before_annotations <<= annotations_;
            h->before_embeds <<= embeds_;
            h->before_anchors <<= anchors_;
        }

        text_.Remove(r.from, old_len);
        text_.Insert(r.from, c.text);
        UiDocPositionMap local_map;
        UiDocPositionMapEntry& e = local_map.edits.Add();
        e.at = r.from;
        e.old_len = old_len;
        e.new_len = c.text.GetCount();
        MapRanges(local_map);
        result.positions.edits.Append(local_map.edits);

        if(h) {
            h->after_styles <<= styles_;
            h->after_annotations <<= annotations_;
            h->after_embeds <<= embeds_;
            h->after_anchors <<= anchors_;
        }

        UiDocRange changed(r.from, r.from + max(old_len, c.text.GetCount()));
        if(result.changed_range.IsEmpty())
            result.changed_range = changed;
        else {
            result.changed_range.from = min(result.changed_range.from, changed.from);
            result.changed_range.to = max(result.changed_range.to, changed.to);
        }
        return true;
    }

    case UiDocCoreChange::SetStyle:
        if(h) {
            h->kind = HistoryStep::Styles;
            h->before_styles <<= styles_;
        }
        ReplaceStyleRange(c.range, c.style, c.style_mask ? c.style_mask : STYLE_ALL);
        if(h)
            h->after_styles <<= styles_;
        return true;

    case UiDocCoreChange::AddAnnotation:
    case UiDocCoreChange::RemoveAnnotation:
    case UiDocCoreChange::UpdateAnnotation:
    case UiDocCoreChange::SetAnnotationFlags: {
        if(h) {
            h->kind = HistoryStep::Annotations;
            h->before_annotations <<= annotations_;
        }
        bool ok = true;
        if(c.type == UiDocCoreChange::AddAnnotation) {
            UiDocAnnotation a = c.annotation;
            a.range = NormalizeRange(a.range);
            if(a.id.IsEmpty())
                a.id = Format("ann_%d", next_annotation_id_++);
            if(FindAnnotation(a.id) >= 0)
                ok = false;
            else
                annotations_.Add(pick(a));
        }
        else {
            int q = FindAnnotation(c.annotation_id);
            if(q < 0)
                ok = false;
            else if(c.type == UiDocCoreChange::RemoveAnnotation)
                annotations_.Remove(q);
            else if(c.type == UiDocCoreChange::UpdateAnnotation)
                UiDocMergeMap(annotations_[q].payload, c.values);
            else {
                if(c.flag_mask & ANNOT_EXPANDED)  annotations_[q].expanded = c.expanded;
                if(c.flag_mask & ANNOT_PRINTABLE) annotations_[q].printable = c.printable;
                if(c.flag_mask & ANNOT_RESOLVED)  annotations_[q].resolved = c.resolved;
            }
        }
        if(ok && h)
            h->after_annotations <<= annotations_;
        return ok;
    }

    case UiDocCoreChange::AddResource:
    case UiDocCoreChange::RemoveResource: {
        if(h) {
            h->kind = HistoryStep::Resources;
            h->before_resources <<= resources_;
        }
        bool ok = true;
        if(c.type == UiDocCoreChange::AddResource) {
            UiDocResource r = c.resource;
            if(r.key.IsEmpty())
                r.key = Format("res_%d", next_resource_id_++);
            if(FindResource(r.key) >= 0)
                ok = false;
            else
                resources_.Add(pick(r));
        }
        else {
            int q = FindResource(c.resource_key);
            if(q < 0)
                ok = false;
            else {
                for(const UiDocEmbedBlock& e : embeds_)
                    if(e.payload.Find("resource_key") >= 0 && AsString(e.payload["resource_key"]) == c.resource_key)
                        ok = false;
                if(ok)
                    resources_.Remove(q);
            }
        }
        if(ok && h)
            h->after_resources <<= resources_;
        return ok;
    }

    case UiDocCoreChange::AddEmbed:
    case UiDocCoreChange::RemoveEmbed:
    case UiDocCoreChange::UpdateEmbed: {
        if(h) {
            h->kind = HistoryStep::Embeds;
            h->before_embeds <<= embeds_;
        }
        bool ok = true;
        if(c.type == UiDocCoreChange::AddEmbed) {
            UiDocEmbedBlock e = c.embed;
            e.range = NormalizeRange(e.range);
            if(e.embed_id.IsEmpty())
                e.embed_id = Format("embed_%d", next_embed_id_++);
            if(FindEmbed(e.embed_id) >= 0)
                ok = false;
            else
                embeds_.Add(pick(e));
        }
        else {
            int q = FindEmbed(c.embed_id);
            if(q < 0)
                ok = false;
            else if(c.type == UiDocCoreChange::RemoveEmbed)
                embeds_.Remove(q);
            else
                UiDocMergeMap(embeds_[q].payload, c.values);
        }
        if(ok && h)
            h->after_embeds <<= embeds_;
        return ok;
    }

    case UiDocCoreChange::SetDocumentMeta:
        if(c.key.IsEmpty())
            return false;
        if(h) {
            h->kind = HistoryStep::Meta;
            h->before_meta = clone(meta_);
        }
        if(IsNull(c.value)) {
            int q = meta_.Find(c.key);
            if(q >= 0)
                meta_.Remove(q);
        }
        else
            meta_.GetAdd(c.key) = c.value;
        if(h)
            h->after_meta = clone(meta_);
        return true;

    case UiDocCoreChange::SetAnchor:
    case UiDocCoreChange::RemoveAnchor:
        if(h) {
            h->kind = HistoryStep::Anchors;
            h->before_anchors <<= anchors_;
        }
        if(c.type == UiDocCoreChange::SetAnchor) {
            if(c.key.IsEmpty())
                return false;
            anchors_.GetAdd(c.key) = ClampPos(c.pos);
        }
        else {
            int q = anchors_.Find(c.key);
            if(q < 0)
                return false;
            anchors_.Remove(q);
        }
        if(h)
            h->after_anchors <<= anchors_;
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
    for(const UiDocCoreChange& c : tx.changes) {
        HistoryStep* step = tx.add_to_history && history_limit_ > 0 ? &record.steps.Add() : nullptr;
        if(!ApplyOne(c, result, step)) {
            if(step)
                record.steps.Drop();
            ApplyHistoryRecord(record, true);
            result.positions.Clear();
            result.error = "transaction refused";
            return result;
        }
    }

    if(tx.add_to_history && history_limit_ > 0 && !record.steps.IsEmpty()) {
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
    UiDocCoreChange c;
    c.type = UiDocCoreChange::ReplaceText;
    c.range = range;
    c.text = text;
    UiDocCoreTransaction tx;
    tx.base_revision = base_revision;
    tx.changes.Add(pick(c));
    return Apply(tx);
}

UiDocApplyResult UiDocCore::SetStyle(UiDocRange range, const UiDocStyleRun& style, dword mask, uint64 base_revision)
{
    UiDocCoreChange c;
    c.type = UiDocCoreChange::SetStyle;
    c.range = range;
    c.style = style;
    c.style_mask = mask;
    UiDocCoreTransaction tx;
    tx.base_revision = base_revision;
    tx.changes.Add(pick(c));
    return Apply(tx);
}

String UiDocCore::AddAnnotation(UiDocRange range, const String& type, const ValueMap& payload, const ValueMap& meta)
{
    UiDocCoreChange c;
    c.type = UiDocCoreChange::AddAnnotation;
    c.annotation.id = Format("ann_%d", next_annotation_id_++);
    c.annotation.range = range;
    c.annotation.type = type;
    c.annotation.payload = clone(payload);
    c.annotation.meta = clone(meta);
    String id = c.annotation.id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
    return Apply(tx).ok ? id : String();
}

bool UiDocCore::RemoveAnnotation(const String& id)
{
    UiDocCoreChange c;
    c.type = UiDocCoreChange::RemoveAnnotation;
    c.annotation_id = id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
    return Apply(tx).ok;
}

bool UiDocCore::UpdateAnnotation(const String& id, const ValueMap& values)
{
    UiDocCoreChange c;
    c.type = UiDocCoreChange::UpdateAnnotation;
    c.annotation_id = id;
    c.values = clone(values);
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
    return Apply(tx).ok;
}

bool UiDocCore::SetAnnotationFlags(const String& id, dword mask, bool expanded, bool printable, bool resolved)
{
    UiDocCoreChange c;
    c.type = UiDocCoreChange::SetAnnotationFlags;
    c.annotation_id = id;
    c.flag_mask = mask;
    c.expanded = expanded;
    c.printable = printable;
    c.resolved = resolved;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
    return Apply(tx).ok;
}

Vector<UiDocAnnotation> UiDocCore::QueryAnnotations(const UiDocRange* range, const String& type) const
{
    Vector<UiDocAnnotation> out;
    UiDocRange rr;
    if(range)
        rr = NormalizeRange(*range);
    for(const UiDocAnnotation& a : annotations_) {
        if(!type.IsEmpty() && a.type != type)
            continue;
        if(range && !UiDocRangesTouch(a.range, rr))
            continue;
        out.Add(a);
    }
    return out;
}

String UiDocCore::AddResource(const UiDocResource& resource, bool dedupe)
{
    if(dedupe && !resource.content_hash.IsEmpty())
        for(const UiDocResource& r : resources_)
            if(r.content_hash == resource.content_hash && r.resource_type == resource.resource_type)
                return r.key;

    UiDocCoreChange c;
    c.type = UiDocCoreChange::AddResource;
    c.resource = resource;
    if(c.resource.key.IsEmpty())
        c.resource.key = Format("res_%d", next_resource_id_++);
    String key = c.resource.key;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
    return Apply(tx).ok ? key : String();
}

bool UiDocCore::RemoveResource(const String& key)
{
    UiDocCoreChange c;
    c.type = UiDocCoreChange::RemoveResource;
    c.resource_key = key;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
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

String UiDocCore::AddEmbed(int pos, const String& type, const ValueMap& payload, const ValueMap& layout, const ValueMap& meta)
{
    UiDocCoreChange c;
    c.type = UiDocCoreChange::AddEmbed;
    c.embed.embed_id = Format("embed_%d", next_embed_id_++);
    c.embed.embed_type = type;
    c.embed.range = UiDocRange(ClampPos(pos), ClampPos(pos));
    c.embed.payload = clone(payload);
    c.embed.layout_hints = clone(layout);
    c.embed.meta = clone(meta);
    String id = c.embed.embed_id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
    return Apply(tx).ok ? id : String();
}

bool UiDocCore::RemoveEmbed(const String& id)
{
    UiDocCoreChange c;
    c.type = UiDocCoreChange::RemoveEmbed;
    c.embed_id = id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
    return Apply(tx).ok;
}

bool UiDocCore::UpdateEmbed(const String& id, const ValueMap& values)
{
    UiDocCoreChange c;
    c.type = UiDocCoreChange::UpdateEmbed;
    c.embed_id = id;
    c.values = clone(values);
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
    return Apply(tx).ok;
}

Vector<UiDocEmbedBlock> UiDocCore::QueryEmbeds(const UiDocRange* range, const String& type) const
{
    Vector<UiDocEmbedBlock> out;
    UiDocRange rr;
    if(range)
        rr = NormalizeRange(*range);
    for(const UiDocEmbedBlock& e : embeds_) {
        if(!type.IsEmpty() && e.embed_type != type)
            continue;
        if(range && !UiDocRangesTouch(e.range, rr))
            continue;
        out.Add(e);
    }
    return out;
}

bool UiDocCore::SetAnchor(const String& id, int pos)
{
    if(id.IsEmpty())
        return false;
    UiDocCoreChange c;
    c.type = UiDocCoreChange::SetAnchor;
    c.key = id;
    c.pos = pos;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
    return Apply(tx).ok;
}

bool UiDocCore::RemoveAnchor(const String& id)
{
    UiDocCoreChange c;
    c.type = UiDocCoreChange::RemoveAnchor;
    c.key = id;
    UiDocCoreTransaction tx;
    tx.changes.Add(pick(c));
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
    redo_.Add(pick(record));
    Touch();
    UiDocApplyResult r;
    r.ok = true;
    r.revision_before = before;
    r.revision_after = revision_;
    WhenChange(r);
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
    undo_.Add(pick(record));
    Touch();
    UiDocApplyResult r;
    r.ok = true;
    r.revision_before = before;
    r.revision_after = revision_;
    WhenChange(r);
    return true;
}

}
