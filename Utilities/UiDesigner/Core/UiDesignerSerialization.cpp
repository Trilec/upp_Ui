#include "UiDesignerSerialization.h"

namespace Upp {

// Designer documents may contain Color and nested Value containers, while
// U++'s JSON writer deliberately rejects arbitrary runtime Value types.
static Value EncodeDocumentValue(const Value& value)
{
    if(value.Is<Color>()) {
        Color c = value;
        ValueMap color;
        color.Set("$type", "Color");
        color.Set("r", c.GetR());
        color.Set("g", c.GetG());
        color.Set("b", c.GetB());
        return color;
    }
    if(value.Is<ValueArray>()) {
        ValueArray array = value;
        ValueArray encoded;
        for(const Value& item : array)
            encoded.Add(EncodeDocumentValue(item));
        return encoded;
    }
    if(value.Is<ValueMap>()) {
        ValueMap map = value;
        ValueMap encoded;
        for(int i = 0; i < map.GetCount(); i++)
            encoded.Set(map.GetKey(i), EncodeDocumentValue(map.GetValue(i)));
        return encoded;
    }
    return value;
}

static Value DecodeDocumentValue(const Value& value)
{
    if(value.Is<ValueMap>()) {
        ValueMap map = value;
        if((String)UiDesignerMapValue(map, "$type", "") == "Color")
            return Color((int)UiDesignerMapValue(map, "r", 0),
                         (int)UiDesignerMapValue(map, "g", 0),
                         (int)UiDesignerMapValue(map, "b", 0));
        ValueMap decoded;
        for(int i = 0; i < map.GetCount(); i++)
            decoded.Set(map.GetKey(i), DecodeDocumentValue(map.GetValue(i)));
        return decoded;
    }
    if(value.Is<ValueArray>()) {
        ValueArray array = value;
        ValueArray decoded;
        for(const Value& item : array)
            decoded.Add(DecodeDocumentValue(item));
        return decoded;
    }
    return value;
}

static ValueMap DecodeDocumentMap(const Value& value)
{
    Value decoded = DecodeDocumentValue(value);
    return decoded.Is<ValueMap>() ? (ValueMap)decoded : ValueMap();
}

static Value ActionToValue(const UiDesignerActionBinding& binding)
{
    ValueMap out;
    out.Set("id", binding.id);
    out.Set("event", binding.event_id);
    out.Set("action", UiDesignerActionTypeName(binding.action));
    out.Set("target", binding.target);
    out.Set("property", binding.target_property);
    out.Set("value", EncodeDocumentValue(binding.value));
    out.Set("delta", binding.delta);
    out.Set("handler", binding.handler_name);
    out.Set("enabled", binding.enabled);
    return out;
}

static Value ResourceToValue(const UiDesignerResource& resource)
{
    ValueMap out;
    out.Set("key", resource.key);
    out.Set("resource_type", resource.resource_type);
    out.Set("content_hash", resource.content_hash);
    out.Set("bytes", Base64Encode(resource.bytes));
    out.Set("mime", resource.mime);
    out.Set("original_name", resource.original_name);
    out.Set("width", resource.width);
    out.Set("height", resource.height);
    out.Set("metadata", EncodeDocumentValue(resource.metadata));
    return out;
}

static bool ResourceFromValue(const Value& value, UiDesignerResource& resource,
                              String& error)
{
    if(!value.Is<ValueMap>()) {
        error = "Resource must be an object";
        return false;
    }
    ValueMap map = value;
    resource.key = UiDesignerMapValue(map, "key", "");
    resource.resource_type = UiDesignerMapValue(map, "resource_type", "");
    resource.content_hash = UiDesignerMapValue(map, "content_hash", "");
    resource.bytes = Base64Decode(UiDesignerMapValue(map, "bytes", ""));
    resource.mime = UiDesignerMapValue(map, "mime", "");
    resource.original_name = UiDesignerMapValue(map, "original_name", "");
    resource.width = (int)UiDesignerMapValue(map, "width", 0);
    resource.height = (int)UiDesignerMapValue(map, "height", 0);
    resource.metadata = DecodeDocumentMap(UiDesignerMapValue(map, "metadata", ValueMap()));
    if(resource.key.IsEmpty() || resource.resource_type.IsEmpty() || resource.bytes.IsEmpty()) {
        error = "Resource is missing key, type, or bytes";
        return false;
    }
    if(resource.content_hash.IsEmpty())
        resource.content_hash = SHA256StringS(resource.bytes);
    if(resource.content_hash != SHA256StringS(resource.bytes)) {
        error = "Resource content hash does not match bytes for " + resource.key;
        return false;
    }
    return true;
}

static bool ActionFromValue(const Value& value,
                            UiDesignerActionBinding& binding,
                            String& error)
{
    if(!value.Is<ValueMap>()) {
        error = "Action binding must be an object";
        return false;
    }
    ValueMap map = value;
    binding.id = UiDesignerMapValue(map, "id", AsString(Uuid::Create()));
    binding.event_id = UiDesignerMapValue(map, "event", "");
    const String action = UiDesignerMapValue(map, "action", "CallNamedHandler");
    if(!UiDesignerParseActionType(action, binding.action)) {
        error = "Unknown action type: " + action;
        return false;
    }
    binding.target = (int64)UiDesignerMapValue(map, "target", 0);
    binding.target_property = UiDesignerMapValue(map, "property", "");
    binding.value = DecodeDocumentValue(UiDesignerMapValue(map, "value", Value()));
    binding.delta = (double)UiDesignerMapValue(map, "delta", 0.0);
    binding.handler_name = UiDesignerMapValue(map, "handler", "");
    binding.enabled = (bool)UiDesignerMapValue(map, "enabled", true);
    return binding.IsValid(&error);
}

static Value NodeToValue(const UiDesignerNode& node)
{
    ValueMap out;
    out.Set("id", node.id);
    out.Set("parent", node.parent);
    out.Set("type", node.type);
    out.Set("name", node.name);
    out.Set("flags", (int64)node.flags);
    ValueArray children;
    for(UiDesignerNodeId id : node.children)
        children.Add(id);
    out.Set("children", children);
    out.Set("properties", EncodeDocumentValue(node.properties));
    if(!node.data.IsEmpty())
        out.Set("data", EncodeDocumentValue(node.data));
    if(!node.theme_overrides.IsEmpty())
        out.Set("theme_overrides", EncodeDocumentValue(node.theme_overrides));
    if(!node.theme_override_saved.IsEmpty())
        out.Set("theme_override_saved", EncodeDocumentValue(node.theme_override_saved));
    ValueArray actions;
    for(const UiDesignerActionBinding& binding : node.actions)
        actions.Add(ActionToValue(binding));
    out.Set("actions", actions);
    return out;
}

Value UiDesignerDocumentToValue(const UiDesignerDocument& document)
{
    ValueMap out;
    out.Set("format", "upp-ui-designer-next");
    out.Set("schema", 4);
    out.Set("ordering", "explicit-children");
    out.Set("document_id", document.GetDocumentId());
    out.Set("revision", (int64)document.GetRevision());

    ValueMap size;
    size.Set("cx", document.GetVirtualSize().cx);
    size.Set("cy", document.GetVirtualSize().cy);
    out.Set("virtual_size", size);

    ValueArray nodes;
    for(const UiDesignerNode& node : document.GetNodes())
        nodes.Add(NodeToValue(node));
    out.Set("nodes", nodes);
    ValueArray resources;
    for(const UiDesignerResource& resource : document.GetResources())
        resources.Add(ResourceToValue(resource));
    out.Set("resources", resources);
    return out;
}

static bool LegacyHexDigit(int c, int& out)
{
    if(c >= '0' && c <= '9') { out = c - '0'; return true; }
    if(c >= 'a' && c <= 'f') { out = c - 'a' + 10; return true; }
    if(c >= 'A' && c <= 'F') { out = c - 'A' + 10; return true; }
    return false;
}

static Value LegacyHexColor(const String& text)
{
    if(text.GetCount() != 7 || text[0] != '#')
        return Null;
    int v[6];
    for(int i = 0; i < 6; i++)
        if(!LegacyHexDigit(text[i + 1], v[i]))
            return Null;
    return Color(v[0] * 16 + v[1], v[2] * 16 + v[3],
                 v[4] * 16 + v[5]);
}

static Value LegacyPropertyValue(const Value& encoded)
{
    if(!encoded.Is<ValueMap>())
        return encoded;
    ValueMap item = encoded;
    const String type = UiDesignerMapValue(item, "type", "string");
    const Value value = UiDesignerMapValue(item, "value", Value());
    if(type == "null") return Null;
    if(type == "bool") return (bool)value;
    if(type == "int") return (int)value;
    if(type == "int64") return (int64)value;
    if(type == "number") return (double)value;
    if(type == "color" && value.Is<String>()) return LegacyHexColor(value);
    return value;
}

static ValueMap LegacyProperties(const Value& encoded)
{
    ValueMap result;
    if(!encoded.Is<ValueMap>())
        return result;
    ValueMap source = encoded;
    for(int i = 0; i < source.GetCount(); i++)
        result.Set(AsString(source.GetKey(i)),
                   LegacyPropertyValue(source.GetValue(i)));
    return result;
}

static String NormalizeLegacyType(String type)
{
    static const char *from[] = {
        "BoxLayout", "GridLayout", "AbsoluteLayout", "Splitter", "QuadSplitter",
        "Panel", "GroupPanel", "ScrollPanel", "Tab", "Stack",
        "Accordion", "TitleCard", "Label", "CheckBox", "RadioButton",
        "Toggle", "Button", "ToolButton", "SplitButton", "LineEdit",
        "IntEdit", "FloatEdit", "PasswordEdit", "MultiEdit", "MaskEdit",
        "ProgressBar", "Slider", "Breadcrumbs", "SliderEdit", "ScrollBar",
        "Table", "Doc", "Tree", "List", "BezierCurveEditor",
        "BezierCurveField", "Dropdown", "Menu", "ColorPicker", "Spacer"
    };
    static const char *to[] = {
        "UiBoxLayout", "UiGridLayout", "UiAbsoluteLayout", "UiSplitter", "UiQuadSplitter",
        "UiPanel", "UiGroupPanel", "UiScrollPanel", "UiTab", "UiStack",
        "UiAccordion", "UiTitleCard", "UiLabel", "UiCheckBox", "UiRadioButton",
        "UiToggle", "UiButton", "UiToolButton", "UiSplitButton", "UiLineEdit",
        "UiIntEdit", "UiFloatEdit", "UiPasswordEdit", "UiMultiEdit", "UiMaskEdit",
        "UiProgressBar", "UiSlider", "UiBreadcrumbs", "UiSliderEdit", "UiScrollBar",
        "UiTable", "UiDoc", "UiTree", "UiList", "UiBezierCurveEditor",
        "UiBezierCurveField", "UiDropdown", "UiMenu", "UiColorPicker", "Spacer"
    };
    for(int i = 0; i < __countof(from); i++)
        if(type == from[i])
            return to[i];
    if(type == "PageSlot" || type == "PaneSlot" ||
       type == "AccordionSectionSlot")
        return "UiPanel";
    return type;
}

static bool LoadActions(UiDesignerNode& node, const ValueArray& encoded,
                        const VectorMap<int64, int64>& id_map, String& error)
{
    node.actions.Clear();
    for(const Value& item : encoded) {
        UiDesignerActionBinding binding;
        if(!ActionFromValue(item, binding, error))
            return false;
        if(binding.target) {
            const int q = id_map.Find(binding.target);
            if(q < 0) {
                error = "Action binding references missing target " +
                        AsString(binding.target);
                return false;
            }
            binding.target = id_map[q];
        }
        node.SetAction(pick(binding));
    }
    return true;
}

static String MigrateLegacySizingMode(const Value& value)
{
    const String mode = AsString(value);
    if(mode == "Fill" || mode == "Expand")
        return "Expand";
    if(mode == "Fixed")
        return "Fixed";
    return "Fit";
}

static void NormalizePlacementProperties(ValueMap& properties,
                                         const String& type = String())
{
    if(properties.Find("cell_align_x") >= 0 &&
       (String)properties.GetValue(properties.Find("cell_align_x")) == "Auto")
        properties.Set("cell_align_x", "Center");
    if(properties.Find("cell_align_y") >= 0 &&
       (String)properties.GetValue(properties.Find("cell_align_y")) == "Auto")
        properties.Set("cell_align_y", "Center");
    const struct { const char *old_id; const char *new_id; } aliases[] = {
        {"minimum_width", "min_width"}, {"minimum_height", "min_height"},
        {"maximum_width", "max_width"}, {"maximum_height", "max_height"},
    };
    for(const auto& alias : aliases)
        if(properties.Find(alias.old_id) >= 0 && properties.Find(alias.new_id) < 0)
            properties.Set(alias.new_id, properties.GetValue(properties.Find(alias.old_id)));
    if(properties.Find("width_mode") < 0 && properties.Find("width") >= 0) {
        properties.Set("width_mode", "Fixed");
        properties.Set("fixed_width", properties.GetValue(properties.Find("width")));
    }
    if(properties.Find("height_mode") < 0 && properties.Find("height") >= 0) {
        properties.Set("height_mode", "Fixed");
        properties.Set("fixed_height", properties.GetValue(properties.Find("height")));
    }
    // h_sizing/v_sizing were the former common-control contract. Keep them
    // only for the semantic Spacer, whose line/blank behavior still owns
    // those fields; ordinary controls are migrated once on load.
    if(type != "Spacer") {
        if(properties.Find("width_mode") < 0 && properties.Find("h_sizing") >= 0)
            properties.Set("width_mode",
                           MigrateLegacySizingMode(properties.GetValue(properties.Find("h_sizing"))));
        if(properties.Find("height_mode") < 0 && properties.Find("v_sizing") >= 0)
            properties.Set("height_mode",
                           MigrateLegacySizingMode(properties.GetValue(properties.Find("v_sizing"))));
        const int horizontal = properties.Find("h_sizing");
        const int vertical = properties.Find("v_sizing");
        if(horizontal >= 0)
            properties.Remove(horizontal);
        if(vertical >= 0)
            properties.Remove(vertical);
    }
}

static bool IsLegacyButtonThemeOverride(const String& id)
{
    static const char *obsolete[] = {
        "role", "icon", "icon_side", "icon_width", "icon_height",
        "icon_render_mode", "scale_icon_to_content", "align_h", "align_v",
        "content_gap", "content_inset_left", "content_inset_top",
        "content_inset_right", "content_inset_bottom", "click_focus",
        "checkable", "checked", "empty_hint"
    };
    for(const char *candidate : obsolete)
        if(id == candidate)
            return true;
    return false;
}

static void NormalizeThemeOverrides(ValueMap& theme_overrides)
{
    for(int i = theme_overrides.GetCount() - 1; i >= 0; i--)
        if(IsLegacyButtonThemeOverride(AsString(theme_overrides.GetKey(i))))
            theme_overrides.Remove(i);
}

static bool RestoreExplicitChildOrder(const ValueArray& nodes,
                                      const VectorMap<int64, int64>& id_map,
                                      UiDesignerDocument& loaded,
                                      String& error)
{
    VectorMap<int64, int64> declared_parent;
    for(const Value& item : nodes) {
        if(!item.Is<ValueMap>())
            continue;
        ValueMap encoded = item;
        const int64 id = UiDesignerMapValue(encoded, "id", 0);
        declared_parent.GetAdd(id) = UiDesignerMapValue(encoded, "parent", 0);
    }
    for(const Value& item : nodes) {
        if(!item.Is<ValueMap>())
            continue;
        ValueMap encoded = item;
        const int64 old_parent = UiDesignerMapValue(encoded, "id", 0);
        const int parent_q = id_map.Find(old_parent);
        if(parent_q < 0)
            continue;
        ValueArray children = UiDesignerMapValue(encoded, "children", ValueArray());
        Index<int64> seen;
        for(int i = 0; i < children.GetCount(); ++i) {
            const int64 old_child = children[i];
            if(seen.Find(old_child) >= 0) {
                error = "Parent contains duplicate child " + AsString(old_child);
                return false;
            }
            seen.Add(old_child);
            const int child_q = id_map.Find(old_child);
            const int declared_q = declared_parent.Find(old_child);
            if(child_q < 0 || declared_q < 0 || declared_parent[declared_q] != old_parent) {
                error = "Explicit child order disagrees with parent for " +
                        AsString(old_child);
                return false;
            }
            if(!loaded.MoveNode(id_map[child_q], id_map[parent_q], i)) {
                error = "Unable to restore child order for " + AsString(old_parent);
                return false;
            }
        }
    }
    return true;
}

static bool LoadNodes(const ValueArray& nodes, bool legacy,
                      UiDesignerDocument& loaded, String& error)
{
    if(nodes.IsEmpty()) {
        error = "Document has no nodes";
        return false;
    }
    ValueMap root_node = nodes[0];
    UiDesignerNode* window = loaded.Find(loaded.GetRootId());
    window->name = UiDesignerMapValue(root_node, "name", "Window");
    window->properties = legacy
        ? LegacyProperties(UiDesignerMapValue(root_node, "properties", ValueMap()))
        : DecodeDocumentMap(UiDesignerMapValue(root_node, "properties", ValueMap()));
    window->theme_overrides = legacy
        ? ValueMap()
        : DecodeDocumentMap(UiDesignerMapValue(root_node, "theme_overrides", ValueMap()));
    NormalizePlacementProperties(window->properties);
    NormalizeThemeOverrides(window->theme_overrides);

    VectorMap<int64, int64> id_map;
    id_map.Add((int64)UiDesignerMapValue(root_node, "id", 1), loaded.GetRootId());
    UiDesignerNodeId next_migration_id = loaded.GetRootId() + 1;
    for(const Value& item : nodes) {
        if(!item.Is<ValueMap>())
            continue;
        ValueMap encoded = item;
        const UiDesignerNodeId id =
            (int64)UiDesignerMapValue(encoded, "id", 0);
        if(next_migration_id <= id)
            next_migration_id = id + 1;
    }
    VectorMap<UiDesignerNodeId, ValueArray> pending_actions;
    pending_actions.Add(loaded.GetRootId(),
                        UiDesignerMapValue(root_node, "actions", ValueArray()));

    Vector<int> pending;
    for(int i = 1; i < nodes.GetCount(); i++)
        pending.Add(i);

    UiDesignerNodeId legacy_root_layout = 0;
    while(!pending.IsEmpty()) {
        bool progressed = false;
        // Add children in source order. AddNodeWithId appends to the parent's
        // child vector, so walking pending backwards inverted legacy layouts.
        for(int p = 0; p < pending.GetCount(); p++) {
            ValueMap n = nodes[pending[p]];
            const int64 old_id = UiDesignerMapValue(n, "id", 0);
            if(old_id <= 0 || id_map.Find(old_id) >= 0) {
                error = "Document contains an invalid or duplicate node ID";
                return false;
            }
            const int64 old_parent = UiDesignerMapValue(n, "parent", 0);
            const int parent_q = id_map.Find(old_parent);
            if(parent_q < 0)
                continue;

            ValueMap properties = legacy
                ? LegacyProperties(UiDesignerMapValue(n, "properties", ValueMap()))
                : DecodeDocumentMap(UiDesignerMapValue(n, "properties", ValueMap()));
            NormalizePlacementProperties(properties,
                                         UiDesignerMapValue(n, "type", String()));
            if(legacy && UiDesignerMapValue(n, "last_rect", Value()).Is<ValueMap>()) {
                ValueMap r = UiDesignerMapValue(n, "last_rect", ValueMap());
                const int left = UiDesignerMapValue(r, "left", 0);
                const int top = UiDesignerMapValue(r, "top", 0);
                const int right = UiDesignerMapValue(r, "right", left + 160);
                const int bottom = UiDesignerMapValue(r, "bottom", top + 32);
                if(properties.Find("x") < 0) properties.Set("x", left);
                if(properties.Find("y") < 0) properties.Set("y", top);
                if(properties.Find("width") < 0) properties.Set("width", max(20, right - left));
                if(properties.Find("height") < 0) properties.Set("height", max(20, bottom - top));
            }

            const String type = legacy
                ? NormalizeLegacyType(UiDesignerMapValue(n, "type", "UiLabel"))
                : (String)UiDesignerMapValue(n, "type", "UiLabel");
            UiDesignerNodeId parent = id_map[parent_q];
            if(legacy && type == "Spacer" && parent == loaded.GetRootId()) {
                if(!legacy_root_layout) {
                    legacy_root_layout = loaded.AddNodeWithId(
                        next_migration_id++, "UiBoxLayout",
                        "legacy_root_layout", loaded.GetRootId(),
                        UiDesignerNodeContainer | UiDesignerNodeLayout);
                    UiDesignerNode* layout = loaded.Find(legacy_root_layout);
                    if(!layout) {
                        error = "Unable to create legacy root layout";
                        return false;
                    }
                    layout->properties.Set("direction", "V");
                    layout->properties.Set("x", 20);
                    layout->properties.Set("y", 20);
                    layout->properties.Set("width", loaded.GetVirtualSize().cx - 40);
                    layout->properties.Set("height", loaded.GetVirtualSize().cy - 40);
                }
                parent = legacy_root_layout;
            }

            dword flags = (dword)(int64)UiDesignerMapValue(n, "flags", 0);
            if(type == "Spacer")
                flags |= UiDesignerNodeStructural | UiDesignerNodeSemanticItem;
            const UiDesignerNodeId new_id = loaded.AddNodeWithId(
                old_id, type, UiDesignerMapValue(n, "name", "control"),
                parent, flags);
            if(!new_id) {
                error = "Unable to restore node identity " + AsString(old_id);
                return false;
            }
            UiDesignerNode* created = loaded.Find(new_id);
            created->properties = pick(properties);
            created->data = legacy
                ? ValueMap()
                : DecodeDocumentMap(UiDesignerMapValue(n, "data", ValueMap()));
            created->theme_overrides = legacy
                ? ValueMap()
                : DecodeDocumentMap(UiDesignerMapValue(n, "theme_overrides", ValueMap()));
            created->theme_override_saved = legacy
                ? ValueMap()
                : DecodeDocumentMap(UiDesignerMapValue(n, "theme_override_saved", ValueMap()));
            NormalizeThemeOverrides(created->theme_overrides);
            id_map.Add(old_id, new_id);
            pending_actions.Add(new_id,
                UiDesignerMapValue(n, "actions", ValueArray()));
            pending.Remove(p);
            progressed = true;
            // Restart from the first unresolved source node after removal.
            // This preserves sibling order even when children precede a
            // not-yet-resolved parent in the flat legacy array.
            break;
        }
        if(!progressed) {
            error = "Document contains a missing or cyclic parent reference";
            return false;
        }
    }

    if(!RestoreExplicitChildOrder(nodes, id_map, loaded, error))
        return false;

    for(int i = 0; i < pending_actions.GetCount(); i++) {
        UiDesignerNode* node = loaded.Find(pending_actions.GetKey(i));
        if(node && !LoadActions(*node, pending_actions[i], id_map, error))
            return false;
    }
    return true;
}

bool UiDesignerDocumentFromValue(const Value& value, UiDesignerDocument& document,
                                 String& error)
{
    if(!value.Is<ValueMap>()) {
        error = "Document root must be an object";
        return false;
    }
    ValueMap root = value;
    const String format = UiDesignerMapValue(root, "format", "");
    const bool legacy = format == "upp-ui-designer";
    if(!legacy && format != "upp-ui-designer-next") {
        error = "Unsupported UiDesigner document format";
        return false;
    }

    ValueArray nodes = UiDesignerMapValue(root, "nodes", ValueArray());
    if(nodes.IsEmpty() || !nodes[0].Is<ValueMap>()) {
        error = "Document has no valid root node";
        return false;
    }
    ValueMap size = UiDesignerMapValue(root, "virtual_size", ValueMap());
    const Size virtual_size((int)UiDesignerMapValue(size, "cx", 1020),
                            (int)UiDesignerMapValue(size, "cy", 668));

    ValueMap encoded_root = nodes[0];
    const UiDesignerNodeId persisted_root_id =
        (int64)UiDesignerMapValue(encoded_root, "id", 1);
    if(persisted_root_id <= 0) {
        error = "Document root has an invalid node ID";
        return false;
    }

    UiDesignerDocument loaded;
    loaded.NewDocument(virtual_size, persisted_root_id);
    if(!legacy)
        loaded.SetDocumentId(UiDesignerMapValue(root, "document_id",
                                                AsString(Uuid::Create())));

    if(!legacy) {
        const ValueArray resources =
            UiDesignerMapValue(root, "resources", ValueArray());
        for(const Value& item : resources) {
            UiDesignerResource resource;
            if(!ResourceFromValue(item, resource, error) ||
               !loaded.AddResource(resource, false))
                return false;
        }
    }

    if(!LoadNodes(nodes, legacy, loaded, error))
        return false;

    document.ReplaceFrom(loaded, legacy ? "Import legacy document"
                                        : "Load document", false);
    error.Clear();
    return true;
}

String UiDesignerSerialize(const UiDesignerDocument& document, bool pretty)
{
    return AsJSON(UiDesignerDocumentToValue(document), pretty);
}

bool UiDesignerDeserialize(const String& json, UiDesignerDocument& document,
                           String& error)
{
    Value value = ParseJSON(json);
    if(IsError(value)) {
        error = GetErrorText(value);
        return false;
    }
    return UiDesignerDocumentFromValue(value, document, error);
}

bool UiDesignerSaveFile(const String& path, const UiDesignerDocument& document,
                        String& error)
{
    if(!SaveFile(path, UiDesignerSerialize(document, true))) {
        error = "Unable to save " + path;
        return false;
    }
    error.Clear();
    return true;
}

bool UiDesignerLoadFile(const String& path, UiDesignerDocument& document,
                        String& error)
{
    const String json = LoadFile(path);
    if(IsNull(json)) {
        error = "Unable to load " + path;
        return false;
    }
    return UiDesignerDeserialize(json, document, error);
}

}
