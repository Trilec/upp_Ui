#ifndef _Utilities_SymbolPicker_SymbolPickerProjectIo_h_
#define _Utilities_SymbolPicker_SymbolPickerProjectIo_h_

#include "SymbolPickerCatalog.h"
#include "SymbolPickerModel.h"

namespace Upp {

bool SaveSymbolPickerProjectJson(const SymbolPickerProject& project, const String& path, String& error);
bool LoadSymbolPickerProjectJson(const String& path, SymbolPickerProject& out, String& error);
bool RunSymbolPickerProjectIoSmokeTests(const SymbolPickerCatalog& catalog, String& error);

}

#endif
