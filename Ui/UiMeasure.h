#ifndef _Ui_UiMeasure_h_
#define _Ui_UiMeasure_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiMeasure
    =========

    Purpose
    - Central measurement helper and shared width-aware sizing contract for
      containers that need more than a plain minimum-size fallback.

    Intent
    - Keep ordinary controls cheap while giving wrapped or width-dependent
      layouts one shared place to report preferred, minimum, and measured
      sizes without mixing post-layout content extents into parent decisions.

    Thread context
    - Measurement should be queried from the GUI thread together with the
      control tree being laid out.

    Usage
    - Layout code calls UiMeasureLayout() for wrapped BoxLayout children,
      container-like panels, and other controls that need width-aware fit
      measurement.
    - Ordinary controls usually stay on the fallback path and just behave like
      a normal GetMinSize() query.

    Changelog
    - 2026-06: introduced the shared measurement helper to unify preferred,
      minimum, and width-aware layout sizing without adding more public size
      methods.
*/

#include <CtrlCore/CtrlCore.h>

namespace Upp {

struct UiLayoutMeasureSpec {
    // Width hint for containers that need to re-measure as they wrap or
    // redistribute children. Leave it at -1 for ordinary unconstrained
    // preferred-size queries.
	int available_width = -1;
};

struct UiLayoutMeasureResult {
    // Natural content size with no width constraint applied.
	Size preferred;
    // Smallest useful size when the control is forced to shrink.
	Size min;
    // Measured size using the supplied width hint, if any.
	Size measured;
    // True when width changes the measurement in a meaningful way.
	bool width_dependent = false;
};


UiLayoutMeasureResult UiMeasureLayout(const Ctrl& c, const UiLayoutMeasureSpec& spec = UiLayoutMeasureSpec());

}

#endif
