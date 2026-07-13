#ifndef _Ui_UiAxis_h_
#define _Ui_UiAxis_h_

/*
    UiAxis
    ======

    Purpose
    - Shared runtime sizing contract for fit/fixed/expand axis resolution.

    Intent
    - Keep size normalization in one place so Designer, direct-content hosts,
      and any future layout consumers do not each invent their own clamp rules.

    Thread context
    - GUI thread together with the control tree being laid out.
*/

#include <CtrlCore/CtrlCore.h>

namespace Upp {

enum class UiAxisMode : byte {
	Fit,
	Fixed,
	Expand
};

struct UiAxisConstraints : Moveable<UiAxisConstraints> {
	UiAxisMode mode = UiAxisMode::Fit;
	int minimum = 0;
	int maximum = 0; // 0 means unbounded.
	int fixed = 0;
	int preferred = 0;
	int available = 0;
};

struct UiResolvedAxis : Moveable<UiResolvedAxis> {
	int minimum = 0;
	int maximum = 0;   // 0 means unbounded.
	int preferred = 0;
	int final_size = 0;
};

UiResolvedAxis UiResolveAxis(const UiAxisConstraints& constraints);

}

#endif
