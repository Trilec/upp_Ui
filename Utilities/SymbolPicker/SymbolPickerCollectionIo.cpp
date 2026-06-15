#include "SymbolPickerCollectionIo.h"

namespace Upp {

bool SaveSymbolPickerCollectionJsonStub(const SymbolPickerCollection&)
{
	// Stub only for the v0.2 foundation pass.
	return false;
}

bool LoadSymbolPickerCollectionJsonStub(const String&, SymbolPickerCollection&)
{
	// Stub only for the v0.2 foundation pass.
	return false;
}

String BuildSymbolPickerGeneratedHeaderStub(const SymbolPickerCollection&)
{
	// Stub only for the v0.2 foundation pass.
	return String();
}

}
