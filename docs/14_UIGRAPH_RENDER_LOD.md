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

The node body path also has an overview-specific allocation guard. The canonical Ui rounded face/frame helper intentionally uses an antialiased temporary raster for rounded surfaces. Once Graph LOD has already removed active shadows, small ordinary rounded/capsule nodes (up to 48 screen pixels on their longest axis) reuse an exact cached AA face/frame raster instead of allocating one temporary `ImageBuffer` per node per frame. This fast path is deliberately narrow: image fills, enabled skins, dashed frames, active shadows and larger nodes continue through the canonical styled renderer unchanged. Authored highlights remain a separate canonical layer.

`UiNodeGraph` exposes read-only evidence for:

- prepared node/edge counts;
- paint node/edge visits;
- painted node/edge counts;
- simplified and hidden edge counts;
- geometry-preparation microseconds;
- connector-paint microseconds;
- node-paint microseconds;
- total last `Paint()` duration.

The scale evidence deliberately separates two workloads instead of conflating them:

- `Utilities/UiNodeGraphScaleTest` retains the heavier 10,000-node / 19,800-edge grid as the generic model/spatial/render stress contract and prints `UINODEGRAPH_LOW_ZOOM_PROFILE` records at zoom `0.20`;
- `UiGraphDemo` 10k mode and `Utilities/UiNodeGraphPanProfileTest` use the representative visual/pan fixture: 10,000 nodes and 9,900 horizontal row-neighbour connectors, with no redundant arrowheads. `UiNodeGraphPanProfileTest` prints `UINODEGRAPH_PAN_PROFILE` records at zoom `0.50` and `0.20`, separating geometry, edge-paint, node-paint and total timings.

The visual fixture intentionally does not add a second vertical connector from every interior node merely to inflate edge count. Input-circle/output-square markers already communicate direction at this density, so arrowheads are also omitted. This keeps the interactive 10k view focused on real retained-node/pan cost while the 19,800-edge regression continues to exercise the denser generic topology separately. Timings are diagnostic evidence, not machine-independent pass thresholds.

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

Ports are positioned on the actual retained node silhouette rather than the hidden rectangular node bounds. A single left/right Capsule port is vertically centred, Triangle ports intersect the sloping side, and the same boundary projection applies to the other built-in paths. The default marker vocabulary is input circle, output square and bidirectional diamond. Port labels are therefore opt-in by default and remain available for hosts that require named endpoints.

Capsule hit/paint geometry uses a true semicircular-ended capsule path rather than a rounded-polygon approximation.

Selection has one frame authority for built-in shapes. A selected built-in node paints the normal body once with the semantic bright-blue selected frame at the same geometry as the body; Graph no longer paints a grey pressed body frame and then a second blue approximation over it. Graph-control focus belongs to the canvas, not to a second focus outline around every unselected node. Custom shapes retain a path-overlay fallback because their body renderer is extension-owned.

Marquee chrome uses the Graph blue frame and a low-alpha light-blue wash. Spatial selection modifiers are explicit and deterministic:

- no modifier: replace;
- Shift: add;
- Ctrl: toggle;
- Alt: subtract.

## Retained custom node content

`WhenPaintNodeContent` is the lightweight retained presentation layer for thumbnails, mini-charts and similar view-owned material. It runs after the node body/ports and before Graph-owned title/subtitle text and receives the already-computed shape-safe content rectangle plus the resolved node style and visual state.

This hook deliberately does not add media semantics to `UiGraphNode` or `UiGraphModel`. The host/demo owns loaded `Image` objects or other presentation resources and maps them to node identity externally. No child `Ctrl` is required per image. The generic hook follows the existing secondary-detail LOD gate; a host may impose a stricter media threshold when full thumbnail detail would be wasteful.

`UiGraphDemo` proves the intended pattern with the published `tests/Images/Elephant.png`, `FilmNoir.png`, `sifi.png` and `Castle.png` fixtures. The demo owns all four `Image`s, aspect-fits them inside shape-safe node content, reserves a title lane, and omits thumbnails below zoom `0.55`. Its `UiGraphModel` still contains only ordinary Graph node/port/edge data.

