# 03 — Ui Model Guide

Model-driven architecture for `upp_Ui`, represented by `PropertyEditorCore`,
`PropertyEditor`, and the model-backed controls (`UiList`, `UiTree`, `UiMenu`,
`UiDropdown`, `UiTable`, and data-presenting controls).

For the complete PropertyEditor schema, factory, adapter, transaction,
resource-provider, layout, and performance contracts, continue with
`05_UI_PROPERTY_EDITOR_GUIDE.md`.

## Model/view separation

- The **control owns interaction**: hit testing, keyboard/pointer input, drag
  threshold, insertion markers, hover/pressed/focus visuals, local selection
  visuals.
- The **owner/model/command layer owns semantic state changes**: validity,
  undo/redo, persistence, business rules, generated-output refresh, and
  cross-view synchronization.
- A view model is a projection, never the source of truth for application-owned
  data.

## Request-first mutation contract

For any data-changing operation the control computes and reports user intent
before mutating authoritative data:

1. The user performs an operation.
2. The control computes the proposed target.
3. The control emits a request event (e.g. `WhenReorderRequest`).
4. Rejected → nothing changes. Handled → the owner performed/scheduled the change.
   Unhandled and internal mutation enabled → the control mutates its local model.
5. A post-change notification may fire after data changed.

Shared request structs live in `UiDataModels.h`:

- `UiReorderRequest` — move within the same list/dropdown.
- `UiTreeMoveRequest` — move to a new parent/container.
- `UiMenuActionRequest` — menu action.
- `UiTableEditRequest` — cell/table edit.

Implemented request hooks: `UiList::WhenReorderRequest`,
`UiDropdown::WhenReorderRequest`, `UiTree::WhenMoveRequest`,
`UiMenu::WhenActionRequest`, `UiTable::WhenEditRequest`.

Internal mutation is explicit and supported for demos/local widgets:

```cpp
control.EnableInternalMutation(true);   // simple/local mode
```

Command-driven tools use request-first with internal mutation disabled:

```cpp
control.EnableInternalMutation(false);
control.WhenReorderRequest = [=](UiReorderRequest& r) {
    r.handled = true;
    Dispatch(MoveNodeCommand(r.from, r.before));
};
```

The default implementation path must remain request-first. "Mutate silently,
notify afterward" is not the target architecture.

## Ownership and lifetime

- The model is owned by the application; the control only references it.
  `PropertyEditor::SetModel(PropertyEditorModel*)` takes a non-owning pointer.
- `PropertyEditorModel` is headless (no `Ctrl` dependency beyond `Draw`) and
  thread-neutral as a data structure; mutation is expected on the GUI thread
  unless the owner synchronizes.

## Values, IDs, and labels

- `PropertyEditorItem` has a stable `id` (programmatic key), a human `label`,
  an optional `group`, `help`, and `unit`.
- IDs are the canonical key for `SetValue`/`GetValue`/`Find`. Labels are for
  display. Never match on labels in logic.

## Current/selection state

- `PropertyEditorItem.value` is the current value; `default_value` supports
  reset. `mixed` and `inherited` flags describe aggregate rows.
- `PropertyEditor::GetSelectedPropertyId()` / `SelectProperty(id)` track the
  selected row; `WhenSelection` reports it.

## Read/write propagation and callbacks

`PropertyEditorModel` emits:

- `WhenStructureChanged` — items added/removed/reordered.
- `WhenValueChanged(id)` — a value changed.
- `WhenPreview(id, value)` — an editor previewed a candidate.
- `WhenCommit(id, value)` — an editor committed a candidate.
- `WhenReset(id)` — a value reset to default.
- `WhenGroupMetadataChanged` — group subtitle changes.

`PropertyEditor` forwards user interaction through `WhenPreview`, `WhenCommit`,
`WhenReset`, `WhenOverride`, `WhenSelection`, `WhenHelp`.

## Validation

- `PropertyEditorItem.normalize` (Function) and `validate` (Function) are
  optional per-item hooks.
