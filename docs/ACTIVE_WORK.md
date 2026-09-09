# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `3ecd53a3c25cc32d4a8266299d844c1ef4717c2e`
TASK: **UIGRAPH-PRESENTATION-RING-01 — correct port/route-handle circular raster geometry**
BRANCH: `main`
STATUS: **PUBLISHED — WINDOWS VALIDATION PENDING**
TOUCHED:
- `Ui/UiGraph/UiNodeGraphPaintRich.inc`
- `Utilities/UiGraphRenderTests/Presentation.cpp`
- `Utilities/UiNodeGraphPresentationTest/main.cpp`
- `docs/ACTIVE_WORK.md`
PUBLISHED: `3ecd53a3c25cc32d4a8266299d844c1ef4717c2e`
VALIDATION: source/API review complete; Windows Debug/Release pending.

## ACCEPTED BASELINE

Eddie's execution consolidation is on main and remains authoritative:
- one explicit implementation owner per UiNodeGraph responsibility;
- one world spatial authority;
- immutable live camera projection + exact settle;
- separate Micro/Rich paint backends with shared admission/LOD policy;
- 10k pan retains projected geometry and reports paint path/fallback reason;
- one replaceable demo viewport observer, no repeating diagnostics clock.

Gary validated the consolidated dependency slice Debug + Release and
`UIGRAPH_EXECUTION_PATH_SUMMARY checks=8 failed=0`.

Do not restore old Base/H2/recovery implementations from pre-consolidation history.

## ROOT CAUSE

`Painter::Ellipse(double x, double y, double rx, double ry)` takes centre/radii.

Two rich-paint call sites incorrectly passed rectangle coordinates/dimensions:
- cached node port marker;
- selected route-handle marker.

For a 9x9 port raster this placed the ellipse centre at 0.5/0.5 with radius ~8,
leaving most of the ring outside the image. The old visual test only proved that
some coloured pixels existed, so a clipped quarter-circle could pass.

## CURRENT IMPLEMENTATION

- Cached port marker now uses `Painter::Circle(centre, radius)`.
- Centre derives from the cached raster dimensions.
- Radius accounts for stroke so the complete ring fits the raster.
- Selected route handle now uses `Painter::Circle` around the prepared route midpoint.
- Semantic port anchors, hit rectangles, edge routes, spatial data and micro paint
  are unchanged.

Regression coverage now checks:
- Left / Right / Top / Bottom side ports.
- Several marker sizes (r=3,4,6) to exercise distinct raster-cache sizes.
- Normal, selected and hot presentation.
- Coloured ring coverage at left/right/top/bottom quadrants.
- Hollow port centre.
- Complete selected route-handle ring.

## WINDOWS GATE

Build/run Debug + Release:
- `UiGraphRenderTests`;
- `UiNodeGraphPresentationTest`;
- `UiGraphDemo`.

Required:
- all automated checks PASS;
- manually confirm the formerly clipped port glyph is a complete circle;
- check input/output/top/bottom marker placement at normal zoom;
- select a routed edge and confirm its midpoint handle is a complete centred circle;
- no regression in edge Circle arrow (separate feature);
- `git diff --check` PASS;
- clean worktree.

## NEXT ACTION

After this small visual checkpoint passes:
1. implement the shared prepared node-content layout/profile contract;
2. build the Design matrix from that production contract;
3. transfer activity remains a separate later slice;
4. collapse remains deferred.

Do not change execution/spatial/micro architecture for presentation work.
