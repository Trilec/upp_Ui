# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; not project history.

## CURRENT
BASE: `620134c56f7fd48559112ad197bc51dcd2582f29` — UiGraph performance P2.
P2: **PASS** on Windows CLANGx64 Debug/Release; no Gary source edits.
TASK: **UiGeometry first-class final-pixel geometry contract**
STATUS: **PUBLISHED — PLATFORM VALIDATION PENDING**
IMPLEMENTATION: `6aa98a23f8449a5eb7f796f43c28b071cb6da938`
PUBLISHED_GEOMETRY: `e4022cef29b2c5962ff9248bab2392ee84c00141`

## CONTRACT
- one library-owned **0.35 device-pixel** error budget for explicit generated geometry;
- geometry decisions use final pixels after DPI/view transforms;
- no control-owned tessellation quality knobs;
- prefer direct `Draw`, then native Painter, then `UiGeometry` explicit points only when required;
- preserve authored discrete topology; geometry remains independent of colour/theme/cache/backend.

## COVERAGE
- arcs/ellipses, rounded rect/capsule, quadratic/cubic, rounded polyline/polygon;
- line/polyline metrics and distance, analytic containment, radial band/pie geometry;
- generated-polyline simplification and pixel-significance tests;
- shared cap/UiTab/Bezier editor and fixed-detail UiNodeGraph geometry/routes migrated;
- paint-only circles use native Painter; Graph hit tests reuse prepared silhouettes.

## VALIDATION / REMAINING
- complete-file/diff/mechanical source review passed before publication;
- Windows geometry/render/Graph Debug+Release gate still required;
- P2 fit-all 10k paint improved ~6x and intermediate micro LOD passed;
- observed P2 demo issue: zoomed-out 10k viewport can continuously repaint while idle (~41% core);
- warm unchanged 10k rebind still rebuilds one spatial index;
- raster efficiency remains separate: blur, gradients, masks, temporary buffers, image analysis.

## NEXT
Run the UiGeometry + render + Graph Windows gate on current `main`.
If green, investigate the idle continuous-repaint cause before further hierarchy work.
