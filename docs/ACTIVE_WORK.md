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

STATUS: **REFERENCE IMPLEMENTATION PUBLISHED — WINDOWS VALIDATION PENDING.**

PUBLISHED:

- `5a51d6d682710db8c5d20822f552749f7743c439` — canonical PropertyEditor override layout and paint-layer reference.
- `564e42c9c4906f59c3d91da0d64bfffab021eee1` — UiLabel demo includes the canonical override layout normalizer.
- `0b63f9cb5483e68a5fc0518cce0a54119ce25757` — PropertyEditor honors stable `sort_order`; UiLabel uses canonical display ordering; focused sort-order test published.
- cross-repository Designer reference head at this checkpoint: `Trilec/upp_uidesigner` `155e51eb696537f8ac6a8a3af1629d2278513f66`.

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
- Designer Skin image editing remains explicitly deferred until the theme-adapter preview contract can resolve `UiDesignerDocument::resources`. Do not fake a Skin image row before that resource-aware contract exists.

NEXT:

1. Windows-validate `Utilities/PropertyEditorSortOrderTest` and build/launch `examples/UiLabelDemo`; visually confirm headings are contiguous and Skin is nested under Face.
2. In `Trilec/upp_uidesigner`, build/run `tests/LabelThemeAdapterTest`, build the Designer, and smoke a UiLabel selection in the Inspector including a Face QuadGradient override.
3. After the Label reference is accepted, normalize UiList and UiBaseEdit, then the composite UiDropdown and UiAccordion demos/adapters.
4. Migrate remaining control demos/adapters without inventing Designer-only terminology or parallel style state.

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
- UiLabel override ids and authored values remain unchanged while the PropertyEditor presentation is normalized to the documented reference layout.

The assistant cannot perform the Windows U++ compile/runtime gate in this environment.

## WINDOWS VALIDATION

Validate exact current `main` HEAD with CLANGx64.

Required gate:

1. Build/run `Utilities/PropertyEditorSortOrderTest` Debug + Release — expected `PROPERTY_EDITOR_SORT_ORDER_SUMMARY checks=5 failed=0`.
2. Build `Ui` Debug to catch header/BLITZ integration issues.
3. Build/launch `examples/UiLabelDemo` Debug + Release. In Theme Overrides confirm one contiguous `General`, `Face`, `Frame`, `Ink`, `Icon`, `Typography`, `Content Margin`, `Focus`, `Shadow`, `Highlight` sequence; `Skin` must appear nested under `Face`, with `Slice` and `Content Inset` nested beneath Skin. Exercise Normal/Hot/Pressed/Disabled Face FillRecipe, Frame/Ink/Icon states, and a Skin image/9-slice.
4. Build/run `Utilities/UiThemeSurfaceRegressionTest` Debug + Release — expected 13/0.
5. Build/run `Utilities/UiModelBindingContractTest` Debug + Release — expected 49/0.
6. Build/run `Utilities/UiDocModelBindingTest` Debug + Release — expected `UIDOC_MODEL_BINDING_SUMMARY checks=22 failed=0`.
7. Build/run `Utilities/UiDocModelTest` Debug + Release — 17 cases, zero case/check failures.
8. Build/run `Utilities/UiDocInteractionTest` Debug + Release — expected 41/0.
9. Build/run `Utilities/UiDocGeometryTest` Debug + Release — expected 16/0.
10. Build/run `Utilities/UiDocImageTest` Debug + Release — expected 75/0.
11. Build/run `Utilities/UiDocMetadataTest` Debug + Release — require its emitted zero-failure summary.
12. Retain the existing convergence gates: `UiModelViewPerformanceTest` 52/0, `UiTreeScaleTest` 11/0, `UiGalleryRegressionTest` 11/0, `UiDropdownMenuRenderTest` 11/0 (Debug + Release).
13. Build/launch `examples/UiDocDemo` Debug + Release and smoke ordinary editing, Undo/Redo, search, comments/metadata, table and image insertion/removal.
14. Focused UiTable Dark smoke: visible cell/header text, explicit selected range and active-cell border, resize guide, alternate/read-only/hover state, then Light restoration.
15. Compile-smoke the migrated model API demos: `UiBreadcrumbsDemo`, `UiListDemo`, `UiTreeDemo`, `UiGraphDemo`, `UiDropdownDemo`.
16. Run `git diff --check`; confirm clean `git status --short` and report exact `git rev-parse HEAD`.

Cross-repository Designer validation for this reference checkpoint:

- repository: `Trilec/upp_uidesigner`;
- expected published reference head before any later doc-only advance: `155e51eb696537f8ac6a8a3af1629d2278513f66`;
- build/run `tests/LabelThemeAdapterTest` Debug + Release and require its emitted zero-failure summary;
- build the Designer Debug + Release and smoke a UiLabel selection in Inspector Theme Overrides;
- verify General -> Face -> Frame -> Ink -> Icon -> Typography -> Content Margin -> Focus -> Shadow -> Highlight ordering;
- verify Face state rows use FillRecipe and a QuadGradient previews/round-trips without losing its four colours/tile/blur;
- Skin is intentionally not exposed in Designer yet because theme-adapter preview has no document resource resolver. Treat any fake raw-path Skin implementation as a regression, not a fix.

If a substantive ownership, lifecycle, notification, rendering or document-state failure appears, stop and return it to implementation. Do not edit source, weaken tests or restore retired model accessors during validation.