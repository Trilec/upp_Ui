# 07 — Drawing and Geometry Guide

This is the canonical drawing, geometry and shape-construction architecture for
`upp_Ui`.

The goal is simple: controls should be highly themeable **and** avoid doing
drawing or geometry work that cannot affect final pixels.

## 1. Rendering hierarchy

Use the cheapest correct representation.

1. **Direct `Draw`** for cheap rectangles, lines, text, images and other simple
   primitives.
2. **Native Painter primitives/curves** for unique antialiased circles, ellipses,
   arcs, rounded rectangles and cubic/quadratic paths.
3. **Shared exact raster cache** for repeated/stable antialiased or composed
   presentation where rerasterizing each paint is wasteful.
4. **Bounded live Painter/BufferPainter** for specialist vector work, animation or
   cases that cannot reuse a stable raster.
5. Rich skins, shadows, image fills and unusual paths remain explicit fallbacks.

Do not route every control through BufferPainter merely for uniformity.

## 2. Final-pixel geometry invariant

**No generated geometry owned by `upp_Ui` should contain detail that cannot
materially affect the final device image.**

The explicit-geometry positional budget is library-owned:

**0.35 final device pixels**

That number is a **flattened-centreline positional tolerance**, not a blanket
pixel-equivalence promise. It is guaranteed only inside the supported
numeric/work envelope reported by `UiGeometry::TessellationStatus`.

Separate allowances apply at later seams:

- explicit `UiGeometry` curve flattening: <= 0.35 px positional error when
  `TessellationStatus::IsExactContract()` is true;
- integer `Draw`/legacy-overlay conversion: one final nearest-pixel rounding,
  with at most about 0.707 px Euclidean coordinate displacement;
- live Graph camera projection: always from one immutable exact prepared
  baseline, so repeated interaction does not accumulate rounding drift;
- stroke outline, joins/caps and antialiasing remain backend raster semantics;
  centreline error alone does not prove identical stroke pixels.

`UiGeometry::VisibleExtentPx()` is presentation policy: detail below that
threshold may be omitted because it is not worth representing explicitly. It is
not a mathematical statement that subpixel coverage can never affect AA pixels.

Geometry quality is therefore decided after all scale transforms:

```text
authored units
    -> DPI
    -> view/camera transform
    -> final device pixels
    -> geometry decision
```

There is no per-control tessellation-quality knob.

Do not introduce arbitrary approximations such as:

- fixed 20/40/100 point curves;
- `radius * 2` segments;
- a private `samples` or `steps` setting.

## 3. The geometry/shape stack

```text
UiGeometry
    final-pixel math + adaptive explicit geometry
        |
UiShapePath
    authored Move/Line/Quadratic/Cubic/Arc/EllipseArc/Close topology
        |
UiShapes
    reusable parameterised stock silhouettes
        |
UiDraw
    Draw/Painter rendering, appearance, raster/cache policy
        |
Controls
```

This is a responsibility stack, **not a mandatory call chain**.

### UiGeometry

Backend-independent final-pixel mathematics:

- pixel significance;
- vector/line/polyline length;
- segment/polyline distance;
- point-at-polyline-fraction;
- analytic ellipse/rounded-rect/polygon containment;
- adaptive circular/elliptic arcs;
- quadratic and cubic flattening;
- rounded polyline/polygon geometry;
- radial band/pie geometry;
- generated-polyline simplification.

Use it directly when explicit points or geometry math are genuinely required.

### UiShapePath

Backend-neutral authored path commands:

- MoveTo;
- LineTo;
- QuadraticTo;
- CubicTo;
- Arc;
- EllipseArc;
- Close;
- multiple contours/holes.

`Flatten()` delegates continuous curve detail to `UiGeometry`.

### UiShapes

Preferred reusable silhouette vocabulary for normal controls:

- Polygon / RoundedPolygon;
- Rectangle / RoundedRectangle / Capsule;
- Ellipse;
- arbitrary regular N-gon;
- arbitrary N-point Star;
- Arrow;
- Chevron;
- ChamferedRectangle;
- Callout with configurable tail;
- Tag with optional punched hole;
- Cloud;
- Document;
- Database/cylinder;
- RingSegment;
- Pie.

A new named silhouette normally belongs here, not in `UiGeometry`.

### UiDraw

Owns drawing policy and appearance:

- styled surfaces;
- Draw/Painter seams;
- exact circular-arc painting;
- raster cache;
- gradients/fills;
- shadows;
- skins;
- `UiPainterShapePath()`.

Geometry types do not own colour, theme state or cache policy.

## 4. Normal-control decision

For a normal control:

1. If direct `Draw` or native Painter already expresses the primitive, use it.
2. If a reusable silhouette exists, use `UiShapes`.
3. If no stock silhouette fits, author a `UiShapePath`.
4. Flatten only when explicit points are actually required.

Example:

```cpp
UiShapePath bubble = UiShapes::Callout(
    Rectf(8, 8, 152, 72),
    UiShapeSide::Bottom,
    0.70, 18.0, 10.0, 8.0);

p.Begin();
UiPainterShapePath(p, bubble);
p.Fill(face);
p.End();
```

