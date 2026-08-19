# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Work directly from refreshed `main`; preserve unrelated concurrent advances.

## CURRENT SUPERVISORY STATE — 2026-08-19

STATUS: **SHARED MODEL AUDIT SOURCE COMPLETE — WINDOWS HEALTH SMOKE PENDING. UI-DEMO-MODERNIZATION-PILOT / UIBUTTON SOURCE REPLACEMENT PUBLISHED — WINDOWS PENDING. GRAPH WINDOWS REVALIDATION REMAINS PENDING. SYMBOL PICKER 5K GALLERY MIGRATION SOURCE PUBLISHED — WINDOWS PENDING.**

Authoritative work branch is `main`. An accidental inert connector-created branch `temp-demo-button-do-not-use` exists only as a cleanup item; do not use it for work. It should be deleted in GitHub when convenient.

Detailed prior history is preserved in:
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`

## SHARED-MODEL-AUDIT-R1 — SYMBOLPICKER / GRAPH LESSONS

OBJECTIVE:
- use the 5,000+ item Symbol Picker Gallery and the Graph scale work as real stress fixtures for the common model/view layer;
- fix genuine shared model/view defects in `upp_Ui` rather than patching them only in an application;
- avoid adding APIs merely for symmetry when the existing domain model has different identity semantics.

SOURCE RESULT:
- ordinary model-backed controls use lifetime-safe weak observer identity (`UiModelObserverSet`) so a destroyed inactive external model cannot suppress a later model allocated at the same address;
- `UiListModel` exposes mutable `Get()` plus ranged `Touch(first,count)` so a bounded prepared range can be mutated and published with one UPDATE rather than one `Set()` notification per row;
- Tree, Table and Menu now have explicit `Touch`, `TouchCell` / `TouchHeader`, and `Touch` publication APIs matching Graph's existing `TouchNode` / `TouchEdge` principle;
- List, Gallery and Dropdown share `UiIsSequentialStructuralChange`, `UiRemapSequentialIndex` and `UiRemapSequentialSelection` rather than carrying divergent insert/erase/move logic;
- true `UiListModel::SwapItems()` identity is encoded as MOVE with `c=0` and handled by the shared remapper;
- List/Gallery sequential selection/cursor state follows the same semantic row through insert/erase/move/swap and is invalidated on reset/clear as appropriate;
- Dropdown now follows the same shared remapper, including true swaps;
- Dropdown bulk checked-state clear/set mutates rows in place and emits one ranged `Touch()` instead of N row updates;
- ordinary Tree node UPDATE no longer rebuilds the entire flattened visible projection unless the update can actually alter lazy/disclosure projection structure;
- `UiTreeModel::ImportList()` emits one bulk structural insert/revision rather than one event per imported child;
- Table deliberately remains coordinate/range based: row/column structural changes clamp active cell/selection rather than pretending the current Table model has stable row IDs;
- Menu/Tree/Graph retain stable domain IDs and therefore deliberately do not use the sequential list remapper;
- Gallery remains a uniform arithmetic grid; no Graph spatial hash/R-tree/quadtree/BVH is introduced.

PUBLISHED MODEL-AUDIT LINE:
- `fc3052cb35de8945854e771edfec42115668a8b7` / `ae6a2948fd3951fbe1d2407f96b75258617dd769` — List/Gallery semantic view-state mutation handling;
- `2bbd8d780ffa10d78c94139bd7eb3dd73e6d9603` through `e7131460f2a53117e44864f3eda5632d77fb2b57` — shared sequential remapping and explicit model Touch contracts;
- `8f451b28a73db4403a8cb095a6d684ecb2673db6` / `8cff510a9831976d4964e469f2fe95e99146a0cb` — Tree projection-local ordinary updates + scale contract;
- `7008d06b1f53ee0aea74f96a33d4a76bbc00c232` / `38217648b91a01861cc48dc8e4b5acbf3b1b4cf0` — ranged List Touch and one-event Tree bulk import in canonical model source;
- `7abbf976e8294c89fc0c33b65aa887a2045347d7` — explicit Tree/Table/Menu Touch implementations;
- `99bc61354e0d18408bb9ba9621540aaa34c0e3b7` / `01c8cd7298dcf5e368c2fa5c8467ea128a6d1fb4` — mutation/swap/import contract coverage and fixture correction;
- `69f7848a468a1abb4986b97659b4ffc303d62b3a` — Dropdown shared remapper + ranged bulk checked-state updates;
- `5259a66a277a82f5ddba85acfbb0d90622b57880` — Dropdown shared mutation regressions;
- `2d5999689e3355745ea630fb94979cd2f3ff7f87` — programmer contract in `docs/13_UI_MODEL_MUTATION_SCALE_CONTRACT.md`.

TEST CONTRACT:
- `Utilities/UiModelMutationContractTest` is the focused mutation/identity regression package and must report zero failures;
- `Utilities/UiModelViewPerformanceTest` remains the structural 100,000-record List/Gallery scale gate: bounded renderer pools, deep direct navigation, viewport-bounded Paint and arithmetic Gallery geometry;
- `Utilities/UiTreeScaleTest` now distinguishes ordinary projection-neutral UPDATE from disclosure/lazy projection-changing UPDATE.

API-AUDIT DECISIONS:
- no `UiListModel::SetAll()` was added merely to collapse `Clear()+AddRange()` from two already-bulk structural notifications to one; this remains a possible future optimization only if measurement shows those two events are themselves material;
- no List-style row identity was imposed on Table without a stable-row-ID model contract;
- no visible-range callbacks were added to every control simply because Gallery has one; `UiGallery::WhenVisibleRange` exists because thumbnail/lazy-asset preparation is a demonstrated provider use case;
- no spatial structure was added to Gallery while uniform-grid arithmetic is strictly cheaper.

PROGRAMMER RULES:
- mutable model access must be followed by the appropriate `Touch*()` publication;
- bulk semantic operations should emit one describable bulk event where practical;
- presentation-only UPDATE must invalidate only affected prepared/visible state unless it genuinely changes a derived projection;
- applications should carry stable identity in model IDs or `UiModelItem::data`, not display text;
- fix the lowest reusable layer that owns a defect; do not move domain semantics into generic models and do not leave shared model defects patched only in apps.

VALIDATION:
- complete touched model/view dependency slices and the relevant high-scale consumers were statically inspected;
- full Windows health smoke is pending while the supervisor freezes source;
- no Windows PASS is claimed for these newest model-audit commits yet.

NEXT ACTION:
- Gary runs the issued interim Windows health smoke against one refreshed `upp_Ui/main`: `UiModelMutationContractTest`, `UiModelViewPerformanceTest`, `UiTreeScaleTest`, corrected Graph 51-check gate, `UiGraphTest`, `UiButtonDemo`, plus Gallery-backed SymbolPicker build/launch;
- only concrete compiler/runtime evidence should reopen the shared-model source architecture.

## UI-DEMO-MODERNIZATION-PILOT — UIBUTTON

BASE:
- `de786f34bbf89dc083d9927e78950e5882e5bbae` — canonical demo modernization plan on `main`.

TASK:
- replace the legacy `examples/UiButtonDemo` builder with the first canonical full-control demo generation;
- use the UiLabel-style shell, production PropertyEditor, model-authored Inspector state, explicit inherited/local Theme Overrides and generated C++;
- preserve normal live UiButton interaction and use this demo as the first pilot before Dropdown.

TOUCHED:
- `examples/UiButtonDemo/UiButtonDemo.h`
- `examples/UiButtonDemo/UiButtonDemo.cpp`
- `examples/UiButtonDemo/UiButtonProperties.cpp`
- `examples/UiButtonDemo/UiButtonOverrides.cpp`
- `examples/UiButtonDemo/UiButtonCode.cpp`
- `examples/UiButtonDemo/main.cpp`
- `examples/UiButtonDemo/UiButtonDemo.upp`
- `docs/04_UI_DEMO_GUIDE.md`
- `docs/ACTIVE_WORK.md`

SOURCE CONTRACT:
- the old demo-only `ButtonConfig` / custom property-row builder is removed rather than retained beside the new path;
- the preview is the real `UiButton`;
- Inspector values map to live public Button APIs, including content/icon layout, sizing, focus, checkable/checked and underline behavior;
- a direct preview click synchronizes the resulting checked state back into the Inspector model so PropertyEditor remains authoritative;
- Theme Overrides use the live Button style surface and canonical General / Face/Skin / Frame / Ink / Icon / Typography / Focus / Shadow / Highlight / Press Offset grammar;
- `press_offset` is exposed because the Button paint path consumes it; `overpaint` is deliberately not exposed because the current Button implementation does not consume it;
- generated code reads the same Inspector/override models and omits custom style code when no local override is active;
- no UiDesigner dependency or replacement demo framework is introduced.

PUBLISHED:
- `8c5b8168206f195711a4b624b9c5793c7d6fb02d` — replaced the legacy UiButton demo with the canonical UiLabel-style shell, production PropertyEditor Inspector/Overrides, model-authoritative live preview and generated-code page.

VALIDATION:
- complete current Button header/implementation and legacy demo package inspected;
- canonical UiLabel demo and current PropertyEditor/override guides used as the reference;
- declarations/definitions, package dependencies, group ownership and generated-code paths statically reviewed;
- implementation commit is one fast-forward commit ahead of the recorded base and its GitHub diff contains only the UiButton demo package replacement;
- no Windows build/run PASS is claimed yet.

NEXT ACTION:
1. Windows Debug build + launch `examples/UiButtonDemo` and smoke Inspector, Overrides, Code, direct checkable interaction and Light/Dark;
2. if green, record acceptance and proceed to `UiDropdownDemo`; do not start the broader pilot rollout from an unvalidated Button shell.

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

`upp_uisymbolpicker` is **not a Graph consumer for its icon library**. Its production library is now `UiGallery` backed by a filtered `UiListModel` presentation projection.

PUBLISHED SYMBOL PICKER BASELINE:
- `5ebd0182033fdc9195df8b91f577eeb9631da8e8` — documented 5K Gallery baseline;
- final code immediately beneath that documentation: `36a5c77c9cc843edb393f54921ae8a88f167a0a8`.

SOURCE RESULT IN `Trilec/upp_uisymbolpicker`:
- removed the old 2,288-line per-icon `SymbolPickerView`, responsive layout and `SymbolPickerIconTile : ParentCtrl` library path;
- removed the hard 240-item All cap;
- full active-style projection is intended to expose roughly 5,057 generated symbols;
- library uses `SymbolPickerLibraryProjection -> UiListModel -> UiGallery`;
- collection surface also uses `SymbolPickerCollectionProjection -> UiListModel -> UiGallery`;
- Gallery retains only a viewport/overscan-sized `UiItemRender` pool rather than one Ctrl hierarchy per logical symbol;
- stable catalog identity is carried through `UiModelItem::data` / projection row mapping;
- catalog lookup by catalog/source ID is indexed rather than repeatedly scanning the generated catalog;
- style-aware category counts correspond to the active style;
- SVG/image previews are decoded lazily from Gallery useful visible range and reused through a simple hashed rendered-image cache with hard 8,192-entry ceiling;
- domain/project/command state remains in SymbolPickerModel/Catalog/CommandStack rather than being duplicated into `UiListModel`;
- structural 5K Gallery smoke covers full active-style exposure, no 240 cap, bounded renderer pool/Paint, deep final-item navigation and stable selection tokens.

SHARED-LAYER LESSONS NOW ROLLED INTO `upp_Ui`:
- lifetime-safe model observation;
- shared sequential List/Gallery/Dropdown remapping;
- ranged `UiListModel::Touch()` for prepared visible image/content batches;
- projection-local UPDATE handling;
- one-event bulk Tree import;
- explicit mutable-access publication APIs across the lightweight models.

The Symbol Picker's current visible-image loops may now be simplified to mutate the prepared image fields and publish one ranged `Touch()` per useful range. That is an application adoption of an already-shared API, not a new model redesign; perform it only after/with concrete build evidence and review the large workspace diff carefully.

Do **not** add Graph's spatial hash to Gallery. Uniform row/column arithmetic is cheaper and already computes visible and marquee-intersecting cells directly.

Potential later Gallery interaction polish, only if measured/useful:
- thin Accent marquee frame + light translucent Accent fill;
- transient marquee preview with semantic commit on release if very large live multi-selection notifications prove expensive;
- damage only old/new marquee and changed visible cells;
- preserve capture-ownership hardening already present in Gallery.

## NEXT

1. Gary runs the interim Windows health smoke against one refreshed common `upp_Ui/main` and reports exact tested SHA.
2. If the shared-model/Graph/Button smoke is green, freeze that common Ui baseline for the first full SymbolPicker Windows acceptance.
3. SymbolPicker Debug NON-BLITZ build/launch must show the full ~5,057 active-style Gallery promptly, with bounded renderer count and lazy preview fill; no restoration of the 240 cap or per-item Ctrl path.
4. Graph still requires the corrected 51/0 Debug scale rerun before its final manual acceptance can be called closed.
5. Delete the inert `temp-demo-button-do-not-use` branch when convenient; it is not a working branch.
