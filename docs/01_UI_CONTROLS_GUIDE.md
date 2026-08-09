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
- Hosts (`UiPanel`, `UiGroupPanel`, `UiScrollPanel`, `UiTitleCard`) normally
  accept zero or one direct child; put a layout inside if more items are needed.
  A second direct content root must be rejected or deliberately wrapped, never
  silently overlapped.

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

> **Retired transitional API:** the former `UiComposite*` property-row family has been removed from the production library. Use primitive `Ui` controls, `UiSliderEdit`, `UiColorMatrix`, and `PropertyEditor` composition instead.

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
- Former Dramatica relationship/pattern APIs (Dynamic, Companion, Dependent,
  U/Z/Butterfly paths) were deliberately removed to keep the control generic;
  do not reintroduce them.

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
`SetMedia`, single-content host with header + content area.

### UiPanel / UiGroupPanel / UiScrollPanel
`UiPanel`: styled single-child host (no flow layout). `UiGroupPanel`: titled
single-content host with header modes and `UiRole` styling. `UiScrollPanel`:
bounded viewport with one scrollable content child.

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
Color picker with palette generation, swatch library, and current/previous slot
previews; the PaletteLab extends it for palette experimentation.

### UiBezierCurveEditor / UiBezierCurveField
Curve editing controls used by the PropertyEditor curve property and custom
surfaces.

### UiMatrixSelector / UiRangeSlider
See the detailed sections above.
