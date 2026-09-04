````markdown
# 04 — Ui Demo Guide

This guide defines the intended demo architecture for `upp_Ui`.

The current **UiLabelDemo** is the canonical reference implementation. It is the
de facto baseline for future full control demos and should be consulted before
creating or substantially redesigning another demo.

The goal is to make every full demo:

- visually consistent with the wider Ui ecosystem;
- useful for exploring the real public API of the control;
- useful as readable example source code;
- powered by the production `PropertyEditor`;
- self-contained and easy for both humans and agents to understand.

## Drawing/shape demo rule

Demos must demonstrate production drawing APIs rather than inventing demo-only
geometry. Normal-control examples should use direct Draw/native Painter or
`UiShapes`/`UiShapePath`; dense scene demos may use the same direct
`UiGeometry` path as the production control. Never introduce fixed curve
sample counts in a demo. See `07_UI_DRAWING_GUIDE.md` and
`07_UI_DRAWING_GUIDE.md`.

## Direction

Future Ui demos share a visual **language** with UiDesigner without depending on
UiDesigner itself.

Each demo remains its own U++ package and uses the normal reusable `Ui` and
`PropertyEditor` infrastructure.

> There is **no** mandatory shared `DemoBase` or `DemoFramework` application
> layer. Small amounts of repeated shell/setup code are preferable to hiding the
> construction of a demo behind another framework.

The important commonality is the **pattern**, not inheritance from a shared demo
application.

## Canonical reference

`examples/UiLabelDemo` establishes the current baseline for:

- package structure;
- header and application actions;
- preview/property-area proportions;
- Inspector / Theme Overrides / optional Data / Code navigation;
- `UiStack` page switching;
- production `PropertyEditor` integration;
- model-driven preview updates;
- explicit inherited versus local style overrides;
- generated U++ example code;
- source naming and organisation.

When implementing another full demo, begin by studying `UiLabelDemo` and adapt
its structure to the needs of the new control rather than inventing another
parallel demo architecture.

The result should look and feel familiar while remaining specific to the control
being demonstrated.

## General structure

A full control demo broadly contains three areas.

### 1. Top header

Use a `UiTitleCard` to identify the demonstrated control.

Typical content:

- **Title** — the control name, e.g. `UiLabel`;
- **Subtitle** — a short description of the control's purpose;
- optional control/icon identity.

At the opposite side provide a small, consistent set of application actions:

- **Light/Dark theme**
- **Help**
- **Exit**

These should remain compact and unobtrusive. The visual language can resemble
UiDesigner, but the implementation belongs entirely to the demo and reusable Ui
controls.

### 2. Main preview

Provide a generous live preview using the **real control being demonstrated**.

The preview should have enough room to inspect meaningful aspects of the
control, including where appropriate:

- size and geometry;
- text and content;
- enabled/disabled behaviour;
- hover/pressed/focus states;
- selection;
- alignment;
- orientation;
- wrapping;
- icons/media;
- ranges and values;
- interaction;
- visual styling;
- control-specific behaviour.

Do not shrink the preview merely to expose more properties.

The preview is the primary visual result of the current PropertyEditor state.

> This is not a miniature UiDesigner. There is no generic canvas selection
> system. If the demonstrated control naturally contains selectable or
> interactive items, those normal control interactions remain available.

### 3. Right-hand property area

The right side follows the same conceptual language as UiDesigner:

1. **Inspector**
2. **Theme Overrides**
3. **Data** — only for model/domain-backed controls
4. **Code**

Ordinary controls omit Data and retain the standard three-page Inspector /
Theme Overrides / Code shell. Use compact `UiToolButton` actions at the top to
select the current page and a `UiStack` underneath to display the selected
content.

Only one page should normally be active at a time.

---

## Inspector

The Inspector represents the **normal public API and behaviour** of the
demonstrated control.

Expose as much useful functionality as can reasonably be changed interactively.

Depending on the control, this can include:

- content and values;
- semantic roles;
- text size;
- enabled/read-only/selectable state;
- alignment;
- direction and orientation;
- dimensions;
- ranges;
- steps;
- icons and media;
- selection;
- wrapping;
- behavioural options;
- control-specific features.

