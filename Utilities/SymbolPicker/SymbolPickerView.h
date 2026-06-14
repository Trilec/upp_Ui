#ifndef _Utilities_SymbolPicker_SymbolPickerView_h_
#define _Utilities_SymbolPicker_SymbolPickerView_h_

#include "SymbolPickerModel.h"
#include "SymbolPickerCommands.h"

namespace Upp {

class SymbolPickerView : public TopWindow {
public:
	typedef SymbolPickerView CLASSNAME;

	SymbolPickerView();

	void SetModel(SymbolPickerModel* model);
	void SetCommands(SymbolPickerCommandStack* commands);
	void RefreshFromModel();

private:
	UiPanel root_;
	UiBoxLayout layout_ { UiDirection::V };
	UiTitleCard header_;
	UiLabel summary_;

	SymbolPickerModel* model_ = nullptr;
	SymbolPickerCommandStack* commands_ = nullptr;
};

}

#endif
