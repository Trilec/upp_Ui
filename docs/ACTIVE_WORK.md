# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Work directly from refreshed `main`; preserve unrelated concurrent advances.

## CURRENT SUPERVISORY STATE — 2026-08-18

STATUS: **UI-NODEGRAPH-FINAL-AUDIT-R1 SOURCE IMPLEMENTATION COMPLETE — FINAL WINDOWS VALIDATION PENDING.**

Detailed prior history is preserved in:
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`

Remote branch state is `main` only.

## ACTIVE TASK — UI-NODEGRAPH-FINAL-AUDIT-R1

Audit start / recovery base:
- `9172d67ebcbbad91b1ced2f1f2d05e07814c6b97` — final-audit recovery marker over Gary's last pre-audit Graph code.

Pre-audit Graph code:
- `1e1d14664a71eb225ab470241ecf7a62338bdbf4` — five explicit `BufferPainter::Finish()` calls. Windows tests remained green, but this did not correct the dark committed-selection appearance.

Final hardening implementation checkpoints:
- `80b02c7d14e53fd3f5956e91a06f4afe140f657a` — prune empty Graph spatial-hash cells.
- `ed25df1e9254e7eab0d0b974db384e16e15887f1` — introduce lifetime-aware shared model observer identity.
- `c4189c324ff5921d8fd16cd0ead0ca0dff7046c5` — make shared model copy/assignment semantics safe with weak identity: copies receive fresh observer identity; assignment preserves destination identity/callbacks.
- List/Gallery/Tree/Table/Dropdown/Menu headers were migrated to `UiModelObserverSet` in bounded commits; no public model API changed.
- `cf930052ab242faf6be6f7c0686f732854728db7` — NodeGraph header migrated to lifetime-safe model observers.
- `34443e9385f19382d2d4f2c9834bd1d8b710b655` — Graph source convergence: semantic Accent selection colour, public spatial hit-test authority, genuinely local prepared node/edge rebuild helpers, local node-style-class preview updates.
- `60e6223d913f78770debe2a481c03cd1d7e19697` — model-binding lifetime regression coverage.
- `2a43ebb4525ef521e0d851ea8b38eee8aadbbe88` — Graph scale regressions for public spatial hit tests and local style-class updates.
- `b3375564dff21c124374472aabfd79d62ef0d51e` — direct `UiNodeGraph::Style` selection default aligned with the bright Accent chrome contract.

Programmer documentation:
- `4a4a2b69cf046fe1be599fccf0893d32b574c19a` — model API/lifetime audit updated.
- `9de91580d717a157e8b3b4ae021fdedd7d7b849f` — model-view scale guide updated with final NodeGraph spatial/prepared contract.

### FINAL SOURCE CONTRACT

Keep these decisions:
- semantic topology lives only in `UiGraphModel`; stable node/edge ids are `int64`;
- per-node adjacency is a derived local-degree index, not a second semantic graph;
- ordinary graph nodes remain painted virtual objects, not one `Ctrl` per node;
- the retained world-space spatial hash is the sole broad-phase spatial index;
- prepared screen geometry is viewport-bounded derived view state;
- public and live pointer node/port/edge hit testing use the same small world-space spatial query followed by exact geometry tests;
- node/edge removal prunes vacated spatial buckets when the bucket becomes empty;
- pan/zoom reuse the retained spatial hash;
- node-style-class edits rebuild only currently prepared users of that class plus prepared incident edges; they do not rebuild the full spatial index or full prepared viewport;
- `RebuildNodeAndEdges()` / `RebuildEdge()` now perform local prepared updates when the prepared scene is clean;
- marquee selection commits on release; local live preview uses the retained hash and marquees over 256 cells deliberately defer preview until release;
- `BeginBatchUpdate()` / `EndBatchUpdate()` coalesce UiNodeGraph retained-index/geometry work around authoritative immediate model mutation;
- middle-button pan remains capture-free; left-button capture release remains re-entrancy-safe;
- committed selection chrome is the semantic bright Accent path and remains an independent approximately 2px shape-following final overlay;
- canonical `Model()/SetModel()/UseInternalModel()/ClearModel()` ownership semantics remain unchanged;
- no quadtree/R-tree/BVH replacement is required for this scale target;
- current Painter backend remains replaceable later without moving semantic/spatial authority into a GPU backend.

### MODEL OBSERVER LIFETIME HARDENING

List, Gallery, Tree, Table, Dropdown, Menu and NodeGraph now share `UiModelObserverSet` rather than retaining raw-address-only bound-model identity.

Rules:
- external active models remain non-owning and must outlive active use;
- inactive callbacks may remain installed but are ignored by views that no longer use that model;
- expired weak identities are pruned before binding deduplication;
- if model B is allocated at the exact address previously occupied by destroyed model A, B receives a fresh observer;
- copying `UiDataModelBase` creates a fresh weak identity and does not copy callbacks;
- assigning `UiDataModelBase` preserves the destination object's weak identity and installed callbacks.

This matches the same-address lifetime hardening already used by UiDoc without changing the ordinary model API.

### DETERMINISTIC TEST CONTRACT AFTER THIS HARDENING

Expected after compile succeeds:
- `Utilities/UiGraphTest`: existing **90/90** Debug + Release.
- `Utilities/UiNodeGraphScaleTest`: **UINODEGRAPH_SCALE_SUMMARY checks=51 failed=0** Debug + Release.
- `Utilities/UiModelBindingContractTest`: **Checks: 55, Fails: 0** Debug + Release.
- `Utilities/UiDataModelsTest`: existing **7535 checks / 0 fails** Debug regression.

New model-binding coverage includes:
- independent weak identity after base copy;
- destination identity preservation after assignment;
- expired weak observer identity after destruction;
- exact same-address helper reuse;
- UiList callback delivery after exact same-address external model reuse;
- UiNodeGraph callback/spatial response after exact same-address external model reuse.

New Graph scale coverage includes:
- public node hit candidate count stays tiny;
- public port hit candidate count stays tiny;
- public edge hit candidate count remains locally bounded;
- updating the actually-used `soft` node style class increments neither the full spatial-build serial nor the full geometry-build serial;
- default committed selection chrome is the explicit bright-blue contract.

No Windows PASS is claimed for these new source changes yet.

### PRE-HARDENING WINDOWS EVIDENCE

Gary previously established on the R2 line:
- `Utilities/UiGraphTest` Debug + Release: **90/90 passed**.
- `Utilities/UiNodeGraphScaleTest` Debug + Release: **49/0** before the two new checks.
- `Utilities/UiDataModelsTest` Debug: **7535 / 0 / PASS**.
- `examples/UiGraphDemo` Debug built/launched responsively; Release built successfully.
- `git diff --check`: PASS; reported worktree clean.

That evidence remains useful regression history but does not validate the final weak-observer/local-prepared changes.

### MANUAL ACCEPTANCE TO RESUME

Gary must start with the original stop condition:
- selected Rectangle must show clearly blue approximately 2px shape-following committed-selection chrome, not dark/black.

If that passes, continue:
- selection chrome across every standard shape;
- Windows-style local marquee and adaptive large-marquee fallback;
- 10k pan/zoom and bounded candidate/prepared counts;
- multi-node batch drag/key/delete;
- PropertyEditor Face FillRecipe / Color live preview and Cancel restoration, with no full prepared/spatial rebuild stall;
- middle-pan capture regression;
- Light/Dark theme smoke;
- Reference -> 10k -> Reference model retention;
- exceptional embedded controls survive model switching.

## SYMBOL PICKER / OTHER CONSUMERS

The architecture is now source-stable. Other sessions may continue design and source review against current `main`, but should wait for the final Gary Windows PASS/final SHA before treating this as the accepted Graph integration baseline.

Do not reintroduce:
- parallel semantic node stores;
- Ctrl-per-ordinary-node rendering;
- prepared-viewport scan hit testing;
- raw-address-only model observer deduplication;
- full spatial/prepared rebuilds for ordinary local style preview;
- retired `GetModel()` / `GetInternalModel()` aliases.

## OTHER ACTIVE WORK

### Four-control override normalization

Status: **SOURCE IMPLEMENTATION COMPLETE — WINDOWS VALIDATION PENDING.**

Authoritative detail and exact gates are in `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`.

### UiLabel

Accepted reference implementation. Continue using the accepted UiLabel PropertyEditor grammar and shared FillRecipe conventions; do not introduce a parallel schema dialect.

## NEXT

1. Perform final static diff/package/declaration review from `9172d67...` to current `main`.
2. Gary runs the final bounded Windows validation below the current main-only branch.
3. If Gary reports only trivial mechanical compile issues, fix minimally and republish; substantive failures return to supervisor.
4. After all automated/manual gates pass, current/final main becomes the accepted Graph/model baseline for Symbol Picker and other consumers.
