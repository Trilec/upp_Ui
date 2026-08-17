# ACTIVE WORK

TASK: `UI-MODEL-API-CONVERGENCE + UIDOC-FIRST-CLASS-MODEL + THEME-AUDIT`.

Remote GitHub is authoritative. Never force-update `main`.

BASE: `a7db124e03e474491fee0691614e52be6e92bffa` — last previously Windows-accepted model/theme baseline recorded for this convergence sequence.

STATUS: **IMPLEMENTATION + SOURCE CLEANUP COMPLETE — WINDOWS VALIDATION PENDING.**

## FOLLOW-ON DESIGN CHECKPOINT — PROPERTYEDITOR OVERRIDE LAYOUT

BASE: `55e02a537b25bd73ff32088a0f543be0946726e0` — exact `upp_Ui/main` inspected before documenting the normalized PropertyEditor override layout.

TASK: `UI-PROPERTY-OVERRIDE-LAYOUT`.

TOUCHED:

- `docs/11_UI_PROPERTY_OVERRIDE_LAYOUT.md`
- `Utilities/PropertyEditor/PropertyEditorLayout.cpp`
- `Utilities/PropertyEditorSortOrderTest/PropertyEditorSortOrderTest.upp`
- `Utilities/PropertyEditorSortOrderTest/main.cpp`
- `examples/UiLabelDemo/UiLabelDemo.h`
- `examples/UiLabelDemo/UiLabelDemo.upp`
- `examples/UiLabelDemo/UiLabelOverrideLayout.cpp`
- `docs/ACTIVE_WORK.md`
- cross-repository consumer: `Trilec/upp_uidesigner` dedicated Label theme adapter + focused test/coverage note.

STATUS: **LABEL REFERENCE ACCEPTED — INTERACTIVE DESIGNER QUADGRADIENT RESELECTION SMOKE DEFERRED.**

PUBLISHED:

- `5a51d6d682710db8c5d20822f552749f7743c439` — canonical PropertyEditor override layout and paint-layer reference.
- `564e42c9c4906f59c3d91da0d64bfffab021eee1` — UiLabel demo includes the canonical override layout normalizer.
- `0b63f9cb5483e68a5fc0518cce0a54119ce25757` — PropertyEditor honors stable `sort_order`; UiLabel uses canonical display ordering; focused sort-order test published.
- `8a016656651fb929c08ac1c2f801a6b8c2f2ab77` — UiLabelDemo selected mode button styling published after Windows validation.
- cross-repository Designer accepted head: `Trilec/upp_uidesigner` `59cb685219dc94bcad8d8cba82d1acc7528c6836`.

SOURCE REVIEW:

- shared `UiDraw.h` confirms the existing three-stage paint contract: Background -> control-owned Content -> Foreground;
- default Background owns outer shadow, Face or active Skin, Frame, inset shadow and Highlight;
- active Skin draws image/9-slice and suppresses ordinary Face fill, so Inspector/demo organisation treats `Skin` as a nested Face path while preserving the API terms `Skin`, `Slice` and `Content Inset`;
- default Foreground currently owns the focus ring only; `WhenPaintForeground` remains the live extension point for custom paint-only overlays such as glass sheen/decorative/animated passes;
- no shared authored `StyledOverlay` exists, so do not expose an empty generic Foreground override group until a real shared API exists;
- inspected representative style contracts: UiLabel, UiList, UiDropdown, UiAccordion and UiBaseEdit;
- composite controls should preserve real nested domains such as Dropdown `Popup/*` and Accordion `Header/*` / `Body/*` rather than flattening them;
- group headings use API vocabulary, state rows do not repeat the heading prefix, `General` stays small, and demos/UiDesigner must converge on the same ids, labels, paths and ordering;
- `PropertyEditorItem::sort_order` is now the stable presentation-order authority. `PropertyEditorModel::Add()` already defaults it to insertion order, so existing models retain their prior order unless they intentionally assign another value;
- UiLabel no longer depends on interleaved construction order for display. Its normalizer assigns the documented General -> Face -> Face/Skin -> Frame -> Ink -> Icon -> Typography -> Content Margin -> Focus -> Shadow -> Highlight order without copying or replacing model items;
- UiDesigner is a consumer of this convention. Its dedicated `label` adapter mirrors the non-resource Label groups and uses `PropertyEditorKind::FillRecipe` for Face states, preserving authored Solid/QuadGradient recipes through preview resolution and generated C++;
- Designer Skin image editing remains explicitly deferred until the theme-adapter preview contract can resolve `UiDesignerDocument::resources`. Do not fake a Skin image row before that resource-aware contract exists;
- Designer commit `59cb685...` was source-reviewed as a mechanical migration only: four Preview accesses plus matching Tree/List test assertions changed from retired `GetInternalModel()` to canonical `Model()`, with no ownership or expectation changes.

