# 23 — UiGraph Backdrop and Subgraph Architecture

## Purpose

Define the generic UiGraph architecture for two intentionally separate concepts:

1. **Backdrop** — same-scope visual organization only.
2. **Subgraph** — true hierarchical graph containment with an explicit external interface.

This document is the implementation contract for UiGraph. It deliberately contains no AgentFlow execution semantics.

Remote `main` is authoritative. Preserve existing Graph performance, rendering, model/view and request-first editing contracts while adding this hierarchy.

---

## 1. Domain boundary

`UiGraph` owns presentation topology and graph editing mechanics only.

Applications such as AgentFlow remain authoritative for what a group means: execution, budgets, spawning, iteration, capabilities, failure policy, scheduling and orchestration remain outside UiGraph.

UiGraph may project application metadata through existing `Value data` fields, but it must not duplicate application semantic models.

---

## 2. Backdrop and Subgraph are different objects

### 2.1 Backdrop

A Backdrop is analogous to a Nuke Backdrop or Houdini Network Box.

It:
- belongs to exactly one graph scope;
- has a world-space rectangle and title;
- is presentation-only;
- does not own nodes or edges;
- does not alter graph topology;
- does not prevent nodes moving across its bounds;
- may be moved/resized independently;
- always paints behind nodes, group nodes and edges;
- may use a light themed fill and dashed/dotted frame;
- is useful for visual regions such as `Planning`, `Evaluation`, `Scene Builder`, etc.

Render order is fixed conceptually:

```
canvas/background
    -> backdrops
    -> edges
    -> nodes / group nodes
    -> selection / route / interaction overlays
```

A Backdrop is not a weak form of Subgraph and must never acquire hidden containment semantics.

### 2.2 Subgraph

A Subgraph is analogous to a Houdini subnet, Nuke Group or Blender node group.

It:
- owns a child graph scope;
- is represented in its parent scope by an ordinary UiGraph node;
- has an explicit input/output interface;
- contains nodes and edges local to that child scope;
- may contain nested subgraphs;
- cannot contain itself directly or indirectly;
- never exposes direct parent edges to internal child nodes;
- preserves child-local node positions when the outer group node moves;
- can be entered/exited without changing semantic topology.

This is true hierarchy, not a large rectangle around root-graph nodes.

---

## 3. Graph scopes

UiGraphModel gains explicit graph scopes.

The root graph is one scope. Every Subgraph owns one child scope.

Conceptual model:

```
UiGraphModel
  root scope
    nodes
    edges
    backdrops
    group node A -> child scope A
                    nodes
                    edges
                    backdrops
                    group node B -> child scope B
```

A node belongs to exactly one scope.
An ordinary edge connects endpoints in exactly one scope.
A Backdrop belongs to exactly one scope.

Child node positions are local to their own scope. Moving the outer group node therefore does not rewrite child coordinates.

This is important for correctness, nesting and large-scene performance.

---

## 4. Subgraph interface

A Subgraph has a first-class authoritative interface.

The interface defines stable external sockets, independent of the physical boundary-node presentation used inside the child scope.

Conceptually:

```
UiGraphSubgraphInterface
  inputs[]
  outputs[]
```

Each interface port owns:
- stable id;
- title / description;
- data type / custom type;
- multiplicity;
- enabled/visible state;
- optional application `Value data`.

The outer group node mirrors the authoritative interface as normal UiGraph ports.

Inside the child scope, two special generic boundary nodes mirror the same interface:

```
[ Group Inputs ] -> internal graph -> [ Group Outputs ]
```

For an external input socket, the internal Group Inputs node provides an output port.
For an external output socket, the internal Group Outputs node accepts an input port.

The interface is authoritative; outer and inner mirror ports must be synchronized by UiGraphModel APIs and must not drift independently.

---

## 5. Edge invariant

Parent edges never connect directly to child nodes.

Valid parent topology:

```
Source -> SceneWorkshop.in
SceneWorkshop.out -> Publish
```

