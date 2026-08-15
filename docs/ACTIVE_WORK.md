# ACTIVE WORK

TASK: `UI-MODEL-API-CONVERGENCE + UIDOC-FIRST-CLASS-MODEL + THEME-AUDIT`.

Remote GitHub is authoritative. Never force-update `main`.

BASE: `a7db124e03e474491fee0691614e52be6e92bffa` — last previously Windows-accepted model/theme baseline recorded for this convergence sequence.

STATUS: **IMPLEMENTATION + SOURCE CLEANUP COMPLETE — WINDOWS VALIDATION PENDING.**

## PUBLISHED CHECKPOINTS

Important checkpoints in the current sequence:

- `c35054d256cf1b63113604726943f148fd65bb87` — canonical `Model()` accessor spelling across the existing model-backed controls and callers.
- `f6ab4896d3a99f6dab6e9df0418ecf50c6e7ba8e` — UiDoc adopts first-class internal/external model binding.
- `b7b4338530280216cc223ea0b82624b23e17f38c` through `d142a2bbae0a5a14c3bade19cf4edbfa2f07fbf2` — UiDoc binding tests, caller migration, legacy `Core()` removal, programmer docs and focused package dependencies.
- `b031c7f751f3aad866bdb12690c94dde8756bcd0` through `7d37ee7bf5b1b228ba873f5df0670fad2d802faf` — weak lifetime identity, same-address model reuse regression, and lifetime contract documentation.
- `33d1c3508f48facbe7a0f2e45ced2c00d6672aec` — UiDoc reconciles transient active annotation/embed/table state after authoritative model changes.
- `6944b15875cdd494afc40f4c2dc8e03a823e0e02` — focused regression for direct external-model removal of an active image.
- `853195ec9db4a41e2230f8dabdad5f55453c192b` / `c3d0dc22b70083475d7f08800ab3aa4392ecada7` — canonical model audit and UiDoc binding documentation brought to the final source contract.

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

`Utilities/UiModelBindingContractTest`
- expected **49 checks / 0 failures**;
- covers List, Gallery, Tree, Table, Dropdown, Menu and NodeGraph ownership/switching semantics.

`Utilities/UiDocModelBindingTest`
- expected **22 checks / 0 failures**;
- covers default internal ownership, external identity/no-copy binding, direct external mutation, idempotent same-model binding, two UiDoc views sharing one model, independent position remapping, inactive-model suppression, active-only `ClearModel()`, retained internal restoration, model-owned history policy, shared Undo authority, editing through the active external model, deterministic same-address lifetime reuse, and stale active-image reconciliation after external removal.

`Utilities/UiThemeSurfaceRegressionTest`
- expected **13 checks / 0 failures**;
- covers the focused semantic Light/Dark surface corrections from the same convergence sequence.

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

Previously implemented theme corrections remain unchanged by the UiDoc model-binding work:

- standalone UiList viewport uses semantic Surface when no explicit custom face is supplied;
- UiTable Dark chrome resolves its table/header/row/selection domain colours through semantic roles;
- Gallery, Tree, Dropdown, Menu, NodeGraph, Accordion, MatrixSelector and ColorMatrix retain their accepted semantic-theme paths.

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
- final source changes are bounded to the model-binding contract and its deterministic regression coverage.

The assistant cannot perform the Windows U++ compile/runtime gate in this environment.

## WINDOWS VALIDATION

Validate exact current `main` HEAD with CLANGx64.

Required gate:

1. Build `Ui` Debug to catch header/BLITZ integration issues.
2. Build/run `Utilities/UiDocModelBindingTest` Debug + Release — expected `UIDOC_MODEL_BINDING_SUMMARY checks=22 failed=0`.
3. Build/run `Utilities/UiModelBindingContractTest` Debug + Release — expected 49/0.
4. Build/run `Utilities/UiDocModelTest` Debug + Release — 17 cases, zero case/check failures.
5. Build/run `Utilities/UiDocInteractionTest` Debug + Release — expected 41/0.
6. Build/run `Utilities/UiDocGeometryTest` Debug + Release — expected 16/0.
7. Build/run `Utilities/UiDocImageTest` Debug + Release — expected 75/0.
8. Build/run `Utilities/UiDocMetadataTest` Debug + Release — require its emitted zero-failure summary.
9. Retain the existing convergence gates: `UiThemeSurfaceRegressionTest` 13/0, `UiModelViewPerformanceTest` 52/0, `UiTreeScaleTest` 11/0, `UiGalleryRegressionTest` 11/0, `UiDropdownMenuRenderTest` 11/0 (Debug + Release).
10. Build/launch `examples/UiDocDemo` Debug + Release and smoke ordinary editing, Undo/Redo, search, comments/metadata, table and image insertion/removal.
11. Compile-smoke the migrated model API demos: `UiBreadcrumbsDemo`, `UiListDemo`, `UiTreeDemo`, `UiGraphDemo`, `UiDropdownDemo`.
12. Run `git diff --check`; confirm clean `git status --short` and report exact `git rev-parse HEAD`.

If a substantive ownership, lifecycle, notification, rendering or document-state failure appears, stop and return it to implementation. Do not edit source, weaken tests or restore retired model accessors during validation.
