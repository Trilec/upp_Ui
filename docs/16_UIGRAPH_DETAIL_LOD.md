# UiNodeGraph Detail LOD — R6

## Purpose

R6 removes several remaining abrupt detail transitions in `UiNodeGraph` and makes the reference demo useful for connector editing as well as node editing. It does not change `UiGraphModel` authority or add runtime/domain semantics to Graph.

## Continuous node detail

Node titles no longer disappear merely because the rendered node crosses the old `micro` size classification. Graph continues reducing the authored font toward the existing 6 px screen-space floor, elides text that no longer fits the available lane, and blends the text toward the node face as the title/detail floor is approached.

Subtitle/detail remains more aggressive than the primary title. Rich descriptions, labels and other expensive secondary content retain their higher-detail thresholds.

## Ports

Port interaction and port presentation are deliberately separate LOD decisions.

At normal detail the built-in direction vocabulary remains:

- input: circle;
- output: square;
- bidirectional: diamond.

The ordinary marker is now one filled primitive rather than a fill plus separate outline. As zoom decreases its radius contracts and its colour blends toward the node face. The marker therefore becomes a tiny subdued point before disappearing, while connector topology can remain visible below that level.

Port hit testing still begins at `LodPolicy::port_zoom`; R6 does not make tiny overview markers interactive.

## Grid

The grid now simplifies continuously instead of disappearing as one layer.

- Minor lines fade first over roughly the 0.48 → 0.28 zoom range.
- Major lines remain longer as orientation/reference structure.
- Once minor lines are effectively invisible, `PaintGrid` iterates only major-grid spacing instead of walking invisible minor lines.
- Origin axes remain an explicit style option.

This specifically reduces visual noise and avoidable line work around the 0.30 overview range.

## Connectors

Low-detail connector width is capped at one screen pixel once the normal simplified-edge LOD is active. Connectivity remains visible longer than ports or secondary text.

Straight route editing normalizes a single waypoint that lies very close to the direct source/target segment back to a direct route. This prevents a small accidental route nick from becoming durable presentation geometry.

For the normal one-handle orthogonal editor, the midpoint now controls a stable corridor. Same-orientation endpoint pairs choose a horizontal detour or vertical bridge based on the dominant handle displacement instead of repeatedly re-deriving arbitrary elbows while dragging. Multi-waypoint authored routes retain the existing generic route builder.

The existing request-first `UiGraphEdgeRouteRequest` contract remains unchanged.

## Embedded controls and retained media

The stock node content-cell threshold is lowered to 0.42. Child controls remain bound while hidden by LOD, and their minimum allocation is scaled with Graph detail before being fitted into the already-scaled node content rectangle.

The reference demo thumbnail hook also remains useful to 0.42 and reserves a scaled title/subtitle lane before aspect-fitting the image. Media remains demo-owned retained content; no image/domain field is added to `UiGraphModel`.

## Connector Inspector

`examples/UiGraphDemo` now projects a selected `UiGraphEdge` into the same PropertyEditor Inspector used for nodes. The edge view exposes:

- ID and endpoints;
- title;
- route;
- stroke;
- arrow;
- directed;
- enabled;
- visible;
- selectable.

Edits are applied through `UiGraphModel::UpdateEdge`. The Style page remains node presentation editing, and generated C++ switches between the selected node and selected connector.

## Focused acceptance

`Utilities/UiNodeGraphDetailLodTest` covers deterministic non-visual contracts:

- a near-direct straight waypoint collapses to a direct segment;
- an intentional displaced straight waypoint remains;
- a one-handle orthogonal drag builds a stable detour corridor without adjacent duplicate points;
- an embedded child remains allocated at 0.45, shrinks from its 1:1 allocation, is hidden below the useful detail threshold, and keeps its binding.

Expected summary:

```
UINODEGRAPH_DETAIL_LOD_SUMMARY checks=12 failed=0
```

Visual acceptance must additionally verify the continuous title/port/grid behaviour because those transitions are intentionally perceptual rather than encoded as pixel-golden tests.
