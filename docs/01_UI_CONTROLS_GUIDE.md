# 01 — Ui Controls Guide

Authoritative current control catalogue for the `Ui` library. Controls are
documented from their **current headers/sources**, not from older concept
documents.

## Common control concepts

These concepts apply to every `Ui*` control and are documented once here.

### Sizing and DPI

- Sizing vocabulary: **Fit** (natural/minimum), **Fixed** (exact requested),
  **Expand** (consume distributed space), **cell/cross alignment** (position a
  non-expanded item). Alignment is not sizing.
- `GetMinSize()` / `GetContentSize()` / `Layout()` must agree. Apply `DPI(...)`
  exactly once per value.
- `SetSizeMin(Size)` sets a user minimum; controls honour caps before settling
  final rects.

### Layouts

- `UiBoxLayout` — ordered row/column flow layout with Fit/Fixed/Expand.
- `UiGridLayout` — stable logical row/column placement.
- `UiAbsoluteLayout` — exact local child rectangles, no auto reflow.
- `UiSplitter` / `UiQuadSplitter` — fixed pane-count containers.
- `UiStack`, `UiTab`, accordion section bodies — page/body hosts.
- Host slots accept zero or one direct root; put a layout inside a slot if more
  items are needed. `UiPanel`, `UiScrollPanel`, and `UiTitleCard` have one such
  content slot. `UiGroupPanel` has two distinct single-root slots, header content
  and body content. A second root in either slot must be rejected or deliberately
  wrapped, never silently overlapped.

### Theme/style roles and state

- `StyledPalette` holds four state slots: `ST_NORMAL`, `ST_HOT`, `ST_PRESSED`,
  `ST_DISABLED`, each with `face`, `frame`, `ink`, `icon`.
- `StyledMetrics` holds radius, frame width/visibility, content margin, shadow,
  highlight, focus, and dashed-frame settings.
- `StyledSkin` is the image-backed 9-slice surface; `slice` is drawing-only
  thickness, `content_inset` is geometry-only thickness.
- Controls expose `StyleDefault()`, `SetCustomStyle(...)`,
  `ClearCustomStyle()`, `HasCustomStyle()`, `GetStyle()`. Theme drives defaults;
  explicit `SetCustomStyle` overrides them (see `02_UI_THEME_GUIDE.md`).

### State handling and icons

- `ResolveStyledState(enabled, hot, pressed)` selects the palette state.
- `UiIconRenderMode`: `Auto`, `MonoTint` (tint with ink), `PreserveColor`.
  Icon colour falls back to ink when `icon` is null.
- Focus rings are part of metrics (`focus_enabled`, `focus_color`, etc.).

### Callbacks and values

- `WhenX` events fire after state is committed. `SetData()`/`GetData()`
  participate in data binding; typed `SetValue()`/`GetValue()` where the family
  has that convention.
- Model-backed controls expose request-first `WhenXRequest` events — see
  `03_UI_MODEL_GUIDE.md`.

### Mouse/keyboard

- Pointer: hover/pressed visual state via `LeftDown`, `LeftUp`, `MouseMove`,
  `MouseLeave`. Drag reorder (list/tree/menu/dropdown/accordion) shows an
  explicit handle, side placement, and an insertion marker; hover never mutates
  the document.
- Keyboard: `Key()` handles arrows/tab/enter as appropriate; controls are
  focusable and draw a focus ring.

### Shadows/elevation

- `StyledShadow` (in `StyledMetrics`) supports enabled, distance, offset,
  alpha, color, inset, and a Bézier `ShadowCurve` (`ShadowSoft`, `ShadowTight`,
  `ShadowLinear`, `ShadowGamma`). Shadow margins are included in
  `UiStyledOuterSizeFromContent`.

> **Retired transitional API:** the former `UiComposite*` property-row family has
> been removed from the production library. Use primitive `Ui` controls,
> `UiSliderEdit`, `UiColorMatrix`, and `PropertyEditor` composition instead.

---

## UiMatrixSelector

Compact styled matrix selector for position, direction, region, and ordered
two-cell choices. One `Ctrl`; no child buttons or parallel drawing system.

### Purpose and appropriate usage

Use it when the user must choose a cell from a small spatial grid: 3x3 position,
compass 8, region map, or an ordered pair of cells (start/end). Reuses the shared
`Ui` palette/metrics/skin primitives and owns matrix geometry, hit testing,
glyphs, readout, pair arrow, and default indication.

