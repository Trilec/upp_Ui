# 22 — Ui Rendering Backend Roadmap

## Purpose

This is the durable progress tracker for the current rendering tranche. The goal is a fast,
backend-neutral `upp_Ui` rendering architecture that can later gain an OpenGL/Vulkan backend
without rewriting control semantics, layout, retained geometry, or invalidation rules.

Remote `main` is authoritative. Refresh it before every implementation/publish step.

## Architectural rule

The target is **not** “all controls use BufferPainter”. R9.3A measured that directly and rejected
it as the default software strategy.

The intended backend-neutral drawing vocabulary remains small and general:

- solid/stroked rectangle and rounded rectangle;
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

The software backend is deliberately hybrid:

- direct `Draw` for cheap flat rectangles, text, lines, images and similar primitives;
- shared cached rasters for repeatable AA geometry and expensive composed imagery;
- bounded `BufferPainter` layers only for scene work where many unique vector primitives share a
  dirty region and measurement shows the layer is worthwhile;
- rich skin/shadow/image rendering remains supported, but is not forced onto every hot path.

Dirty-region ownership is unchanged. A slider thumb change must not repaint a whole window. A
Graph pan may repaint most of the Graph control because the visible scene itself changed.

## Starting point

The tranche started from:

`7f2bbbdcdd564ce0c2255c122ec473ff7dfa9799`
`UI: expose shared render layer`

Prior Graph profiling had already established:

- core large -> small model binding is fast (~140–160 us Release);
- remaining Reference <-> 10k demo delay is above core model switching;
- ordinary Reference Node Paint was over frame budget;
- 10k Node Paint dominated total paint;
- the ordinary rounded node path ultimately reached `UiPaintFaceFrameDash`, which allocated a new
  AA `ImageBuffer` for each rounded surface.

## Current shared seams

### `UiRenderLayer`

`Ui/UiRenderLayer.h` owns a bounded `ImageBuffer` + `BufferPainter`, translates the painter into
caller screen coordinates, executes prepared drawing, and composites once.

It is a useful scene seam but **not** the default software renderer and is **not** yet frozen as the
future Vulkan API.

### `UiRasterCache`

The existing shared raster cache is now the preferred software path for repeated AA surfaces when
a stable cache key can describe the visual result. The existing global memory budget and size
limits continue to bound cache growth.

## R9.3A — Rendering benchmark

Status: **ACCEPTED — WINDOWS DEBUG + RELEASE PASS**

Validated SHA:

`e7c5007efa2d1578a9866d8064bc108d0d06d5f3`

Packages:

- `Utilities/UiRenderBenchmark`
- `examples/UiRenderBenchmarkDemo`

Validation:

- Debug: `UI_RENDER_BENCH_SUMMARY checks=108 failed=0`
- Release: `UI_RENDER_BENCH_SUMMARY checks=108 failed=0`
- cache evidence: `entries=5 bytes=52368 hits=25985 misses=30 evictions=0 skipped=0`
- demo built/runs and mode/count switching remained responsive.

### Key Release evidence — full redraw average

| Workload | Strategy | 10 | 100 | 1000 |
| --- | --- | ---: | ---: | ---: |
| flat | direct | 156 us | 1,591 us | 16,976 us |
| flat | batched AA | 948 us | 2,749 us | 26,279 us |
| rounded | current styled | 8,050 us | 77,427 us | 765,398 us |
| rounded | cached AA | **296 us** | **2,682 us** | **29,298 us** |
| rounded | batched AA | 1,506 us | 3,311 us | 33,714 us |
| button-like | current styled | 7,771 us | 75,914 us | 778,223 us |
| button-like | cached AA | **422 us** | **4,257 us** | **45,229 us** |
| slider-like | current styled | 8,279 us | 73,216 us | 819,277 us |
| slider-like | cached AA | **319 us** | **3,245 us** | **32,597 us** |
| ring | local AA | 7,837 us | 79,194 us | 806,436 us |
| ring | cached AA | **265 us** | **2,609 us** | **28,878 us** |
| ring | batched AA | 1,464 us | 5,531 us | 55,241 us |
| 9-slice | direct composition | 36,779 us | 368,362 us | 3,704,326 us |
| 9-slice | cached composed | **130 us** | **1,287 us** | **14,229 us** |

