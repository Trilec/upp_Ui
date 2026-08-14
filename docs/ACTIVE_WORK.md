# ACTIVE WORK

TASK: `UI-MODEL-API-CONVERGENCE + THEME-AUDIT`.

Remote GitHub is authoritative. Never force-update `main`.

BASE: `a7db124e03e474491fee0691614e52be6e92bffa` — R2D final Windows-validated demo cleanup. All R1/R2A/R2B/R2C/R2D and Gallery corrective acceptance is closed at this baseline.

## OBJECTIVE

Simplify the programmer-facing model contract without introducing Qt-style duplicate widget/model families.

Every genuine model-backed control must expose the same ownership vocabulary:

```cpp
ModelType& Model();
const ModelType& Model() const;
Control& SetModel(ModelType& model);
Control& UseInternalModel();
bool IsUsingInternalModel() const;
Control& ClearModel();
```

Semantics:
- every model-backed control owns an internal model from construction;
- `Model()` always means the model currently driving the control;
- without `SetModel(...)`, `Model()` is the internal model;
- after `SetModel(external)`, `Model()` is exactly that external object;
- switching models never copies or clears either model;
- inactive internal data is retained and can be restored with `UseInternalModel()`;
- `ClearModel()` clears the currently active model only and never switches ownership;
- model mutation notifications remain the sole synchronization authority; no mirror/refresh API returns.

## AUDIT — TRUE MODEL VIEWS

These already use one internal model plus one active non-owning model pointer and should converge on the contract above:
- `UiList` -> `UiListModel`
- `UiGallery` -> `UiListModel`
- `UiTree` -> `UiTreeModel`
- `UiTable` -> `UiTableModel`
- `UiDropdown` -> `UiListModel`
- `UiMenu` -> `UiMenuModel`
- `UiNodeGraph` -> `UiGraphModel`

All seven model types already provide `Clear()`. Existing `bound_models_` callback registration is retained: callbacks from inactive models are ignored unless the observed model is the current `model_`. Switching is O(1) ownership selection plus each view's legitimate selection/projection/geometry reset; it introduces no record copy or renderer-per-item allocation.

## AUDIT — DELIBERATE NON-MODEL CONTROLS

Do not manufacture model classes for these:
- `UiAccordion`: composite container; section bodies own real child Ctrls and section/open state is container structure.
- `UiMatrixSelector`: tiny preset/value selector with a bounded fixed cell set.
- `UiColorMatrix`: one compact 1-8-colour value/editor.

These are not duplicate "widget versions" of model controls. Adding model objects would increase API complexity without providing shared data/view scale value.

## CLEANUP QUESTIONS / FINDINGS

- Public `GetInternalModel()` / `GetModel()` spelling is implementation-oriented. Migrate repository callers to `Model()` and remove those accessors rather than carrying compatibility clutter.
- `UiTable` currently seeds its internal model with a hidden 12 x 6 sample grid in the control constructor. Audit callers/tests before removing this; under the new contract a default model view should normally start with an empty model and demos should own demo data.
- Dropdown convenience `Add/Remove/SetItem...` methods operate directly on the active model and do not create duplicate state. They may remain as ergonomic control helpers unless audit finds a second authority.

## THEME AUDIT

The Gallery corrective established an important rule: row/item presentation may be transparent, but a standalone model view must still have a coherent theme-aware viewport/surface.

Audit Light/Dark propagation for:
- List standalone viewport versus transparent Minimal rows;
- Tree surface/rows;
- Table explicit table/header/selection colours;
- Dropdown collapsed/popup surfaces;
- Menu bar/popup/domain chrome;
- NodeGraph canvas/node/edge styles;
- Accordion/MatrixSelector/ColorMatrix theme-live styles despite their non-model status.

Avoid hard-coded "dark mode fixes" where a semantic theme role can provide the surface. Also audit style-mutating setup calls that accidentally freeze a theme snapshot into custom style.

## VALIDATION PLAN

Add focused deterministic model-binding tests covering all seven true model views:
- default internal ownership;
- `Model()` identity and mutation;
- external `SetModel` identity;
- mutation through `Model()` updates the external object;
- `ClearModel()` clears only the active model;
- `UseInternalModel()` restores retained internal data;
- switching never mirrors/copies data.

Retain existing high-scale acceptance (`UiModelViewPerformanceTest` 52/0, `UiTreeScaleTest` 11/0) and add theme regressions only where the audit identifies real standalone surface defects.

STATUS: **AUDIT IN PROGRESS — IMPLEMENTATION NOT YET COMPLETE.**