### Dimensional/configuration API

```cpp
enum class UiMatrixPreset { Position9, Compass8, Region5, QuadPair };
UiMatrixSelector m;
m.SetPreset(UiMatrixPreset::Position9);      // loads the cell grid
m.SetSelectionMode(UiMatrixSelectionMode::SingleCell); // or Pair
int rows = m.GetRows();                       // 3 for Position9
int cols = m.GetColumns();
int n = m.GetCellCount();
```

`SetPreset` configures rows/columns and cell labels. `Cell` carries
`short_label`, `label`, `Value value`, `Image icon`, `glyph`, `visible`,
`enabled`.

### Labels/options

- `SetCell(int index, short_label, label, value)`
- `SetCellLabel`, `SetCellValue`, `SetCellIcon`, `SetCellGlyph`, `EnableCell`,
  `ShowCell`.
- `SetRole`/`SetSelectedRole`/`SetReadoutRole` (semantic `UiRole`) select theme
  defaults for the surface, selected cells, and readout.

### Selection/value behaviour

```cpp
m.SelectIndex(2, /*fire_action=*/true);      // single-cell selection
int sel = m.GetSelectedIndex();
String label = m.GetSelectedLabel();
Value data = m.GetData();                     // SetData/GetData binding
```

Pair mode: `SetPair(first, second)`, `GetPairStartIndex()`,
`GetPairEndIndex()`, `HasCompletePair()`, `GetPairOrientation()`,
`GetPairOrientationName()`, `GetPairDirectionLabel()`, `GetReadoutText()`.
`SetPair(..., true)` fires `WhenAction`.

### Default indication

`SetDefault(index)`, `ClearDefault()`, `ShowDefault(bool)`,
`IsDefaultSelected()`. The default cell is drawn with a dashed frame so the
recommended/fallback cell is visible without being selected.

### Interaction

- Click selects the cell under the pointer (`HitTest`).
- Arrow keys move selection to the next enabled cell; `WhenChanging` fires on
  preview, `WhenAction` on committed activation.
- `SetCellGap`, `SetCellRadius`, `ShowCellFace`, `ShowCellFrame`,
  `SetGlyphInset`, `SetIconInset` tune cell look.

### Layout/sizing

`GetMinSize()` derives from cell count, cell metrics, readout width and gap.
`SetSizeMin(Size)` overrides the user minimum. `GetMatrixRect()`,
`GetReadoutRect()`, `GetCellRect(int)` expose geometry for custom paint.

### Readout

`ShowReadout(bool)`, `SetReadoutWidth(int)`, `SetReadoutGap(int)`,
`SetReadoutFont`, `ShowReadoutFace`, `ShowReadoutFrame`,
`SetReadoutRadius`. The readout prints the current selection/pair text.

### Non-obvious capabilities

- `UiMatrixGlyph` draws one of eight direction arrows or a dot inside a cell,
  useful for compass/direction UIs.
- Pair mode draws a direction-preserving arrow between the two selected cells
  and reports orientation (`Horizontal`/`Vertical`/`Diagonal`).
- The control is fully theme-role driven (`SetRole`) so it follows light/dark
  and preset changes without hard-coded colours.
- Former domain-specific relationship/pattern APIs were deliberately removed to
  keep the control generic; specialised domain visualisers should be separate
  controls rather than expanding this API.

---

## UiColorMatrix

Styled multi-colour value field for editing a small related set of colours as one
contiguous control. The current public capacity is one through eight colours,
matching `UiColorPicker`'s current multi-slot editing contract.

### Purpose and appropriate usage

Use `UiColorMatrix` when a property is naturally a colour set rather than a
single colour: a four-colour theme group, a compact palette, paired colours, or
similar small sets. It is a first-class control, not a generic property-row
wrapper.

```cpp
UiColorMatrix colors;
colors.SetColorCount(4)
      .SetColor(0, Color(40, 120, 230))
      .SetColor(1, Color(240, 190, 45))
      .SetColor(2, Color(45, 180, 105))
      .SetColor(3, Color(215, 70, 80));
```

`SetColors(const Vector<Color>&)` replaces the current set in one operation.
`GetColors()` returns the typed vector. `SetColorLabel` supplies per-slot labels
used by the picker.

### Contiguous editing and picker behaviour

