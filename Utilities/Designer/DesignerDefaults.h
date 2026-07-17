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

	DESIGNER_GRID_CELL_WIDTH = 10,
	DESIGNER_GRID_CELL_HEIGHT = 10,
	DESIGNER_GRID_MIN_WIDTH = 10,
	DESIGNER_GRID_MIN_HEIGHT = 10,

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

using DesignerResolvedAxis = UiResolvedAxis;

inline UiAxisMode DesignerAxisModeFromString(const String& sizing_mode)
{
	if(sizing_mode == "Fixed")
		return UiAxisMode::Fixed;
	if(sizing_mode == "Expand")
		return UiAxisMode::Expand;
	return UiAxisMode::Fit;
}

inline DesignerResolvedAxis ResolveDesignerAxis(const String& sizing_mode, int fixed_value,
                                                int explicit_min, int explicit_max,
                                                int preferred_value, int available_value)
{
	UiAxisConstraints c;
	c.mode = DesignerAxisModeFromString(sizing_mode);
	c.minimum = explicit_min;
	c.maximum = explicit_max;
	c.fixed = fixed_value;
	c.preferred = preferred_value;
	c.available = available_value;
	return UiResolveAxis(c);
}

}

#endif
