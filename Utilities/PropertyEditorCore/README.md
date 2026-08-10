# PropertyEditorCore

`Utilities/PropertyEditorCore` is the headless property model used by the reusable PropertyEditor visual package.

Version: **1.0.0**

## Package layout

- `PropertyEditorCore` - headless model package

## Design goals

- headless property schema and value model;
- categories and grouping data;
- normalization, validation, reset, and revisions;
- no dependency on `CtrlCore`, `CtrlLib`, `Ui`, or Designer;
- suitable for GUI, CLI, MCP, and tests.

## Model contract

- `PropertyEditorModel` stores property items, value revisions, and structure revisions.
- `PropertyEditorItem` carries visibility, enablement, read-only, default,
  range, validation, row-span and semantic editor metadata.
- `PropertyEditorChoice` stores choice value, label, and optional icon data.

## Value conventions

- Vector2 and Vector3 use `ValueArray` numeric components.
- Curves use a `ValueArray` of two-element `ValueArray` points.
- Curve coordinates are normalized to `0..1`.
- A mixed value is represented by `PropertyEditorItem::mixed`, not by corrupting the stored value.

## Integration boundary

The package does not paint rows, create editors, or write application state directly. Visual packages and host applications decide how model events map to previews, commits, undo, runtime controls, or MCP commands.

Fields such as `custom_editor`, `editor_variant`, and `picker_provider` are
opaque identifiers. Core stores them without depending on the concrete editor,
matrix, slider, picker, Designer, or SymbolPicker implementations.
