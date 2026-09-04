# Ui Geometry Contract

## Status

This is the rendering geometry contract for all new `upp_Ui` work.

## Absolute invariant

**No `upp_Ui` code we own may generate geometric detail that cannot materially
affect the final device image.**

Geometry quality is determined in final screen pixels, not authored units and
not a control-specific zoom guess:

`authored units -> DPI -> view transform -> device pixels -> geometry decision`

The library-wide explicit-geometry error budget is **0.35 device pixels**.

## Primitive hierarchy

Use the cheapest representation that preserves the authored appearance:

1. direct `Draw` primitive when available;
2. native Painter line/circle/ellipse/arc/rounded-rect/quadratic/cubic;
3. `UiGeometry` adaptive explicit points only when points are actually required;
4. never use arbitrary approximation counts such as `20`, `40`,
   `radius * 2`, or a fixed 101-point curve.

Painter already adaptively flattens its native curves. Pre-flattening those
curves into an oversized polyline defeats that work.

## UiGeometry responsibility

`UiGeometry` is deliberately small and backend-independent. It knows only
final pixel-space geometry:

- pixel significance;
- line/vector and polyline measurement;
- line-segment/polyline distance;
- analytic ellipse/rounded-rect/polygon containment;
- circles/arcs;
- ellipses;
- radial bands and pie sectors;
- rounded rectangles and capsules;
- quadratic Beziers;
- cubic Beziers;
- rounded polylines and rounded polygons;
- generated-polyline simplification.

Rounded rectangles, capsules, pills, rounded polygons, route corners, rings,
gauges, knobs, handles, wave/envelope editors, graphs and future diagram
controls are compositions of those primitives.

Discrete authored vertices are not automatically simplified. Geometry is also
independent of colour, theme, state, raster cache, Ctrl ownership and render
backend.

Semantic positions such as route midpoints, labels, edit handles and anchors
must be derived from geometry (for example visible arc length or analytic
intersection), never from a tessellation vertex index. Adaptive flattening is
allowed to change point count without changing authored interaction semantics.

## Lines

A straight line has no tessellation requirement. Use `DrawLine` or a native
Painter line.

Dashed/dotted lines may repeat visible dash primitives, but must not introduce
sub-pixel pattern work that cannot affect the image. This is a drawing-policy
concern, not a reason to tessellate the line itself.

## DPI

`UiGeometry` has no DPI setting by design. Callers pass final pixel-space
values after DPI and any canvas transform. This makes the same contract correct
at 100/150/200% desktop scale and under arbitrary canvas zoom.

## Raster work is a separate contract

Pixel-area costs are audited separately: temporary ImageBuffers, blur,
gradients, masks and immutable-image analysis. Do not mix those concerns into
geometry tessellation.

## Migration gate

The repository is not geometry-contract complete while production drawing code
contains fixed or radius-proportional approximation loops.

Known migration targets at contract introduction:

- shared styled-cap arc sampling;
- classic UiTab active outline;
- UiBezierCurveEditor fixed 101-point curve;
- UiNodeGraph fixed ellipse/port/rounded-polygon paths;
- UiNodeGraph fixed-sample Bezier and rounded orthogonal route geometry.

Native Painter/Draw primitives that are already device-space adaptive are
compliant and should not be rewritten merely for uniformity.

## Acceptance

Deterministic tests must prove:

- larger projected curves receive more geometry than tiny curves;
- circular sagitta remains within 0.35 px;
- quadratic/cubic flattening becomes denser only as projected curvature grows;
- generated polyline simplification preserves endpoints and stays inside the
  same error budget;
- no control-specific quality knob changes this contract.

Performance benchmarks remain informational by machine; geometric error and
structural work counts are deterministic.


## Repository audit — 2026-09-04

The geometry-contract migration audited the production `Ui/` drawing code for
manual curve sampling, trig-generated approximation loops and fixed point
counts.

Migrated to the shared contract or native primitives:

- shared styled-cap rounding;
- classic `UiTab` active outlines;
- `UiBezierCurveEditor` fixed 101-point curve -> native Painter cubic;
- UiNodeGraph stock ellipse/capsule/rounded polygon silhouettes;
- UiNodeGraph port and route-handle circles -> native Painter ellipses;
- UiNodeGraph rounded headers;
- UiNodeGraph Bezier routes -> adaptive final-pixel cubic flattening;
- UiNodeGraph rounded orthogonal routes -> adaptive rounded polylines;
- UiNodeGraph runtime hit testing now reuses prepared silhouettes.

Audited as already compliant and intentionally unchanged:

- ordinary `DrawRect`, `DrawLine`, `DrawEllipse` primitives;
- native Painter circles, ellipses, arcs, rounded rectangles and cubics;
- colour-picker polar coordinate calculations (authored positions, not
  approximation sampling);
- discrete arrow/triangle/diamond vertices;
- grid/dash repetition where spacing itself is authored visible geometry.

The next separate efficiency audit is raster work: blur, masks, gradients,
temporary ImageBuffers, immutable-image analysis and render-layer allocation.
Those are not geometry tessellation and remain outside this contract.
