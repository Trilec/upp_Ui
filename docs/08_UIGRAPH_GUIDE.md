# 08 — UiGraph Guide

UiGraph is a generic graph model/editor/view. It owns topology, graph editing,
view state and presentation. Application execution, scheduling, budgets, retries
and AgentFlow behavior stay with the host. Internal layout/performance and workspace
contracts are in [Graph Development](09_UIGRAPH_DEVELOPMENT.md).

## Model and view

UiGraphModel owns scopes, nodes, ports, edges, backdrops, subgraph interfaces and
graph-document metadata. UiNodeGraph owns active binding/scope, camera, selection,
gestures, spatial/projection state, retained presentation, LOD and active embedded
controls. It owns an internal model unless an external one is bound:

```cpp
UiNodeGraph graph;
graph.Model();
// graph.SetModel(external_model); // borrowed; must outlive its active binding
// graph.UseInternalModel();      // original internal data remains
```

SetModel switches without copying/merging/clearing. Scope/model changes cancel
incompatible gestures and reconcile selection/attached controls so reused IDs
cannot inherit another graph's transient state. Model notifications update the
view; mutable node/edge edits must publish through TouchNode/TouchEdge or the
appropriate mutation API. See [Models](03_UI_MODEL_GUIDE.md).

## Shapes and actual controls

Eight canonical built-ins: Rectangle, Ellipse, Diamond, Triangle, Hexagon, Cloud,
Document, Database; Custom is the callback extension. Equal Rectangle dimensions
make a square, radius/aspect make a capsule, equal Ellipse dimensions make a circle.
Historical enum values remain compatible; do not multiply shape types for sizes.

Ordinary nodes/ports are painted geometry in one UiNodeGraph, not child Ctrl trees.
SetNodeCtrl is the sparse explicit escape hatch. Registration and activation are
separate: offscreen/out-of-scope/LOD-suppressed bindings can stay registered, but
only prepared, visible, useful-size controls attach. GetRegisteredNodeCtrlCount,
GetActiveNodeCtrlCount and GetLastNodeCtrlCandidateCount distinguish those costs.
Host code owns real-control lifetime; a painted Actions component is not a tiny
live button/editor.

## Shared templates and node content

Register a validated C++ UiGraphNodeTemplate once per class and use its style class
on nodes. Do not construct templates or parse authoring JSON per node paint.

```cpp
String error;
UiGraphNodeTemplate layout;
// Configure the shared layout using UiGraphNodeTemplate's public API.
if(!graph.SetNodeTemplateClass("asset", layout, error))
    Panic(~error);
UiGraphNode node;
node.style_class = "asset";
graph.Model().AddNode(node);
```

Identified components have stable nonempty IDs, not identity derived from slot
order. Text/Icon/Image/Progress/Fields/Tags/Actions are bounded painted kinds;
repeated Text or Icon components need no Title2/Icon2 enum. Bindings resolve node
fields, node data or explicit resources. Use IDs again after reordering rather
than retaining an array index. Legacy unnamed/rich hooks remain supported within
their documented bounds but do not run as a Micro fallback.

The structure is optional Header / Body / Footer. Body has independent Content
and Overlay layers, each with Left/Main/Right regions. Overlay paints last and
never consumes Content space. Ports remain graph-owned reservations. Width, height,
placement, alignment, component style and semantic binding are separate concepts.

Normal / LOD 1 / LOD 2 / LOD 3 are author-facing inclusion levels; representation
(Text/Bar/Dot/Hidden, etc.) also depends on real capacity, data readiness and budget.
On requests a legal representation, not unreadable text or escape from the shape.
Micro/Rich execution is a separate decision. The workspace displays the actual
representation/reason alongside authored inclusion.

## Camera, grid and size

Programmatic SetZoom, SetPan, PanBy and Fit remain exact. Compatible live pan may
project retained geometry; live zoom has explicit admission and exact fallback.
Named-component scale reuse is not generally enabled yet. See the development
guide before making performance claims about that path.

The hierarchical grid is world-origin aligned: fine levels fade as coarser levels
become useful. Grid presentation never changes authored grid size or snapping.
World coordinates have no arbitrary global cap; an inspector's bounded scrub range
is user-interface policy, not a topology limit.

Changing LOD thresholds with UiRangeSegments changes presentation policy, not camera
zoom or world-space node size. Fit, 1:1 and LOD-jump camera actions are explicit.
The existing collapsed flag suppresses body content/port labels; a future size-only
disclosure must not silently reinterpret it.

## Edges and interaction

Built-in routes are Straight, Bezier and Orthogonal, with Custom as an extension.
Route detail is adaptive in final pixels. Orthogonal stock lead is zero unless the
host authors a positive lead. Endpoint markers are None, Open, Triangle, Tee,
Square, Circle and Diamond; their wire values stay stable/append-only.

Route edits use UiGraphEdgeRouteRequest. Near-direct straight waypoints normalize
to direct routes; Bezier midpoint movement respects useful port-forward half-planes;
orthogonal corridor editing retains its existing orientation/hysteresis policy.
Midpoint/label handles use visible arc length, not a tessellation vertex index.

Selection is semantic, separate from ordinary frame styling. Clicking an already
selected member can preserve the group during drag; plain release can collapse the
selection. Modifier add/toggle/subtract and marquee preview/commit are explicit.
Application-owned mutations use the request-first policy rather than mutating before
asking permission. Port glyphs, edge arrows and semantic port anchors are distinct.

## Backdrops and hierarchy

A Backdrop is same-scope presentation organization, painted behind content. It does
not own nodes/edges, change topology or prevent objects crossing its bounds.

A Subgraph owns one child scope. Each node belongs to one scope; ordinary edges
connect endpoints within that scope; nested child positions are local. Scope cycles
are rejected. The parent presents the subgraph as an ordinary group node with an
authoritative stable input/output interface mirrored as normal outer ports.

Inside, Group Inputs exposes external inputs as internal outputs; Group Outputs
accepts values for external outputs. Parent edges never connect directly to child-
internal nodes. Interface changes preserve IDs/metadata/multiplicity and reject
self-containment. Enter/Exit changes the view's active scope, not topology.

## Examples and boundaries

UiGraphDemo remains the general graph/10k reference; UiGraphHierarchyDemo teaches
scope hierarchy. UiGraphComponentStudio is the current single-preview family/node
workspace with real PropertyEditor, diagrams, structure/LOD table, files and C++.
DesignMatrix is retired, not another active editor.

Workspace files are authoring interchange, not runtime layout programs. Base layout
and appearance inherit independently into eight shapes. Explicit detach creates a
section snapshot. Inherited sections are read-only; selecting a shape alone does
not author an override. The generated C++ has no workspace/PropertyEditor dependency.

Use node data/provider hooks for application tags, thumbnails and status rather
than universal graph fields. Declare extension paint/hit bounds before painting
outside stock geometry. Read the development guide for exact size/budget, source,
file schema, projection and validation contracts. ACTIVE_WORK is the current
publication/platform boundary, not a promise that every proposed workspace feature
or optimization is already complete.
