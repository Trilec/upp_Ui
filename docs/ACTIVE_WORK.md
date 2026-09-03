# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
This file is recovery state, not project history.

## CURRENT

BASE: `c58cdf338b835540a58155759dd7474d8169f0c5`
TASK: **UiGraph hierarchy H2 — active-scope projection/navigation and Backdrop view shell**
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING**
PUBLISHED: `f126f6d40639c35b5ede101c70ed4b6521f22903` source checkpoint
VALIDATION: complete-file/source review passed; Windows CLANGx64 Debug/Release pending

Canonical architecture: `docs/23_UIGRAPH_BACKDROP_SUBGRAPH_ARCHITECTURE.md`.

H1 remains the model foundation:
- stable root/child `UiGraphScopeRef` ownership with globally stable node IDs;
- true Subgraph child scopes represented by an ordinary parent `UiGraphNode`;
- authoritative Subgraph inputs/outputs mirrored to parent group and child boundary nodes;
- hard same-scope edge invariant; nested Subgraphs with cycle rejection;
- separate presentation-only `UiGraphBackdrop` collection;
- schema 3 hierarchy data with schema-2 root-only migration;
- `Utilities/UiGraphHierarchyTest` contract coverage.

H2 now integrates:
- `UiNodeGraph` active scope, `SetScope`, Enter/Exit, path and `WhenScopeChanged`;
- scope-local Fit, selection, data selection, layout, geometry and spatial indexing;
- Backdrop paint layer fixed below edges/nodes;
- ordinary group-node `SetNodeCtrl()` support preserved;
- model switches normalize hierarchy view state to root so stale child scopes cannot resurrect;
- focused `Utilities/UiNodeGraphHierarchyViewTest` coverage;
- real package membership for the H2 source slice; `.preview/.next` staging files removed.

Recovery structure during Windows acceptance:
- `UiGraph/UiNodeGraphBase.h` is the exact pre-H2 validated public header blob;
- `UiGraph/UiNodeGraph.h` is a narrow H2 declaration wrapper around that base;
- `UiGraph/UiNodeGraphBase.inc` and `UiGraph/UiNodeGraphRender.inc` remain retained recovery slices;
- `UiGraph/UiNodeGraphSpatialH2.cpp` is the only packaged spatial translation unit;
- wrapper/recovery structure may be collapsed after the Windows gate, not before.

## SOURCE REVIEW

- package compiles one spatial implementation only;
- active-scope spatial rebuild enumerates `GetScopeNodes()` / `GetScopeEdges()`, not hidden descendants;
- Enter/Exit changes view scope only and does not rewrite model topology;
- root/child Backdrops remain model-independent presentation objects;
- selection and Fit reject/filter out-of-scope objects;
- group nodes remain ordinary Ctrl-bearing nodes;
- existing render/live-camera policy is preserved rather than rewritten;
- temporary package/header preview artifacts are removed from the checkpoint.

Supervisor environment has no Windows U++ toolchain, so no build/run claim is made here.

Existing Graph performance remains **OPEN**:
- retained live pan is structurally accepted (`geometry_us=0` inside coverage);
- 16-node Reference still requires Release acceptance evidence;
- flat 10k exact preparation/node paint remains a later dedicated performance tranche.

## NEXT — GARY COMBINED H1/H2 GATE

Build CLANGx64 Debug + Release:
- `Ui`
- `Utilities/UiGraphHierarchyTest`
- `Utilities/UiNodeGraphHierarchyViewTest`
- `Utilities/UiGraphTest`

Run both hierarchy tests and `UiGraphTest` in Debug + Release with exit 0 / zero failures.
Report exact summary lines, compiler errors if any, and a brief manual smoke check of root -> child -> parent navigation and Backdrop layering.
Gary may make only minor obvious compile/API corrections; no hierarchy/model/render architectural changes.

If the gate passes, close H2 and proceed to H3 editing ergonomics:
Backdrop selection/move/resize, explicit group Enter gesture, interface editing helpers, then Collapse Selected into Subgraph.

## RECOVERY RULE

REFRESH -> INSPECT -> IMPLEMENT -> REVIEW -> PUBLISH -> VERIFY -> VALIDATE.
Preserve concurrent changes; complete-file review; `git diff --check`; coherent fast-forward checkpoints only.
