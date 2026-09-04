# UiNodeGraph Overview Connector LOD

## Purpose

`UIGRAPH-LOD-R3` addresses the remaining minimum-zoom scale blocker measured after the first rendering LOD pass.

At `zoom=0.20`, the 10,000-node scale fixture was already simplifying each connector to a direct segment, but still retained and painted roughly 7,175 connectors per frame. Release paint time remained roughly 0.78–0.83 seconds per frame. The remaining cost was therefore connector population, not route complexity.

## Contract

Below `LodPolicy::minimal_edge_zoom`, a large viewport may prepare a connector overview instead of one `EdgeGeometry` record for every spatially relevant semantic edge.

The overview does **not** mutate `UiGraphModel`. All nodes, ports, edges, identities and adjacency remain authoritative and unchanged in the model.

Overview reduction is applied only while rebuilding retained geometry. Ordinary paint lookup, node/port/edge hit testing and marquee interaction still receive exact spatial-query results; they simply find paintable edge geometry only for the representatives retained by the overview rebuild.

Population LOD and curve tessellation are separate concerns. Overview reduction
decides **which semantic representatives are prepared**; `UiGeometry` decides
**how much explicit projected curve detail can affect final pixels**. No overview
stage owns a fixed Bezier/arc sample count. Because Graph is a dense scene, it
may use `UiGeometry` directly instead of allocating `UiShapePath` objects per
prepared node/edge. See `24_UI_GEOMETRY_CONTRACT.md` and
`25_UI_SHAPE_PATH.md`.

## Representative density

The minimum-zoom overview uses world-anchored bins corresponding to approximately 64 screen pixels at the current zoom. Ordinary edges are grouped by:

- midpoint bin;
- one of eight route-direction buckets;
- edge style class;
- enabled state;
- directed state.

The lowest edge ID in each bin is the stable representative. World anchoring plus deterministic representative selection avoids random frame-to-frame sampling and limits representative churn while panning.

The overview is enabled only when more than 512 spatial edge candidates are present. Smaller graphs keep their complete connector set.

## Context preservation

The overview always retains:

- selected edges;
- the hot edge;
- edges incident to selected nodes;
- edges whose source or target cannot be resolved safely.

At or above `minimal_edge_zoom`, detailed connector preparation returns normally.

## Extension-point safety

If `WhenResolveEdgeStyle` is installed, overview sampling is disabled. A state-sensitive resolver can legally make edge presentation depend on information not represented by the ordinary grouping key, so generic Graph must preserve the complete candidate set rather than silently merge those semantics.

Custom and other conservatively indexed global edges retain the existing global-query behavior; R3 does not narrow that extension-point safety contract.

## Validation

`Utilities/UiNodeGraphOverviewLodTest` uses a deterministic 60 x 60 grid (3,600 nodes / 7,080 semantic edges) and checks:

- detailed connector preparation at the threshold;
- a non-empty, bounded overview population at zoom 0.20;
- more than threefold prepared-edge reduction;
- unchanged semantic edge count;
- paint does not rebuild geometry;
- painted edge work is bounded by the overview population;
- middle-pan reuses the spatial index and remains bounded;
- selected-node context remains represented;
- detailed connectors return above the overview band;
- a dynamic edge-style resolver disables sampling.

The existing `UiNodeGraphScaleTest` remains the production-scale measurement gate and reports real static and middle-pan `paint_us` values. Timing remains diagnostic rather than a hardware-independent pass threshold.

## Acceptance target

Windows CLANGx64 Debug and Release must show a substantial reduction from the previous zoom-0.20 baseline of approximately 8,300 prepared / 7,175 painted edges and roughly 0.78–0.83 seconds Release paint time, while the overview remains visually stable and detailed connectors return when zooming back in.
