# 22 — Ui Rendering Backend Roadmap

## Purpose

This document is the durable progress tracker for the current rendering tranche.
It exists to keep Graph performance work aligned with the wider `upp_Ui` goal of
a fast, backend-neutral control library that can move toward OpenGL/Vulkan later
without rewriting control semantics or layout.

Remote `main` is authoritative. Refresh it before every implementation/publish
step and update this document as checkpoints are completed.

## Starting point

The tranche started from remote `main`:

`7f2bbbdcdd564ce0c2255c122ec473ff7dfa9799`
`UI: expose shared render layer`

The immediately preceding Graph validation established:

- `UiNodeGraph` core large -> small model rebind is fast (~140–160 us Release);
- the remaining Reference <-> 10k demo delay is above the core model switch;
- ordinary Reference node paint was already far beyond a 16.67 ms frame budget;
- the heavy 10k view was dominated by Node Paint, not edge paint;
- the stock rounded-node path ultimately reaches `UiPaintFaceFrameDash`, which
  creates a temporary AA `ImageBuffer` for each rounded surface;
- the repository already has a shared `UiRasterCache`, cached AA helpers and a
  hybrid example in `UiSlider` (direct drawing for cheap primitives, cached AA
  raster for the thumb).

## Rendering direction

The architectural target is **not** “all controls use BufferPainter”.

The target is a small backend-neutral rendering vocabulary so a control can
describe what it wants drawn while the current software backend chooses the
cheapest implementation and a later accelerated backend can implement the same
contract.

Expected primitive vocabulary:

- solid/stroked rectangle;
- rounded rectangle;
- ellipse/circle;
- arc/ring;
- line/polyline/path;
- text;
- image/tinted image;
- 9-slice image;
- gradient;
- clip;
- transform;
- opacity/layer.

Software rendering remains deliberately hybrid:

- direct `Draw` for cheap isolated rectangles, text, lines and image/9-slice work;
- small cached AA rasters for isolated repeated details such as slider thumbs;
- one bounded AA render layer for dense scene-like controls where many vector
  primitives share the same dirty region;
- rich skins/shadows remain explicit fallbacks rather than the mandatory hot path.

Dirty-region ownership is preserved. A slider changing one thumb must not redraw
a whole window. A Graph pan can legitimately redraw most of the Graph control
because its visible scene has changed.

## Current shared seam

`Ui/UiRenderLayer.h` is the first software implementation seam.

It owns a bounded `ImageBuffer` + `BufferPainter`, translates it into the caller's
screen coordinate space, executes a prepared render callback and composites the
result once.

It is intentionally **not** frozen as the eventual Vulkan-facing API. The
benchmark and Graph migration will determine what a broader `UiRenderContext`
needs before that API is committed.

## R9.3A — Rendering benchmark

Status: **PUBLISHED — WINDOWS VALIDATION / TIMING EVIDENCE PENDING**

Packages:

- `Utilities/UiRenderBenchmark` — deterministic console harness and timing output;
- `examples/UiRenderBenchmarkDemo` — visual Current / Cached AA / Batched AA
  comparison at 10 / 100 / 1000 objects with explicit offscreen timing action.

Published checkpoint before this documentation update:

`9d02bd9a643208979309feab1129bc45847c96fe`
`UIRENDER: add interactive rendering strategy comparison`

The console benchmark measures representative visual workloads at:

- 10 objects;
- 100 objects;
- 1,000 objects.

Strategies where applicable:

1. current styled/direct path;
2. explicit per-item AA buffer;
3. shared cached AA raster;
4. one bounded batched AA layer.

Representative workloads:

- flat rectangle/panel;
- rounded rectangle;
- rounded rectangle + direct text (button-like hybrid);
- slider-like direct track + AA thumb;
- ring/arc;
- 9-slice direct versus cached composed result.

Each scalable workload also has two invalidation cases:

- full scene redraw;
- one dirty object from a 10/100/1000 logical scene, with the render layer bounded
  to that object's dirty rectangle rather than the full canvas.

Timing evidence is informational, never a deterministic pass/fail threshold.
Output records cold/first draw, warm average and peak time so Windows Debug and
Release results can be compared without encoding machine-specific limits.

The demo deliberately allocates no child control per rendered item. It is a
visual rendering-strategy comparison, not a control-allocation benchmark.

Do not start broad control migration from intuition alone. R9.3B/C is gated on
actual Debug/Release benchmark output unless a correctness defect requires an
independent fix.

## R9.3B — Shared render-context seam

Status: **PENDING BENCHMARK EVIDENCE**

After R9.3A data is available, formalise only the smallest useful shared API.
Do not migrate every control merely for uniformity.

Acceptance questions:

