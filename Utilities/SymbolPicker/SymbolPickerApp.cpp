#include "SymbolPickerApp.h"

namespace Upp {

bool SymbolPickerApp::Init(String& error)
{
	if(!RunSymbolPickerCommandSmokeTests(error))
		return false;
	Wire();
	return true;
}

void SymbolPickerApp::Wire()
{
	view_.SetModel(&model_);
	view_.SetCommands(&commands_);
	model_.WhenChanged = [=] {
		view_.RefreshFromModel();
	};
}

void SymbolPickerApp::Run()
{
	view_.OpenMain();
}

}
