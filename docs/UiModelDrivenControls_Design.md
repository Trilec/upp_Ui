# Model-Driven Control Mutation Contract

## Purpose

Define one consistent mutation pattern for controls that present ordered,
hierarchical, or database-backed data.

This applies to the model-backed/data-presenting control family:

- `UiList`
- `UiTree`
- `UiMenu`
- `UiDropdown`
- `UiTable`

Structural controls such as `UiAccordion`, `UiTab`, and `UiStack` can keep
direct page/section APIs unless a later use case needs command-driven
interception. They are not part of this first model-driven contract.

## Problem

Older widget APIs often let a control directly mutate its local data and then
notify the application afterward. That works for small standalone controls, but
it becomes fragile when the same control is used with:

- undo/redo command stacks
- external or shared models
- database-backed records
- designer/code-generation state
- validation rules
- multi-view synchronization
- collaboration or transaction logs

The control should own interaction. The application should own semantic state
changes when it has a command pipeline.

## Core Rule

Controls must calculate and report user intent before mutating authoritative
data.

The control owns:

- hit testing
- keyboard and pointer interaction
- drag threshold handling
- visible insertion markers
- hover/pressed/focus state
- local selection visuals
- accessibility-friendly affordances

The owner/model/command layer owns:

- whether the operation is valid
- how the operation is recorded
- undo/redo transactions
- persistence/database updates
- business rules
- generated output refresh
- cross-view synchronization

## Request-First API Shape

Each data-changing operation should have a request event. The request carries
the proposed operation, and the receiver can accept, reject, or fully handle it.

Example shape:

```cpp
struct UiReorderRequest {
    int from = -1;
    int before = -1;
    bool accept = true;
    bool handled = false;
};

Event<UiReorderRequest&> WhenReorderRequest;
Event<int, int> WhenReordered;
```

Control behavior:

1. The user performs an operation.
2. The control computes the proposed target.
3. The control emits a request event.
4. If the request is rejected, nothing changes.
5. If the request is handled, the owner has performed or scheduled the change.
6. If unhandled and internal mutation is enabled, the control mutates its local
   model.
7. A post-change notification may be emitted after the data has changed.

## Internal Mutation

Internal mutation remains useful for demos, local widgets, and simple tools, but
it should be explicit.

Recommended API:

```cpp
control.EnableInternalMutation(true);
control.EnableDragReorder(true);
```

For command-driven tools such as the Designer:

```cpp
control.EnableInternalMutation(false);
control.WhenReorderRequest = [=](UiReorderRequest& r) {
    r.handled = true;
    Dispatch(MoveNodeCommand(r.from, r.before));
};
```

The default implementation path must still be request-first. Internal mutation
is an explicit supported mode for simple/local controls, not the old
"mutate silently, notify afterward" design.

## Common Request Types

Use consistent naming and structure across controls.

- `UiReorderRequest`: move item within the same parent/list.
- `UiMoveRequest`: move item to a new parent/container.
- `UiInsertRequest`: insert/create item.
- `UiRemoveRequest`: remove item.
- `UiRenameRequest`: rename item.
- `UiActivateRequest`: action/execute/open item.
- `UiCheckRequest`: toggle checked state.
- `UiSelectRequest`: selection change where selection is application-owned.

Requests should include stable references where possible. Flat controls can use
indexes. Tree/menu controls should use node references plus parent and insert
position. Database-backed controls should allow an item key or `Value` token in
addition to transient row indexes.

## Control Families

### List and Dropdown

`UiList` and list-backed dropdowns should share reorder and item-action request
contracts. A dropdown popup is only a presentation of a list model, so model
changes should follow the same path.

### Tree and Menu

`UiTree` and `UiMenu` should use the same hierarchical move contract:

```cpp
struct UiTreeMoveRequest {
    UiTreeNodeRef node;
    UiTreeNodeRef new_parent;
    int insert_pos = -1;
    bool accept = true;
    bool handled = false;
};
```

Menus should not be treated as a special case. A menu is a hierarchical model
with activation behavior and optional checked/radio state. Editing, reordering,
or database-backed menu construction should feel familiar to tree users.

