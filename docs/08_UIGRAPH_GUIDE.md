# 08 — UiGraph Guide

This is the canonical architecture and usage guide for `UiGraphModel` and
`UiNodeGraph`.

Read with:

- `03_UI_MODEL_GUIDE.md` — model/view ownership;
- `06_UI_SCALE_AND_LOD_GUIDE.md` — large-scale view architecture;
- `07_UI_DRAWING_GUIDE.md` — final-pixel geometry and rendering.

## 1. Scope

UiGraph is a generic graph editor/view. It owns:

- graph presentation topology;
- node/port/edge editing mechanics;
- spatial/view state;
- generic hierarchy and backdrops.

It does **not** own application execution/orchestration semantics.

AgentFlow or another host remains authoritative for budgets, execution,
scheduling, retries, spawning and domain behavior.

## 2. Model and view ownership

`UiGraphModel` owns semantic graph data:

- scopes;
- nodes;
- ports;
- edges;
- backdrops;
- subgraph interfaces;
- style/data metadata that belongs to the graph document.

`UiNodeGraph` owns transient/derived view state:

- active model binding;
- active scope;
- pan/zoom;
- selection/hot/focus;
- gestures;
- retained spatial index;
- prepared projected geometry;
- LOD;
- attached visible child controls;
- profiling evidence.

The control owns an internal model by default and can bind an external model
without copying:

```cpp
graph.Model();
graph.SetModel(external);
graph.UseInternalModel();
```

## 3. Node vocabulary

Canonical authored concepts:

- Rectangle — arbitrary width/height + corner radius;
- Ellipse — circle when dimensions are equal;
- Diamond;
- Triangle;
- Hexagon;
- Cloud;
- Document;
- Database;
- Custom.

Equal rectangle dimensions represent a square. A half-height radius represents a
pill/capsule. Avoid multiplying enum variants when dimensions/metrics already
express the distinction.

Graph is a dense scene, so its hot projected geometry may use `UiGeometry`
directly rather than allocate `UiShapePath` objects per node. Reusable normal
controls should still prefer `UiShapes`.

## 4. Ports and ordinary nodes are painted, not child controls

Ordinary nodes and ports are retained geometry inside one `UiNodeGraph : Ctrl`.

Do not create one child control per ordinary graph object.

`SetNodeCtrl()` is the explicit escape hatch for a small useful set of real
embedded controls. Hidden/off-scope/LOD-suppressed controls must not remain an
unbounded attached population.

## 5. Spatial architecture

UiNodeGraph uses one retained world-space spatial hash as its broad phase.

It supports:

- bounded visible/prepared scene queries;
- point hit candidates;
- dirty-region paint candidates;
- local marquee candidates;
- local node/edge mutation;
- style-class-local prepared rebuilds.

Exact shape/route tests happen after candidate lookup.

Do not add a second prepared-viewport scan path for hit testing, and do not add a
parallel spatial tree without measured evidence.

## 6. View transactions and model changes

Composite view changes coalesce to one final exact geometry frame.

Local mutations rebuild only affected prepared geometry and incident routes where
possible. Structural/model/scope changes rebuild the structures they actually
invalidate.

Switching model authority cancels incompatible gestures and reconciles selection,
hover and attached controls so reused stable IDs cannot inherit state from a
different graph.

## 7. Rendering and LOD

Graph follows the generic scale architecture in
`06_UI_SCALE_AND_LOD_GUIDE.md`.

LOD is runtime view policy, not serialized model/style data.

The scene progressively removes work as projected detail disappears:

- rich node content at normal scale;
- secondary text/icons/shadows disappear before primary identity;
- ports and labels disappear at their own thresholds;
- connectors simplify before they disappear;
- physically micro nodes use a direct Draw scene without rich details/content;
- extreme overview may reduce ordinary connector population while preserving
  semantic topology and selected/hot context.

Geometry detail itself follows `UiGeometry` final-pixel error, not a Graph-owned
sample count.

A diamond remains a diamond and edges remain attached to the same semantic sides
through LOD transitions.

## 8. Grid

The grid is hierarchical and world-origin aligned.

As the finest grid becomes too dense it fades while coarser major levels become
the stable orientation reference. This avoids a sudden empty canvas and prevents
the grid from swimming under pan/zoom.

Grid presentation LOD does not change authored `grid_size` or snap semantics.

## 9. Live camera behavior

Public/programmatic `SetZoom`, `SetPan`, `PanBy`, Fit and host setup remain
exact.

Live interaction may reuse retained prepared geometry:

- middle-pan translates projected geometry while retained coverage is valid;
- wheel zoom projects prepared geometry about the pointer while LOD/coverage
  constraints remain compatible;
- quiet after the gesture triggers one exact settle rebuild;
- unsafe coverage/LOD boundary triggers exact fallback immediately.

The world spatial index remains authoritative.

## 10. Edge routing

Built-in route styles:

- Straight;
- Bezier;
- Orthogonal;
- Custom.

