#include "UiDocCore.h"

namespace Upp {

static String UiDocPackMap(const ValueMap& map)
{
    return Base64Encode(StoreAsString(map));
}

static ValueMap UiDocUnpackMap(const Value& value)
{
    ValueMap out;
    String data = AsString(value);
    if(!data.IsEmpty())
        LoadFromString(out, Base64Decode(data));
    return out;
}

static Value UiDocColorValue(Color c)
{
    if(IsNull(c))
        return Value();
    ValueArray rgb;
    rgb.Add(c.GetR());
    rgb.Add(c.GetG());
    rgb.Add(c.GetB());
    return rgb;
}

static Color UiDocColorFromValue(const Value& value)
{
    if(!value.Is<ValueArray>())
        return Null;
    ValueArray rgb = value;
    if(rgb.GetCount() != 3)
        return Null;
    return Color(clamp((int)rgb[0], 0, 255),
                 clamp((int)rgb[1], 0, 255),
                 clamp((int)rgb[2], 0, 255));
}

static void UiDocCollectResourceKeys(const Value& value, Index<String>& out)
{
    if(value.Is<ValueMap>()) {
        ValueMap map = value;
        for(int i = 0; i < map.GetCount(); i++) {
            String name = AsString(map.GetKey(i));
            Value child = map.GetValue(i);
            if(name == "resource_key") {
                String key = AsString(child);
                if(!key.IsEmpty())
                    out.FindAdd(key);
            }
            UiDocCollectResourceKeys(child, out);
        }
    }
    else if(value.Is<ValueArray>()) {
        ValueArray array = value;
        for(int i = 0; i < array.GetCount(); i++)
            UiDocCollectResourceKeys(array[i], out);
    }
}

void UiDocCore::RebuildIds()
{
    next_block_id_ = 1;
    while(FindBlock(Format("block_%d", next_block_id_)) >= 0)
        ++next_block_id_;

    next_annotation_id_ = 1;
    while(FindAnnotation(Format("ann_%d", next_annotation_id_)) >= 0)
        ++next_annotation_id_;

    next_resource_id_ = 1;
    while(FindResource(Format("res_%d", next_resource_id_)) >= 0)
        ++next_resource_id_;

    next_embed_id_ = 1;
    while(FindEmbed(Format("embed_%d", next_embed_id_)) >= 0)
        ++next_embed_id_;
}

bool UiDocCore::Validate(String& error) const
{
    Index<String> ids;
    int last_style_to = 0;
    for(const UiDocStyleRun& r : styles_) {
        if(r.from < 0 || r.to < r.from || r.to > text_.GetCount()) {
            error = "style range outside document";
            return false;
        }
        if(r.from < last_style_to) {
            error = "style runs overlap";
            return false;
        }
        last_style_to = r.to;
    }

    ids.Clear();
    for(const UiDocBlock& b : blocks_) {
        if(b.id.IsEmpty() || b.range.from < 0 || b.range.to < b.range.from || b.range.to > text_.GetCount()) {
            error = "invalid block";
            return false;
        }
        if(ids.Find(b.id) >= 0) { error = "duplicate block id"; return false; }
        ids.Add(b.id);
    }

    ids.Clear();
    for(const UiDocAnnotation& a : annotations_) {
        if(a.id.IsEmpty() || a.type.IsEmpty() || a.range.from < 0 || a.range.to < a.range.from || a.range.to > text_.GetCount()) {
            error = "invalid annotation";
            return false;
        }
        if(ids.Find(a.id) >= 0) { error = "duplicate annotation id"; return false; }
        ids.Add(a.id);
    }

    ids.Clear();
    for(const UiDocResource& r : resources_) {
        if(r.key.IsEmpty() || r.resource_type.IsEmpty()) { error = "invalid resource"; return false; }
        if(ids.Find(r.key) >= 0) { error = "duplicate resource key"; return false; }
        ids.Add(r.key);
    }

    ids.Clear();
    for(const UiDocEmbedBlock& e : embeds_) {
        if(e.embed_id.IsEmpty() || e.embed_type.IsEmpty() || e.range.from < 0 || e.range.to < e.range.from || e.range.to > text_.GetCount()) {
            error = "invalid embed";
            return false;
        }
        if(ids.Find(e.embed_id) >= 0) { error = "duplicate embed id"; return false; }
        if(!e.block_id.IsEmpty() && FindBlock(e.block_id) < 0) { error = "embed references missing block"; return false; }
        ids.Add(e.embed_id);
    }

    Index<String> referenced_resources;
    for(const UiDocEmbedBlock& e : embeds_)
        UiDocCollectResourceKeys(e.payload, referenced_resources);
    for(int i = 0; i < referenced_resources.GetCount(); i++) {
        if(FindResource(referenced_resources[i]) < 0) {
            error = "embed references missing resource";
            return false;
        }
    }

    for(int i = 0; i < anchors_.GetCount(); i++) {
        int pos = anchors_[i];
        if(pos < 0 || pos > text_.GetCount()) {
            error = "anchor outside document";
            return false;
        }
    }
    return true;
}

String UiDocCore::ToJson(bool pretty) const
{
    ValueMap root;
    root.Set("format", "UiDoc");
    root.Set("version", 2);
    root.Set("text", text_.ToString());
    root.Set("meta", UiDocPackMap(meta_));

    ValueArray styles;
    for(const UiDocStyleRun& r : styles_) {
        ValueMap m;
        m.Set("from", r.from);
        m.Set("to", r.to);
        m.Set("flags", (int)r.flags);
        m.Set("ink", UiDocColorValue(r.ink));
        m.Set("font", r.font_face);
        m.Set("height", r.font_height);
        m.Set("size_delta", r.size_delta);
        m.Set("leading_delta", r.leading_delta);
        m.Set("tracking_delta", r.tracking_delta);
        styles.Add(m);
    }
    root.Set("styles", styles);

    ValueArray blocks;
    for(const UiDocBlock& b : blocks_) {
        ValueMap m;
        m.Set("id", b.id);
        m.Set("from", b.range.from);
        m.Set("to", b.range.to);
        m.Set("role", b.role);
        m.Set("indent", b.indent);
        m.Set("meta", UiDocPackMap(b.meta));
        blocks.Add(m);
    }
    root.Set("blocks", blocks);

    ValueArray annotations;
    for(const UiDocAnnotation& a : annotations_) {
        ValueMap m;
        m.Set("id", a.id);
        m.Set("from", a.range.from);
        m.Set("to", a.range.to);
        m.Set("type", a.type);
        m.Set("payload", UiDocPackMap(a.payload));
        m.Set("meta", UiDocPackMap(a.meta));
        m.Set("expanded", a.expanded);
        m.Set("printable", a.printable);
        m.Set("resolved", a.resolved);
        annotations.Add(m);
    }
    root.Set("annotations", annotations);

    ValueArray resources;
    for(const UiDocResource& r : resources_) {
        ValueMap m;
        m.Set("key", r.key);
        m.Set("type", r.resource_type);
        m.Set("hash", r.content_hash);
        m.Set("bytes", Base64Encode(r.bytes));
        m.Set("mime", r.mime);
        m.Set("name", r.original_name);
        m.Set("width", r.width);
        m.Set("height", r.height);
        m.Set("meta", UiDocPackMap(r.meta));
        resources.Add(m);
    }
    root.Set("resources", resources);

    ValueArray embeds;
    for(const UiDocEmbedBlock& e : embeds_) {
        ValueMap m;
        m.Set("block_id", e.block_id);
        m.Set("id", e.embed_id);
        m.Set("type", e.embed_type);
        m.Set("from", e.range.from);
        m.Set("to", e.range.to);
        m.Set("payload", UiDocPackMap(e.payload));
        m.Set("layout", UiDocPackMap(e.layout_hints));
        m.Set("meta", UiDocPackMap(e.meta));
        embeds.Add(m);
    }
    root.Set("embeds", embeds);

    ValueArray anchors;
    for(int i = 0; i < anchors_.GetCount(); i++) {
        ValueMap m;
        m.Set("id", anchors_.GetKey(i));
        m.Set("pos", anchors_[i]);
        anchors.Add(m);
    }
    root.Set("anchors", anchors);
    return AsJSON(root, pretty);
}

bool UiDocCore::FromJson(const String& data, String* out_error)
{
    auto Fail = [&](const String& error) {
        if(out_error)
            *out_error = error;
        return false;
    };

    Value value = ParseJSON(data);
    if(value.IsError() || !IsValueMap(value))
        return Fail("invalid UiDoc JSON");
    ValueMap root = value;
    if(AsString(root["format"]) != "UiDoc")
        return Fail("not a UiDoc document");
    if((int)root["version"] != 2)
        return Fail("unsupported UiDoc version");

    UiDocCore next;
    next.history_limit_ = history_limit_;
    next.text_ = AsString(root["text"]).ToWString();
    next.meta_ = UiDocUnpackMap(root["meta"]);

    if(root.Find("styles") >= 0 && root["styles"].Is<ValueArray>()) {
        ValueArray aa = root["styles"];
        for(int i = 0; i < aa.GetCount(); i++) {
            if(!aa[i].Is<ValueMap>()) return Fail("invalid style entry");
            ValueMap m = aa[i];
            UiDocStyleRun r;
            r.from = (int)m["from"];
            r.to = (int)m["to"];
            r.flags = (byte)(int)m["flags"];
            r.ink = UiDocColorFromValue(m["ink"]);
            r.font_face = AsString(m["font"]);
            r.font_height = (int)m["height"];
            r.size_delta = (int)m["size_delta"];
            r.leading_delta = (int)m["leading_delta"];
            r.tracking_delta = (int)m["tracking_delta"];
            next.styles_.Add(pick(r));
        }
    }

    if(root.Find("blocks") >= 0 && root["blocks"].Is<ValueArray>()) {
        ValueArray aa = root["blocks"];
        for(int i = 0; i < aa.GetCount(); i++) {
            if(!aa[i].Is<ValueMap>()) return Fail("invalid block entry");
            ValueMap m = aa[i];
            UiDocBlock b;
            b.id = AsString(m["id"]);
            b.range = UiDocRange((int)m["from"], (int)m["to"]);
            b.role = AsString(m["role"]);
            b.indent = max(0, (int)m["indent"]);
            b.meta = UiDocUnpackMap(m["meta"]);
            next.blocks_.Add(pick(b));
        }
    }

    if(root.Find("annotations") >= 0 && root["annotations"].Is<ValueArray>()) {
        ValueArray aa = root["annotations"];
        for(int i = 0; i < aa.GetCount(); i++) {
            if(!aa[i].Is<ValueMap>()) return Fail("invalid annotation entry");
            ValueMap m = aa[i];
            UiDocAnnotation a;
            a.id = AsString(m["id"]);
            a.range = UiDocRange((int)m["from"], (int)m["to"]);
            a.type = AsString(m["type"]);
            a.payload = UiDocUnpackMap(m["payload"]);
            a.meta = UiDocUnpackMap(m["meta"]);
            a.expanded = (bool)m["expanded"];
            a.printable = (bool)m["printable"];
            a.resolved = (bool)m["resolved"];
            next.annotations_.Add(pick(a));
        }
    }

    if(root.Find("resources") >= 0 && root["resources"].Is<ValueArray>()) {
        ValueArray aa = root["resources"];
        for(int i = 0; i < aa.GetCount(); i++) {
            if(!aa[i].Is<ValueMap>()) return Fail("invalid resource entry");
            ValueMap m = aa[i];
            UiDocResource r;
            r.key = AsString(m["key"]);
            r.resource_type = AsString(m["type"]);
            r.content_hash = AsString(m["hash"]);
            r.bytes = Base64Decode(AsString(m["bytes"]));
            r.mime = AsString(m["mime"]);
            r.original_name = AsString(m["name"]);
            r.width = max(0, (int)m["width"]);
            r.height = max(0, (int)m["height"]);
            r.meta = UiDocUnpackMap(m["meta"]);
            next.resources_.Add(pick(r));
        }
    }

    if(root.Find("embeds") >= 0 && root["embeds"].Is<ValueArray>()) {
        ValueArray aa = root["embeds"];
        for(int i = 0; i < aa.GetCount(); i++) {
            if(!aa[i].Is<ValueMap>()) return Fail("invalid embed entry");
            ValueMap m = aa[i];
            UiDocEmbedBlock e;
            e.block_id = AsString(m["block_id"]);
            e.embed_id = AsString(m["id"]);
            e.embed_type = AsString(m["type"]);
            e.range = UiDocRange((int)m["from"], (int)m["to"]);
            e.payload = UiDocUnpackMap(m["payload"]);
            e.layout_hints = UiDocUnpackMap(m["layout"]);
            e.meta = UiDocUnpackMap(m["meta"]);
            next.embeds_.Add(pick(e));
        }
    }

    if(root.Find("anchors") >= 0 && root["anchors"].Is<ValueArray>()) {
        ValueArray aa = root["anchors"];
        for(int i = 0; i < aa.GetCount(); i++) {
            if(!aa[i].Is<ValueMap>()) return Fail("invalid anchor entry");
            ValueMap m = aa[i];
            String id = AsString(m["id"]);
            if(id.IsEmpty() || next.anchors_.Find(id) >= 0)
                return Fail("invalid anchor id");
            next.anchors_.Add(id, (int)m["pos"]);
        }
    }

    String error;
    if(!next.Validate(error))
        return Fail(error);
    next.NormalizeStyles();
    next.RebuildIds();

    uint64 before = revision_;
    text_ = pick(next.text_);
    styles_ = pick(next.styles_);
    blocks_ = pick(next.blocks_);
    annotations_ = pick(next.annotations_);
    resources_ = pick(next.resources_);
    embeds_ = pick(next.embeds_);
    anchors_ = pick(next.anchors_);
    meta_ = pick(next.meta_);
    next_block_id_ = next.next_block_id_;
    next_annotation_id_ = next.next_annotation_id_;
    next_resource_id_ = next.next_resource_id_;
    next_embed_id_ = next.next_embed_id_;
    undo_.Clear();
    redo_.Clear();
    Touch();

    UiDocApplyResult result;
    result.ok = true;
    result.revision_before = before;
    result.revision_after = revision_;
    WhenChange(result);
    if(out_error)
        out_error->Clear();
    return true;
}

}