### Dirty-one evidence

The bounded Painter layer behaved correctly: one dirty object stayed roughly constant regardless
of logical scene size. It was nevertheless much slower than cache/direct rendering:

- rounded: cached ~63.6 us vs batched ~3,799 us;
- button: cached ~127 us vs batched ~3,852 us;
- slider: cached ~80 us vs batched ~4,177 us;
- ring: cached ~63 us vs batched ~3,829 us.

### R9.3A decisions

1. **Flat/simple primitive:** direct `Draw` wins and remains the default.
2. **Repeated rounded AA:** exact shared raster cache wins at 10, 100 and 1000; there is no useful
   crossover where the bounded layer becomes the preferred primary path.
3. **Single dirty control/object:** direct/cache wins decisively; never substitute a full scene
   layer merely for architectural uniformity.
4. **Ring/arc:** cache strongly preferred for stable presentation.
5. **9-slice:** caching the fully composed result can be dramatically cheaper than re-composing
   nine source regions each paint.
6. **Graph:** use a hybrid strategy — direct flat primitives plus cached repeatable rounded
   surfaces; reserve the Painter layer for unique scene vectors where later evidence supports it.

## R9.3B — shared styled-surface cache routing

Status: **SOURCE PUBLISHED — WINDOWS VALIDATION PENDING**

Current source checkpoint:

`a4ae8caa0a3e76fe07d3ef082b3a5c31cb7779f4`
`UIRENDER: test styled-surface cache routing`

Implementation:

- `Ui/UiDraw.h` is now a small rendering-policy facade.
- `Ui/UiDrawBase.h` preserves the complete pre-facade implementation byte-for-byte as the
  fallback implementation; it is not a second style system.
- `Ui/Ui.upp` includes the preserved base header.
- common solid rounded `UiPaintFaceFrameDash` surfaces use an exact-size shared raster cache;
- flat surfaces stay on the established direct Draw path;
- dashed and image-filled surfaces stay on the established implementation;
- skins retain the established implementation;
- cached shadows retain their established composition and cache;
- dirty-region behavior is unchanged.

Focused structural package:

`Utilities/UiStyledSurfaceCacheTest`

It asserts:

- first rounded solid surface creates one raster entry/miss;
- an identical repeat hits without rerasterising;
- flat surface produces no cache traffic;
- dashed rounded surface retains fallback behavior;
- different presentation creates a distinct key;
- shadowed rounded surfaces retain cached shadow + cached body reuse.

### Publication recovery note

A contents-write sequencing error briefly advanced `main` to
`0704a4a1f59cb5d4be58ecdfb1d267f9a8aa6547` with only the facade file. That commit is an
intermediate historical commit, **not a valid checkpoint**. It was immediately completed by the
normal fast-forward `73284a6...`, then the focused test by `a4ae8caa...`. No force update was used
and the concurrent theme commit `337d299...` was preserved.

## R9.3C — Graph hot-path + visual baseline

Status: **NEXT AFTER R9.3B WINDOWS/PROFILE EVIDENCE**

The first R9.3C validation question is now very narrow: does routing the existing ordinary Graph
rounded surface through the shared cache materially collapse Node Paint on the real Reference and
10k views?

If yes, retain that shared solution and proceed with the visual cleanup. If not, use Diagnostics to
find the next dominant Graph phase instead of adding speculative renderer complexity.

Planned visual/performance changes after that gate:

- stock Graph node baseline flat/subtle/shadowless; Layered or explicit styles may still opt into
  shadows;
