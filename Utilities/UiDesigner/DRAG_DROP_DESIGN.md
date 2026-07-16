# UiDesigner drag-and-drop contract

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

- Root/freeform containers: snapped x/y coordinates are written as one transaction.
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
