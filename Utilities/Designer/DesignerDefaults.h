#ifndef _Utilities_DesignerDefaults_h_
#define _Utilities_DesignerDefaults_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerDefaults
    ================

    Purpose
    - Public header for the DesignerDefaults component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include <Ui/Ui.h>

namespace Upp {

enum DesignerSizeDefault {
	DESIGNER_DEFAULT_WIDTH = 120,
	DESIGNER_DEFAULT_HEIGHT = 32,
	DESIGNER_MIN_WIDTH = 24,
	DESIGNER_MIN_HEIGHT = 20,
	DESIGNER_MIN_CLAMP = 10,

	DESIGNER_GRID_CELL_WIDTH = 25,
	DESIGNER_GRID_CELL_HEIGHT = 25,
	DESIGNER_GRID_MIN_WIDTH = 25,
	DESIGNER_GRID_MIN_HEIGHT = 25,

	DESIGNER_WINDOW_WIDTH = 760,
	DESIGNER_WINDOW_HEIGHT = 460,
	DESIGNER_WINDOW_MIN_WIDTH = 40,
	DESIGNER_WINDOW_MIN_HEIGHT = 40,

	DESIGNER_FIXED_FALLBACK_WIDTH = 120,
	DESIGNER_FIXED_FALLBACK_HEIGHT = 32,
	DESIGNER_SPLITTER_FALLBACK_HEIGHT = 80
};

inline Size DesignerDefaultSize()
{
	return Size(DESIGNER_DEFAULT_WIDTH, DESIGNER_DEFAULT_HEIGHT);
}

inline Size DesignerMinSize()
{
	return Size(DESIGNER_MIN_WIDTH, DESIGNER_MIN_HEIGHT);
}

inline Size DesignerGridCellSize()
{
	return Size(DESIGNER_GRID_CELL_WIDTH, DESIGNER_GRID_CELL_HEIGHT);
}

inline Size DesignerWindowSize()
{
	return Size(DESIGNER_WINDOW_WIDTH, DESIGNER_WINDOW_HEIGHT);
}

inline Size DesignerWindowMinSize()
{
	return Size(DESIGNER_WINDOW_MIN_WIDTH, DESIGNER_WINDOW_MIN_HEIGHT);
}

inline int DesignerClampMin(int value, int minimum = DESIGNER_MIN_CLAMP)
{
	return value < minimum ? minimum : value;
}

struct DesignerResolvedAxis {
	String sizing_mode;
	int minimum = 0;
	int maximum = 0; // 0 means unbounded.
	int preferred = 0;
	int fixed = 0;
	int available = 0;
	bool expand = false;
	bool fixed_mode = false;
	bool fit_mode = true;

	int Resolve() const
	{
		int value = fixed_mode ? fixed : expand ? available : preferred;
		if(maximum > 0)
			value = min(value, maximum);
		value = max(value, minimum);
		return max(0, value);
	}
};

inline DesignerResolvedAxis ResolveDesignerAxis(const String& sizing_mode, int fixed_value,
                                                int explicit_min, int explicit_max,
                                                int preferred_value, int available_value)
{
	DesignerResolvedAxis axis;
	axis.sizing_mode = sizing_mode;
	axis.minimum = max(0, explicit_min);
	axis.maximum = max(0, explicit_max);
	if(axis.maximum > 0 && axis.minimum > axis.maximum)
		Swap(axis.minimum, axis.maximum);
	axis.preferred = max(axis.minimum, preferred_value);
	if(axis.maximum > 0)
		axis.preferred = min(axis.preferred, axis.maximum);
	axis.fixed = max(axis.minimum, fixed_value);
	if(axis.maximum > 0)
		axis.fixed = min(axis.fixed, axis.maximum);
	axis.available = max(axis.minimum, available_value);
	if(axis.maximum > 0)
		axis.available = min(axis.available, axis.maximum);
	axis.fixed_mode = sizing_mode == "Fixed";
	axis.expand = sizing_mode == "Expand";
	axis.fit_mode = !axis.fixed_mode && !axis.expand;
	return axis;
}

}

#endif
