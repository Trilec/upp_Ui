#ifndef _Ui_UiIndicatorSupport_h_
#define _Ui_UiIndicatorSupport_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiIndicatorSupport
    ==================

    Purpose
    - Shared internal helpers for indicator-bearing controls such as
      UiCheckBox and UiRadioButton.

    Intent
    - Reuse state mapping and indicator/text block layout helpers without
      forcing unrelated controls into a common base class too early.

    Thread context
    - Thread-neutral helper functions.

    Usage
    - Use these helpers from indicator controls that already rely on
      UiBlocksLayout and shared styled geometry primitives.

    Changelog
    - 2026-03-31: introduced as phase-2 shared layout/state support on the
      path toward a possible UiIndicatorBase.
*/

#include <Ui/UiStyle.h>

namespace Upp {

inline StyledState UiIndicatorStyledState(bool enabled, bool pressed, bool hover)
{
    if(!enabled) return ST_DISABLED;
    if(pressed)  return ST_PRESSED;
    if(hover)    return ST_HOT;
    return ST_NORMAL;
}

inline UiBlocksLayout UiComputeIndicatorBlocksLayout(const Rect& content,
                                                     Size support_natural,
                                                     Size main_natural,
                                                     UiAlign align_h,
                                                     UiAlign align_v,
                                                     UiAlign indicator_side,
                                                     int gap,
                                                     int min_support_side)
{
    return UiComputeBlocksLayout(content,
                                 support_natural,
                                 main_natural,
                                 align_h,
                                 align_v,
                                 indicator_side,
                                 min_support_side,
                                 gap);
}

inline Size UiMeasureIndicatorBlocksContent(Size support_natural,
                                            Size main_natural,
                                            UiAlign indicator_side,
                                            int gap,
                                            int empty_w,
                                            int empty_h,
                                            int min_support_side,
                                            bool have_main)
{
    return UiMeasureBlocksContent(support_natural,
                                  main_natural,
                                  indicator_side,
                                  true,
                                  have_main,
                                  empty_w,
                                  empty_h,
                                  min_support_side,
                                  gap);
}

}

#endif

