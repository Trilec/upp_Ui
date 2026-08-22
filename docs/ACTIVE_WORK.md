# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Refresh current `main` before work/publication and preserve unrelated concurrent advances.

Detailed architecture/history:
- `docs/04_UI_DEMO_GUIDE.md`
- `docs/05_UI_PROPERTY_EDITOR_GUIDE.md`
- `docs/11_UI_PROPERTY_OVERRIDE_LAYOUT.md`
- `docs/12_UI_DEMO_MODERNIZATION_PLAN.md`
- `docs/13_UI_MODEL_MUTATION_SCALE_CONTRACT.md`
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`

## CURRENT SUPERVISORY STATE — 2026-08-23

STATUS: **DEMO MODERNIZATION TRANCHE SOURCE/BOOKKEEPING COMPLETE; WINDOWS BUILD + VISUAL ACCEPTANCE PENDING.**

Authoritative branch: `main`.

Source checkpoint immediately before this ACTIVE_WORK update:

- `52ea845aa23da14cdbee577898cfa1d65e7aca1a` — demo-modernization plan updated for self-contained family demos and code-generation contract;
- `fc7d59bac6a3efaa620db352e3d7e2252f9d0f82` — obsolete Dropdown `NormalizedDemo.cpp` removed after package-membership review;
- `80c74d9c14a6aa7dfa0e087c768796210ba4ac35` / parent `0bb3fbe1c79ede92e0645a4d37d389004a13c6d5` — standalone RangeSlider demo retired after its material was merged into the Slider family demo;
- `de5848451fc2387c5dd68de7019092a293de8113` — production Code view added to combined Edit family;
- `1adfcf17fbd46ef58b43c08241537fe221a15275` + `175899286305b2f98117cfb76c03f2c6bd1d5f72` — Tab rebuilt around PropertyEditor and code-editor setup corrected;
- `2f350e54aade41a904032039722566d966669031` + `f019cc24a69771cc9f7b133a75908070a367fad6` — Slider/RangeSlider combined family + callback hardening;
- `b5be9c34d2468f19f6dc31acc16bd66b762181da` + `7e4e198ae2ae0ddae5cb3ebebf2db7d7707118b3` — Dropdown rebuilt with model-authoritative Data page and Code modes;
- `38e526eb070a459a6e1ec093ca1bd9b57db984e7` + `8c9267bceaea92f914ab657d04a389a3c8aeb2be` — Toggle rebuilt around PropertyEditor and code-editor setup corrected;
- `c152d47e190ea096bb372157f2f758d66c1533ae` + `e383a468a6a91e983b160559f3e1f2c2b52ba062` — RadioButton rebuilt and API/code-generation corrections applied;
- `d6b432c9c6d8bde3ef581514db82b349e0de241b` — Code view added to CheckBox;
- `9eed37895c6967b830d210e207e590b79e2752a9` — Button explicit icon-size/render-mode contract coverage;
- `9f08dd9d2ff7f3806eb45a84c7fc66521d8d03f3` — combined selectable Edit family foundation.

Shared PropertyEditor baseline retained beneath the tranche:

- `00502ca0b779187314d72d51fc11fbb16f5ebad2` — Cardinal4 editors preserve canonical caller values;
- `9d89914ef3068bd88b53697a3552051d174efe0d` — focused numeric mouse-wheel editing remains with the numeric editor rather than scrolling the outer PropertyEditor.

No Windows PASS is claimed for this tranche yet.

---

## DEMO RULES NOW LOCKED

### Standalone source

Every demo must remain understandable from its own package. Do not create a shared demo base/framework solely to remove repeated shell/code-generation setup. Production `Ui` / PropertyEditor facilities remain shared; demo application scaffolding does not.

### PropertyEditor

- production PropertyEditor is the normal property UI;
- bounded numerics retain value-to-slider interaction;
- focused numeric mouse wheel edits the value;
- directional four-way values use Cardinal4 where appropriate;
- icon/colour/font/range rich editors remain available;
- normal property editing is independent of style inherited/authored state;
- 38% label/value divider remains the demo convention.

### Generated code

Generated code is a first-class demo output and should be paste-oriented executable documentation.

Modernized demos prefer:

```text
Usage
Current changes
Full explicit
```

- Usage: concise public API relying on UiTheme.
- Current changes: normal setup plus only local design/style deviations.
- Full explicit: complete relevant local style for learning/debugging.

Button and Label were source-reviewed again. They predate the three-mode selector but already generate from their authoritative Inspector/Override models and emit local style from authored overrides; no risky cosmetic rewrite was made merely to make the selector identical.

---

## CURRENT DEMO TRANCHE

### UiLabelDemo

Canonical interaction reference. Retains Inspector / Theme Overrides / generated C++. Current PropertyEditor numeric, Cardinal4, icon and render-mode behavior is the reference for the tranche.

### UiButtonDemo

Already production-PropertyEditor based. Re-reviewed against current Button API and generated code. `UiButtonInteractionContractTest` now covers explicit non-square icon size, render mode and scale-to-content in addition to interaction geometry.

### UiCheckBoxDemo

Production PropertyEditor covers Content, State, Layout, Body and Indicator. The real checkbox updates PropertyEditor state on user interaction. Code view now demonstrates ordinary setup and explicit local style.

### UiRadioButtonDemo

Standalone focused exclusive-selection demo. Three real sibling radios show group semantics. PropertyEditor covers visual/state/layout/body/indicator. Code modes are Usage / Current changes / Full explicit.

### UiToggleDemo

Standalone PropertyEditor demo with State, Layout, Track and Thumb groups. Real toggle interaction synchronizes state. Code modes separate normal usage from local Track/Thumb design.

### UiDropdownDemo

Properties / Data / Code. The Data page edits the exact external `UiListModel` bound to the live Dropdown. It demonstrates collapsed control and Popup/marker/reorder behavior. Obsolete `NormalizedDemo.cpp` is removed and the package now contains only the current implementation.

### UiSliderDemo — Slider family

One package now demonstrates both `UiSlider` and `UiRangeSlider`, which share `UiSlider::Style`. Both remain visible; selection changes the PropertyEditor and generated code. RangeSlider uses the production range/adjustable-range editing vocabulary. The old `UiRangeSliderDemo` package is retired.

### UiEditDemo — Edit family

One package demonstrates `UiLineEdit`, `UiPasswordEdit`, `UiMaskEdit` and `UiMultiEdit`. All remain visible; selecting a sample changes the one PropertyEditor and Code output. Common `UiBaseEdit` groups stay common while Password/Mask/Multi-line properties are subtype-specific.

Older specialized edit demo packages remain until the combined family is Windows/visual accepted and any unique fixture value is checked.

### UiTabDemo

Focused standalone Tab explanation. PropertyEditor separates Behavior, Layout, Typography, Body, Tab Surface and Indicator so the difficult nested style ownership is visible. Placement/Icon Side use Cardinal4. Code modes deliberately show which `UiTab::Style` fields control body, individual tab surface and active indicator.

---

## STATIC REVIEW / BOOKKEEPING COMPLETED

- reviewed the complete tranche diff from prior accepted demo checkpoint `914efd9b1dd661ef197db4af2a73994b7b73fd67` through the current source line;
- package files for RadioButton, Toggle, Dropdown, Slider and Tab directly depend on `Ui` + `Utilities/PropertyEditor`; no UiDesigner dependency was introduced;
- Dropdown package membership contains only `main.cpp`; obsolete `NormalizedDemo.cpp` has been removed;
- RangeSlider duplicate package files have been removed; production `UiRangeSlider` remains unchanged and is exercised by the combined Slider demo;
- RadioButton invalid icon/API setup and the new demos' read-only code-editor chaining/callback issues were corrected before this gate;
- Button and Label code generation were re-read: both generate from the same authoritative property/override state used by their preview and separate normal usage from active local style conceptually;
- documentation now explicitly forbids a shared demo application framework and records family-combination/code-generation rules.

Windows compile/runtime validation remains required before acceptance.

---

## WINDOWS VALIDATION GATE

Validate **current final `upp_Ui/main`**, not an earlier ancestor.

Automated Debug + Release:

1. `Utilities/PropertyEditorV1RunTests` — require `Fails: 0`.
2. `Utilities/UiButtonInteractionContractTest` — require `Fails: 0`.
3. Run the existing RangeSlider-focused deterministic test package if present in the local assembly; require zero failures. If no separate package exists, report that rather than inventing one.

Build Debug + Release:

4. `UiLabelDemo`
5. `UiButtonDemo`
6. `UiCheckBoxDemo`
7. `UiRadioButtonDemo`
8. `UiToggleDemo`
9. `UiDropdownDemo`
10. `UiSliderDemo`
11. `UiEditDemo`
12. `UiTabDemo`

Visual / interaction smoke:

- Label: slider-toggle numerics, focused mouse wheel, Cardinal4 Icon Side, icon size and Auto/PreserveColor/MonoTint, generated code.
- Button: same PropertyEditor interactions; exact/non-square icon sizing; checkable interaction; Overrides and generated code remain coherent.
- CheckBox: Classic/Chip/List, two-/tri-state behavior, user state sync, icon/colour/numeric editors, Code view.
- Radio: exclusive sibling selection, visual/indicator settings, all three Code modes.
- Toggle: on/off interaction, Track/Thumb geometry and colors, Cardinal4 Track Side, all Code modes.
- Dropdown: Properties/Data/Code; Data Add/Remove/Edit/Up/Down mutate the same live model; popup/marker/reorder options; all Code modes.
- Slider family: select Slider then RangeSlider; each PropertyEditor is correct; live drag/wheel sync; Cardinal4 Tick Side; range editing; shared Track/Thumb style; all Code modes.
- Edit family: select Line/Password/Mask/Multi; each retains its state; subtype-only properties appear correctly; password/mask/multiline behavior; Code follows selected type.
- Tab: all placements/visuals, tab icon/layout controls, Body vs Tab Surface vs Indicator styling, active tab interaction, all Code modes.
- Light/Dark for every demo should update preview and PropertyEditor coherently.

Leave `UiTabDemo` and `UiEditDemo` running for Curt to inspect/close after the smoke.

If a compile/runtime failure is substantive, stop and report exact HEAD, package/configuration, complete diagnostic and shortest reproduction. Mechanical U++ build/API corrections are acceptable; do not redesign demo architecture during validation.

---

## OTHER PENDING LINES — DO NOT CONFLATE

Separate older validation lines remain for:

- UiDesigner Theme Studio / PropertyEditor colour-transfer integration;
- shared model / List-Gallery-Dropdown-Tree scale audit;
- Graph final revalidation;
- SymbolPicker/Gallery convergence.

Do not move those production subsystems under this demo gate unless a concrete shared regression requires it.

## NEXT AFTER ACCEPTANCE

Continue with a small group such as SplitButton / ProgressBar / ScrollBar / MatrixSelector, then the remaining model-backed List/Tree/Table/Gallery/Menu demos once the accepted Dropdown Data-page pattern is reused deliberately.