### Table and Document-Like Controls

`UiTable` and document controls need command-driven mutation by default because
cell edits, table structure changes, embedded blocks, comments, and metadata all
need undoable transactions.

## Designer Rule

The Designer must never allow a view model to become the source of truth.

Hierarchy tree operations should:

- let `UiTree` handle drag visuals and target calculation
- convert the drop target to a Designer operation
- dispatch a Designer command
- rebuild or refresh the hierarchy view from `DesignerModel`

The tree model is a projection. `DesignerModel` remains authoritative.

## Designer Canonical Data Area

Each Designer node may carry an authored `data` map separate from ordinary
control properties, theme overrides, and event bindings. This map is the
canonical editable model seed for model-driven controls; runtime models are
rebuilt from it and must not become the source of truth.

Data changes use the same command and history path as property changes. The
Designer document serializes the map under the node's `data` member, and
generated code consumes the canonical data when a control has authored data.

## Appearance And Theme Source

Appearance is a separate authored concern from canonical control data. A
control's face and frame each use a typed surface recipe. The default recipe is
shown to users as **Use theme** and means inheritance from the active theme.
Explicit `None` means that the surface is intentionally absent. `Solid`,
`Gradient`, `Image`, `Nine-slice`, and `Dashed` are authored choices where the
surface supports them.

The Designer must preserve this distinction through the document model,
preview, undo/redo, JSON, and code generation. It must not convert a choice into
an ordinary string property or apply a temporary paint patch that is lost on a
preview rebuild. Image choices reference embedded document resources by stable
key. The resource bytes are serialized once in the resource table and resolved
by the preview and generated package paths.

Image placement has two names and meanings: `Fill` scales to the target
rectangle, while `Fit` preserves aspect ratio and crops overflow. No separate
Designer-only image calculation is permitted; the same resolved `StyledSkin`
and `UiDraw` path must determine runtime painting and preview geometry.

This source model is intentionally control-adapter-owned. A new control does
not inherit a universal panel schema by accident: its adapter declares which
surface recipes it supports, how state values are represented, how resources
are resolved, and how the effective style is emitted. Shared resolver helpers
may implement common behavior, but ownership of the contract remains with the
control adapter.

For `UiTree`, the first canonical shape is a `root` map containing `text`, an
optional stable `key`/`data` value, and a `children` array of the same shape.
List, Dropdown, Menu, and Table data schemas will be defined from their
respective model APIs rather than represented as ad-hoc control properties.

## Implementation Guidance

1. Add shared request structs where the operation is conceptually shared.
2. Add `When*Request` before existing post-change notifications.
3. Preserve post-change notifications as observations, not authorization.
4. Add explicit internal mutation switches where direct mutation is useful.
5. Keep drag-marker behavior inside the control so all views feel consistent.
6. Route command-driven apps through request handlers.
7. Avoid duplicating reorder logic in application code; only the semantic state
   change belongs outside the control.

## Current Implemented Surface

The first implementation pass adds shared request structs in `UiDataModels.h`:

- `UiReorderRequest`
- `UiTreeMoveRequest`
- `UiMenuActionRequest`
- `UiTableEditRequest`

Implemented request hooks:

- `UiList::WhenReorderRequest`
- `UiDropdown::WhenReorderRequest`
- `UiTree::WhenMoveRequest`
- `UiMenu::WhenActionRequest`
- `UiTable::WhenEditRequest`

Implemented internal mutation switches:

- `UiList::EnableInternalMutation(bool)`
- `UiDropdown::EnableInternalMutation(bool)`
- `UiTree::EnableInternalMutation(bool)`
- `UiMenu::EnableInternalMutation(bool)`
- `UiTable::EnableInternalMutation(bool)`

The Designer hierarchy disables internal tree mutation because its tree model is
a projection of `DesignerModel`.

## Historical Notes

`archive/UiList_DragReorder_Note.md` documents the earlier list reorder pass.
That note is useful for understanding the drag-lane visuals, but its
control-owned mutation contract is not the target architecture for
command-driven systems.
