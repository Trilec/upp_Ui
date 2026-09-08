# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `c9375e6dcf141058e23347124ce624bc15b49aa8`
TASK: **UIGRAPH-PERF-STYLE-01 — remove measured 10k style-preparation bottleneck**
TOUCHED:
- `Ui/UiGraph/UiNodeGraphBase.h`
- `Ui/UiGraph/UiNodeGraphPerformance.inc`
- `Utilities/UiGraphScaleTests/Performance.cpp`
- `Utilities/UiNodeGraphPerformanceTest/main.cpp`
- `examples/UiGraphDemo/UiGraphDemo.h`
- `examples/UiGraphDemo/UiGraphDemo.cpp`
- `docs/ACTIVE_WORK.md`
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING**
PUBLISHED: supervisor checkpoint pending squash/merge to `main`
VALIDATION: source/diff review + deterministic regression coverage added; Windows Debug/Release pending.

## MEASURED CAUSE

Validated live profiling now accumulates correctly at `c9375e6...`.
10k L3 captures showed node preparation dominated by style work:
- nodes ~622 ms, style ~619 ms in one capture;
- nodes ~483 ms, style ~481 ms in another;
- silhouette/anchors were ~1–2 ms combined.

This satisfies the prior profiling gate. Do not reopen spatial/path/raster work without new evidence.

## CURRENT IMPLEMENTATION

- exact projected-micro preparation now splits `style` evidence into:
  - resolver cost;
  - metric-scaling cost;
- the aggregate style counter is exactly resolver + scale;
- UiGraphDemo reports both subphases in Live profiling;
- the 10k demo resolver now caches deterministic resolved preset styles by
  role + preset instead of rebuilding palette/font/preset state for every node;
- custom per-node styles and selected-state preview remain uncached and preserve
  existing semantics;
- theme toggle clears the demo cache;
- generic `WhenResolveNodeStyle` semantics remain per-node and unchanged.

## CONTRACTS PRESERVED

- no callback-result cache was added to generic UiNodeGraph;
- semantic model remains authoritative;
- projected-micro silhouette/path/raster contracts are unchanged;
- diagnostics remain observer-only and settle idle;
- exact/programmatic camera operations remain synchronous.

## WINDOWS VALIDATION NEEDED

Build/run:
- `UiGraphScaleTests` Debug + Release;
- `UiNodeGraphPerformanceTest` Debug + Release;
- `UiGraphDemo` Debug + Release.

Record at 10k L3:
- total node prep;
- style / resolve / scale;
- silhouette / anchors;
- paint / node surface;
- micro_rasters / cached_draws / direct_fallbacks;
- idle behavior.

Acceptance:
- all builds/tests PASS;
- resolver+scale equals aggregate style evidence;
- 10k style/node preparation is materially lower than the ~480–620 ms measured baseline;
- no regression in raster/path-cache evidence;
- Reference mode remains visually/functionally unchanged.

## NEXT ACTION

After this checkpoint is validated/published:
1. connector visual correctness;
2. Undo/Redo + proximity auto-connect;
3. left authoring palette.

If the new split shows metric scaling rather than resolver work still dominates, stop and report
the evidence before another optimisation.
