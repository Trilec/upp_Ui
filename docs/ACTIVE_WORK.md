# ACTIVE WORK

TASK: `UI-MODEL-API-CONVERGENCE + THEME-AUDIT`.

Remote GitHub is authoritative. Never force-update `main`.

BASE: `a7db124e03e474491fee0691614e52be6e92bffa` — R2D final Windows-validated demo cleanup. All R1/R2A/R2B/R2C/R2D and Gallery corrective acceptance is closed at this baseline.

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
- `SetModel(external)` switches authority without copying or clearing either model;
- switching external A -> external B leaves A untouched;
- `UseInternalModel()` restores retained internal data;
- `ClearModel()` clears only the currently active model and does not switch ownership;
- normal model mutation notifications remain the synchronization authority;
- there is no parallel item mirror or model/widget variant.

Implemented for:
- `UiList` / `UiListModel`
- `UiGallery` / `UiListModel`
- `UiTree` / `UiTreeModel`
- `UiTable` / `UiTableModel`
- `UiDropdown` / `UiListModel`
- `UiMenu` / `UiMenuModel`
- `UiNodeGraph` / `UiGraphModel`

All seven retain the existing high-scale architecture: one active non-owning model pointer, one owned internal model, bounded renderer/cached-view state where applicable, and inactive-model callbacks ignored by pointer identity.

`UiTree::SetModel(same_model)` is now idempotent, matching the other model views.

## DELIBERATE NON-MODEL CONTROLS

Audit concluded these should stay model-free:
- `UiAccordion` — real child-control composite/container state.
- `UiMatrixSelector` — small bounded preset/value selector.
- `UiColorMatrix` — one compact 1-8-colour value/editor.

Adding model classes to these would create indirection without sharing/scale value and would move the library toward the duplicate widget/model family we are deliberately avoiding.

`PropertyEditorModel` remains a specialized schema/transaction model; it is not a competing ordinary control family.

## TABLE CLEANUP

The hidden 12 x 6 sample dataset formerly created inside `UiTable` construction has been removed. A new Table now starts with an empty internal model like the other model views. Demos own their demo data.

## THEME AUDIT

Real defects corrected:

1. **UiList standalone viewport**
   - Minimal List rows may remain transparent/lightweight.
   - A theme-driven standalone List now fills missing viewport faces from semantic `UiPanelRole::Surface`.
   - explicit custom List transparency remains caller-owned.

2. **UiTable Dark chrome**
   - the central resolver covered only part of Table's explicit domain colours.
   - `UiTable::SyncThemeStyle()` now completes table/header/row-header, alternate, hover, read-only, selection/active and resize colours from semantic Surface/Subtle/List roles.
   - warning/error semantic fills are dark-transformed from established defaults.
   - explicit custom Table styles remain untouched.

Audited and already coherent:
- Gallery semantic Surface corrective — previously Windows accepted.
- Tree theme-derived surface/selection/glyph colours.
- Dropdown popup/collapsed theme — previously Windows accepted.
- Menu bar/popup theme — previously Windows accepted.
- NodeGraph semantic canvas/node/edge theme.
- Accordion, MatrixSelector and ColorMatrix semantic theme roles with revision tracking.

Styling rule: prefer semantic theme roles; do not scatter unrelated hard-coded Dark colours. Avoid configuration calls that accidentally enter `StyleEdit()` merely to repeat defaults, because that freezes a theme snapshot as a custom style.

## NEW DETERMINISTIC TESTS

`Utilities/UiModelBindingContractTest`
- expected **49 checks**;
- seven checks for each of List, Gallery, Tree, Table, Dropdown, Menu and NodeGraph;
- covers default internal ownership, active `Model()` mutation, external identity, external mutation, external A -> B switching without copy/clear, active-only `ClearModel()`, and retained internal restoration.

`Utilities/UiThemeSurfaceRegressionTest`
- expected **13 checks**;
- standalone List Dark/Light viewport paint;
- complete Table Dark surfaces/domain chrome;
- Tree, Dropdown, Menu and NodeGraph dark surfaces;
- Accordion, MatrixSelector and ColorMatrix semantic dark theme state.

Canonical docs:
- `docs/03_UI_MODEL_GUIDE.md` — programmer-facing `Model()` usage and ownership/switching semantics.
- `docs/09_UI_MODEL_API_AUDIT.md` — full control-by-control architecture/theme audit.

## RECOVERY / DIFF INTEGRITY

A temporary Tree checkpoint was accidentally written from a partial file view during implementation. Full-diff review caught it before validation. Published recovery `15edcad3f4ddf2a5b5e957c784e8e86a9f7c25fe` restored the exact complete pre-task Tree source blob; `e4a3ff441692b16703234465af5f494f2c343123` then added only the intended two-line idempotent SetModel guard from the complete blob.

The current compare against BASE is ahead-only and Tree source differs by only those two added guard lines.

## LEGACY ACCESSOR SPELLING

`Model()` is canonical. A handful of existing repository demos still use old thin aliases such as `GetInternalModel()` / `GetModel()`.

Known mechanical caller migration includes:
- `examples/UiBreadcrumbsDemo/main.cpp`
- `examples/UiDocDemo/UiDocReviewPanel.h`
- `examples/UiListDemo/main.cpp`
- `examples/UiTreeDemo/main.cpp`
- `examples/UiGraphDemo/main.cpp`
- `examples/UiDropdownDemo/main.cpp`
- plus internal Menu/Dropdown popup call sites where search finds the old active-model spelling.

The old accessors currently remain as thin aliases only so published `main` is not deliberately broken between checkpoints. They do **not** own/copy/synchronize another model. New docs/new code must use `Model()`.

This remaining migration is mechanical and may be completed during the Windows validation sweep; once repository callers are clean, remove the transitional aliases from the seven public headers.

## WINDOWS VALIDATION

Required after caller migration:
- Ui Debug CLANGx64 source compile.
- `UiModelBindingContractTest`: Debug/Release **49/0**.
- `UiThemeSurfaceRegressionTest`: Debug/Release **13/0**.
- existing `UiModelViewPerformanceTest`: Debug/Release **52/0**.
- existing `UiTreeScaleTest`: Debug/Release **11/0**.
- existing `UiGalleryRegressionTest`: Debug/Release **11/0**.
- existing `UiDropdownMenuRenderTest`: Debug/Release **11/0**.
- focused Light/Dark smoke for standalone List, Table and NodeGraph plus compile smoke for migrated demos.
- `git diff --check` and clean final status.

Gary may repair/commit tiny mechanical compile or accessor-spelling migrations and continue. Substantive model ownership, rendering, lifecycle, performance or theme defects return to implementation.

STATUS: **CORE IMPLEMENTATION COMPLETE — LEGACY SPELLING MIGRATION + PLATFORM VALIDATION PENDING.**
