# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Refresh current `main` before work/publication and preserve unrelated concurrent advances.

Detailed architecture/history:
- `docs/04_UI_DEMO_GUIDE.md`
- `docs/05_UI_PROPERTY_EDITOR_GUIDE.md`
- `docs/11_UI_PROPERTY_OVERRIDE_LAYOUT.md`
- `docs/12_UI_DEMO_MODERNIZATION_PLAN.md`
- `docs/13_UI_MODEL_MUTATION_SCALE_CONTRACT.md`
- `docs/14_UIGRAPH_RENDER_LOD.md`
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`

## CURRENT SUPERVISORY STATE — 2026-08-26

STATUS: **UIBUTTON ICON-ONLY SIZING IMPLEMENTATION COMPLETE — WINDOWS VALIDATION PENDING. UIGRAPH ROUTE EDITING + VISUAL LOD R3 IMPLEMENTATION COMPLETE — WINDOWS PLATFORM/PERFORMANCE/VISUAL VALIDATION PENDING. OTHER RECORDED UI ACCEPTANCE LINES REMAIN PENDING.**

Authoritative branch: `main`.

Do not claim the R3 Graph tranche or the UiButton icon-only sizing tranche Windows-accepted until the then-current `main` is built and exercised on Windows/U++.

### UIBUTTON ICON-ONLY SIZING CHECKPOINT — 2026-08-26

BASE: `3ab8f173957b2b74aadede0c073b3ac5ace42f5a` — current `main` after the concurrent Graph-only advance. The Graph change touched only `Ui/UiGraph/UiNodeGraph.cpp/.h` and was preserved unchanged.

TASK: `UIBUTTON-ICON-ONLY-R1` — make icon-only `UiButton` natural/layout sizing honor an explicit icon size without carrying full text-button margins or silently collapsing the icon while padding can still yield.

TOUCHED:

- `Ui/UiButton.h`
- `Ui/UiButton.cpp`
- `Ui/UiToolButton.cpp`
- `Utilities/UiButtonInteractionContractTest/main.cpp`
- `docs/ACTIVE_WORK.md`

STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.**

PUBLISHED SOURCE CHECKPOINT: `d52c96b5464e01ec584ee80c4579803efcb966d8` — reviewed source/test commit immediately before this bookkeeping commit. Validation must use this commit or a later non-conflicting descendant of `main`.

VALIDATION / REVIEW:

- Root cause was the `UiButton` content-layout/min-size contract, not icon rasterization: icon-only buttons reused the normal text-button `content_margin` (`14/8/14/8` by default), and the inherited 70x24 Button minimum then kept the control wide even when only an icon was present.
- Icon-only Button geometry now caps authored content margins at `DPI(6)` per side for the compact presentation path. With the default one-pixel frame, `SetIconSize(18, 18)` therefore naturally resolves to approximately 32x32 at 100% DPI rather than a wide text-button footprint.
- When an icon-only Button is forcibly allocated below its natural size, icon-only padding reduces first. An explicit/stable icon size is reduced only when the styled face itself is too small to contain it.
- `icon_side` and `content_gap` remain irrelevant when there is no text; text-only and text+icon layout continue to use the existing normal Button margin/gap/side contract.
- The built-in 70x24 UiButton minimum is treated as an implicit ordinary-button baseline and is skipped only for icon-only mode. A caller-authored `SetMinSize(...)` remains authoritative in icon-only mode.
- `UiToolButton` deliberately keeps its own existing 12x12 minimum baseline; the derived constructor marks that baseline explicit so the UiButton icon-only exception cannot shrink ToolButton below its prior contract.
- `UiLabel` was audited against the same scenario and does not need a production fix: its default content margin is already zero and its icon-only natural size already reserves the requested icon size. Regression coverage now locks that behavior without modifying `UiLabel` source.
- `Utilities/UiButtonInteractionContractTest` adds deterministic coverage for ordinary Button baseline preservation, 18x18 icon-only natural sizing, gap/side irrelevance, padding-yields-first behavior, impossible undersizing, explicit minimum authority, ToolButton minimum preservation, and UiLabel icon-only natural sizing. Expected total is 25 checks / 0 failures.
- The complete source/test diff was source-reviewed before publication. Local Windows/U++ compilation and `git diff --check` are unavailable in the supervisor environment and remain validator gates.

NEXT ACTION:

1. Gary fetches current `main`, reports the exact tested HEAD, confirms `d52c96b5464e01ec584ee80c4579803efcb966d8` is an ancestor, runs `git diff --check`, and confirms no later UiButton/UiToolButton/UiLabel conflict was introduced.
2. Build/run `Utilities/UiButtonInteractionContractTest` under CLANGx64 Debug and Release; require `Checks: 25, Fails: 0` in both configurations.
3. Build `UiButtonDemo` and `UiLabelDemo` under CLANGx64 Debug and Release with zero compile/link errors.
4. Manual Debug smoke: a normal `UiButton` with no text and `SetIconSize(18,18)` should present a clearly visible 18x18 icon in a compact ~32x32 default button; changing icon side/gap must not alter icon-only geometry; ordinary text Buttons remain normal; icon-only `UiLabel` remains naturally icon-sized without Button padding.
5. Stop and report any compile/runtime/test failure or any text-button/ToolButton/Label regression. Do not patch Designer/application code around a control defect.

---

### UIGRAPH ROUTE / VISUAL LOD / 10K PROFILING CHECKPOINT — 2026-08-26

BASE: `91f2926f57a86dfc6df08d9b0ae10173085dcbf5` — Gary-validated `main` immediately before this follow-up Graph tranche.

TASK: `UIGRAPH-LOD-ROUTE-R3` — correct capsule selection authority, make font/icon detail scale progressively, define deterministic spatial selection modifiers, add simple request-first connector midpoint editing, and instrument the remaining 10,000-node pan cost before attempting another performance rewrite.

TOUCHED:

- `Ui/UiGraph/UiNodeGraph.h`
- `Ui/UiGraph/UiNodeGraph.cpp`
- `Ui/UiGraph/UiNodeGraphInteraction.cpp`
- `Utilities/UiNodeGraphRouteEditTest/UiNodeGraphRouteEditTest.upp`
- `Utilities/UiNodeGraphRouteEditTest/main.cpp`
- `Utilities/UiNodeGraphSelectionModifierTest/UiNodeGraphSelectionModifierTest.upp`
- `Utilities/UiNodeGraphSelectionModifierTest/main.cpp`
- `Utilities/UiNodeGraphPanProfileTest/UiNodeGraphPanProfileTest.upp`
- `Utilities/UiNodeGraphPanProfileTest/main.cpp`
- `docs/14_UIGRAPH_RENDER_LOD.md`
- `docs/ACTIVE_WORK.md`

STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.**

PUBLISHED SOURCE CHECKPOINT: `00bcaba5d311ece072b2d6b90ba6cad0776f7bca` — reviewed Graph source/tests plus updated LOD/route contract immediately before this bookkeeping commit. The exact final `main` SHA is the commit containing this ACTIVE_WORK update or a later non-conflicting advance and must be reported by the Windows validator.

VALIDATION / REVIEW:

- Gary previously validated base `91f2926...` cleanly under CLANGx64 Debug and Release: `UiNodeGraphDragDamageTest` 8/0, `UiNodeGraphScaleTest` 53/0, `UiNodeGraphRenderLodTest` 16/0, `UiNodeGraphOverviewLodTest` 11/0, `UiNodeGraphInteractionStateTest` 24/0, `UiGraphTest` 90/90, `UiModelBindingContractTest` 55/0, `UiThemeSurfaceRegressionTest` 13/0, and Debug/Release `UiGraphDemo` builds passed;
- complete current `UiNodeGraph` header/renderer/interaction, `UiGraphModel` update API, spatial query path, style/shadow helpers, focused tests and `Ui/Ui.upp` production membership were source-reviewed before handoff;
- upstream U++ `CtrlCore.h` was checked directly: `K_ALT`, `K_SHIFT` and `K_CTRL` are valid mouse/key modifier flags for the implemented selection contract;
- no `UiGraphModel` semantic/runtime authority was added: route editing emits `UiGraphEdgeRouteRequest`; internal `UiGraphModel::UpdateEdge(...)` occurs only when accepted, internal mutation is enabled and the host did not handle the request;
- selected/hot built-in Straight/Bezier/Orthogonal edges expose one ordinary midpoint route handle at/above `route_edit_zoom`; Bezier interprets the midpoint as a smooth curve bias, Straight as a bend waypoint, Orthogonal as a preferred corridor; Custom routes keep extension authority;
- built-in selected nodes now have one body-frame authority instead of a grey pressed frame plus a separate blue approximation; capsule path/hit geometry uses true semicircular ends;
- font geometry and paint share a compressed continuous zoom scale; default title/secondary/icon/port thresholds are approximately `0.34 / 0.42 / 0.70 / 0.32`, with edge/port labels around `0.45` and route editing around `0.55`;
- Graph marquee frame/fill uses the Graph blue contract; spatial selection semantics are no modifier = replace, Shift = add, Ctrl = toggle, Alt = subtract;
- spatial hash queries were re-read during final cleanup and already perform exact retained-world-bounds intersection after cell candidate lookup, so no redundant marquee false-positive filter was added;
- non-rectangular shadow falloff uses the authored shadow curve with bounded LOD layer counts and a lower-right default offset;
- `UiNodeGraph` now exposes geometry-preparation, edge-paint, node-paint and total-paint microseconds; `UiNodeGraphPanProfileTest` builds 10,000 nodes / 19,800 connectors and reports separate mid (`0.50`) and overview (`0.20`) pan profiles;
- `UiNodeGraphRouteEditTest` contains 18 deterministic checks; final cleanup explicitly includes `<cmath>`, uses `std::sqrt`, and removes an unused local to avoid avoidable compiler portability/warning noise;
- `UiNodeGraphSelectionModifierTest` contains 10 deterministic checks, including Ctrl marquee toggling only the two nodes inside the marquee while a third node sharing nearby spatial cells remains selected;
- `UiNodeGraphPanProfileTest` contains 9 deterministic checks plus diagnostic profile output; timing values themselves are evidence, not fixed machine thresholds;
- `docs/14_UIGRAPH_RENDER_LOD.md` now describes the implemented route-edit contract instead of the stale deferred-feature note;
- no additional speculative 10k panning rewrite was added during cleanup: current user evidence says selection/multi-drag improved materially while whole-view pan remains the unresolved path, so the new phase timings must identify whether geometry preparation, edge paint or node paint dominates before another optimization;
- local Windows/U++ compilation and `git diff --check` are not available in the supervisor container; Gary must perform those gates on current `main`.

NEXT ACTION:

1. Gary grabs latest `main`, reports the actual tested HEAD, runs `git diff --check`, then builds/runs the three R3 focused tests plus the existing Graph/model regression set in CLANGx64 Debug and Release;
2. Gary copies the `UINODEGRAPH_PAN_PROFILE` and existing low-zoom profile lines and identifies the dominant 10k pan phase from measured Windows evidence;
3. Curt performs the `UiGraphDemo` visual/interaction check for capsule frame agreement, progressive font/icon LOD, blue marquee modifiers, midpoint route editing and dense panning;
4. only after that evidence decide whether another 10k optimization tranche is needed and which phase it should target.

---

## 0. ITEM-DECORATION LAYOUT REPAIR

PlaylistLab exposed a shared presentation defect: control-owned decorations could paint over renderer-owned text instead of reserving geometry. A focused cross-control audit was therefore completed before further PlaylistLab work.

Source base:

- `ceb70739a97f5de25f23505114be24ff8607c359` — pre-repair `main`.

Source checkpoint before this bookkeeping commit:

- `537ad1d7e102d43f5ed8e7f80492d3075aa6583f` — `Use shared reservation for Table sort chrome`.

### Contract

- Every control-owned decoration must reserve its actual extent plus its authored gap before renderer text/content is laid out.
- Reservations compose additively on left/right/top/bottom; one decoration must not overwrite another decoration's reservation.
- The owning control remains the single paint/interaction authority for control-owned chrome; renderers receive only the remaining content geometry.
- If an owning control paints a specialised presentation such as a List badge, the corresponding ordinary renderer presentation is suppressed to avoid double ownership.
- Decoration-side changes and show/hide changes that alter geometry must invalidate prepared renderer layout.

### Implemented

- `UiItemRender`
  - added a shared per-instance content-reservation contract used by owning controls without changing durable model data or renderer style configuration;
  - clone/configuration remains independent while owner-applied reservations are transient prepared-layout state.

- `UiList`
  - left/right drag handles reserve `drag_size + drag_gap` before row renderer content;
  - right-text badge mode reserves the measured badge lane and suppresses ordinary renderer right-text so the badge has one presentation owner;
  - combined left drag + check + icon + metadata geometry now composes in sequence instead of later helpers overwriting the drag reservation;
  - drag show/side changes reset prepared renderer geometry;
  - row painting is clipped against the rounded inner List viewport so square row selection/hot fills cannot spill into rounded outer corners; frame/focus chrome is repainted as the final outer authority.

- `UiTable`
  - sortable column-header sort glyphs reserve their right-side lane through the same shared renderer reservation contract;
  - custom header renderers retain their full header surface while only content moves away from sort chrome;
  - sort glyph placement honours authored header padding and existing header updates already invalidate/reprepare the affected header renderer.

### Audited without source change

- `UiDropdown`, `UiTree`, `UiMenu`, and `UiAccordion` already reduce/reserve their own indicator/disclosure/check/submenu/header chrome before content presentation and did not justify parallel rewrites.

### Deterministic coverage

- `Utilities/UiListStyleContractTest` now checks additive left/right reservation semantics and single-owner badge data projection.
- Existing `UiModelViewPerformanceTest` remains the scale/renderer-pool regression gate for List/Table prepared-layout behaviour.

### Windows acceptance gate for this repair

Gary should fetch current `main`, report exact HEAD, then:

1. Build/run `Utilities/UiListStyleContractTest` under CLANGx64 Debug and Release; require `UILIST_STYLE_CONTRACT_SUMMARY ... failed=0`.
2. Build/run `Utilities/UiModelViewPerformanceTest` under CLANGx64 Debug and Release; require its actual summary to report zero failures.
3. Build `Utilities/UiTableRunTests` and `examples/UiListDemo` under CLANGx64 Debug and Release with zero compile/link errors.
4. In a normal interactive Windows desktop, visually check `UiListDemo` with a rounded List: selected/hot rows must stay inside the rounded inner viewport; drag handle plus icon/check/metadata/text must not overlap; left and right drag sides must shift content by the authored gap; right-text badge mode must render one badge, not badge plus duplicate right text.
5. Visually exercise a sortable `UiTable` header and confirm header text/custom renderer content does not overlap the sort glyph.
6. Do not redesign controls during validation. Tiny obvious U++ compile/API corrections are acceptable; otherwise stop and report exact diagnostic and diff.

---

## 1. CONTROL HYGIENE CHECKPOINT

A focused hygiene pass was performed across the current Ui control surface, with extra attention on recently touched controls and PropertyEditor integration.

Hygiene base:

- `cb5ff715e770169f0b630112a490639c06d54d2d` — `Document semantic PropertyEditor expansion`.

Hygiene source checkpoint through:

- `3aecaccc7ea9b6f6ccc388dc287f2689ab812a8f` — `Clean UiMultiEdit public documentation`.

The hygiene source line is eleven commits ahead of the recorded hygiene base, zero behind at the source-review checkpoint. Final bookkeeping commits may advance `main`; validation must always use the then-current exact HEAD.

### Source fixes and tests

- `UiSlider`
  - corrected the vertical geometry contract so `track_size.cx` consistently represents preferred major-axis length and `track_size.cy` track thickness, matching `UiRangeSlider`;
  - added keyboard focus participation to both constructors;
  - disabled sliders now reject keyboard and mouse-wheel value editing;
  - tick configuration now invalidates natural layout as well as paint;
  - added deterministic `Utilities/UiSliderRunTests` coverage for value normalization, data binding, disabled input, event counts, horizontal/vertical geometry, expanding tracks and tick sizing.

- `UiRangeSlider`
  - added keyboard focus participation;
  - bound-handle requests normalize to the corresponding lower/upper selection handle while adjustable bounds are disabled;
  - disabling adjustable bounds cannot leave a stale inactive bound handle as the keyboard/wheel target;
  - extended `Utilities/UiRangeSliderRunTests` for disabled input and active-handle normalization.

- edit family
  - removed the stale undefined one-argument `UiBaseEdit::GetPos(int line)` declaration while retaining the implemented `GetPos(int line, int col = 0)` authority;
  - cleaned stale/corrupted `UiPasswordEdit` API documentation and implementation comments without changing password semantics;
  - cleaned `UiMultiEdit` documentation so examples and recommendations use real current APIs.

- `UiCheckBox`
  - removed damaged/stale public-header changelog text.

### Reviewed without further source change

The pass also re-read the relevant implementation/package slices for:

- `UiLabel`;
- `UiButton`, `UiToolButton`, `UiSplitButton`;
- `UiRadioButton`, `UiToggle`;
- `UiDropdown`;
- `UiLineEdit`, `UiIntEdit`, `UiFloatEdit`, `UiMaskEdit`;
- `UiTab`;
- `UiMatrixSelector`;
- `UiDateTime`;
- `UiProgressBar`;
- `UiScrollBar`;
- production `Utilities/PropertyEditor`, including layout, inline editors, interaction, paint, rich/semantic value editors and package membership.

No additional architecture change was justified in those reviewed slices. In particular, PropertyEditor retains the existing model/host authority, focused numeric wheel routing, rich editor metadata, semantic adapter registration, viewport-bounded inline editors, and separate reset/override/action-editor responsibilities. The hygiene pass did not add a parallel PropertyEditor state or editor framework.

---

## 2. SEMANTIC PROPERTYEDITOR CHECKPOINT

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

## 3. DEMO MODERNIZATION CHECKPOINT RETAINED

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
- `9d89914ef3068bd88b53697a3552051d174efe0d` — focused numeric mouse wheel edits the numeric value rather than the outer rail.

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

## 4. COMBINED WINDOWS ACCEPTANCE GATE

Validate **current final `upp_Ui/main`**, not an earlier hygiene, semantic or demo ancestor. Fetch first and report the exact SHA. If main has advanced, confirm the recorded checkpoints remain ancestors before testing.

### Automated — Debug + Release

1. `Utilities/UiSliderRunTests`
   - require `UISLIDER_SUMMARY ... failed=0`.
2. `Utilities/UiRangeSliderRunTests`
   - require `Fails: 0`.
3. `Utilities/PropertyEditorSemanticRunTests`
   - require `PROPERTYEDITOR_SEMANTIC_SUMMARY ... failed=0`.
4. `Utilities/PropertyEditorV1RunTests`
   - require `Fails: 0`.
5. `Utilities/PropertyEditorTests`
   - run if present/buildable in the current assembly; require zero failures and report its actual summary.
6. `Utilities/UiButtonInteractionContractTest`
   - require `Fails: 0`.

### Build — Debug + Release

7. `Utilities/PropertyEditorSemanticDemo`
8. `UiLabelDemo`
9. `UiButtonDemo`
10. `UiCheckBoxDemo`
11. `UiRadioButtonDemo`
12. `UiToggleDemo`
13. `UiDropdownDemo`
14. `UiSliderDemo`
15. `UiEditDemo`
16. `UiTabDemo`

### Hygiene-focused visual / interaction smoke

- Slider: horizontal and vertical tracks use the expected major axis; keyboard focus works; disabled keyboard/wheel input is inert; expanding track consumes allocated major-axis space.
- RangeSlider: keyboard focus works; lower/upper selection remains editable after adjustable bounds are disabled; no stale hidden bound handle owns wheel/keyboard input.
- Edit family: Line/Password/Mask/Multi still edit normally; Password visibility eye remains functional without stealing edit focus; multiline scrolling/layout remains coherent.
- PropertyEditor: focused numeric wheel changes the numeric value rather than the outer rail; numeric slider-toggle, Cardinal4, icon, colour, range and semantic editors remain coherent; reset/override state remains separate from editor action controls.

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

Leave Debug `PropertyEditorSemanticDemo`, `UiSliderDemo` and `UiEditDemo` running for Curt to inspect/close after the smoke.

If a compile/runtime failure is substantive, stop and report exact HEAD, package/configuration, complete diagnostic and shortest reproduction. Mechanical U++ build/API corrections are acceptable; do not redesign PropertyEditor or demo architecture during validation.

---

## 5. OTHER PENDING LINES — DO NOT CONFLATE

Separate older validation/work lines remain for:

- UiDesigner Theme Studio / PropertyEditor colour-transfer integration;
- shared model / List-Gallery-Dropdown-Tree scale audit;
- Graph final revalidation;
- SymbolPicker/Gallery convergence.

Do not move those subsystems under this combined gate unless a concrete shared regression requires it.

---

## 6. NEXT AFTER ACCEPTANCE

After the combined gate passes:

1. mark the control hygiene, semantic PropertyEditor and current demo tranche Windows-accepted with the exact tested SHA and results;
2. retain the semantic demo as the capability matrix/reference for future PropertyEditor additions;
3. continue demo modernization with a small group such as SplitButton / ProgressBar / ScrollBar / MatrixSelector;
4. then continue the remaining model-backed List/Tree/Table/Gallery/Menu demos using the accepted Dropdown Data-page pattern deliberately.
