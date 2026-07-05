#include <CtrlLib/CtrlLib.h>

#include "../SymbolPickerExport.h"
#include "../SymbolPickerGeneratedCatalog.h"
#include "../SymbolPickerUppExport.h"

#include "GeneratedRawHeader.h"
#include "GeneratedRleHeader.h"

using namespace Upp;

static bool WriteFixtureHeaders(String& error)
{
	SymbolPickerCatalog catalog;
	if(!LoadGeneratedSymbolPickerCatalog(catalog)) {
		error = "Could not load generated catalog.";
		return false;
	}

	SymbolPickerProject project;
	project.project_name = "SmokeHeaderCompile";
	project.output_base_name = "SmokeHeaderCompile";
	project.symbol_prefix = "ICON_SMOKE_";
	project.default_size = 24;
	project.default_tint = Null;
	project.default_style = SymbolPickerIconStyle::Outlined;
	project.active_collection_index = 0;

	SymbolPickerCollection collection;
	collection.name = "Smoke";

	SymbolPickerIconRef raw_item;
	raw_item.catalog_id = "action/save/outlined";
	raw_item.source_id = "action/save";
	raw_item.alias = "Save";
	raw_item.size = 24;
	raw_item.tint = Null;
	collection.items.Add(raw_item);

	SymbolPickerIconRef rle_item;
	rle_item.catalog_id = "content/content_copy/outlined";
	rle_item.source_id = "content/content_copy";
	rle_item.alias = "Copy";
	rle_item.size = 24;
	rle_item.tint = Color(32, 64, 128);
	collection.items.Add(rle_item);

	project.collections.Add(pick(collection));

	Vector<String> raw_warnings;
	Vector<String> rle_warnings;
	String raw = BuildSymbolPickerUppRawHeader(project, catalog, SymbolPickerExportScope::ActiveCollection, &raw_warnings);
	String rle = BuildSymbolPickerUppRleHeader(project, catalog, SymbolPickerExportScope::ActiveCollection, &rle_warnings);
	if(raw.IsEmpty() || rle.IsEmpty()) {
		error = "Header builders returned empty text.";
		return false;
	}
	if(!SaveFile("GeneratedRawHeader.h", raw)) {
		error = "Could not write GeneratedRawHeader.h.";
		return false;
	}
	if(!SaveFile("GeneratedRleHeader.h", rle)) {
		error = "Could not write GeneratedRleHeader.h.";
		return false;
	}
	return true;
}

GUI_APP_MAIN
{
	if(FindIndex(CommandLine(), "--regen-fixtures") >= 0) {
		String error;
		if(!WriteFixtureHeaders(error))
			Exclamation(error);
		return;
	}

	Image raw = ICON_SMOKE_RAW_SAVE();
	Image rle = ICON_SMOKE_RLE_SAVE();
	if(raw.IsEmpty() || rle.IsEmpty())
		Exclamation("Smoke headers returned empty images.");
}