Activating any swatch makes it current and opens **one** `UiColorPicker` loaded
with the entire colour set. The active swatch becomes the picker's active slot.
Live picker changes update the matrix through `WhenChanging`; accepting commits
the complete set and fires `WhenAction`; cancelling restores the complete opening
set.

`EnablePicker(false)` makes the field selection-only. `SetPickerTitle()` controls
the modal picker title and `EditColors()` opens it programmatically.

### Adaptive layout and wrapping

The matrix does not assume a single horizontal strip. `ResolveGrid()` evaluates
possible column counts against the actual inner width and height, chooses the
largest square swatch size that fits, and uses the arrangement with least empty
cells as the tie-breaker. This means a constrained control can wrap two, four,
or eight swatches onto additional rows rather than clipping them.

`GetSlotRect(index)` exposes the settled geometry and `HitTest(point)` uses the
same rectangles for interaction.

### Styling

Swatch **faces are always the actual colours**. Surrounding visuals use the
normal Ui style primitives:

- `SetSlotGap(int)` — gap between swatches;
- `SetSlotRadius(int)` — rounded swatch corners;
- `SetSlotFrameWidth(int)` / `ShowSlotFrame(bool)` — standard frame contract;
- `SetSlotShadow(bool)` — standard `StyledShadow` on each swatch;
- `SetSurfaceRadius(int)`, `ShowSurface(bool)`, `ShowSurfaceFrame(bool)`,
  `SetSurfaceShadow(bool)` — containing surface treatment;
- `SetMinimumSlotSize(int)` / `SetMaximumSlotSize(int)` — adaptive-grid bounds;
- `SetRole(UiRole)` / `SetActiveRole(UiRole)` — semantic theme roles.

Default theme resolution uses `UiPanel` styling for the surrounding surface and
`UiButton` styling for slot/active frame state, so light/dark and theme revisions
follow the rest of the library. A custom `Style` can override the same shared
`StyledPalette`, `StyledMetrics`, and `StyledSkin` values without introducing a
special painting system.

### Value/model and interaction contract

- `SetData(Color)` / `GetData()` use a scalar `Color` when the matrix has one
  value.
- Multi-colour data uses `ValueArray` of `Color` values.
- Arrow keys move the active swatch according to the settled grid.
- Enter/Space opens the picker.
- `WhenSelect(int)` reports active-slot changes; `WhenChanging` is live colour
  editing; `WhenAction` is committed colour-set editing.

The current eight-colour maximum is deliberately aligned with the picker. If the
picker's slot contract grows later, the capacity can be widened without changing
the adaptive layout model.

---

## UiRangeSlider

Styled two-handle slider for selecting an ordered interval (lower/upper) inside a
scalar range.

### Purpose and appropriate usage

Use it when the user must pick a range (e.g., min/max, start/end, zoom window)
rather than a single value. It reuses `UiSlider::Style`, so handles, ticks, and
the selected track stay visually aligned with the standard slider.

### Domain and value API

```cpp
UiRangeSlider r;
r.SetRange(0.0, 100.0);          // scalar domain
r.SetStep(1.0);
r.SetValues(20.0, 80.0);         // ordered interval (lower, upper)
double lo = r.GetLowerValue();
double hi = r.GetUpperValue();
double mn = r.GetMin();
double mx = r.GetMax();
```

`SetMin`/`SetMax` adjust one bound. `SetStart`/`SetEnd`/`SetStartEnd` are
animation-friendly aliases for lower/upper — one authoritative pair of values,
not a second state model.

### Clamping/order semantics

`NormalizeValue` clamps to `[min_, max_]` and snaps to `step_`. Values are
normalized so `lower <= upper`; the active handle determines which bound moves
during drag. `SetActiveHandle(Handle::Lower|Upper)` picks the keyboard/wheel
target.

### Dragging and callbacks

- Drag either thumb; `PickHandle(point)` selects the nearest handle. `WhenChanging`
  fires on live edits, `WhenAction` on the committed drag release.
- Mouse wheel nudges the active handle; arrow keys adjust it.

### Orientation

`SetDirection(UiDirection::H|V)`; default horizontal.

### Ticks, track, and thumb

```cpp
r.SetTicks(true, /*major=*/10, /*minor_per_major=*/0);
r.SetTickSide(UiAlign side);
r.SetTrackSize(Size);
r.SetThumbSize(Size);
```

