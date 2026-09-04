# 05 - Ui PropertyEditor Guide

Canonical integration guide for `Utilities/PropertyEditorCore` and
`Utilities/PropertyEditor`. PropertyEditor is a reusable package in `upp_Ui`;
it is not owned by UiDesigner and must not depend on Designer or SymbolPicker.

For naming in PropertyEditor implementation, application code, demos, and host
integration, follow the shared control-prefix convention in
`01_UI_CONTROLS_GUIDE.md`. In particular, use
`pe_` for visual editors, `pe_model_` for their models, `btn_` for actions,
and `edit_` for editable text controls. This keeps the type and ownership
visible to readers and to agents inspecting a demo without renaming existing
library members.

## Architecture

- `PropertyEditorCore` is headless and depends only on Core and Draw. It owns
  property schema, typed values, normalization, validation, revisions, groups,
  defaults, mixed state, inheritance state, and impact metadata.
- `PropertyEditor` is the visual Ui-backed browser. It owns rows, selection,
  filtering, grouping, editor activation, inline-editor virtualization, and
  transaction interaction.
- `PropertyEditorFactory` is the single authority that maps semantic kinds and
  stable custom adapter ids to concrete `Ui` controls.
- The host application owns commands, undo history, runtime preview updates,
  resource browsers, persistence, and domain-specific meaning.

Do not add application models or picker implementations to Core. Do not add a
second editor factory or restore the retired `UiComposite*` property-row layer.

## Minimal integration

```cpp
#include <Utilities/PropertyEditor/PropertyEditor.h>

PropertyEditorModel model;
PropertyEditor editor;

model.AddText("name", "Name", "Object", "Identity")
     .SetHelp("Stable display name.");
model.AddNumericDouble("opacity", "Opacity", 1.0, 0.0, 1.0, 0.01,
                       "Appearance")
     .SetImpact(PropertyImpactPaint);
model.AddBoolean("enabled", "Enabled", true, "Behaviour");

editor.SetModel(&model); // non-owning
editor.WhenPreview = [&](String id, Value value) {
    // Apply temporary runtime state.
};
editor.WhenCommit = [&](String id, Value value) {
    // Create one durable command/undo entry.
};
editor.WhenUndoRequest = [&](String id) {
    // Delegate Ctrl+Z to the host's undo stack.
};
```

The model must outlive the editor or be detached with `SetModel(nullptr)` before
destruction/replacement.

## Property schema

Built-in `PropertyEditorKind` values cover Text, Multiline, Integer, Double,
NumericInt, NumericDouble, Boolean, Choice, Color, ColorPalette, FillRecipe,
FilePath, SliderInt, SliderDouble, Vector2, Vector3, Curve, ReadOnly, and Custom.

Use item metadata rather than control-specific row branches:

- `SetRange(min, max, step)` for bounded numeric semantics.
- `SetDefault(value)` for ordinary Reset behavior.
- `SetMixed()` for real multi-selection disagreement.
- `SetInherited()` and override flags for theme/local-state rows.
- `SetDomain()` and `SetImpact()` for host routing.
- `SetIndent()` for property hierarchy inside a group.
- `SetRowSpan()` for an always-taller row.
- `SetExpandedRowSpan()` for compact rows that can expand temporarily.
- `SetInlineEditor()` when a compact compound control should remain mounted.
- `SetPickerProvider()` for application-owned resource selection.

Nested group paths create collapsible headings. `SetGroupSubtitle` supplies the
optional right-side group summary without creating a fake property.

## Built-in presentations

- Boolean supports `Check`, `OnOff`, and `TrueFalse` presentations.
- A single Color is one stable inline swatch plus `#RRGGBB`.
- ColorPalette supports one through eight ordered, directly clickable swatches.
- FillRecipe uses one swatch for Solid and four ordered corner swatches for Quad
  Gradient; one picker transaction transfers the complete recipe.
- NumericInt/NumericDouble can expose a text/slider toggle when the schema has
  meaningful bounds.
- Vector2/Vector3 remain compact on one row and may expand when narrow.
- Multiline, Matrix, Curve, and Image can remain compact, expand in place, or
  open a dialog according to adapter capability.

