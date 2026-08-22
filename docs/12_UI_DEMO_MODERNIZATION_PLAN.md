# 12 — Ui Demo Modernization Plan

## Status

OBJECTIVE: modernize the `examples/*Demo` applications into self-contained executable documentation built around the production `PropertyEditor`, real control APIs, live preview and useful generated U++.

`UiLabelDemo` remains the interaction reference. The current accepted source direction also includes Button, CheckBox, RadioButton, Toggle, Dropdown, the combined Slider/RangeSlider family, the combined Edit family, and Tab. Windows/visual acceptance is tracked separately in `docs/ACTIVE_WORK.md`.

## Non-negotiable architecture

Each demo stands on its own.

Do **not** create or reintroduce a shared `DemoBase`, `BuilderDemoSupport`, shared demo shell, shared generated-code helper layer, or other demo-only framework whose purpose is to reduce duplication between examples. Some duplicated setup is intentional: a programmer should be able to open one demo and understand that control without tracing through another package.

Reuse production `Ui` controls, `PropertyEditor`, models and theme resolvers. Shared behavior belongs in production code only when it is genuinely a reusable product capability.

Typical full demo shape:

```text
UiTitleCard header
  control identity/title/subtitle                  Theme / Help / Exit

live control preview                               right rail
                                                   Properties / Overrides
                                                   Data* / Code
```

`Data` is present only where the demonstrated control owns or binds meaningful model/domain data.

## PropertyEditor contract

- The PropertyEditor represents real public control properties or real style fields.
- Its model is the demo's authoritative authored configuration; the preview and generated code read the same state.
- Bounded numerics use the standard value-to-slider affordance.
- Focused numeric mouse-wheel edits adjust the value rather than scrolling the PropertyEditor.
- Cardinal directions use the shared `Cardinal4` matrix when the domain is Left / Right / Top / Bottom.
- Icon, colour, font and other rich values use the production PropertyEditor editors.
- Normal properties and authored/inherited style state remain separate concepts.
- Demos use the same concise grouping language as UiDesigner where the underlying API is the same.

## Theme / style editing

A standalone demo may expose a large amount of style detail because one of its purposes is to explain a difficult control in isolation.

- Start from the real `UiTheme::Resolve...()` style where available.
- Expose only real fields that affect the demonstrated control.
- Keep composite style ownership visible: e.g. Slider/Track/Thumb, Dropdown/Popup, Tab/Body/Tab Surface/Indicator, Accordion/Header/Body.
- Do not flatten unrelated style domains just to shorten the PropertyEditor.
- Do not invent state symmetry that the production control does not have.

Theme Studio answers “how should the application look as a whole?” A standalone demo answers “how do I use and locally configure this control?”

## Generated code is a first-class output

The Code page is not decoration. A demo is successful when a programmer can copy the generated U++ and understand how to reproduce what is being shown.

For modernized demos, prefer these code views where practical:

1. **Usage** — concise normal public API, relying on `UiTheme` for appearance.
2. **Current changes** — the normal usage plus only local design/style changes that differ from the demo defaults/theme.
3. **Full explicit** — the complete relevant local style recipe for learning/debugging.

Generated code must:

- use current public APIs;
- use meaningful variable names;
- separate ordinary control behavior from optional local design/style;
- include short comments where ownership is otherwise non-obvious;
- omit demo-window/layout scaffolding;
- include the active data/model fixture where that is essential to understanding the control;
- remain paste-oriented and readable rather than dumping internal state.

`UiLabelDemo` and `UiButtonDemo` predate the three-mode selector but were re-reviewed during the current slice: both already generate from the same Inspector/Overrides state and emit local style only from authored overrides. Do not destabilize those accepted reference demos merely to make the selector UI identical; converge them later only when there is a concrete UX benefit.

## Family demos

Combining demos is appropriate only when the controls genuinely share a production API/style foundation and separate full shells would mostly repeat the same lesson.

Current family decisions:

