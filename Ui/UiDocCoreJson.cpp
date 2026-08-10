#include "UiDocCore.h"

namespace Upp {

namespace {

const int UIDOC_JSON_VERSION = 2;

String PackMap(const ValueMap& map)
{
    ValueMap copy = clone(map);
    return Base64Encode(StoreAsString(copy));
}

bool UnpackMap(const Value& value, ValueMap& map)
{
    map.Clear();
    if(IsNull(value))
        return true;
    return LoadFromString(map, Base64Decode(AsString(value)));
}

ValueMap RangeToValue(const UiDocRange& range)
{
    ValueMap out;
    out.Add("from", range.from);
    out.Add("to", range.to);
    return out;
}

bool RangeFromValue(const Value& value, UiDocRange& range)
{
    if(!value.Is<ValueMap>())
        return false;
    ValueMap map = value;
    if(map.Find("from") < 0 || map.Find("to") < 0)
        return false;
    range.from = (int)map["from"];
    range.to = (int)map["to"];
    return true;
}

ValueMap StyleRunToValue(const UiDocStyleRun& run)
{
    ValueMap out;
    out.Add("from", run.from);
    out.Add("to", run.to);
    out.Add("flags", (int)run.style.flags);
    if(!IsNull(run.style.ink)) {
        out.Add("ink_r", run.style.ink.GetR());
        out.Add("ink_g", run.style.ink.GetG());
        out.Add("ink_b", run.style.ink.GetB());
    }
    if(!run.style.font_face.IsEmpty())
        out.Add("font_face", run.style.font_face);
    if(run.style.font_height)
        out.Add("font_height", run.style.font_height);
    if(run.style.size_delta)
        out.Add("size_delta", run.style.size_delta);
    if(run.style.leading_delta)
        out.Add("leading_delta", run.style.leading_delta);
    if(run.style.tracking_delta)
        out.Add("tracking_delta", run.style.tracking_delta);
    return out;
}

bool StyleRunFromValue(const Value& value, UiDocStyleRun& run)
{
    if(!value.Is<ValueMap>())
        return false;
    ValueMap map = value;
    if(map.Find("from") < 0 || map.Find("to") < 0)
        return false;
    run = UiDocStyleRun();
    run.from = (int)map["from"];
    run.to = (int)map["to"];
    if(map.Find("flags") >= 0)
        run.style.flags = (byte)(int)map["flags"];
    bool has_r = map.Find("ink_r") >= 0;
    bool has_g = map.Find("ink_g") >= 0;
    bool has_b = map.Find("ink_b") >= 0;
    if(has_r || has_g || has_b) {
        if(!(has_r && has_g && has_b))
            return false;
        run.style.ink = Color(clamp((int)map["ink_r"], 0, 255),
                              clamp((int)map["ink_g"], 0, 255),
                              clamp((int)map["ink_b"], 0, 255));
    }
    if(map.Find("font_face") >= 0)      run.style.font_face = AsString(map["font_face"]);
    if(map.Find("font_height") >= 0)    run.style.font_height = max(0, (int)map["font_height"]);
    if(map.Find("size_delta") >= 0)     run.style.size_delta = (int)map["size_delta"];
    if(map.Find("leading_delta") >= 0)  run.style.leading_delta = (int)map["leading_delta"];
    if(map.Find("tracking_delta") >= 0) run.style.tracking_delta = (int)map["tracking_delta"];
    return true;
}

ValueMap BlockToValue(const UiDocBlock& block)
{
    ValueMap out;
    out.Add("id", block.id);
    out.Add("range", RangeToValue(block.range));
    out.Add("role", block.role);
    if(block.indent)
        out.Add("indent", block.indent);
    if(block.meta.GetCount())
        out.Add("meta", PackMap(block.meta));
    return out;
}

bool BlockFromValue(const Value& value, UiDocBlock& block)
{
    if(!value.Is<ValueMap>())
        return false;
    ValueMap map = value;
    if(map.Find("id") < 0 || map.Find("range") < 0)
        return false;
    block = UiDocBlock();
    block.id = AsString(map["id"]);
    if(!RangeFromValue(map["range"], block.range))
        return false;
    if(map.Find("role") >= 0)
        block.role = AsString(map["role"]);
    if(map.Find("indent") >= 0)
        block.indent = max(0, (int)map["indent"]);
    if(map.Find("meta") >= 0 && !UnpackMap(map["meta"], block.meta))
        return false;
    return !block.id.IsEmpty();
}

ValueMap AnnotationToValue(const UiDocAnnotation& annotation)
{
    ValueMap out;
    out.Add("id", annotation.id);
    out.Add("range", RangeToValue(annotation.range));
    out.Add("type", annotation.type);
    out.Add("payload", PackMap(annotation.payload));
    if(annotation.meta.GetCount())
        out.Add("meta", PackMap(annotation.meta));
    out.Add("expanded", annotation.expanded);
    out.Add("printable", annotation.printable);
    out.Add("resolved", annotation.resolved);
    return out;
}

bool AnnotationFromValue(const Value& value, UiDocAnnotation& annotation)
{
    if(!value.Is<ValueMap>())
        return false;
    ValueMap map = value;
    if(map.Find("id") < 0 || map.Find("range") < 0 || map.Find("type") < 0)
        return false;
    annotation = UiDocAnnotation();
    annotation.id = AsString(map["id"]);
    annotation.type = AsString(map["type"]);
    if(!RangeFromValue(map["range"], annotation.range))
        return false;
    if(map.Find("payload") >= 0 && !UnpackMap(map["payload"], annotation.payload))
        return false;
    if(map.Find("meta") >= 0 && !UnpackMap(map["meta"], annotation.meta))
        return false;
    if(map.Find("expanded") >= 0)  annotation.expanded = (bool)map["expanded"];
    if(map.Find("printable") >= 0) annotation.printable = (bool)map["printable"];
    if(map.Find("resolved") >= 0)  annotation.resolved = (bool)map["resolved"];
    return !annotation.id.IsEmpty() && !annotation.type.IsEmpty();
}

ValueMap ResourceToValue(const UiDocResource& resource)
{
    ValueMap out;
    out.Add("key", resource.key);
    out.Add("type", resource.resource_type);
    out.Add("hash", resource.content_hash);
    out.Add("bytes", Base64Encode(resource.bytes));
    out.Add("mime", resource.mime);
    out.Add("name", resource.original_name);
    if(resource.width > 0)  out.Add("width", resource.width);
    if(resource.height > 0) out.Add("height", resource.height);
    if(resource.meta.GetCount())
        out.Add("meta", PackMap(resource.meta));
    return out;
}

bool ResourceFromValue(const Value& value, UiDocResource& resource)
{
    if(!value.Is<ValueMap>())
        return false;
    ValueMap map = value;
    if(map.Find("key") < 0 || map.Find("type") < 0 || map.Find("bytes") < 0)
        return false;
    resource = UiDocResource();
    resource.key = AsString(map["key"]);
    resource.resource_type = AsString(map["type"]);
    if(map.Find("hash") >= 0) resource.content_hash = AsString(map["hash"]);
    resource.bytes = Base64Decode(AsString(map["bytes"]));
    if(map.Find("mime") >= 0) resource.mime = AsString(map["mime"]);
    if(map.Find("name") >= 0) resource.original_name = AsString(map["name"]);
    if(map.Find("width") >= 0) resource.width = max(0, (int)map["width"]);
    if(map.Find("height") >= 0) resource.height = max(0, (int)map["height"]);
    if(map.Find("meta") >= 0 && !UnpackMap(map["meta"], resource.meta))
        return false;
    return !resource.key.IsEmpty() && !resource.resource_type.IsEmpty();
}

ValueMap EmbedToValue(const UiDocEmbedBlock& embed)
{
    ValueMap out;
    out.Add("id", embed.id);
    out.Add("type", embed.type);
    out.Add("range", RangeToValue(embed.range));
    out.Add("payload", PackMap(embed.payload));
    if(embed.layout.GetCount()) out.Add("layout", PackMap(embed.layout));
    if(embed.meta.GetCount()) out.Add("meta", PackMap(embed.meta));
    return out;
}

bool EmbedFromValue(const Value& value, UiDocEmbedBlock& embed)
{
    if(!value.Is<ValueMap>())
        return false;
    ValueMap map = value;
    if(map.Find("id") < 0 || map.Find("type") < 0 || map.Find("range") < 0)
        return false;
    embed = UiDocEmbedBlock();
    embed.id = AsString(map["id"]);
    embed.type = AsString(map["type"]);
    if(!RangeFromValue(map["range"], embed.range))
        return false;
    if(map.Find("payload") >= 0 && !UnpackMap(map["payload"], embed.payload))
        return false;
    if(map.Find("layout") >= 0 && !UnpackMap(map["layout"], embed.layout))
        return false;
    if(map.Find("meta") >= 0 && !UnpackMap(map["meta"], embed.meta))
        return false;
    return !embed.id.IsEmpty() && !embed.type.IsEmpty();
}

bool ReadArray(const ValueMap& root, const char *key, ValueArray& out)
{
    int q = root.Find(key);
    if(q < 0) {
        out.Clear();
        return true;
    }
    if(!root.GetValue(q).Is<ValueArray>())
        return false;
    out = root.GetValue(q);
    return true;
}

} // namespace

String UiDocCore::ToJson(bool pretty) const
{
    ValueMap root;
    root.Add("kind", "UiDoc");
    root.Add("version", UIDOC_JSON_VERSION);
    root.Add("text", ToUtf8(text_));
    if(meta_.GetCount())
        root.Add("meta", PackMap(meta_));

    ValueArray styles;
    for(const UiDocStyleRun& run : styles_)
        styles.Add(StyleRunToValue(run));
    root.Add("styles", styles);

    ValueArray blocks;
    for(const UiDocBlock& block : blocks_)
        blocks.Add(BlockToValue(block));
    root.Add("blocks", blocks);

    ValueArray annotations;
    for(const UiDocAnnotation& annotation : annotations_)
        annotations.Add(AnnotationToValue(annotation));
    root.Add("annotations", annotations);

    ValueArray resources;
    for(const UiDocResource& resource : resources_)
        resources.Add(ResourceToValue(resource));
    root.Add("resources", resources);

    ValueArray embeds;
    for(const UiDocEmbedBlock& embed : embeds_)
        embeds.Add(EmbedToValue(embed));
    root.Add("embeds", embeds);

    ValueArray anchors;
    for(int i = 0; i < anchors_.GetCount(); i++) {
        ValueMap anchor;
        anchor.Add("id", AsString(anchors_.GetKey(i)));
        anchor.Add("pos", anchors_[i]);
        anchors.Add(anchor);
    }
    root.Add("anchors", anchors);

    return AsJSON(root, pretty);
}

bool UiDocCore::FromJson(const String& data, String* error)
{
    auto Fail = [&](const String& text) {
        if(error)
            *error = text;
        return false;
    };

    Value root_value = ParseJSON(data);
    if(root_value.IsError() || !IsValueMap(root_value))
        return Fail("invalid UiDoc JSON");
    ValueMap root = root_value;
    if(root.Find("kind") < 0 || AsString(root["kind"]) != "UiDoc")
        return Fail("not a UiDoc document");
    if(root.Find("version") < 0 || (int)root["version"] != UIDOC_JSON_VERSION)
        return Fail("unsupported UiDoc version");
    if(root.Find("text") < 0)
        return Fail("UiDoc text is missing");

    UiDocCore next;
    next.history_limit_ = history_limit_;
    next.text_ = ToUnicode(AsString(root["text"]), CHARSET_UTF8);
    next.styles_.Clear();
    next.blocks_.Clear();
    next.annotations_.Clear();
    next.resources_.Clear();
    next.embeds_.Clear();
    next.anchors_.Clear();
    next.meta_.Clear();
    next.undo_.Clear();
    next.redo_.Clear();
    if(root.Find("meta") >= 0 && !UnpackMap(root["meta"], next.meta_))
        return Fail("invalid document metadata");

    ValueArray array;
    if(!ReadArray(root, "styles", array))
        return Fail("invalid styles array");
    for(int i = 0; i < array.GetCount(); i++) {
        UiDocStyleRun run;
        if(!StyleRunFromValue(array[i], run))
            return Fail("invalid style run");
        next.styles_.Add(pick(run));
    }
    next.NormalizeStyles();

    if(!ReadArray(root, "blocks", array))
        return Fail("invalid blocks array");
    for(int i = 0; i < array.GetCount(); i++) {
        UiDocBlock block;
        if(!BlockFromValue(array[i], block))
            return Fail("invalid block");
        next.blocks_.Add(pick(block));
    }

    if(!ReadArray(root, "annotations", array))
        return Fail("invalid annotations array");
    for(int i = 0; i < array.GetCount(); i++) {
        UiDocAnnotation annotation;
        if(!AnnotationFromValue(array[i], annotation))
            return Fail("invalid annotation");
        next.annotations_.Add(pick(annotation));
    }

    if(!ReadArray(root, "resources", array))
        return Fail("invalid resources array");
    for(int i = 0; i < array.GetCount(); i++) {
        UiDocResource resource;
        if(!ResourceFromValue(array[i], resource))
            return Fail("invalid resource");
        next.resources_.Add(pick(resource));
    }

    if(!ReadArray(root, "embeds", array))
        return Fail("invalid embeds array");
    for(int i = 0; i < array.GetCount(); i++) {
        UiDocEmbedBlock embed;
        if(!EmbedFromValue(array[i], embed))
            return Fail("invalid embed");
        next.embeds_.Add(pick(embed));
    }

    if(!ReadArray(root, "anchors", array))
        return Fail("invalid anchors array");
    for(int i = 0; i < array.GetCount(); i++) {
        if(!array[i].Is<ValueMap>())
            return Fail("invalid anchor");
        ValueMap anchor = array[i];
        if(anchor.Find("id") < 0 || anchor.Find("pos") < 0)
            return Fail("invalid anchor");
        String id = AsString(anchor["id"]);
        if(id.IsEmpty() || next.anchors_.Find(id) >= 0)
            return Fail("invalid anchor id");
        next.anchors_.Add(id, (int)anchor["pos"]);
    }

    String validation;
    if(!next.Validate(&validation))
        return Fail(validation);

    int old_length = text_.GetCount();
    uint64 before = revision_;
    text_ = pick(next.text_);
    styles_ = pick(next.styles_);
    blocks_ = pick(next.blocks_);
    annotations_ = pick(next.annotations_);
    resources_ = pick(next.resources_);
    embeds_ = pick(next.embeds_);
    anchors_ = pick(next.anchors_);
    meta_ = pick(next.meta_);
    undo_.Clear();
    redo_.Clear();
    RebuildIds();
    Touch();

    UiDocApplyResult result;
    result.ok = true;
    result.revision_before = before;
    result.revision_after = revision_;
    result.changed_range = UiDocRange(0, max(old_length, text_.GetCount()));
    WhenChange(result);
    if(error)
        error->Clear();
    return true;
}

bool UiDocCore::Save(const String& path, String* error) const
{
    if(path.IsEmpty()) {
        if(error)
            *error = "empty UiDoc path";
        return false;
    }
    if(!Upp::SaveFile(path, ToJson(true))) {
        if(error)
            *error = "unable to save UiDoc file";
        return false;
    }
    if(error)
        error->Clear();
    return true;
}

bool UiDocCore::Load(const String& path, String* error)
{
    if(path.IsEmpty()) {
        if(error)
            *error = "empty UiDoc path";
        return false;
    }
    if(!FileExists(path)) {
        if(error)
            *error = "unable to load UiDoc file";
        return false;
    }
    return FromJson(Upp::LoadFile(path), error);
}

}
