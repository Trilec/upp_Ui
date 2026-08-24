# UiNodeGraph Rendering LOD Contract

## Purpose

`UiNodeGraph` keeps semantic topology in `UiGraphModel` and owns rendering/detail policy in the view. Large graphs must therefore reduce visual work as screen detail disappears instead of preserving full-size connector, shadow, text and interaction costs at every zoom.

`UiNodeGraph::LodPolicy` is runtime view policy. It is intentionally not serialized through `UiNodeGraph::Style` and does not add graph/runtime semantics to `UiGraphModel`.

## Default detail bands

The defaults are continuous thresholds rather than separate rendering modes:

- `zoom >= 0.75`: full node detail, full routed connectors, labels, arrows and shadows;
- `0.50 <= zoom < 0.75`: routed connectors remain, but width/arrow/shadow detail scales down and secondary content is reduced;
- `0.25 <= zoom < 0.50`: connectors are low-detail, dashes are suppressed, Bezier sampling is reduced, arrows/text/ports are absent according to their independent thresholds;
- `edge_hide_zoom <= zoom < 0.25`: connectors become direct endpoint segments with a shrinking screen footprint;
- below `edge_hide_zoom`: connector geometry and connector paint work are omitted.

The policy exposes independent thresholds for edge labels, arrows, node titles, secondary text, shadows, ports and port labels. Hosts may tune the policy for their device/workload without changing model data or serialized style state.

## Spatial and paint workload

Built-in Straight, Bezier and Orthogonal edges use route-envelope world bounds rather than the previous isotropic `max(96, distance * 0.5)` inflation. Bezier bounds include their control handles; Orthogonal bounds include route leads and authored waypoints. Custom routes and state-sensitive edge-style resolvers retain the conservative bound because their path can legally leave the built-in envelope.

Prepared and dirty-paint spatial queries use a screen-space margin that shrinks with LOD instead of a fixed 160-pixel margin. Low-detail edges also use fewer Bezier samples and reduced interaction/bounds inflation.

`UiNodeGraph` exposes read-only evidence for:

- prepared node/edge counts;
- paint node/edge visits;
- painted node/edge counts;
- simplified and hidden edge counts;
- last `Paint()` duration in microseconds.

`Utilities/UiNodeGraphScaleTest` additionally prints `UINODEGRAPH_LOW_ZOOM_PROFILE` records at `zoom=0.20` for a static frame and a middle-button pan frame. Timing is evidence, not a machine-independent pass threshold.

## Low-detail interaction

Visual LOD and editing LOD agree:

- ports are not hittable below `port_zoom`;
- edges are not selectable/editable through full edge hit testing below `edge_simplify_zoom`;
- connection preview is suppressed below `edge_simplify_zoom`.

Node hit testing remains available at low zoom.

## Node shape content

U++ `Draw` child controls are rectangular, while arbitrary path clipping is a `Painter` operation. Per-node offscreen masking would work against the scale objective, so built-in non-rectangular shapes use a shape-safe inscribed content rectangle for text and attached controls:

- ellipse/circle, diamond, triangle, hexagon, capsule, cloud, document and database each reserve an interior that stays within the useful silhouette;
- capsule content is kept clear of rounded ends;
- document content avoids the folded corner;
- attached controls disappear with secondary detail rather than surviving as tiny child windows.

Painted node bodies, non-rectangular shadows, marquee preview and committed selection continue to use the actual retained node path.

## Shadows

Non-rectangular node shadows no longer fill one offset path copy per scaled shadow-distance pixel. Soft shadows use two bounded path layers; hard shadows use one. Shadow opacity fades between `shadow_zoom` and `full_detail_zoom` and is disabled below the threshold or for micro nodes.

Rectangular/capsule styled shadows follow the same LOD fade/disable rule before `UiPaintStyledBackground`.

## Selection

Committed node selection is a final shape-path overlay using the semantic bright-blue selection colour. The path is expanded beyond the normal frame and uses the configurable `selection_outline_width`, so circles, ellipses, diamonds, hexagons, documents and capsules no longer fall back to weak rectangular selection chrome.

## Validation

Focused deterministic gate:

- `Utilities/UiNodeGraphRenderLodTest`
  - full/routed connector state;
  - reduced-detail connector state;
  - single-segment low-detail state;
  - configurable hidden-edge state;
  - low-detail port/edge hit-test shutdown;
  - LOD policy normalization;
  - capsule and diamond attached-control safe geometry.

Scale evidence:

- `Utilities/UiNodeGraphScaleTest`
  - existing 10,000-node spatial/prepared/paint bounds;
  - static and middle-button-pan profiles at `zoom=0.20`.

Windows interactive validation must still inspect circle, ellipse, hexagon, document, diamond and capsule shadow/selection/content appearance, plus smooth panning at the reported dense/low-zoom cases.

## Deferred interaction feature

Existing `UiGraphEdge::waypoints` and `UiGraphModel::UpdateEdge(...)` provide semantic storage for route edits, but `UiNodeGraph` does not yet expose a request-first route-edit interaction contract. Midpoint/Bezier-handle editing should be implemented as a separate interaction slice with a request object analogous to node movement, not by mutating `UiGraphModel` directly from an ad-hoc paint handle.
