# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
This file is recovery state, not project history.

## CURRENT

Task: **UiGraph hierarchy H1 — Backdrop/Subgraph model shell, Windows acceptance pending**.

Canonical architecture: `docs/23_UIGRAPH_BACKDROP_SUBGRAPH_ARCHITECTURE.md`.

H1 implements:
- stable root/child `UiGraphScopeRef` ownership while keeping globally stable node IDs;
- true Subgraph child scopes represented by an ordinary parent `UiGraphNode`;
- authoritative Subgraph inputs/outputs mirrored to the parent group node and child `Group Inputs` / `Group Outputs` nodes;
- hard same-scope edge invariant: parent edges cannot address child endpoints;
- nested Subgraphs with cycle rejection;
- separate presentation-only `UiGraphBackdrop` collection;
- schema 3 hierarchy data appended at model level; existing `UiGraphNode` stream layout is unchanged;
- schema-2 root-only migration assigns existing nodes to root with no synthetic groups/backdrops;
- focused `Utilities/UiGraphHierarchyTest` contract test.

Published sequence:
- architecture doc: `855308d1f048c6a619ac4bc0211ef4a11b159b8c`;
- H1 API shell: `76207450cf5ea0b22766bc2058e4faac9ce0b8fc`;
- H1 model implementation: `7ad9574e49ce214e17e1403b796157bc077c4a2f`;
- U++ VectorMap index correction: `c163effee4480aca5930dc58a9131249af36e989`;
- hierarchy contract test: `060917668c5182f45cec108ceb3ae10452482e50`.

## VALIDATION

H1 has not yet been Windows compiled/run.
Gary gate: CLANGx64 Debug + Release build `Ui`, `Utilities/UiGraphHierarchyTest`, `Utilities/UiGraphTest`; run both tests with zero failures and preserve exact summaries.

Existing Graph performance remains **OPEN**:
- retained live pan is accepted structurally (`geometry_us=0` inside coverage);
- 16-node Reference improved materially but is still above desired frame budget in Debug;
- flat 10k exact geometry preparation/node paint remains far too slow and requires a later dedicated performance tranche.
Do not call Graph performance closed.

## NEXT

If H1 Windows gate is clean, implement H2 immediately:
- `UiNodeGraph` active scope + Enter/Exit navigation;
- scope-local geometry/spatial/paint/hit/selection/Fit;
- Backdrop paint layer below edges/nodes;
- preserve ordinary group-node `SetNodeCtrl()` support (`UiChartRing`, `UiProgressRing`, etc.);
- separate hierarchy demo/test; do not disturb the 16-node Reference performance fixture.

H3 later: Backdrop editing, interface editing helpers, Collapse Selected into Subgraph, Move to Parent/Ungroup.
H4 optional later: Nuke-style inline group view/peek.

## RECOVERY RULE

REFRESH -> INSPECT -> IMPLEMENT -> REVIEW -> PUBLISH -> VERIFY -> VALIDATE.
Preserve concurrent changes; complete-file review; `git diff --check`; coherent fast-forward checkpoints only.
