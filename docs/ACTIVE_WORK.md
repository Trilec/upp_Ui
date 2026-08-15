# ACTIVE WORK

TASK: `UI-MODEL-API-CONVERGENCE + UIDOC-FIRST-CLASS-MODEL + THEME-AUDIT`.

Remote GitHub is authoritative. Never force-update `main`.

BASE: `a7db124e03e474491fee0691614e52be6e92bffa` — last previously Windows-accepted model/theme baseline recorded for this convergence sequence.

STATUS: **IMPLEMENTATION + SOURCE CLEANUP COMPLETE — WINDOWS VALIDATION PENDING.**

## PUBLISHED CHECKPOINTS

Current sequence includes:

- `c35054d256cf1b63113604726943f148fd65bb87` — canonical `Model()` accessor spelling across the existing model-backed controls and callers.
- `f6ab4896d3a99f6dab6e9df0418ecf50c6e7ba8e` — UiDoc adopts first-class internal/external model binding.
- `b7b4338530280216cc223ea0b82624b23e17f38c` through `d142a2bbae0a5a14c3bade19cf4edbfa2f07fbf2` — UiDoc binding tests, caller migration, legacy `Core()` removal, programmer docs and focused package dependencies.
- `c10e27d2dba7ddfe6a1f40dd8f4122dd470d8b3f` — lifetime hardening test added after static review.
- `7d37ee7bf5b1b228ba873f5df0670fad2d802faf` — UiDoc lifetime-binding contract documented.

The final status-only `ACTIVE_WORK.md` commit may advance `main` beyond the code/document checkpoints above.

## CANONICAL MODEL CONTRACT

Every genuine model-backed control now exposes:

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

Key accepted design points:

- `Model()` is the only public model accessor; the legacy `Core()` alias is removed.
- `SetModel()` is idempotent when passed the already-active model.
- multiple UiDoc views can bind the same external `UiDocCore` and keep independent caret/selection/viewport state;
- edits or Undo/Redo performed on the shared model update every actively bound view;
- callbacks from previously bound but inactive live models are ignored;
- history depth belongs to `UiDocCore`, not visual `UiDoc::Style`;
- `UiDocCore` is `Pte<UiDocCore>` so UiDoc can retain weak `Ptr<UiDocCore>` binding identities;
- when an inactive external model is destroyed, its remembered binding identity expires; a later model created at the same address receives a fresh observer binding;
- an external model remains non-owning and must outlive the period during which it is actively used by UiDoc.

Canonical detail: `docs/10_UIDOC_MODEL_BINDING.md`.

## LEGACY ACCESSOR MIGRATION

Repository UiDoc callers and tests have been migrated from `Core()` to `Model()` and the `Core()` alias is gone.

The wider List/Gallery/Tree/Table/Dropdown/Menu/NodeGraph transitional `GetInternalModel()` / `GetModel()` aliases were also removed after repository caller migration. New code must use `Model()`.

## DETERMINISTIC TESTS

`Utilities/UiModelBindingContractTest`
- expected **49 checks / 0 failures**;
- covers List, Gallery, Tree, Table, Dropdown, Menu and NodeGraph ownership/switching semantics.

`Utilities/UiDocModelBindingTest`
- expected **20 checks / 0 failures**;
- covers default internal ownership, external identity/no-copy binding, direct external mutation, idempotent same-model binding, two UiDoc views sharing one model, independent position remapping, inactive-model suppression, active-only `ClearModel()`, retained internal restoration, model-owned history policy, shared Undo authority, editing through the active external model, and deterministic same-address lifetime reuse.

`Utilities/UiThemeSurfaceRegressionTest`
- expected **13 checks / 0 failures**;
- covers the focused semantic Light/Dark surface corrections from the same convergence sequence.

Existing scale/regression gates retained from the model/theme audit:
- `UiModelViewPerformanceTest` — **52/0**;
- `UiTreeScaleTest` — **11/0**;
- `UiGalleryRegressionTest` — **11/0**;
- `UiDropdownMenuRenderTest` — **11/0**.

Existing UiDoc model/interaction/geometry/image/metadata suites remain authoritative and must not be weakened if a platform failure exposes a regression.

## THEME AUDIT STATE

Previously implemented theme corrections remain unchanged by the UiDoc model-binding work:

- standalone UiList viewport uses semantic Surface when no explicit custom face is supplied;
- UiTable Dark chrome resolves its table/header/row/selection domain colours through semantic roles;
- Gallery, Tree, Dropdown, Menu, NodeGraph, Accordion, MatrixSelector and ColorMatrix retain their accepted semantic-theme paths.

Styling rule remains: prefer semantic theme roles; do not scatter unrelated hard-coded Dark colours or enter custom-style mode merely to repeat defaults.

## STATIC REVIEW / DIFF INTEGRITY

Current UiDoc binding review confirmed:

- production UiDoc implementation uses `Model()` as the active semantic authority;
- history policy no longer lives in `UiDoc::Style`;
- no repository `doc.Core()` callers remain;
- UiDoc test package declares the focused Core/Draw/CtrlCore/CtrlLib/Ui dependencies;
- no package membership change is required for production UiDoc sources;
- the lifetime hardening uses native U++ `Pte`/`Ptr` weak lifetime tracking rather than ownership or a second model store.

The assistant cannot perform the Windows U++ compile/runtime gate in this environment.

## WINDOWS VALIDATION

Validate exact current `main` HEAD with CLANGx64 Debug and Release where requested.

Minimum focused gate:

1. Build/run `Utilities/UiDocModelBindingTest` Debug + Release — expected `UIDOC_MODEL_BINDING_SUMMARY checks=20 failed=0`.
2. Build/run `Utilities/UiModelBindingContractTest` Debug + Release — expected 49/0.
3. Build `Ui` Debug CLANGx64 to catch header/BLITZ integration issues.
4. Build/run the existing UiDoc model, interaction and geometry tests in Debug; any existing expected summary must remain green.
5. Build and launch `examples/UiDocDemo` Debug; smoke ordinary editing, Undo/Redo, metadata/search, table and image insertion.
6. Run `git diff --check` and confirm a clean worktree at the exact tested HEAD.

If a substantive ownership, lifecycle, notification, rendering or document-state failure appears, stop and return it to implementation. Do not weaken tests or restore retired model accessors.
