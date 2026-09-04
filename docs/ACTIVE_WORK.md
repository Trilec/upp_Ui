# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; not project history.

## CURRENT
MAIN: `620134c56f7fd48559112ad197bc51dcd2582f29` — UiGraph performance P2, Gary validation in flight.
BASE: `620134c56f7fd48559112ad197bc51dcd2582f29`
TASK: **UiGeometry first-class final-pixel geometry contract**
BRANCH: `supervisor/ui-geometry-contract-20260904`
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING**
IMPLEMENTATION: `6aa98a23f8449a5eb7f796f43c28b071cb6da938`
VALIDATION: complete-file/diff/mechanical source review passed; Windows CLANGx64 Debug/Release pending.

## CONTRACT
- explicit generated geometry has one library-owned **0.35 device-pixel** error budget;
- geometry decisions use final pixels after DPI/view transforms; no control-owned quality knob;
- prefer direct `Draw`, then native Painter, then `UiGeometry` explicit points only when required;
- authored discrete topology is preserved; geometry is independent of colour/theme/cache/backend.

## COVERAGE
- arcs/ellipses, rounded rect/capsule, quadratic/cubic, rounded polyline/polygon;
- line/polyline metrics and distance, analytic containment, radial band/pie geometry;
- generated-polyline simplification and pixel-significance tests;
- shared cap/UiTab/Bezier editor and fixed-detail UiNodeGraph geometry/routes migrated;
- paint-only circles use native Painter; Graph hit tests reuse prepared silhouettes.

## AUDIT / REMAINING
- production `Ui/` manual curve sampling audit completed; known fixed/radius-proportional approximation loops migrated.
- P2 performance semantics preserved; no GPU, world-tile or persistent-scene cache introduced.
- raster efficiency is a separate next audit: blur, gradients, masks, temporary buffers and immutable-image analysis.
- warm unchanged 10k rebind still has the separately measured spatial rebuild.

## NEXT
Receive Gary P2 result first. Reconcile any authorised P2 fix before moving `main`.
Then publish this geometry checkpoint on the accepted P2 head and run UiGeometry + render + Graph Debug/Release regressions.
