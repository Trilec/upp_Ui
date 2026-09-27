# 07 — Drawing, Geometry and Performance

Canonical rendering and scale rules for `upp_Ui`. Geometry defines where content
belongs; raster policy defines how pixels are produced/reused. Keep those concerns
separate. Graph-specific retained execution is in [Graph Development](09_UIGRAPH_DEVELOPMENT.md).

## Choose the cheapest correct representation

1. Direct Draw for cheap rectangles, straight lines, text and images.
2. Native Painter curves for smooth circles, ellipses, rounded rectangles and paths.
3. Shared exact raster caching for stable repeated AA/composed presentation.
4. Bounded live Painter/BufferPainter for changing vector content that cannot
   truthfully reuse a cached raster.
5. Explicit fallback for rich skins, shadows, image fills and unusual paths.

Do not route every control through a full-size BufferPainter for uniformity.
Conversely, painting aliased direct-Draw curves over an AA background does not
preserve smooth edges. RangeSegments uses a shared rounded content clip so even
very narrow end segments cannot leak square corners. Only its content strip and
thumbs are rasterized, not readout whitespace; unchanged strips/thumbs are cached
and live drag work bypasses position-cache pollution. Straight internal partitions
remain sharp and text stays direct Draw.

Bounds and invalidation must be correct before introducing caching. A cache cannot
repair a dirty-region bug or turn an unbounded workload into bounded work.

## Final-device-pixel geometry contract

Generated explicit geometry uses one library-owned flattened-centreline positional
budget: **0.35 final device pixels**, within the numeric/work envelope reported by
UiGeometry::TessellationStatus::IsExactContract(). This is not a blanket guarantee
of pixel-identical stroking, joins, caps, clipping or antialiasing across backends.
Integer Draw conversion makes one final nearest-pixel rounding (up to roughly
0.707 px Euclidean displacement); later raster semantics are a separate seam.

Apply authored units, DPI and camera/view transforms before deciding curve detail.
Apply DPI once. UiGeometry/UiShapePath/UiShapes accept final pixel-space values;
there is no hidden global DPI setting. VisibleExtentPx is presentation significance
policy, not a theorem that every subpixel feature has zero coverage.

No fixed 20/40/100-point circles, radius*2 subdivision, per-control sample count or
private quality slider. Native Painter curves stay native when no explicit points
are needed. Semantic labels/handles/anchors use parameters, analytic intersections
or arc length, not `vertices[count/2]`, because adaptive point counts may change.

## Responsibility stack

| Layer | Responsibility |
| --- | --- |
| UiGeometry | final-pixel math, containment, lengths/distances, adaptive explicit geometry |
| UiShapePath | authored Move/Line/Quadratic/Cubic/Arc/EllipseArc/Close topology |
| UiShapes | reusable parameterized silhouettes |
| UiDraw | Draw/Painter seam, appearance, fills, shadows, raster/cache policy |
| Control | semantic state, layout/hit policy, interaction and visible content |

This is a responsibility stack, not a mandatory call chain. Normal controls use
native primitives or stock UiShapes where appropriate. Dense Graph scenes may
use UiGeometry directly rather than allocate authored path commands per item.
Both paths obey the same final-pixel rule.

Stock silhouettes include Polygon/RoundedPolygon, Rectangle/RoundedRectangle/
Capsule/Ellipse, regular N-gons, stars, arrows, chevrons, chamfers, callouts, tags
with holes, cloud/document/database, RingSegment and Pie. Add a generally useful
silhouette to UiShapes; a genuinely private shape can be a local UiShapePath.

```cpp
UiShapePath shape = UiShapes::RoundedRectangle(Rectf(0, 0, width, height), radius);
p.Begin();
UiPainterShapePath(p, shape);
p.Fill(face);
p.End();
```

UiPainterShapePath forwards supported circular Arc and cubic commands natively.
Its authored elliptical-arc path uses UiGeometry flattening where no verified
direct Painter command is available. Flatten only for a consumer that needs
explicit points: hit testing, routing, clipping, retained geometry or a backend seam.

Authored polygon vertices are semantic topology. Do not silently simplify them.
Generated-polyline simplification needs a declared combined budget: flattening at
0.35 and independently simplifying at 0.35 is not a 0.35 end-to-end guarantee.

## Circular controls and clipping

UiProgressRing/UiChartRing use native stroked arcs through UiPaintCircularArc.
Filled wedges/donut sections use UiShapes::Pie/RingSegment. A complete RingSegment
has opposite-winding outer/inner contours so a stroke does not reveal a fake radial
bridge; ArcBandPath's single bridged contour is a fill-oriented helper.

Paint and hit testing share the appropriate prepared shape/capacity. Keep state
balanced and valid for zero/tiny rectangles, large dimensions, reversal and both
orientations. Corner clipping must cover all participating layers, not just the
first and last item. Test seams, fractional AA coverage, explicit None, alpha and
selected outlines against actual pixels where deterministic.

