#include <Core/Core.h>

#include "../SymbolPickerModel.h"
#include "../SymbolPickerCatalog.h"
#include "../SymbolPickerGeneratedCatalog.h"
#include "../SymbolPickerExport.h"
#include "../SymbolPickerUppExport.h"
#include "../SymbolPickerImageRender.h"

using namespace Upp;

static bool IsFlag(const String& arg, const char* flag)
{
	return ToLower(TrimBoth(arg)) == flag;
}

static String MakeProjectHeaderPath(const String& folder, const char* name)
{
	return AppendFileName(folder, name);
}

static bool HasVisiblePixels(const Image& img)
{
	Size sz = img.GetSize();
	for(int y = 0; y < sz.cy; ++y) {
		const RGBA* row = img[y];
		for(int x = 0; x < sz.cx; ++x) {
			if(row[x].a > 0)
				return true;
		}
	}
	return false;
}

static const SymbolPickerIconEntry* PickRenderableIcon(const SymbolPickerCatalog& catalog, int preferred_index, int pixel_size, Color tint)
{
	const Vector<SymbolPickerIconEntry>& icons = catalog.GetIcons();
	if(icons.IsEmpty())
		return nullptr;
	for(int i = 0; i < icons.GetCount(); ++i) {
		int idx = (preferred_index + i) % icons.GetCount();
		if(!icons[idx].available)
			continue;
		String error;
		Image img = RenderSymbolPickerIconImage(icons[idx], pixel_size, tint, &error);
		if(img.IsEmpty() || !HasVisiblePixels(img))
			continue;
		return &icons[idx];
	}
	return nullptr;
}

static SymbolPickerProject MakeRawProject(const SymbolPickerCatalog& catalog)
{
	const SymbolPickerIconEntry* entry = PickRenderableIcon(catalog, 0, 24, Null);
	if(!entry)
		return SymbolPickerProject();

	SymbolPickerProject project;
	project.project_name = "SmokeHeaderCompileRaw";
	project.output_base_name = "SmokeHeaderCompileRaw";
	project.symbol_prefix = "ICON_SMOKE_RAW_";
	project.default_size = 24;
	project.default_tint = Null;
	project.default_style = SymbolPickerIconStyle::Outlined;
	project.active_collection_index = 0;
	project.comment = "line1\nline2\nint injected = 1;\\";

	SymbolPickerCollection collection;
	collection.name = "Raw\nline2\tint injected = 1;\\";
	collection.comment = "collection line1\ncollection line2\\";

	SymbolPickerIconRef item;
	item.catalog_id = entry->catalog_id;
	item.source_id = entry->source_id;
	item.alias = "Save";
	item.size = 24;
	item.tint = Null;
	item.comment = "item line1\nitem line2\tint injected = 1;\\";
	item.unresolved = false;
	collection.items.Add(item);

	project.collections.Add(pick(collection));
	return project;
}

static SymbolPickerProject MakeRleProject(const SymbolPickerCatalog& catalog)
{
	const SymbolPickerIconEntry* entry = PickRenderableIcon(catalog, 1, 24, Color(32, 64, 128));
	if(!entry)
		entry = PickRenderableIcon(catalog, 0, 24, Color(32, 64, 128));
	if(!entry)
		return SymbolPickerProject();

	SymbolPickerProject project;
	project.project_name = "SmokeHeaderCompileRle";
	project.output_base_name = "SmokeHeaderCompileRle";
	project.symbol_prefix = "ICON_SMOKE_RLE_";
	project.default_size = 24;
	project.default_tint = Null;
	project.default_style = SymbolPickerIconStyle::Outlined;
	project.active_collection_index = 0;
	project.comment = "line1\nline2\nint injected = 1;\\";

	SymbolPickerCollection collection;
	collection.name = "Rle\nline2\tint injected = 1;\\";
	collection.comment = "collection line1\ncollection line2\\";

	SymbolPickerIconRef item;
	item.catalog_id = entry->catalog_id;
	item.source_id = entry->source_id;
	item.alias = "Copy";
	item.size = 24;
	item.tint = Color(32, 64, 128);
	item.comment = "item line1\nitem line2\tint injected = 1;\\";
	item.unresolved = false;
	collection.items.Add(item);

	project.collections.Add(pick(collection));
	return project;
}

