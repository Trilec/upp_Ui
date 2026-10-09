# 01 — Controls Guide

Public control catalogue for Ui. Each concrete type has an entry below; shared
base/model/drawing helpers are listed separately. The [release inventory](../tests/ui_release_inventory.json)
is the audit/validation register. A listed demo is a starting point for exploration,
not a claim that its entire API or generated code has already passed release acceptance.

Start with [Control usage recipes](CONTROL_USAGE.md) for a small public API
example of every concrete catalogue entry. The sections below explain the shared
contracts and the cases that need more than a constructor and a setter.

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

The generic media composites [UiColorProbe and UiPlaybackBar](MEDIA_CONTROLS.md) share existing Ui children and delegate media work to their host.

| Control | Purpose | Reference example |
| --- | --- | --- |
| [UiColorProbe](../Ui/UiColorProbe.h) | Raw RGB(A) probe readout, display swatch and sampling choices | [UiMediaControlsDemo](../examples/UiMediaControlsDemo) |
| [UiPlaybackBar](../Ui/UiPlaybackBar.h) | Transport requests, frame scrubbing, range, markers and cache coverage | [UiMediaControlsDemo](../examples/UiMediaControlsDemo) |
| [UiLabel](../Ui/UiLabel.h) | Styled text, selection, wrapping, icons and media. | [UiLabelDemo](../examples/UiLabelDemo) |
| [UiTag](../Ui/UiTag.h) | Ultralight prepared semantic tag/status marker; optional host-routed interaction, text and/or icon. Not a Ctrl. | [UiTagDemo](../examples/UiTagDemo) |
| [UiButton](../Ui/UiButton.h) | Primary stateful action. | [UiButtonDemo](../examples/UiButtonDemo) |
| [UiToolButton](../Ui/UiToolButton.h) | Compact toolbar action, toggle and authored style demonstration. | [UiButtonDemo](../examples/UiButtonDemo) |
| [UiSplitButton](../Ui/UiSplitButton.h) | Primary action plus a separate dropdown action. | [UiButtonDemo](../examples/UiButtonDemo) |
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
| [UiSliderEdit](../Ui/UiSliderEdit.h) | Slider with a direct numeric editor. | [UiSliderDemo](../examples/UiSliderDemo) |
| [UiRangeSliderEdit](../Ui/UiRangeSliderEdit.h) | Interval slider with lower/upper numeric editors. | [UiSliderDemo](../examples/UiSliderDemo) |
| [UiRangeSegments](../Ui/UiRangeSegments.h) | Contiguous labeled segments over one fixed scalar domain. | [UiRangeSegmentsDemo](../examples/UiRangeSegmentsDemo) |
| [UiScrollBar](../Ui/UiScrollBar.h) | Scroll position/extent and themed arrows/thumb. | [UiScrollBarDemo](../examples/UiScrollBarDemo) |
| [UiProgressBar](../Ui/UiProgressBar.h) | Linear determinate/indeterminate progress. | [UiProgressBarDemo](../examples/UiProgressBarDemo) |
| [UiProgressRing](../Ui/UiProgressRing.h) | One amount against a total, circular presentation. | [UiProgressRingDemo](../examples/UiProgressRingDemo) |
| [UiChartRing](../Ui/UiChartRing.h) | Several proportional values composing one ring. | [UiChartRingDemo](../examples/UiChartRingDemo) |
| [UiMatrixSelector](../Ui/UiMatrixSelector.h) | Spatial cell/ordered-pair choice with shared glyphs. | [UiMatrixSelectorDemo](../examples/UiMatrixSelectorDemo) |
| [UiColorMatrix](../Ui/UiColorMatrix.h) | One to eight ordered color values with one shared picker. | [UiColorMatrixDemo](../examples/UiColorMatrixDemo) |
| [UiDateTime](../Ui/UiDateTime.h) | Local date/time/date-time input and picker. | [UiDateTimeDemo](../examples/UiDateTimeDemo) |
| [UiColorPickerMicro](../Ui/UiColorPickerMicro.h) | Compact palette, Apply/hex footer, optional ramps and RGB sliders. [Contract](../Ui/UiColorPickerMicro.md). | [UiColorPickerDemo](../examples/UiColorPickerDemo) |
| [UiColorPicker](../Ui/UiColorPicker/UiColorPicker.h) | Multi-slot color editing, palettes and image/screen picking. | [UiColorPickerDemo](../examples/UiColorPickerDemo) |
| [UiDropdown](../Ui/UiDropdown.h) | Collapsed choice and model-backed popup. | [UiDropdownDemo](../examples/UiDropdownDemo) |
| [UiMenu](../Ui/UiMenu.h) | Command/check/radio/submenu model presentation. | [UiMenuDemo](../examples/UiMenuDemo) |
| [UiPanel](../Ui/UiPanel.h) | Styled surface with ordinary child parenting; use a layout child to arrange content. | [UiPanelDemo](../examples/UiPanelDemo) |
| [UiDirectContentHost](../Ui/UiDirectContentHost.h) | Borrowed single child with independent Fit/Fixed/Expand axes. | [UiLayoutDemo](../examples/UiLayoutDemo) |
| [UiGroupPanel](../Ui/UiGroupPanel.h) | Titled frame with separate header and body root slots. | [UiPanelDemo](../examples/UiPanelDemo) |
| [UiTitleCard](../Ui/UiTitleCard.h) | Title/subtitle/media with an adjacent content cell. | [UiTitleCardDemo](../examples/UiTitleCardDemo) |
| [UiMediaCard](../Ui/UiMediaCard.h) | Optional header/footer around media with non-consuming tags/overlay; shared live/render presentation. | [UiMediaCardDemo](../examples/UiMediaCardDemo) |
| [UiStack](../Ui/UiStack.h) | Exclusive page hosting and measurement. | [UiLayoutDemo](../examples/UiLayoutDemo) |
| [UiAccordion](../Ui/UiAccordion.h) | Collapsible real-child sections with optional reorder. | [UiAccordionDemo](../examples/UiAccordionDemo) |
| [UiScrollPanel](../Ui/UiScrollPanel.h) | Bounded viewport around one content root. | [UiScrollPanelDemo](../examples/UiScrollPanelDemo) |
| [UiTab](../Ui/UiTab.h) | Tabbed page host with role-owned cap and strip fills. | [UiTabDemo](../examples/UiTabDemo) |
| [UiSplitter](../Ui/UiSplitter.h) | Pane sizing with styled split handles. | [UiSplitterDemo](../examples/UiSplitterDemo) |
| [UiQuadSplitter](../Ui/UiQuadSplitter.h) | Four-pane composition over ordinary splitters. | [UiSplitterDemo](../examples/UiSplitterDemo) |
| [UiAbsoluteLayout](../Ui/UiAbsoluteLayout.h) | Exact local child rectangles without automatic reflow. | [UiLayoutDemo](../examples/UiLayoutDemo) |
| [UiGridLayout](../Ui/UiGridLayout.h) | Logical rows and columns with stable placement. | [UiLayoutDemo](../examples/UiLayoutDemo) |
| [UiBoxLayout](../Ui/UiBoxLayout.h) | Ordered row/column flow with Fit/Fixed/Expand. | [UiLayoutDemo](../examples/UiLayoutDemo) |
| [UiList](../Ui/UiList.h) | Sequential model view and visible renderer pooling. | [UiCollectionDemo](../examples/UiCollectionDemo) |
| [UiTree](../Ui/UiTree.h) | Stable hierarchical model identity and visible projection. | [UiTreeDemo](../examples/UiTreeDemo) |
| [UiTable](../Ui/UiTable.h) | Coordinate/range model view with editing and headers. | [UiTableDemo](../examples/UiTableDemo) |
| [UiGallery](../Ui/UiGallery.h) | Tile/image presentation of a list model. | [UiCollectionDemo](../examples/UiCollectionDemo) |
| [UiDoc](../Ui/UiDoc/UiDoc.h) | Document view/editor over the authoritative UiDocCore. | [UiDocDemo](../examples/UiDocDemo) |
| [UiBezierCurveEditor](../Ui/UiBezierCurveEditor.h) | Editable cubic curve with selection and data binding. | [UiBezierCurveDemo](../examples/UiBezierCurveDemo) |
| [UiBezierCurveField](../Ui/UiBezierCurveField.h) | Curve editor with optional formula and copy composition. | [UiBezierCurveDemo](../examples/UiBezierCurveDemo) |
| [UiNodeGraph](../Ui/UiGraph/UiNodeGraph.h) | Retained graph topology, routing, hierarchy and presentation. | [UiGraphDemo](../examples/UiGraphDemo) |
| [UiFileBrowser](../Ui/UiFileBrowser/UiFileBrowser.h) | Optional embeddable file/sequence browser, previews and starter files. [Contract](../Ui/UiFileBrowser/README.md). | [UiFileBrowserDemo](../examples/UiFileBrowserDemo) |
| [UiOsFileDialog](../Ui/UiOsFileDialog/UiOsFileDialog.h) | Native file/folder selection through a platform wrapper. | [UiOsFileDialogDemo](../examples/UiOsFileDialogDemo) |