- `Preview(id, candidate, error)` and `Commit(id, candidate, error)` route
  through `PropertyEditorNormalizeValue` before applying.
- `SetValidationError(id, error)` surfaces a per-property error.
- Validate enum values and clamp ranges when loading persisted values.

## Commit/revert semantics

- `Preview` applies a candidate without committing (live inspector feedback).
- `Commit` finalizes; `Reset` restores the default value.
- `WhenPreview`/`WhenCommit` events let the owner stage/undo as needed.

## Avoiding feedback loops

- Do not call `SetValue` from inside a `WhenValueChanged` handler for the same
  id without a guard.
- `PropertyEditor` uses internal flags (`syncing_editor_`,
  `applying_editor_preview_`, `dispatching_editor_callback_`) to avoid
  re-entrant commits while an inline editor is active.
- One authoritative source of state: the model (or the application-owned model
  behind it). The view refreshes from the model; it never becomes the source of
  truth.

## PropertyEditorCore responsibilities

- Headless property schema + value model (`PropertyEditorModel`,
  `PropertyEditorItem`).
- Property kinds (`PropertyEditorKind`): Text, Multiline, Integer, Double,
  NumericInt, NumericDouble, Boolean, Choice, Color, ColorPalette, FillRecipe,
  FilePath, SliderInt, SliderDouble, Vector2, Vector3, Curve, ReadOnly, Custom.
- Domains (`PropertyEditorDomain`): General, Content, Behaviour, Layout,
  Appearance, Theme, Runtime, DesignerOnly.
- Impact bits (`PropertyEditorImpact`): Paint, ControlState, LocalLayout,
  AncestorLayout, Subtree, Structure, Selection, InspectorSchema, Code,
  ThemeGlobal, FullPreview — used to decide what to refresh when a property
  changes.
- Value/vector/curve helpers: `PropertyEditorMakeVector`, `PropertyEditorReadVector`,
  `PropertyEditorMakeCurve`, `PropertyEditorNormalizeCurve`, etc.

## PropertyEditor responsibilities

- The `Ui`-backed `ParentCtrl` that renders the model, group rows, filter box,
  reset/override actions, inline editors, and the value summary.
- `SetModel(PropertyEditorModel*)`, `SetFactory(PropertyEditorFactory*)`,
  `SetStyle(PropertyEditorStyle)`, `SetPaletteMode(...)` (FollowUiTheme/Light/
  Dark).
- `ShowFilter`, `SetFilter`, `ExpandAll`, `CollapseAll`, `RefreshModel`,
  `RefreshValue(id)`, label width/ratio controls.

## Adding model-backed properties/editors

1. Add the item to the model:
   ```cpp
   model.AddNumericInt("row_height", "Row height", 28, 8, 64, 1, "Layout")
        .SetHelp("Height of each row.");
   ```
2. Choose the right `PropertyEditorKind`; for choices use `AddChoice` +
   `AddChoice(value, label)`.
3. Set impact so the owner knows what to refresh
   (`item.SetImpact(PropertyImpactLocalLayout)`).
4. Wire `WhenPreview`/`WhenCommit` to apply the property to the live object and
   `WhenReset` to restore the default.
5. For custom editors implement a `PropertyValueEditor` via the factory.

## Integrating a control into PropertyEditor

- Expose the control's editable state through typed setters that the inspector
  can call (SetX/GetX pairs).
- Advertise a style/theme override surface for appearance properties — see
  `02_UI_THEME_GUIDE.md`.
- Do not mutate the control directly from `WhenValueChanged` in a way that
  re-enters the model; route through preview/commit and refresh the view.

## Common implementation mistakes

- Matching items by label instead of id.
- Mutating a control from inside its own change callback (feedback loop).
- Using the view as the source of truth for application-owned data.
- Forgetting `impact` so the owner cannot decide what to refresh.
- Skipping validation/normalization for enum or range-backed values.
- Mixing `WhenX` post-change notifications with authorization; notifications are
  observations, requests are authorization.
