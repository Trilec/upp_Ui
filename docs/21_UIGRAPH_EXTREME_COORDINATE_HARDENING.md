# UIGRAPH-R9 — Extreme-coordinate authoring hardening

## Purpose

R9 closes the demo failure mode where the node Inspector could scrub an X/Y coordinate across a `-1,000,000 .. +1,000,000` range, immediately producing routes hundreds of thousands of world units long and making the interactive demo appear hung.

This is a bounded authoring/fixture correction before the planned node-layout-policy work. It does not change `UiGraphModel` coordinate semantics or impose a global world-size limit.

## Inspector coordinate authoring

R9 removes the dangerous million-unit X/Y slider range. Property normalization keeps Inspector-authored coordinates within one current viewport span beyond each visible edge. The bound is resolved in world space through `ScreenToWorld`, so it naturally follows the current zoom and pan rather than using a fixed arbitrary coordinate limit.

R9.1 restores the useful slider and mouse-wheel editing experience without restoring that dangerous range. X/Y use the PropertyEditor's working-range double editor: numeric entry remains authoritative, `UiFloatEdit` supplies 1-world-unit wheel stepping, and the slider range follows the current viewport plus one viewport span of overscan. The same viewport normalizer remains authoritative for preview/commit.

This is deliberately an Inspector policy, not a `UiGraphModel` restriction. Application code can still author large world coordinates directly when that is meaningful.

## 10k spatial fixture

The demo previously installed `WhenResolveEdgeStyle` only to reduce all connector widths by 14 percent. Production Graph correctly treats an arbitrary edge-style resolver conservatively because a resolver may change route-dependent presentation. In the demo that cosmetic callback therefore made every 10k connector use the global-edge fallback and prevented ordinary overview reduction.

R9 removes the cosmetic resolver. The reference and 10k fixtures now use normal Graph edge-style resolution, so the 9,900-edge interactive fixture keeps the spatially bounded path it is intended to demonstrate.

No production Graph spatial semantics are weakened to accommodate the demo.

## R9.1 interaction/performance regression recovery

Post-R9 visual testing exposed two regressions that the existing structural scale tests could not detect:

- the small Reference graph felt clunky during repeated graph wheel/zoom work;
- returning from the 10k fixture to Reference could take seconds on the Windows Debug runtime.

The R8 presentation baseline had introduced repeated `Font::FindFaceNameIndex` discovery inside the demo's per-node style resolver. R9.1 resolves the preferred sans and mono faces once per process, then reuses those names while constructing the actual fonts. The node/edge model, LOD policy and generic Graph style resolver contract are unchanged.

`SetScaleMode` also no longer performs an inactive style-transaction sync before every switch. It records `UIGRAPH_DEMO_SWITCH_PROFILE`, including total switch time and the latest Graph geometry-preparation time.

A separate `UiNodeGraphModelSwitchProfileTest` exercises 10,000 nodes / 9,900 edges against 16-node external and internal models. It reports `UINODEGRAPH_MODEL_SWITCH_PROFILE` for large-to-small, small-to-large and large-to-internal transitions. The profile deliberately has structural assertions but no machine-dependent timing threshold.

## Known boundary

The retained renderer still supports application-authored/custom routes outside the demo's local coordinate-editing policy. R9/R9.1 do not claim that every arbitrarily long dashed or dotted custom route is constant-cost to paint. If validation shows a practical remaining need, viewport clipping of manual dash subdivision should be handled as a separate production-renderer hardening task rather than hidden inside the authoring guard.

## Validation

Platform validation should use the then-current published `main` and confirm:

- `git diff --check` is clean;
- `examples/UiGraphDemo` builds in CLANGx64 Debug and Release;
- X/Y show numeric entry plus a working-range slider toggle;
- mouse wheel over the X/Y numeric editor changes the value by one world unit per editor step;
- the slider range tracks the current viewport working area rather than exposing a million-unit scrub range;
- entering an extreme value such as `-723000` is normalized near the current working viewport and does not stall the UI;
- repeated Reference -> 10k -> Reference switching remains responsive and emits `UIGRAPH_DEMO_SWITCH_PROFILE` timing evidence;
- 10k status evidence does not report all 9,900 connectors as candidates merely because of a demo edge-style callback;
- `Utilities/PropertyEditorWorkingRangeTest` passes in Debug and Release;
- `Utilities/UiNodeGraphModelSwitchProfileTest` passes in Debug and Release and retains all model-switch profile lines;
- `Utilities/UiNodeGraphPanProfileTest` passes in Debug and Release with its current zero-failure summary;
- `Utilities/UiNodeGraphScaleTest` passes in Debug and Release with its current zero-failure summary;
- panning at the existing 1.0 / 0.50 / 0.20 visual checkpoints remains coherent.

R10 is intentionally separate: simplify the node-shape vocabulary where useful and introduce retained node-layout policy/results without bringing forward unnecessary compatibility constraints.
