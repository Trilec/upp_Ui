#ifndef _Ui_UiMeasure_h_
#define _Ui_UiMeasure_h_

#include <CtrlCore/CtrlCore.h>

namespace Upp {

struct UiLayoutMeasureSpec {
	int available_width = -1;
};

struct UiLayoutMeasureResult {
	Size preferred;
	Size min;
	Size measured;
	bool width_dependent = false;
};

UiLayoutMeasureResult UiMeasureLayout(const Ctrl& c, const UiLayoutMeasureSpec& spec = UiLayoutMeasureSpec());

}

#endif