### Styles/theme

`UiRangeSlider` inherits `UiSlider::Style` and the `CtrlStyled` mixin:
`StyleDefault()`, `SetCustomStyle`, `ClearCustomStyle`, `GetStyle`,
`StyledPaletteRef()`, `StyledMetricsRef()`, `StyledSkinRef()`, `OnStyleChanged()`.
It follows the theme slider role for track/thumb palettes.

### Sizing

`GetMinSize()` derives from track/thumb sizes and direction. `SetSizeMin(Size)`
/ `SetSizeFixed(Size)` set the user minimum. `WhenPaintBackground` /
`WhenPaintForeground` allow custom painting of the track surface and the
selected interval.

### Keyboard behaviour

`Key()` adjusts the active handle by step; focus ring is drawn per metrics.

### Non-obvious capabilities

- `SetData`/`GetData` bind the interval as a `Value` (a pair), so the control
  participates in model/PropertyEditor integration.
- Reuses the standard slider theme, so dark mode and preset changes apply
  consistently to both handles and the selected track.

### Adjustable bounds and endpoint markers

`EnableAdjustableBounds()` adds a second ordered pair inside the hard domain.
The four authoritative values are lower bound, lower selection, upper
selection, and upper bound. `SetBounds`, `GetLowerBound`, and `GetUpperBound`
edit that inner domain without changing the hard `SetRange` limits. Bound
handles remain independently draggable and keyboard accessible through
`Handle::LowerBound` and `Handle::UpperBound`.

`ShowEndpointMarkers(bool)` controls the small start/end domain markers. They
are visible by default so a light track still has legible limits.

### UiRangeSliderEdit

`UiRangeSliderEdit` composes the authoritative `UiRangeSlider` with fixed-width
lower/upper `UiFloatEdit` fields. The slider expands into all remaining space;
`SetFieldWidth`, `SetGap`, and `SetInset` control only the composition geometry.
It preserves the same two-element `ValueArray`, `WhenChanging`, and
`WhenAction` contract as `UiRangeSlider`. Use `Slider()` when the composition
must enable adjustable bounds or tune endpoint markers.

---

## UiFloatEdit

Typed floating-point entry built on `UiBaseEdit`. It accepts ordinary decimal
and scientific notation, including signed exponents such as `-1.25e+6`.

```cpp
UiFloatEdit value;
value.MinMax(-1000.0, 1000.0)
     .Step(0.25)
     .Precision(3)
     .ShowSpin(true);
value.SetValue(12.5);
```

- `Min`, `Max`, and `MinMax` clamp committed values.
- `Step` drives spin buttons, Up/Down keys, and the mouse wheel.
- `Precision` controls normalized display formatting after commit/focus loss.
- `TryGetValue(double&)` and `IsInputComplete()` distinguish incomplete or
  invalid text from a valid finite value without forcing a destructive rewrite.
- `NotNull` controls whether an empty value is permitted.
- `SetData` accepts numeric values and numeric text; `GetData` returns the typed
  floating-point value or Null when empty values are allowed.

The editor permits temporarily incomplete text while typing and normalizes only
complete values. This is important for exponent entry and PropertyEditor
transactions: validation must not treat a partial mantissa or exponent as a
committed number.

---

## UiDateTime

Styled local date/time field that can represent a date, a time, or a combined
local date-time while keeping one authoritative U++ `Time` value.

### Modes and formatting

```cpp
UiDateTime field;
field.DateMode();                         // date only
field.TimeMode();                         // time only
field.DateTimeMode();                     // combined
field.SetFormatStyle(UiDateTimeFormatStyle::Locale); // or Iso
field.SetClockFormat(UiClockFormat::Locale);         // or Hour12 / Hour24
field.ShowSeconds(true);
```

Locale presentation uses U++ language information; ISO mode uses deterministic
ISO-style date/time formatting. `SetLanguage(int)` or `SetLanguage(const char*)`
selects the language context. `SetFirstDayOfWeek()` configures calendar layout.

The V1 value is deliberately **local/naive time**. Time-zone conversion and
instant/offset semantics belong outside this control.

### Editable and presentation modes

`SetEditable(true)` provides text editing and the picker button.
`SetPresentation(true)` makes the value read-only and hides the picker button.
Presentation styling is intentionally chromeless by default; callers can opt
back into a framed read-only surface with `ShowPresentationFrame(true)`.

