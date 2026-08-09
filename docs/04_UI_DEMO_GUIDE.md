# 04 — Ui Demo Guide

Intended demo architecture for `upp_Ui`. This guide defines the target; the
first full implementation will be a new **Label demo** used as the reference
implementation. Do not convert every existing demo to this structure yet.

## Direction

Future Ui demos share a visual **language** with the new UiDesigner — without
depending on UiDesigner itself. Each demo is a self-contained U++ package.

> There is **no** mandatory shared `DemoBase`/`DemoFramework` application
> dependency. Small duplicated shell/setup code is preferable to hiding basic
> application construction behind a large shared demo framework. Sharing normal
> production `Ui`/`PropertyEditor` controls is expected; sharing a special demo
> application framework is not.

## General structure

Each full control demo broadly contains:

### 1. Top header

A `UiTitleCard` identifying the demonstrated control:

- **Title**: the control name (e.g. `UiLabel`).
- **Subtitle**: a short explanation of what the control is for.

Same clean visual concept as the new UiDesigner. At the opposite/top action
area provide a small consistent set of application actions:

- **Help**
- **Light/Dark theme toggle**
- **Exit**

These feel related to the Designer's top-level controls without requiring a
shared Designer implementation.

### 2. Main preview

A generous central preview area. The demonstrated control must have enough room
to genuinely inspect:

- sizing
- states
- text/content
- interaction
- alignment
- wrapping
- icons
- visual styling
- orientation
- control-specific behaviour

Do not make the preview tiny to fit more inspector controls. The preview is the
actual live result of the current settings.

> This is **not** a mini visual designer. There is no generic canvas-selection
> system, and the user normally does not select arbitrary children in the
> preview. If the demonstrated control naturally has selectable items, those
> normal control interactions remain available.

### 3. Right-hand property area

Use the `PropertyEditor` / `PropertyEditorCore` approach. The right-hand side
follows the same broad conceptual language as the new UiDesigner but only
exposes what is meaningful to demonstrating one control.

Conceptual areas:

1. **Inspector**
2. **Theme Overrides**
3. **Code**

#### Inspector

Expose essentially every useful public feature of the demonstrated control API
that can reasonably be changed interactively. Examples (depending on control):

- text/value
- enabled
- visible where useful
- size
- alignment
- orientation
- wrapping
- ranges
- steps
- icons
- selection
- behavioural flags
- control-specific options

Changing a property immediately updates the preview. The purpose is executable
API exploration: a developer should understand what the control can do by
manipulating the inspector.

#### Theme Overrides

Expose relevant visual/theme properties supported by the control, reflecting the
style/theme concepts in `02_UI_THEME_GUIDE.md`. Where practical, demonstrate the
difference between:

- inherited/current theme appearance;
- explicit local override.

Do not invent styling capabilities the control does not support.

#### Code

Provide a useful representation of the U++ code corresponding to the
demonstrated configuration. The exact implementation will be defined with the
first new Label demo. The intent: a developer can configure the control visually
and understand how the equivalent public API would be expressed in code.

### PropertyEditor integration

Inspector and Theme Overrides are constructed through the same `PropertyEditor`
family the library provides. The demo therefore also acts as a practical
demonstration of PropertyEditor integration — see `03_UI_MODEL_GUIDE.md`.

## Self-contained packages

Despite sharing this visual language:

- Each demo remains understandable by opening its own package.
- No mandatory common demo framework.
- Small duplicated shell/setup code is preferable to a large shared demo
  framework.

## Reference implementation

The first canonical implementation of this structure is the future **Label
demo**. Do not attempt to guess all details now; implement the target clearly and
let the Label implementation refine this guide afterward.

## Current state

Existing demos are retained as-is for this cleanup. They compile and act as a
manual regression suite. The migration to the structure above happens after the
Label reference implementation is accepted.
