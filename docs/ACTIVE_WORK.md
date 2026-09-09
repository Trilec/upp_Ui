# ACTIVE WORK

BASE: `97f50981b415f1e7123a36f6f8da9c3d4f4c95ff` / main
TASK: UIGRAPH-PRESENTATION — audit node layout, port glyphs and designer-facing styling.
BRANCH: main. Consolidation fast-forwarded from performance/uigraph-execution-consolidation-20260908 at Curt's explicit request.
STATUS: MERGE COMPLETE; PRESENTATION AUDIT COMPLETE; RELEASE VALIDATION PARTIAL.
TOUCHED THIS CHECKPOINT: docs/ACTIVE_WORK.md; docs/UIGRAPH_PRESENTATION_AUDIT.md;
docs/08_UIGRAPH_GUIDE.md.
PUBLISHED: Consolidation/Gary validation at dee2cd060af281e34f9dc6f70a147fa34dfc6d26;
initial presentation audit at 97f50981b415f1e7123a36f6f8da9c3d4f4c95ff.
The documentation checkpoint is the commit containing this file.
VALIDATION: Remote ancestry, branch diff and main ref verified. Gary's tested source
12934211ded09895c5f519bf8e123dba15660b35 is unchanged by this documentation checkpoint.
NEXT ACTION: Use docs/UIGRAPH_PRESENTATION_AUDIT.md. First correct port raster coordinates
and strengthen visual tests; then agree the small shared layout/profile contract.
No presentation fixes or new APIs have been implemented during this audit.

## Presentation requirements clarified

- Normal is the authored approximately 1:1 composition; enlargement preserves it.
- Three simplification levels: LOD 1, LOD 2, LOD 3 (smallest).
- Design matrix: eight stock shapes x Normal/LOD 1/LOD 2/LOD 3; production rendering.
- Shared header/body/optional-footer regions and coordinated input/output labels.
- Rich forms need suitable shape/space; no forced identical form in every silhouette.
- Transfer indication is host-driven, bounded and LOD-aware; no idle clock or
  geometry rebuilds for motion. Implement as a separate follow-up after layout.
- Collapse is deferred for Curt's consideration.
- These are design requirements/proposals; current diagnostic bands are unchanged.
- Supervisor read order and implementation boundaries are in the presentation audit.

## Accepted evidence

Gary: E:\upp-18468\umk.exe, CLANGx64, GitHubOut.var.
Debug + Release passed: UiGraphModelTests, UiGraphViewTests, UiGraphRenderTests,
UiGraphScaleTests, UiNodeGraphPanProfileTest, UiNodeGraphPerformanceTest,
UiNodeGraphPresentationTest and UiGraphDemo build.
UIGRAPH_EXECUTION_PATH_SUMMARY: checks=8 failed=0 in both configurations.

Release baseline -> branch, same validator setup:
- Mid pan node paint: 64.426 -> 19.285 ms; paint: 79.644 -> 33.533 ms.
- Overview pan node paint: 179.974 -> 115.700 ms; paint: 191.716 -> 130.306 ms.
- L3 at zoom 0.20: micro path, fallback none, details/ports=0, content/text=0.
Curt also reports improved 10k pan and no geometry preparation during reusable pan.
These samples do not imply a universal 60 fps guarantee.

## Open presentation findings

- Quarter-circle port is still visible in Curt's latest screenshots. This supersedes
  the earlier broad port-visual PASS; compilation/execution-path results remain valid.
- PaintCachedPortMarker uses Painter::Ellipse as x/y/width/height although its
  numeric overload takes centre/radii. Route-handle paint repeats the error.
  Existing test only checks outside/inside pixel presence, not a complete ring.
- Demo images/badges and graph text use the same content area without shared layout.
- Compact mode forces centred titles; nonlinear font/icon scaling differs from body.
- Shape-safe content fractions do not guarantee rounded silhouette containment.
- Real child controls are resized/thresholded, not uniformly camera-scaled.

## Boundaries / remaining validation

Keep spatial authority, immutable live projection, micro caches and accepted style
optimisations. Eight built-in shapes and three routes remain. No GPU, new scene
graph, live C++ editor or general layout framework is required by these findings.
The recommended small layout callback and profiles are proposals only.

Full enabled/disabled/hidden status matrix, full baseline visual comparison and
manual connection/proximity sweep remain open. Proximity includes Yes/Enter, No,
Escape, Always for unambiguous non-replacement candidates, explicit replacement,
and no silent connection for incompatible/ambiguous candidates.
Main integration was explicitly authorised despite partial manual validation;
do not interpret that as completion of the presentation release gate.
