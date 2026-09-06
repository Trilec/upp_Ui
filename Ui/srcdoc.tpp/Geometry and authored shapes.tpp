Geometry and authored shapes

Ui drawing uses one layered geometry contract:

- UiGeometry owns final-device-pixel math and adaptive explicit geometry
- UiShapePath owns backend-neutral authored path commands
- UiShapes owns reusable parameterised stock silhouettes
- UiDraw owns Draw/Painter rendering, appearance, caching, and UiPainterShapePath

The library-wide explicit curve **positional** budget is 0.35 final device
pixels inside UiGeometry's supported numeric/work envelope. TessellationStatus
reports when a work/numeric limit prevents that tolerance. Final integer Draw
conversion, live-view projection and stroke rasterization are separate seams:
Graph preserves Pointf routes and projects each live frame from one immutable
exact baseline, while integer rounding is performed only at the final Draw or
legacy-overlay boundary. Controls do not own sample-count or curve-quality
settings.

Usage rule:
- simple paint-only primitive -> direct Draw or native Painter
- normal control needing a reusable silhouette -> UiShapes
- normal control needing a custom silhouette -> UiShapePath
- explicit points/math -> UiGeometry
- dense/high-count scenes such as UiNodeGraph may use UiGeometry directly when
  allocating a UiShapePath per item would be unnecessary work

Normal controls can use UiShapes; dense scenes such as Graph may go directly to
UiGeometry.

Apply DPI and view transforms before final-pixel geometry. Semantic positions
such as handles, labels, and anchors must not depend on tessellation vertex
indexes.

See repository docs:
- docs/07_UI_DRAWING_GUIDE.md
- docs/06_UI_SCALE_AND_LOD_GUIDE.md
