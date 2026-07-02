#include "SymbolPickerExport.h"

namespace Upp {

static const char* SymbolPickerIconStyleText(SymbolPickerIconStyle style)
{
	switch(style) {
	case SymbolPickerIconStyle::Outlined: return "outlined";
	case SymbolPickerIconStyle::Rounded:  return "rounded";
	case SymbolPickerIconStyle::Sharp:    return "sharp";
	}
	return "outlined";
}

static String NormalizePrefix(const String& text)
{
	String prefix = MakeSymbolPickerSafeCppIdentifierSegment(text);
	if(prefix.IsEmpty())
		prefix = "ICON";
	if(!prefix.EndsWith("_"))
		prefix << '_';
	return prefix;
}

static bool StartsWithIgnoreCase(const String& text, const String& prefix)
{
	if(prefix.IsEmpty() || text.GetCount() < prefix.GetCount())
		return false;
	for(int i = 0; i < prefix.GetCount(); ++i)
		if(ToUpper((byte)text[i]) != ToUpper((byte)prefix[i]))
			return false;
	return true;
}

static String BuildDerivedAlias(const SymbolPickerIconEntry* entry)
{
	if(!entry)
		return String();
	String out;
	if(!entry->category.IsEmpty())
		out << entry->category << '_';
	if(!entry->display_name.IsEmpty())
		out << entry->display_name << '_';
	out << SymbolPickerIconStyleText(entry->style);
	return out;
}

String MakeSymbolPickerSafeCppIdentifierSegment(const String& text)
{
	String out;
	for(int i = 0; i < text.GetCount(); ++i) {
		const int c = (byte)text[i];
		if(IsAlNum(c)) {
			if(!out.IsEmpty() && out[out.GetCount() - 1] == '_' && c == '_')
				continue;
			out.Cat(ToUpper((wchar)c));
		}
		else if(out.IsEmpty() || out[out.GetCount() - 1] != '_')
			out.Cat('_');
	}
	while(!out.IsEmpty() && out[out.GetCount() - 1] == '_')
		out.Trim(out.GetCount() - 1);
	if(out.IsEmpty())
		out = "_";
	if(IsDigit((byte)out[0]))
		out = "_" + out;
	return out;
}

String MakeSymbolPickerSafeIconAlias(const String& text)
{
	return MakeSymbolPickerSafeCppIdentifierSegment(text);
}

String MakeSymbolPickerExportDisplayName(const SymbolPickerProject& project,
	const SymbolPickerCollection& collection,
	const SymbolPickerIconRef& item,
	const SymbolPickerIconEntry* catalog_entry)
{
	String alias = TrimBoth(item.alias);
	if(!alias.IsEmpty())
		return alias;

	if(catalog_entry) {
		String out;
		if(!catalog_entry->category.IsEmpty())
			out << catalog_entry->category;
		if(!catalog_entry->display_name.IsEmpty()) {
			if(!out.IsEmpty())
				out << " / ";
			out << catalog_entry->display_name;
		}
		if(!out.IsEmpty()) {
			out << " (" << SymbolPickerIconStyleText(catalog_entry->style) << ")";
			return out;
		}
	}

	String fallback = TrimBoth(item.category_override);
	if(fallback.IsEmpty())
		fallback = TrimBoth(collection.name);
	if(fallback.IsEmpty())
		fallback = TrimBoth(project.project_name);
	if(fallback.IsEmpty())
		fallback = item.source_id.IsEmpty() ? item.catalog_id : item.source_id;
	if(fallback.IsEmpty())
		fallback = "Unresolved";
	return fallback;
}

String ResolveExportCategory(const SymbolPickerProject& project,
	const SymbolPickerCollection& collection,
	const SymbolPickerIconRef& item,
	const SymbolPickerIconEntry* catalog_entry)
{
	String category = TrimBoth(item.category_override);
	if(!category.IsEmpty())
		return category;

	category = TrimBoth(collection.name);
	if(!category.IsEmpty())
		return category;

	if(catalog_entry && !TrimBoth(catalog_entry->category).IsEmpty())
		return TrimBoth(catalog_entry->category);

	(void)project;
	return "Unresolved";
}

String MakeSymbolPickerExportSymbolName(const SymbolPickerProject& project,
	const SymbolPickerCollection& collection,
	const SymbolPickerIconRef& item,
	const SymbolPickerIconEntry* catalog_entry,
	Index<String>& used_names)
{
	String source = TrimBoth(item.alias);
	if(source.IsEmpty())
		source = BuildDerivedAlias(catalog_entry);
	String symbol = MakeSymbolPickerSafeCppIdentifierSegment(source);
	String prefix = NormalizePrefix(project.symbol_prefix);
	if(!TrimBoth(item.alias).IsEmpty() && StartsWithIgnoreCase(TrimBoth(item.alias), TrimBoth(project.symbol_prefix)))
		prefix.Clear();
	else if(StartsWithIgnoreCase(symbol, prefix))
		prefix.Clear();
	String candidate = prefix + symbol;
	if(candidate.IsEmpty())
		candidate = "ICON_";

	String unique = candidate;
	if(used_names.Find(unique) >= 0) {
		for(int n = 2;; ++n) {
			String next = candidate + "_" + AsString(n);
			if(used_names.Find(next) < 0) {
				unique = next;
				break;
			}
		}
	}
	used_names.Add(unique);
	(void)collection;
	return unique;
}

static bool IsCollectionSelectedForExport(const SymbolPickerProject& project, int collection_index, SymbolPickerExportScope scope)
{
	switch(scope) {
	case SymbolPickerExportScope::ActiveCollection:
		return collection_index == project.active_collection_index;
	case SymbolPickerExportScope::AllCollections:
		return true;
	}
	return false;
}

static String BuildExportWarningBlock(const Vector<String>& warnings)
{
	String out;
	for(const String& warning : warnings)
		out << "// " << warning << '\n';
	if(!out.IsEmpty())
		out << '\n';
	return out;
}

static String EscapeCppString(const String& text)
{
	String out;
	for(int i = 0; i < text.GetCount(); ++i) {
		const char c = text[i];
		switch(c) {
		case '\\': out << "\\\\"; break;
		case '"': out << "\\\""; break;
		case '\n': out << "\\n"; break;
		case '\r': out << "\\r"; break;
		case '\t': out << "\\t"; break;
		default:   out.Cat(c); break;
		}
	}
	return out;
}

static String StyleLabel(const SymbolPickerExportItem& item)
{
	String out = SymbolPickerIconStyleText(item.style);
	if(item.unresolved)
		out << " (unresolved)";
	return out;
}

Vector<SymbolPickerExportItem> BuildSymbolPickerExportItems(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings)
{
	Vector<SymbolPickerExportItem> out;
	Index<String> used_names;

	for(int ci = 0; ci < project.collections.GetCount(); ++ci) {
		const SymbolPickerCollection& collection = project.collections[ci];
		if(!IsCollectionSelectedForExport(project, ci, scope))
			continue;

		for(int ii = 0; ii < collection.items.GetCount(); ++ii) {
			const SymbolPickerIconRef& item = collection.items[ii];
			const SymbolPickerIconEntry* entry = catalog.FindByCatalogId(item.catalog_id);
			if(!entry && !item.source_id.IsEmpty())
				entry = catalog.FindBySourceId(item.source_id);
			if(!entry || item.unresolved) {
				if(warnings) {
					warnings->Add(Format("Skipped unresolved icon %s%s%s in collection '%s'.",
						(item.catalog_id.IsEmpty() ? String("(missing catalog_id)") : item.catalog_id),
						(item.source_id.IsEmpty() ? String() : " / "),
						(item.source_id.IsEmpty() ? String() : item.source_id),
						collection.name));
				}
				continue;
			}

			SymbolPickerExportItem& ex = out.Add();
			ex.category = ResolveExportCategory(project, collection, item, entry);
			ex.symbol_name = MakeSymbolPickerExportSymbolName(project, collection, item, entry, used_names);
			ex.display_name = MakeSymbolPickerExportDisplayName(project, collection, item, entry);
			ex.catalog_id = item.catalog_id;
			ex.source_id = item.source_id;
			ex.alias = item.alias;
			ex.comment = item.comment;
			ex.size = item.size > 0 ? item.size : project.default_size;
			ex.tint = item.tint;
			ex.style = item.has_style_override ? item.style_override : entry->style;
			ex.unresolved = false;
		}
	}

	return out;
}

Vector<SymbolPickerExportCategory> BuildSymbolPickerExportCategories(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings)
{
	Vector<SymbolPickerExportCategory> out;
	VectorMap<String, int> category_index;
	Vector<SymbolPickerExportItem> items = BuildSymbolPickerExportItems(project, catalog, scope, warnings);
	for(const auto& item : items) {
		int cat_pos = category_index.Find(item.category);
		if(cat_pos < 0) {
			cat_pos = out.GetCount();
			category_index.Add(item.category, cat_pos);
			SymbolPickerExportCategory& cat = out.Add();
			cat.name = item.category;
		}
		out[cat_pos].items.Add(item);
	}

	return out;
}

String BuildIconIdExport(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings)
{
	Vector<String> local_warnings;
	Vector<String>& warn = warnings ? *warnings : local_warnings;
	Vector<SymbolPickerExportItem> items = BuildSymbolPickerExportItems(project, catalog, scope, &warn);
	String out = BuildExportWarningBlock(warn);
	for(const auto& item : items) {
		out << item.catalog_id;
		if(!item.alias.IsEmpty())
			out << " // " << item.alias;
		else if(!item.display_name.IsEmpty())
			out << " // " << item.display_name;
		if(!item.category.IsEmpty())
			out << " | " << item.category;
		out << '\n';
	}
	return out;
}

String BuildImageCallExport(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings)
{
	Vector<String> local_warnings;
	Vector<String>& warn = warnings ? *warnings : local_warnings;
	Vector<SymbolPickerExportItem> items = BuildSymbolPickerExportItems(project, catalog, scope, &warn);
	String out = BuildExportWarningBlock(warn);
	for(const auto& item : items)
		out << item.symbol_name << '\n';
	return out;
}

String BuildCppSnippetExport(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings)
{
	Vector<String> local_warnings;
	Vector<String>& warn = warnings ? *warnings : local_warnings;
	Vector<SymbolPickerExportItem> items = BuildSymbolPickerExportItems(project, catalog, scope, &warn);
	String out = BuildExportWarningBlock(warn);
	out << "struct SymbolPickerExportRow {\n"
		<< "    const char* symbol_name;\n"
		<< "    const char* catalog_id;\n"
		<< "    const char* source_id;\n"
		<< "    const char* category;\n"
		<< "    int size;\n"
		<< "    const char* comment;\n"
		<< "};\n\n";
	out << "static const SymbolPickerExportRow kSymbolPickerExport[] = {\n";
	for(const auto& item : items) {
		out << "    { \"" << EscapeCppString(item.symbol_name) << "\", \"" << EscapeCppString(item.catalog_id) << "\", \""
			<< EscapeCppString(item.source_id) << "\", \"" << EscapeCppString(item.category) << "\", " << item.size
			<< ", \"" << EscapeCppString(item.comment) << "\" },\n";
	}
	out << "};\n";
	return out;
}

String BuildCategoryListExport(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings)
{
	Vector<String> local_warnings;
	Vector<String>& warn = warnings ? *warnings : local_warnings;
	Vector<SymbolPickerExportCategory> categories = BuildSymbolPickerExportCategories(project, catalog, scope, &warn);
	String out = BuildExportWarningBlock(warn);
	for(const auto& cat : categories) {
		out << cat.name << ":\n";
		for(const auto& item : cat.items)
			out << "  " << item.symbol_name << '\n';
		out << '\n';
	}
	return out;
}

bool RunSymbolPickerExportSmokeTests(const SymbolPickerCatalog& catalog, String& error)
{
	error.Clear();
	auto Fail = [&](const String& msg) {
		error = msg;
		return false;
	};

	SymbolPickerProject project;
	project.project_name = "Export Smoke";
	project.symbol_prefix = "ICON_MYAPP_";
	project.default_size = 48;
	project.active_collection_index = 0;

	SymbolPickerCollection primary;
	primary.name = "Primary";
	primary.comment = "primary collection";

	SymbolPickerIconRef a;
	a.catalog_id = "action/save/outlined";
	a.source_id = "action/save";
	a.alias = "Save action!";
	a.size = 48;
	a.tint = Color(1, 2, 3);
	a.comment = "first";
	a.category_override = "Pinned";
	a.unresolved = false;
	primary.items.Add(a);

	SymbolPickerIconRef b;
	b.catalog_id = "action/save/outlined";
	b.source_id = "action/save";
	b.alias = "Save action!";
	b.size = 64;
	b.tint = Color(4, 5, 6);
	b.comment = "second";
	b.unresolved = false;
	primary.items.Add(b);

	SymbolPickerIconRef c;
	c.catalog_id = "action/save/rounded";
	c.source_id = "action/save";
	c.alias = "ICON_MYAPP Save action!";
	c.size = 32;
	c.tint = Color(7, 8, 9);
	c.comment = "third";
	c.unresolved = false;
	primary.items.Add(c);
	project.collections.Add(pick(primary));

	SymbolPickerCollection secondary;
	secondary.name = "Secondary";
	secondary.comment = "secondary collection";

	SymbolPickerIconRef d;
	d.catalog_id = "content/content_copy/outlined";
	d.source_id = "content/content_copy";
	d.alias = "Copy now";
	d.size = 24;
	d.tint = Color(10, 11, 12);
	d.comment = "fourth";
	d.category_override = "Content";
	d.unresolved = false;
	secondary.items.Add(d);

	SymbolPickerIconRef f;
	f.catalog_id = "action/save/sharp";
	f.source_id = "action/save";
	f.alias = "Quote \"Alias\" \\ sample";
	f.size = 128;
	f.tint = Color(16, 17, 18);
	f.comment = "line1\nline2\t\"tail\"\\";
	f.category_override = "Content";
	f.unresolved = false;
	secondary.items.Add(f);

	SymbolPickerIconRef e;
	e.catalog_id = "legacy/missing_icon/outlined";
	e.source_id = "legacy/missing_icon";
	e.alias = "Missing Glyph";
	e.size = 16;
	e.tint = Color(13, 14, 15);
	e.comment = "fifth";
	e.unresolved = true;
	secondary.items.Add(e);
	project.collections.Add(pick(secondary));

	Vector<String> active_warnings;
	Vector<String> all_warnings;
	Vector<SymbolPickerExportItem> active_items = BuildSymbolPickerExportItems(project, catalog, SymbolPickerExportScope::ActiveCollection, &active_warnings);
	Vector<SymbolPickerExportItem> all_items = BuildSymbolPickerExportItems(project, catalog, SymbolPickerExportScope::AllCollections, &all_warnings);
	Vector<SymbolPickerExportCategory> active_categories = BuildSymbolPickerExportCategories(project, catalog, SymbolPickerExportScope::ActiveCollection, &active_warnings);
	Vector<SymbolPickerExportCategory> all_categories = BuildSymbolPickerExportCategories(project, catalog, SymbolPickerExportScope::AllCollections, &all_warnings);
	if(active_items.GetCount() != 3 || all_items.GetCount() != 5
		|| active_categories.GetCount() != 2 || all_categories.GetCount() != 3) {
		error = "Export smoke did not create enough categories.";
		return false;
	}

	if(!catalog.FindByCatalogId("action/save/outlined")
		|| !catalog.FindByCatalogId("content/content_copy/outlined")) {
		error = "Export smoke could not resolve expected catalog ids.";
		return false;
	}
	if(catalog.FindByCatalogId("legacy/missing_icon/outlined")) {
		error = "Export smoke fake unresolved catalog id unexpectedly resolved.";
		return false;
	}

	int pinned = -1;
	int primary_cat = -1;
	int content = -1;
	for(int i = 0; i < active_categories.GetCount(); ++i) {
		if(active_categories[i].name == "Pinned")
			pinned = i;
		else if(active_categories[i].name == "Primary")
			primary_cat = i;
	}
	for(int i = 0; i < all_categories.GetCount(); ++i) {
		if(all_categories[i].name == "Content")
			content = i;
	}
	if(pinned < 0 || primary_cat < 0 || content < 0) {
		error = "Export smoke category resolution failed.";
		return false;
	}

	if(active_categories[pinned].items.GetCount() != 1
		|| active_categories[primary_cat].items.GetCount() != 2
		|| all_categories[content].items.GetCount() != 2) {
		error = "Export smoke category item counts are wrong.";
		return false;
	}

	if(active_categories[pinned].items[0].symbol_name != "ICON_MYAPP_SAVE_ACTION"
		|| active_categories[primary_cat].items[0].symbol_name != "ICON_MYAPP_SAVE_ACTION_2"
		|| active_categories[primary_cat].items[1].symbol_name != "ICON_MYAPP_SAVE_ACTION_3") {
		error = "Export smoke symbol naming failed.";
		return false;
	}
	if(all_categories[content].items[0].symbol_name != "ICON_MYAPP_COPY_NOW"
		|| all_categories[content].items[1].symbol_name != "ICON_MYAPP_QUOTE_ALIAS_SAMPLE") {
		error = "Export smoke derived symbol naming failed.";
		return false;
	}
	if(active_categories[pinned].items[0].display_name.IsEmpty()
		|| all_categories[content].items[0].display_name.IsEmpty()) {
		error = "Export smoke display names were not built.";
		return false;
	}
	if(active_categories[pinned].items[0].tint != Color(1, 2, 3)
		|| all_categories[content].items[0].tint != Color(10, 11, 12)) {
		error = "Export smoke tint preservation failed.";
		return false;
	}
	if(active_categories[pinned].items[0].category != "Pinned"
		|| active_categories[primary_cat].items[0].category != "Primary"
		|| all_categories[content].items[0].category != "Content") {
		error = "Export smoke category naming failed.";
		return false;
	}
	if(active_warnings.GetCount() != 0 || all_warnings.IsEmpty()) {
		error = "Export smoke did not report unresolved skips.";
		return false;
	}
	if(all_items.GetCount() <= active_items.GetCount()) {
		error = "Export smoke all-collections mode did not include more than active collection coverage.";
		return false;
	}

	String icon_id_export = BuildIconIdExport(project, catalog, SymbolPickerExportScope::AllCollections);
	String image_call_export = BuildImageCallExport(project, catalog, SymbolPickerExportScope::AllCollections);
	String cpp_snippet_export = BuildCppSnippetExport(project, catalog, SymbolPickerExportScope::AllCollections);
	String category_list_export = BuildCategoryListExport(project, catalog, SymbolPickerExportScope::AllCollections);
	if(icon_id_export.IsEmpty() || image_call_export.IsEmpty() || cpp_snippet_export.IsEmpty() || category_list_export.IsEmpty()) {
		error = "Export smoke text builders produced empty output.";
		return false;
	}
	if(icon_id_export.Find("action/save/outlined") < 0
		|| image_call_export.Find("ICON_MYAPP_SAVE_ACTION") < 0)
		return Fail("Export smoke text builders did not include expected content.");
	if(cpp_snippet_export.Find("SymbolPickerExportRow") < 0
		|| category_list_export.Find("Pinned:") < 0
		|| category_list_export.Find("Primary:") < 0
		|| category_list_export.Find("Content:") < 0)
		return Fail("Export smoke text builders did not format the expected structure.");
	if(cpp_snippet_export.Find("\\\"Alias\\\"") < 0
		|| cpp_snippet_export.Find("\\\\ sample") < 0
		|| cpp_snippet_export.Find("\\nline2") < 0
		|| cpp_snippet_export.Find("\\t\\\"tail\\\"\\\\") < 0)
		return Fail("Export smoke C++ escaping failed.");
	if(icon_id_export.Find("ICON_MYAPP_ICON_MYAPP") >= 0
		|| image_call_export.Find("ICON_MYAPP_ICON_MYAPP") >= 0
		|| cpp_snippet_export.Find("ICON_MYAPP_ICON_MYAPP") >= 0) {
		error = "Export smoke prefix handling produced a double prefix.";
		return false;
	}

	String current_all_text = BuildImageCallExport(project, catalog, SymbolPickerExportScope::AllCollections);
	String current_active_text = BuildIconIdExport(project, catalog, SymbolPickerExportScope::ActiveCollection);
	if(current_all_text.IsEmpty() || current_active_text.IsEmpty()) {
		error = "Export smoke all/active text builders returned empty text.";
		return false;
	}

	return true;
}

}
