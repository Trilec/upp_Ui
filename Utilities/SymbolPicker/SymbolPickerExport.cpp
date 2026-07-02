#include "SymbolPickerExport.h"
#include "SymbolPickerGeneratedCatalog.h"

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

static String MakeSafeFileComponent(const String& text)
{
	String out;
	for(int i = 0; i < text.GetCount(); ++i) {
		const char c = text[i];
		if(IsAlNum((byte)c) || c == '_' || c == '-' || c == '.')
			out.Cat(c);
		else if(out.IsEmpty() || out[out.GetCount() - 1] != '_')
			out.Cat('_');
	}
	while(!out.IsEmpty() && (out[out.GetCount() - 1] == '_' || out[out.GetCount() - 1] == '.'))
		out.Trim(out.GetCount() - 1);
	if(out.IsEmpty())
		out = "export";
	return out;
}

static bool HasString(const Vector<String>& values, const String& value)
{
	for(const String& item : values) {
		if(item == value)
			return true;
	}
	return false;
}

static String MakeUniqueFileComponent(const String& base, const Vector<String>& used)
{
	String name = MakeSafeFileComponent(base);
	if(!HasString(used, name))
		return name;
	for(int n = 2;; ++n) {
		String candidate = name + "_" + AsString(n);
		if(!HasString(used, candidate))
			return candidate;
	}
}

static bool EnsureDirectoryPath(const String& path)
{
	String p = NormalizePath(path);
	if(p.IsEmpty())
		return false;
	if(DirectoryExists(p))
		return true;
	String parent = GetFileFolder(p);
	if(!parent.IsEmpty() && parent != p && !DirectoryExists(parent) && !EnsureDirectoryPath(parent))
		return false;
	return DirectoryCreate(p) || DirectoryExists(p);
}

static String SvgHexColor(Color c)
{
	return Format("#%02X%02X%02X", c.GetR(), c.GetG(), c.GetB());
}

static void ReplaceAttribute(String& text, const String& attr, const String& value)
{
	String pattern1 = attr + "=\"";
	int q = text.Find(pattern1);
	if(q >= 0) {
		int start = q + pattern1.GetCount();
		int end = text.Find('"', start);
		if(end >= 0)
			text = text.Left(start) + value + text.Mid(end);
		return;
	}

	String pattern2 = attr + "='";
	q = text.Find(pattern2);
	if(q >= 0) {
		int start = q + pattern2.GetCount();
		int end = text.Find('\'', start);
		if(end >= 0)
			text = text.Left(start) + value + text.Mid(end);
		return;
	}

	int svg = text.Find("<svg");
	if(svg < 0)
		return;
	int gt = text.Find('>', svg);
	if(gt < 0)
		return;
	text.Insert(gt, Format(" %s=\"%s\"", attr, value));
}