## Raster lifetime and cache policy

Audit temporary buffers, masks, blur, gradients, asset decoding, 9-slice composition,
cache keys and image lifetime separately from curve complexity. Stable repeated
AA work is a cache candidate only when all pixel-affecting inputs form an exact
key, size/memory are bounded and reuse beats rerasterization. Include resolved
colors/state, dimensions, radius/stroke, fill/skin/asset revisions and any other
consumed input. Do not scale exact cached edges from a quantized bucket unless
that approximation is explicitly part of the policy.

Use transparent premultiplied buffers correctly; clear newly allocated buffers.
A cache admission failure needs a bounded correct fallback, not an unbounded image
allocation under another name. Remember retained Image handles may keep memory
alive outside the cache's entry budget. Arbitrarily unique per-item images/styles
are not free merely because the cache has a size limit.

## Measurement and invalidation

GetMinSize, GetContentSize, width-aware measurement, Layout, paint and hit testing
must agree on the same geometry vocabulary. Expensive text/image preparation moves
to the narrowest appropriate invalidation seam. A small ordinary control may have
cheap measurement; high-scale views must not remeasure every record on every Paint.

Geometry changes invalidate layout and paint. Color-only changes invalidate paint.
A global theme revision refreshes inherited styles; explicit custom styles stay
explicit. No model mutation, event emission, loading or timer startup inside Paint.
Do not add a second per-item layout cache when prepared geometry can own the result.

## Large views: logical size is not live visual size

The semantic model feeds visible/overscan/spatial candidates, bounded prepared
presentation and then Paint/HitTest. A 100,000-row model must not create 100,000
controls or renderer objects. List/Gallery/Table use direct arithmetic for regular
layouts; Tree may retain a flattened visible projection; irregular Graph uses a
retained broad phase. Introduce a new spatial tree only when measured workloads
justify replacing the current strategy.

UiItemRenderData carries presentation, not universal domain semantics. UiItemRender
is a lightweight non-Ctrl renderer inside a rectangle assigned by its view. Pools
are bounded to useful visible/overscan surfaces; renderer Layout prepares data,
Paint/HitTest consume it. Tree disclosure, Table headers/editing, Menu commands,
Dropdown selection and Graph ports/routes remain with their own views.

A local record/appearance update should not rebuild uniform grid geometry or an
entire hierarchy/spatial projection. Structural changes may rebuild the structure
they invalidate. Explicit Select All, export, filtering and structural rebuild can
be O(N); ordinary scrolling, hover and hit testing must not silently become O(N).
Use bulk/ranged model notifications for a semantic batch. Do not duplicate the
model to solve a rendering problem.

Prepare expensive assets at the visible-range seam with stable keys and bounded
providers/caches. A transient actual editor is an explicit sparse escape hatch,
not one child Ctrl per ordinary logical item. Model replacement reconciles that
editor and view identity; inactive model notifications must not revive old state.

## Three distinct LOD questions

Population LOD chooses which objects need presentation. Presentation LOD chooses
which information is useful at projected size. Geometry LOD chooses explicit curve
detail at final pixels. None changes semantic topology or authored values.

Identity survives simplification: a diamond stays a diamond and a connector stays
attached to the same endpoints. Rich details/shadows/secondary text can disappear
before primary identity; a proxy is not fabricated readable content or a tiny
working editor. Thresholds and actual projected footprint are different inputs.

A camera change need not rebuild a compatible retained scene. Project from one
immutable exact baseline, not repeatedly rounded output. Unsafe coverage/capacity/
representation/LOD changes require exact fallback; quiet may trigger one exact
settle. Public SetZoom/SetPan/Fit remain exact unless explicitly documented.
Graph's named-component scale-reuse boundary remains documented separately; generic
camera principles are not proof that that path already reuses every wheel frame.

Dirty-region paint and hit testing use the same broad-phase authority. Query
intersecting candidates and then exact-test; do not add a second full-viewport scan.
For huge marquee previews, deferring expensive preview work until release may be
appropriate without changing committed semantic selection.

## Evidence, idle behavior and future backends

A static control with no animation/mutation/invalidation should settle idle.
Caret blinking and deliberate animation are exceptions with explicit lifecycle.
Before optimizing Paint, find any unwanted Refresh/timer loop. Disabled diagnostics
return before allocating strings, scanning data or scheduling another callback.

Prefer structural evidence: candidates/prepared/painted counts, layout/build serials,
renderer/active-control counts, cache hits/misses and path vertices. Timings are
machine/workload-specific evidence, not a portable FPS assertion. Record cold/warm
conditions, DPI, compiler, dataset and complete input-event cost, not only Paint.
Run only the relevant performance path for a changed subsystem.

No hard OpenGL/Vulkan/upp_render dependency is required by this library. A future
backend should consume the same retained semantic/presentation seams rather than
force controls into another model. Source simplification or fewer files is never
proof of runtime speed.
