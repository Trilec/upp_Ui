# PropertyEditor Visual Design

Version: **1.0.0**

## Visual package responsibilities

- browser rendering and row painting;
- active-editor virtualization;
- editor factory and custom editor registration;
- `Ui` control mapping for built-in value editors;
- theme-aware browser/editor presentation;
- groups, filtering, selection, keyboard interaction;
- popup/dialog editors and commit/preview semantics;
- application-supplied custom editors.

## Editor creation

`PropertyEditorFactory` is the single editor-creation authority for built-in
and custom editors. `PropertyValueEditors.cpp` contains the built-in semantic
kind mapping; `PropertyV1Editors.cpp` registers the first-class Range, Matrix,
Icon, Font and Image adapters through that same factory. There is no parallel
advanced-editor path.

The factory also owns provider registrations. Picker callbacks authorize and
return application-defined resource values. Optional thumbnail callbacks map a
value to an `Image` for compact presentation. Providers keep project browsers,
file loading and SymbolPicker outside this package.

Curve editors use `PropertyEditorItem::editor_variant` to select compatible
value contracts. The default polyline contract stores point pairs. The
`bezier` contract stores four cubic control values and the visual factory hosts
`UiBezierCurveEditor`, preserving the compact animation/easing representation
without adding a second curve renderer. X is normalized to `[0, 1]`; Y may
overshoot unless the property supplies explicit minimum and maximum bounds.

## Boundaries

- `PropertyEditorCore` owns the schema, normalization, validation, and revision tracking;
- the visual package owns the live browser, delegates, and user interaction;
- the visual package should not re-implement model rules that already live in core.

`PropertyEditorCore` depends only on Core and Draw. It may describe an editor
using semantic strings such as `custom_editor`, `editor_variant`, and
`picker_provider`, but it must not include `Ui`, control implementations, the
Designer, or resource-picker applications.

## Rows and virtualization

- Filter remains fixed above the scrolling viewport and updates live.
- Nested group collapse state is retained while a filter temporarily reveals
  matching descendants.
- Label sizing supports Auto, Fixed and Ratio, with a draggable divider and
  double-click restoration to Auto.
- Row geometry is `row_height * row_span`; adapters request one, two or three
  lines through metadata rather than hard-coded control heights.
- `expanded_row_span` declares optional rich-editor capacity without storing
  transient view state in the model. `PropertyEditor` owns expansion by
  property id and defers callback-triggered row reconstruction until the next
  GUI turn so an editor is never destroyed inside its own action callback.
- Inline editors exist only for visible rows plus one-row overscan. Layout and
  paint never create editors for the complete model.
- Group override summaries are aggregated during row rebuild and cached in the
  display rows; Paint does not scan the complete model.

## Transactions

The first preview for a property captures value, mixed and inherited state.
Commit normalizes through Core and ends the transaction. Escape restores the
captured origin and emits `WhenCancel`. `Ctrl+Z` only emits
`WhenUndoRequest(property_id)`; undo ownership remains with the host.

Structure replacement clears active and inline editors before model rows are
discarded, preventing stale focus callbacks from addressing old items. This
ordering is a protected regression contract.

## Performance contract

Filtering and structural rebuilds may scan the model once. Paint is render-only
and proportional to visible rows. Scrolling updates the bounded inline-editor
window. Deterministic acceptance exercises 1,000 properties with repeated
filtering, scrolling, expand/collapse, replacement and detachment; elapsed-time
thresholds are intentionally left to Release-mode smoke testing.
