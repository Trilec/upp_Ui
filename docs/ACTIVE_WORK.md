# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `cd587fa51a8323b84820c8e041e9f819cba32a09`
TASK: **UIGRAPH-VIS-EDGE-01 — connector visual correctness**
TOUCHED:
- `Ui/UiGraph/UiNodeGraphBase.inc`
- `Ui/UiGraph/UiNodeGraphRender.inc`
- `Utilities/UiGraphRenderTests/Presentation.cpp`
- `Utilities/UiNodeGraphPresentationTest/main.cpp`
- `docs/ACTIVE_WORK.md`
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING**
PUBLISHED: supervisor checkpoint pending squash/merge to `main`
VALIDATION: source/diff review + mirrored render regressions added; Windows Debug/Release pending.

## PREVIOUS CHECKPOINT

`cd587fa51a8323b84820c8e041e9f819cba32a09`
- split 10k projected-micro style preparation into resolver vs metric scaling;
- UiGraphDemo caches deterministic scale preset resolution by role + preset;
- generic per-node `WhenResolveNodeStyle` semantics remain unchanged;
- platform performance validation still required.

## ROOT CAUSES

1. Circle edge marker was centred on the route endpoint, which is the node boundary.
   Nodes paint after edges, so the inward half of the marker was covered by the node.

2. Detailed edges used two paint backends:
   - None/Open arrows often used integer `DrawLine`;
   - Circle/Diamond/Triangle forced antialiased `Painter`.
   This produced different apparent thickness/aliasing for otherwise equivalent connectors.

## CURRENT IMPLEMENTATION

- Circle marker centre is shifted backward by its radius so its forward edge is tangent
  to the target endpoint and the complete marker stays outside the node body.
- Non-simplified connectors at/above `edge_simplify_zoom` now share the antialiased
  Painter path with retained sub-pixel coordinates.
- Direct integer Draw remains the overview/simplified path for scale performance.
- Mirrored presentation tests cover:
  - visible circle extent behind the target tangent;
  - antialiased detailed ordinary connector rendering.

## CONTRACTS PRESERVED

- route construction/tessellation is unchanged;
- semantic port anchors are unchanged;
- edge bounds already conservatively include arrow extent;
- low-detail/simplified 10k connector path remains direct and cheap;
- no model or interaction semantics changed.

## WINDOWS VALIDATION NEEDED

Build/run Debug + Release:
- `UiGraphRenderTests`;
- `UiNodeGraphPresentationTest`;
- `UiGraphDemo`.

Manual UiGraphDemo:
- compare Straight / Bezier / Orthogonal detailed connectors at ~0.8–1.0 zoom;
- verify None/Open/Circle/Diamond/Triangle edges have consistent line quality;
- verify Circle marker is complete, correctly sized, and tangent to the node/port;
- verify overview 10k remains responsive and uses the existing simplified/direct path.

## NEXT ACTION

After this checkpoint:
1. Undo/Redo command history;
2. proximity auto-connect integrated into the same command path;
3. left authoring palette.

Do not begin authoring-palette work before Undo/Redo and auto-connect semantics are stable.