The intention is that someone unfamiliar with the control can explore the
Inspector and quickly understand what the public control is capable of.

Inspector properties should normally map clearly to real public control APIs.
Avoid maintaining a second demo-only interpretation of the control.

### Choose the right property presentation

Do not default every value to a text field or dropdown.

Use the most expressive PropertyEditor adapter available.

For example:

- spatial side/direction/position → `UiMatrixSelector` adapter;
- bounded numeric value → numeric field or slider-capable editor;
- range → range adapter;
- colour → colour editor;
- ordered colour set → colour palette/matrix editor;
- icon → icon adapter;
- font → font adapter;
- image → provider-backed image adapter;
- curve → curve or Bézier adapter;
- compound values → expandable inline editor where appropriate.

Where a matrix communicates the value more clearly than a dropdown, prefer the
matrix.

The current `Cardinal4` matrix use in `UiLabelDemo` is a good example: choosing
Top/Right/Bottom/Left visually is clearer than reading those four entries from a
generic dropdown.

---

## Theme Overrides

Theme Overrides represents the **actual styling API supported by the control**.

The user should be able to distinguish between:

- appearance inherited from the current theme/role;
- an explicit local style override.

Inactive override properties remain inherited. Activating an override applies
the local authored value.

Expose only style capabilities that genuinely exist on the demonstrated control.

Depending on the control this may include:

- normal/hot/pressed/disabled palette values;
- face/fill;
- frame colours and width;
- radius;
- ink;
- icon ink;
- typography;
- content margins;
- focus treatment;
- shadow;
- highlight;
- skins/images;
- control-specific style metrics.

Do not create a generic collection of styling options merely because
PropertyEditor can display them.

The Overrides page should document the real style contract of the control.

---

## Data

A Data page is appropriate only when the demonstrated control owns or binds
meaningful model/domain data, such as Dropdown, List, Tree, Table, Gallery or
Menu.

The page edits the **same active model that drives the live preview**. Do not
maintain a second demo-only array, tree or table and manually synchronize it with
the control.

Use the public model API for mutations such as add, remove, edit, reorder or
reparent where those operations are supported. Changes should become visible in
the live control immediately and continue to respect the control's normal
request-first interaction semantics.

Richer domain controls such as Graph or Doc may keep specialized model tooling
rather than being forced into a generic list-style Data editor. Their underlying
`UiGraphModel`, `UiDocCore` or equivalent production model remains authoritative.

Ordinary non-model controls should not display an empty Data page.

---

## PropertyEditor usage

Full demos should use the production `PropertyEditor` and
`PropertyEditorCore` rather than hand-built property rows.

Use PropertyEditor to its full practical potential.

The demo should be a useful example of both:

1. the demonstrated Ui control; and
2. real-world PropertyEditor integration.

### Models are authoritative

Prefer the pattern established by `UiLabelDemo`:

- PropertyEditor models contain the authored property state;
- the live preview reads that state;
- generated code reads that same state;
- style override activation is represented explicitly;
- avoid parallel configuration structures that can drift apart.

For simple controls this may remain very small. More complex controls may need
separate Inspector and Override models, as `UiLabelDemo` does.

### Use first-class adapters

Where appropriate use the PropertyEditor helpers/adapters for:

- Matrix
- Range
- Adjustable Range
- Color
- Color Palette
- Fill Recipe
- Icon
- Font
- Image
- Curve
- Bézier Curve
- Numeric slider/text editing
- expandable compound values

Refer to `05_UI_PROPERTY_EDITOR_GUIDE.md` for the complete current PropertyEditor
contract.

### Keep domain-specific behaviour outside PropertyEditor

PropertyEditor provides reusable schema, presentation and editing behaviour.

The demo remains responsible for:

- interpreting the values for the control;
- applying them to the preview;
- generating useful example code;
- application-specific resource providers;
- any demo-specific help.

Do not add demo-specific meaning back into the generic PropertyEditor packages.

---

## Code

The Code page shows useful U++ source corresponding to the current configuration.

`UiLabelDemo` is the current reference for this behaviour.

