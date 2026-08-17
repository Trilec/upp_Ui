# ACTIVE WORK

Remote GitHub is authoritative. Never force-update `main`. Work directly from refreshed `main`; preserve unrelated concurrent advances.

## CURRENT SUPERVISORY STATE — 2026-08-17

STATUS: **UIGRAPH SPATIAL INTERACTION R2 SOURCE COMPLETE — WINDOWS VALIDATION PENDING; UI LABEL REFERENCE ACCEPTED; FOUR-CONTROL SOURCE WORK COMPLETE — WINDOWS VALIDATION PENDING.**

Detailed prior history is preserved in:
- `docs/ACTIVE_WORK_ARCHIVE_PRE_FOUR_CONTROL_2026-08-17.md`
- `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`

## ACTIVE TASK — UI-NODEGRAPH-SPATIAL-INTERACTION-R2

BASE: `5ff17bdf2d365fbf95d172690f1c1f7fc30e0e1a` (`main` at R2 refresh).

Concurrent `main` advanced during implementation. The final code checkpoint was carried onto `b149d9825d9e16e3bf0997e8a282706745921065`; the concurrent delta touched only `Utilities/UiListStyleContractTest/main.cpp` and was preserved.

PUBLISHED:
- `f85751adb4620776bdb34e0128fb035ee3ed5a06` — retained-hash marquee candidate cache and cached mouse-up commit path.
- `7d06f5b96767faa33eea0f7690a8a0dfe6dbe581` — R2 recovery checkpoint.
- `ef3d518a1a1ddd7edca1c4e5857433fc6f36d527` — final R2 code: adaptive Windows-style marquee, candidate preview overlay, nested view-side batch coalescing and deterministic batch coverage.

TOUCHED BY R2:
- `Ui/UiGraph/UiNodeGraph.h`
- `Ui/UiGraph/UiNodeGraph.cpp`
- `Ui/UiGraph/UiNodeGraphInteraction.cpp`
- `Utilities/UiNodeGraphScaleTest/main.cpp`
- `docs/ACTIVE_WORK.md`

### SOURCE RESULT

Spatial broad phase:
- the existing retained world-space spatial hash remains the only broad-phase index; no quadtree/R-tree/BVH duplicate was introduced;
- production mouse-down, hover, double-click, port targeting and edge targeting query small world-space neighborhoods through the hash, then exact-test local prepared geometry;
- ordinary node mutation updates the affected node spatial record plus incident edge records rather than rebuilding the complete spatial index.

Marquee:
- committed selection does not change while the drag is active;
- normal/local marquee movement converts the rectangle to world space and queries the same retained hash used by viewport culling;
- candidate IDs are transient view state only and receive a subtle shape-following blue preview hint;
- the visible rectangle follows the Windows-style convention: 1px blue frame and approximately 9–10% translucent blue fill over the existing scene;
- marquee repaint is damage-bounded to old/new border strips, changed translucent-fill strips and nodes whose preview membership changed;
- if the marquee spans more than 256 spatial cells, live candidate preview is deliberately deferred so very large/zoomed-out drags remain a cheap overlay; mouse-up performs one final spatial query;
- local mouse-up commits the already-cached preview IDs rather than issuing a duplicate query.

Batch mutation:
- `UiNodeGraph::BeginBatchUpdate()` / `EndBatchUpdate()` are nested view-side coalescing scopes around authoritative `graph.Model()` mutation;
- UiGraphModel data, revision and notifications remain immediate for other observers; only UiNodeGraph retained spatial/geometry response is deferred;
- touched/removed node and edge IDs are deduplicated during the transaction;
- the outer commit updates final node records and unique incident-edge records once, then performs one prepared-geometry pass;
- if the spatial index was already dirty, or Reset/Clear occurs inside the batch, the outer commit performs one final full spatial rebuild rather than repeated intermediate rebuilds;
- same-model `SetModel()` remains idempotent; switching to a different model while a batch is open is rejected/asserted;
- internal multi-node drag commit, Delete selection and arrow-key multi-node movement use the batch path.

Rendering/model invariants retained:
- ordinary nodes remain painted virtual objects, not Ctrl-per-node;
- semantic topology remains solely in UiGraphModel;
- transient drag positions remain outside the authoritative spatial index until commit;
- edges remain below nodes;
- committed selection remains the independent final antialiased 2px shape-path overlay;
- middle-button pan remains capture-free;
- canonical `Model()/SetModel()/UseInternalModel()/ClearModel()` ownership semantics are unchanged.

