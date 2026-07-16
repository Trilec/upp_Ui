# UiDesigner behavior-binding contract

## Principle

Behavior is durable typed document data, not free-form C++ hidden inside ordinary control properties. The same binding model is edited by the Behavior Inspector, serialized in projects, consumed by CodeGen and exposed through CLI/MCP.

## Canonical records

Each node owns zero or more `UiDesignerActionBinding` records. One binding is permitted per declared event ID.

A binding contains:

- stable binding ID;
- catalog-declared event ID;
- `UiDesignerActionType`;
- optional stable target node ID;
- optional target property;
- typed value or page index;
- numeric adjustment delta;
- optional named handler;
- enabled state.

The catalog publishes `UiDesignerEventSpec` for each authorable control family. Validation rejects events not declared by that control and targets that no longer exist. Removing a target subtree cleans dependent bindings atomically.

## Built-in actions

The first release supports:

- `CloseWindow`;
- `AcceptDialog`;
- `CancelDialog`;
- `ExitApplication`;
- `SetProperty`;
- `ToggleProperty`;
- `AdjustValue`;
- `ActivatePage`;
- `CallNamedHandler`.

Built-in actions generate deterministic wiring in `BindGeneratedActions()`. Page activation uses the common `SetData()` contract implemented by Stack, Tab and Accordion-style page containers.

## Custom handlers

`CallNamedHandler` generates a protected virtual handler declaration in the generated base and an override stub in the user-owned subclass. Regeneration may replace `.generated.*` files but never overwrites an existing user-owned header or source.

Generated code does not accept arbitrary inline C++ in this release. This avoids unsafe string insertion, accidental loss during regeneration and protocol-specific code fragments in the canonical model.

## Inspector behavior

The Behavior Inspector appears for one selected control with declared events. It edits:

- event;
- action type;
- target;
- target property;
- value/page;
- adjustment;
- handler name;
- enabled state.

Changing a field creates or replaces one binding through `UiDesignerCommandService`. Removing a binding is also a command. Each completed edit is undoable and updates CodeGen. Model rebuild is non-reentrant; UI refresh notifications are emitted only by terminal edits or explicit event selection.

## Event signatures

CodeGen maps event IDs to their real callback signatures, including split-button selection, page changes and section toggles. Designer-level compatibility aliases are resolved during generation rather than emitting nonexistent members.

## Serialization and migration

Bindings are stored in the current document schema with stable node IDs. Copy/import remaps targets when the referenced node is included and rejects dangling references during validation.

## CLI and MCP

Automation supports listing, creating/replacing and removing bindings. Mutations require an optional expected document revision and report revision conflicts without partial mutation. The `uidesigner://behaviors` resource exposes the current typed bindings.

## Future extension

Additional actions can be introduced by adding an enum value, validation rule, Inspector fields when needed and a CodeGen emitter. Actions must remain declarative and transport-neutral. Application-specific code continues to live in user-owned handler implementations.