- Is direct `Draw` still the best path for simple isolated controls?
- At what object count does batched AA beat per-item AA?
- How much does shared raster caching help repeated identical primitives?
- What is the overhead of a bounded layer for a single dirty control?
- Which primitives must be first-class for ProgressRing, Slider, Button, Panel,
  Label/Edit, Graph and later Timeline/Chart controls?

## R9.3C — Graph render hot-path migration

Status: **PENDING R9.3A RESULTS**

Planned bounded changes:

- batch ordinary visible node/vector bodies into a bounded shared AA layer;
- keep image/skin/shadow fallback separate;
- remove repeated per-node temporary rounded buffers from the common path;
- reuse resolved node presentation within a paint pass where practical;
- make stock Graph nodes flat/subtle/shadowless;
- replace crossed line grid with a subtle hierarchical dot field:
  - authored spacing 20 world units;
  - every fifth dot is the next level (100, 500, 2500...);
  - cross-fade levels based on screen-space spacing so dense dots disappear
    gradually rather than pop;
- use small hollow circular port visuals with larger independent hit targets;
- keep connectors thin and visually quiet;
- extend Diagnostics with aggregate node surface/details/content timing;
- remove redundant demo PropertyEditor synchronisation from model changes and
  refresh only the visible right-rail editor when possible.

Before/after Graph evidence must retain the R9.2 baseline profile output.

## R10 — Graph visual / structural cleanup

Status: **PENDING R9.3 PERFORMANCE ACCEPTANCE**

Remove redundant silhouette aliases rather than preserving compatibility that no
production caller needs:

- Rectangle owns arbitrary width/height + corner radius, including square,
  rounded-rectangle and capsule/pill forms;
- Ellipse owns arbitrary width/height, including circles;
- genuinely distinct shapes remain (Diamond, Triangle, Hexagon, Cloud, Document,
  Database, Custom; add another only when its silhouette is semantically useful).

Separate silhouette from internal composition. Expected small built-in policy
family:

- Auto;
- Compact;
- HeaderBody;
- PortRows;
- Custom.

Node layout remains retained/non-Ctrl and shape-safe.

## R11 — transient edge activity

Status: **PENDING STATIC RENDER PERFORMANCE**

Add generic runtime flow presentation without putting agent/runtime semantics into
`UiGraphModel`:

- Pulse: one small marker travels along a retained edge route;
- Flow: one or a very small bounded set of markers loops while an edge is active;
- one owned `UiFrameTicker` for all active edges;
- dirty repaint bounded to old/new marker damage where practical.

## R12 — evidence-driven adoption by other controls

Status: **PENDING BENCHMARK + GRAPH EVIDENCE**

Candidate review order:

1. `UiProgressRing` — currently builds a complete AA raster each Paint;
2. `UiSlider` — already hybrid and may need no redesign if measurements confirm it;
3. `UiButton`, `UiPanel`, `UiLabel`, edit controls — retain direct drawing unless
   the benchmark demonstrates a concrete benefit;
4. later scene-like controls (Timeline/Charts/etc.) can consume the shared render
   context/layer directly.

## Recovery log

BASE: `7f2bbbdcdd564ce0c2255c122ec473ff7dfa9799`

TASK: R9.3A rendering benchmark, then evidence-driven shared render/Graph migration.

TOUCHED / PUBLISHED IN THIS TRANCHE:

- `Ui/UiRenderLayer.h`
- `Ui/Ui.upp`
- `Ui/Ui.h`
- `docs/ACTIVE_WORK.md`
- `docs/22_UI_RENDER_BACKEND_ROADMAP.md`
- `Utilities/UiRenderBenchmark/UiRenderBenchmark.upp`
- `Utilities/UiRenderBenchmark/main.cpp`
- `examples/UiRenderBenchmarkDemo/UiRenderBenchmarkDemo.upp`
- `examples/UiRenderBenchmarkDemo/main.cpp`

STATUS: R9.3A source + visual demo published; Windows build/runtime timings pending.

PUBLISHED: `9d02bd9a643208979309feab1129bc45847c96fe` was the source/demo checkpoint.
Always refresh current `main` because this documentation update advances it.

VALIDATION: Windows CLANGx64 Debug + Release required for benchmark and Graph
packages before performance conclusions are accepted.

NEXT:

1. build/run `Utilities/UiRenderBenchmark` Debug + Release and preserve every
   `UI_RENDER_BENCH` line plus summary;
2. build/run `examples/UiRenderBenchmarkDemo`, visually compare Current/Cached/
   Batched at 10/100/1000 and record Run timing output;
3. use the measured crossover to choose the R9.3B shared render-context minimum;
4. immediately apply the winning dense-scene path to R9.3C Graph and rerun the
   existing Graph profiles against the R9.2 baseline.
