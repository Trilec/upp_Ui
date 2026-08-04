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
- summary-on-idle rows by default, with explicit reusable inline hosting for compact compound editors;
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
- Typed numeric integer and double editors with optional slider mode
- Boolean
- Choice
- Color
- One-to-four color palette
- Fill recipe with persistent Solid/Gradient controls
- File path
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

A compact compound editor can keep its real controls mounted in the value row:

```cpp
PropertyEditorItem& item = model.Add(
    "recipe", "Recipe", PropertyEditorKind::Custom, value, "Appearance");
item.custom_editor = "my-editor";
item.SetInlineEditor();
```

Inline hosting is a presentation choice only. It uses the same factory, model normalization, preview, commit, validation, reset, mixed-value and inherited-value paths as an editor activated on demand. New control adapters should therefore declare the correct property kind and metadata; they should not add control-specific row logic to `PropertyEditor`.

`FillRecipe` keeps its established inline presentation automatically. Its Solid mode edits one colour and Quad Gradient edits four colours as one recipe transaction.

## Value conventions

- Vector2 and Vector3 use `ValueArray` numeric components.
- Curves use a `ValueArray` of two-element `ValueArray` points.
- Curve coordinates are normalized to `0..1`.
- A mixed value is represented by `PropertyEditorItem::mixed`, not by corrupting the stored value.
- Numeric ranges and slider-toggle availability belong to the property schema; the view must not invent semantic ranges.

## Integration boundary

The package does not write application state directly. The owning application decides how preview and commit events map to commands, undo, live runtime objects, theme documents, or MCP requests.