VALIDATION:

- `Utilities/PropertyEditorSortOrderTest` Debug + Release: `PROPERTY_EDITOR_SORT_ORDER_SUMMARY checks=5 failed=0`.
- `examples/UiLabelDemo` Debug + Release: PASS, zero errors/warnings; canonical grouped order confirmed and mode-button selection styling smoke passed.
- `tests/LabelThemeAdapterTest` in `Trilec/upp_uidesigner` Debug + Release: `checks=242 failed=0`.
- UiDesigner Debug + Release: PASS, zero errors/warnings; Release launched and remained responsive.
- Gary's Designer repository grep after `59cb685...`: no `GetInternalModel()` matches.
- `git diff --check`: PASS in both validation repos.
- Designer Skin remains absent as intended.
- The specific add/select UiLabel + QuadGradient refresh/reselection interaction remains manually unverified because custom U++ controls expose no Windows UI Automation descendants. This is a deferred interactive smoke, not a source/build blocker.

NEXT:

1. Normalize UiList and UiBaseEdit using the accepted Label reference convention.
2. Then normalize the composite UiDropdown and UiAccordion demos/adapters while preserving Popup/Header/Body domains.
3. When convenient interactive Windows access is available, perform the deferred Designer UiLabel QuadGradient reselection smoke; do not block the next implementation slice on UI Automation limitations.
4. Continue remaining control demos/adapters without inventing Designer-only terminology or parallel style state.

The reference checkpoint changes PropertyEditor presentation ordering only; it does not change control runtime style ownership or introduce new visual state. `sort_order` defaults preserve existing insertion order for models that do not opt into explicit ordering.

## PUBLISHED CHECKPOINTS

Important checkpoints in the current sequence:

- `c35054d256cf1b63113604726943f148fd65bb87` — canonical `Model()` accessor spelling across the existing model-backed controls and callers. Gary validated the mechanical migration itself with Ui Debug 0 warnings/errors and `UiModelBindingContractTest` 49/0 in Debug + Release before stopping at a pre-existing Table theme failure.
- `f6ab4896d3a99f6dab6e9df0418ecf50c6e7ba8e` — UiDoc adopts first-class internal/external model binding.
- `b7b4338530280216cc223ea0b82624b23e17f38c` through `d142a2bbae0a5a14c3bade19cf4edbfa2f07fbf2` — UiDoc binding tests, caller migration, legacy `Core()` removal, programmer docs and focused package dependencies.
- `b031c7f751f3aad866bdb12690c94dde8756bcd0` through `7d37ee7bf5b1b228ba873f5df0670fad2d802faf` — weak lifetime identity, same-address model reuse regression, and lifetime contract documentation.
- `33d1c3508f48facbe7a0f2e45ced2c00d6672aec` — UiDoc reconciles transient active annotation/embed/table state after authoritative model changes.
- `6944b15875cdd494afc40f4c2dc8e03a823e0e02` — focused regression for direct external-model removal of an active image.
- `853195ec9db4a41e2230f8dabdad5f55453c192b` / `c3d0dc22b70083475d7f08800ab3aa4392ecada7` — canonical model audit and UiDoc binding documentation brought to the final source contract.
- `43a91954ac40e99f82d5e29807eea569d1a5a2b5` — Table Dark-mode corrective after Windows `UiThemeSurfaceRegressionTest` stopped at 12/13: role-tuned Table header/cell ink now comes from semantic Standard role colours and selection/active/resize borders from semantic Accent role colours.

The final status/documentation commit may advance `main` beyond these checkpoints. Always validate the exact remote HEAD supplied in the validation task.

## CANONICAL MODEL CONTRACT

Every genuine model-backed control exposes:

```cpp
ModelType& Model();
const ModelType& Model() const;
Control& SetModel(ModelType& model);
Control& UseInternalModel();
bool IsUsingInternalModel() const;
Control& ClearModel();
```

Semantics:

- the internal model always exists and is active by default;
- `Model()` always returns the currently active model;
- `SetModel(external)` switches authority without copying, merging or clearing either model;
- switching external A -> external B leaves A untouched;
- `UseInternalModel()` restores retained internal data;
- `ClearModel()` clears only the currently active model and does not switch authority;
- model mutation notifications are the synchronization authority;
- controls do not maintain parallel semantic mirrors.

