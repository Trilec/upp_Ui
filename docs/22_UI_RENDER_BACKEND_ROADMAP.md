# 22 — Ui Rendering Backend Roadmap

## Purpose

Durable progress tracker for the current rendering tranche. The goal is a fast, backend-neutral
`upp_Ui` rendering architecture that can later gain OpenGL/Vulkan without rewriting control
semantics, layout, retained geometry, or invalidation rules.

Remote `main` is authoritative. Refresh before implementation/publication.

## Architectural rule

The target is **not** “all controls use BufferPainter”. R9.3A measured and rejected that as the
default software strategy.

The intended backend-neutral vocabulary remains small and general:

- solid/stroked rectangle and rounded rectangle;
- ellipse/circle;
- arc/ring;
- line/polyline/path;
- text;
- image/tinted image;
- 9-slice image;
- gradient;
- clip/transform/opacity/layer.

Software rendering is deliberately hybrid:

- direct `Draw` for cheap flat rectangles, text, lines, images and similar primitives;
- shared cached rasters for repeatable AA geometry and expensive composed imagery;
- bounded `BufferPainter` layers only for scene work where many unique vector primitives share a
  dirty region and measurement shows that layer is worthwhile;
- rich skin/shadow/image rendering remains supported but is not forced onto every hot path.

Dirty-region ownership is preserved. A slider thumb does not repaint a whole window. A Graph pan
may repaint most of the Graph control because its visible scene changed.

## Starting point

Tranche base:

`7f2bbbdcdd564ce0c2255c122ec473ff7dfa9799`
`UI: expose shared render layer`

Pre-tranche Graph evidence:

- core large -> small model binding ~140–160 us Release;
- Reference <-> 10k demo delay was above core model switching;
- ordinary Reference Node Paint was over frame budget;
- 10k Node Paint dominated total paint;
- ordinary rounded surfaces reached an AA `ImageBuffer` allocation path per surface.

## Shared seams

### `UiRenderLayer`

Bounded software `ImageBuffer` + `BufferPainter` scene layer. Useful for unique vector batches, but
**not** the default software renderer and **not** frozen as the eventual Vulkan API.

### `UiRasterCache`

Preferred software path for repeatable AA/composed surfaces when a stable key describes the visual
result. Existing global memory budget and size limits bound growth.

---

## R9.3A — rendering benchmark

Status: **ACCEPTED — WINDOWS DEBUG + RELEASE PASS**

Validated SHA:

`e7c5007efa2d1578a9866d8064bc108d0d06d5f3`

Packages:

- `Utilities/UiRenderBenchmark`
- `examples/UiRenderBenchmarkDemo`

Validation:

- Debug: `UI_RENDER_BENCH_SUMMARY checks=108 failed=0`
- Release: `UI_RENDER_BENCH_SUMMARY checks=108 failed=0`
- cache: `entries=5 bytes=52368 hits=25985 misses=30 evictions=0 skipped=0`

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
| 9-slice | direct composition | 36,779 us | 368,362 us | 3,704,326 us |
| 9-slice | cached composed | **130 us** | **1,287 us** | **14,229 us** |

Dirty-one evidence proved a bounded Painter layer remains bounded but is much slower than cache or
direct rendering for one changed object (~3.8–4.2 ms versus tens of microseconds).

### R9.3A decision

1. Flat/simple primitive -> direct `Draw`.
2. Repeatable rounded AA -> shared exact raster cache.
3. Single dirty object -> direct/cache, not a large layer.
4. Stable ring/arc -> strong cache candidate.
5. Stable final 9-slice composition -> strong cache candidate.
6. Dense Graph -> hybrid; do not invent a second renderer without evidence.

---

## R9.3B — shared styled-surface cache routing

Status: **ACCEPTED — WINDOWS DEBUG + RELEASE PASS**

Implementation checkpoints culminated in:

`9a906b09bb88815e451e5c3d728b4eef24a067b6`
`UIRENDER: record benchmark decision and cache checkpoint`

Implementation:

- `Ui/UiDraw.h` is the rendering-policy facade;
- `Ui/UiDrawBase.h` preserves the complete established implementation as fallback;
- common solid rounded styled surfaces use the shared exact raster cache;
- flat surfaces stay direct;
- dashed/image/skin special cases retain established fallbacks;
- cached shadow composition remains supported;
- dirty-region behavior is unchanged.