### DETERMINISTIC SCALE CONTRACT

`Utilities/UiNodeGraphScaleTest` expected result after R2: **49 checks / 0 failures**.

Fixture remains:
- 10,000 nodes;
- 19,800 bounded-neighbour edges;
- deterministic mixed shapes, semantic roles and reusable style classes.

R2 evidence includes:
- 0.5 zoom pointer hover uses a tiny local spatial neighborhood;
- local marquee movement leaves geometry and spatial-build serials unchanged;
- local marquee preview candidate count stays bounded and mouse-up commits that preview;
- nested batch mutation performs no spatial-update or geometry-build work until the outer commit;
- inner `EndBatchUpdate()` does not flush;
- outer commit increments the batch-flush serial once, leaves full spatial-build serial unchanged for local mutations, updates only touched nodes/unique incident edges, and rebuilds prepared geometry once.

VALIDATION: **source reviewed; no Windows/U++ PASS claimed yet.**

## WINDOWS VALIDATION — UIGRAPH

Gary/validator must validate a current `main` descendant containing `ef3d518a1a1ddd7edca1c4e5857433fc6f36d527`.

Required automated gates:
1. `Utilities/UiGraphTest` Debug + Release — expected 90/90 unless current authoritative source has legitimately advanced the count; report exact output.
2. `Utilities/UiNodeGraphScaleTest` Debug + Release — require `UINODEGRAPH_SCALE_SUMMARY checks=49 failed=0`.
3. `Utilities/UiDataModelsTest` Debug — expected existing baseline 7535/0 unless current authoritative source has legitimately advanced it; report exact output.
4. `examples/UiGraphDemo` Debug + Release build; launch Debug.
5. `git diff --check` and clean `git status --short`.

Manual Graph checks:
- Reference mode authored 1:1 presentation remains compact and readable.
- FillRecipe/Color PropertyEditor live preview and Cancel rollback work.
- all standard shapes use the same final 2px committed-selection chrome.
- double-click leaves only the double-clicked node selected.
- local marquee looks like the Windows selection rectangle: thin blue frame + very light translucent fill; candidate nodes may show the subtle preview hint while dragging.
- at 0.5 zoom local marquee remains smooth; no committed selection appears until release.
- very large/zoomed-out marquee remains smooth even when live candidate hints are suppressed; selection resolves on release.
- 10k mode reports bounded prepared/candidate counts during local navigation.
- multi-node drag/keyboard move/delete show no obvious per-node rebuild stall.
- middle-pan regression remains clean, including leave/re-enter/release-outside cases.
- Reference -> 10k -> Reference preserves internal model and attached-control ownership.

Stop on substantive runtime/assertion/model/rendering/performance failure. Gary may publish only an obvious local Windows/U++ mechanical correction; architecture returns to the supervisor.

## OTHER ACTIVE WORK

### Four-control override normalization

Status: **SOURCE IMPLEMENTATION COMPLETE — WINDOWS VALIDATION PENDING.**

Authoritative detail and exact gates are in `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`.

Key checkpoints include:
- UiList renderer authority: `d0579b8753748ca765710f6c29805d2859ddf6aa`.
- UiList striped-row persistence: `97d1531192f712365cddea1f9390a1a031e01836`.
- UiDesigner List/Edit adapters: `c27f499c8d51ad73037d9a60481bb73d870d38a7`.
- UiDesigner Dropdown/Accordion adapters: `ec02f1cbcc040f70ad55e656b98ec64640142cec`.

### UiLabel

Accepted reference implementation. Do not invent a parallel PropertyEditor grouping/schema dialect; new control demos/Designer adapters should continue converging on the accepted UiLabel grammar and shared FillRecipe conventions.

## NEXT

1. Run the UiGraph Windows gate above; diagnose any substantive failure rather than weakening tests or reverting the retained spatial architecture.
2. Run the four-control Windows gates in `docs/ACTIVE_WORK_UI_OVERRIDE_ROLLOUT.md`.
3. Publish only genuinely mechanical platform corrections if required.
4. Close accepted validation checkpoints, then continue remaining control normalization from the same model/style/PropertyEditor conventions.
