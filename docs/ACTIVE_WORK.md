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

STATUS: **DEMO MODERNIZATION + SEMANTIC PROPERTYEDITOR SOURCE/BOOKKEEPING COMPLETE; COMBINED WINDOWS BUILD + VISUAL ACCEPTANCE PENDING.**

Authoritative branch: `main`.

Do not claim Windows acceptance until the exact current main line is built and exercised on Windows/U++.

---

## 1. SEMANTIC PROPERTYEDITOR CHECKPOINT

Semantic PropertyEditor work was built directly on the previously recorded demo-modernization checkpoint:

- base: `a990601cafc06362f50e13af70e8b2b16a245997` — `Record demo modernization validation tranche`;
- semantic source line through: `9cd6d01cefdd89b5405738f21662b9380e740bac` — `Preserve semantic editor presentation state`.

The semantic line is 17 commits ahead of the recorded demo checkpoint, zero behind.

Important commits in the semantic line:

- `57be9122a37216b9c09c943f81bf5d38df7e58f7` — declare semantic adapter APIs;
- `3860e17d7895d658bccbb5a576ab5fce70786dd4` / `a8f6202c8a15e9544ea6be22873fa19ac3d3252a` — initial semantic implementation and package inclusion;
- `dc863afec414a0745c43d5a44ab82aecf244e17d` — complete standard adapter registration;
- `4b96b02eddbaf5b729d82c34ed9852943f5daa1b` / `11d57d8c972f2da8c5f3244dbb97507e9790a4a4` — deterministic semantic contract test package/gate;
- `ebe74e3c158b3fdb9bc30ae74b30e899ecf0408f` / `437086b67e2ab00e0ad2ea6b998738ca401da042` — dedicated semantic capability demo;
- `ce9d3a1e9e039ce2d766e2c9e4ad5b38bd756aae` through `dbc0f824c6a81256b02090f0f9934ec800aacb2d` — split implementation into coherent scalar/collection/gradient slices and package them;
- `3ef0f9d138ec17df959fa18ba945250f7c46af7c` — align normalization tests with rejection/normalization contracts;
- `7f2eaa90798774cb27d3e6428c5e3ee32d24a211` — document semantic capability set;
- `9cd6d01cefdd89b5405738f21662b9380e740bac` — preserve Duration unit and Geometry link presentation state without manufacturing model commits.

Touched semantic dependency slice from `a990601...` through `9cd6d01...`:

- `Utilities/PropertyEditor/PropertyEditor.upp`
- `Utilities/PropertyEditor/PropertyEditorBase.cpp`
- `Utilities/PropertyEditor/PropertyValueEditors.h`
- `Utilities/PropertyEditor/PropertySemanticEditorsInternal.h`
- `Utilities/PropertyEditor/PropertySemanticEditors.cpp`
- `Utilities/PropertyEditor/PropertySemanticScalarEditors.cpp`
- `Utilities/PropertyEditor/PropertySemanticCollectionEditors.cpp`
- `Utilities/PropertyEditor/PropertySemanticGradientEditor.cpp`
- `Utilities/PropertyEditor/README.md`
- `Utilities/PropertyEditorSemanticDemo/PropertyEditorSemanticDemo.upp`
- `Utilities/PropertyEditorSemanticDemo/main.cpp`
- `Utilities/PropertyEditorSemanticRunTests/PropertyEditorSemanticRunTests.upp`
- `Utilities/PropertyEditorSemanticRunTests/main.cpp`

No `PropertyEditorCore` source was moved under Ui dependencies. The semantic layer remains in the visual `Utilities/PropertyEditor` package and uses `PropertyEditorKind::Custom` plus stable adapter ids so the headless schema remains independent of Ui controls.

### Standard semantic capability set

The complete standard registration point is now:

```cpp
RegisterPropertyEditorEditors(factory);
```

`PropertyEditor` itself uses the complete registration set by default. `RegisterPropertyEditorV1Editors()` remains available for compatibility where a caller deliberately wants only the earlier rich adapter set.

New first-class semantic adapters/helpers cover:

