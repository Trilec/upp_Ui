#include "SymbolPickerCatalogSeed.h"

namespace Upp {

static const char* CatalogStyleSuffix(SymbolPickerIconStyle style)
{
	switch(style) {
	case SymbolPickerIconStyle::Outlined: return "outlined";
	case SymbolPickerIconStyle::Rounded:  return "rounded";
	case SymbolPickerIconStyle::Sharp:    return "sharp";
	}
	return "outlined";
}

static void AddSeed(SymbolPickerCatalog& catalog,
	const char* category,
	const char* name,
	SymbolPickerIconStyle style,
	const String& symbol)
{
	SymbolPickerIconEntry e;
	e.category = category;
	e.display_name = name;
	e.source_id = String(category) + "/" + name;
	e.catalog_id = e.source_id + "/" + CatalogStyleSuffix(style);
	e.style = style;
	e.source_symbol = symbol;
	catalog.Add(e);
}

void SeedSymbolPickerCatalog(SymbolPickerCatalog& catalog)
{
	catalog.Clear();

	static const struct SeedRow {
		const char* category;
		const char* name;
	} rows[] = {
		{"action", "save"},
		{"action", "delete"},
		{"action", "search"},
		{"navigation", "menu"},
		{"navigation", "close"},
		{"alert", "warning"},
		{"image", "photo"},
		{"content", "copy"},
	};

	for(const auto& row : rows) {
		AddSeed(catalog, row.category, row.name, SymbolPickerIconStyle::Outlined, Format("%s_%s_outlined", row.category, row.name));
		AddSeed(catalog, row.category, row.name, SymbolPickerIconStyle::Rounded,  Format("%s_%s_rounded", row.category, row.name));
		AddSeed(catalog, row.category, row.name, SymbolPickerIconStyle::Sharp,    Format("%s_%s_sharp", row.category, row.name));
	}
	
	// Keep the initial catalog intentionally read-only and seed-based until the
	// generated registration loader replaces this file in a later pass.
}

}