Implemented for:

- `UiList` / `UiListModel`
- `UiGallery` / `UiListModel`
- `UiTree` / `UiTreeModel`
- `UiTable` / `UiTableModel`
- `UiDropdown` / `UiListModel`
- `UiMenu` / `UiMenuModel`
- `UiNodeGraph` / `UiGraphModel`
- `UiDoc` / `UiDocCore`

`UiDocCore` deliberately remains a document-specific model. It is not flattened into `UiModelItem` and does not inherit `UiDataModelBase` merely for naming symmetry. Its transaction/result/position-map contract remains authoritative for document edits.

## UIDOC MODEL BINDING

`UiDoc` now has one active model pointer plus one retained internal `UiDocCore`.

Accepted design points:

- `Model()` is the only public model accessor; the legacy `Core()` alias is removed.
- `SetModel()` is idempotent when passed the already-active model.
- multiple UiDoc views can bind the same external `UiDocCore` and keep independent caret/selection/viewport state;
- edits or Undo/Redo performed on the shared model update every actively bound view;
- callbacks from previously bound but inactive live models are ignored;
- history depth belongs to `UiDocCore`, not visual `UiDoc::Style`;
- `UiDocCore` is `Pte<UiDocCore>` so UiDoc can retain weak `Ptr<UiDocCore>` binding identities;
- when an inactive external model is destroyed, its remembered binding identity expires; a later model created at the same address receives a fresh observer binding;
- after an authoritative model notification, active annotation/embed/table state is retained only if that object still exists and the active table coordinate is still valid;
- an external model remains non-owning and must outlive the period during which it is actively used by UiDoc.

Canonical detail: `docs/10_UIDOC_MODEL_BINDING.md`.

## LEGACY ACCESSOR MIGRATION

Repository UiDoc callers and tests have been migrated from `Core()` to `Model()` and the `Core()` alias is gone.

The wider List/Gallery/Tree/Table/Dropdown/Menu/NodeGraph transitional `GetInternalModel()` / `GetModel()` aliases were removed after repository caller migration. New ordinary model-backed code must use `Model()`.

`PropertyEditorModel` remains a specialized schema/transaction API by design and is not part of that mechanical alias rule.

## DETERMINISTIC TESTS

`Utilities/PropertyEditorSortOrderTest`
- expected **5 checks / 0 failures**;
- verifies explicit `sort_order`, keyboard traversal in presentation order, stable equal-order ties, and unchanged insertion ordering for ordinary models.

`Utilities/UiModelBindingContractTest`
- expected **49 checks / 0 failures**;
- covers List, Gallery, Tree, Table, Dropdown, Menu and NodeGraph ownership/switching semantics.
- Gary already observed 49/0 in Debug and Release on the canonical accessor-cleanup commit; rerun on the final combined HEAD because later UiDoc/theme commits legitimately advanced main.

`Utilities/UiDocModelBindingTest`
- expected **22 checks / 0 failures**;
- covers default internal ownership, external identity/no-copy binding, direct external mutation, idempotent same-model binding, two UiDoc views sharing one model, independent position remapping, inactive-model suppression, active-only `ClearModel()`, retained internal restoration, model-owned history policy, shared Undo authority, editing through the active external model, deterministic same-address lifetime reuse, and stale active-image reconciliation after external removal.

`Utilities/UiThemeSurfaceRegressionTest`
- expected **13 checks / 0 failures**;
- covers the focused semantic Light/Dark surface corrections from the same convergence sequence.
- Windows at the pre-corrective accessor-cleanup state produced 12/13 in both Debug and Release; the only failure was `Table Dark mode keeps readable ink and explicit selection/active borders`.
- `43a91954...` resolves that invariant from semantic Standard/Accent role colours without changing the 13-check contract.

Existing scale/regression gates retained from the model/theme audit:
- `UiModelViewPerformanceTest` — **52/0**;
- `UiTreeScaleTest` — **11/0**;
- `UiGalleryRegressionTest` — **11/0**;
- `UiDropdownMenuRenderTest` — **11/0**.

Existing UiDoc suites remain authoritative:
- `UiDocModelTest` — 17 cases, zero case failures / zero check failures required; report the emitted check total;
- `UiDocInteractionTest` — expected **41/0**;
- `UiDocGeometryTest` — expected **16/0**;
- `UiDocImageTest` — expected **75/0**;
- `UiDocMetadataTest` — run and require its emitted zero-failure summary.

Do not weaken an existing suite if a platform failure exposes a regression.

