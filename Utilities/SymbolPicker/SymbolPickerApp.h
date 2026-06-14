#ifndef _Utilities_SymbolPicker_SymbolPickerApp_h_
#define _Utilities_SymbolPicker_SymbolPickerApp_h_

#include "SymbolPickerView.h"

namespace Upp {

class SymbolPickerApp {
public:
	bool Init(String& error);
	void Run();

	SymbolPickerModel& GetModel() { return model_; }
	SymbolPickerCommandStack& GetCommands() { return commands_; }
	SymbolPickerView& GetView() { return view_; }

private:
	void Wire();

	SymbolPickerModel model_;
	SymbolPickerCommandStack commands_;
	SymbolPickerView view_;
};

}

#endif