## 5. Dense-scene exception

**Normal controls can use `UiShapes`; dense scenes such as Graph may go
directly to `UiGeometry`.**

Do not allocate thousands of temporary authored command objects just to force a
high-count scene through every abstraction layer.

Graph is the canonical example: its projected paths may use `UiGeometry`
directly because that preserves the same 0.35 px contract with less allocation.

This is deliberate architecture, not a special exemption from quality.

## 6. Native curves stay native

Painter already flattens its verified native curves appropriately.

If a control only needs to paint a cubic or circular arc, do not pre-flatten it
into a large polyline first. `UiPainterShapePath` forwards circular `Arc` and
cubic commands natively. The Painter API used by this package has no verified
direct authored elliptical-arc command, so `EllipseArc` is intentionally
flattened once through `UiGeometry` at the shared positional budget.

Use explicit UiGeometry points only when another consumer actually needs points,
for example:

- hit testing;
- clipping;
- routing;
- a custom backend seam;
- retained explicit scene geometry.

## 7. Semantic positions are not tessellation vertices

Adaptive flattening is allowed to change point count.

Therefore a label, edit handle, midpoint or anchor must not use
`path[path.GetCount()/2]` or another vertex-index shortcut.

Derive semantic positions from geometry:

- arc length;
- analytic intersection;
- authored parameters;
- stable semantic anchors.

`UiGeometry::PointAtPolylineFraction()` exists for visible arc-length positions.

## 8. Authored topology versus generated detail

Discrete authored vertices are semantic shape topology and are not simplified
implicitly.

Generated curve/polyline detail may be simplified only inside a declared
combined error budget. Do not flatten a curve at the full 0.35 px allowance and
then spend another independent 0.35 px simplification allowance while claiming a
0.35 px end-to-end result. `SimplifyPolyline` is for generated/sample
polylines whose simplification allowance is itself the contract.

This distinction keeps a triangle a triangle while allowing a large smooth curve
to receive more points than a 6-pixel curve.

## 9. DPI

`UiGeometry`, `UiShapePath` and `UiShapes` accept final pixel-space values.

Apply DPI exactly once and apply any view/camera transform before asking the
geometry layer to decide explicit detail.

There is intentionally no global DPI setting inside UiGeometry.

## 10. Circular controls

`UiProgressRing` and `UiChartRing` demonstrate an important choice.

Their visible rings are stroked arcs, so they keep one exact native Painter arc
through `UiPaintCircularArc` rather than converting the stroke into explicit
ring polygons.

`UiShapes::RingSegment` and `UiShapes::Pie` exist for controls that actually
need a **filled radial silhouette**. A full `UiShapes::RingSegment` uses
separate opposite-winding outer/inner closed contours, so a stroke never exposes
an artificial radial bridge. The lower-level `UiGeometry::ArcBandPath` returns
one explicit bridged polyline and is therefore a fill-oriented geometry helper;
do not use that single contour as a stroked annulus contract.

Choose the representation required by the control, not the most abstract API.

## 11. Raster work is separate from geometry work

Geometry answers **where** the shape is and how much curve detail can affect the
image.

Raster policy answers **how pixels are produced/reused**.

Audit separately:

- temporary ImageBuffers;
- blur;
- masks;
- gradients;
- immutable-image analysis;
- 9-slice composition;
- raster-cache keys and lifetime;
- animation rasters.

Do not put raster caching or colour/theme state into UiGeometry/UiShapePath.

## 12. Shared raster-cache rule

Stable repeated antialiased presentation is a strong cache candidate when:

- all raster-affecting inputs can form an exact key;
- requested size is bounded by cache policy;
- the cached image is reused enough to beat rerasterization;
- caching does not hide a dirty-region/invalidation defect.

Animation normally uses live bounded raster work unless a stable frame/result can
truthfully be reused.

## 13. Future backend direction

`upp_Ui` has no hard dependency on `upp_render` and does not require
OpenGL/Vulkan for acceptable control performance.

A future GPU backend should consume the same presentation boundaries:

- retained semantic scene;
- camera transform;
- rect/rounded rect;
- ellipse/arc/ring;
- line/polyline/path;
- text;
- image/tint;
- 9-slice;
- gradient;
- clip;
- opacity/layer.

The geometry contract remains useful regardless of backend.

## 14. New control checklist

Before publishing drawing code, verify:

- final pixels drive generated curve detail;
- direct Draw/native Painter was considered first;
- an existing `UiShapes` silhouette was considered before local shape code;
- broadly reusable silhouettes are added to `UiShapes`;
- custom continuous curves use native Painter or `UiShapePath`, not fixed
  sampling;
- explicit points obey `UiGeometry::ErrorPx()`;
- semantic positions do not depend on tessellation indexes;
- dense scenes may use `UiGeometry` directly when that avoids real overhead;
- raster/cache/blur policy remains outside geometry;
- a static control does not repaint continuously.

The current deterministic foundations are exercised by
`UiGeometryContractTest`, `UiShapePathTest` and the control/Graph regression
packages.
