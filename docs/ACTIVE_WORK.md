# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; not project history.

## CURRENT
TASK: **UiGraph 10k idle continuous repaint diagnosis**
STATUS: **READY — GEOMETRY + AUTHORED SHAPE FOUNDATION ACCEPTED**
BASE: `8b8f6c3c8c776814c0d9ceda99456c3931840505`
SHAPE_LAYER: `d9b956abeccc4220a481a50aa153da21c93b5b22` — **PASS**
ROUTE_FIX: `454f35a98db17044f6a45652024e8beedb4296b9` — **PASS**

## ACCEPTED FOUNDATION
Windows CLANGx64 Debug + Release at exact `8b8f6c3c8c776814c0d9ceda99456c3931840505`:
- `UiShapePathTest`: 27/27 PASS;
- `UiGeometryContractTest`: 25/25 PASS;
- `UiNodeGraphRouteEditTest`: 25/25 PASS;
- `UiNodeGraphCanonicalShapeTest`: 14/14 PASS;
- `UiGraphTest`: 90/90 PASS;
- Release `UiProgressRingDemo` smoke: PASS;
- Release `UiGraphDemo` smoke: PASS;
- `git diff --check cc0ecba..HEAD`: PASS;
- package/API exposure and `UiPainterShapePath` link/runtime coverage: PASS.

The geometry + authored-shape foundation is therefore **ACCEPTED**.

## CONTRACT
- generated curves use one library-owned **0.35 final-device-pixel** error budget;
- direct Draw/native Painter remains first choice for simple paint;
- normal controls use `UiShapes` / `UiShapePath` when reusable authored topology is useful;
- **dense scenes such as UiNodeGraph may use `UiGeometry` directly** when authored-path allocation would be unnecessary work;
- semantic handles/labels/anchors never depend on tessellation vertex indexes;
- raster/cache policy remains separate from geometry;
- a static view must eventually become idle.

## NEXT — IDLE REPAINT
Objective: determine why a static zoomed-out ~10k UiGraph viewport was previously observed repainting continuously.

First pass:
1. reproduce/verify the current behavior on current `main`;
2. identify the owner of repeated `Refresh` / invalidation / timer/frame activity;
3. distinguish repaint scheduling from paint-path cost;
4. inspect existing frame/timer ownership before changing rendering;
5. make the smallest coherent fix only after root cause is demonstrated;
6. re-run the affected Graph regression/performance slice and GUI smoke.

Do not optimize raster/geometry merely because paint is occurring; first prove why paint is being scheduled.

## DEFERRED
- warm unchanged 10k Reference→10k rebind may still perform one avoidable spatial rebuild — verify after idle repaint;
- raster efficiency audit remains separate: blur, gradients, masks, temporary ImageBuffers, immutable-image analysis, render-layer allocation/clearing;
- further Graph hierarchy work waits until current performance defects are reconciled.

## BRANCH STATE
Remote branches after validated cleanup:
- `main`;
- `supervisor/test-example-hygiene-20260905`.

**Do not delete `supervisor/test-example-hygiene-20260905`; it contains unique unmerged UiDoc test-consolidation work.**

## CANONICAL DOCS
- `00` Coding
- `01` Controls
- `02` Theme
- `03` Model
- `04` Demo
- `05` PropertyEditor
- `06` Large-scale Views & LOD
- `07` Drawing & Geometry
- `08` UiGraph
- `09` UiDoc

Git history is the implementation history; do not recreate tranche-specific guide files.
