# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Work directly from refreshed `main`; preserve unrelated concurrent advances.

## CURRENT SUPERVISORY STATE — 2026-08-18

STATUS: **UIGRAPH R2 AUTOMATED WINDOWS GATES PASS; FINAL GRAPH/MODEL CLEANUP AUDIT IN PROGRESS; MANUAL ACCEPTANCE STILL BLOCKED ON SELECTION CHROME.**

Detailed prior history is preserved in:
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`

Remote branch state has been pruned to `main` only.

## ACTIVE TASK — UI-NODEGRAPH-FINAL-AUDIT-R1

REFRESHED BASE: `1e1d14664a71eb225ab470241ecf7a62338bdbf4` — current `main` at audit start.

Published Graph checkpoints immediately preceding this audit:
- `ef3d518a1a1ddd7edca1c4e5857433fc6f36d527` — R2 retained spatial interaction / adaptive marquee / batch-update implementation.
- `13337c98b98b26898c0c5fb5c1eddd0fac1088ce` — Gary mechanical Windows compile fix: explicit integer casts around `StyledState` bounds in `UiGraphDemo.cpp`.
- `1e1d14664a71eb225ab470241ecf7a62338bdbf4` — five explicit `BufferPainter::Finish()` calls before Graph temporary image buffers are consumed.

### WINDOWS EVIDENCE

Gary's focused automated gate is green on the published R2 line:
- `Utilities/UiGraphTest` Debug + Release: **90/90 passed**.
- `Utilities/UiNodeGraphScaleTest` Debug + Release: **UINODEGRAPH_SCALE_SUMMARY checks=49 failed=0**.
- `Utilities/UiDataModelsTest` Debug: **7535 checks / 0 fails / PASS**.
- `examples/UiGraphDemo` Debug: builds, launches and remains responsive.
- `examples/UiGraphDemo` Release: builds successfully.
- `git diff --check`: PASS; worktree clean at each reported stop.

The explicit Painter-finalization experiment did **not** change the manual failure: the initially selected Rectangle still appears with a dark/black committed-selection outline. Manual acceptance remains stopped before marquee/10k/batch/PropertyEditor/pan/theme/model-retention smoke.

### ACCEPTED R2 ARCHITECTURE

Keep these decisions:
- semantic topology lives only in `UiGraphModel`; stable node/edge ids are `int64`;
- per-node adjacency is a derived local-degree index, not a second semantic graph;
- ordinary graph nodes remain painted virtual objects, not one `Ctrl` per node;
- the retained world-space spatial hash is the sole broad-phase spatial index;
- prepared screen geometry is viewport-bounded derived view state;
- point interaction uses a small world-space spatial query followed by exact shape/port/edge tests;
- marquee selection commits on release; local live preview uses the retained hash and large marquess deliberately defer preview after the 256-cell threshold;
- `BeginBatchUpdate()` / `EndBatchUpdate()` coalesce UiNodeGraph retained-index/geometry work around authoritative immediate model mutation;
- middle-button pan remains capture-free; left-button capture release remains re-entrancy-safe;
- canonical `Model()/SetModel()/UseInternalModel()/ClearModel()` ownership semantics remain unchanged;
- no quadtree/R-tree/BVH replacement is required for this scale target.

### FINAL AUDIT FINDINGS TO CORRECT BEFORE SYMBOL-PICKER INTEGRATION

1. **Selection colour resolution**
   - theme resolution currently sets `selection_box_frame` to Standard `accent_pressed`, RGB `(0,96,176)`;
   - this is considerably darker than the Windows-style Standard `accent`, RGB `(0,120,212)`, and the explicit Painter-finalization test proved buffer completion was not the visual cause;
   - use the semantic Accent colour for committed selection/marquee chrome and add rendered-image coverage instead of changing model/spatial architecture.

2. **Duplicate public hit-test path**
   - live interaction uses `HitTestNodeSpatial/PortSpatial/EdgeSpatial`;
   - public `HitTestNode/Port/Edge` still contain the older prepared-geometry scan loops;
   - converge the public API onto the retained spatial broad phase and remove the duplicate scan implementation.

3. **Empty spatial-cell retention**
   - node/edge removal removes ids from hash buckets but currently leaves empty `SpatialCell` entries behind;
   - prune a bucket when both node and edge vectors become empty so repeated long-distance editing cannot accumulate empty cells.

4. **Node-style preview still rebuilds all prepared geometry**
   - `SetNodeStyleClass()` / `RemoveNodeStyleClass()` correctly avoid rebuilding the world spatial index, but currently invalidate/rebuild the entire prepared viewport;
   - update only currently prepared nodes using the changed class plus their prepared incident edges, repainting their old/new damage;
   - off-screen nodes need no work and resolve the new class when they later enter the viewport.

5. **Misleading incremental helper names**
   - `RebuildNodeAndEdges()` / `RebuildEdge()` currently invalidate and rebuild the complete prepared viewport despite their names;
   - either make them genuinely local as part of item 4 or remove/rename them so the implementation matches the API intent.

6. **Ordinary model observer lifetime identity**
   - List/Gallery/Tree/Table/Dropdown/Menu/NodeGraph still deduplicate model observers with raw model addresses in `bound_models_`;
   - UiDoc already solved the same-address-reuse problem with `Pte`/`Ptr` weak lifetime bookkeeping;
   - harden the ordinary model-backed controls to the same lifetime-safe identity rule without changing public ownership semantics, and add a deterministic same-address-reuse contract regression.

### CLEAN FINDINGS

- `UiGraphModel` adjacency maintenance is coherent across add/update/reconnect/remove and serialization rebuild; ordinary local topology operations no longer scan the complete edge set.
- spatial edge bounds deliberately overestimate routes, avoiding false-negative culling; custom/dynamic routes conservatively fall back to global candidates.
- pan/zoom reuse the retained spatial hash.
- current Graph header has a useful Purpose/Intent/Thread/Model-ownership description and its private areas are broadly grouped by style, model binding, spatial, prepared geometry, paint and interaction.
- public scale diagnostics are extensive but currently useful to demos/tests; do not remove them during this corrective unless a compatibility-safe diagnostics snapshot is introduced deliberately.

## SYMBOL PICKER / OTHER CONSUMERS

Other sessions may read and design against the R2 architecture now, but should **not publish an integration pinned to `1e1d146...` as the final Graph baseline**. Wait for this final audit corrective and its Windows validation SHA.

The final handoff will preserve:
- compact authored 1:1 nodes;
- stable model identities and canonical model binding;
- retained spatial-hash culling;
- viewport-prepared geometry;
- local point/marquee interaction;
- batched view updates;
- UiTheme/style-class presentation;
- UiLabel-compatible PropertyEditor grouping and shared FillRecipe use.

## OTHER ACTIVE WORK

### Four-control override normalization

Status: **SOURCE IMPLEMENTATION COMPLETE — WINDOWS VALIDATION PENDING.**

Authoritative detail and exact gates are in `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`.

### UiLabel

Accepted reference implementation. Continue using the accepted UiLabel PropertyEditor grammar and shared FillRecipe conventions; do not introduce a parallel schema dialect.

## NEXT

1. Implement and publish the six bounded final-audit corrections above in recoverable checkpoints.
2. Source-review the full Graph/model-binding diff and update programmer comments/docs where the final contracts changed.
3. Gary reruns the focused Graph tests plus `UiModelBindingContractTest` and resumes manual Graph acceptance from selection chrome onward.
4. Once Windows accepted, publish the final Graph/model handoff SHA for Symbol Picker and other consumers.
