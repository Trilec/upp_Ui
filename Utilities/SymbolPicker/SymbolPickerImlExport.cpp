#include "SymbolPickerImlExport.h"
#include "SymbolPickerImageRender.h"
#include "../IconExportCore/IconExportCore.h"

namespace Upp {

static String SafeCommentLines(const String& text)
{
	String norm = text;
	norm.Replace("\r\n", "\n");
	norm.Replace("\r", "\n");
	String out;
	int start = 0;
	for(;;) {
		int end = norm.Find('\n', start);
		String line = end >= 0 ? norm.Mid(start, end - start) : norm.Mid(start);
		String safe;
		for(int i = 0; i < line.GetCount(); ++i) {
			byte c = (byte)line[i];
			if(c == '\t' || c < 32 || c == 127)
				safe.Cat(' ');
			else
				safe.Cat((char)c);
		}
		if(!safe.IsEmpty() && safe[safe.GetCount() - 1] == '\\')
			safe << " [backslash]";
		out << "// " << safe << '\n';
		if(end < 0)
			break;
		start = end + 1;
	}
	return out;
}

static String WarningBlock(const Vector<String>& warnings)
{
	if(warnings.IsEmpty())
		return String();
	String out = "// Export warnings:\n";
	for(const String& warning : warnings)
		out << SafeCommentLines(warning);
	return out + '\n';
}

String BuildSymbolPickerUppIml(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings)
{
	Vector<String> local;
	Vector<String>& warn = warnings ? *warnings : local;
	Vector<SymbolPickerExportItem> items = BuildSymbolPickerExportItems(project, catalog, scope, &warn);
	Index<String> used;
	String body;
	int emitted = 0;
	for(const SymbolPickerExportItem& item : items) {
		const SymbolPickerIconEntry* entry = catalog.FindByCatalogId(item.catalog_id);
		if(!entry || item.unresolved) {
			warn.Add(Format("Skipped unresolved IML item %s.", item.catalog_id));
			continue;
		}
		int size = item.size > 0 ? item.size : project.default_size;
		Image image = RenderSymbolPickerIconImage(*entry, size, item.tint);
		if(image.IsEmpty()) {
			warn.Add(Format("Could not render IML item %s.", item.catalog_id));
			continue;
		}
		String payload, codec_error;
		if(!BuildUppImlPayload(image, payload, &codec_error)) {
			warn.Add(Format("Could not encode IML item %s: %s", item.catalog_id, codec_error));
			continue;
		}
		SymbolPickerIconRef naming_ref;
		naming_ref.alias = item.alias;
		naming_ref.category_override = item.category;
		String token = MakeSymbolPickerExportSymbolName(project, SymbolPickerCollection(), naming_ref, entry, used);
		String entry_text;
		if(!BuildUppImlEntryText(token, payload, item.catalog_id, image.GetSize(), entry_text, &codec_error)) {
			warn.Add(Format("Could not format IML item %s: %s", item.catalog_id, codec_error));
			continue;
		}
		body << SafeCommentLines("Collection: " + item.category);
		body << SafeCommentLines("Display: " + item.display_name);
		body << SafeCommentLines("Source: " + item.source_id);
		body << SafeCommentLines(item.comment);
		body << entry_text;
		++emitted;
	}
	if(!emitted) {
		warn.Add("No valid icons were emitted for the IML export.");
		return String();
	}
	return WarningBlock(warn) + "PREMULTIPLIED\n\n" + body;
}

bool RunSymbolPickerImlExportSmokeTests(const SymbolPickerCatalog& catalog, String& error)
{
	SymbolPickerProject project;
	project.project_name = "IML smoke";
	project.output_base_name = "iml_smoke";
	project.symbol_prefix = "ICON_IML_";
	SymbolPickerCollection collection;
	collection.name = "Smoke";
	const SymbolPickerIconEntry* entry = nullptr;
	for(const SymbolPickerIconEntry& candidate : catalog.GetIcons()) {
		if(candidate.available && !candidate.catalog_id.IsEmpty()) {
			entry = &candidate;
			break;
		}
	}
	if(!entry) {
		error = "IML smoke could not find an available catalog entry.";
		return false;
	}
	SymbolPickerIconRef item;
	item.catalog_id = entry->catalog_id;
	item.source_id = entry->source_id;
	item.alias = "Smoke icon";
	item.size = 24;
	collection.items.Add(item);
	project.collections.Add(pick(collection));
	project.active_collection_index = 0;
	Vector<String> warnings;
	String first = BuildSymbolPickerUppIml(project, catalog, SymbolPickerExportScope::ActiveCollection, &warnings);
	String second = BuildSymbolPickerUppIml(project, catalog, SymbolPickerExportScope::ActiveCollection);
	if(first.IsEmpty() || first != second || first.Find("PREMULTIPLIED") < 0 || first.Find("IMAGE_ID(") < 0) {
		error = "IML smoke output was empty, non-deterministic, or incomplete.";
		return false;
	}
	SymbolPickerProject empty;
	empty.project_name = project.project_name;
	empty.output_base_name = project.output_base_name;
	empty.symbol_prefix = project.symbol_prefix;
	SymbolPickerCollection empty_collection;
	empty_collection.name = "Empty";
	empty.collections.Add(pick(empty_collection));
	empty.active_collection_index = 0;
	if(!BuildSymbolPickerUppIml(empty, catalog, SymbolPickerExportScope::ActiveCollection, &warnings).IsEmpty()) {
		error = "IML smoke accepted a zero-output export.";
		return false;
	}
	return true;
}

}