## Demo scale presentation

The 10k demo intentionally keeps style/shape variety while dropping scale-only decorative work that obscures the retained-scene measurement:

- `raised` and `glow` still retain their authored shadow treatment, but their extra outer `StyledHighlight` rectangle is suppressed only while 10k scale mode is active;
- the reference view continues to show those highlights so the style feature remains demonstrable;
- scale connectors are row-neighbour only and have `UiGraphArrowStyle::None` because port marker shapes already communicate direction;
- no production Graph style, serializer, route contract or model topology rule is changed by these demo choices.

This distinction matters: visual benchmark cleanup belongs in the benchmark/demo fixture, not in generic Graph rendering policy.

## Shadows

Non-rectangular shadow falloff derives from the authored `StyledShadow.curve` instead of a hand-linear opacity sequence. LOD reduces the bounded layer count (full/mid/low detail) and fades/disables shadows before overview detail. The default offset is lower-right rather than straight down.

Rectangular/capsule styled shadows follow the same LOD fade/disable rule before styled background painting. Only after a shadow becomes inactive can a small simple rounded/capsule surface enter the overview raster-cache path described above.

## Request-first connector route editing

`UiGraphEdge::waypoints` remains the durable generic route data, but user editing is request-first.

A selected or hot non-custom connector exposes one ordinary midpoint handle above `route_edit_zoom`. Dragging is transient view state. Mouse-up emits `UiGraphEdgeRouteRequest` containing the edge, resolved route style, and before/after waypoint vectors. A command-driven host can set `handled` and update its own authoritative graph; Graph only calls `UiGraphModel::UpdateEdge(...)` when the request is accepted, internal mutation is enabled, and the host did not handle it.

The same simple gesture has route-specific presentation semantics:

- Bezier: the middle waypoint is a curve bias. Graph builds two C1-continuous cubic halves through that point, preserves the endpoint port tangents and avoids the backtracking/flip failure caused by translating both endpoint controls under large drags;
- Straight: the midpoint becomes a bend waypoint;
- Orthogonal: the midpoint becomes the preferred routing corridor;
- Custom routes: no built-in midpoint editor is imposed because route authority belongs to the extension.

The visible route handle remains compact, but its hit target is larger and the handle is drawn in the top interaction layer after node bodies/text so a short connector cannot hide its only grab point under a neighbouring node or port marker.

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
- `Utilities/UiNodeGraphPresentationTest`
  - real Elephant/FilmNoir/sifi/Castle fixture loading;
  - retained thumbnail painting without child controls or model media fields;
  - shape-safe aspect-fit content and low-zoom thumbnail omission;
  - Capsule/Triangle silhouette port anchoring;
  - ordinary selection and node-drag behavior for image-rich nodes;
  - practical short-edge route-handle hit geometry;
  - stable large-bias Bezier midpoint/tangent behavior;
  - repeated overview paint reuses the small rounded/capsule AA surface cache.
- `Utilities/UiNodeGraphPanProfileTest`
  - 10,000-node / 9,900 visible row-connector fixture matching the interactive scale view;
  - no redundant per-edge arrowheads in the representative pan profile;
  - retained spatial-index reuse while panning;
  - viewport/LOD-bounded prepared geometry;
  - separated preparation/edge/node timing output.
- `Utilities/UiNodeGraphScaleTest`
  - existing 10,000-node / 19,800-edge spatial/prepared/paint stress bounds and low-zoom profile evidence.

Windows interactive validation must still inspect the four retained thumbnails, shape-aware port placement, single-frame node selection, progressive text/media LOD, marquee modifiers, midpoint route gestures, connection/node-drag regressions, and dense panning at the requested zooms. In 10k mode it must also confirm there is no second vertical connector/arrow from every node and no raised/glow outer highlight rectangle. The phase timings remain evidence for whether any further 10k optimization tranche is justified; they are not fixed pass/fail speed thresholds.
