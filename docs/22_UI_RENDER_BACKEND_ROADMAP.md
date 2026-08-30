# 22 — Ui Rendering Backend Roadmap

## Purpose

Durable tracker for the Ui rendering / UiNodeGraph performance tranche.

The target is a fast backend-neutral drawing architecture that can later gain OpenGL/Vulkan
without rewriting control semantics, layout, retained geometry, dirty-region ownership or model
contracts.

Remote `main` is authoritative. Refresh before implementation/publication.

## Rendering architecture

Do **not** force every control through BufferPainter.

The measured software strategy is deliberately hybrid:

- direct `Draw` for cheap flat rectangles, lines, text and images;
- shared exact raster cache for repeatable AA/composed surfaces;
- bounded `BufferPainter` layers only where many unique vector primitives share a dirty region and
  measurement proves the layer worthwhile;
- retain rich skin/shadow/image fallbacks without forcing them onto hot paths.

Dirty-region ownership remains local. A slider thumb change does not repaint a whole window. A
Graph pan may repaint most of the Graph because the visible scene changed.

The intended future render vocabulary remains backend-neutral:

- rect / rounded rect;
- ellipse;
- arc/ring;
- line/polyline/path;
- text;
- image/tinted image;
- 9-slice;
- gradient;
- clip/transform/opacity/layer.

`UiRenderLayer` is a useful current software seam, not the frozen future Vulkan API.

---

## R9.3A — rendering benchmark

Status: **ACCEPTED — DEBUG + RELEASE**

Validated:

`e7c5007efa2d1578a9866d8064bc108d0d06d5f3`

Packages:

- `Utilities/UiRenderBenchmark`
- `examples/UiRenderBenchmarkDemo`

Validation:

- Debug: `UI_RENDER_BENCH_SUMMARY checks=108 failed=0`
- Release: `UI_RENDER_BENCH_SUMMARY checks=108 failed=0`

Key Release full-redraw evidence:

| Workload | Strategy | 10 | 100 | 1000 |
| --- | --- | ---: | ---: | ---: |
| flat | direct | 156 us | 1,591 us | 16,976 us |
| rounded | current styled | 8,050 us | 77,427 us | 765,398 us |
| rounded | cached AA | **296 us** | **2,682 us** | **29,298 us** |
| button-like | cached AA | **422 us** | **4,257 us** | **45,229 us** |
| slider-like | cached AA | **319 us** | **3,245 us** | **32,597 us** |
| ring | local AA | 7,837 us | 79,194 us | 806,436 us |
| ring | cached AA | **265 us** | **2,609 us** | **28,878 us** |
| 9-slice direct composition | direct | 36,779 us | 368,362 us | 3,704,326 us |
| 9-slice composed | cached | **130 us** | **1,287 us** | **14,229 us** |

Dirty-one evidence showed a bounded Painter layer remains bounded but was ~3.8–4.2 ms for one
object, far slower than direct/cache rendering.

Decision:

1. flat/simple -> direct `Draw`;
2. repeatable AA/composed -> shared cache;
3. single dirty object -> direct/cache;
4. dense Graph -> hybrid;
5. do not invent a second renderer without evidence.

---

## R9.3B — shared styled-surface cache

Status: **ACCEPTED — DEBUG + RELEASE**

Accepted checkpoint:

`9a906b09bb88815e451e5c3d728b4eef24a067b6`

Implementation:

- `Ui/UiDraw.h` is the rendering-policy facade;
- `Ui/UiDrawBase.h` preserves established fallbacks;
- ordinary solid rounded styled surfaces use the shared exact raster cache;
- flat surfaces remain direct;
- dashed/image/skin special cases retain established fallback behavior;
- dirty-region behavior unchanged.

Focused validation:

- `UI_STYLED_SURFACE_CACHE_SUMMARY checks=11 failed=0` Debug + Release;
- `UI_RENDER_BENCH_SUMMARY checks=108 failed=0` Debug + Release.

R9.3A -> R9.3B Release `current_styled` improvement:

