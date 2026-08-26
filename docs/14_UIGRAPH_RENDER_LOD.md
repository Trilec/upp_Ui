# UiNodeGraph Rendering, LOD and Route-Edit Contract

## Purpose

`UiNodeGraph` keeps semantic topology in `UiGraphModel` and owns rendering/detail and transient interaction policy in the view. Large graphs must reduce visual work as screen detail disappears instead of preserving full-size connector, shadow, text and interaction costs at every zoom.

`UiNodeGraph::LodPolicy` is runtime view policy. It is intentionally not serialized through `UiNodeGraph::Style` and does not add AgentFlow/runtime semantics to `UiGraphModel`.

## Default detail bands

The defaults are continuous thresholds rather than separate rendering modes:

- `zoom >= 0.75`: full routed connector detail and full node-detail scale;
- `0.50 <= zoom < 0.75`: routed connectors remain while width, arrow, shadow and text/icon presentation reduce progressively;
- `0.25 <= zoom < 0.50`: connectors use reduced geometry/detail and dashes are suppressed below the simplification threshold;
- `edge_hide_zoom <= zoom < 0.25`: connectors become direct endpoint segments with a shrinking screen footprint;
- below `edge_hide_zoom`: connector geometry and connector paint work are omitted.

Independent default thresholds are deliberately staggered:

- title: `0.34`;
- secondary text: `0.42`;
- edge and port labels: `0.45`;
- ports: `0.32`;
- connector route editing: `0.55`;
- shadows: `0.60`;
- icons: `0.70`;
- full detail: `0.75`.

Fonts do not remain at their authored screen size and then vanish. Below 1:1, Graph uses a compressed continuous text scale (`zoom^0.45`, with a practical lower bound); above 1:1 it grows more slowly (`sqrt(zoom)`, capped near 2x). An authored font around 11 px therefore renders around 8 px at zoom `0.5`, and the title remains useful until the low-title threshold before disappearing cleanly. Icons follow the same compressed scale while visible.

Hosts may tune `LodPolicy` for their device/workload without changing model data or serialized style state.

## Spatial and paint workload

Built-in Straight, Bezier and Orthogonal edges use route-envelope world bounds rather than the old isotropic `max(96, distance * 0.5)` inflation. Bezier bounds include control-handle reach; Orthogonal bounds include route leads and authored waypoints. Custom routes and state-sensitive edge-style resolvers retain conservative bounds because their path can legally leave the built-in envelope.

Spatial hash queries are candidate queries, but returned nodes are filtered through their retained exact world bounds. Pointer hit tests, marquee selection and dirty-paint lookup therefore do not treat every shared hash cell as exact membership.

Prepared and dirty-paint spatial queries use a screen-space margin that shrinks with LOD instead of the former fixed 160-pixel margin. Low-detail edges also use fewer Bezier samples and reduced interaction/bounds inflation. The minimum-zoom overview may reduce non-selected connector representation while preserving semantic topology and selected/hot objects.

`UiNodeGraph` exposes read-only evidence for:

- prepared node/edge counts;
- paint node/edge visits;
- painted node/edge counts;
- simplified and hidden edge counts;
- geometry-preparation microseconds;
- connector-paint microseconds;
- node-paint microseconds;
- total last `Paint()` duration.

`Utilities/UiNodeGraphScaleTest` prints `UINODEGRAPH_LOW_ZOOM_PROFILE` records at zoom `0.20`. `Utilities/UiNodeGraphPanProfileTest` builds 10,000 nodes / 19,800 connectors and prints `UINODEGRAPH_PAN_PROFILE` records at zoom `0.50` and `0.20`, separating geometry, edge-paint, node-paint and total timings. Timings are diagnostic evidence, not machine-independent pass thresholds.

## Low-detail interaction

Visual LOD and editing LOD agree:

- ports are not hittable below `port_zoom`;
- full edge hit testing is disabled below `edge_simplify_zoom`;
- connection preview is suppressed below `edge_simplify_zoom`;
- route midpoint handles are unavailable below `route_edit_zoom`;
- node hit testing remains available at low zoom.