The generated code should:

- use the real public control API;
- reflect current Inspector values;
- include active local overrides where relevant;
- omit unnecessary demo infrastructure;
- be readable;
- be suitable for selecting/copying as a practical starting point.

The generated code does not need to reproduce the demo application itself. Its
purpose is to show how the configured control would be constructed and styled in
ordinary U++ application code.

Where no explicit style overrides are active, prefer concise role/theme-based
code rather than emitting every resolved style field.

---


## Generated-code modes

For substantial builder-style demos, generated code should answer three useful
questions where practical:

1. **Usage** — concise ordinary public API, relying on UiTheme.
2. **Current changes** — usage plus only authored local changes.
3. **Full explicit** — the complete relevant local style recipe for
   learning/debugging.

A mature reference demo does not need to be destabilized merely to make the
selector UI identical if it already emits equivalent useful code.

## Family demos

Combine demos only when controls genuinely share a production API/style
foundation.

Current examples:

- the Edit family may share one demo for Line/Password/Mask/MultiEdit;
- Slider and RangeSlider may share one family demo because they share the slider
  style foundation.

Do not combine controls merely because their visuals resemble one another.

## Model-backed Data pages

For model-backed collection controls, the Data page edits the **same active
model** driving the preview.

Examples:

- Dropdown/List/Gallery -> `UiListModel`;
- Tree -> `UiTreeModel`;
- Table -> `UiTableModel`;
- Menu -> `UiMenuModel`.

Never maintain a second demo-only mirror collection.

## Source structure and readability

A demo is documentation in executable form.

Its source therefore needs to remain easy to navigate.

Prefer clear, descriptive methods such as:

```cpp
BuildHeader();
BuildPreview();
BuildRightRail();
BuildInspectorModel();
BuildOverrideModel();
ConfigureEditors();
ConnectEvents();
ApplyProjection();
ApplyTheme();
UpdateGeneratedCode();
````

The exact names depend on the control, but the source should reveal the demo's
structure without requiring a reader to reverse-engineer it.

For a substantial demo, a small package split such as:

```text
UiControlDemo/
    UiControlDemo.h
    UiControlDemo.cpp
    main.cpp
    UiControlDemo.upp
```

is preferable to an excessively large `main.cpp`.

Do not fragment a small demo unnecessarily.

### Documentation inside the source

Provide a concise header comment explaining:

* what the demo demonstrates;
* its main structure;
* how PropertyEditor state relates to the preview;
* any important non-obvious architectural decision.

Add comments where they clarify intent or unusual behaviour.

Do not comment every straightforward assignment.

The source should be approachable to:

* a developer learning the control;
* a developer learning PropertyEditor;
* an agent using the demo as an implementation reference.

---

## Self-contained packages

Despite sharing a common design language:

* each demo should remain understandable by opening its own package;
* there is no mandatory shared demo application framework;
* demos must not depend on UiDesigner;
* normal reusable `Ui` and `PropertyEditor` packages should be shared;
* a small amount of repeated shell construction is acceptable.

Avoid creating abstractions solely to eliminate harmless duplication between
demo applications.

If several demos eventually reveal a genuinely reusable **production control**
or PropertyEditor capability, that may belong in the library. A convenience that
only hides demo construction usually does not.

---

## Migration of existing demos

`UiLabelDemo` is now the accepted starting baseline for the new demo generation.

Existing demos do not need to be converted all at once.

Migrate them incrementally, using `UiLabelDemo` as the reference and adjusting
the Inspector, Overrides and preview to suit each control.

For each migration:

1. inspect the complete public API of the control;
2. identify the meaningful Inspector properties;
3. identify its real style/override surface;
4. choose the best available PropertyEditor adapters;
5. provide a generous live preview;
6. add a model-authoritative Data page when the control/domain warrants it;
7. generate useful corresponding code;
8. keep the package self-contained and readable;
9. compile and visually validate it before using it as the basis for another
   migration.

The objective is consistency without forcing every control into an identical
property schema.

`UiLabelDemo` establishes the **architecture and interaction language**. Each
control determines the actual content.

```
```