| Row | n=10 | n=100 | n=1000 |
| --- | ---: | ---: | ---: |
| rounded | 8,050 -> **271** | 77,427 -> **3,494** | 765,398 -> **37,228** |
| button | 7,771 -> **467** | 75,914 -> **4,885** | 778,223 -> **65,287** |
| slider | 8,279 -> **341** | 73,216 -> **3,697** | 819,277 -> **44,437** |

Real Graph improvement was modest (~5–13%), proving node-body rasterisation was only one part of
Node Paint. Do not respond by adding another renderer.

---

## R9.3C — Graph render-policy / visual cleanup

Status: **ACCEPTED — DEBUG + RELEASE**

Source checkpoints:

`782c9b5787f777a76974a7e35a63403f51166150`
`UIGRAPH-R9.3C: isolate clean render policy and paint phases`

`4ac9296bb6e6c75487011ce903dd9e1447d35b7e`
`UIGRAPH-R9.3C: keep flat stock faces aligned to retained geometry`

Validated compile correction:

`5f0231ec134d1bd04c38cfe94e33949f0f57727f`
`added include ui.h`

The one-line `#include <Ui/Ui.h>` must remain before the legacy Paint macro block so control headers
are guarded before macro renaming occurs.

Implementation:

- prior Graph implementation preserved in `UiNodeGraphBase.inc` as a single-TU recovery slice;
- active render policy replaces only Graph paint stages;
- node/edge presentation resolved once per painted object/frame;
- aggregate surface/details/content counters added;
- unnecessary AA details layer skipped at overview;
- stock nodes shadowless while explicit/custom shadow recipes remain supported;
- shadowless face keeps retained geometry/port alignment;
- stock ports are small hollow circular rings (4-DPI visible radius, 10-DPI hit radius);
- stock edges thinner with small open arrow;
- crossed grid replaced by subdued hierarchical dots;
- dot hierarchy: 20 -> 100 -> 500 -> 2500... world units with screen-space fade;
- low-zoom unreadable text/detail work skipped.

Validation:

- all six requested packages built Debug + Release;
- `UiStyledSurfaceCacheTest`: `checks=11 failed=0`;
- `UiNodeGraphModelSwitchProfileTest`: `checks=11 failed=0`;
- `UiNodeGraphPanProfileTest`: `checks=10 failed=0`;
- `UiNodeGraphScaleTest`: `checks=53 failed=0`.

Release performance:

| Metric | R9.3B | R9.3C | Change |
| --- | ---: | ---: | ---: |
| Pan overview Node Paint | ~189,993 us | **85,752 us** | ~55% lower |
| Pan mid Node Paint | ~40,140 us | 46,598 us | noisy / slightly higher |
| Scale low zoom | ~199,430 us | **98,772 us** | ~50% lower |
| Scale low zoom Debug | ~510,998 us | **398,106 us** | ~22% lower |

Conclusion: R9.3C materially improves overview paint. Renderer direction is accepted.

Remaining ~9 second 10k -> Reference delay is **above core Graph switching** and belongs to demo
orchestration/editor synchronization.

---

## R9.3D — demo switch / diagnostics cleanup

Status: **SOURCE PUBLISHED — WINDOWS VALIDATION REQUIRED**

Current source checkpoint when this document was written:

`3de027d3d153832938573822ec46ff35a249230f`
`UIGRAPH: activate optimized demo runtime`

Files:

- `examples/UiGraphDemo/UiGraphDemoRuntime.cpp`
- `examples/UiGraphDemo/UiGraphDemo.h`
- `examples/UiGraphDemo/UiGraphDemo.upp`
- `examples/UiGraphDemo/main.cpp`

Purpose:

- suppress intermediate PropertyEditor synchronization while Reference/10k model/view state is
  changing;
- perform one final synchronization after the switch;
- refresh only the currently visible Inspector or Style page;
- expose R9.3C node surface/details/content timing in Diagnostics;
- log individual demo-switch stages rather than guessing where remaining delay lives.

New evidence line:

`UIGRAPH_DEMO_SWITCH_STAGE mode=... ensure_us=... bind_us=... attach_us=... view_us=... final_sync_us=... total_us=...`

### Generated-code invariant

The generated C++ page is a **lazy snapshot**, not real-time graph state.