## Prepared presentation primitives

UiTag is intentionally not a Ctrl. Use UiLabel for ordinary display UI and
UiButton/UiToolButton for independently focusable commands. UiTag is the dense
presentation path for semantic markers such as READY, ERROR, ACTOR, 4K, time
codes, and icon-only information/status cues.

UiTagData may contain text, an icon, or both. `interactive` means the owning
control/view may include that prepared tag in its hit-test/action policy; the tag
itself owns no Event, focus, capture, animation or child-control lifecycle. Stable
`id` plus opaque `value` let the host route information/navigation/actions.

Style uses the normal Ui `StyledPalette` and `StyledMetrics` vocabulary: role/state
face, frame, ink and icon colours; face/frame alpha; frame width/radius; Font;
content margin; one icon/text gap; explicit icon box; Left/Right icon placement;
and `UiIconRenderMode` Auto/MonoTint/PreserveColor. Soft, Filled and Outline are
the compact variants. UiTag deliberately does not carry a nine-slice StyledSkin;
dense tag backgrounds use `UiFill` (solid or image) prepared into the shared
raster cache.

`UiPrepareTag` performs measurement, ellipsis, icon aspect-fit/rescale, state
colour resolution and cached true-alpha face/frame decoration before Paint.
`UiPaintTag` clips to the authored bounds and consumes only prepared resources.
Passive tags share one prepared decoration across pointer states; interactive tags
prepare the states they can enter. This is the intended path for renderer/Graph
scale. UiGraph may adopt it for readable Tags/Actions only after its own LOD/Micro,
bounded-item and 10k-node performance contracts are preserved.

