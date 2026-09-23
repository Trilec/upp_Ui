# 01 — Controls Guide

Public control catalogue for Ui. Each concrete type has an entry below; shared
base/model/drawing helpers are listed separately. The [release inventory](../tests/ui_release_inventory.json)
is the audit/validation register. A listed demo is a starting point for exploration,
not a claim that its entire API or generated code has already passed release acceptance.

## Common contracts

Include `<Ui/Ui.h>` for the full library or the narrower public header. Read the
[Coding Guide](00_UPP_CODING_GUIDE.md) for ownership and naming. Controls normally
run on the GUI thread. Adding a child establishes parenting, not C++ deletion
ownership; keep borrowed children/models alive or detach them explicitly.

Fit/Fixed/Expand select size; alignment selects position in an allocation. Apply
DPI once. GetMinSize/GetContentSize/Layout and hit geometry must use the same
frame, skin inset, content margin, gap and item-spacing vocabulary. Single-root
hosts need a layout inside their slot for several controls; do not overlap multiple
roots accidentally. UiGroupPanel has distinct header and body slots.

Themeable controls use Minimal Standard/Subtle/Accent/Alert in Light/Dark with
family-appropriate semantics. Typography roles are independent. Palette states
are Normal/Hot/Pressed/Disabled; selection, focus and read-only are separate.
StyleDefault is immutable; SetCustomStyle owns a snapshot; ClearCustomStyle
restores the current theme. Convenience setters can create a complete custom
snapshot: consult the header rather than assuming partial live inheritance.
Explicit None means intentionally absent, not inherited. See [Theme](02_UI_THEME_GUIDE.md).

Callbacks must document programmatic versus user behavior and preview/commit/
cancellation. A committed notification sees new state. Model-backed mutations
can be request-first; see [Models](03_UI_MODEL_GUIDE.md). SetData/GetData is binding,
not permission to coerce invalid types or use Null sentinels as real values.

Native Draw is suitable for simple straight geometry/text; Painter supplies
antialiased curves. Do not duplicate shape or raster systems per control. The
[Drawing Guide](07_UI_DRAWING_GUIDE.md) owns geometry/cache/performance rules.
The retired UiComposite property-row family must not return: compose Ui controls
and the production PropertyEditor instead.

## Catalogue

No dedicated demo is implied where the Example cell says "family coverage to accept".
A shell using a control does not substitute for its behavioral/property coverage.

