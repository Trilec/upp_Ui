# 12 — Ui Demo Modernization Plan

## Status

BASE: `01c8cd7298dcf5e368c2fa5c8467ea128a6d1fb4` (`main`)

OBJECTIVE: Replace the remaining legacy `examples/*Demo` applications with a coherent generation of full control demos based on the `UiLabelDemo` shell, production `PropertyEditor`, canonical override grouping, and model-authoritative Data pages where appropriate.

`UiLabelDemo` remains the canonical reference. This is a demo/documentation modernization program; it must not redesign production controls merely to make a demo convenient.

## Non-negotiable architecture

Every migrated full demo should use the same visual and interaction language while remaining a self-contained U++ package:

```text
UiTitleCard header
  control identity/title/subtitle
                                      Theme   Help   Exit

Main preview / live interaction       Right rail
                                      [Inspector]
                                      [Overrides]
                                      [Data]*
                                      [Code]
```

`Data` is present only where the control owns or binds meaningful model/domain data.

The shell is a pattern, not a new `DemoBase` framework. Do not hide the demos behind a demo-only inheritance layer. Reuse production Ui/PropertyEditor facilities and only extract genuinely reusable production helpers after repeated need is proven.

### Inspector

- Represents the control's real public API and behaviour.
- Uses production `PropertyEditor` and the most expressive available adapters.
- PropertyEditor models are authoritative authored demo state.
- Preview and generated code read the same state.
- No hand-built parallel property-row system.

### Theme Overrides

- Exposes only style fields that are live in the current control.
- Uses the canonical grammar in `11_UI_PROPERTY_OVERRIDE_LAYOUT.md`.
- Common groups remain API nouns: General, Face/Skin, Frame, Ink, Icon, Typography, Content Margin, Focus, Shadow, Highlight.
- Composite controls preserve nested domains such as Slider/Track/Thumb, Dropdown/Popup, Accordion/Header/Body.
- State vocabulary follows the real control; do not manufacture Normal/Hot/Pressed/Disabled symmetry where it does not exist.
- Demo and UiDesigner property ids, labels, group paths, ordering and inherited/override semantics should converge.

### Data page

Required for the normal model-backed collection controls:

- `UiDropdownDemo` -> active `UiListModel`
- `UiListDemo` -> active `UiListModel`
- `UiTreeDemo` -> active `UiTreeModel`
- `UiTableDemo` -> active `UiTableModel`
- `UiGalleryDemo` -> active `UiListModel`
- `UiMenuDemo` -> active `UiMenuModel`

The Data page edits the same active model driving the preview. It must not maintain a second array/tree/table mirror that requires manual synchronization. Mutations use the public model APIs and respect request-first control semantics.

`UiGraphDemo` and `UiDocDemo` have richer domain models. Their eventual model/data page should preserve `UiGraphModel` / `UiDocCore` authority and existing specialized tooling rather than forcing them into a generic list editor.

Data surfaces should support the operations meaningful to the model: add, remove, rename/edit, reorder/reparent where supported, and useful item/header metadata. They should make model changes visible immediately in the live control.

### Code page

- Emits readable U++ using the current public API.
- Reflects Inspector state, active local overrides and model fixture where practical.
- Omits demo-only scaffolding.
- When no local override is authored, prefer concise role/theme-based code.

## Repository inventory

The current `examples/` tree contains 36 demo packages. `UiLabelDemo` is the accepted canonical baseline; the remaining 35 are migration candidates, subject to a fresh source/API audit before each replacement.

Large legacy files are a reason to simplify source structure, not a reason to preserve legacy demo architecture. Substantial replacements should normally use a readable `Demo.h` / `Demo.cpp` / `main.cpp` / `.upp` split. Small demos may remain one source file.

Do not retain old and new demo implementations side by side once the replacement is accepted. Files such as legacy `NormalizedDemo.cpp` should be removed only after confirming that no package/test still depends on them.

## Implementation order

### Pilot sequence — establish the generation

These five are deliberately first because together they prove the important patterns before broad rollout:

1. `UiButtonDemo`
   - establish the canonical shell, Inspector, Overrides and Code implementation on a conventional styled control;
   - verify state palette, icon/content, sizing, behaviour and generated code.

2. `UiDropdownDemo`
   - establish the fourth `Data` page;
   - edit the bound `UiListModel` directly;
   - exercise collapsed control + nested Popup override domains;
   - retire legacy/normalized dual-demo structure if no longer required.

3. `UiSliderDemo`
   - establish a composite control override layout with real Track/Thumb domains;
   - use bounded numeric/range PropertyEditor presentations rather than generic text fields.

4. `UiCheckBoxDemo`
   - establish indicator/content grouping and checked/indeterminate/disabled interaction where supported.

5. `UiToggleDemo`
   - prove the same shell can express a related control without copying a second configuration architecture.

Do not begin a large parallel rewrite before these five have been source-reviewed and Windows-smoked. Lessons from the pilot become the exact migration pattern for later waves.

### Wave 2 — related interaction/value controls

- `UiRadioButtonDemo`
- `UiRangeSliderDemo`
- `UiSplitButtonDemo`
- `UiProgressBarDemo`
- `UiScrollBarDemo`
- `UiMatrixSelectorDemo`

Focus: stateful indicators, ranges, orientation, compound menus/actions and control-specific style domains.

### Wave 3 — remaining model/data controls

- `UiListDemo`
- `UiTreeDemo`
- `UiTableDemo`
- `UiGalleryDemo`
- `UiMenuDemo`