Verified source behavior to preserve:

- `WhenViewport` never calls `MarkGeneratedCodeDirty()`;
- pan/scroll does not dirty or regenerate code;
- mouse-wheel zoom does not dirty or regenerate code;
- Fit and 1:1 viewport operations do not dirty or regenerate code;
- selection does not dirty or regenerate code;
- graph node dragging does not regenerate code;
- scale-mode navigation does not regenerate code;
- actual generation occurs only through `EnsureGeneratedCode()` on Code/Copy/Save when the
  authored-design dirty flag is already set.

Do not put code generation, export-string construction, or dirtying into viewport/paint callbacks.

The complete reference snapshot currently remains invalidated by authored node/edge/style changes,
which preserves the earlier full-design handoff contract. If this contract is later narrowed to
Style-page-only export, change the generated-code content contract at the same time rather than
silently allowing a stale full-design snapshot.

### R9.3D validation gate

1. Build UiGraphDemo Debug + Release.
2. Repeat Reference -> 10k -> Reference switches and preserve both `SWITCH_STAGE` and
   `SWITCH_PROFILE` lines.
3. Confirm intermediate selection/model callbacks no longer rebuild both PropertyEditors.
4. Identify any remaining dominant switch stage from the new timing line.
5. Diagnostics must show `surface/details/content/total` node timing.
6. Generate Code once, then verify pan/zoom/Fit/1:1/viewport drag/node drag do not change/regenerate
   the code snapshot; a real authored design/style edit must still dirty the snapshot and regenerate
   lazily on Code/Copy/Save.

---

## R10 — Graph structural cleanup

Status: **NEXT AFTER R9.3D VALIDATION**

Remove redundant silhouette aliases:

- Rectangle owns arbitrary width/height + corner radius, including square, rounded rectangle and
  capsule/pill forms;
- Ellipse owns arbitrary width/height, including circles;
- distinct silhouettes remain: Diamond, Triangle, Hexagon, Cloud, Document, Database, Custom.

Separate silhouette from internal composition:

- Auto;
- Compact;
- HeaderBody;
- PortRows;
- Custom.

`PortRows` uses existing ordered ports and retained row geometry. No child Ctrl per row.

## R11 — transient edge activity

Status: **PENDING STATIC RENDER / R10**

- Pulse: one small marker travels along retained route;
- Flow: one/small bounded set loops while active;
- one owned `UiFrameTicker` for all active edges;
- dirty repaint bounded to old/new marker damage where practical;
- no agent/runtime semantics in `UiGraphModel`.

## R12 — evidence-driven adoption by other controls

Status: **PENDING GRAPH TRANCHE CLOSURE**

- `UiProgressRing`: strong cache candidate for stable presentations;
- `UiSlider`: retain direct-track + cached-AA-detail hybrid unless new evidence says otherwise;
- Button/Panel/Label/Edit: leverage shared styled facade while retaining direct text/cheap geometry;
- stable 9-slice users: consider final-composition caching;
- Timeline/Charts/dense controls: use the same direct/cache/layer policy.

---

## Recovery log

BASE: `7f2bbbdcdd564ce0c2255c122ec473ff7dfa9799`

TASK: evidence-driven shared rendering -> Graph render cleanup -> demo switch closure.

LATEST SOURCE CHECKPOINT WHEN WRITTEN:
`3de027d3d153832938573822ec46ff35a249230f`

STATUS:

- R9.3A accepted;
- R9.3B accepted;
- R9.3C accepted with ~50–55% overview Node Paint improvement;
- R9.3D source published; Windows compile/runtime validation pending;
- R10/R11 wait for R9.3D switch evidence.

NEXT:

1. Build/run UiGraphDemo Debug + Release from current remote `main`.
2. Capture `UIGRAPH_DEMO_SWITCH_STAGE` + `UIGRAPH_DEMO_SWITCH_PROFILE` for repeated switches.
3. Verify Diagnostics surface/details/content line.
4. Verify the generated-code invariant under pan/zoom/Fit/1:1/viewport drag/node drag.
5. If switch delay is closed or isolated to one stage, finish R9.3D and begin R10 shape/layout cleanup.
