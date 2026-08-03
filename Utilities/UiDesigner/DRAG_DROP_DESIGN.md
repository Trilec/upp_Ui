# UiDesigner drag-and-drop contract

## Release gate: Stage 0 nested catalog-drag stability

This gate must pass before any semantic drop-region visuals are added or tuned.
The known failure mode is a nested `Grid -> Box -> Grid` hierarchy where dragging
another catalog layout can leave the cursor captured while selection and normal
input stop responding. A visible cursor is not proof that the drop system is alive;
sometimes it is just holding the mouse hostage.

### Required diagnosis

Reproduce the nested drag under the debugger and record the active GUI-thread stack,
capture owner, drag type, resolved target, Preview geometry generation, document
generation, and whether `TrackCatalogDrag`, target resolution, `Layout`, or
`RebuildDocument` is recursively active. Distinguish capture leakage, recursive
tracking/rebuild, stale snapshot state, and excessive synchronous work. Do not fix
this by adding an unconditional `ReleaseCapture()` without identifying the cause.

### Drag lifecycle contract

Catalog dragging has one lifecycle:

```text
Idle -> Tracking -> Completing -> Idle
Idle -> Tracking -> Cancelling -> Idle
```

The lifecycle owns capture, drag type, resolved region, plan, indicator, status, and
the document/geometry generations used by that plan. Every terminal path clears all
of them: successful or invalid drop, release outside the Designer, Escape, capture
loss, `CancelMode`, source destruction, Preview/document rebuild, geometry invalidation,
application deactivation, and guarded early return. `CancelMode()` is idempotent.
It is a notification of capture cancellation, not a request to release capture
again. Overlay-initiated release marks the gesture terminal and sets its release
guard before calling U++ `ReleaseCapture()`; the synchronous `CancelMode()`
callback then returns without releasing again. External capture loss follows the
same logical cleanup without calling a physical release API.

Cancellation releases capture only when owned by the drag surface, restores the
cursor, repaints once, performs no document mutation, and cannot recursively cancel
itself. A plan retains IDs or indices, never raw pointers into a replaceable snapshot.
Before execution, both recorded generations must still match; otherwise the plan is
invalidated and resolved again once.

### Rebuild and work bounds

Drag tracking must not synchronously rebuild Preview. If geometry changes during a
drag, invalidate the plan and resolve on the next input event or deferred update.
One mouse move permits at most one target resolution and one semantic plan validation,
with no document mutation, Preview rebuild, subtree rebuild, Inspector rebuild, or
nested target-resolution call.

### Release-blocking acceptance

Before continuing to semantic drop-region work, test a nested Grid/Box/Grid hierarchy:
move a catalog Grid between outer Grid, Box, and nested Grid for at least 30 seconds;
cancel with Escape; release outside; perform invalid and valid drops; and repeat after
Preview rebuilds. Every attempt must return to `Idle`, release owned capture, restore
selection and Inspector input, and leave no indicator behind.

Focused coverage must include capture loss, `CancelMode`, double cancellation, source
destruction, snapshot replacement, target deletion/rebuild, valid and invalid drops,
and proof that a tracking move does not mutate the document or recursively resolve.

## Purpose

UiDesigner uses one insertion and move service for mouse drag/drop, hierarchy reordering, click-to-add, keyboard insertion, CLI and MCP. UI controls never mutate the canonical document during hover.

## Catalog presentation

The toolbox is a searchable flat catalog with category selection. Categories include All, Layouts, Containers, Ui Controls, Composites, Presets and U++ Controls. The authored left-hand pill icons select these categories; they do not create a second discovery tree.

A catalog row supports:

- filtering by display name, type ID and help text;
- keyboard selection and Enter activation;
- double-click insertion;
- drag using the versioned UiDesigner text payload;
- the same `UiDesignerDropService` used by all other insertion surfaces.

## Payloads

Two payload classes are supported:

- catalog payload: one registered type ID;
- node payload: one or more stable node IDs in document order.

Payloads are versioned text envelopes. They contain identity only. They never contain authoritative property snapshots or document pointers.

## Pure planning

`UiDesignerDropService::PlanAdd()` and `PlanMove()` return a `UiDesignerDropPlan` without changing the document. A plan includes:

- operation type;
- target parent and insertion index;
- ordered source node IDs or catalog type;
- snapped canvas position when relevant;
- default properties and layout-property updates;
- validation result, human-readable reason and indicator label.

Every query-phase drag call recalculates a plan from the current document revision. The plan shown by the canvas or hierarchy is transient.

## Parent compatibility

Compatibility comes from the catalog and document validator, including:

- semantic Spacer only under Box or Grid layouts;
- no children under non-containers;
- no self or descendant reparenting;
- two panes maximum for Splitter;
- four panes maximum for QuadSplitter;
- one direct child for direct-content and scroll hosts;
- registered page children for Stack, Tab and Accordion;
- stable order for multi-node moves.

## Placement modes

- Window/root positioned content: snapped x/y coordinates are written as one transaction.
- `UiAbsoluteLayout`: exact local child rectangles are authored here; this is the freeform placement system. Host controls such as Panel and GroupPanel are not freeform layout engines.
- BoxLayout: semantic insertion index; child geometry is not authoritative.
- GridLayout: row/column are calculated from the target cell and written in the same transaction.
- Stack/Tab/Accordion: semantic page or section order.
- Splitter/QuadSplitter: pane order with capacity validation.
- Hierarchy: before, inside or after target, translated into parent/index.

## Terminal execution

No document mutation occurs before a terminal paste. On paste:

1. recalculate the plan;
2. reject a stale or invalid target;
3. execute all reparenting, ordering and placement changes in one command transaction;
4. emit one terminal change set;
5. create one undo entry;
6. update selection after success.

CancelMode, capture loss and drag cancellation clear local gesture state and indicators only. They do not release unrelated capture recursively or mutate the model.

## Projection

The preview paints the active drop indicator from the plan. Semantic items such as Spacer have Designer-only geometry and remain selectable without manufacturing a runtime `Ctrl`.

## Automation

CLI/MCP expose plan and apply operations separately. Mutating operations accept an expected document revision and reject stale revisions without partial changes.