static bool VerifyTextFile(const String& path, const String& expected, String& error)
{
	String actual = LoadFile(path);
	if(actual.IsEmpty() && !FileExists(path)) {
		error = Format("Missing committed fixture '%s'.", path);
		return false;
	}
	if(actual != expected) {
		error = Format("Fixture '%s' does not match the generated builder output.", path);
		return false;
	}
	return true;
}

static bool BuildFixtures(const String& output_folder, bool verify, String& error)
{
	SymbolPickerCatalog catalog;
	if(!LoadGeneratedSymbolPickerCatalog(catalog)) {
		error = "Could not load generated catalog.";
		return false;
	}

	SymbolPickerProject raw_project = MakeRawProject(catalog);
	SymbolPickerProject rle_project = MakeRleProject(catalog);
	if(raw_project.collections.IsEmpty() || rle_project.collections.IsEmpty()) {
		error = "Could not select valid fixture catalog entries.";
		return false;
	}

	Vector<String> raw_warnings;
	Vector<String> rle_warnings;
	String raw_text = BuildSymbolPickerUppRawHeader(raw_project, catalog, SymbolPickerExportScope::ActiveCollection, &raw_warnings);
	String rle_text = BuildSymbolPickerUppRleHeader(rle_project, catalog, SymbolPickerExportScope::ActiveCollection, &rle_warnings);
	if(raw_text.IsEmpty() || rle_text.IsEmpty()) {
		if(!raw_warnings.IsEmpty()) {
			Cout() << "RAW warnings:\n";
			for(const String& warning : raw_warnings)
				Cout() << warning << '\n';
		}
		if(!rle_warnings.IsEmpty()) {
			Cout() << "RLE warnings:\n";
			for(const String& warning : rle_warnings)
				Cout() << warning << '\n';
		}
		error = "Header builders returned empty output.";
		return false;
	}
	if(!raw_warnings.IsEmpty() || !rle_warnings.IsEmpty()) {
		error = "Header builders produced unexpected warnings for the fixture projects.";
		return false;
	}

	String raw_path = MakeProjectHeaderPath(output_folder, "GeneratedRawHeader.h");
	String rle_path = MakeProjectHeaderPath(output_folder, "GeneratedRleHeader.h");

	if(verify) {
		if(!VerifyTextFile(raw_path, raw_text, error))
			return false;
		if(!VerifyTextFile(rle_path, rle_text, error))
			return false;
		return true;
	}

	if(!DirectoryExists(output_folder) && !DirectoryCreate(output_folder)) {
		error = Format("Could not create output folder '%s'.", output_folder);
		return false;
	}
	if(!SaveFile(raw_path, raw_text)) {
		error = Format("Could not write '%s'.", raw_path);
		return false;
	}
	if(!SaveFile(rle_path, rle_text)) {
		error = Format("Could not write '%s'.", rle_path);
		return false;
	}
	return true;
}

CONSOLE_APP_MAIN
{
	String output_folder;
	bool verify = false;
	bool smoke = false;
	for(const String& arg : CommandLine()) {
		String t = TrimBoth(arg);
		if(t.IsEmpty())
			continue;
		if(IsFlag(t, "--verify")) {
			verify = true;
			continue;
		}
		if(IsFlag(t, "--smoke")) {
			smoke = true;
			continue;
		}
		if(output_folder.IsEmpty()) {
			output_folder = t;
			continue;
		}
	}

	SymbolPickerCatalog catalog;
	if(!LoadGeneratedSymbolPickerCatalog(catalog)) {
		Cout() << "Could not load generated catalog.\n";
		SetExitCode(1);
		return;
	}

	if(smoke) {
		String error;
		if(!RunSymbolPickerExportSmokeTests(catalog, error)) {
			Cout() << error << '\n';
			SetExitCode(1);
			return;
		}
		Cout() << "Smoke OK\n";
		return;
	}

	if(output_folder.IsEmpty()) {
		Cout() << "Usage: SmokeHeaderGenerate <output-folder> [--verify]\n";
		SetExitCode(1);
		return;
	}

	String error;
	if(!BuildFixtures(output_folder, verify, error)) {
		Cout() << error << '\n';
		SetExitCode(1);
		return;
	}

	Cout() << (verify ? "Verified" : "Generated") << " fixture headers in " << output_folder << '\n';
}