- Date, Time and DateTime using production `UiDateTime`;
- Duration stored canonically in seconds with ms/s/min/h presentation;
- Point, Size and Rect compound geometry;
- Insets/Padding/Margins-style four-sided geometry;
- four-corner radii;
- linked/unlinked four-sided editing as editor presentation state;
- Flags / multi-choice;
- bounded small ordered string collections with Add/Remove/Up/Down;
- normalized Linear/Radial gradient recipes with arbitrary ordered stops, alpha, angle and interpolation;
- canonical key chords such as `Ctrl+Shift+S`;
- application-provided Resource/Reference browsing through picker providers;
- explicit nullable Optional values for text/int/double, distinct from inherited/theme state.

Existing Range, Adjustable Range, Matrix, Icon, Font, Image, point Curve and cubic Bezier Curve adapters remain part of the standard visual capability set.

### Semantic contracts locked

- model/application state remains owned by the host; PropertyEditor emits preview/commit/reset/undo requests and does not own application history;
- Duration durable unit is seconds; changing only the display unit must not create a commit/undo record;
- Geometry link/unlink is editing-mode state. Enabling Link commits only if propagating one side actually changes the durable four-component value;
- semantic geometry uses numeric `ValueArray` shapes, keeping the headless model free of Ui geometry-control dependencies;
- small ordered String List is property-sized data only and does not replace model-authoritative Data pages for List/Tree/Table/Gallery domains;
- Resource/Reference values remain application-defined; the application owns picker meaning and returned durable Value;
- Optional Null is a real durable value and is independent of inherited/authored override state;
- Gradient recipes are normalized to at least two stops, clamped/sorted stop positions, clamped alpha and canonical mode/interpolation values.

---

## 2. DEMO MODERNIZATION CHECKPOINT RETAINED

The earlier demo tranche remains source-reviewed and still awaits the same Windows acceptance run.

Important source checkpoints include:

- `52ea845aa23da14cdbee577898cfa1d65e7aca1a` — standalone-family/code-generation plan update;
- `fc7d59bac6a3efaa620db352e3d7e2252f9d0f82` — obsolete Dropdown normalized demo removed;
- `80c74d9c14a6aa7dfa0e087c768796210ba4ac35` / `0bb3fbe1c79ede92e0645a4d37d389004a13c6d5` — RangeSlider merged into Slider family and duplicate standalone package retired;
- `de5848451fc2387c5dd68de7019092a293de8113` — production Code view added to combined Edit family;
- `1adfcf17fbd46ef58b43c08241537fe221a15275` + `175899286305b2f98117cfb76c03f2c6bd1d5f72` — Tab rebuilt around PropertyEditor and code-editor setup corrected;
- `2f350e54aade41a904032039722566d966669031` + `f019cc24a69771cc9f7b133a75908070a367fad6` — Slider/RangeSlider combined family + callback hardening;
- `b5be9c34d2468f19f6dc31acc16bd66b762181da` + `7e4e198ae2ae0ddae5cb3ebebf2db7d7707118b3` — Dropdown rebuilt with model-authoritative Data page and Code modes;
- `38e526eb070a459a6e1ec093ca1bd9b57db984e7` + `8c9267bceaea92f914ab657d04a389a3c8aeb2be` — Toggle rebuilt around PropertyEditor;
- `c152d47e190ea096bb372157f2f758d66c1533ae` + `e383a468a6a91e983b160559f3e1f2c2b52ba062` — RadioButton rebuilt and API/code-generation corrections applied;
- `d6b432c9c6d8bde3ef581514db82b349e0de241b` — Code view added to CheckBox;
- `9eed37895c6967b830d210e207e590b79e2752a9` — Button explicit icon-size/render-mode coverage;
- `9f08dd9d2ff7f3806eb45a84c7fc66521d8d03f3` — combined selectable Edit family foundation.

Shared PropertyEditor behavior retained below both tranches:

- `00502ca0b779187314d72d51fc11fbb16f5ebad2` — Cardinal4 editors preserve canonical caller values;
- `9d89914ef3068bd88b53697a3552051d174efe0d` — focused numeric mouse wheel edits the numeric value rather than scrolling the outer PropertyEditor.

### Demo rules still locked

- demos remain understandable from their own package; do not create a shared demo application framework solely to remove repeated shell setup;
- production PropertyEditor is the normal property UI;
- 38% label/value divider remains the demo convention;
- model-backed domains use their real model/Data-page authority rather than mirrored demo arrays;
- generated code is first-class paste-oriented documentation;
- modernized demos prefer `Usage`, `Current changes`, `Full explicit` code modes where appropriate;
- family demos may combine controls that genuinely share style/behavior authority, while each selectable sample must preserve its own state.

