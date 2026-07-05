#ifndef _Utilities_SymbolPicker_SymbolPickerUppExport_h_
#define _Utilities_SymbolPicker_SymbolPickerUppExport_h_

#include "SymbolPickerExport.h"

namespace Upp {

String BuildSymbolPickerUppRawHeader(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings = nullptr);

String BuildSymbolPickerUppRleHeader(const SymbolPickerProject& project,
	const SymbolPickerCatalog& catalog,
	SymbolPickerExportScope scope,
	Vector<String>* warnings = nullptr);

bool RunSymbolPickerUppExportSmokeTests(String& error);

}

#endif
