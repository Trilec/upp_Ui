# Ui Shape Path and Stock Shape Layer

## Status

Source implementation and static review are complete; Windows CLANGx64
Debug/Release acceptance is required after publication.

`UiShapePath` and `UiShapes` sit above the first-class `UiGeometry`
final-device-pixel contract.

**Mandatory agent rule:** **normal controls can use `UiShapes`; dense scenes
such as Graph may go directly to `UiGeometry`.** Do not force a
`UiShapePath` allocation into a high-count path merely for architectural
uniformity.

They solve a different problem from `UiGeometry`:

- `UiGeometry` owns mathematical accuracy, distances, containment and adaptive
  explicit tessellation;
- `UiShapePath` owns authored vector-path topology;
- `UiShapes` owns reusable named/parameterised silhouettes;
- `UiDraw` owns Painter/Draw rendering and appearance.

## Layering

```
UiGeometry
    final-pixel math + 0.35px explicit-geometry contract
        |
UiShapePath
    MoveTo / LineTo / QuadraticTo / CubicTo / Arc / EllipseArc / Close
        |
UiShapes
    reusable stock silhouettes
        |
UiDraw
    Painter adapter, fill/stroke/theme/raster policy
        |
Controls
```

This layering is intentionally not mandatory in hot scene code. A control such
as UiNodeGraph may call `UiGeometry` directly when constructing thousands of
explicit paths and an intermediate authored-command object would only add
allocation/work. Reuse must never be purchased by violating the performance
contract.

## How to use this in another control

For a normal control, choose in this order:

- if `DrawRect`, `DrawLine`, `DrawEllipse`, a native Painter arc/cubic, or
  another direct primitive already expresses the paint, use it directly;
- if the control needs a reusable silhouette, start with `UiShapes`;
- if no stock silhouette fits, build a `UiShapePath` from authored commands;
- call `UiPainterShapePath()` from `UiDraw` when Painter rendering is needed;
- call `Flatten()` only when explicit points are genuinely needed for hit
  testing, clipping, routing or another geometry consumer;
- never add a per-control segment/sample/quality setting.

Example:

```cpp
UiShapePath bubble = UiShapes::Callout(
    Rectf(8, 8, 152, 72), UiShapeSide::Bottom,
    0.70, 18.0, 10.0, 8.0);

p.Begin();
UiPainterShapePath(p, bubble);
p.Fill(face);
p.End();
```

For a dense scene, skip the authored command object where it is overhead:

```cpp
Vector<Pointf> path = UiGeometry::EllipsePath(projected_rect);
// consume the explicit final-pixel path directly
```

Apply DPI and view/camera transforms before creating final-pixel geometry.

## UiShapePath contract

Coordinates are final device/screen pixels.

A path may contain multiple contours. `Close()` terminates the current
contour; a subsequent contour starts with `MoveTo()`.

Curve commands remain authored curves. `Flatten()` delegates curve detail to
`UiGeometry`; callers never choose sample counts or approximation quality.

`UiPainterShapePath()` lives in `UiDraw`, not the path layer. It keeps
circular arcs and cubics native to Painter where possible and uses
`UiGeometry` only when explicit points are required.

Appearance is not part of a shape path: no colours, gradients, stroke widths,
line joins/caps, shadows, themes, hover state, animation or raster-cache state.

## Stock vocabulary

Initial `UiShapes` coverage:

- Polygon / RoundedPolygon;
- Rectangle / RoundedRectangle / Capsule;
- Ellipse;
- arbitrary regular N-gon;
- arbitrary N-point star/burst;
- directional Arrow;
- directional Chevron;
- ChamferedRectangle;
- rounded Callout with parameterised tail;
- Tag with optional punched hole;
- Cloud;
- Document/fold silhouette;
- Database/cylinder silhouette;
- RingSegment / Pie.

The stock layer is deliberately parameterised rather than multiplying enums:
triangle/hexagon/octagon are regular polygons, and arbitrary stars do not
require new APIs.

Future silhouettes such as shields, tickets, braces, squircle-like controls,
timeline markers or specialised diagram nodes should normally be added only to
`UiShapes` (or authored directly as `UiShapePath`). They must not create a
new tessellation algorithm. A named shape is not a reason to modify
`UiGeometry`.

## Generality boundary

The general path vocabulary is the escape hatch for shapes not yet named by
`UiShapes`. New controls therefore should almost never require modifying
`UiGeometry`.

Only genuinely new geometry mathematics belongs in `UiGeometry`. A new named
silhouette does not.

Discrete authored topology is preserved. Adaptive flattening is only for
continuous curve approximation and generated detail.

Semantic positions such as handles, labels and anchors remain independent of
path vertex count, per the UiGeometry contract.

## Fill holes and winding

Multiple contours are supported. Stock shapes that contain holes use opposite
contour winding so ordinary non-zero Painter filling produces the hole without
requiring fill-rule state inside `UiShapePath`.

Directional stock-shape transforms preserve winding.

## Performance

The layer must not become a mandatory allocation tax:

- direct Draw primitives remain preferable for simple paint-only controls;
- native Painter primitives remain preferable where available;
- UiShapePath is for reusable authored topology;
- high-count scene code may use UiGeometry directly;
- no stock builder accepts a tessellation/sample-count quality knob.

The deterministic `UiShapePathTest` covers path flattening, stock topology,
multiple contours/holes and the shared Painter adapter.

## New-control review checklist

Before publishing a new control or new silhouette, confirm:

- authored dimensions are converted to final pixels exactly once;
- direct Draw/native Painter was considered before explicit geometry;
- an existing `UiShapes` builder was considered before control-local shape code;
- a broadly reusable new silhouette lives in `UiShapes`, not in one control;
- custom continuous curves use `UiShapePath`/native Painter rather than fixed
  sampling;
- explicit points, if required, obey `UiGeometry::ErrorPx()`;
- semantic handles/anchors/labels do not depend on tessellation vertex index;
- dense scenes may bypass `UiShapePath` and use `UiGeometry` directly;
- raster caching/blur/gradient policy remains outside the geometry layer.
