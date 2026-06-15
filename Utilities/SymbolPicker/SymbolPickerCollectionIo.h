#ifndef _Utilities_SymbolPicker_SymbolPickerCollectionIo_h_
#define _Utilities_SymbolPicker_SymbolPickerCollectionIo_h_

#include "SymbolPickerModel.h"

namespace Upp {

// Intended formats:
// - .uppicons.json is the primary editable source format.
// - generated .h is a derived output artifact.
// - generated .h may include a metadata block for provenance.
// - generated .h is not the primary editable source.

bool SaveSymbolPickerCollectionJsonStub(const SymbolPickerCollection& collection);
bool LoadSymbolPickerCollectionJsonStub(const String& path, SymbolPickerCollection& out);
String BuildSymbolPickerGeneratedHeaderStub(const SymbolPickerCollection& collection);

}

#endif
