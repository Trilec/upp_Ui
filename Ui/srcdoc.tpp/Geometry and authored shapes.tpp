Geometry and authored shapes

Ui drawing uses one layered geometry contract:

- UiGeometry owns final-device-pixel math and adaptive explicit geometry
- UiShapePath owns backend-neutral authored path commands
- UiShapes owns reusable parameterised stock silhouettes
- UiDraw owns Draw/Painter rendering, appearance, caching, and UiPainterShapePath

The library-wide explicit curve error budget is 0.35 final device pixels.
Controls do not own sample-count or curve-quality settings.

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
- docs/24_UI_GEOMETRY_CONTRACT.md
- docs/25_UI_SHAPE_PATH.md
- docs/22_UI_RENDER_BACKEND_ROADMAP.md