Focused package:

`Utilities/UiStyledSurfaceCacheTest`

Windows validation at `9a906b09...`:

- Debug + Release: `UI_STYLED_SURFACE_CACHE_SUMMARY checks=11 failed=0`
- Debug + Release render benchmark: `UI_RENDER_BENCH_SUMMARY checks=108 failed=0`
- cache traffic: `entries=8 bytes=79200 hits=41591 misses=33`

### R9.3B Release before/after

| Row | R9.3A n=10 | R9.3B n=10 | R9.3A n=100 | R9.3B n=100 | R9.3A n=1000 | R9.3B n=1000 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| rounded/current_styled | 8,050 | **271** | 77,427 | **3,494** | 765,398 | **37,228** |
| button/current_styled | 7,771 | **467** | 75,914 | **4,885** | 778,223 | **65,287** |
| slider/current_styled | 8,279 | **341** | 73,216 | **3,697** | 819,277 | **44,437** |

`local_aa` remained ~800–900 us/item, proving the shared cache rather than machine variance caused
the change. Flat/direct had no material regression.

### Real Graph effect

The shared cache helped, but only modestly because node body rasterisation was only one part of
Graph Node Paint:

| Metric | R9.2 | R9.3B | Change |
| --- | ---: | ---: | ---: |
| PanProfile overview Node Paint Release | 204,842 us | **189,993 us** | ~7% lower |
| ScaleTest low-zoom paint Release | 229,477 us | **199,430 us** | ~13% lower |
| ScaleTest low-zoom paint Debug | 534,436 us | **510,998 us** | ~4% lower |
| PanProfile mid Node Paint Release | 40,667 us | 40,140 us | ~1% lower |

Conclusion: shared cache solved the generic rounded-surface pathology. Remaining Graph cost belongs
to Graph presentation/content work. Do **not** respond by adding another renderer.

### Historical publication note

`0704a4a...` was an incomplete contents-write intermediate while introducing the facade; it was
immediately completed by normal fast-forward commits. It is not a valid checkpoint. No force update
was used and concurrent theme work was preserved.

---

## R9.3C — Graph render-policy / visual cleanup

Status: **SOURCE PUBLISHED — WINDOWS BUILD/PERFORMANCE VALIDATION REQUIRED**

Current R9.3C source checkpoint:

`4ac9296bb6e6c75487011ce903dd9e1447d35b7e`
`UIGRAPH-R9.3C: keep flat stock faces aligned to retained geometry`

Parent source checkpoint:

`782c9b5787f777a76974a7e35a63403f51166150`
`UIGRAPH-R9.3C: isolate clean render policy and paint phases`

### Implementation

The validated R9.2/R9.3B Graph implementation is preserved as:

`Ui/UiGraph/UiNodeGraphBase.inc`

The active `UiNodeGraph.cpp` compiles that dependency slice with legacy names and replaces only:

- `Paint()`;
- `PaintGraphGeometry()`;
- `PaintNodeDetails()`.

This is intentionally a recoverable render-policy experiment, not a spatial/model rewrite.

Current R9.3C behavior:

- node and edge presentation styles are resolved once per painted object/frame and reused across
  surface/details/content passes;
- new aggregate read-only timings:
  - node surface paint;
  - node details/ports paint;
  - node content/text paint;
- the AA node-details viewport buffer is skipped when the current visible scene contains only
  rectangular nodes with no visible header-band/port detail that requires it;
- stock unclassified nodes suppress the old exact default soft-shadow recipe;
- explicit/custom/preset shadow styles remain supported;
- shadowless rectangular stock faces paint into the already-retained face rectangle, preserving
  the previous body/port-anchor geometry;
- stock port visual radius is 4 DPI while the independent hit radius remains 10 DPI;
- all stock ports use a small hollow circular ring; direction remains topology/hit semantics;
- inactive port colour is subdued, while hover/connection validation can strengthen/tint it;
- stock edges are thinner and default to a smaller open arrow rather than a filled triangle;
- crossed grid lines are replaced by direct 1-pixel dots;
- authored base spacing remains 20 world units;
- hierarchy promotes every fifth level (20 -> 100 -> 500 -> 2500... world units);
- finer dots fade by screen spacing and a retained hierarchy stays around useful navigation scale;
- low-zoom text work is skipped before it becomes visually useful, while retained geometry remains
  available for interaction/LOD.