| Control | Purpose | Reference example |
| --- | --- | --- |
| [UiLabel](../Ui/UiLabel.h) | Styled text, selection, wrapping, icons and media. | [UiLabelDemo](../examples/UiLabelDemo) |
| [UiButton](../Ui/UiButton.h) | Primary stateful action. | [UiButtonDemo](../examples/UiButtonDemo) |
| [UiToolButton](../Ui/UiToolButton.h) | Compact toolbar action; used in the reference shell. | [UiLabelDemo](../examples/UiLabelDemo) |
| [UiSplitButton](../Ui/UiSplitButton.h) | Primary action plus a separate dropdown action. | [UiSplitButtonDemo](../examples/UiSplitButtonDemo) |
| [UiCheckBox](../Ui/UiCheckBox.h) | Independent checked state and supported visual variants. | [UiCheckBoxDemo](../examples/UiCheckBoxDemo) |
| [UiRadioButton](../Ui/UiRadioButton.h) | Exclusive-choice presentation and grouping behavior. | [UiRadioButtonDemo](../examples/UiRadioButtonDemo) |
| [UiToggle](../Ui/UiToggle.h) | Boolean switch with track/thumb styling. | [UiToggleDemo](../examples/UiToggleDemo) |
| [UiBreadcrumbs](../Ui/UiBreadcrumbs.h) | Path navigation with optional icons and separators. | [UiBreadcrumbsDemo](../examples/UiBreadcrumbsDemo) |
| [UiLineEdit](../Ui/UiLineEdit.h) | Single-line text; shared UiBaseEdit behavior. | [UiEditDemo](../examples/UiEditDemo) |
| [UiIntEdit](../Ui/UiIntEdit.h) | Integer entry with numeric bounds and step behavior. | [UiIntFloatDemo](../examples/UiIntFloatDemo) |
| [UiFloatEdit](../Ui/UiFloatEdit.h) | Floating-point/scientific input with incomplete-input handling. | [UiIntFloatDemo](../examples/UiIntFloatDemo) |
| [UiPasswordEdit](../Ui/UiPasswordEdit.h) | Password masking and visibility controls. | [UiEditDemo](../examples/UiEditDemo) |
| [UiMultiEdit](../Ui/UiMultiEdit.h) | Multi-line editing and whitespace/tab policy. | [UiEditDemo](../examples/UiEditDemo) |
| [UiMaskEdit](../Ui/UiMaskEdit.h) | Mask-driven entry, formatting and validation. | [UiEditDemo](../examples/UiEditDemo) |
| [UiSlider](../Ui/UiSlider.h) | One scalar value within a domain. | [UiSliderDemo](../examples/UiSliderDemo) |
| [UiRangeSlider](../Ui/UiRangeSlider.h) | Ordered interval; optional adjustable inner bounds. | [UiSliderDemo](../examples/UiSliderDemo) |
| [UiSliderEdit](../Ui/UiSliderEdit.h) | Slider with a direct numeric editor. | family coverage to accept |
| [UiRangeSliderEdit](../Ui/UiRangeSliderEdit.h) | Interval slider with lower/upper numeric editors. | family coverage to accept |
| [UiRangeSegments](../Ui/UiRangeSegments.h) | Contiguous labeled segments over one fixed scalar domain. | [UiRangeSegmentsDemo](../examples/UiRangeSegmentsDemo) |
| [UiScrollBar](../Ui/UiScrollBar.h) | Scroll position/extent and themed arrows/thumb. | [UiScrollBarDemo](../examples/UiScrollBarDemo) |
| [UiProgressBar](../Ui/UiProgressBar.h) | Linear determinate/indeterminate progress. | [UiProgressBarDemo](../examples/UiProgressBarDemo) |
| [UiProgressRing](../Ui/UiProgressRing.h) | One amount against a total, circular presentation. | [UiProgressRingDemo](../examples/UiProgressRingDemo) |
| [UiChartRing](../Ui/UiChartRing.h) | Several proportional values composing one ring. | [UiChartRingDemo](../examples/UiChartRingDemo) |
| [UiMatrixSelector](../Ui/UiMatrixSelector.h) | Spatial cell/ordered-pair choice with shared glyphs. | [UiMatrixSelectorDemo](../examples/UiMatrixSelectorDemo) |
| [UiColorMatrix](../Ui/UiColorMatrix.h) | One to eight ordered color values with one shared picker. | family coverage to accept |
| [UiDateTime](../Ui/UiDateTime.h) | Local date/time/date-time input and picker. | [UiDateTimeDemo](../examples/UiDateTimeDemo) |
| [UiColorPicker](../Ui/UiColorPicker/UiColorPicker.h) | Multi-slot color editing, palettes and image/screen picking. | [UiColorPickerDemo](../examples/UiColorPickerDemo) |
| [UiDropdown](../Ui/UiDropdown.h) | Collapsed choice and model-backed popup. | [UiDropdownDemo](../examples/UiDropdownDemo) |
| [UiMenu](../Ui/UiMenu.h) | Command/check/radio/submenu model presentation. | [UiMenuDemo](../examples/UiMenuDemo) |
| [UiPanel](../Ui/UiPanel.h) | Styled single-root content host, not a flow layout. | [UiPanelDemo](../examples/UiPanelDemo) |
| [UiDirectContentHost](../Ui/UiDirectContentHost.h) | Borrowed single child with independent Fit/Fixed/Expand axes. | family coverage to accept |
| [UiGroupPanel](../Ui/UiGroupPanel.h) | Titled frame with separate header and body root slots. | [UiPanelDemo](../examples/UiPanelDemo) |
| [UiTitleCard](../Ui/UiTitleCard.h) | Title/subtitle/media with an adjacent content cell. | [UiTitleCardDemo](../examples/UiTitleCardDemo) |
| [UiStack](../Ui/UiStack.h) | Exclusive page hosting and measurement. | family coverage to accept |
| [UiAccordion](../Ui/UiAccordion.h) | Collapsible real-child sections with optional reorder. | [UiAccordionDemo](../examples/UiAccordionDemo) |
| [UiScrollPanel](../Ui/UiScrollPanel.h) | Bounded viewport around one content root. | [UiScrollPanelDemo](../examples/UiScrollPanelDemo) |
| [UiTab](../Ui/UiTab.h) | Tabbed page host with role-owned cap and strip fills. | [UiTabDemo](../examples/UiTabDemo) |
| [UiSplitter](../Ui/UiSplitter.h) | Pane sizing with styled split handles. | [UiSplitterDemo](../examples/UiSplitterDemo) |
| [UiQuadSplitter](../Ui/UiQuadSplitter.h) | Four-pane composition over ordinary splitters. | [UiSplitterDemo](../examples/UiSplitterDemo) |
| [UiAbsoluteLayout](../Ui/UiAbsoluteLayout.h) | Exact local child rectangles without automatic reflow. | family coverage to accept |
| [UiGridLayout](../Ui/UiGridLayout.h) | Logical rows and columns with stable placement. | family coverage to accept |
| [UiBoxLayout](../Ui/UiBoxLayout.h) | Ordered row/column flow with Fit/Fixed/Expand. | family coverage to accept |
| [UiList](../Ui/UiList.h) | Sequential model view and visible renderer pooling. | [UiListDemo](../examples/UiListDemo) |
| [UiTree](../Ui/UiTree.h) | Stable hierarchical model identity and visible projection. | [UiTreeDemo](../examples/UiTreeDemo) |
| [UiTable](../Ui/UiTable.h) | Coordinate/range model view with editing and headers. | [UiTableDemo](../examples/UiTableDemo) |
| [UiGallery](../Ui/UiGallery.h) | Tile/image presentation of a list model. | [UiGalleryDemo](../examples/UiGalleryDemo) |
| [UiDoc](../Ui/UiDoc/UiDoc.h) | Document view/editor over the authoritative UiDocCore. | [UiDocDemo](../examples/UiDocDemo) |
| [UiBezierCurveEditor](../Ui/UiBezierCurveEditor.h) | Editable cubic curve with selection and data binding. | family coverage to accept |
| [UiBezierCurveField](../Ui/UiBezierCurveField.h) | Curve editor with optional formula and copy composition. | family coverage to accept |
| [UiNodeGraph](../Ui/UiGraph/UiNodeGraph.h) | Retained graph topology, routing, hierarchy and presentation. | [UiGraphDemo](../examples/UiGraphDemo) |

