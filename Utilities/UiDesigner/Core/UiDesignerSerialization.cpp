#include "UiDesignerSerialization.h"

namespace Upp {

static Value ActionToValue(const UiDesignerActionBinding& binding)
{
    ValueMap out;
    out.Set("id", binding.id);
    out.Set("event", binding.event_id);
    out.Set("action", UiDesignerActionTypeName(binding.action));
    out.Set("target", binding.target);
    out.Set("property", binding.target_property);
    out.Set("value", binding.value);
    out.Set("delta", binding.delta);
    out.Set("handler", binding.handler_name);
    out.Set("enabled", binding.enabled);
    return out;
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
    binding.value = UiDesignerMapValue(map, "value", Value());
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
    out.Set("properties", node.properties);
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
    out.Set("schema", 3);
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
        : (ValueMap)UiDesignerMapValue(root_node, "properties", ValueMap());

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
        for(int p = pending.GetCount() - 1; p >= 0; p--) {
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
                : (ValueMap)UiDesignerMapValue(n, "properties", ValueMap());
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
            id_map.Add(old_id, new_id);
            pending_actions.Add(new_id,
                UiDesignerMapValue(n, "actions", ValueArray()));
            pending.Remove(p);
            progressed = true;
        }
        if(!progressed) {
            error = "Document contains a missing or cyclic parent reference";
            return false;
        }
    }

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