Valid child topology:

```
GroupInputs.in -> Gather -> Dialogue -> GroupOutputs.out
```

Invalid topology:

```
parent Source -> child Gather
child Dialogue -> parent Publish
```

This invariant removes dynamic proxy rewiring from collapse/expand and makes nested graphs easier to validate and optimize.

Entering, exiting or collapsing a Subgraph does not rewrite edges.

---

## 6. Collapse Selected into Subgraph

A future editing command may convert selected same-scope nodes into a Subgraph.

Algorithm contract:

1. all selected nodes must belong to the same scope;
2. create an outer group node and child scope;
3. move selected nodes/fully internal edges into that child scope while preserving their relative world layout as child-local layout;
4. inspect cut edges crossing the selection boundary;
5. create interface inputs/outputs for those cut edges;
6. create/mirror Group Inputs and Group Outputs ports;
7. reconnect parent edges to the outer interface;
8. reconnect child edges through the internal boundary nodes;
9. preserve stable application identity where possible;
10. perform as one coherent model transaction.

This command is not required for the first shell checkpoint; the architecture must make it possible without later model surgery.

---

## 7. Navigation and editing scope

Primary V1 editing uses one UiNodeGraph canvas at a time.

Double-click / explicit Enter on a group node enters its child scope.
Exit returns to the parent scope.

UiNodeGraph should expose generic navigation APIs/events such as:

```
SetScope(...)
GetScope()
EnterSubgraph(...)
ExitScope()
CanExitScope()
GetScopePath(...)
WhenScopeChanged
```

Breadcrumb presentation remains application/Workbench territory; existing `UiBreadcrumbs` can consume the path.

Applications may later open a child scope in another tab/window. UiGraph must not force that policy.

Nested independent scrollable canvases inside nodes are deliberately rejected as the primary design because they complicate input ownership, clipping, drag/drop and rendering cost.

A future Nuke-style inline Group View/peek may be added as an alternate presentation of the same scope model, not as a separate topology model.

---

## 8. Group node presentation and child controls

The parent representation of a Subgraph is an ordinary UiGraph node.

Therefore all existing node presentation mechanics remain available:
- style classes;
- selection;
- canonical silhouettes;
- ports;
- request-first interaction;
- retained node content;
- attached child Ctrl through `SetNodeCtrl()`.

This means applications can attach `UiChartRing`, `UiProgressRing`, `UiToggle`, `UiButton` or another suitable Ctrl to a group node without adding domain-specific drawing to UiGraph.

Example use:

```
Scene Workshop
12 agents · iterative      [UiChartRing]
```

The child scope knows nothing about that control. The application updates it from authoritative application state.

---

## 9. Backdrop model

Backdrop shell should be a separate first-class collection, not a node subtype.

Conceptual fields:

```
UiGraphBackdropRef
UiGraphBackdrop
  ref
  scope
  title
  style_class
  position
  size
  z_order        // only relative to other backdrops
  visible
  selectable
  movable
  resizable
  Value data
```

All Backdrops paint below edges/nodes regardless of their numeric z-order. Backdrop z-order only orders Backdrops against each other.

V1 backdrop interaction may be limited to model/API + painting; resize handles/drag commands can follow without changing the model.

---

## 10. Scope-local performance model

Hierarchy is also a scene partition.

UiNodeGraph must prepare, spatially query, hit-test and paint only the active scope.

If root contains 100 group nodes whose child scopes collectively contain 10,000 nodes, root view work should be approximately the 100 visible parent objects, not the hidden 10,000 descendants.

Entering one group then operates on that child scope only.

This does not replace the flat 10k benchmark: flat large graphs must still be optimized. It prevents hierarchical applications from paying flat-global costs unnecessarily.

The existing world-space spatial index contract should evolve toward scope-local buckets/index state rather than filtering a permanently global 10k candidate set after lookup.

---

## 11. Serialization and compatibility

Do not change the existing serialized `UiGraphNode` record layout merely to add hierarchy.