## Edit family

UiBaseEdit provides shared editing, selection, placeholder, caret, side/spin and
style behavior. Concrete Line/Password/Multi/Mask/Int/Float types retain their own
input policy; sharing a base is not proof that specialized validation is identical.
UiEditDemo is the text-family direction; UiIntFloatDemo is its numeric companion.
Older individual demos remain until capability/export coverage is reconciled.

UiFloatEdit supports decimal/scientific notation, signed exponents and temporarily
incomplete text. Min/Max/MinMax constrain committed values; Step drives numeric
editing; Precision formats a complete value; NotNull controls emptiness.
TryGetValue and IsInputComplete distinguish a valid finite value from incomplete
input without destructive rewriting while typing. SetData accepts numeric data/
text and GetData returns the typed value or permitted Null.

```cpp
UiFloatEdit amount;
amount.MinMax(-1000, 1000).Step(0.25).Precision(3).ShowSpin(true);
amount.SetValue(12.5);
```

Mask validators/formatters, password visibility, multiline whitespace and numeric
spin behavior need separate family cases. Clipboard, Unicode, focus loss, Escape,
readonly/disabled transitions and partial exponents are acceptance inputs, not
just a successful constructor.

## Scalar sliders, intervals and segmented ranges

UiSlider owns one scalar value. UiRangeSlider reuses its style for an ordered
lower/upper interval inside a hard range. SetRange sets the domain, SetStep sets
snap units, SetValues sets the interval, GetLowerValue/GetUpperValue read it.
Active handles are explicit keyboard/wheel targets. SetStart/End/StartEnd are
established animation-friendly aliases over the same state, not another model.

EnableAdjustableBounds adds lower-bound/lower-selection/upper-selection/upper-bound
ordering inside the hard range. SetBounds edits those inner bounds. Bound handles
remain distinct targets; ShowEndpointMarkers controls the hard-domain markers.
UiRangeSliderEdit composes the authoritative slider with two UiFloatEdit fields;
SetFieldWidth/SetGap/SetInset affect composition, not semantic value. Slider() exposes
the actual range slider. Its binding remains the documented two-element ValueArray.

UiRangeSegments partitions one fixed domain into N contiguous labeled spans with
N-1 boundaries. It is neither a chart nor a continuous color-gradient editor.

```cpp
UiRangeSegments ranges;
ranges.SetRange(0, 100).SetStep(1).SetSegmentCount(4);
Vector<double> thresholds;
thresholds << 18 << 48 << 76;
ranges.SetBoundaryValues(thresholds);
Vector<double> current = ranges.GetBoundaryValues();
```

