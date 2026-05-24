#include "DesignerSerialization.h"
#include "DesignerDefaults.h"

namespace Upp {

static constexpr int DESIGNER_JSON_SCHEMA = 1;

static Value JsonGet(const ValueMap& m, const char *key, const Value& def = Null)
{
	int q = m.Find(key);
	return q >= 0 ? m.GetValue(q) : def;
}

static ValueMap SizeToJson(Size sz)
{
	ValueMap m;
	m.Set("cx", sz.cx);
	m.Set("cy", sz.cy);
	return m;
}

static Size SizeFromJson(const Value& v, Size def)
{
	if(!IsValueMap(v))
		return def;
	ValueMap m = v;
	return Size((int)JsonGet(m, "cx", def.cx), (int)JsonGet(m, "cy", def.cy));
}

static ValueMap RectToJson(Rect r)
{
	ValueMap m;
	m.Set("left", r.left);
	m.Set("top", r.top);
	m.Set("right", r.right);
	m.Set("bottom", r.bottom);
	return m;
}

static Rect RectFromJson(const Value& v)
{
	if(!IsValueMap(v))
		return Rect(0, 0, 0, 0);
	ValueMap m = v;
	return Rect((int)JsonGet(m, "left", 0), (int)JsonGet(m, "top", 0),
	            (int)JsonGet(m, "right", 0), (int)JsonGet(m, "bottom", 0));
}

static String ColorToHex(Color c)
{
	return Format("#%02x%02x%02x", c.GetR(), c.GetG(), c.GetB());
}

static bool HexDigit(int c, int& out)
{
	if(c >= '0' && c <= '9') { out = c - '0'; return true; }
	if(c >= 'a' && c <= 'f') { out = c - 'a' + 10; return true; }
	if(c >= 'A' && c <= 'F') { out = c - 'A' + 10; return true; }
	return false;
}

static Value HexToColor(const String& s)
{
	if(s.GetCount() != 7 || s[0] != '#')
		return Null;
	int v[6];
	for(int i = 0; i < 6; i++)
		if(!HexDigit(s[i + 1], v[i]))
			return Null;
	return Color(v[0] * 16 + v[1], v[2] * 16 + v[3], v[4] * 16 + v[5]);
}

static ValueMap PropertyToJson(const Value& v)
{
	ValueMap m;
	if(IsNull(v)) {
		m.Set("type", "null");
		m.Set("value", Null);
	}
	else if(v.Is<bool>()) {
		m.Set("type", "bool");
		m.Set("value", (bool)v);
	}
	else if(v.Is<int>()) {
		m.Set("type", "int");
		m.Set("value", (int)v);
	}
	else if(v.Is<int64>()) {
		m.Set("type", "int64");
		m.Set("value", (int64)v);
	}
	else if(v.Is<double>() || v.Is<float>()) {
		m.Set("type", "number");
		m.Set("value", (double)v);
	}
	else if(v.Is<Color>()) {
		m.Set("type", "color");
		m.Set("value", ColorToHex((Color)v));
	}
	else {
		m.Set("type", "string");
		m.Set("value", AsString(v));
	}
	return m;
}

static Value PropertyFromJson(const Value& v)
{
	if(!IsValueMap(v))
		return v;
	ValueMap m = v;
	String type = JsonGet(m, "type", "string");
	Value value = JsonGet(m, "value");
	if(type == "null")
		return Null;
	if(type == "bool")
		return (bool)value;
	if(type == "int")
		return (int)value;
	if(type == "int64")
		return (int64)value;
	if(type == "number")
		return (double)value;
	if(type == "color")
		return HexToColor(value);
	return AsString(value);
}

static ValueMap PropertiesToJson(const ValueMap& props)
{
	ValueMap out;
	for(int i = 0; i < props.GetCount(); i++)
		out.Set(AsString(props.GetKey(i)), PropertyToJson(props.GetValue(i)));
	return out;
}

static void ApplyJsonProperties(ValueMap& props, const Value& v)
{
	if(!IsValueMap(v))
		return;
	ValueMap in = v;
	for(int i = 0; i < in.GetCount(); i++)
		props.Set(AsString(in.GetKey(i)), PropertyFromJson(in.GetValue(i)));
}

static ValueArray IdsToJson(const Vector<DesignerNodeId>& ids)
{
	ValueArray out;
	for(DesignerNodeId id : ids)
		out.Add(id);
	return out;
}

static Vector<DesignerNodeId> IdsFromJson(const Value& v)
{
	Vector<DesignerNodeId> out;
	if(!IsValueArray(v))
		return out;
	ValueArray a = v;
	for(int i = 0; i < a.GetCount(); i++)
		out.Add((int)a[i]);
	return out;
}

String StoreDesignerModelJson(const DesignerModel& model)
{
	ValueMap root;
	root.Set("format", "upp-ui-designer");
	root.Set("schema", DESIGNER_JSON_SCHEMA);
	root.Set("virtual_size", SizeToJson(model.GetVirtualSize()));
	root.Set("selection", IdsToJson(model.GetSelection()));

	ValueArray nodes;
	for(const DesignerNode& n : model.GetNodes()) {
		ValueMap item;
		item.Set("id", n.id);
		item.Set("parent", n.parent);
		item.Set("type", n.type_id);
		item.Set("name", n.name);
		item.Set("children", IdsToJson(n.children));
		item.Set("expanded", n.expanded);
		item.Set("last_rect", RectToJson(n.last_rect));
		item.Set("properties", PropertiesToJson(n.properties));
		nodes.Add(item);
	}
	root.Set("nodes", nodes);
	return AsJSON(root, true);
}

static bool InitDefaults(DesignerNodeState& s, const DesignerRegistry& registry)
{
	const DesignerType* type = registry.Find(s.type_id);
	if(!type)
		return false;
	DesignerNode n;
	n.id = s.id;
	n.parent = s.parent;
	n.type_id = s.type_id;
	n.name = s.name;
	if(type->init_defaults)
		type->init_defaults(n);
	s.properties = n.properties;
	return true;
}

bool LoadDesignerModelJson(DesignerModel& model, const DesignerRegistry& registry,
                           const String& json, String& error, Vector<String>* notes)
{
	error.Clear();
	Value parsed = ParseJSON(json);
	if(parsed.IsError() || !IsValueMap(parsed)) {
		error = "Not a valid designer JSON document.";
		return false;
	}
	ValueMap doc = parsed;
	if(AsString(JsonGet(doc, "format")) != "upp-ui-designer") {
		error = "This is not a U++ Ui Designer document.";
		return false;
	}
	Value nodes_value = JsonGet(doc, "nodes");
	if(!IsValueArray(nodes_value)) {
		error = "Designer document has no nodes array.";
		return false;
	}

	ValueArray node_items = nodes_value;
	Vector<DesignerNodeState> states;
	for(int i = 0; i < node_items.GetCount(); i++) {
		if(!IsValueMap(node_items[i])) {
			error = Format("Node entry %d is not an object.", i);
			return false;
		}
		ValueMap item = node_items[i];
		DesignerNodeState& s = states.Add();
		s.id = (int)JsonGet(item, "id", Designer_NULL);
		s.parent = (int)JsonGet(item, "parent", s.id == Designer_ROOT ? Designer_NULL : Designer_ROOT);
		String saved_type = AsString(JsonGet(item, "type", "Generic"));
		s.type_id = saved_type;
		if(!registry.Find(s.type_id)) {
			s.type_id = registry.Find("Generic") ? "Generic" : "UiPanel";
			if(notes)
				notes->Add(Format("Loaded unknown control '%s' as Generic.", saved_type));
		}
		s.name = AsString(JsonGet(item, "name", s.type_id));
		InitDefaults(s, registry);
		ApplyJsonProperties(s.properties, JsonGet(item, "properties"));
		if(s.type_id == "Generic" && saved_type != "Generic" &&
		   (s.properties.Find("original_type") < 0 ||
		    AsString(s.properties.GetValue(s.properties.Find("original_type"))).IsEmpty()))
			s.properties.Set("original_type", saved_type);
		s.children = IdsFromJson(JsonGet(item, "children"));
		s.expanded = (bool)JsonGet(item, "expanded", true);
		s.last_rect = RectFromJson(JsonGet(item, "last_rect"));
	}
	Size virtual_size = SizeFromJson(JsonGet(doc, "virtual_size"), DesignerWindowSize());
	Vector<DesignerNodeId> selection = IdsFromJson(JsonGet(doc, "selection"));
	return model.ReplaceDocument(states, virtual_size, selection, error);
}

}
