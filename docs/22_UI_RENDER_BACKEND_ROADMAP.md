# 22 — Ui Rendering Backend Roadmap

## Purpose

Durable tracker for the Ui rendering / UiNodeGraph performance and structural cleanup tranche.
Remote `main` is authoritative. Refresh before implementation/publication.

The long-term goal is backend-neutral Ui drawing semantics that can later participate in opt-in GPU rendering without moving control state/layout/input authority out of U++/upp_Ui. `upp_Ui` must not acquire a hard dependency on `upp_render` merely to preserve that direction.

## Rendering architecture — accepted direction

Do **not** force every control through BufferPainter.

Measured software policy:
- direct `Draw` for cheap flat rectangles, lines, text and images;
- shared exact raster cache for repeatable AA/composed surfaces;
- bounded `BufferPainter` layers only where many unique vector primitives share a dirty region and measurement proves the layer worthwhile;
- preserve rich skin/shadow/image fallbacks without forcing them onto hot paths.

Dirty-region ownership remains local. A slider thumb change does not repaint a whole window. A Graph pan may repaint most of the Graph because the visible scene changed.

Future backend-neutral drawing vocabulary should continue to cover rect/rounded rect, ellipse, arc/ring, line/polyline/path, text, image/tint, 9-slice, gradient, clip, transform, opacity/layer. `UiRenderLayer` is a current software helper, not the frozen future GPU API.

---

## R9.3A — rendering benchmark

Status: **ACCEPTED — DEBUG + RELEASE**

Validated checkpoint:
`e7c5007efa2d1578a9866d8064bc108d0d06d5f3`

Validation:
- `UI_RENDER_BENCH_SUMMARY checks=108 failed=0` Debug + Release.

Key Release evidence:

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

Decision:
1. flat/simple -> direct Draw;
2. repeatable AA/composed -> shared cache;
3. single dirty object -> direct/cache;
4. dense Graph -> hybrid;
5. do not invent another renderer without evidence.

---

## R9.3B — shared styled-surface cache

Status: **ACCEPTED — DEBUG + RELEASE**

Accepted checkpoint:
`9a906b09bb88815e451e5c3d728b4eef24a067b6`

Implementation:
- `Ui/UiDraw.h` is the rendering-policy facade;
- `Ui/UiDrawBase.h` preserves established fallback behavior;
- ordinary solid rounded styled surfaces use shared exact raster caching;
- flat surfaces remain direct;
- dashed/image/skin special cases retain established fallback behavior;
- dirty-region behavior unchanged.

Focused validation:
- `UI_STYLED_SURFACE_CACHE_SUMMARY checks=11 failed=0` Debug + Release;
- `UI_RENDER_BENCH_SUMMARY checks=108 failed=0` Debug + Release.

R9.3A -> R9.3B Release `current_styled`:

| Row | n=10 | n=100 | n=1000 |
| --- | ---: | ---: | ---: |
| rounded | 8,050 -> **271** | 77,427 -> **3,494** | 765,398 -> **37,228** |
| button | 7,771 -> **467** | 75,914 -> **4,885** | 778,223 -> **65,287** |
| slider | 8,279 -> **341** | 73,216 -> **3,697** | 819,277 -> **44,437** |

Real Graph improvement was only ~5–13%, proving body rasterisation was one part of Node Paint rather than the whole bottleneck.

---

## R9.3C — Graph render policy / visual cleanup

Status: **ACCEPTED — DEBUG + RELEASE**

Source checkpoints:
- `782c9b5787f777a76974a7e35a63403f51166150`
- `4ac9296bb6e6c75487011ce903dd9e1447d35b7e`
- compile-order correction `5f0231ec134d1bd04c38cfe94e33949f0f57727f`

Implementation:
- previous Graph implementation retained in `UiNodeGraphBase.inc` as a single-TU recovery slice;
- active render policy replaces Graph paint stages without changing model/spatial ownership;
- node/edge presentation resolved once per painted object/frame;
- surface/details/content timing counters;
- unnecessary AA details skipped at overview;
- stock nodes shadowless while explicit/custom shadow recipes remain supported;
- subdued hierarchical dot grid: 20 -> 100 -> 500 -> 2500... world units;
- hollow circular ports: ~4-DPI visual radius, larger independent hit radius;
- thinner quiet connectors with small open arrow;
- low-zoom unreadable text/detail work skipped.

Windows validation:
- styled cache `11/0`;
- model switch `11/0`;
- pan `10/0`;
- scale `53/0`.

Release performance:

| Metric | R9.3B | R9.3C | Change |
| --- | ---: | ---: | ---: |
| Pan overview Node Paint | ~189,993 us | **85,752 us** | ~55% lower |
| Scale low zoom | ~199,430 us | **98,772 us** | ~50% lower |
| Scale low zoom Debug | ~510,998 us | **398,106 us** | ~22% lower |

Renderer direction accepted. Do not keep optimizing rounded-body rendering without new evidence.

---

## R9.3D — demo synchronization / diagnostics

Status: **ACCEPTED — DEBUG + RELEASE**

Source checkpoint:
`3de027d3d153832938573822ec46ff35a249230f`

Result:
- intermediate model/selection callbacks no longer rebuild both PropertyEditors;
- one final sync refreshes only the visible Inspector or Style page;
- Diagnostics exposes node surface/details/content/total;
- switch-stage timing isolates ensure/bind/attach/view/final-sync;
- `final_sync_us` measured around 1.5 ms, proving PropertyEditor was not the remaining stall.

