# PropertyEditor

`Utilities/PropertyEditor` is a reusable Ui-backed property-browser package. It is not tied to UiDesigner.

Version: **1.1.0**

## Package layout

- `Utilities/PropertyEditor` — reusable Ui-backed library package
- `Utilities/PropertyEditorCore` — headless property schema/model/normalization
- `Utilities/PropertyEditorDemo` — broad built-in/editor interaction demonstration
- `Utilities/PropertyEditorSemanticDemo` — semantic value/editor capability demonstration
- `Utilities/PropertyEditorTests` — legacy and regression coverage
- `Utilities/PropertyEditorV1RunTests` — focused interaction and stress coverage
- `Utilities/PropertyEditorSemanticRunTests` — semantic adapter/model contract coverage
- `Utilities/PropertyEditorCoreProbe` — verifies the headless package boundary

## Design goals

- one headless property schema/model;
- collapsible categories and live filtering;
- alternate row shading and Ui/System/Light/Dark palettes;
- summary-on-idle rows with reusable compact inline hosting;
- preview and final-commit events;
- mixed, inherited, validation and reset state;
- first-class rich value adapters without UiDesigner dependencies;
- custom editor/provider registration for application-specific domains;
- viewport-bounded rich editor construction;
- no application history/state ownership inside PropertyEditor.

`PropertyEditorCore` remains the authority for schema, normalization and validation. The visual package maps semantic metadata to production `Ui` controls through `PropertyEditorFactory`; applications remain responsible for commands, undo, theme/document ownership and domain-specific resource browsers.

## Built-in editors

Core/basic presentations:

- Text and multiline text
- Integer and Double
- typed numeric Integer/Double with value-to-slider mode
- Boolean (`Check`, `OnOff`, `TrueFalse`)
- Choice
- Color
- ordered ColorPalette (1–8 slots)
- FillRecipe
- FilePath
- Integer/Double sliders
- Vector2 / Vector3
- point Curve and cubic Bézier Curve
- ReadOnly
- Custom factory editor

First-class visual adapters:

- Range (`UiRangeSliderEdit`)
- Adjustable Range
- Matrix (`UiMatrixSelector`, including Position9, Cardinal4 and QuadPair)
- Icon catalog
- Font catalog
- provider-driven Image with compact thumbnails

Semantic adapters:

- Date
- Time
- DateTime
- unit-aware Duration
- Point
- Size
- Rect
- Insets / Padding / Margins-style four-sided geometry
- four-corner radii
- Flags / multi-choice
- small ordered String List
- Gradient recipe with Linear/Radial mode, arbitrary ordered stops, stop alpha, angle and interpolation
- keyboard Key Chord
- generic provider-driven Resource / Reference
- explicit Optional / Null values (`text`, `int`, `double` variants)

The PropertyEditor constructor and `SetFactory()` register the complete standard adapter set. New code that prepares a factory directly can call:

```cpp
PropertyEditorFactory factory;
RegisterPropertyEditorEditors(factory);
```

`RegisterPropertyEditorV1Editors()` remains available for compatibility with older callers that intentionally want only the original adapter set.

## Minimal usage

```cpp
#include <Utilities/PropertyEditor/PropertyEditor.h>

PropertyEditorModel model;
PropertyEditor editor;

model.AddText("name", "Name", "Object", "General")
     .SetHelp("Display name.");

model.AddNumericDouble("opacity", "Opacity", 1.0, 0.0, 1.0, 0.01,
                       "Appearance");
model.AddBoolean("enabled", "Enabled", true, "Behaviour");

AddPropertyInsets(model, "padding", "Padding",
                  12, 12, 12, 12, true, "Layout");
AddPropertyKeyChord(model, "save_key", "Save shortcut",
                    "Ctrl+S", "Input");

editor.SetModel(&model);

model.WhenPreview = [&](String id, Value value) {
    // Apply temporary live preview.
};

model.WhenCommit = [&](String id, Value value) {
    // Create durable command / undo record.
};
```

## Semantic value conventions

### Date / Time / DateTime

`AddPropertyDate()` stores a `Date` and uses production `UiDateTime` in Date mode.

`AddPropertyTime()` stores a `Time` anchored to `1970-01-01`, making the value unambiguous while retaining the existing U++ `Time` type. Seconds can be enabled or omitted.

`AddPropertyDateTime()` stores a normal U++ `Time` with date and clock fields.

All three are nullable and use the real `UiDateTime` picker/editor rather than parsing a generic text field.

### Duration

`AddPropertyDuration()` stores **seconds** as the durable value. The visual editor can display/edit milliseconds, seconds, minutes or hours without changing the application-facing unit. Minimum, maximum and step are also declared in seconds.

### Semantic geometry

Geometry helpers store numeric `ValueArray` values:

