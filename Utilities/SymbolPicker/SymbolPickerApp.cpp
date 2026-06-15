#include "SymbolPickerApp.h"

namespace Upp {

bool SymbolPickerApp::Init(String& error)
{
	if(!RunSymbolPickerCommandSmokeTests(error))
		return false;
	model_.CreateCollection("Core Set");
	model_.CreateCollection("Marketing Draft");
	model_.SetActiveCollection(0);
	model_.AddIconToBin("action/account_balance");
	model_.AddIconToBin("alert/ad_group_off");
	SymbolPickerIconRef unresolved;
	unresolved.source_id = "legacy/missing_icon";
	unresolved.alias = "LegacyMissing";
	unresolved.unresolved = true;
	model_.AddIconToCollection(0, unresolved);
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
	view_.Run();
}

}