### Picker behaviour

The picker follows the active mode:

- Date → themed U++ `Calendar`;
- Time → themed U++ `Clock`;
- DateTime → combined `CalendarClock`.

Picked values update the same authoritative `Time`. There is no second popup
model. The calendar/clock surfaces are restyled from the active Ui theme so they
remain visually coherent in light and dark modes.

### Clipboard policy

Copy and paste are independently controllable:

```cpp
field.AllowCopy(true);
field.AllowPaste(false);
field.CopyValueToClipboard();
field.PasteValueFromClipboard();
```

Keyboard copy/paste/cut paths honour the same policy. Paste additionally requires
editable mode. This allows a presentation field to remain selectable/copyable
without silently becoming editable.

### Validation, nulls, and ranges

- `AllowNull(bool)` / `ClearValue()` govern empty values.
- `SetRange(Time minimum, Time maximum)` and `SetDateRange(Date, Date)` constrain
  accepted values; `ClearRange()` removes bounds.
- `CommitText()` parses and validates the complete edit; invalid input restores
  the formatted stored value and fires `WhenInvalid(String)` rather than
  corrupting state.
- Date, time, and date-time parsing all verify real calendar/time ranges.

Typing is intentionally permissive rather than governed by a rigid input mask;
commit-time parsing provides the authoritative validation so locale, ISO, 12/24
hour entry, and pasted values remain practical.

### Data and events

`SetValue(Time)` / `GetValue()` are the canonical typed API. Convenience methods
include `SetDate`, `GetDate`, `SetTime`, `SetNow`, and `SetToday`.
`SetData`/`GetData` integrate with U++ data binding.

`WhenChanging` reports a valid live value change; `WhenAction` reports a committed
change; `WhenInvalid` reports rejected text; `WhenOpenPicker` reports picker
activation.

### Styling

`UiDateTime::Style` contains the editable edit style, presentation edit style,
picker-button style, and minimum/button metrics. Theme defaults resolve through
`UiTheme::ResolveEdit` and `ResolveToolButton`; explicit custom styles remain
caller-owned.

Known V1 rule: `ShowSeconds()` is part of the public contract and must be honoured
consistently by all formatting paths. Locale-specific presentation should be
validated whenever language/time-format behaviour is changed.

---

## UiGroupPanel

Framed titled section with two explicit single-root slots:

```cpp
UiGroupPanel group;
UiBoxLayout header_actions;
UiGridLayout body;

group.SetTitle("Account")
     .SetSubTitle("Identity and access")
     .SetHeaderContent(header_actions)
     .SetContent(body)
     .SetTitleAlign(UiAlign::LEFT, UiAlign::CENTER)
     .SetHeaderContentAlign(UiAlign::RIGHT, UiAlign::CENTER);
```

`SetHeaderContent` owns the optional header accessory relationship and
`SetContent` owns the body relationship. Each accepts exactly one root `Ctrl`;
put a box/grid/absolute layout in the relevant slot when multiple controls are
needed. Replacing or clearing one slot does not affect the other. Moving the same
control between slots is supported, and external child removal clears the stored
slot pointer through `ChildRemoved`.

### Header geometry

`SetHeaderPlacement` supports Top, Bottom, Left, and Right.
`SetHeaderMode` supports Outside, Center, and Inside. The built-in identity block
contains icon, title, and subtitle. Header content uses the available region on
the opposite side of that block:

- a Left/Top title places content in the trailing Right/Bottom region;
- a Right/Bottom title places content in the leading Left/Top region;
- a centered title retains the centre and content uses the trailing remainder.

`GetHeaderContentRect()` returns that authoritative prospective region even when
the slot is empty. An attached child normally keeps its measured minimum size;
`SetHeaderContentAlign` positions it within the region. `GetBodyRect()` remains
the authoritative body rectangle.

`GetMinSize()` includes the title identity, header child, configured gaps and
insets, body minimum, and styled surface geometry. Forced undersize layouts
clamp without negative rectangles or title/header-child overlap.

### Frame and style behavior

In Center mode the styled frame is segmented around both occupied header
rectangles for every placement; the implementation does not draw a line beneath
the title or header child. Header children retain their own styles.

The retired text-only `SideTitle` API is replaced by ordinary header content.
Legacy SideTitle style fields are still consumed in their historical stream
positions solely to preserve serialized style compatibility; they no longer
affect runtime measurement, painting, or theme resolution.

