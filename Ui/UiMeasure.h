#ifndef _Ui_UiMeasure_h_
#define _Ui_UiMeasure_h_

#include <CtrlCore/CtrlCore.h>

namespace Upp {

struct UiLayoutMeasureSpec {
    // `available_width` is the width hint for width-dependent controls.
    // Leave it at -1 for unconstrained/preferred measurement.
	int available_width = -1;
};

struct UiLayoutMeasureResult {
    // Natural size without width constraint.
	Size preferred;
    // Smallest useful size when constrained.
	Size min;
    // Result under `available_width` if one was supplied.
	Size measured;
    // True when width changes the measurement in a meaningful way.
	bool width_dependent = false;
};

// Central measurement helper for V1.
// Ordinary controls should remain cheap and just return GetMinSize() defaults.
// Only width-dependent containers should participate here, and GetContentSize()
// must not be used for parent pre-layout shrink decisions.
UiLayoutMeasureResult UiMeasureLayout(const Ctrl& c, const UiLayoutMeasureSpec& spec = UiLayoutMeasureSpec());

}

#endif