static String NormalizeSvgRoot(String svg_xml, int size, Color tint)
{
	String out = svg_xml;
	int svg = out.Find("<svg");
	if(svg < 0)
		return out;

	int gt = out.Find('>', svg);
	if(gt < 0)
		return out;

	String root = out.Mid(svg, gt - svg + 1);
	ReplaceAttribute(root, "width", AsString(size));
	ReplaceAttribute(root, "height", AsString(size));

	if(!IsNull(tint)) {
		String hex = SvgHexColor(tint);
		ReplaceAttribute(root, "color", hex);
		if(root.Find("fill=") < 0)
			ReplaceAttribute(root, "fill", hex);
		out.Replace("currentColor", hex);
	}

	out.Remove(svg, gt - svg + 1);
	out.Insert(svg, root);
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

String BuildSymbolPickerSvgFileName(const SymbolPickerExportItem& item)
{
	String name = TrimBoth(item.symbol_name);
	if(name.IsEmpty())
		name = "icon";
	return MakeSafeFileComponent(name) + ".svg";
}

String BuildSymbolPickerSvgText(const SymbolPickerExportItem& item, const String& svg_xml)
{
	String out = NormalizeSvgRoot(svg_xml, item.size > 0 ? item.size : 48, item.tint);
	if(!IsNull(item.tint)) {
		String hex = SvgHexColor(item.tint);
		if(out.Find("currentColor") < 0 && out.Find("fill=") < 0) {
			int svg = out.Find("<svg");
			if(svg >= 0) {
				int gt = out.Find('>', svg);
				if(gt >= 0)
					out.Insert(gt, Format(" fill=\"%s\"", hex));
			}
		}
	}
	return out;
}

static bool WriteSvgWarningsFile(const String& folder, const Vector<String>& warnings)
{
	if(warnings.IsEmpty())
		return true;
	String path = AppendFileName(folder, "_export_warnings.txt");
	String out;
	for(const String& warning : warnings)
		out << warning << '\n';
	return SaveFile(path, out);
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

bool ExportSymbolPickerSvgFiles(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	const String& output_folder,
	Vector<String>* warnings,
	int* files_written,
	int* files_skipped)
{
	if(files_written)
		*files_written = 0;
	if(files_skipped)
		*files_skipped = 0;

	Vector<String> local_warnings;
	Vector<String>& warn = warnings ? *warnings : local_warnings;
	if(!EnsureDirectoryPath(output_folder)) {
		warn.Add(Format("Could not create output folder '%s'.", output_folder));
		return false;
	}

	bool ok = true;
	int written = 0;
	int skipped = 0;
	Vector<String> used_collection_folders;

	for(int ci = 0; ci < project.collections.GetCount(); ++ci) {
		if(scope == SymbolPickerExportScope::ActiveCollection && ci != project.active_collection_index)
			continue;

		const SymbolPickerCollection& collection = project.collections[ci];
		String collection_dir = output_folder;
		if(scope == SymbolPickerExportScope::AllCollections) {
			String folder = MakeUniqueFileComponent(collection.name, used_collection_folders);
			used_collection_folders.Add(folder);
			collection_dir = AppendFileName(output_folder, folder);
		}
		if(!EnsureDirectoryPath(collection_dir)) {
			warn.Add(Format("Could not create SVG export folder '%s'.", collection_dir));
			ok = false;
			continue;
		}

		Index<String> used_names;
		for(const auto& item : collection.items) {
			const SymbolPickerIconEntry* entry = catalog.FindByCatalogId(item.catalog_id);
			if(!entry && !item.source_id.IsEmpty())
				entry = catalog.FindBySourceId(item.source_id);
			if(!entry || item.unresolved) {
				warn.Add(Format("Skipped unresolved icon %s%s%s in collection '%s'.",
					(item.catalog_id.IsEmpty() ? String("(missing catalog_id)") : item.catalog_id),
					(item.source_id.IsEmpty() ? String() : " / "),
					(item.source_id.IsEmpty() ? String() : item.source_id),
					collection.name));
				++skipped;
				continue;
			}

			SymbolPickerExportItem ex;
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

			String svg_xml;
			if(!DecodeGeneratedSymbolPickerSvg(ex.catalog_id, svg_xml) || svg_xml.IsEmpty()) {
				warn.Add(Format("Could not decode SVG for %s.", ex.catalog_id));
				++skipped;
				ok = false;
				continue;
			}

			String file_name = BuildSymbolPickerSvgFileName(ex);
			String path = AppendFileName(collection_dir, file_name);
			String svg_text = BuildSymbolPickerSvgText(ex, svg_xml);
			if(!SaveFile(path, svg_text)) {
				warn.Add(Format("Could not write SVG file '%s'.", path));
				++skipped;
				ok = false;
				continue;
			}
			++written;
		}
	}

	if(written == 0) {
		warn.Add("SVG export completed without writing any files.");
		ok = false;
	}

	if(files_written)
		*files_written = written;
	if(files_skipped)
		*files_skipped = skipped;

	if(!WriteSvgWarningsFile(output_folder, warn))
		ok = false;
	return ok;
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

	String dup_temp_dir = AppendFileName(GetTempPath(), "symbolpicker_svg_dup_smoke");
	DeleteFolderDeep(dup_temp_dir);
	if(!EnsureDirectoryPath(dup_temp_dir)) {
		error = "SVG duplicate-folder smoke could not create its temp folder.";
		return false;
	}

	SymbolPickerProject dup_project;
	dup_project.project_name = "SVG Duplicate Folders";
	dup_project.symbol_prefix = "ICON_MYAPP_";
	dup_project.default_size = 32;
	dup_project.active_collection_index = 0;

	SymbolPickerCollection dup_a;
	dup_a.name = "Toolbar";
	SymbolPickerIconRef dup_a_item;
	dup_a_item.catalog_id = "action/save/outlined";
	dup_a_item.source_id = "action/save";
	dup_a_item.alias = "Toolbar Save";
	dup_a_item.unresolved = false;
	dup_a.items.Add(dup_a_item);
	dup_project.collections.Add(pick(dup_a));

	SymbolPickerCollection dup_b;
	dup_b.name = "Toolbar";
	SymbolPickerIconRef dup_b_item;
	dup_b_item.catalog_id = "content/content_copy/outlined";
	dup_b_item.source_id = "content/content_copy";
	dup_b_item.alias = "Toolbar Copy";
	dup_b_item.unresolved = false;
	dup_b.items.Add(dup_b_item);
	dup_project.collections.Add(pick(dup_b));

	Vector<String> dup_warnings;
	int dup_written = 0;
	int dup_skipped = 0;
	if(!ExportSymbolPickerSvgFiles(dup_project, catalog, SymbolPickerExportScope::AllCollections, dup_temp_dir, &dup_warnings, &dup_written, &dup_skipped)) {
		DeleteFolderDeep(dup_temp_dir);
		error = "SVG duplicate-folder smoke export failed.";
		return false;
	}
	if(dup_written != 2) {
		DeleteFolderDeep(dup_temp_dir);
		error = "SVG duplicate-folder smoke did not write both SVG files.";
		return false;
	}
	if(!FileExists(AppendFileName(dup_temp_dir, "Toolbar\\ICON_MYAPP_TOOLBAR_SAVE.svg"))
		|| !FileExists(AppendFileName(dup_temp_dir, "Toolbar_2\\ICON_MYAPP_TOOLBAR_COPY.svg"))) {
		DeleteFolderDeep(dup_temp_dir);
		error = "SVG duplicate-folder smoke did not create unique folders.";
		return false;
	}
	DeleteFolderDeep(dup_temp_dir);

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

	String svg_temp_dir = AppendFileName(GetTempPath(), "symbolpicker_svg_smoke");
	DeleteFolderDeep(svg_temp_dir);
	if(!EnsureDirectoryPath(svg_temp_dir)) {
		error = "SVG smoke could not create its temp folder.";
		return false;
	}

	SymbolPickerProject svg_project;
	svg_project.project_name = "SVG Smoke";
	svg_project.symbol_prefix = "ICON_MYAPP_";
	svg_project.default_size = 32;
	svg_project.active_collection_index = 0;

	SymbolPickerCollection svg_collection;
	svg_collection.name = "SVG Collection";

	SymbolPickerIconRef svga;
	svga.catalog_id = "action/save/outlined";
	svga.source_id = "action/save";
	svga.alias = "Svg One";
	svga.size = 16;
	svga.tint = Null;
	svg_collection.items.Add(svga);

	SymbolPickerIconRef svgb;
	svgb.catalog_id = "content/content_copy/outlined";
	svgb.source_id = "content/content_copy";
	svgb.alias = "Tinted Svg";
	svgb.size = 48;
	svgb.tint = Color(255, 0, 0);
	svg_collection.items.Add(svgb);

	SymbolPickerIconRef svgc;
	svgc.catalog_id = "legacy/missing_icon/outlined";
	svgc.source_id = "legacy/missing_icon";
	svgc.alias = "Broken";
	svgc.unresolved = true;
	svg_collection.items.Add(svgc);

	svg_project.collections.Add(pick(svg_collection));

	Vector<String> svg_warnings;
	int written = 0;
	int skipped = 0;
	if(!ExportSymbolPickerSvgFiles(svg_project, catalog, SymbolPickerExportScope::ActiveCollection, svg_temp_dir, &svg_warnings, &written, &skipped)) {
		DeleteFolderDeep(svg_temp_dir);
		error = "SVG smoke export failed.";
		return false;
	}
	String svg_one = AppendFileName(svg_temp_dir, "ICON_MYAPP_SVG_ONE.svg");
	String svg_two = AppendFileName(svg_temp_dir, "ICON_MYAPP_TINTED_SVG.svg");
	if(written != 2 || skipped == 0 || !FileExists(svg_one) || !FileExists(svg_two)) {
		DeleteFolderDeep(svg_temp_dir);
		error = "SVG smoke did not write the expected files.";
		return false;
	}
	String svg_one_text = LoadFile(svg_one);
	String svg_two_text = LoadFile(svg_two);
	if(svg_one_text.Find("<svg") < 0 || svg_one_text.Find("width=\"16\"") < 0 || svg_one_text.Find("height=\"16\"") < 0) {
		DeleteFolderDeep(svg_temp_dir);
		error = "SVG smoke did not normalize SVG size.";
		return false;
	}
	if(svg_two_text.Find("<svg") < 0 || svg_two_text.Find("#FF0000") < 0) {
		DeleteFolderDeep(svg_temp_dir);
		error = "SVG smoke did not apply tint.";
		return false;
	}
	if(svg_warnings.IsEmpty() || !FileExists(AppendFileName(svg_temp_dir, "_export_warnings.txt"))) {
		DeleteFolderDeep(svg_temp_dir);
		error = "SVG smoke did not emit warnings.";
		return false;
	}

	DeleteFolderDeep(svg_temp_dir);

	return true;
}

bool RunSymbolPickerSvgExportSmokeTests(const SymbolPickerCatalog& catalog, String& error)
{
	return RunSymbolPickerExportSmokeTests(catalog, error);
}

}
