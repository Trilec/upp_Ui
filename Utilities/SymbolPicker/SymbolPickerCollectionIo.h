#ifndef _Utilities_SymbolPicker_SymbolPickerCollectionIo_h_
#define _Utilities_SymbolPicker_SymbolPickerCollectionIo_h_

#include "SymbolPickerModel.h"

namespace Upp {

// Intended formats:
// - .uppicons.json is the primary editable source format.
// - generated .h is a derived output artifact.
// - generated .h may include a metadata block for provenance.
// - generated .h is not the primary editable source.
// - JSON remains the source of truth for v1.
// - generated header metadata is only for provenance/recovery.
// - parsing arbitrary edited C++ is not part of v1.

bool SaveSymbolPickerCollectionJsonStub(const SymbolPickerCollection& collection);
bool LoadSymbolPickerCollectionJsonStub(const String& path, SymbolPickerCollection& out);
String BuildSymbolPickerGeneratedHeaderStub(const SymbolPickerCollection& collection);

}

#endif