Current demo tranche:

- `UiLabelDemo`
- `UiButtonDemo`
- `UiCheckBoxDemo`
- `UiRadioButtonDemo`
- `UiToggleDemo`
- `UiDropdownDemo`
- `UiSliderDemo` — Slider + RangeSlider family
- `UiEditDemo` — Line/Password/Mask/Multi family
- `UiTabDemo`

---

## 3. COMBINED WINDOWS ACCEPTANCE GATE

Validate **current final `upp_Ui/main`**, not an earlier semantic or demo ancestor. Fetch first and report the exact SHA. If main has advanced, confirm the semantic checkpoint and demo checkpoint remain ancestors before testing.

### Automated — Debug + Release

1. `Utilities/PropertyEditorSemanticRunTests`
   - require `PROPERTYEDITOR_SEMANTIC_SUMMARY ... failed=0`.
2. `Utilities/PropertyEditorV1RunTests`
   - require `Fails: 0`.
3. `Utilities/PropertyEditorTests`
   - run if present/buildable in the current assembly; require zero failures and report its actual summary.
4. `Utilities/UiButtonInteractionContractTest`
   - require `Fails: 0`.
5. Run the existing RangeSlider-focused deterministic test package if present in the local assembly; require zero failures. If no separate package exists, report that rather than inventing one.

### Build — Debug + Release

6. `Utilities/PropertyEditorSemanticDemo`
7. `UiLabelDemo`
8. `UiButtonDemo`
9. `UiCheckBoxDemo`
10. `UiRadioButtonDemo`
11. `UiToggleDemo`
12. `UiDropdownDemo`
13. `UiSliderDemo`
14. `UiEditDemo`
15. `UiTabDemo`

### Semantic PropertyEditor visual / interaction smoke

Launch Debug `PropertyEditorSemanticDemo` and check:

- Date, Time and DateTime use the real `UiDateTime` interaction and commit valid values;
- Duration edits correctly in ms/s/min/h while the displayed status/model value remains canonical seconds; changing only unit does not create a false Commit event;
- Point/Size/Rect expose named compound numeric fields;
- Insets and Corner radii link/unlink correctly; linking unequal values propagates and commits, while toggling presentation state without changing values does not manufacture a commit;
- Flags open a true independent multi-choice editor and commit the complete selection;
- Ordered values support Add/Remove/Edit/Up/Down and preserve order;
- Gradient editor supports Linear/Radial, interpolation, angle, arbitrary stop navigation/add/remove, stop position, colour and alpha;
- Key chord canonicalizes common modifier/key spelling;
- Resource/Reference invokes the registered provider and stores the returned Value;
- Optional text/int/double can be explicitly Set/unset and Null remains distinct from reset/default;
- existing point Curve and Bezier examples still open/edit normally;
- Light/Dark switch leaves the PropertyEditor and child editors coherent.

### Existing demo visual / interaction smoke

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

Leave Debug `PropertyEditorSemanticDemo`, `UiTabDemo` and `UiEditDemo` running for Curt to inspect/close after the smoke.

If a compile/runtime failure is substantive, stop and report exact HEAD, package/configuration, complete diagnostic and shortest reproduction. Mechanical U++ build/API corrections are acceptable; do not redesign PropertyEditor or demo architecture during validation.

---

## 4. OTHER PENDING LINES — DO NOT CONFLATE

Separate older validation/work lines remain for:

- UiDesigner Theme Studio / PropertyEditor colour-transfer integration;
- shared model / List-Gallery-Dropdown-Tree scale audit;
- Graph final revalidation;
- SymbolPicker/Gallery convergence.

Do not move those subsystems under this combined gate unless a concrete shared regression requires it.

---

## 5. NEXT AFTER ACCEPTANCE

After the combined gate passes:

1. mark the semantic PropertyEditor and current demo tranche Windows-accepted with the exact tested SHA and results;
2. retain the semantic demo as the capability matrix/reference for future PropertyEditor additions;
3. continue demo modernization with a small group such as SplitButton / ProgressBar / ScrollBar / MatrixSelector;
4. then continue the remaining model-backed List/Tree/Table/Gallery/Menu demos using the accepted Dropdown Data-page pattern deliberately.
