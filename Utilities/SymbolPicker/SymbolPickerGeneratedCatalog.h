#ifndef _Utilities_SymbolPicker_SymbolPickerGeneratedCatalog_h_
#define _Utilities_SymbolPicker_SymbolPickerGeneratedCatalog_h_

#include "SymbolPickerCatalog.h"

namespace Upp {

int LoadGeneratedSymbolPickerCatalog(SymbolPickerCatalog& catalog);
bool DecodeGeneratedSymbolPickerSvg(const String& catalog_id, String& svg_xml);
bool RunSymbolPickerGeneratedCatalogSmokeTests(String& error);

}

#endif