## Node shape content and selection

U++ `Draw` child controls are rectangular while arbitrary path clipping is a `Painter` operation. Per-node offscreen masking would work against the scale objective, so built-in non-rectangular shapes use shape-safe inscribed content regions for text and attached controls.

Ellipse/circle, diamond, triangle, hexagon, capsule, cloud, document and database each reserve a useful interior. Capsule content stays clear of its semicircular ends; document content avoids the folded corner; attached controls disappear with secondary detail rather than surviving as tiny child windows.

Capsule hit/paint geometry now uses a true semicircular-ended capsule path rather than a rounded-polygon approximation.

Selection has one frame authority for built-in shapes. A selected built-in node paints the normal body once with the semantic bright-blue selected frame at the same geometry as the body; Graph no longer paints a grey pressed body frame and then a second blue approximation over it. Custom shapes retain a path-overlay fallback because their body renderer is extension-owned.

Marquee chrome uses the Graph blue frame and a low-alpha light-blue wash. Spatial selection modifiers are explicit and deterministic:

- no modifier: replace;
- Shift: add;
- Ctrl: toggle;
- Alt: subtract.

## Shadows

Non-rectangular shadow falloff derives from the authored `StyledShadow.curve` instead of a hand-linear opacity sequence. LOD reduces the bounded layer count (full/mid/low detail) and fades/disables shadows before overview detail. The default offset is lower-right rather than straight down.

Rectangular/capsule styled shadows follow the same LOD fade/disable rule before `UiPaintStyledBackground`.

## Request-first connector route editing

`UiGraphEdge::waypoints` remains the durable generic route data, but user editing is request-first.

A selected or hot non-custom connector exposes one ordinary midpoint handle above `route_edit_zoom`. Dragging is transient view state. Mouse-up emits `UiGraphEdgeRouteRequest` containing the edge, resolved route style, and before/after waypoint vectors. A command-driven host can set `handled` and update its own authoritative graph; Graph only calls `UiGraphModel::UpdateEdge(...)` when the request is accepted, internal mutation is enabled, and the host did not handle it.

The same simple gesture has route-specific presentation semantics:

- Bezier: the middle waypoint is a curve bias and the cubic is adjusted to pass through it, avoiding a visible kink and avoiding expert tangent handles;
- Straight: the midpoint becomes a bend waypoint;
- Orthogonal: the midpoint becomes the preferred routing corridor;
- Custom routes: no built-in midpoint editor is imposed because route authority belongs to the extension.

External edge mutation/removal during an active route drag cancels the stale gesture instead of committing against changed authority.

## Validation

Focused deterministic gates:

- `Utilities/UiNodeGraphRenderLodTest`
  - routed/reduced/simplified/hidden connector states;
  - low-detail hit-test shutdown;
  - LOD normalization;
  - capsule/diamond shape-safe child-control geometry.
- `Utilities/UiNodeGraphRouteEditTest`
  - default visual/edit LOD thresholds;
  - Bezier midpoint mathematics;
  - request interception without model mutation;
  - internal Bezier/Straight/Orthogonal midpoint commits;
  - route-handle LOD and phase-timing evidence.
- `Utilities/UiNodeGraphSelectionModifierTest`
  - Graph-blue marquee chrome;
  - replace/add/toggle/subtract point and marquee semantics.
- `Utilities/UiNodeGraphPanProfileTest`
  - 10,000-node / 19,800-edge fixture;
  - retained spatial-index reuse while panning;
  - viewport/LOD-bounded prepared geometry;
  - separated preparation/edge/node timing output.
- `Utilities/UiNodeGraphScaleTest`
  - existing 10,000-node spatial/prepared/paint bounds and low-zoom profile evidence.

Windows interactive validation must still inspect capsule and other shape selection/body agreement, progressive font/icon LOD, marquee modifiers, midpoint route gestures, connection/node-drag regressions, and dense panning at the requested zooms. The new phase timings should be used to identify the next 10k bottleneck before another optimization is attempted.
