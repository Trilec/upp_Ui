# UiNodeGraph route robustness — R7

## Purpose

R7 hardens the existing request-first single-midpoint route editor after visual testing exposed unstable or misleading connector states on compact and extreme drags.

The semantic contract is unchanged: `UiGraphModel` remains authoritative graph data, route edits still flow through `UiGraphEdgeRouteRequest`, and the view does not add a second routing authority.

## Changes

- The stock orthogonal lead is `0.0`. A fixed positive lead could make short facing connectors cross their own endpoint stubs. Hosts may still opt into a positive lead through `UiGraphEdgeStyle`.
- Straight midpoint drags snap back onto the direct segment when they are within the existing near-line tolerance.
- Bezier midpoint drags are constrained to the useful half-planes between facing endpoint tangents, preventing a midpoint dragged behind a port from folding the curve back through that endpoint.
- Equal-orientation orthogonal midpoint drags are canonicalized onto the corridor they control. A small hysteresis prevents route-orientation chatter near the horizontal/vertical decision boundary.
- Mixed-orientation orthogonal routes keep their unique elbow behavior; the current one-handle corridor editor does not manufacture an unstable extra degree of freedom for them.

## Regression evidence

`Utilities/UiNodeGraphRouteEditTest` now covers 23 deterministic checks, including:

- compact facing orthogonal endpoints do not backtrack;
- a canonical orthogonal midpoint lies on its route;
- request-first route editing remains intact;
- committed orthogonal midpoint geometry remains on the visible corridor;
- an extreme Bezier drag stays inside facing endpoint half-planes;
- route-edit LOD and renderer timing instrumentation remain intact.

Expected summary:

`UINODEGRAPH_ROUTE_EDIT_SUMMARY checks=23 failed=0`

Windows CLANGx64 Debug/Release validation is still required after publication.