`Curve` has two explicit value contracts. The default contract is an ordered
`ValueArray` of `[x, y]` point pairs for editable linear curves. Setting
`editor_variant` to `bezier` selects a fixed cubic contract stored as exactly
four scalars `[x1, y1, x2, y2]`. Use `AddBezierCurve`,
`PropertyEditorMakeBezierCurve`, and `PropertyEditorNormalizeBezierCurve` for
that form. The visual package reuses `UiBezierCurveEditor`; Core remains
headless and does not duplicate curve drawing or interaction code. X is
constrained to normalized time, while Y may overshoot for easing curves. Set
the item's minimum and maximum when its output domain is bounded.

## First-class control adapters

Range, Adjustable Range, Matrix, Icon, Font, and Image use stable Custom adapter
ids so Core remains independent of their concrete controls. Prefer the supplied
helpers rather than assigning ids manually:

```cpp
AddPropertyRange(model, "range", "Allowed range", 20, 80, 0, 100, 1,
                 "Layout");
AddPropertyAdjustableRange(model, "window", "Window",
                           0, 100, 250, 680, 900, 1000, 1, "Layout");
AddPropertyMatrix(model, "anchor", "Anchor", value, "Position9", "Layout");

// Exact top/right/bottom/left selection without a centre or diagonals.
AddPropertyMatrix(model, "icon_side", "Icon side", "top", "Cardinal4", "Layout");
AddPropertyIcon(model, "icon", "Icon", "ICON_DESIGN_HOME_48", "Resources");
AddPropertyFont(model, "font", "Font", "Arial", "Resources");
AddPropertyImage(model, "image", "Image", path, "project-image", "Resources");
```

`UiRangeSliderEdit` supplies compact direct endpoints around the expanding
range track. Adjustable Range uses four ordered values: lower bound, lower
selection, upper selection, upper bound. Matrix uses generic
`UiMatrixSelector` presets, including `QuadPair`; adapters must not introduce
domain-specific matrix APIs.

Icon and Font catalogues are initialized lazily and shared. Image remains
provider-driven and may contain one value or an ordered `ValueArray`; compact
and expanded thumbnails preserve aspect ratio.

## Resource providers

Register project-specific resource access on the factory:

```cpp
PropertyEditorFactory& factory = PropertyEditorFactory::Global();
factory.RegisterPicker("project-image", [](Value& value, Ctrl *owner) {
    return OpenProjectImageBrowser(value, owner);
});
factory.RegisterThumbnailProvider("project-image", [](const Value& value) {
    return ResolveProjectThumbnail(value);
});
```

The picker receives the current value and returns true only after acceptance.
PropertyEditor does not load arbitrary project files and does not hard-link a
specific icon, image, or symbol-picker application.

## Transactions and host refresh

The first preview captures the original value, mixed flag, and inherited state.
`WhenPreview` is temporary; `WhenCommit` is the durable normalized value;
Escape restores the captured origin and emits `WhenCancel`. Ctrl+Z emits
`WhenUndoRequest` and never creates an internal application undo stack.

Hosts should use `PropertyEditorImpact` to choose the narrowest truthful update:
paint, control state, local/ancestor layout, subtree, structure, selection,
Inspector schema, generated code, global theme, or full preview. Do not rebuild
an entire preview for every character when a local paint/layout update is enough.

A committing popup/modal inline editor may synchronously receive the model's
same-property change notification while still inside its own commit callback.
PropertyEditor suppresses only that redundant same-property refresh, then performs
the normal explicit post-commit reconfiguration. Do not allow a committed Color,
Fill, Font or similar editor to be re-entered into stale configuration by its own
synchronous notification.

## Inheritance, overrides, and Reset

Inherited theme state and ordinary Reset are separate contracts. An
`overrideable` item uses its state action to switch between inherited and local
authored values; `override_active` identifies the active local recipe and
`WhenOverride(id, active)` asks the host to perform that state change. Inactive
saved values may remain in the model without affecting Preview.