### Publication recovery note

During staging, a one-byte `Ui/UiGraph/THIS_SHOULD_NOT_EXIST.tmp` placeholder was accidentally
created on `main` at `52b6063...` and immediately removed by the normal fast-forward
`a1bd606...`. Final tree content returned to the exact pre-placeholder tree before R9.3C was
published. Neither commit is a valid implementation checkpoint; no force update was used.

### R9.3C validation gate

Before changing Graph architecture further, Windows CLANGx64 Debug + Release must establish:

1. `Ui` and all focused Graph packages compile with the preserved `.inc` slice.
2. Existing deterministic Graph tests remain zero-failure.
3. New surface/details/content counters are coherent and sum to the dominant part of Node Paint.
4. Dot grid has no line-grid remnants, moire/popping, or excessive prominence at 1.0 / 0.5 / 0.2.
5. Hollow ports remain centered on the node silhouette and the 10-DPI hit target remains practical.
6. Stock shadow removal does not move the face/ports; explicit shadow presets still draw.
7. Edge route interaction remains unchanged despite quieter visible width/arrow styling.
8. Compare R9.3C Node Paint with R9.3B (~190 ms overview Release / ~40 ms mid Release) and identify
   whether surface, details, or content is now dominant.

Do not start a new GPU/software renderer if these numbers remain high. Use the new aggregate phases
to choose the next bounded optimization.

---

## R10 — Graph structural cleanup

Status: **PENDING R9.3C PERFORMANCE/VISUAL ACCEPTANCE**

Remove redundant silhouette aliases:

- Rectangle owns arbitrary width/height + corner radius, including square, rounded rectangle and
  capsule/pill forms;
- Ellipse owns arbitrary width/height, including circles;
- distinct silhouettes remain: Diamond, Triangle, Hexagon, Cloud, Document, Database, Custom.

Separate silhouette from internal composition. Expected built-in policy family:

- Auto;
- Compact;
- HeaderBody;
- PortRows;
- Custom.

`PortRows` uses existing ordered `UiGraphPort`s and retained row geometry. Arbitrary app body rows
remain generic app-owned data/presentation. No child Ctrl per row.

## R11 — transient edge activity

Status: **PENDING STATIC RENDER PERFORMANCE**

- Pulse: one small marker travels along a retained route;
- Flow: one/small bounded set loops while active;
- one owned `UiFrameTicker` for all active edges;
- dirty repaint bounded to old/new marker damage where practical;
- no agent/runtime semantics added to `UiGraphModel`.

## R12 — evidence-driven adoption by other controls

Status: **PENDING R9.3C ACCEPTANCE**

1. `UiProgressRing`: strong cache evidence for stable presentation; dynamic value/gradient keying
   requires care.
2. `UiSlider`: benchmark supports retaining the current direct-track + cached-AA-detail hybrid.
3. Button/Panel/Label/Edit: shared styled-surface facade may provide benefits automatically; keep
   direct text and cheap primitives.
4. Stable 9-slice users: evaluate caching final composed output.
5. Timeline/Charts/dense controls: use the same direct/cache/layer policy rather than inventing a
   second buffering architecture.

## Recovery log

BASE: `7f2bbbdcdd564ce0c2255c122ec473ff7dfa9799`

TASK: evidence-driven shared rendering, then Graph render-policy/visual cleanup.

LATEST SOURCE CHECKPOINT WHEN WRITTEN:
`4ac9296bb6e6c75487011ce903dd9e1447d35b7e`

STATUS:
- R9.3A accepted Debug + Release;
- R9.3B accepted Debug + Release and generic cache pathology closed;
- R9.3C source published; Windows compile/profile/visual validation pending;
- demo PropertyEditor synchronization cleanup is still pending after the R9.3C production build
  checkpoint is proven;
- R10/R11 intentionally wait for R9.3C evidence.

NEXT:

1. Build `Ui`, `UiGraphDemo`, and focused Graph tests Debug + Release from current remote `main`.
2. Preserve existing profile lines and capture new node surface/details/content timing evidence.
3. Visually check dot-grid hierarchy, hollow ports, shadowless stock nodes and quieter edges.
4. If source is sound, apply the remaining demo-only selection/editor synchronization cleanup and
   expose the new aggregate phases in Diagnostics.
5. Then decide the next bounded Graph optimization from the measured dominant phase, not intuition.