## Edit family

UiBaseEdit provides shared editing, selection, placeholder, caret, side/spin and
style behavior. Concrete Line/Password/Multi/Mask/Int/Float types retain their own
input policy; sharing a base is not proof that specialized validation is identical.
UiEditDemo is the maintained text-family demo; UiIntFloatDemo is its numeric
companion. Their selectors show one concrete control and generate its actual API.

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

## Date/time and color fields

UiDateTime provides local date, time and combined date/time modes with locale or
ISO formatting, optional seconds and 12/24-hour display. It does not perform
time-zone conversion. Editable and presentation-only modes have separate frame
and clipboard policies; read the public header for null and range constraints.
Its picker uses the dropdown arrow artwork with preserved aspect ratio.

UiColorMatrix holds one to eight related colors and opens one UiColorPicker for
the complete set. Theme roles style the surrounding surface; they do not recolor
the authored swatches. Use the picker directly when the application needs its
larger editing surface rather than a compact multi-color field.

## Scalar sliders, intervals and segmented ranges

UiSlider owns one scalar value. UiRangeSlider reuses its style for an ordered
lower/upper interval inside a hard range. SetRange sets the domain, SetStep sets
snap units, SetValues sets the interval, GetLowerValue/GetUpperValue read it.
UiSlider::SetTrackInset chooses a device-pixel end inset for compact composites;
-1 restores automatic spacing, and half the thumb is always reserved. ExpandTrack
uses the available width. GetTrackGeometry reports the painted track for aligned
labels and interaction tests.

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