UiRangeSegment stores span, label, optional color (Null means automatic), and Value
data. SetSegments normalizes proportional weights; invalid/negative weights count
as zero and all-zero weights become equal. The domain endpoints must be finite
and have a finite difference. Invalid scalar setters leave state unchanged.
MinimumSegmentSpan enforces feasible minima; boundary i changes only spans i/i+1.
SetBoundaryValues validates the whole vector before changing structure. Split inserts
a boundary; Remove merges into a neighbor. Endpoints never move during a drag.

Direction/reverse/resize changes only projection, never scalar ordering or payloads.
Percent versus Domain labels are presentation. ShowLabels, ShowBoundaryValues,
ShowEndpointValues, ShowValuesOnInteraction and ShowDividers control presentation.
Selection of a segment is independent from the active boundary. Labels are omitted
when they cannot fit; a narrow vertical track is not a promise of readable long text.

WhenChanging reports live user edits and WhenAction committed edits; programmatic
setters are silent. WhenSegmentSelect/WhenBoundarySelect report user selection.
Capture loss/disable/Escape ends a live drag without another commit and retains its
last live value. Callbacks see committed state and may rebuild/destroy the control.
GetData returns ValueArray of maps (span/label/optional color/data); SetData accepts
that representation. There is no second mutable model.

Up to eight palette anchors form deterministic Series or sampled Gradient colors.
Extra authored Series entries cycle with predictable light/dark variants. Inherited
semantic ramps span the actual segment count; explicit segment colors win. Whole
style snapshots stay explicit through a theme change. The actual content strip and
thumbs use bounded AA/shared exact rasters; text stays Draw and no timer is owned.
UiReleaseSmoke protects numeric, lifetime, role and real-pixel/cache cases alongside
UiRangeSegmentsRunTests. In Graph, thresholds must not resize the preview camera.

## Matrix and color choices

UiMatrixSelector is one small bounded Ctrl for Position9, Compass8, Region5,
QuadPair or Cardinal4 spatial choices. Cardinal4 has no invalid center/diagonals.
SetPreset configures the grid; cells carry label/value/icon/glyph/visibility/enabled.
SetCell/Label/Value/Icon/Glyph and EnableCell/ShowCell expose content. Cell, selected
and readout roles are distinct. Grid dimensions/count and settled cell/readout
rectangles are queryable; sizing derives from actual geometry.

Single selection uses SelectIndex/GetSelectedIndex/GetSelectedLabel and SetData/
GetData. Pair selection uses SetPair, pair indices/completeness/orientation/direction
and readout. SetDefault/ClearDefault/ShowDefault supplies a dashed default marker
separate from committed selection. Arrow navigation skips ineligible cells;
WhenChanging is preview and WhenAction committed activation. Programmatic fire_action
arguments are explicit. Cell/readout gaps, radius, face/frame, fonts and insets are
style, not specialized domain relationships.

UiColorMatrix edits one through eight ordered colors. SetColors/SetColorCount/
SetColor/GetColors and per-slot labels describe the same set. Activating a swatch
opens one picker with the complete set, not a separate picker/model per color.
Cancel restores all opening colors; WhenChanging previews, WhenAction commits and
WhenSelect identifies the active slot. EnablePicker(false) makes selection-only.

Adaptive grid layout fits useful square swatches within actual capacity. Slot
size bounds/gap/frame/radius/shadow and containing-surface style use shared Ui
vocabulary. Actual swatch faces are the values, not role-colored decorations.
One color binds as Color; multiple colors as ValueArray. Arrows follow the settled
grid and Enter/Space opens the picker. Capacity matches the current picker contract.

UiColorPicker's authoritative slots are SetSlotCount, SetActiveSlot, SetSlot and
GetSlots (one through eight). It supports alpha/color models, palettes, image
analysis, stash/session and screen picking. Preview/commit/dialog acceptance/cancel
are distinct. Ordered multi-selection/drag transfer must preserve palette order.
UiColorPickerPaletteLab is reusable conversion/palette/analysis support, not another
control required per swatch.

## Local date and time

UiDateTime holds one local/naive U++ Time. DateMode/TimeMode/DateTimeMode, Locale/Iso
format, locale/12/24 clock, ShowSeconds, language and first-day settings affect
presentation. Time-zone/instant conversion belongs outside this control.

