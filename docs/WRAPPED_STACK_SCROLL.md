# Wrapped stacks in scroll panels

UiMeasureLayout propagates width-dependent Fit heights through vertical
UiBoxLayout stacks and UiPanel surfaces. A scroll host can therefore measure
wrapped rows at its viewport width before assigning the content height.
Do not substitute the previous child rectangles or add application padding.

Vertical measurement uses the same fixed heights, main-axis bounds, row caps,
logical visibility, gaps and insets as layout. Expand remains a viewport
allocation; intrinsic measurement does not distribute spare height.

The focused native regression is in Utilities/UiReleaseSmoke. It composes six
execution-like rows with a trailing Available label, then repeatedly narrows and
widens a vertical-only scroll panel in Light and Dark. It checks measured height,
scroll-range growth/shrinkage, complete bottom-label visibility and stable repeat
layout, plus Fixed and hidden-entry sizing. Full-library and non-Windows
acceptance are separate gates.
