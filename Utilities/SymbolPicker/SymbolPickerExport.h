#ifndef _Utilities_SymbolPicker_SymbolPickerExport_h_
#define _Utilities_SymbolPicker_SymbolPickerExport_h_

#include "SymbolPickerCatalog.h"

namespace Upp {

struct SymbolPickerExportItem : Moveable<SymbolPickerExportItem> {
	String                symbol_name;
	String                category;
	String                display_name;
	String                catalog_id;
	String                source_id;
	String                alias;
	String                comment;
	int                   size = 0;
	Color                 tint = Null;
	SymbolPickerIconStyle style = SymbolPickerIconStyle::Outlined;
	bool                  unresolved = false;
};

struct SymbolPickerExportCategory : Moveable<SymbolPickerExportCategory> {
	String name;
	Vector<SymbolPickerExportItem> items;
};

enum class SymbolPickerExportScope : byte {
	ActiveCollection,
	AllCollections,
};

String MakeSymbolPickerSafeCppIdentifierSegment(const String& text);
String MakeSymbolPickerSafeIconAlias(const String& text);
String MakeSymbolPickerExportDisplayName(const SymbolPickerProject& project,
	const SymbolPickerCollection& collection,
	const SymbolPickerIconRef& item,
	const SymbolPickerIconEntry* catalog_entry);
String ResolveExportCategory(const SymbolPickerProject& project,
	const SymbolPickerCollection& collection,
	const SymbolPickerIconRef& item,
	const SymbolPickerIconEntry* catalog_entry);
String MakeSymbolPickerExportSymbolName(const SymbolPickerProject& project,
	const SymbolPickerCollection& collection,
	const SymbolPickerIconRef& item,
	const SymbolPickerIconEntry* catalog_entry,
	Index<String>& used_names);
Vector<SymbolPickerExportItem> BuildSymbolPickerExportItems(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope = SymbolPickerExportScope::ActiveCollection,
	Vector<String>* warnings = nullptr);
Vector<SymbolPickerExportCategory> BuildSymbolPickerExportCategories(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope = SymbolPickerExportScope::ActiveCollection,
	Vector<String>* warnings = nullptr);
String BuildIconIdExport(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope = SymbolPickerExportScope::ActiveCollection,
	Vector<String>* warnings = nullptr);
String BuildImageCallExport(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope = SymbolPickerExportScope::ActiveCollection,
	Vector<String>* warnings = nullptr);
String BuildCppSnippetExport(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope = SymbolPickerExportScope::ActiveCollection,
	Vector<String>* warnings = nullptr);
String BuildCategoryListExport(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope = SymbolPickerExportScope::ActiveCollection,
	Vector<String>* warnings = nullptr);
bool RunSymbolPickerExportSmokeTests(const SymbolPickerCatalog& catalog, String& error);

}

#endif