UiPanel parents ordinary children; UiScrollPanel supplies a Content() parent for
scrolling children. Use one box/grid/absolute layout root to arrange several
children automatically. UiTitleCard borrows an adjacent SetContentCell;
UiGroupPanel has independently replaceable header-content and body-content roots.
Parenting still does not imply deletion ownership.
A derived UiPanel may override the protected ResolveThemeStyle hook to refine
inherited metrics without freezing an explicit style. InvalidateStyleCache once
after derived construction; explicit native style setters and ClearCustomStyle
retain the normal panel contract (as exercised by UiColorPickerMicro).

UiMediaCard is a media-centric presentation rather than a general child host. Header
and Footer are optional prepared text bands; absent content reserves no geometry.
The default card/header/footer surfaces are transparent and frameless, while the
Media surface owns an independent face/frame/radius, so a common tile can show only
a bordered image with free-standing footer text. Top/Bottom UiTag groups and the
3x3-aligned Overlay paint over Media without consuming its layout. Prepared media is
contained to the rounded Media surface, while Header/Footer/Tag paint is clipped to
its authored region. Cover preparation preserves the original Image identity when
using CachedRescale. UiMediaCardRender uses the same prepared UiMediaCardPresentation
through UiItemRender for Gallery/List scale; it does not allocate a child-control
tree per item.

UiMediaCard does not own file, asset or project semantics. WhenDrop forwards a
PasteClip to the host; the host decides what formats to accept and supplies resulting
media. This lets application/demo code implement choose/drop behavior without turning
the generic card into a file picker or viewer.

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

UiScrollPanel's scrolling children belong under Content(). GetViewportRect is the
visible allocation and GetContentSize is the measured content extent. WhenScroll
reports a changed scroll origin after applying it, including programmatic changes;
GetScrollBarAt accepts panel-local coordinates and returns a borrowed scrollbar
for interaction routing. Wheel input uses the horizontal axis when it is the only
available scrollbar, or with Shift when horizontal scrolling is available.

Panel, GroupPanel and ScrollPanel consume the shared Frame Accent metrics:
independent Top/Bottom/Left/Right edge selection, thickness, colour and alpha.
It follows their rounded surface just inside the ordinary frame, preserves
GroupPanel's centered header gaps, and stays fixed around ScrollPanel's viewport.
It is off by default and adds no layout inset. See the
[usage recipe](CONTROL_USAGE.md#frame-accent) and
[theme contract](02_UI_THEME_GUIDE.md#frame-accent).

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

UiList's natural height follows the active model's row count, with at least one
row when empty, plus styling insets. It no longer reserves four placeholder rows.
Constrain the containing layout or viewport when a large list must stay bounded.

UiGallery is a uniform tile view, with a vertical UiItemRenderImage by default.
Use Model().AddRange for bulk content, SetModel for a borrowed shared dataset,
and SetItemRender for an owned clone of a presentation prototype. The model must
outlive its active binding. SetItemSize sets the base size and resets zoom; SetGap,
SetInset and SetOverscanRows configure the grid. Geometry and renderer preparation
stay outside Paint; dirty-region candidates are computed from grid coordinates.
Renderer slots retain overlapping data during scrolling and release surplus
renderers/assets when the useful visible/overscan range shrinks.

WhenVisibleRange(first,last) reports the inclusive overscan range, with (-1,-1)
for an empty range. Prepare lazy assets on the GUI thread and publish edits with
Model().Touch(first,count); external loading/caching belongs to the host. A data
update preserves grid geometry and reconciles only affected selection indices.
Structural edits remap selection and cancel an opening marquee safely. GetData
uses item.data tokens, falling back to indices for Null data; duplicate data keys
resolve to the first selectable match. Single-mode SetData(ValueArray) takes the
first valid match; multi-mode additive Select toggles membership. Selection
notifications are synchronous, including programmatic setters. Escape/capture
loss restores the opening marquee selection and cursor. Null/non-finite zoom
inputs and unrepresentable zoomed tile sizes are ignored. Native geometry uses
32-bit pixel extents; total content height saturates at INT_MAX.

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
