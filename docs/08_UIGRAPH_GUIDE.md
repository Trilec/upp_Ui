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

`SetNodeCtrl()` is the explicit escape hatch for real embedded controls.
Registration and activation are separate:

- a binding may remain registered while its node is offscreen/out-of-scope/LOD-suppressed;
- only controls whose nodes are prepared, visible and above the content LOD are
  attached as live child controls;
- camera/layout updates inspect the active set plus prepared nodes with
  registrations, not the entire registration map;
- `GetRegisteredNodeCtrlCount()`, `GetActiveNodeCtrlCount()` and
  `GetLastNodeCtrlCandidateCount()` expose the distinction.

Ordinary graph objects still must not become one child `Ctrl` per node.

## 5. Spatial architecture

UiNodeGraph uses one retained world-space spatial hash as its broad phase.

It supports:

- bounded visible/prepared scene queries;
- point hit candidates;
- dirty-region paint candidates;
- local marquee candidates;
- local node/edge mutation;
- style-class-local prepared rebuilds.

Exact shape/route tests happen after candidate lookup. Port and edge broad-phase
radii are derived from the same effective final-pixel hit policy used by prepared
bounds/exact tests.

### Extension bounds

Host callbacks may legally paint/hit outside stock geometry, but they must
declare conservative bounds through `UiNodeGraph::ExtensionBounds`:

- node paint/hit margins — final device pixels;
- maximum dynamically resolved port hit radius / edge hit width — final device pixels;
- edge overlay paint margin — final device pixels;
- custom-route escape margin — authored world units, because the retained broad
  phase is world-space and must survive camera changes.

Changing the declaration invalidates the spatial/prepared scene before the next
paint/hit. The declaration is a bounds contract, not a curve-quality or LOD knob.

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

### Endpoint markers

The core authored marker vocabulary is:

- None;
- Open;
- Triangle;
- Tee;
- Square;
- Circle;
- Diamond.

These are presentation choices on the edge; they do not change topology or port
semantics. Existing wire values are stable and new marker values are append-only.

Filled/hollow presentation should be treated as a separate style dimension if it
is added later rather than multiplying the shape enum into duplicate variants.

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
- geometry/node/edge/surface/details/content phase timing;
- registered/active embedded-control counts and candidate work;
- spatial cell/global/raw-edge probes and in-place bound updates;
- backdrop candidate count.

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


## 20. Execution ownership and source map

`UiNodeGraph.h` contains the actual class declarations. `UiNodeGraph.cpp` includes
internal implementation parts exactly once, without method-renaming macros. The
`.inc` suffix deliberately preserves shared helper linkage and the existing build
boundary; it does not indicate a second implementation or a runtime backend.

| Responsibility | Source / entry point |
| --- | --- |
| Lifetime, styles, notifications, attached controls | `UiNodeGraphCore.inc` |
| Exact node/edge preparation, anchors and geometry LOD | `UiNodeGraphGeometry.inc`: `PrepareViewGeometry`, `BuildViewNodeGeometry`, `BuildNodeGeometry` |
| World-space queries and scope filtering | `UiNodeGraphSpatial.cpp` |
| Programmatic camera and batched view updates | `UiNodeGraphCamera.inc` |
| Live pan/zoom projection and settle | `UiNodeGraphProjection.inc`: `ProjectLiveView`, `SettleLiveViewProjection` |
| Scope navigation, selection, fit/layout and backdrops | `UiNodeGraphHierarchy.inc` |
| Model replacement | `UiNodeGraphModelBinding.inc` |
| Frame/grid/overlay orchestration | `UiNodeGraphPaint.inc`: `Paint` |
| Backend admission and micro surfaces/edges | `UiNodeGraphPaintMicro.inc`: `PaintGraphGeometry` |
| Rich surfaces, port glyphs, details/content and edges | `UiNodeGraphPaintRich.inc`: `PaintGraphRich` |
| Shared projected-size, visibility and edge-backend policy | `UiNodeGraphLod.h` |
| Editing gestures and interaction lifecycle | `UiNodeGraphInteraction.cpp` |

H2 was a hierarchy migration label, not a second runtime spatial mode. Its accepted
scope-aware spatial implementation is now the sole `UiNodeGraphSpatial.cpp` in
`Ui.upp`. Old spatial sources and replaced methods are recoverable from Git history.
Do not restore a parallel production copy to fix a regression.

Two paint backends remain intentional. Micro drawing avoids rich per-node work;
merging their drawing loops would risk the measured 10k improvement. Shared policy
is small, inline and allocation-free. Backend admission completes before drawing,
so a Painter-only edge cannot disappear after a partial micro frame. Admitted edge
styles are reused between preflight and micro drawing.

LOD is not a fixed number of scene-wide bands. Host zoom thresholds and each node's
projected size both matter: a mixed-size scene can contain micro and rich nodes at
the same zoom. Micro nodes omit content and port glyphs even when another node or
edge requires rich scene paint. Semantic port anchors remain available. Idle and
middle-pan permit micro drawing; semantic editing gestures, rich-sized nodes,
custom painting and Painter-required edges can intentionally select rich paint.
`GetLastPaintPath()` and `GetLastPaintFallbackReason()` expose that choice directly.
Camera projection eligibility and render backend admission are separate checks.

The supported canonical built-ins remain Rectangle, Ellipse, Diamond, Triangle,
Hexagon, Cloud, Document and Database (eight), plus the Custom callback extension.
Historical shape enum values remain for compatibility. No shapes or route types
were added or removed by this consolidation; Straight, Bezier and Orthogonal remain
the three built-in connection routes. Shape identity and authored styling survive LOD.

