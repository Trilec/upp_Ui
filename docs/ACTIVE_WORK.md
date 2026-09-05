# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; not project history.

## CURRENT
TASK: **final-pixel geometry + authored shape foundation**
STATUS: **PUBLISHED — SHAPE-LAYER WINDOWS VALIDATION PENDING**
ROUTE_FIX: `454f35a98db17044f6a45652024e8beedb4296b9` — **PASS**
SHAPE_LAYER: `d9b956abeccc4220a481a50aa153da21c93b5b22`
DOCS: `9cda7327616e2ebebb02e3e488441bd218b42365`

## CONTRACT
- generated curves use one library-owned **0.35 final-device-pixel** error budget;
- direct Draw/native Painter remains first choice for simple paint;
- normal controls use `UiShapes` / `UiShapePath` when reusable authored topology is useful;
- **dense scenes such as UiNodeGraph may use `UiGeometry` directly** when authored-path allocation would be unnecessary work;
- semantic handles/labels/anchors never depend on tessellation vertex indexes;
- raster/cache policy remains separate from geometry.

## ACCEPTED
- P2 Windows Debug/Release: PASS;
- geometry contract Windows Debug/Release: PASS;
- route-midpoint correction Windows Debug/Release: PASS;
- `UiGeometryContractTest`: 25/25;
- `UiNodeGraphRouteEditTest`: 25/25;
- core Graph regressions and quick Graph smoke: PASS.

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

## VALIDATION / NEXT
1. Gary: validate published `UiShapePath` / `UiShapes` layer on current `main`.
2. If green, mark geometry + authored-shape foundation PASS.
3. Clean obsolete supervisor branches that are confirmed ancestors/duplicates.
   **Do not delete `supervisor/test-example-hygiene-20260905`; it still contains unique work.**
4. Investigate zoomed-out 10k idle continuous repaint before further Graph work.