SetEditable and SetPresentation distinguish editable/picker from read-only display;
presentation is normally chromeless unless ShowPresentationFrame is enabled.
The mode selects Calendar, Clock or CalendarClock, themed through Ui. No second
popup model owns a competing value. AllowCopy/AllowPaste apply to keyboard and
explicit clipboard APIs; paste also requires editable mode.

AllowNull/ClearValue, SetRange/SetDateRange/ClearRange constrain values. CommitText
validates complete calendar/time input; invalid text restores the stored formatted
value and reports WhenInvalid instead of corrupting state. SetValue/GetValue,
SetDate/GetDate/SetTime/SetNow/SetToday and SetData/GetData share the same authority.
WhenChanging, WhenAction, WhenInvalid and WhenOpenPicker are distinct notifications.
All locale/ISO/12/24 formatting paths must honor ShowSeconds.

## Containers and layout

UiPanel/UiScrollPanel/UiTitleCard each host one content root; UiTitleCard uses its
adjacent SetContentCell. UiGroupPanel has independently replaceable header-content
and body-content roots. Use a box/grid/absolute layout inside a root for several
children. Parenting still does not imply deletion ownership.

GroupPanel header placement supports Top/Bottom/Left/Right, mode Outside/Center/
Inside, identity alignment and separate header-child alignment. Opposite/trailing
space is reserved for header content without covering the identity block. Public
GetHeaderContentRect/GetBodyRect match prospective geometry. Minimum measurement
includes identity, child, gaps, insets and surface; forced undersize stays valid.
In Center mode the frame avoids both occupied header rectangles. Retired SideTitle
stream fields remain only for compatibility, not runtime styling.

UiDirectContentHost supplies Fit/Fixed/Expand, fixed/min/max sizes and alignment
without painted styling. It borrows one child and ignores destroyed/externally
reparented content. ClearContent detaches only its own child. Self/ancestor parenting
is rejected. UiStack and UiTab own page-selection semantics; UiAccordion owns
section/collapse/reorder semantics. Layout helpers are not replacements for those
page states. Hidden pages must not be accidentally re-shown by flow participation.

## Progress, rings and custom painting

UiProgressRing is one current amount/total, optional percent/custom center text,
gradient, intro animation and determinate/indeterminate state. Set/Get/GetTotal/
GetRatio/GetPercent describe its value. Its centered square stays circular; stable
rasters cache, animation owns UiFrameTicker and stops with its lifecycle.

UiChartRing is several nonnegative segment values, labels and optional colors.
Positive values normally define total; a larger explicit total can leave track
remainder. Gap and cap-roundness are visible metrics. It has no implicit legend,
selection, nested/exploded-ring interaction. Both use shared native stroked arcs;
filled wedges/donuts use UiShapes helpers instead.

Controls with supported background/content/foreground or part-aware paint hooks
use their documented context/handled contracts. Slider Track/ActiveTrack/Thumb,
ScrollBar parts and Toggle Track/Thumb are examples; do not assume every control
has every hook. Generated code must reproduce a selected override with real APIs,
not unexplained demo-local overpainting. Explicit paint and hit bounds must agree.

## Collections, documents and supporting APIs

List/Gallery/Dropdown use UiListModel, Tree UiTreeModel, Table UiTableModel and
Menu UiMenuModel; ordinary content uses bounded UiItemRender pools. View-owned
selection, disclosure, command/check/radio state and editing remain appropriate to
the domain. GetMinSize/Layout/paint never allocate one Ctrl per logical item.
UiDoc/UiDocCore ownership, transactions and future extraction are in the Models
Guide. Graph usage and retained development have their own two guides.

Supporting public surfaces include UiBaseEdit and UiIndicatorBase, UiAxis,
UiLayoutCursor/UiMeasure, UiStyle/UiTheme, UiGeometry/UiShapePath/UiShapes/UiDraw,
UiRenderLayer/UiFrameTicker, UiIcons, model/render types and Graph/Doc core types.
The separate optional UiOsFileDialog package has platform-specific integration;
it is not evidence of native dialog validation on every platform. PropertyEditor
and PropertyEditorCore are reusable siblings, not dependencies on UiDesigner.

These helpers need documented responsibility and dependency/lifetime coverage,
not invented visual roles or a separate executable demo for every header. The
release inventory tracks missing dedicated/family demonstration, source review,
generated-code and platform evidence explicitly rather than inferring completion
from the catalogue or a general Ui compile.