A successful authored value commit to an inactive `overrideable` item requests
`WhenOverride(id, true)` automatically. This rule is editor-independent: inline
numeric fields, Choice/Dropdown, Font, Color and other popup/custom editors must
all behave the same way. Hosts still own the state change itself by updating
`override_active`/inheritance in `WhenOverride`; PropertyEditor does not silently
mutate host override state. Explicitly clicking the override action remains the
way to turn an already-active override off.

An ordinary `resettable` property instead uses `default_value` and emits
`WhenReset`. It does not become a theme override merely because it has a default.
Group summaries can report local/total override counts without scanning the
whole model during Paint.

Mixed state means selected objects currently disagree. The editor starts empty;
entering one valid value commits it as the shared replacement and clears mixed
state. Never encode mixed numeric values with `DBL_MIN`, `DBL_MAX`, NaN, or a
display string in the authoritative value.


## Override layout grammar

PropertyEditor should present control style concepts using the same nouns as the
runtime API and UiDesigner.

For a conventional styled control, prefer this order when applicable:

```text
GENERAL
FACE
  SKIN
FRAME
INK
ICON
TYPOGRAPHY
CONTENT MARGIN
FOCUS
SHADOW
HIGHLIGHT
<control-specific groups>
```

Rules:

- group headings use real API nouns, not parallel design-language synonyms;
- the group supplies the prefix: `Frame / Width`, not `Frame / Frame Width`;
- keep General small;
- Skin is the image-backed implementation of Face and is nested under Face;
- Face and Frame remain separate;
- use the control's real state vocabulary rather than manufacturing symmetry;
- composite controls expose real nested domains such as
  `Dropdown / Popup`, `Accordion / Header / Body`, `Slider / Track / Thumb`;
- expose only fields/setters the control actually consumes;
- Background/Content/Foreground are paint-order concepts, not mandatory
  PropertyEditor headings.

Demo and UiDesigner presentations of the same control should preserve the same
stable property id, label, group path, ordering, state names and inheritance
semantics.

Before adding an override row, verify that preview/runtime application and code
generation can express the same authored value without creating parallel state.

## Layout and performance

- Filtering is live and fixed above the scrolling viewport.
- Filter matches temporarily reveal descendants without destroying saved group
  collapse state.
- Labels support Auto, Fixed, and Ratio sizing plus a draggable divider.
- Row height is `row_height * row_span`; rich editors do not hard-code pixels.
- Inline editors exist only for visible rows plus overscan. Paint and layout must
  not instantiate editors for every model item.
- Model replacement clears active/inline editors before old items disappear.
- Alternating rows, group hierarchy, validation, selection, mixed, inherited,
  and disabled states must remain visually distinguishable.

Use `GetInlineEditorCount()` in deterministic stress tests to prove bounded
editor creation. Actual responsiveness should also be smoke-tested in Release.

## Styling and interaction

`PropertyEditorStyle` controls palette, fonts, row/group/filter dimensions,
the explicit `filter_gap` before the scrolling viewport, indent width, action
slots, label constraints, Reset icon, and compact action icons. `SetPaletteMode`
supports FollowUiTheme, Light, and Dark. Group headings derive nested depth
styling from the main group background.

`SetLabelAuto`, `SetLabelWidth`, and `SetLabelRatio` configure label geometry.
`SetPropertyExpanded` controls temporary rich-row expansion by stable property
id. `SelectProperty` and `GetSelectedPropertyId` provide programmatic selection.

## Demonstration and validation

- `Utilities/PropertyEditorDemo` is the complete interactive editor matrix.
- `Utilities/PropertyEditorTests` protects legacy/regression behavior.
- `Utilities/PropertyEditorOverrideCommitTest` specifically protects generic
  commit-time override activation through a real Choice/UiDropdown editor.
- `Utilities/PropertyEditorV1RunTests` covers v1 interaction and stress rules.
- `Utilities/PropertyEditorCoreProbe` proves the headless package boundary.

Also read `03_UI_MODEL_GUIDE.md` for model-driven controls and the package-local
`README.md`/`DESIGN.md` files for lower-level implementation details.
