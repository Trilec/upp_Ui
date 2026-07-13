# PropertyEditor

`Utilities/PropertyEditor` is a reusable Ui-backed property-browser package. It is not tied to the Ui Designer.

Version: **1.0.0-rc1**

## Package layout

- `Utilities/PropertyEditor` - reusable Ui-backed library package
- `Utilities/PropertyEditorDemo` - interactive demonstration
- `Utilities/PropertyEditorTests` - headless model tests

## Design goals

- one headless property schema/model;
- collapsible categories;
- filter/search;
- alternate row shading;
- Follow Ui theme, System, Light and Dark row palettes;
- one active value editor at a time rather than one permanent control per row;
- preview and final-commit events;
- custom editor registration;
- Ui-backed value delegates for live preview and commit;
- mixed and inherited values;
- validation and reset support;
- no dependency on the Designer model or window.

## Built-in editors

- Text
- Multiline text
- Integer
- Double
- Boolean
- Choice
- Color
- Integer slider
- Double slider
- Vector2
- Vector3
- Curve
- Read-only
- Custom factory editor

## Minimal usage

```cpp
#include <Utilities/PropertyEditor/PropertyEditor.h>

PropertyEditorModel model;
PropertyEditor editor;

model.AddText("name", "Name", "Object", "General")
     .SetHelp("Display name.");

model.AddDouble("opacity", "Opacity", 1.0, "Appearance")
     .SetRange(0.0, 1.0, 0.01)
     .SetImpact(PropertyImpactPaint);

model.AddBoolean("enabled", "Enabled", true, "Behaviour");

editor.SetModel(&model);

model.WhenPreview = [&](String id, Value value) {
    // Apply a temporary live preview.
};

model.WhenCommit = [&](String id, Value value) {
    // Create the durable command / undo record.
};
```

## Custom editors

Register a custom editor once:

```cpp
PropertyEditorFactory::Global().RegisterCustom(
    "my-editor",
    [] { return MakeOne<MyPropertyValueEditor>(); });
```

Then declare the item with `PropertyEditorKind::Custom` and set `custom_editor`.

## Value conventions

- Vector2 and Vector3 use `ValueArray` numeric components.
- Curves use a `ValueArray` of two-element `ValueArray` points.
- Curve coordinates are normalized to `0..1`.
- A mixed value is represented by `PropertyEditorItem::mixed`, not by corrupting the stored value.

## Integration boundary

The package does not write application state directly. The owning application decides how preview and commit events map to commands, undo, live runtime objects, theme documents, or MCP requests.