- hierarchical dot grid instead of crossed line grid:
  - base spacing 20 world units;
  - every fifth level promotes to 100, 500, 2500...;
  - screen-space cross-fade prevents popping/dense noise;
- small hollow circular port visuals with larger independent hit areas;
- clean thin low-contrast connectors;
- aggregate node surface/details/content diagnostic timings only where needed;
- remove redundant demo PropertyEditor synchronization from model changes and refresh only the
  visible editor where possible;
- keep rich skin/image/custom shape fallback separate from the ordinary high-scale path.

Before/after Graph evidence must retain the R9.2 profile outputs rather than relying on subjective
responsiveness alone.

## R10 — Graph visual / structural cleanup

Status: **PENDING R9.3 PERFORMANCE ACCEPTANCE**

Remove redundant silhouette aliases rather than preserving compatibility that no production caller
needs:

- Rectangle owns arbitrary width/height + corner radius, including square, rounded rectangle and
  capsule/pill forms;
- Ellipse owns arbitrary width/height, including circles;
- distinct silhouettes remain: Diamond, Triangle, Hexagon, Cloud, Document, Database, Custom.

Separate silhouette from internal composition. Expected small built-in policy family:

- Auto;
- Compact;
- HeaderBody;
- PortRows;
- Custom.

`PortRows` uses existing ordered `UiGraphPort`s and retained row geometry; arbitrary app body rows
remain generic app-owned data/presentation. No child Ctrl per row.

## R11 — transient edge activity

Status: **PENDING STATIC RENDER PERFORMANCE**

Add generic runtime flow presentation without putting agent/runtime semantics into `UiGraphModel`:

- Pulse: one marker travels along a retained route;
- Flow: one or a very small bounded set loops while active;
- one owned `UiFrameTicker` for all active edges;
- dirty repaint bounded to old/new marker damage where practical.

## R12 — evidence-driven adoption by other controls

Status: **PENDING R9.3 SHARED-SURFACE ACCEPTANCE**

Candidate review order based on R9.3A:

1. `UiProgressRing` — strong cache evidence for stable ring presentation; dynamic value/gradient
   keying must be designed carefully rather than caching blindly.
2. `UiSlider` — already a hybrid design; benchmark supports retaining direct track + cached AA
   details rather than moving the whole control into a layer.
3. Button/Panel/Label/Edit family — shared styled-surface facade may provide the useful benefit
   automatically; keep direct text and other cheap primitives.
4. 9-slice users — evaluate caching the final composed result when dimensions/presentation are
   stable, because R9.3A showed a very large win.
5. later scene controls (Timeline/Charts/etc.) consume the same direct/cache/layer policy rather
   than inventing their own buffering architecture.

## Recovery log

BASE: `7f2bbbdcdd564ce0c2255c122ec473ff7dfa9799`

TASK: evidence-driven shared rendering, then Graph hot-path/visual cleanup.

LATEST VERIFIED SOURCE CHECKPOINT WHEN WRITTEN:
`a4ae8caa0a3e76fe07d3ef082b3a5c31cb7779f4`

STATUS:
- R9.3A benchmark accepted Debug + Release at `e7c5007...`;
- R9.3B shared styled-surface cache source + deterministic test published;
- R9.3B Windows build/test and real Graph before/after profile still required;
- R9.3C visual/Graph-specific changes intentionally wait for that evidence.

NEXT:

1. Build `Ui`, `Utilities/UiStyledSurfaceCacheTest`, `Utilities/UiRenderBenchmark`, and Graph
   validation packages Debug + Release.
2. Run the cache test and preserve its exact summary/stats.
3. Rerun the render benchmark: `current_styled` rounded/button/slider paths should now show the
   shared facade effect while explicit `local_aa` remains the uncached control.
4. Capture real Graph Reference + 10k Node Paint before/after evidence.
5. If the shared cache materially closes the node-body bottleneck, proceed directly to R9.3C dot
   grid/shadowless baseline/hollow ports/diagnostic cleanup.
