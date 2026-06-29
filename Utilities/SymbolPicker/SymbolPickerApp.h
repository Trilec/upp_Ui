#ifndef _Utilities_SymbolPicker_SymbolPickerApp_h_
#define _Utilities_SymbolPicker_SymbolPickerApp_h_

#include "SymbolPickerGeneratedCatalog.h"
#include "SymbolPickerCatalogSeed.h"
#include "SymbolPickerView.h"

namespace Upp {

class SymbolPickerApp {
public:
	bool Init(String& error);
	void Run();

	SymbolPickerModel& GetModel() { return model_; }
	SymbolPickerCatalog& GetCatalog() { return catalog_; }
	SymbolPickerCommandStack& GetCommands() { return commands_; }
	SymbolPickerView& GetView() { return view_; }

private:
	void Wire();
	void SeedDemoData();

	SymbolPickerModel model_;
	SymbolPickerCatalog catalog_;
	SymbolPickerCommandStack commands_;
	SymbolPickerView view_;
};

}

#endif
