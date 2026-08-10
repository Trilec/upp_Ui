# PropertyEditorCore design

## Why it is a separate utility

The property browser is useful beyond the Ui Designer:

- Theme Designer
- material and render settings
- node editors
- asset metadata
- animation and curve settings
- application preferences
- MCP-driven headless inspection and editing

The package therefore depends only on Core and Draw and exposes a generic
`Value` model. It does not depend on controls or Designer-specific nodes.

## Patterns adopted

### Manager/model separate from editors

Mature property systems separate the property definition and value manager from the widget used to edit the current value. The property model in this package is headless; editor controls are delegates.

### Editor factory

Editor creation is centralized in the visual `PropertyEditor` package. Core
only defines semantic kinds and opaque adapter/provider identifiers.

### Categories and folding

Groups are model data. The browser renders category headers and remembers their open state. Categories are not permanent nested windows.

### Virtualization metadata

Core provides `inline_editor` and `row_span` metadata without constructing any
controls. The visual package virtualizes rich inline editors to its viewport
and paints summaries for unmounted rows.

### Preview versus commit

Continuous editors emit preview values while dragging or typing and a final commit when editing completes. Discrete controls emit both in one operation.

### Stable external ownership

The property browser does not own the application document, undo stack, selection, theme model, or preview renderer. Those systems subscribe to the headless model.

## Comparison notes

- Classic Qt Designer separates the form window, property editor, object inspector, widget database and widget factory.
- Qt Property Browser uses property managers plus editor factories.
- Godot's inspector uses property editor plugins, categories, folding, revert state, validation and a changing/final distinction.
- U++ LayDes has a useful property factory pattern, but its historical one-control-per-property approach is not used here because the Designer needs stable and inexpensive selection changes.

## Current extensible boundaries

The current API deliberately leaves room for:

- `ColorPalette` provides one atomic array of one to four colour swatches.
- `FilePath` provides an editable path with a standard browse action; the host
  remains responsible for deciding whether the path is external or a document
  resource.
- `SliderInt` and `SliderDouble` are the range-aware numeric editors. They
  provide both slider manipulation and direct numeric entry; callers should
  use them when a bounded value benefits from visual adjustment.
- font, image, icon and resource editors are supplied by visual adapters and
  provider callbacks;
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