Use model-level schema evolution.

Existing schema-2 models load into:
- root scope;
- all existing nodes/edges assigned to root;
- no subgraphs;
- no backdrops.

New hierarchy/backdrop/scope/interface collections are appended at the UiGraphModel schema layer.

This avoids corrupting old node/edge streams and preserves historical shape-wire migration.

---

## 12. Initial public concepts

Names may be refined during source implementation, but the separation is fixed.

Likely generic concepts:

```
UiGraphScopeRef
UiGraphBackdropRef
UiGraphBackdrop
UiGraphSubgraph
UiGraphSubgraphPort
```

Model operations should cover at least:

```
GetRootScope()
GetNodeScope(node)
GetParentScope(scope)
GetOwningGroupNode(scope)
GetScopeNodes(scope)
GetScopeEdges(scope)

AddBackdrop / UpdateBackdrop / RemoveBackdrop
GetScopeBackdrops

CreateSubgraph(parent_scope, group_node)
RemoveSubgraph(...)
GetChildScope(group_node)
IsSubgraphNode(group_node)

AddSubgraphInput / AddSubgraphOutput
UpdateSubgraphPort / RemoveSubgraphPort
GetSubgraphInterface
GetGroupInputNode / GetGroupOutputNode
```

Mutation APIs must enforce the same-scope edge invariant and prevent scope cycles.

---

## 13. Initial implementation checkpoints

### H1 — architecture shell

Implement and Windows-validate:
- scope refs/root scope;
- model-level node-to-scope ownership without changing node record layout;
- Subgraph metadata and child-scope ownership;
- authoritative interface + generated/mirrored Group Inputs/Outputs nodes;
- Backdrop model collection;
- schema migration from existing root-only graphs;
- validation rules;
- focused deterministic model tests.

### H2 — UiNodeGraph scope projection

Implement:
- active scope;
- scope-local geometry/spatial/paint/hit/selection;
- Enter/Exit navigation;
- scope-aware Fit;
- Backdrop paint layer below edges/nodes;
- group node remains ordinary Ctrl-bearing UiGraph node;
- focused view tests.

### H3 — editing ergonomics

Implement:
- group-node Enter/double-click request;
- Backdrop selection/move/resize;
- interface editing helpers;
- Collapse Selected into Subgraph transaction;
- optional Move to Parent/Ungroup mechanics.

### H4 — optional presentation

Only after V1 is stable:
- inline Group View/peek;
- richer backdrop fold/presentation;
- additional navigation/tab integration.

---

## 14. Non-goals

Do not add to UiGraph:
- AgentFlow budgets;
- orchestration;
- execution state machines;
- spawn policy;
- iteration policy;
- retries/failure policy;
- application breadcrumbs;
- one child Ctrl per ordinary internal node;
- nested scrollable UiNodeGraph controls as the primary hierarchy model.

---

## 15. Acceptance principles

The feature is only correct when:

1. Backdrops have no topology authority.
2. Nodes cannot geometrically escape a Subgraph because they live in a different scope.
3. Parent edges cannot address child endpoints.
4. Interface port identity is stable and authoritative.
5. Outer and boundary mirror ports cannot drift.
6. Nested Subgraphs cannot form cycles.
7. Child coordinates survive parent-group movement unchanged.
8. Enter/Exit changes view scope, not topology.
9. Existing root-only graphs continue to load and behave unchanged.
10. Scope-local viewing does not prepare/paint hidden descendant scopes.
11. Group nodes retain ordinary `SetNodeCtrl()` capability, including `UiChartRing`/`UiProgressRing`.
12. No AgentFlow semantic state is duplicated in UiGraph.

---

## Recovery rule

`REFRESH -> INSPECT -> IMPLEMENT -> REVIEW -> PUBLISH -> VERIFY -> VALIDATE`

Preserve concurrent work. Remote `main` is authoritative. Publish coherent fast-forward checkpoints only. Gary performs Windows acceptance; architectural fixes return to the supervisor.
