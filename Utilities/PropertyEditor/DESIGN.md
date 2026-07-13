# PropertyEditor design

## Why it is a separate utility

The property browser is useful beyond the Ui Designer:

- Theme Designer
- material and render settings
- node editors
- asset metadata
- animation and curve settings
- application preferences
- MCP-driven headless inspection and editing

The package therefore depends on ordinary U++ controls and a generic `Value` model, not on Designer-specific nodes.

## Patterns adopted

### Manager/model separate from editors

Mature property systems separate the property definition and value manager from the widget used to edit the current value. The property model in this package is headless; editor controls are delegates.

### Editor factory

Editor creation is centralized. Built-in editor kinds use a switch inside the factory, while applications can register custom editor IDs.

### Categories and folding

Groups are model data. The browser renders category headers and remembers their open state. Categories are not permanent nested windows.

### Active-editor virtualization

Only the currently edited property owns a live child editor control. Other rows are painted summaries. This avoids creating and destroying dozens or hundreds of composite controls during every selection change.

### Preview versus commit

Continuous editors emit preview values while dragging or typing and a final commit when editing completes. Discrete controls emit both in one operation.

### Stable external ownership

The property browser does not own the application document, undo stack, selection, theme model, or preview renderer. Those systems subscribe to the headless model.

## Comparison notes

- Classic Qt Designer separates the form window, property editor, object inspector, widget database and widget factory.
- Qt Property Browser uses property managers plus editor factories.
- Godot's inspector uses property editor plugins, categories, folding, revert state, validation and a changing/final distinction.
- U++ LayDes has a useful property factory pattern, but its historical one-control-per-property approach is not used here because the Designer needs stable and inexpensive selection changes.

## Future-compatible boundaries

The current API deliberately leaves room for:

- font, file, image, icon and resource editors;
- quaternion and matrix editors;
- nested object/sub-inspector properties;
- keyframe and animation indicators;
- per-property context menus;
- copied/pasted property paths;
- property favorites;
- asynchronous or remote property sources;
- Theme Designer property sources;
- MCP schemas and commands;
- a dedicated out-of-process preview host.

These additions should extend the model and factory rather than introducing parallel property pipelines.