## THEME AUDIT STATE

Implemented theme corrections:

- standalone UiList viewport uses semantic Surface when no explicit custom face is supplied;
- UiTable Dark chrome resolves table/header/row surfaces through semantic roles;
- for Minimal/Pill, Table text uses semantic Standard role ink and Table range/active/resize borders use semantic Accent role colours, because List rows are allowed to be frame-less while Table active-cell chrome is not;
- Gallery, Tree, Dropdown, Menu, NodeGraph, Accordion, MatrixSelector and ColorMatrix retain their accepted semantic-theme paths.

The Table corrective is deliberately local to theme-driven styles. Explicit `SetCustomStyle(...)` remains caller-owned and is not overwritten.

Styling rule remains: prefer semantic theme roles; do not scatter unrelated hard-coded Dark colours or enter custom-style mode merely to repeat defaults.

## STATIC REVIEW / DIFF INTEGRITY

Current review confirmed:

- production UiDoc implementation uses `Model()` as the active semantic authority;
- history policy no longer lives in `UiDoc::Style`;
- no repository `doc.Core()` callers remain;
- UiDoc binding test package declares Core/Draw/CtrlCore/CtrlLib/Ui dependencies;
- no package membership change is required for production UiDoc sources;
- lifetime hardening uses native U++ `Pte`/`Ptr` weak lifetime tracking rather than ownership or a second model store;
- external model object removal now reconciles transient active view state without clearing valid selections on unrelated edits;
- canonical List/Gallery/Tree/Table/Dropdown/Menu/NodeGraph accessor migration is published as `c35054d...` and does not maintain compatibility aliases;
- Table theme corrective `43a91954...` changes only `ResolveUiTableChrome(...)` and leaves model/render/geometry code untouched;
- PropertyEditor presentation ordering now uses stable `sort_order`, with raw model index as the equal-order tie-breaker;
- UiLabel override ids and authored values remain unchanged while the PropertyEditor presentation is normalized to the documented reference layout;
- cross-repository Designer preview/test callers are migrated to `Model()` at `59cb685...`; Gary's post-migration repository grep reported no remaining `GetInternalModel()` calls.

The assistant cannot perform the Windows U++ compile/runtime gate in this environment.

## WINDOWS VALIDATION

Validate exact current `main` HEAD with CLANGx64.

Label reference subset already completed on Windows:

- `PropertyEditorSortOrderTest` Debug + Release: 5/0;
- UiLabelDemo Debug + Release: clean builds and GUI ordering smoke;
- Designer `LabelThemeAdapterTest` Debug + Release at `59cb685...`: 242/0;
- UiDesigner Debug + Release: clean builds; Release launch responsive;
- deferred only: manual custom-control QuadGradient refresh/reselection smoke.

Remaining wider convergence gate:

1. Build `Ui` Debug to catch header/BLITZ integration issues.
2. Build/run `Utilities/UiThemeSurfaceRegressionTest` Debug + Release — expected 13/0.
3. Build/run `Utilities/UiModelBindingContractTest` Debug + Release — expected 49/0.
4. Build/run `Utilities/UiDocModelBindingTest` Debug + Release — expected `UIDOC_MODEL_BINDING_SUMMARY checks=22 failed=0`.
5. Build/run `Utilities/UiDocModelTest` Debug + Release — 17 cases, zero case/check failures.
6. Build/run `Utilities/UiDocInteractionTest` Debug + Release — expected 41/0.
7. Build/run `Utilities/UiDocGeometryTest` Debug + Release — expected 16/0.
8. Build/run `Utilities/UiDocImageTest` Debug + Release — expected 75/0.
9. Build/run `Utilities/UiDocMetadataTest` Debug + Release — require its emitted zero-failure summary.
10. Retain the existing convergence gates: `UiModelViewPerformanceTest` 52/0, `UiTreeScaleTest` 11/0, `UiGalleryRegressionTest` 11/0, `UiDropdownMenuRenderTest` 11/0 (Debug + Release).
11. Build/launch `examples/UiDocDemo` Debug + Release and smoke ordinary editing, Undo/Redo, search, comments/metadata, table and image insertion/removal.
12. Focused UiTable Dark smoke: visible cell/header text, explicit selected range and active-cell border, resize guide, alternate/read-only/hover state, then Light restoration.
13. Compile-smoke the migrated model API demos: `UiBreadcrumbsDemo`, `UiListDemo`, `UiTreeDemo`, `UiGraphDemo`, `UiDropdownDemo`.
14. Run `git diff --check`; confirm clean `git status --short` and report exact `git rev-parse HEAD`.