- `UiEditDemo` is the combined reference for `UiLineEdit`, `UiPasswordEdit`, `UiMaskEdit` and `UiMultiEdit`. All samples remain visible; selecting one changes the single PropertyEditor and code output. Common `UiBaseEdit` properties stay common while subtype-specific fields appear only for that subtype.
- `UiSliderDemo` is the combined Slider family reference for `UiSlider` and `UiRangeSlider`. The two controls share `UiSlider::Style`; selection changes value/range-specific properties and code generation. The former standalone `UiRangeSliderDemo` package has been retired.

Do not combine controls merely because they look similar. RadioButton and CheckBox, for example, remain useful focused examples even though their indicator concepts overlap.

## Model-backed Data pages

Required for ordinary model-backed collection controls:

- `UiDropdownDemo` -> the exact active `UiListModel`;
- `UiListDemo` -> active `UiListModel`;
- `UiTreeDemo` -> active `UiTreeModel`;
- `UiTableDemo` -> active `UiTableModel`;
- `UiGalleryDemo` -> active `UiListModel`;
- `UiMenuDemo` -> active `UiMenuModel`.

The Data page must edit the same model driving the preview. Never maintain a second mirror collection just for the demo.

The modernized Dropdown establishes the first accepted source pattern: Properties / Data / Code, with the Data page mutating the same external `UiListModel` bound to the live dropdown. The obsolete `NormalizedDemo.cpp` path has been removed.

## Current modernization tranche

Source-complete, Windows/visual acceptance pending:

- `UiLabelDemo` — canonical PropertyEditor reference; reviewed for current icon/editor and generated-code behavior.
- `UiButtonDemo` — canonical styled-control reference; reviewed against current Button API and generated code.
- `UiCheckBoxDemo` — PropertyEditor + production Code view.
- `UiRadioButtonDemo` — focused exclusive-selection example with Usage / Current changes / Full explicit code.
- `UiToggleDemo` — Track/Thumb focused example with the same code modes.
- `UiDropdownDemo` — model-authoritative Data page plus collapsed/Popup style and code modes.
- `UiSliderDemo` — combined Slider / RangeSlider family with shared style and range-specific properties/code.
- `UiEditDemo` — combined Line / Password / Mask / Multi-line family with subtype-specific code.
- `UiTabDemo` — focused explanation of the difficult Tab style domains with code modes.

## Likely next groups after this tranche is accepted

Interaction/value:

- `UiSplitButtonDemo`
- `UiProgressBarDemo`
- `UiScrollBarDemo`
- `UiMatrixSelectorDemo`

Model/data:

- `UiListDemo`
- `UiTreeDemo`
- `UiTableDemo`
- `UiGalleryDemo`
- `UiMenuDemo`

Remaining edit/value selectors:

- `UiIntFloatDemo`
- `UiDateTimeDemo`
- `UiFontSelectorDemo`

Containers/navigation:

- `UiPanelDemo`
- `UiScrollPanelDemo`
- `UiSplitterDemo`
- `UiAccordionDemo`
- `UiBreadcrumbsDemo`
- `UiTitleCardDemo`

Specialized/large demos remain later: ColorPicker, Theme, OS file dialog, Graph and Doc. Preserve current Graph validation evidence before touching Graph demo code.

## Per-demo procedure

1. Refresh remote `main` and record HEAD.
2. Read the complete existing demo package, `.upp`, production control header/style and relevant tests.
3. Inventory the real public behavior/style surface.
4. Build the self-contained live preview and production PropertyEditor.
5. Add a model-authoritative Data page only when required.
6. Produce clean generated U++ from the same authored state.
7. Remove obsolete demo-only implementations only after package/caller checks.
8. Review declarations/definitions, includes, API calls, package membership and the complete diff.
9. Publish a bounded checkpoint to `main`; never force update.
10. Run the Windows build/visual gate before calling the demo accepted.

## Definition of done

A migrated demo is complete when:

- it is self-contained and understandable without another demo package;
- it demonstrates the real production control and normal interaction;
- PropertyEditor changes visibly update that control;
- styling maps to actual style ownership;
- generated code is useful executable documentation;
- Data pages, where present, edit the real active model;
- Light/Dark works coherently;
- package dependencies are direct and appropriate;
- no obsolete competing demo implementation remains;
- relevant deterministic tests/builds and Windows visual smoke pass.

No source-review result alone is a Windows PASS.
