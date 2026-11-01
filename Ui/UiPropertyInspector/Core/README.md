# UiPropertyInspectorCore

`Ui/UiPropertyInspector/Core` is the headless property model used by the reusable UiPropertyInspector visual package.

Version: **1.0.0**

## Package layout

- `Ui/UiPropertyInspector/Core` - headless model package (`Core.upp`)

Include `<Ui/UiPropertyInspector/Core/UiPropertyInspectorCore.h>` for the public
headless facade, or `UiPropertyInspectorModel.h` from the same directory for the
model declarations. Visual consumers use the parent `Ui/UiPropertyInspector`
package. The physical location under Ui does not add a GUI dependency. See the
[inspector guide](../../../docs/05_UI_PROPERTY_EDITOR_GUIDE.md).

## Design goals

- headless property schema and value model;
- categories and grouping data;
- normalization, validation, reset, and revisions;
- no dependency on `CtrlCore`, `CtrlLib`, `Ui`, or Designer;
- suitable for GUI, CLI, MCP, and tests.

## Model contract

- `UiPropertyInspectorModel` stores property items, value revisions, and structure revisions.
- `UiPropertyInspectorItem` carries visibility, enablement, read-only, default,
  range, validation, row-span and semantic editor metadata.
- `UiPropertyInspectorChoice` stores choice value, label, and optional icon data.

## Value conventions

- Vector2 and Vector3 use `ValueArray` numeric components.
- Curves use a `ValueArray` of two-element `ValueArray` points.
- Curve coordinates are normalized to `0..1`.
- A mixed value is represented by `UiPropertyInspectorItem::mixed`, not by corrupting the stored value.

## Integration boundary

The package does not paint rows, create editors, or write application state directly. Visual packages and host applications decide how model events map to previews, commits, undo, runtime controls, or MCP commands.

Fields such as `custom_editor`, `editor_variant`, and `picker_provider` are
opaque identifiers. Core stores them without depending on the concrete editor,
matrix, slider, picker, Designer, or SymbolPicker implementations.
