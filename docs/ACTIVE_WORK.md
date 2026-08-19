# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Refresh current `main` before work and preserve unrelated concurrent advances.

Detailed older history:
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`
- `docs/13_UI_MODEL_MUTATION_SCALE_CONTRACT.md` — current shared mutation/identity/scale contract.

## CURRENT SUPERVISORY STATE — 2026-08-19

STATUS: **SHARED MODEL AUDIT SOURCE COMPLETE — WINDOWS HEALTH SMOKE PENDING. GRAPH CORRECTIVE SOURCE COMPLETE — WINDOWS REVALIDATION PENDING. UIBUTTON MODERNIZED DEMO SOURCE PUBLISHED — WINDOWS PENDING. SYMBOL PICKER 5K GALLERY SOURCE PUBLISHED — WINDOWS PENDING.**

An inert accidental branch `temp-demo-button-do-not-use` exists only as a cleanup item. Do not work from it; `main` is authoritative.

---

## SHARED-MODEL-AUDIT-R1 — SOURCE COMPLETE

### Objective

Use Graph and the real 5,000+ item Symbol Picker Gallery as stress fixtures for the reusable model/view layer. Fix defects in `upp_Ui` when the defect is generic; do not hide shared-model problems inside application code and do not add APIs only for symmetry.

### Final contract

Ownership remains:

```cpp
control.Model();
control.SetModel(external);
control.UseInternalModel();
control.IsUsingInternalModel();
control.ClearModel();
```

External binding is non-owning. Ordinary model controls use lifetime-safe weak observer identity through `UiModelObserverSet`, including same-address model reuse.

Mutable access has an explicit publication path:

```cpp
UiListModel::Touch(first, count);
UiTreeModel::Touch(node);
UiTableModel::TouchCell(row, col);
UiTableModel::TouchHeader(axis, index);
UiMenuModel::Touch(node);
UiGraphModel::TouchNode(id);
UiGraphModel::TouchEdge(id);
```

Sequential controls share one structural identity vocabulary:

```cpp
UiIsSequentialStructuralChange(change);
UiRemapSequentialIndex(index, change);
UiRemapSequentialSelection(selection, change);
```

This applies to **List / Gallery / Dropdown**. INSERT/ERASE preserve the same semantic row, MOVE follows the moved row, and `SwapItems()` is a true swap (`UI_MODEL_MOVE`, `c=0`). RESET/CLEAR invalidate sequential indexes.

**Tree / Menu / Graph** retain stable domain IDs and deliberately do not use the sequential remapper.

**Table** deliberately retains coordinate/range identity. Its rows currently do not have stable row IDs, so structure changes clamp active cell/selection rather than pretending List semantics exist.

### Scale / update rules

- `UiListModel::Touch(first,count)` publishes one ranged UPDATE after bounded in-place presentation preparation.
- Dropdown multi-check clear/set uses one ranged Touch rather than N row notifications.
- ordinary Tree item UPDATE does not rebuild the flattened visible projection unless lazy/disclosure/placeholder structure can actually change.
- `UiTreeModel::ImportList()` emits one bulk INSERT/revision.
- List/Gallery item UPDATE invalidates only affected prepared renderer data; normal presentation changes do not rebuild uniform geometry.
- Gallery remains a regular arithmetic grid. Do **not** add Graph's spatial hash/R-tree/quadtree/BVH.
- List/Gallery large multi-selection restore uses `UiResolveSequentialSelectionTokens()`: all stable `item.data` tokens resolve in one model scan, followed only by numeric-index fallback for unresolved numeric tokens. This removes the previous O(selection × model) restore path.
- stable `item.data` match retains precedence over numeric row-index fallback.

### Published audit line

Earlier core audit:
- `fc3052cb35de8945854e771edfec42115668a8b7` / `ae6a2948fd3951fbe1d2407f96b75258617dd769` — List/Gallery semantic structural state.
- `2bbd8d780ffa10d78c94139bd7eb3dd73e6d9603` through `e7131460f2a53117e44864f3eda5632d77fb2b57` — shared remapping / Touch API work.
- `8f451b28a73db4403a8cb095a6d684ecb2673db6` / `8cff510a9831976d4964e469f2fe95e99146a0cb` — Tree projection-local UPDATE + scale regression.
- `7008d06b1f53ee0aea74f96a33d4a76bbc00c232` / `38217648b91a01861cc48dc8e4b5acbf3b1b4cf0` — ranged List Touch + one-event Tree import.
- `7abbf976e8294c89fc0c33b65aa887a2045347d7` — Tree/Table/Menu Touch implementations.
- `99bc61354e0d18408bb9ba9621540aaa34c0e3b7` / `01c8cd7298dcf5e368c2fa5c8467ea128a6d1fb4` — mutation/swap/import regression line.

Final convergence:
- `69f7848a468a1abb4986b97659b4ffc303d62b3a` — Dropdown shared remapper + ranged bulk updates.
- `5259a66a277a82f5ddba85acfbb0d90622b57880` — Dropdown regression coverage.
- `36d41d3f510cbf05c3bb0a2d76565cf2b7515d0c` — single-pass sequential stable-token resolver.
- `27b0fca40c6873d05a651f7b36e57244d8a1a9ff` — UiList uses single-pass multi-selection restore.
- `1fc5e5f4f6ff9bbe9e18c91397dafc0c14782579` — UiGallery uses the same restore path.
- `c024dd97ea01d4ab5dabff7acff70b6d70e3c253` — 10,000-row deterministic linear-selection regression.
- `67476bed7fe6add82aeda4e9c85a39c3c747f195` — programmer documentation records the stable-selection rule.

### Test gates

- `Utilities/UiModelMutationContractTest` — zero failures. Includes explicit Touch contracts, List/Gallery/Dropdown semantic mutation behavior, Tree bulk import, and 10k single-pass selection-token restoration.
- `Utilities/UiModelViewPerformanceTest` — zero failures; 100,000-record List/Gallery structural scale gate.
- `Utilities/UiTreeScaleTest` — zero failures; projection-neutral vs projection-changing update behavior.

No Windows PASS is claimed for the newest audit line yet. Gary has the interim Windows health-smoke task.

### Explicit non-changes

- no `UiListModel::SetAll()` merely to reduce `Clear()+AddRange()` from two already-bulk events to one; add only if measurement makes those two events material.
- no stable-row identity retrofit for Table without a real model requirement.
- no visible-range callbacks added to every view just for symmetry; Gallery has a demonstrated lazy-asset use case.
- no generic model batching abstraction added where domain-specific request/ID semantics already fit.

---

## UI-NODEGRAPH-FINAL-AUDIT-R1 — WINDOWS REVALIDATION PENDING

Corrected Graph production line:
- `97d87013bd5894e8d6927f4caadfd37b31bb10a0`.

Gary's earlier run at pre-correction `b923699776f563abdbb905ed2a7c1b898be1f7fd` passed `UiGraphTest` 90/90 Debug+Release but stopped at `UiNodeGraphScaleTest` 51 checks / 1 failure. Root cause was redundant `RefreshLayout()` after an otherwise-local node-style preview rebuild. The correction removes those two calls; no Graph architecture changed.

Required restart on the final common `main`:
1. `UiNodeGraphScaleTest` Debug — **51/0** required.
2. then Release 51/0.
3. `UiGraphTest` D+R — 90/90 expected.
4. model-binding/data-model regressions.
5. only then complete Graph GUI/manual acceptance including bright ~2px Accent selection chrome, marquee, 10k navigation, batch mutation, PropertyEditor, middle pan, theme and Reference↔10k retention.

Keep one Graph semantic store, one adjacency authority, one retained spatial broad phase, viewport-bounded prepared geometry, local hit queries and nested view-side batch coalescing.

---

## SYMBOL PICKER / GALLERY CONVERGENCE — WINDOWS PENDING

Repository: `Trilec/upp_uisymbolpicker`.

Published documented baseline:
- `5ebd0182033fdc9195df8b91f577eeb9631da8e8`.
- final code directly beneath docs: `36a5c77c9cc843edb393f54921ae8a88f167a0a8`.

The old per-icon Ctrl/Flow architecture and hard 240-item cap are removed. Production library and collection surfaces are model-driven Galleries:

```text
SymbolPickerCatalog / SymbolPickerModel
    -> SymbolPickerLibraryProjection / SymbolPickerCollectionProjection
    -> UiListModel
    -> UiGallery
    -> viewport/overscan UiItemRender pool
