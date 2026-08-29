# UIGRAPH-R9 — Extreme-coordinate authoring hardening

## Purpose

R9 closes the demo failure mode where the node Inspector could scrub an X/Y coordinate across a `-1,000,000 .. +1,000,000` range, immediately producing routes hundreds of thousands of world units long and making the interactive demo appear hung.

This is a bounded authoring/fixture correction before the planned node-layout-policy work. It does not change `UiGraphModel` coordinate semantics or impose a global world-size limit.

## Inspector coordinate authoring

`UiGraphDemo` no longer presents X/Y as million-unit `NumericDouble` ranges.

X and Y use ordinary numeric editing. Property normalization keeps Inspector-authored coordinates within one current viewport span beyond each visible edge. The bound is resolved in world space through `ScreenToWorld`, so it naturally follows the current zoom and pan rather than using a fixed arbitrary coordinate limit.

This is deliberately an Inspector policy, not a `UiGraphModel` restriction. Application code can still author large world coordinates directly when that is meaningful.

## 10k spatial fixture

The demo previously installed `WhenResolveEdgeStyle` only to reduce all connector widths by 14 percent. Production Graph correctly treats an arbitrary edge-style resolver conservatively because a resolver may change route-dependent presentation. In the demo that cosmetic callback therefore made every 10k connector use the global-edge fallback and prevented ordinary overview reduction.

R9 removes the cosmetic resolver. The reference and 10k fixtures now use normal Graph edge-style resolution, so the 9,900-edge interactive fixture keeps the spatially bounded path it is intended to demonstrate.

No production Graph spatial semantics are weakened to accommodate the demo.

## Known boundary

The retained renderer still supports application-authored/custom routes outside the demo's local coordinate-editing policy. R9 does not claim that every arbitrarily long dashed or dotted custom route is constant-cost to paint. If validation shows a practical remaining need, viewport clipping of manual dash subdivision should be handled as a separate production-renderer hardening task rather than hidden inside the R9 authoring guard.

## Validation

Platform validation should use the then-current published `main` and confirm:

- `git diff --check` is clean;
- `examples/UiGraphDemo` builds in CLANGx64 Debug and Release;
- Reference mode X/Y are ordinary numeric fields rather than million-unit sliders;
- entering an extreme value such as `-723000` is normalized near the current working viewport and does not stall the UI;
- repeated Reference -> 10k -> Reference switching remains responsive;
- 10k status evidence no longer reports all 9,900 connectors as candidates merely because of a demo edge-style callback;
- `Utilities/UiNodeGraphPanProfileTest` passes in Debug and Release with its current zero-failure summary;
- `Utilities/UiNodeGraphScaleTest` passes in Debug and Release with its current zero-failure summary;
- panning at the existing 1.0 / 0.50 / 0.20 visual checkpoints remains coherent.

R10 is intentionally separate: simplify the node-shape vocabulary where useful and introduce retained node-layout policy/results without bringing forward unnecessary compatibility constraints.
