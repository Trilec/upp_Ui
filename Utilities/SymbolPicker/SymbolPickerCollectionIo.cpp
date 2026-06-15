#include "SymbolPickerCollectionIo.h"

namespace Upp {

bool SaveSymbolPickerCollectionJsonStub(const SymbolPickerCollection&)
{
	// Stub only for the v0.2 foundation pass.
	// JSON will be the source of truth when real save/load lands.
	return false;
}

bool LoadSymbolPickerCollectionJsonStub(const String&, SymbolPickerCollection&)
{
	// Stub only for the v0.2 foundation pass.
	// We do not parse arbitrary edited C++ headers in v1.
	return false;
}

String BuildSymbolPickerGeneratedHeaderStub(const SymbolPickerCollection&)
{
	// Stub only for the v0.2 foundation pass.
	// Generated headers are output artifacts; any metadata is provenance only.
	return String();
}

}