Route geometry is adaptive in final pixels.

The stock orthogonal lead is zero; a host may opt into a positive lead through
edge style when it has a concrete presentation reason.

### Editing

Route editing is request-first through `UiGraphEdgeRouteRequest`.

Straight near-direct waypoints normalize back to a direct route.

Bezier midpoint drags remain in useful port-forward half-planes so the route does
not fold back through endpoints.

Same-orientation orthogonal midpoint editing controls a stable corridor with
hysteresis around orientation changes. Mixed-orientation routes keep their
single useful elbow behavior.

### Semantic midpoint rule

The visible route handle/label midpoint is derived from visible arc length, not
a tessellation vertex index.

Adaptive flattening may reduce a straight Bezier to only its endpoints without
moving the semantic handle.

## 11. Selection and editing

Selection is semantic and independent from ordinary node frame styling.

- point selection distinguishes click from group drag;
- mouse-down on an already selected member may preserve the group for a drag;
- plain click/release may collapse to that item;
- modifier add/toggle/subtract semantics remain explicit;
- marquee preview is transient; semantic selection commits on release.

Model mutation may be internal or request-first depending on host policy.

## 12. Backdrops

A Backdrop is presentation-only same-scope organization.

It:

- belongs to one scope;
- has a world rectangle/title;
- paints behind edges/nodes;
- does not own nodes or edges;
- does not alter topology;
- does not prevent nodes crossing its bounds.

A Backdrop is never a weak Subgraph.

## 13. Subgraphs and scopes

A Subgraph is true hierarchy.

- the root graph is one scope;
- every Subgraph owns one child scope;
- a node belongs to exactly one scope;
- an ordinary edge connects endpoints inside one scope;
- child node positions are local to the child scope;
- nesting is allowed;
- scope cycles are rejected.

The parent scope represents the Subgraph through an ordinary group node.

### Interface

A Subgraph has an authoritative stable input/output interface.

Each interface port owns stable identity plus its graph-facing metadata:
title/description, data/custom type, multiplicity, enabled/visible state and
optional application `Value data`.

The outer group node mirrors that interface as normal ports.

Inside the child scope:

- **Group Inputs** exposes external inputs as internal outputs;
- **Group Outputs** accepts internal values for external outputs.

Parent edges never connect directly to child-internal nodes. Ordinary edges
remain same-scope.

Outer/interface/boundary mirror ports must not drift. Interface mutation must
preserve stable ids and reject scope cycles/self-containment.

### Navigation

Enter/Exit changes only the active view scope. It does not rewrite topology.

Scope-local viewing prepares/paints only that scope, not hidden descendants.

## 14. Presentation metadata

Application/demo-specific metadata such as small tags or image thumbnails should
use generic data/provider hooks where appropriate rather than adding domain
fields to UiGraphModel.

The Graph demo may display tags/media without making them universal graph
semantics.

## 15. Coordinate authoring

UiGraphModel does not impose an arbitrary global world-size limit.

Interactive inspectors, however, should not offer a million-unit scrub range that
can accidentally create enormous routes. The Graph demo uses a viewport-relative
working range while preserving explicit numeric entry.

This is authoring UX, not model semantics.

## 16. Performance evidence

The current deterministic large fixture is 10,000 nodes.

Graph exposes observer-only evidence including:

- candidates;
- prepared nodes/edges;
- painted nodes/edges;
- LOD population;
- path vertices;
- geometry/spatial build counts;
- geometry/node/edge/surface/details/content phase timing.

The important contracts are structural:

- live reusable camera movement does not rebuild geometry every event;
- micro nodes do not return to rich details/content;
- fit/overview stays bounded;
- hit testing uses spatial candidates;
- generated/demo diagnostics do not become part of measured interaction work.

A static viewport must eventually become idle; continuous idle repaint is a
separate defect even when individual paint paths are efficient.

## 17. Demo and generated code

The reference demo is executable documentation, not another model authority.

- Reference and 10k fixtures exercise the same production control;
- property/appearance editors act on real APIs;
- generated C++ is lazy behind explicit Code/Copy/Save actions;
- selection/code output may include multiple selected nodes/edges;
- demo-only style callbacks must not accidentally force conservative full-scene
  renderer paths.

## 18. Acceptance surface

The Graph regression family covers:

- general model/API;
- canonical shapes;
- hierarchy/scope view;
- live camera reuse;
- pan/profile behavior;
- scale/spatial behavior;
- model switching;
- route editing;
- presentation/detail/render LOD;
- selection/interaction state;
- 10k performance evidence.

When changing Graph, build the complete touched test slice rather than weakening a
single failing assertion.

## 19. Non-goals

Do not put into UiGraph:

- AgentFlow execution semantics;
- one child Ctrl per normal node;
- nested UiNodeGraph controls as the primary hierarchy mechanism;
- a second model/topology authority;
- a private curve-quality/sample-count system;
- a GPU dependency merely to compensate for avoidable CPU work.
