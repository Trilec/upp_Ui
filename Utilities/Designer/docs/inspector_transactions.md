# Inspector Transactions

This note documents current ownership for Designer single-node inspector edits.

## Ownership

`DesignerInspector`

- Builds inspector row controls.
- Emits `DesignerInspectorEditIntent`.
- Does not mutate `DesignerModel` directly.

`DesignerWindow`

- Owns `DesignerModel`.
- Owns `DesignerCommandStack`.
- Owns current synchronous single-node inspector transaction execution.
- Validates intents, applies preview/model patches or durable commands, then applies projection and readback.

`DesignerTrace`

- Optional diagnostics only.
- Must not affect behaviour.
- Intended to be disabled during normal editing unless transaction investigation is needed.

`upp_statemachine`

- Reusable core FSM package.
- Not currently authoritative for single-node inspector edits.
- Reserved for a future `DesignerCoordinator` once broader Designer workflows are unified under one explicit coordinator.

## Current Single-Node Flow

Preview edit:

1. Inspector row emits `DesignerInspectorEditIntent`.
2. `DesignerWindow::SubmitInspectorIntent()` accepts the intent.
3. `DesignerWindow` runs preview work synchronously.
4. Preview projection updates preview only; inspector rebuild is deferred until final commit.

Final commit:

1. Inspector row emits `DesignerInspectorEditIntent`.
2. `DesignerWindow::SubmitInspectorIntent()` validates and builds a pending transaction.
3. `DesignerWindow` executes the command synchronously.
4. Projection and inspector/model readback complete in the same transaction path.

## Important Constraint

The real `designer_fsm_` is intentionally not the owner of single-node inspector preview/commit transactions in the current stabilisation phase. Re-entrant GUI projection and slider preview behavior require deterministic same-call completion. Reintroduce FSM ownership only after the broader Designer coordinator is explicit and proven stable.

## Smoke Checklist

`UiTitleCard`

- `role`
- `h_sizing`
- `fixed_width`
- `v_sizing`
- `fixed_height`
- `icon`

`UiScrollPanel`

- `h_sizing`
- `fixed_width`
- `v_sizing`
- `fixed_height`

`UiLineEdit`

- `h_sizing`
- `v_sizing`
- `min_height`
- `placeholder/text`

Expected for each edit:

- model persists
- inspector readback matches
- preview changes
- reselect shows same values
- actual FSM remains `Idle`
