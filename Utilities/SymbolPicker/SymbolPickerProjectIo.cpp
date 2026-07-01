#include "SymbolPickerProjectIo.h"

namespace Upp {

static Value JsonGet(const ValueMap& m, const char *key, const Value& def = Null)
{
	int q = m.Find(key);
	return q >= 0 ? m.GetValue(q) : def;
}

static String ColorToHex(Color c)
{
	if(IsNull(c))
		return String();
	return Format("#%02X%02X%02X", c.GetR(), c.GetG(), c.GetB());
}

static bool HexDigit(int c, int& out)
{
	if(c >= '0' && c <= '9') { out = c - '0'; return true; }
	if(c >= 'a' && c <= 'f') { out = c - 'a' + 10; return true; }
	if(c >= 'A' && c <= 'F') { out = c - 'A' + 10; return true; }
	return false;
}

static Color ColorFromHex(const String& s)
{
	if(IsNull(s) || s.IsEmpty())
		return Null;
	if(s.GetCount() != 7 || s[0] != '#')
		return Null;
	int v[6];
	for(int i = 0; i < 6; ++i)
		if(!HexDigit(s[i + 1], v[i]))
			return Null;
	return Color(v[0] * 16 + v[1], v[2] * 16 + v[3], v[4] * 16 + v[5]);
}

static String IconStyleToString(SymbolPickerIconStyle style)
{
	switch(style) {
	case SymbolPickerIconStyle::Outlined: return "outlined";
	case SymbolPickerIconStyle::Rounded:  return "rounded";
	case SymbolPickerIconStyle::Sharp:    return "sharp";
	}
	return "outlined";
}

static SymbolPickerIconStyle IconStyleFromString(const String& text)
{
	String s = ToLower(TrimBoth(text));
	if(s == "rounded")
		return SymbolPickerIconStyle::Rounded;
	if(s == "sharp")
		return SymbolPickerIconStyle::Sharp;
	return SymbolPickerIconStyle::Outlined;
}

static ValueMap IconRefToJson(const SymbolPickerIconRef& ref)
{
	ValueMap m;
	m.Set("catalog_id", ref.catalog_id);
	m.Set("source_id", ref.source_id);
	m.Set("alias", ref.alias);
	m.Set("size", ref.size);
	m.Set("tint", ColorToHex(ref.tint));
	m.Set("comment", ref.comment);
	if(ref.has_style_override)
		m.Set("style", IconStyleToString(ref.style_override));
	if(!ref.category_override.IsEmpty())
		m.Set("category_override", ref.category_override);
	m.Set("unresolved", ref.unresolved);
	return m;
}

static SymbolPickerIconRef IconRefFromJson(const ValueMap& m)
{
	SymbolPickerIconRef ref;
	ref.catalog_id = AsString(JsonGet(m, "catalog_id"));
	ref.source_id = AsString(JsonGet(m, "source_id"));
	ref.alias = AsString(JsonGet(m, "alias"));
	ref.size = max(1, (int)JsonGet(m, "size", 48));
	ref.tint = ColorFromHex(AsString(JsonGet(m, "tint")));
	ref.comment = AsString(JsonGet(m, "comment"));
	ref.category_override = AsString(JsonGet(m, "category_override"));
	String style = AsString(JsonGet(m, "style"));
	ref.has_style_override = !TrimBoth(style).IsEmpty();
	ref.style_override = ref.has_style_override ? IconStyleFromString(style) : SymbolPickerIconStyle::Outlined;
	ref.unresolved = (bool)JsonGet(m, "unresolved", false);
	return ref;
}

static ValueMap CollectionToJson(const SymbolPickerCollection& collection)
{
	ValueMap m;
	m.Set("name", collection.name);
	m.Set("comment", collection.comment);
	ValueArray items;
	for(const auto& item : collection.items)
		items.Add(IconRefToJson(item));
	m.Set("items", items);
	return m;
}

static SymbolPickerCollection CollectionFromJson(const ValueMap& m)
{
	SymbolPickerCollection collection;
	collection.name = AsString(JsonGet(m, "name"));
	collection.comment = AsString(JsonGet(m, "comment"));
	Value items_v = JsonGet(m, "items");
	if(IsValueArray(items_v)) {
		ValueArray items = items_v;
		for(int i = 0; i < items.GetCount(); ++i) {
			if(IsValueMap(items[i]))
				collection.items.Add(IconRefFromJson((ValueMap)items[i]));
		}
	}
	collection.dirty = false;
	return collection;
}

static String DefaultProjectNameFromPath(const String& path)
{
	String title = GetFileTitle(path);
	return title.EndsWith(".uppicons") ? title.Left(title.GetCount() - 9) : title;
}

bool SaveSymbolPickerProjectJson(const SymbolPickerProject& project, const String& path, String& error)
{
	error.Clear();
	String target = TrimBoth(path);
	if(target.IsEmpty()) {
		error = "Project path is empty.";
		return false;
	}

	ValueMap root;
	root.Set("format", "upp-symbol-picker");
	root.Set("version", 1);
	root.Set("project_name", project.project_name);
	root.Set("comment", project.comment);
	root.Set("output_base_name", project.output_base_name);
	root.Set("symbol_prefix", project.symbol_prefix);
	root.Set("default_size", max(1, project.default_size));
	root.Set("default_tint", ColorToHex(project.default_tint));
	root.Set("default_style", IconStyleToString(project.default_style));
	root.Set("active_collection_index", project.active_collection_index);

	ValueArray collections;
	for(const auto& collection : project.collections)
		collections.Add(CollectionToJson(collection));
	root.Set("collections", collections);

	if(!SaveFile(target, AsJSON(root, true))) {
		error = Format("Failed to write project JSON: %s", target);
		return false;
	}
	return true;
}

bool LoadSymbolPickerProjectJson(const String& path, SymbolPickerProject& out, String& error)
{
	error.Clear();
	String text = LoadFile(path);
	if(text.IsVoid()) {
		error = Format("Failed to read project JSON: %s", path);
		return false;
	}

	Value parsed = ParseJSON(text);
	if(parsed.IsError() || !IsValueMap(parsed)) {
		error = "Project file is not valid JSON.";
		return false;
	}

	ValueMap root = parsed;
	if(AsString(JsonGet(root, "format")) != "upp-symbol-picker") {
		error = "This is not a SymbolPicker project file.";
		return false;
	}
	if((int)JsonGet(root, "version", 0) != 1) {
		error = "Unsupported SymbolPicker project version.";
		return false;
	}
	Value collections_v = JsonGet(root, "collections");
	if(!IsValueArray(collections_v)) {
		error = "Project file is missing a collections array.";
		return false;
	}

	SymbolPickerProject project;
	project.project_name = AsString(JsonGet(root, "project_name", DefaultProjectNameFromPath(path)));
	project.comment = AsString(JsonGet(root, "comment"));
	project.file_path = path;
	project.output_base_name = AsString(JsonGet(root, "output_base_name", "symbols"));
	project.symbol_prefix = AsString(JsonGet(root, "symbol_prefix", "ICON_"));
	project.default_size = max(1, (int)JsonGet(root, "default_size", 48));
	project.default_tint = ColorFromHex(AsString(JsonGet(root, "default_tint")));
	project.default_style = IconStyleFromString(AsString(JsonGet(root, "default_style", "outlined")));
	project.active_collection_index = (int)JsonGet(root, "active_collection_index", -1);

	ValueArray collections = collections_v;
	for(int i = 0; i < collections.GetCount(); ++i) {
		if(!IsValueMap(collections[i]))
			continue;
		SymbolPickerCollection collection = CollectionFromJson((ValueMap)collections[i]);
		collection.file_path = path;
		project.collections.Add(pick(collection));
	}

	out = pick(project);
	return true;
}

bool RunSymbolPickerProjectIoSmokeTests(const SymbolPickerCatalog& catalog, String& error)
{
	error.Clear();

	SymbolPickerProject project;
	project.project_name = "Smoke Project";
	project.comment = "json smoke";
	project.output_base_name = "smoke_symbols";
	project.symbol_prefix = "SMOKE_";
	project.default_size = 32;
	project.default_tint = Color(16, 32, 48);
	project.default_style = SymbolPickerIconStyle::Rounded;
	project.active_collection_index = 1;

	SymbolPickerCollection first;
	first.name = "Primary";
	first.comment = "first collection";
	SymbolPickerIconRef first_a;
	first_a.catalog_id = "action/save/rounded";
	first_a.source_id = "action/save";
	first_a.alias = "Save";
	first_a.size = 32;
	first_a.tint = Color(80, 90, 100);
	first_a.comment = "primary icon";
	first_a.category_override = "action";
	first_a.style_override = SymbolPickerIconStyle::Rounded;
	first_a.has_style_override = true;
	first_a.unresolved = false;
	first.items.Add(first_a);

	SymbolPickerIconRef first_b;
	first_b.catalog_id = "navigation/menu/sharp";
	first_b.source_id = "navigation/menu";
	first_b.alias = "Menu";
	first_b.size = 24;
	first_b.tint = Color(10, 20, 30);
	first_b.comment = "menu icon";
	first_b.unresolved = false;
	first.items.Add(first_b);
	project.collections.Add(pick(first));

	SymbolPickerCollection second;
	second.name = "Secondary";
	second.comment = "second collection";
	SymbolPickerIconRef second_a;
	second_a.catalog_id = "legacy/missing_icon/outlined";
	second_a.source_id = "legacy/missing_icon";
	second_a.alias = "Missing";
	second_a.size = 48;
	second_a.tint = Null;
	second_a.comment = "missing icon";
	second_a.unresolved = true;
	second.items.Add(second_a);

	SymbolPickerIconRef second_b;
	second_b.catalog_id = "content/copy/outlined";
	second_b.source_id = "content/copy";
	second_b.alias = "Copy";
	second_b.size = 64;
	second_b.tint = Color(1, 2, 3);
	second_b.comment = "copy icon";
	second_b.unresolved = false;
	second.items.Add(second_b);
	project.collections.Add(pick(second));

	String path = AppendFileName(GetTempPath(), "symbolpicker_project_smoke.uppicons.json");
	String save_error;
	if(!SaveSymbolPickerProjectJson(project, path, save_error)) {
		error = "Project save smoke failed: " + save_error;
		return false;
	}

	SymbolPickerProject loaded;
	String load_error;
	if(!LoadSymbolPickerProjectJson(path, loaded, load_error)) {
		error = "Project load smoke failed: " + load_error;
		return false;
	}

	if(loaded.project_name != project.project_name
		|| loaded.output_base_name != project.output_base_name
		|| loaded.symbol_prefix != project.symbol_prefix
		|| loaded.default_size != project.default_size
		|| loaded.default_style != project.default_style) {
		error = "Loaded project settings do not match saved settings.";
		return false;
	}
	if(loaded.collections.GetCount() != 2) {
		error = "Loaded project collection count is wrong.";
		return false;
	}
	if(loaded.collections[0].items.GetCount() != 2 || loaded.collections[1].items.GetCount() != 2) {
		error = "Loaded project item count is wrong.";
		return false;
	}
	if(loaded.collections[0].comment != "first collection" || loaded.collections[1].comment != "second collection") {
		error = "Loaded collection comments are wrong.";
		return false;
	}
	if(loaded.collections[0].items[0].catalog_id != "action/save/rounded"
		|| loaded.collections[0].items[0].source_id != "action/save"
		|| loaded.collections[0].items[0].alias != "Save"
		|| loaded.collections[0].items[1].alias != "Menu"
		|| loaded.collections[1].items[0].alias != "Missing"
		|| loaded.collections[1].items[1].alias != "Copy") {
		error = "Loaded project aliases or identities are wrong.";
		return false;
	}
	if(loaded.collections[0].items[0].comment != "primary icon"
		|| loaded.collections[0].items[1].comment != "menu icon"
		|| loaded.collections[1].items[0].comment != "missing icon"
		|| loaded.collections[1].items[1].comment != "copy icon"
		|| !loaded.collections[0].items[0].has_style_override
		|| loaded.collections[0].items[0].style_override != SymbolPickerIconStyle::Rounded) {
		error = "Loaded project item metadata is wrong.";
		return false;
	}
	if(loaded.collections[0].items[0].size != 32
		|| loaded.collections[0].items[1].size != 24
		|| loaded.collections[1].items[0].size != 48
		|| loaded.collections[1].items[1].size != 64) {
		error = "Loaded project item sizes are wrong.";
		return false;
	}
	if(loaded.collections[0].items[0].tint != Color(80, 90, 100)
		|| loaded.collections[0].items[1].tint != Color(10, 20, 30)
		|| !IsNull(loaded.collections[1].items[0].tint)
		|| loaded.collections[1].items[1].tint != Color(1, 2, 3)) {
		error = "Loaded project item tints are wrong.";
		return false;
	}
	if(!catalog.FindByCatalogId("action/save/rounded")
		|| !catalog.FindByCatalogId("navigation/menu/sharp")
		|| !catalog.FindByCatalogId("content/copy/outlined")) {
		error = "Smoke test valid ids are not present in the active catalog.";
		return false;
	}
	if(catalog.FindByCatalogId("legacy/missing_icon/outlined")) {
		error = "Smoke test fake unresolved id unexpectedly exists in the active catalog.";
		return false;
	}
	if(!loaded.collections[1].items[0].unresolved) {
		error = "Loaded fake unresolved item did not stay unresolved.";
		return false;
	}
	DeleteFile(path);
	return true;
}

}
