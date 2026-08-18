# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Work directly from refreshed `main`; preserve unrelated concurrent advances.

## CURRENT SUPERVISORY STATE — 2026-08-18

STATUS: **UI-NODEGRAPH-FINAL-AUDIT-R1 SOURCE CORRECTIVE PUBLISHED — WINDOWS REVALIDATION PENDING.**

Remote branch state is `main` only.

Detailed prior history is preserved in:
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`

## UI-NODEGRAPH-FINAL-AUDIT-R1

Audit/recovery base:
- `9172d67ebcbbad91b1ced2f1f2d05e07814c6b97`

Final hardening line before the latest corrective:
- `b923699776f563abdbb905ed2a7c1b898be1f7fd` — source hardening plus programmer docs, awaiting Windows validation.

### Gary validation at `b9236997...`

Gary refreshed exact `main`, clean tree and matching remote.

Passed:
- `Utilities/UiGraphTest` Debug: **90/90**.
- `Utilities/UiGraphTest` Release: **90/90**.

Stopped correctly on:
- `Utilities/UiNodeGraphScaleTest` Debug: **checks=51 failed=1**.
- failing assertion: `prepared node style-class preview updates locally without rebuilding the spatial index or full viewport geometry`.

Release scale, model-binding, data-model and GUI/manual acceptance were not run after that stop.

### Root cause and corrective

The local prepared-node/incident-edge rebuild path itself was correct. The regression was caused by `SetNodeStyleClass()` and `RemoveNodeStyleClass()` calling `RefreshLayout()` after their local rebuild. `UiNodeGraph::Layout()` deliberately invalidates and prepares the complete viewport geometry, defeating the local style-preview contract and incrementing `geometry_build_serial_`.

Corrective publication:
- `f2c1c66d9ba2c5ba536ba08e483c8e4e07f1d77d` — removed the two redundant `RefreshLayout()` calls. Parent-diff review caught an accidental reconstruction typo in `SetEdgeStyleClass()` before validation.
- `97d87013bd5894e8d6927f4caadfd37b31bb10a0` — restored the correct `edge_styles_.Add(...)` insertion. This correction changes only that accidental line.

Net production diff from `b9236997...` to `97d87013...` is the two intended node-style `RefreshLayout()` removals plus EOF newline normalization. No Graph/model architecture changed.

### Final Graph/model contract

Keep:
- semantic topology only in `UiGraphModel`, stable `int64` identities;
- derived per-node adjacency for incident-edge work;
- painted ordinary nodes rather than one `Ctrl` per node;
- retained world-space spatial hash as the sole Graph broad phase;
- viewport-bounded prepared geometry;
- public/live node, port and edge hit testing through small spatial queries plus exact tests;
- pruning of empty spatial buckets;
- local node-style-class preview rebuild of prepared users plus prepared incident edges, with no full spatial or viewport geometry rebuild;
- adaptive spatial marquee preview with semantic selection commit on release;
- nested view-side batch coalescing around immediate authoritative model mutation;
- bright semantic Accent committed-selection chrome;
- canonical `Model()/SetModel()/UseInternalModel()/ClearModel()` semantics;
- `UiModelObserverSet` lifetime-safe external-model observer identity across List/Gallery/Tree/Table/Dropdown/Menu/Graph.

Do not add a second spatial index or parallel semantic store.

### Required Windows restart

Gary should refresh current `main` and first rerun:
- `Utilities/UiNodeGraphScaleTest` Debug — required `UINODEGRAPH_SCALE_SUMMARY checks=51 failed=0`.

If green, continue the previously issued final acceptance:
- scale Release 51/0;
- `UiModelBindingContractTest` Debug+Release: **55/0**;
- `UiDataModelsTest` Debug: **7535/0**;
- `UiGraphDemo` Debug+Release;
- blue ~2px shape-following selection chrome;
- local/adaptive marquee;
- 10k navigation;
- batch mutation;
- PropertyEditor preview/Cancel;
- middle-pan regression;
- Light/Dark;
- Reference -> 10k -> Reference and embedded-control retention.

## SYMBOL PICKER / GALLERY CONVERGENCE

`upp_uisymbolpicker` is **not a Graph consumer for its icon library**. The appropriate reusable control is `UiGallery` backed by `UiListModel`.

Current Symbol Picker library still creates one `SymbolPickerIconTile : ParentCtrl` plus two child `Label`s for every displayed icon inside a wrapping `UiBoxLayout`, and hard-caps the unfiltered All view to 240 items. Its own supervisor handoff confirms that removing the cap and eagerly creating all 5,057 current-style icons blocks startup for tens of seconds.

`UiGallery` was specifically redesigned to remove that scaling failure:
- model item count is independent of visual Ctrl/renderer count;
- uniform grid geometry is arithmetic;
- renderers are non-`Ctrl` and recycled for visible+overscan cells only;
- normal paint/hit/scroll work is viewport bounded;
- `WhenVisibleRange` exists for lazy image preparation;
- multi-selection, marquee, selection chrome and zoom are view-owned;
- semantic items remain in `UiListModel`;
- model binding now has lifetime-safe observer identity.

Do **not** add Graph's spatial hash to Gallery. Uniform row/column arithmetic is cheaper and already computes visible and marquee-intersecting cells directly.

Potential Gallery polish that may be useful to Symbol Picker, but must stay surgical:
- align marquee appearance with the Windows-style interaction language (thin Accent frame + light translucent Accent fill);
- consider transient marquee preview with semantic selection committed on release if live selection notifications prove expensive at very large/zoomed-out ranges;
- damage only old/new marquee and changed visible cells rather than repainting unrelated viewport content;
- keep selection frame independent from renderer-selected presentation;
- preserve capture-ownership hardening already present in Gallery.

The primary Symbol Picker migration should not wait for a spatial structure: use `UiGallery` + a filtered `UiListModel` projection, remove the 240 cap after migration, and drive SVG/image decode from useful visible range plus cache.

## NEXT

1. Gary reruns the corrected Graph 51-check Debug gate at current `main`, then completes final acceptance if green.
2. Symbol Picker session refreshes `Trilec/upp_uisymbolpicker` `main`, reads `docs/SUPERVISOR_LIBRARY_DISPLAY_HANDOFF.md`, and migrates the library viewport from eager `SymbolPickerIconTile` controls / `UiBoxLayout` to `UiGallery` + `UiListModel`.
3. Preserve SymbolPicker catalog/project/command semantics; the Gallery model is a derived filtered presentation projection, not a replacement for `SymbolPickerModel` or `SymbolPickerCatalog`.
4. Measure with the real 5,057-icons-per-style All fixture and remove `kLibraryAllInitialLimit` only when the Gallery path is active.