Generated-code invariant remains mandatory:
- no generated-code construction/dirtying in `WhenViewport` or Paint;
- pan/zoom/Fit/1:1/selection/node drag/mode switching do not regenerate code;
- Code/Copy/Save regenerate lazily only after an authored design/style invalidation.

R9.3D exposed ~8.2 s of 10k -> Reference time outside measured stages, traced to first-paint auto-fit being re-enabled while the 10k model was still bound.

---

## R9.3E — auto-fit stall closure

Status: **ACCEPTED — DEBUG + RELEASE**

Source checkpoint:
`6d941dd5e1e9a3e5b1ad6860c2bb6927cc57220d`

Validated later on descendant HEAD `1780907a1aa50b3a64a4baf70d573718ebe45161`.

Structural result:

| Transition | R9.3D | R9.3E |
| --- | ---: | ---: |
| 10k -> Reference #1 | 8.43 s | **0.144 s** |
| 10k -> Reference #2 | 8.98 s | **0.189 s** |

For both Reference returns, `total_us` now equals the sum of measured stages. The unexplained ~8.2 s disappeared. Reference returned correctly fitted at zoom ~0.87 with all 16 nodes and embedded controls visible, no delayed second auto-fit, and generated-code invariants intact.

**R9.3A–E CLOSED.**

---

## R10A — canonical node silhouettes

Status: **SOURCE PUBLISHED — WINDOWS ACCEPTANCE PENDING**

Initial checkpoint:
`0576a97e1de84bd2da46a0315ba7694c37e39b63`
`UIGRAPH-R10A: canonicalize node silhouettes`

Current corrected source checkpoint when this document was updated:
`01b80c8dcd8dd4686e1da2216bc5a50f77a09c74`
`UIGRAPH-R10A: stabilize legacy shape migration enum`

Concurrent descendants between those checkpoints include unrelated UiMenu fixes and small Graph compile-compatibility fixes; all were preserved.

Canonical authored direction:
- `Rectangle` owns arbitrary width/height + `corner_radius`;
- radius 0 = flat rectangle;
- radius > 0 = rounded rectangle;
- equal width/height = square;
- radius at/above half smaller dimension = pill/capsule after renderer clamp;
- `Ellipse` owns arbitrary width/height; equal dimensions = circle;
- distinct silhouettes remain Diamond, Triangle, Hexagon, Cloud, Document, Database, Custom.

Wire policy:
- canonical authored `Rectangle` uses wire 13;
- old wire 0 remains distinguishable as historical flat Rectangle;
- surviving canonical silhouettes retain established wire ids;
- historical shape values remain migration/source-compatibility values while old serialized/recovery source is supported;
- the enum definition is identical in every translation unit to avoid BLITZ/ODR instability.

Authored surfaces:
- new nodes default to canonical Rectangle with corner radius 8;
- Inspector choices are canonical authored silhouettes only;
- generated code must emit canonical authored concepts;
- focused deterministic package: `Utilities/UiNodeGraphCanonicalShapeTest`.

R10A is not accepted until Windows Debug + Release builds/tests and the Reference visual check pass.

### R10A Windows gate

Build/run:
- `Ui`
- `Utilities/UiNodeGraphCanonicalShapeTest`
- `Utilities/UiGraphTest`
- `Utilities/UiNodeGraphModelSwitchProfileTest`
- `Utilities/UiNodeGraphPanProfileTest`
- `Utilities/UiNodeGraphScaleTest`
- `Utilities/UiNodeGraphRenderLodTest`
- `Utilities/UiNodeGraphOverviewLodTest`
- `Utilities/UiNodeGraphPresentationTest`
- `Utilities/UiNodeGraphDragDamageTest`
- `examples/UiGraphDemo`

Acceptance requires zero-failure focused/existing tests, canonical Inspector/code output, equivalent flat/rounded/square/pill/circle/ellipse demo examples, correct port/silhouette alignment, and no regression to R9.3E Reference switching or generated-code invariants.

---

## R10B — node internal composition

Status: **NEXT AFTER R10A ACCEPTANCE**

Separate silhouette from internal composition:
- `Auto`;
- `Compact`;
- `HeaderBody`;
- `PortRows`;
- `Custom`.

`PortRows` uses existing ordered ports and retained row geometry. No child Ctrl per row.

---

## R11 — transient edge activity

Status: **PENDING R10**

- Pulse: one small marker travels along retained route;
- Flow: one/small bounded set loops while active;
- one owned `UiFrameTicker` for all active edges;
- dirty repaint bounded to old/new marker damage where practical;
- no agent/runtime semantics in `UiGraphModel`.

---

## R12 — evidence-driven adoption by other controls

Status: **PENDING GRAPH STRUCTURAL TRANCHE**

- `UiProgressRing`: strong cache candidate for stable presentations;
- `UiSlider`: retain direct-track + cached-AA-detail hybrid unless new evidence says otherwise;
- Button/Panel/Label/Edit: use shared styled facade while retaining direct text/cheap geometry;
- stable 9-slice users: consider final-composition caching;
- Timeline/Charts/dense controls: use the same direct/cache/layer policy.

---

## Recovery log

Refresh current `main` before acting.

Current task: R10A Windows acceptance.
Current source checkpoint when written: `01b80c8dcd8dd4686e1da2216bc5a50f77a09c74`.
R9.3A–E: accepted/closed.
R10A: source published, Windows acceptance pending.
R10B/R11: do not start until R10A acceptance.
