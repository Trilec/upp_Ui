# UI OVERRIDE ROLLOUT — RECOVERY LOG

Remote GitHub is authoritative. Never force-update `main`.

BASE: `9619f93fd633b0e974c2c270f2b0f3437bc3b2ef` — `upp_Ui/main` when the four-control rollout began.

TASK: normalize UiList, UiBaseEdit, UiDropdown and UiAccordion against the accepted UiLabel PropertyEditor/Designer override convention.

STATUS: **SOURCE IMPLEMENTATION COMPLETE — WINDOWS VALIDATION PENDING.**

## PUBLISHED UI CHECKPOINTS

- `a53154e19afc2ee654821775373d6a51435e7cc2` — owning List style becomes authoritative for the built-in row renderer and temporary connector-probe files are absent from the resulting tree.
- `d0579b8753748ca765710f6c29805d2859ddf6aa` — List renderer package membership and focused style-contract integration complete.
- `97d1531192f712365cddea1f9390a1a031e01836` — `striped_rows` joins `UiList::Style::Serialize()` and the focused contract gains a persistence round-trip assertion.
- this commit — four normalized interactive demo references, package routing, and recovery-document fold-forward.

## PUBLISHED DESIGNER CHECKPOINTS

- `Trilec/upp_uidesigner` `c27f499c8d51ad73037d9a60481bb73d870d38a7` — dedicated normalized UiList + UiBaseEdit adapters and focused contract test.
- `Trilec/upp_uidesigner` `ec02f1cbcc040f70ad55e656b98ec64640142cec` — dedicated normalized UiDropdown + UiAccordion adapters, legacy Color-to-FillRecipe bridge, focused composite test and coverage docs.

## IMPLEMENTED CONTRACT

UiList:
- viewport Face/Frame/Focus/Shadow/Highlight remains control-owned;
- built-in row renderers project the owning List style rather than independently re-resolving the global theme;
- row geometry/state, checks/metadata, badges and drag presentation are List-owned;
- explicit custom item renderers retain renderer ownership;
- renderer-pool invalidation remains bounded and preserves the unchanged-second-Layout performance contract;
- striped rows persist through Style serialization.

UiBaseEdit:
- normalized Designer ownership: General, Face, Frame, Ink, Typography, Content Margin, Editing, Underline, Whitespace, Focus, Shadow, Highlight;
- existing authored ids are preserved;
- Face uses FillRecipe in Designer while the interactive demo keeps a single local config as preview authority.

UiDropdown:
- collapsed control chrome remains separate from nested `Popup/Layout`, `Popup/Face`, `Popup/Frame`, `Popup/Items/*`, `Popup/Marker`, `Popup/Badge` and Drag ownership;
- legacy generic authored ids remain recognized;
- old Color Face values normalize to Solid FillRecipe rather than being discarded.

UiAccordion:
- outer chrome remains separate from Section, composed `Header/*`, composed `Body/*`, Behaviour and Animation domains;
- existing `style_*` authored ids remain recognized;
- Header is still `UiTitleCard::Style`; Body is still `UiPanel::Style`; no flattened parallel style state was added.

Resource rule:
- Designer Skin/custom-image resource editing remains deferred until the theme-adapter preview contract can resolve document resources;
- do not add raw file-path or compatibility Skin workarounds.

## FINAL DEMO SLICE

- `examples/UiListDemo/NormalizedDemo.cpp` is the compiled List ownership reference; the previous large `main.cpp` stays in the repository unmodified for history/reference.
- `examples/UiDropdownDemo/NormalizedDemo.cpp` is the compiled Dropdown ownership reference; the previous large `main.cpp` likewise remains unmodified.
- `examples/UiLineEditDemo/main.cpp` is rebuilt as the compact UiBaseEdit ownership reference.
- `examples/UiAccordionDemo/main.cpp` is rebuilt as the compact composite ownership reference.
- each demo keeps one config/model authority; no duplicate semantic store is introduced.
- every visible style ownership section contains runtime-backed controls; no intentionally empty style group remains.

## VALIDATION PENDING

`Trilec/upp_Ui` final `main`:
- `Utilities/UiListStyleContractTest` Debug + Release: require `UILIST_STYLE_CONTRACT_SUMMARY checks=13 failed=0`.
- `Utilities/UiModelViewPerformanceTest` Debug + Release: require 52/0 and unchanged second List Layout = zero renderer relayouts.
- build all four normalized demos Debug + Release; launch each for a focused GUI smoke.
- `git diff --check` and clean status required.

`Trilec/upp_uidesigner` `main` (source checkpoint `ec02f1cbcc040f70ad55e656b98ec64640142cec`):
- `tests/ListEditThemeAdapterTest` Debug + Release: zero failures.
- `tests/DropdownAccordionThemeAdapterTest` Debug + Release: zero failures.
- UiDesigner Debug + Release build and GUI smoke.
- verify FillRecipe/QuadGradient refresh/codegen behavior and legacy Color compatibility.
- verify no fake Skin resource-path editor.

The assistant environment cannot perform the Windows U++ compile/runtime gate. A platform failure is evidence to diagnose; do not weaken tests, restore retired model accessors or redesign ownership during validation.

## NEXT

1. Windows validate the accumulated meaningful checkpoints above.
2. Source-review and publish only genuinely mechanical Windows corrections if needed.
3. Close the four-control rollout when the focused tests/builds/GUI smokes are accepted.
4. Then continue remaining control normalization from the same UiLabel-derived convention.