- Point: `[x, y]`
- Size: `[width, height]`
- Rect: `[x, y, width, height]`
- Insets: `[left, top, right, bottom]`
- Corners: `[topLeft, topRight, bottomRight, bottomLeft]`

Insets and Corners can begin in linked editing mode. Link/unlink is an editor presentation state: it changes how the user edits the four components, not the stored value shape.

### Flags / multi-choice

`AddPropertyFlags()` stores a `ValueArray` of selected choice values. Add the available domain using the returned item's normal `AddChoice()` API. The visual editor presents those choices as independent checkboxes and commits the complete selected set.

### Small ordered lists

`AddPropertyStringList()` stores a bounded `ValueArray` of strings and provides Add / Remove / Up / Down editing. This adapter is intentionally for **property-sized collections**.

It is not a replacement for model-authoritative Data pages or `UiListModel` / `UiTreeModel` / `UiTableModel` when the collection is the application's real data domain.

### Gradient

Gradient recipes use a `ValueMap`:

```text
{
  mode: "Linear" | "Radial",
  angle: 0..360,
  interpolation: "Linear" | "Smooth",
  stops: [
    { position: 0..1, color: Color, alpha: 0..255 },
    ...
  ]
}
```

Use `PropertyEditorMakeGradientStop()` and `PropertyEditorMakeGradient()` to build normalized recipes. At least two stops are retained; positions and alpha are clamped and stops are ordered by position.

### Key chord

`AddPropertyKeyChord()` stores a canonical human-readable string such as `Ctrl+Shift+S`. Common modifier aliases are normalized and modifiers are ordered consistently.

### Resource/reference

`AddPropertyReference()` stores an application-defined `Value` and delegates browsing to a registered picker provider. PropertyEditor does not interpret project IDs, asset URIs or repository resources.

Register the provider with the global/default factory (the same provider boundary used by Image):

```cpp
PropertyEditorFactory::Global().RegisterPicker(
    "project-resource",
    [](Value& value, Ctrl *owner) {
        return PickProjectResource(value, owner);
    });
```

### Optional/null

`AddPropertyOptional()` makes null an explicit durable state, with a separate Set/unset affordance. It is intentionally different from `inherited`/theme override state. Current built-in optional variants are text, integer and double; Reset returns to the helper's supplied fallback/default value.

## Rich editor interaction

Single Color properties use a stable inline swatch plus `#RRGGBB`. FillRecipe keeps persistent Solid/Gradient controls. ColorPalette supports one through eight ordered directly clickable swatches.

Rich editors that support compact and expanded presentations use `SetExpandedRowSpan(rows)`. Matrix, Curve, Image and Multiline adapters remain one row by default, expand in place on demand and retain separate dialog actions where a larger editor is useful. Vector2/Vector3 can stack components when an Inspector is narrow.

`PropertyEditorStyle::action_icons` centrally controls expand, collapse, dialog, browse and numeric-slider actions. Reset remains a row-state action rather than an editor-mode action.

Numeric slider-toggle mode, semantic geometry link mode and other editor affordances are view/editing concepts. They do not replace reset, inherited/authored or mixed-value state.

## Custom editors

Register a custom editor once:

```cpp
PropertyEditorFactory::Global().RegisterCustom(
    "my-editor",
    [] { return MakeOne<MyPropertyValueEditor>(); });
```

Then declare the item with `PropertyEditorKind::Custom` and set `custom_editor`.

A compact compound editor can keep real controls mounted in the value row:

```cpp
PropertyEditorItem& item = model.Add(
    "recipe", "Recipe", PropertyEditorKind::Custom, value, "Appearance");
item.custom_editor = "my-editor";
item.SetInlineEditor();
```

Inline hosting uses the same model normalization, preview, commit, validation, reset, mixed and inherited paths as an editor activated on demand. New control adapters should declare semantic metadata rather than adding control-specific row logic to `PropertyEditor`.

## Resource providers

Icon choices use the shared `UiIconCatalog`, and font faces use the platform font catalogue. Both catalogues are initialized lazily and reused.

Image/reference values stay application-defined. Image providers may additionally register a thumbnail resolver. The PropertyEditor package itself does not load project assets and has no SymbolPicker dependency.

## Existing curve support

`AddCurve()` stores a normalized multi-point `0..1` curve. The editor can add/manipulate points and `ResetLinear()` restores `(0,0) → (1,1)`.

`AddBezierCurve()` stores cubic Bézier control points and uses `UiBezierCurveEditor` for inline/dialog editing. These are distinct capabilities and remain available alongside the new Gradient recipe editor.

## Integration boundary

The package does not write application state directly. The owning application decides how preview/commit maps to commands, undo, live runtime objects, theme documents or external requests.

An edit transaction captures its original value when editing starts. Escape restores that origin and emits `WhenCancel`. `Ctrl+Z` emits `WhenUndoRequest` so the host can invoke its own undo stack; PropertyEditor deliberately does not own application history.