```

Current source intent:
- ~5,057 active-style symbols in All, not 240.
- no Ctrl per logical symbol.
- arithmetic Gallery geometry.
- stable catalog IDs through projection / `item.data`.
- indexed catalog lookup.
- style-aware category counts.
- lazy SVG/image decode from useful visible range.
- simple hashed rendered-image cache with hard 8,192 ceiling.
- project/command semantics remain in SymbolPicker domain model, not UiListModel.

The shared audit now removes the previous O(selection × model) Gallery selection-restoration risk. SymbolPicker projection refresh can restore a large stable selection through Gallery's single-pass token resolver.

SymbolPicker's current visible-image loops still use per-row `Set()` while preparing the bounded visible range. `UiListModel::Touch(first,count)` now provides a cleaner one-notification adoption path, but do not change SymbolPicker underneath Gary's active interim smoke. Revisit only after concrete smoke evidence or as the next bounded app cleanup.

Do **not** add a spatial tree to Gallery.

---

## UI-DEMO-MODERNIZATION-PILOT — UIBUTTON WINDOWS PENDING

Published implementation:
- `8c5b8168206f195711a4b624b9c5793c7d6fb02d` — modernized `examples/UiButtonDemo` using the canonical UiLabel-style shell, production PropertyEditor Inspector/Overrides, live preview and generated-code page.

Current gate: Debug build/launch and smoke Preview / Inspector / Overrides / Code / direct checkable behavior / Light-Dark. Do not roll the complete demo family forward from an unvalidated pilot.

---

## NEXT

1. **Do not move the shared `upp_Ui` model source while Gary runs the interim smoke.** Record his exact tested SHA.
2. If `UiModelMutationContractTest`, `UiModelViewPerformanceTest`, `UiTreeScaleTest`, Graph 51/0, `UiGraphTest`, UiButtonDemo and SymbolPicker build/launch are green, record the common Windows-smoke baseline.
3. If Gary tested before the final single-pass selection commits (`36d41d3f...` through `67476bed...`), rerun at least `UiModelMutationContractTest` plus a Ui compile on the final descendant before acceptance.
4. After smoke, optionally simplify SymbolPicker visible-image preparation to mutable image updates + one ranged `UiListModel::Touch()` per useful range; review it as an app adoption of the shared API, not another model redesign.
5. Complete Graph manual acceptance after its corrected automated gates pass.
6. Delete the inert `temp-demo-button-do-not-use` branch when convenient.
