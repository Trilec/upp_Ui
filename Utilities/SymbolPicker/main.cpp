#include "SymbolPickerApp.h"

using namespace Upp;

GUI_APP_MAIN
{
	SymbolPickerApp app;
	String error;
	if(!app.Init(error)) {
		Exclamation(error.IsEmpty() ? "SymbolPicker startup smoke tests failed." : error);
		return;
	}
	app.Run();
}