Cross-repository Designer reference checkpoint:

- repository: `Trilec/upp_uidesigner`;
- accepted published head before any later docs-only advance: `59cb685219dc94bcad8d8cba82d1acc7528c6836`;
- `tests/LabelThemeAdapterTest` Debug + Release passed 242/0;
- Designer Debug + Release builds passed with zero warnings/errors and Release launched responsively;
- General -> Face -> Frame -> Ink -> Icon -> Typography -> Content Margin -> Focus -> Shadow -> Highlight schema ordering is covered by the focused adapter test/source review;
- Face state rows use FillRecipe and authored QuadGradient values are preserved by field resolution/codegen tests;
- the remaining manual add/select/reselect QuadGradient interaction is deferred because Windows UI Automation cannot reach custom U++ descendants;
- Skin is intentionally not exposed in Designer yet because theme-adapter preview has no document resource resolver. Treat any fake raw-path Skin implementation as a regression, not a fix.

If a substantive ownership, lifecycle, notification, rendering or document-state failure appears, stop and return it to implementation. Do not edit source, weaken tests or restore retired model accessors during validation.

## UI-NODEGRAPH-SCALE-R1

BASE: `c07ae7f273ec1712be5a381a05d137a08775decd` — current `main` immediately before Graph promotion; its Label acceptance audit was preserved unchanged.

PUBLISHED: `f5069a2bd95a3002eb020e5739a85dc0b1866764` — reviewed Graph code/test/demo promotion onto current `main`.

STATUS: **IMPLEMENTATION + SOURCE REVIEW COMPLETE — WINDOWS U++ VALIDATION PENDING.**

TOUCHED:
- `Ui/Ui.upp`
- `Ui/UiGraph/UiGraphModel.h/.cpp`
- `Ui/UiGraph/UiNodeGraph.h/.cpp`
- `Ui/UiGraph/UiNodeGraphSpatial.cpp`
- `Ui/UiGraph/UiNodeGraphInteraction.cpp`
- `Utilities/UiNodeGraphScaleTest/`
- `examples/UiGraphDemo/`

SOURCE RESULT:
- retained per-node edge adjacency makes ordinary incident/port/connection work local-degree based;
- retained world-space spatial hashing bounds ordinary viewport preparation, paint and hit testing to relevant candidates;
- pan/zoom and live node-style-class editing reuse the world spatial index rather than rebuilding the 10,000-node scene;
- Paint consumes prepared geometry; explicit whole-graph operations remain intentionally O(N);
- generic authored node size is `64x44`; compact LOD retains shape/title/ports and richer explicit nodes retain secondary content;
- `FitToGraph()` may zoom out but never auto-enlarges above authored 1:1;
- edges/labels paint below node surfaces/content; current vector rendering remains antialiased and backend-neutral for future OpenGL/Vulkan work;
- middle-button pan is capture-free and left-button capture release is explicitly owned/re-entrancy safe;
- no Ctrl-per-node path was introduced; externally owned child controls remain exceptional node content only.

FOCUSED SCALE TEST:
- deterministic 10,000 nodes + 19,800 nearest-neighbour edges;
- deterministic variation across standard shapes, semantic roles and reusable style classes;
- bounded prepared/paint/hit work, deep navigation, local spatial mutation, pan/zoom reuse, live style-preview reuse, model switching and active-only `ClearModel()`;
- small-graph fit asserts exactly 1.0 rather than auto-enlargement;
- exact emitted check count must be reported by Windows validation; zero failures required.

DEMO:
- canonical UiLabel-style PropertyEditor shell with Reference / 10k modes;
- Reference mode uses the retained internal graph model and demonstrates all standard shapes, route families, arrow/stroke variants, waypoints, roles/presets and two externally owned embedded controls;
- 10k mode binds a separate external model with high explicit stable IDs, deterministic visual variety and 19,800 bounded edges;
- Inspector writes selected-node state through `graph.Model().UpdateNode(...)`; PropertyEditor is not a parallel node store;
- Style page authors real UiNodeGraph presentation state; status exposes model/prepared/candidate counts and zoom.

VALIDATION: source/static review complete; Windows U++/CLANGx64 compile/runtime pending. No platform PASS is claimed.

NEXT ACTION: run the focused Graph Windows validation on current `main`; if accepted, delete the obsolete `agent/uigraph-scale-hardening` branch. Small mechanical U++/Windows fixes are allowed; architecture/model/spatial/public-API/test-weakening changes are not.
