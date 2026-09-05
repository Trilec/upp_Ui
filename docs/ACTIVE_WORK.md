# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; not project history.

## CURRENT
TASK: **UiGraph 10k idle repaint**
STATUS: **SOURCE FIX PUBLISHED — WINDOWS IDLE VALIDATION PENDING**
BASE: `a6ef8ab843d31ddae6d074772a59420c2a479653`
FIX: `6c05ed14019db90fab4f63de7aa0734b92f98f99`
SHAPE_LAYER: `d9b956abeccc4220a481a50aa153da21c93b5b22` — **PASS**
ROUTE_FIX: `454f35a98db17044f6a45652024e8beedb4296b9` — **PASS**

## IDLE-REPAINT SOURCE DIAGNOSIS
- `UiNodeGraph::Paint()` is render-only and does not issue `Refresh()`.
- Graph owns no repeating frame ticker.
- live zoom owns one 140 ms `TimeCallback` settle; it is one-shot and clears approximate state.
- camera setters refresh only when view state changes; hover refreshes only when the hot semantic object changes.
- the installed UiGraphDemo runtime already debounces diagnostics to one post-interaction sample.
- a second fallback implementation in `UiGraphDemo.cpp` still started an unconstrained repeating 200 ms diagnostics ticker.
- `6c05ed...` removes that periodic fallback authority: diagnostics now stop the ticker and refresh only on demand.
- current source therefore has no unbounded UiGraph/UiGraphDemo diagnostics frame clock.

The old fallback ticker was a real idle-contract defect. Because the normal executable installs
`UiGraphDemoRuntime.cpp`, which already masked it with debounced behavior, Windows validation must
still determine whether the previously observed 10k idle CPU is now fully gone rather than claiming
the historic observation was proven to come from this one path.

## WINDOWS IDLE GATE
Validate exact current `main` / `6c05ed...` or descendant:
1. Debug + Release build `examples/UiGraphDemo`.
2. Open **10k scale**, Fit, then stop all mouse/keyboard interaction.
3. With diagnostics **off**, verify the static graph settles to idle and does not continuously repaint.
4. Open Diagnostics, enable **Live profiling**, perform one pan/zoom, then stop.
5. Verify one deferred diagnostics update occurs after interaction and the process returns to idle;
   no 200 ms repeating diagnostics refresh may remain.
6. Switch Reference -> 10k -> Reference -> 10k and repeat the idle check.
7. Confirm ordinary selection, pan, wheel zoom and exact settle still work.
8. Re-run the high-value Graph regression/performance slice affected by view/runtime behavior.
9. `git diff --check`; final worktree clean.

Prefer structural evidence (no continuing paint/invalidation activity) over a machine-specific CPU
percentage. If repaint continues, identify the caller issuing the next `Refresh`/layout before any
paint-path optimization.

## ACCEPTED FOUNDATION
Windows CLANGx64 Debug + Release at exact `8b8f6c3c8c776814c0d9ceda99456c3931840505`:
- `UiShapePathTest`: 27/27 PASS;
- `UiGeometryContractTest`: 25/25 PASS;
- `UiNodeGraphRouteEditTest`: 25/25 PASS;
- `UiNodeGraphCanonicalShapeTest`: 14/14 PASS;
- `UiGraphTest`: 90/90 PASS;
- Release `UiProgressRingDemo` + `UiGraphDemo` smoke: PASS;
- package/API exposure + `UiPainterShapePath`: PASS.

Geometry + authored-shape foundation is **ACCEPTED**.

## CONTRACT
- generated curves use one library-owned **0.35 final-device-pixel** error budget;
- direct Draw/native Painter remains first choice for simple paint;
- normal controls use `UiShapes` / `UiShapePath` when reusable authored topology is useful;
- dense scenes such as UiNodeGraph may use `UiGeometry` directly;
- semantic handles/labels/anchors never depend on tessellation vertex indexes;
- raster/cache policy remains separate from geometry;
- a static view must eventually become idle.

## DEFERRED
- warm unchanged 10k Reference->10k rebind may still perform one avoidable spatial rebuild — verify after idle repaint;
- raster efficiency audit: blur, gradients, masks, temporary ImageBuffers, immutable-image analysis,
  render-layer allocation/clearing;
- further Graph hierarchy work waits until current performance defects are reconciled.

## BRANCH STATE
Remote branches:
- `main`;
- `supervisor/test-example-hygiene-20260905`.

**Do not delete `supervisor/test-example-hygiene-20260905`; it contains unique unmerged UiDoc test-consolidation work.**

## CANONICAL DOCS
`00` Coding · `01` Controls · `02` Theme · `03` Model · `04` Demo ·
`05` PropertyEditor · `06` Large-scale Views & LOD · `07` Drawing & Geometry ·
`08` UiGraph · `09` UiDoc

Git history is implementation history; do not recreate tranche-specific guide files.
