# HTML to native Ui handoff

| Browser region/behavior | Native direction | Important boundary |
| --- | --- | --- |
| Header/body/footer, matrix | UiGridLayout or nested UiBoxLayout | Explicit expanding area, meaningful row/column constraints |
| Toolbar, action row, wrapping cards | UiBoxLayout | Supported wrap and cross-axis policy; not every flex rule exists |
| Card/surface/group | UiPanel / UiGroupPanel | Surface vs layout; group has header/body slots |
| Heading and explanatory text | UiLabel / UiTitleCard | Use TitleCard only when its media/content structure helps |
| Action, compact icon, split action | UiButton / UiToolButton / UiSplitButton | Distinct main and dropdown actions |
| Check/switch/radio | UiCheckBox / UiToggle / UiRadioButton | Different semantics and keyboard behavior |
| Text/numeric/date/password input | UiLineEdit, UiMultiEdit, UiIntEdit, UiFloatEdit, UiDateTime, UiPasswordEdit, UiMaskEdit | Incomplete input vs committed value; read-only vs disabled |
| Choice or commands | UiDropdown / UiMenu | Item model vs command model; don't substitute on appearance alone |
| Value/interval with entry | UiSlider(Edit) / UiRangeSlider(Edit) | Drag preview/commit and numeric bounds |
| Bounded overflow | UiScrollPanel | One scrolling content root; measured extent |
| Tabs, pages, disclosure | UiTab / UiStack / UiAccordion | Active page and disclosure own visibility |
| Adjustable panes | UiSplitter / UiQuadSplitter | Define limits and persisted proportions |
| Flat/hierarchical/tabular/tile data | UiList / UiTree / UiTable / UiGallery | Model-driven, visible renderer pool, sparse editors |
| Property inspector | Utilities/PropertyEditor | Typed schema, preview/commit/reset/inheritance, host undo |
| Document | UiDoc | Its own document core, not an arbitrary HTML browser |
| Progress and proportional rings | UiProgressBar / UiProgressRing / UiChartRing | Single progress vs multiple series; legend may need composition |
| Palettes and structured numeric/color selection | UiColorPicker / UiColorMatrix / UiMatrixSelector / UiRangeSegments | Check each control's value and capacity contract |
| Curves and graph workspaces | UiBezierCurveEditor / UiNodeGraph | Existing model and geometry; app execution is separate |
| Canvas-like exact placement | UiAbsoluteLayout or custom Draw/Painter control | Use deliberately; doesn't provide automatic reflow |

For every region record its preferred size, minimum, overflow and behavior when
space is insufficient. Distinguish viewport size from content size. Mark optional
responsive transformations as host layout logic rather than claiming the native
control automatically implements CSS media queries.

Theme handoff should map background/surface, ink, muted ink, accent, alert, frame,
focus and interaction states to the corresponding family style. Radius, border,
padding and shadows are geometry, not just decoration. Preserve image aspect ratio
and state tint; provide native icon identifiers only when verified in UiIcons.h.

CSS blur/backdrop filters, arbitrary transforms, sticky DOM behavior, rich web
editors and browser-specific animation are not automatically native Ui features.
When they are essential, propose a simpler supported treatment or explicitly
identify the implementation cost. Don't quietly convert every interesting part
into a canvas just to claim reproducibility.
