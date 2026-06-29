#include "SymbolPickerApp.h"

namespace Upp {

static const bool kEnableLocalDemoData = true;

bool SymbolPickerApp::Init(String& error)
{
	if(!RunSymbolPickerCommandSmokeTests(error))
		return false;
	if(!RunSymbolPickerCatalogSmokeTests(error))
		return false;
	if(LoadGeneratedSymbolPickerCatalog(catalog_) <= 0)
		SeedSymbolPickerCatalog(catalog_);
	Wire();
	if(kEnableLocalDemoData)
		SeedDemoData();
	return true;
}

void SymbolPickerApp::Wire()
{
	view_.SetModel(&model_);
	view_.SetCatalog(&catalog_);
	view_.SetCommands(&commands_);
	model_.WhenChanged = [=] {
		view_.RefreshFromModel();
	};
}

void SymbolPickerApp::SeedDemoData()
{
	// Local demo seed only for the shell phase. These values are intentionally
	// obvious placeholders so they do not look like real application defaults.
	model_.CreateCollection("Demo Collection A");
	model_.CreateCollection("Demo Collection B");
	model_.SetActiveCollection(0);
	model_.AddIconToBin("demo/action_placeholder");
	model_.AddIconToBin("demo/alert_placeholder");

	SymbolPickerIconRef unresolved;
	unresolved.catalog_id = "demo/unresolved_placeholder/outlined";
	unresolved.source_id = "demo/unresolved_placeholder";
	unresolved.alias = "DemoMissing";
	unresolved.unresolved = true;
	model_.AddIconToCollection(0, unresolved);
}

void SymbolPickerApp::Run()
{
	view_.Run();
}

}
