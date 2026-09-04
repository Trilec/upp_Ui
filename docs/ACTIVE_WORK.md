# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; not project history.

## CURRENT
BASE: `cc0ecba394c8fb2d52cdd7cb1dbbe92ead5cd88e`
TASK: **final-pixel geometry + authored path/stock shape foundation**
STATUS: **PUBLISHED — WINDOWS VALIDATION PENDING**
SHAPE_LAYER: `d9b956abeccc4220a481a50aa153da21c93b5b22`
ROUTE_FIX: `454f35a98db17044f6a45652024e8beedb4296b9`

## CONTRACT
- explicit generated curves use one library-owned **0.35 final-device-pixel** error budget;
- direct Draw/native Painter remains first choice for simple paint-only primitives;
- normal controls use `UiShapes` for reusable silhouettes and `UiShapePath` for custom authored silhouettes;
- **dense scenes such as UiNodeGraph may use `UiGeometry` directly** when UiShapePath allocation would be unnecessary work;
- no control-owned sample-count/curve-quality knobs;
- semantic handles/labels/anchors never depend on tessellation vertex indexes;
- raster policy remains separate from geometry.

## PUBLISHED COVERAGE
- `UiGeometry`: final-pixel math, adaptive curves, containment, radial and polyline helpers;
- `UiShapePath`: Move/Line/Quadratic/Cubic/Arc/EllipseArc/Close + multi-contour flattening;
- `UiShapes`: polygon/rounded, rect/capsule/ellipse, N-gon/star, arrow/chevron, chamfer, callout, tag, cloud/document/database, ring/pie;
- `UiPainterShapePath()` lives in UiDraw;
- docs/README/Topic++ updated; Graph hot source unchanged in shape-layer tranche.

## VALIDATION
- static full-diff/mechanical review PASS; no forbidden sampling knobs or added whitespace defects;
- `UiShapePathTest`: **27 deterministic checks** added;
- prior geometry gate: all PASS except RouteEdit midpoint semantics; fix published, targeted retest still pending;
- shape-layer Debug/Release Windows gate still required.

## NEXT
1. Gary: run route-fix + UiShapePath targeted Debug/Release gate and clean obsolete supervisor branches.
2. If green, mark geometry/shape foundation PASS.
3. Investigate zoomed-out 10k idle continuous repaint before further hierarchy/render work.