Before changing a threshold, update the shared policy and check live projection
compatibility. Before changing a glyph, distinguish the node port glyph from the
separate edge arrow. Run the execution-path suite and 10k pan profile; a paint speed
claim requires runtime measurements, not file reduction or source inspection.

Demo viewport observation lives in `examples/UiGraphDemo/UiGraphDemoObservation.cpp`.
One `WhenViewport` callback schedules a replaceable 200 ms observer. Normal status
updates regardless of the diagnostics page/toggle; diagnostics sampling checks both
at execution time. Hiding or disabling diagnostics cannot cancel normal status.
After the observer runs, no repeating timer remains. Runtime fixture setup does not
replace this callback. Diagnostic zoom gates describe configured thresholds; actual
paint-path/fallback/port evidence describes the rendered frame.

## 21. Node presentation

The agreed designer vocabulary is **Normal, LOD 1, LOD 2, LOD 3**: Normal is
the authored composition and retains its arrangement/proportions when enlarged;
LOD 1-3 are progressively simplified, with LOD 3 smallest.
This is the target presentation contract, not a claim that today's internal
micro/rich paths or diagnostic L0-L4 bands already implement four matching modes.
Historical diagnostics and measurements retain their original labels.

See [the presentation audit](UIGRAPH_PRESENTATION_AUDIT.md) for the proposed shared
header/body/optional-footer layout, coordinated port-label lanes, eight-shape
Design matrix and bounded host-driven transfer indication.
Collapse and animation remain deferred. The layout contract below is implemented;
the earlier audit is historical design guidance.

### Prepared layout contract (UIGRAPH-PRESENTATION-LAYOUT-01)

`UiGraphNodePresentation` now owns node-content allocation. `NodeGeometry` contains
one result; the former competing content/title/control rectangles were removed.
`UiNodeGraphPresentation.inc::BuildNodePresentation` runs from exact rich node
preparation. Micro geometry keeps an empty LOD 3 result and skips the host resolver.
Compatible live projection transforms the result without running layout again.
Presentation-level or native-control activation changes reject approximate reuse.

The result contains `safe`, `header`, `title`, `subtitle`, `icon`, `badge`, `body`,
`media`, `description`, `control`, `footer`, four physical-side `port_lanes`, a
`level`, `text_align`, `profile`, `fits`, and explicit `show_*` flags. `safe`,
`header` and `body` are parent regions; leaf regions are disjoint. Hidden leaves
remain reserved. `fits=false` reports insufficient requested space; it is not a
text-width guarantee (titles may ellipsize). Empty micro results do not assess
Normal capacity. Native-control minimum-size failure also sets `fits=false`.

The eight stock silhouettes use their actual prepared outline to validate the
initial capacity estimate, with the resolved paint radius for canonical Rectangle.
Unsafe rectangles shrink conservatively. This is a bounded interior, not a maximal
packing solution. Custom shapes must honor their declared rectangular content
capacity; arbitrary host-painted silhouettes are not geometrically introspected.

`UiGraphPresentationRequest` offers three runtime profiles:

- `Standard`: authored header height and resolved text alignment.
- `Centred`: the same allocation with centred title/subtitle alignment.
- `MediaCard`: a compact authored header based on text/icon height, leaving body
  space for media. It does not move the header as zoom changes.

Requests can reserve authored `badge_height`, `footer_height` and declare
`media_min_height`. Use DPI-adjusted units at zoom 1. The optional
`WhenResolveNodePresentation(node, style, request)` supplies these values during
exact preparation. It cannot supply arbitrary rectangles or change topology.
Call `InvalidateNodePresentation()` after changing callback captures or replacing
the callback; this participates in existing batch/geometry invalidation. Do not
mutate the graph or invalidate recursively inside the resolver.

`GetNodePresentation(ref, result)` copies an already-prepared result and returns
false for unprepared nodes. It never prepares geometry, making it safe inside
`WhenPaintNodeContent`. That existing paint hook now receives the **media slot**,
not the old whole-content rectangle. Remove any independently guessed title lane.
It may also use visible badge/footer slots from the result. Graph clips the hook
to safe content and excludes stock text/header, native controls and port lanes.
Respect `show_*`; the host still owns its images/status values. Background,
foreground and overlay extension hooks retain their separate existing contracts.

Node fonts/icons and slot dimensions now scale linearly with the authored view;
font pixel rounding cannot change region ownership. Native child internals are
not camera-scaled: a child activates only at Normal, above its configured zoom
gate, and when its allocated slot meets its actual minimum size. The reserved
slot survives suppression. `GetNodeCtrlRect` reports active-eligible geometry;
use `GetNodePresentation` to inspect a hidden reservation. Painted control proxies
are a follow-up, not implemented by this change.

The initial presentation decision uses projected safe size: below 38x26 pixels
LOD 3, below DPI(80)xDPI(48) LOD 2, below DPI(160)xDPI(96) LOD 1, otherwise Normal
(either deficient dimension reduces the level). Existing configured title/content/
icon/port-label gates remain additional visibility limits. These defaults are a
starting policy, not a renaming of diagnostic L0-L4 or Micro/Rich backends.

The reference demo exercises real allocated badges/media. No Design matrix page,
animation or collapse feature is added. Build the future four-row Rectangle proof
from these prepared results and production paint, then extend it to eight shapes.
