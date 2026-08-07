#include "SymbolPickerImlExport.h"
#include "SymbolPickerImageRender.h"
#include "../IconExportCore/IconExportCore.h"

namespace Upp {

struct SymbolPickerImlEmission : Moveable<SymbolPickerImlEmission> {
	String token;
	String category;
	String display_name;
	String catalog_id;
	String source_id;
	String comment;
	String payload;
	Size   image_size;
};

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

static String EscapeCppString(const String& text)
{
	String out;
	for(int i = 0; i < text.GetCount(); ++i) {
		char c = text[i];
		switch(c) {
		case '\\': out << "\\\\"; break;
		case '"':  out << "\\\""; break;
		case '\n': out << "\\n"; break;
		case '\r': out << "\\r"; break;
		case '\t': out << "\\t"; break;
		default:
			if((byte)c >= 32 && (byte)c != 127)
				out.Cat(c);
			else
				out.Cat(' ');
			break;
		}
	}
	return out;
}

static Vector<SymbolPickerImlEmission> BuildImlEmissions(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>& warnings)
{
	Vector<SymbolPickerExportItem> items = BuildSymbolPickerExportItems(project, catalog, scope, &warnings);
	Vector<SymbolPickerImlEmission> out;
	Index<String> used;
	for(const SymbolPickerExportItem& item : items) {
		const SymbolPickerIconEntry* entry = catalog.FindByCatalogId(item.catalog_id);
		if(!entry || item.unresolved) {
			warnings.Add(Format("Skipped unresolved IML item %s.", item.catalog_id));
			continue;
		}
		int size = item.size > 0 ? item.size : project.default_size;
		Image image = RenderSymbolPickerIconImage(*entry, size, item.tint);
		if(image.IsEmpty()) {
			warnings.Add(Format("Could not render IML item %s.", item.catalog_id));
			continue;
		}
		String payload, codec_error;
		if(!BuildUppImlPayload(image, payload, &codec_error)) {
			warnings.Add(Format("Could not encode IML item %s: %s", item.catalog_id, codec_error));
			continue;
		}
		SymbolPickerIconRef naming_ref;
		naming_ref.alias = item.alias;
		naming_ref.category_override = item.category;
		SymbolPickerImlEmission& emission = out.Add();
		emission.token = MakeSymbolPickerExportSymbolName(project, SymbolPickerCollection(), naming_ref, entry, used);
		emission.category = item.category;
		emission.display_name = item.display_name;
		emission.catalog_id = item.catalog_id;
		emission.source_id = item.source_id;
		emission.comment = item.comment;
		emission.payload = payload;
		emission.image_size = image.GetSize();
	}
	if(out.IsEmpty())
		warnings.Add("No valid icons were emitted for the IML export.");
	return out;
}

static String BuildImlText(const Vector<SymbolPickerImlEmission>& emissions, const Vector<String>& warnings)
{
	if(emissions.IsEmpty())
		return String();
	String body;
	for(const SymbolPickerImlEmission& emission : emissions) {
		String entry_text, codec_error;
		if(!BuildUppImlEntryText(emission.token, emission.payload, emission.catalog_id, emission.image_size, entry_text, &codec_error))
			return String();
		body << SafeCommentLines("Collection: " + emission.category);
		body << SafeCommentLines("Display: " + emission.display_name);
		body << SafeCommentLines("Source: " + emission.source_id);
		body << SafeCommentLines(emission.comment);
		body << entry_text;
	}
	return WarningBlock(warnings) + "PREMULTIPLIED\n\n" + body;
}

static String BuildImlLibraryHeaderText(const Vector<SymbolPickerImlEmission>& emissions,
	const String& iml_file_name)
{
	if(emissions.IsEmpty())
		return String();

	String base = MakeSymbolPickerSafeCppIdentifierSegment(GetFileTitle(iml_file_name));
	if(base.IsEmpty() || base == "_")
		base = "SYMBOLS";
	String image_class = base + "Img";
	String catalog_type = base + "CatalogEntry";
	String catalog_fn = "Get" + base + "Catalog";
	String categories_fn = "Get" + base + "Categories";
	String guard = "_SYMBOLPICKER_GENERATED_" + base + "_h_";

	Index<String> categories;
	for(const SymbolPickerImlEmission& emission : emissions)
		categories.FindAdd(emission.category);

	String out;
	out << "#ifndef " << guard << "\n#define " << guard << "\n\n";
	out << "#include <CtrlCore/CtrlCore.h>\n\nnamespace Upp {\n\n";
	out << "#define IMAGECLASS " << image_class << "\n";
	out << "#define IMAGEFILE \"" << EscapeCppString(GetFileName(iml_file_name)) << "\"\n";
	out << "#include <Draw/iml_header.h>\n\n";
	for(const SymbolPickerImlEmission& emission : emissions)
		out << "inline Image " << emission.token << "() { return " << image_class << "::" << emission.token << "(); }\n";
	out << "\nstruct " << catalog_type << " {\n"
	       "\tconst char* category;\n"
	       "\tconst char* display_name;\n"
	       "\tconst char* symbol_name;\n"
	       "\tImage (*factory)();\n"
	       "};\n\n";
	out << "inline const " << catalog_type << "* " << catalog_fn << "(int& count)\n{\n"
	       "\tstatic const " << catalog_type << " catalog[] = {\n";
	for(const SymbolPickerImlEmission& emission : emissions) {
		out << "\t\t{ \"" << EscapeCppString(emission.category) << "\", \""
		    << EscapeCppString(emission.display_name) << "\", \""
		    << EscapeCppString(emission.token) << "\", &" << emission.token << " },\n";
	}
	out << "\t};\n"
	       "\tcount = (int)(sizeof(catalog) / sizeof(catalog[0]));\n"
	       "\treturn catalog;\n"
	       "}\n\n";
	out << "inline const char* const* " << categories_fn << "(int& count)\n{\n"
	       "\tstatic const char* categories[] = {\n";
	for(int i = 0; i < categories.GetCount(); ++i)
		out << "\t\t\"" << EscapeCppString(categories[i]) << "\",\n";
	out << "\t};\n"
	       "\tcount = (int)(sizeof(categories) / sizeof(categories[0]));\n"
	       "\treturn categories;\n"
	       "}\n\n"
	       "}\n\n#endif\n";
	return out;
}

String BuildSymbolPickerUppIml(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings)
{
	Vector<String> local;
	Vector<String>& warn = warnings ? *warnings : local;
	Vector<SymbolPickerImlEmission> emissions = BuildImlEmissions(project, catalog, scope, warn);
	return BuildImlText(emissions, warn);
}

String BuildSymbolPickerUppImlLibraryHeader(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	const String& iml_file_name,
	Vector<String>* warnings)
{
	Vector<String> local;
	Vector<String>& warn = warnings ? *warnings : local;
	Vector<SymbolPickerImlEmission> emissions = BuildImlEmissions(project, catalog, scope, warn);
	return BuildImlLibraryHeaderText(emissions, iml_file_name);
}

bool BuildSymbolPickerUppImlLibrary(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	const String& iml_file_name,
	String& iml_text,
	String& header_text,
	Vector<String>* warnings)
{
	Vector<String> local;
	Vector<String>& warn = warnings ? *warnings : local;
	Vector<SymbolPickerImlEmission> emissions = BuildImlEmissions(project, catalog, scope, warn);
	iml_text = BuildImlText(emissions, warn);
	header_text = BuildImlLibraryHeaderText(emissions, iml_file_name);
	return !iml_text.IsEmpty() && !header_text.IsEmpty();
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

	String library_iml, library_header;
	warnings.Clear();
	if(!BuildSymbolPickerUppImlLibrary(project, catalog, SymbolPickerExportScope::ActiveCollection,
		"UiIcons.iml", library_iml, library_header, &warnings)) {
		error = "IML library smoke could not build the paired output.";
		return false;
	}
	if(library_iml != first || library_header.Find("#define IMAGECLASS UIICONSImg") < 0
	|| library_header.Find("#define IMAGEFILE \"UiIcons.iml\"") < 0
	|| library_header.Find("CatalogEntry") < 0
	|| library_header.Find("Categories") < 0
	|| library_header.Find("inline Image ICON_IML_SMOKE_ICON()") < 0) {
		error = "IML library smoke output was inconsistent or incomplete.";
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
