# PropertyEditor

`Utilities/PropertyEditor` is a reusable Ui-backed property-browser package. It is not tied to the Ui Designer.

Version: **1.0.0**

## Package layout

- `Utilities/PropertyEditor` - reusable Ui-backed library package
- `Utilities/PropertyEditorDemo` - interactive demonstration
- `Utilities/PropertyEditorTests` - legacy and regression coverage
- `Utilities/PropertyEditorV1RunTests` - focused v1 interaction and stress coverage
- `Utilities/PropertyEditorCoreProbe` - verifies the headless package boundary

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

`PropertyEditorCore` remains the only authority for property schema,
normalization and validation. The visual package maps that semantic metadata to
concrete `Ui` controls through one `PropertyEditorFactory`; applications remain
responsible for commands, undo and domain-specific resource browsers.

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
- Range (`UiRangeSliderEdit`)
- Matrix (`UiMatrixSelector`, including `QuadPair`)
- Icon and Font catalog choices
- Provider-driven Image selection with an optional compact thumbnail provider

Boolean properties support `Check`, `OnOff`, and `TrueFalse` presentation.
Single Color properties are always hosted inline as a stable swatch plus
`#RRGGBB`, so the first click opens the picker without replacing or shifting the
row.

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

`FillRecipe` keeps its established inline presentation automatically. Its
Solid mode opens one UiColorPicker slot and Quad Gradient opens four ordered
slots as one recipe transaction. `ColorPalette` supports one through eight
ordered, directly clickable swatches and transfers the complete slot array on
preview and commit.

Rich inline controls are virtualized to the visible viewport plus one-row
overscan. Scrolling, filtering, group changes and model replacement destroy
slots that are no longer visible; the editor never constructs one rich control
per model item. Compact row height comes from `row_span`.

`SetIndent(levels)` expresses property hierarchy independently of group paths.
Use nested group paths for collapsible subheadings and indentation for child
properties within those headings. Multiline and composite adapters can request
additional rows with `SetRowSpan`; dialog-backed editors such as Curve retain a
compact summary and open their complete editor on demand.

Rich editors that support compact and inline presentations declare
`SetExpandedRowSpan(rows)`. The model stores only that capability; temporary
expanded/collapsed state belongs to the PropertyEditor view and is controlled
with `SetPropertyExpanded(id, expanded)`. Matrix, Curve, Image and Multiline
adapters use this contract to remain one row by default, expand in place when
requested, and retain a separate dialog action where a larger editor is useful.
Clicking or keyboard-activating a compact expandable summary opens it directly.
Expanded mode gives the
editor the complete value rectangle and keeps compact tool actions in a narrow
right-hand rail instead of repeating the compact summary above the editor.
Vector2 and Vector3 can use the same contract to stack components when a narrow
Inspector cannot present all values clearly.

`PropertyEditorStyle::action_icons` centrally controls the expand, collapse,
dialog, browse, and numeric-slider images and their common compact size.
`filter_gap` reserves separation between the fixed filter and row viewport.
`reset_icon` remains
separate because Reset is a row-state action rather than an editor action.
Nested group rows derive a restrained 10% lighter background from
`group_background`; applications do not need to maintain a second heading
colour.

Mixed values represent a real multi-selection state: selected objects currently
have different values, so the editor starts empty. Entering one value commits
that value as the shared replacement and clears the mixed state. Mixed is not a
special floating-point notation.

## Resource providers

Icon choices use the shared `UiIconCatalog`, and font faces use the platform
font catalogue. Both catalogues are initialized lazily once and reused by all
editor instances.

Image values stay application-defined. Register a chooser and, optionally, a
thumbnail resolver under the same provider id:

```cpp
factory.RegisterPicker("project-image", [](Value& value, Ctrl *owner) {
    return OpenProjectImageBrowser(value, owner);
});
factory.RegisterThumbnailProvider("project-image", [](const Value& value) {
    return ResolveProjectThumbnail(value);
});
```

The thumbnail is fitted into a compact one-line cell without changing aspect
ratio. PropertyEditor does not load project files and has no SymbolPicker
dependency.

## Value conventions

- Vector2 and Vector3 use `ValueArray` numeric components.
- Curves use a `ValueArray` of two-element `ValueArray` points.
- Curve coordinates are normalized to `0..1`.
- A mixed value is represented by `PropertyEditorItem::mixed`, not by corrupting the stored value.
- Numeric ranges and slider-toggle availability belong to the property schema; the view must not invent semantic ranges.

## Integration boundary

The package does not write application state directly. The owning application decides how preview and commit events map to commands, undo, live runtime objects, theme documents, or MCP requests.

An edit transaction captures its original value when editing starts. Escape
restores that origin and emits `WhenCancel`. `Ctrl+Z` emits `WhenUndoRequest` so
the host can invoke its own undo stack; PropertyEditor deliberately does not own
application history.