---

## Other substantial controls (quick reference)

All entries below follow the common concepts above; read the matching header for
the exact API.

### UiLabel
Styled text display with selection, alignment, icon/media, wrapping, and richer
presentation options. `SetText`/`GetText`, `SetIcon`/`SetIconSide`,
`SetAlignH`/`SetAlignV`, roles via `SetRole(UiLabelRole)` (Body, Headline,
Subheadline, Title, Caption, Badge, Footnote). Semantic label roles are
geometry-neutral.

### UiButton / UiToolButton / UiSplitButton
Stateful buttons with `UiRole`/`UiButtonRole` (Standard, Accent, Subtle, Icon,
Danger). `SetText`, `SetIcon`, `SetIconSide`, `SetRole`, `WhenAction`.
`UiToolButton` is the compact toolbar form; `UiSplitButton` exposes a primary
action plus a dropdown.

### UiTitleCard
Structured title/subtitle/media header surface. `SetTitle`, `SetSubTitle`,
`SetMedia`, and one adjacent content-cell root through `SetContentCell`. It is
an information/header composition rather than a framed two-slot section.

### UiPanel / UiGroupPanel / UiScrollPanel
`UiPanel`: styled single-child host (no flow layout). `UiGroupPanel`: framed
two-slot titled section described above. `UiScrollPanel`: bounded viewport with
one scrollable content child.

### UiBaseEdit family (UiLineEdit, UiIntEdit, UiFloatEdit, UiPasswordEdit,
UiMultiEdit, UiMaskEdit)
Text/number entry with `UiEditRole` (Field, Subtle, Strong), validation,
placeholder, selection, spin toggles, and `WhenAction`/`WhenChange`.

### UiCheckBox / UiRadioButton / UiToggle
Boolean controls with `UiCheckVisual` / `UiRadioVisual` variants (Classic, Chip,
List, Pills) and `WhenAction`.

### UiSlider / UiSliderEdit / UiProgressBar / UiScrollBar
`UiSlider`: single-handle value slider (the RangeSlider shares its style).
`UiSliderEdit`: slider + numeric edit composition. `UiProgressBar`: determinate
and indeterminate progress with role-tuned fills. `UiScrollBar`: themed
scrollbar with arrow layouts and thin/thick modes.

### UiAccordion / UiTab / UiStack
Page/body containers. `UiAccordion`: collapsible sections with drag reorder and
explicit handle. `UiTab`: tabbed pages. `UiStack`: page stack host.

### UiSplitter / UiQuadSplitter
Fixed pane-count containers with themed thumb and pane sizing.

### UiDropdown / UiList / UiTree / UiMenu / UiTable
Model-backed controls with request-first mutation (`WhenReorderRequest`,
`WhenMoveRequest`, `WhenActionRequest`, `WhenEditRequest`) and
`EnableInternalMutation(bool)` — see `03_UI_MODEL_GUIDE.md`.

### UiBreadcrumbs / UiDoc
`UiBreadcrumbs`: path/navigation display with optional icons and dividers.
`UiDoc`: rich document display/editor surface with annotation lanes
(`UiDoc::AnnotationLane`) for comments/metadata.

### UiColorPicker / UiColorPickerPaletteLab
Large multi-slot colour editor now contained under `Ui/UiColorPicker/`. It supports
one through eight active slots, alpha, multiple colour models/readouts, palette
library/generation, image analysis, user stash/session state, and screen picking.
`SetSlotCount`, `SetActiveSlot`, `SetSlot`, and `GetSlots` are the authoritative
ordered-slot API; a one-colour caller must request one slot rather than relying
on a later refresh. `WhenChanging` previews, `WhenAction` commits edits,
`WhenAccept` accepts the dialog, and `WhenCancel` restores the opening values.
Generated, static, image, stash, and primary swatches share multi-selection and
ordered drag transfer, so moving a palette preserves slot order.
`UiColorPickerPaletteLab` contains the reusable colour conversion, palette,
generator, and image-analysis support for that component.

### UiBezierCurveEditor / UiBezierCurveField
Curve editing controls used by the PropertyEditor curve property and custom
surfaces.

### UiMatrixSelector / UiColorMatrix / UiRangeSlider / UiDateTime
See the detailed sections above.