All receive the Data page and demonstrate the canonical model-binding contract. Use the active model as semantic authority, preserve virtualization, and never create one persistent child editor/control per logical record merely for the demo.

### Wave 4 — edit/input family

- `UiLineEditDemo`
- `UiMultiEditDemo`
- `UiPasswordEditDemo`
- `UiMaskEditDemo`
- `UiIntFloatDemo`
- `UiDateTimeDemo`
- `UiFontSelectorDemo`

Follow the BaseEdit-style organization where applicable: common surface groups plus Editing, Underline, Whitespace and control-specific value/validation behaviour.

### Wave 5 — containers/navigation/layout surfaces

- `UiPanelDemo`
- `UiScrollPanelDemo`
- `UiSplitterDemo`
- `UiTabDemo`
- `UiAccordionDemo`
- `UiBreadcrumbsDemo`
- `UiTitleCardDemo`

Preserve a generous live preview. Composite controls use nested real sub-style domains; for example Accordion keeps Header and Body rather than flattening them into a generic appearance list.

### Wave 6 — specialized/large demos

- `UiColorPickerDemo`
- `UiThemeDemo`
- `UiOsFileDialogDemo`
- `UiGraphDemo`
- `UiDocDemo`

These are last because their demos already carry specialized workflows or platform/domain behaviour. Normalize shell and PropertyEditor organization without discarding useful specialized functionality.

`UiGraphDemo` must not be disturbed while the currently published Graph hardening line is still awaiting its Windows acceptance restart. Refresh and preserve that evidence before touching Graph demo-only files.

## Per-demo migration procedure

Before replacing any demo:

1. Refresh remote `main` and record HEAD.
2. Read the complete current demo package, `.upp`, demonstrated control header/style, relevant implementation paint/layout/interaction files, and associated tests.
3. Inventory the real public Inspector API and live style surface.
4. Map override ids/groups to `11_UI_PROPERTY_OVERRIDE_LAYOUT.md`; preserve real control-specific nested domains.
5. Build the new UiLabel-style shell and generous live preview.
6. Build production PropertyEditor Inspector and Theme Overrides models.
7. Add Data page only where the control/domain model warrants it.
8. Add generated Code page using the real public API.
9. Remove obsolete legacy demo paths only after callers/package membership are checked.
10. Review the full diff, declarations/definitions, includes, `.upp` dependencies and `git diff --check`.
11. Publish the coherent demo checkpoint directly to `main` and verify the remote commit/diff.
12. Update `docs/ACTIVE_WORK.md` with BASE / TASK / TOUCHED / STATUS / PUBLISHED / VALIDATION / NEXT ACTION.

## Definition of done for each demo

A migrated demo is not complete merely because it resembles `UiLabelDemo`.

Required:

- canonical TitleCard header with compact Theme / Help / Exit actions;
- generous real-control preview;
- Inspector uses production `PropertyEditor` and maps to live public APIs;
- Theme Overrides uses actual supported style fields and canonical grouping;
- inherited versus authored override behaviour is explicit;
- Code page follows current authored state;
- Data page is present and model-authoritative where required;
- Light/Dark switching updates the complete demo coherently;
- normal control interaction remains available in preview;
- no duplicate authoritative demo configuration/model store;
- no UiDesigner dependency;
- package membership/dependencies are correct;
- source is readable enough to serve as executable documentation.

## Validation policy

### During implementation

Every published demo checkpoint receives static review and `git diff --check` where available. No Windows PASS is claimed from source review alone.

### Windows validation

For each meaningful checkpoint, build the migrated demo in Debug first and launch it. Manually check:

- header/sizing at normal and resized window dimensions;
- Inspector preview and commit behaviour;
- override enable/disable/inheritance and Reset semantics;
- Code page updates;
- Data mutation/model synchronization for data controls;
- primary keyboard/mouse interaction of the demonstrated control;
- Light/Dark theme;
- no obvious clipping, stale state or layout collapse.

Run the control's relevant deterministic Utility tests. For model-backed waves also preserve the shared model-binding/mutation/data-model suites. Run Release builds at wave acceptance, not necessarily after every trivial source edit.

Stop on the first compile/runtime/regression failure and correct the root cause rather than weakening a test or hiding a control capability.

## Publishing cadence

Work on `main` only.

- Refresh `main` immediately before each publish.
- A substantial demo replacement normally deserves its own checkpoint.
- Closely related small demos may share a commit only when they share the same implementation/validation path.
- Do not publish one giant 35-demo rewrite.
- Do not create meaningless one-line checkpoint commits.
- After every publish, fetch/verify the resulting remote commit and inspect its diff.
- Keep `docs/ACTIVE_WORK.md` current so a crashed session can continue from repository state without chat reconstruction.
- Never force-update `main` or overwrite unrelated concurrent work.

## Documentation changes to make with the first implementation checkpoint

Update `04_UI_DEMO_GUIDE.md` to formalize the optional fourth `Data` page for model/domain-backed demos while keeping Inspector / Theme Overrides / Code as the standard three-page shell for ordinary controls.

Keep `11_UI_PROPERTY_OVERRIDE_LAYOUT.md` as the naming/grouping authority. Add control-specific examples there only when migration work proves a stable grouping that will also be useful to UiDesigner.

## Completion target

The program is complete when all 36 current demo packages have been audited and every applicable demo either:

- uses the new canonical shell/PropertyEditor architecture; or
- has a documented, justified specialized structure that preserves the same interaction language without forcing an unsuitable generic shell.

The final acceptance should include a repository-wide demo build/smoke sweep, relevant deterministic Utility suites, and a final source audit ensuring the legacy demo generation is no longer being used as a reference.
