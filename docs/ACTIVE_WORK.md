# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; not project history.

## CURRENT
BASE: `ffe51a2750db4944f54877d60aa15bbde036f2c9`
GEOMETRY CONTRACT: Windows gate PARTIAL only because RouteEdit midpoint semantics regressed; all geometry/render/P2 checks otherwise PASS.
TASK: **Route midpoint semantics independent of adaptive tessellation**
STATUS: **PUBLISHED — TARGETED WINDOWS RETEST PENDING**
FIX: `454f35a98db17044f6a45652024e8beedb4296b9`

## ROOT CAUSE / FIX
- adaptive Bezier flattening correctly reduced a straight cubic to two endpoints;
- Graph still used `route[count/2]` as its semantic midpoint, which selected the target endpoint;
- `UiGeometry::PointAtPolylineFraction()` now derives positions by visible arc length;
- Graph label/initial route-handle midpoint now uses arc length, not vertex index;
- authored waypoint handle semantics and the 0.35px geometry contract are unchanged;
- geometry contract now explicitly forbids semantic positions from depending on tessellation indices.

## VALIDATION
Previously PASS on `ffe51a2`:
- UiGeometryContractTest 24/24;
- all Graph suites except UiNodeGraphRouteEditTest;
- P2 micro/render contracts;
- UiGraphDemo and UiProgressRingDemo visual smoke;
- diff-check clean.
New deterministic expectations:
- UiGeometryContractTest: **25 checks**;
- UiNodeGraphRouteEditTest: **25 checks**.

## REMAINING
- targeted Debug+Release retest required for the two changed suites plus core Graph regressions;
- zoomed-out 10k idle continuous repaint remains the next performance issue;
- warm unchanged 10k rebind still rebuilds one spatial index;
- raster efficiency remains separate: blur, gradients, masks, temporary buffers, image analysis.

## NEXT
Run targeted Windows gate on current `main`.
If green, mark geometry contract PASS and investigate idle continuous repaint before further hierarchy work.
