#ifndef _Utilities_SymbolPicker_SymbolPickerImlExport_h_
#define _Utilities_SymbolPicker_SymbolPickerImlExport_h_

#include "SymbolPickerExport.h"

namespace Upp {

String BuildSymbolPickerUppIml(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings = nullptr);

String BuildSymbolPickerUppImlLibraryHeader(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	const String& iml_file_name,
	Vector<String>* warnings = nullptr);

bool BuildSymbolPickerUppImlLibrary(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	const String& iml_file_name,
	String& iml_text,
	String& header_text,
	Vector<String>* warnings = nullptr);

bool RunSymbolPickerImlExportSmokeTests(const SymbolPickerCatalog& catalog, String& error);

}

#endif
