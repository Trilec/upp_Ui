# 22 — Ui Rendering Backend Roadmap

## Purpose

Durable rendering/backend direction for `upp_Ui`, with UiNodeGraph as the current dense-scene proving ground.
Remote `main` is authoritative. `upp_Ui` must not gain a hard dependency on `upp_render`; future GPU rendering remains opt-in.

## Accepted rendering architecture

Do **not** route every control through BufferPainter.

Measured software policy:
1. cheap flat rectangles/lines/text/images -> direct U++ `Draw`;
2. native Painter primitives/curves -> use Painter without pre-flattening when
   that is the cheapest exact representation;
3. repeatable antialiased/composed presentation -> shared exact raster cache;
4. unique specialist vector work -> bounded Painter layer only where useful;
5. rich skin/shadow/image paths remain explicit fallbacks;
6. controls/views keep layout, input, dirty-region, state and model ownership;
   rendering backend consumes presentation semantics.

The geometry/shape stack is now explicit:

- `UiGeometry` — backend-independent final-pixel math and the library-owned
  0.35 px explicit-geometry error contract;
- `UiShapePath` — backend-neutral authored Move/Line/Quadratic/Cubic/Arc/
  EllipseArc/Close topology;
- `UiShapes` — reusable parameterised stock silhouettes;
- `UiDraw` — Draw/Painter rendering, appearance and the
  `UiPainterShapePath()` adapter.

**Normal controls can use `UiShapes`; dense scenes such as Graph may go
directly to `UiGeometry`.** The layer stack is not a mandatory traversal path:
do not allocate an authored command object merely for uniformity.

Backend-neutral presentation still needs rect/rounded rect, ellipse/arc/ring,
line/polyline/path, text, image/tint, 9-slice, gradient, clip, transform and
opacity/layer semantics, but named silhouettes should normally be authored via
`UiShapePath`/`UiShapes` rather than expanding backend primitive APIs.

`UiRenderLayer` is a current software helper, not the frozen GPU API.

## R9.3A — benchmark — ACCEPTED

Checkpoint: `e7c5007efa2d1578a9866d8064bc108d0d06d5f3`
Debug + Release: `UI_RENDER_BENCH_SUMMARY checks=108 failed=0`.

Key Release evidence:

| workload | representative result |
| --- | ---: |
| flat/direct, n=1000 | 16,976 us |
| rounded/current styled, n=1000 | 765,398 us |
| rounded/cached AA, n=1000 | **29,298 us** |
| ring/cached AA, n=1000 | **28,878 us** |
| 9-slice direct, n=1000 | 3,704,326 us |
| 9-slice cached composition, n=1000 | **14,229 us** |

Decision: direct/cache hybrid. No new general renderer without evidence.

## R9.3B — shared styled-surface cache — ACCEPTED

Checkpoint: `9a906b09bb88815e451e5c3d728b4eef24a067b6`.

- `UiDraw.h` = rendering-policy facade;
- `UiDrawBase.h` = preserved established implementation/fallback;
- solid rounded face/frame = exact shared raster cache;
- flat = direct;
- dashed/image/skin/unusual = fallback;
- dirty-region ownership unchanged.

Validation:
- styled cache `11/0` Debug + Release;
- render benchmark `108/0` Debug + Release.

Graph improved only modestly (~5-13%), proving body rasterisation was only one bottleneck.

## R9.3C-E — Graph visual/render/demo cleanup — ACCEPTED

Accepted outcomes:
- active Graph render policy isolated from retained model/spatial implementation;
- quiet shadowless stock nodes; explicit/custom shadows preserved;
- hierarchical dot grid;
- small hollow circular ports with larger independent hit area;
- thinner connectors/small open arrows;
- low-zoom text/detail LOD;
- node surface/details/content timing evidence;
- visible-page-only demo editor synchronization;
- generated C++ stays lazy behind Code/Copy/Save and never runs from viewport/paint/selection/drag/mode switching;
- R9.3E removed redundant 10k auto-fit: 10k -> Reference fell from ~8.4-9.0 s to ~0.144-0.189 s.

## R10A — canonical silhouettes — ACCEPTED FOUNDATION

Canonical authored concepts:
- Rectangle owns arbitrary width/height + corner radius;
- equal dimensions can represent square;
- half-height radius can represent pill/capsule;
- Ellipse owns ellipse/circle through dimensions;
- distinct silhouettes: Diamond, Triangle, Hexagon, Cloud, Document, Database, Custom.

Historical enum/wire values remain only for migration/source compatibility while retained recovery source exists.

Canonical named silhouettes are now conceptually separate from geometry
tessellation. New reusable silhouettes belong in `UiShapes`; genuinely new
continuous geometry mathematics belongs in `UiGeometry`. Graph may retain its
direct `UiGeometry` path because it is a dense scene.

## Interactive-frame audit — ACCEPTED

Evidence that triggered this gate:
- Reference scene: only 16 nodes / 15 edges;
- Debug observed roughly 70-100 ms total Paint;
- repeated tens-of-ms costs remained in details/ports, edges and geometry preparation.

Audit conclusions:
- this was not an acceptable node-count cost;
- `UiDraw` itself already follows the measured hybrid policy and does **not** need a broad rewrite;
- remaining waste was Graph-specific presentation and camera preparation.

### Paint corrections

- ordinary solid/dashed/dotted/open-arrow edges use direct Draw;
- only specialist edge presentation uses Painter;
- ordinary hollow port markers use shared cached AA rasters;
- ordinary non-rect node bodies without active shadow/dash use shared cached AA rasters;
- cached shape factories capture scalar presentation data, not whole node objects;
- dot grid uses a reusable cached tile;
- minor spacing is quantized once and major spacing derives from the same integer period, preventing fractional-zoom tile seams/drift;
- local Painter fallback bounds include explicit shadow offset/distance and selection extent, preventing Raised/Glow/custom clipping.

### Camera corrections

Public/programmatic view API remains synchronous/exact:
- `SetZoom`;
- `SetPan`;
- `PanBy`;
- Fit / 1:1 / host-driven view setup.

Actual live interaction is different:
- middle-pan translates already prepared screen geometry while the new visible viewport remains inside the exact prepared world-query coverage;
- wheel zoom projects prepared geometry about the pointer anchor during the gesture;
- wheel projection is allowed only while LOD thresholds, compact/micro classification, attached-control threshold, retained coverage and cumulative-scale safety remain compatible;
- wheel quiet for ~140 ms -> one exact geometry rebuild;
- unsafe boundary -> immediate exact fallback;
- world spatial index stays authoritative.

This is deliberately aligned with the future GPU model: retained scene/resources + changing camera transform, rather than rebuilding conceptual scene data because the camera moved.

### Recovery structure

- `UiNodeGraphBase.inc`: retained core implementation slice;
- `UiNodeGraphRender.inc`: active software paint policy;
- `UiNodeGraphInteractionBase.inc`: byte-for-byte retained interaction implementation;
- `UiNodeGraphInteraction.cpp`: small recovery wrapper;
- `UiNodeGraphView.inc`: active live-camera policy.

The `.inc` files are package members for source visibility but must remain single-TU includes, not separate compiled translation units.

### Demo/profiling contract

Profiling is observer-only:
- Live Profiling OFF -> `WhenViewport` returns before comparisons, strings, timers or control updates;
- Diagnostics hidden -> same zero-observer-work early-out;
- enabled + visible -> viewport activity only re-arms a post-interaction debounce;
- after ~200 ms quiet, stored Graph counters are copied to Diagnostics once;
- diagnostics drawing/text updates are never part of the measured Graph interaction.

### Deterministic contracts

- `UiNodeGraphPanProfileTest`: real small middle-pan inside retained coverage must keep geometry serial unchanged and expose `geometry_us=0`;
- `UiNodeGraphLiveViewTest`: real pan/wheel reuse retained geometry immediately; programmatic `SetZoom`/`PanBy` remain exact; spatial index remains retained.

## Historical P2 Windows acceptance gate — PASS

The interactive-frame/P2 gate passed on Windows CLANGx64 Debug/Release.
Subsequent geometry/shape-layer validation is tracked in `ACTIVE_WORK.md`.
Gary validates only; architecture changes return to supervisor.

Build/run Debug + Release:
- Ui;
- UiStyledSurfaceCacheTest;
- UiNodeGraphLiveViewTest;
- UiNodeGraphCanonicalShapeTest;
- UiGraphTest;
- UiNodeGraphModelSwitchProfileTest;
- UiNodeGraphPanProfileTest;
- UiNodeGraphScaleTest;
- UiNodeGraphRenderLodTest;
- UiNodeGraphOverviewLodTest;
- UiNodeGraphPresentationTest;
- UiNodeGraphDragDamageTest;
- UiNodeGraphRouteEditTest;
- UiGraphDemo.

Performance/visual acceptance:
- Reference Release complete Paint <16.67 ms target, preferably <10 ms;
- no details/ports/edge phase remains tens of ms for 16 nodes;
- live pan inside retained coverage: no geometry rebuild;
- repeated wheel notches: no exact rebuild per notch; one settle rebuild after quiet;
- no missing 10k objects or stale hit regions when coverage is exceeded;
- no grid seam/drift or shadow clipping;
- R9.3E mode-switch performance does not regress;
- generated-code invariant remains intact.

## Next

The P2 gate has passed. Current order is:

1. validate the published geometry/shape foundation;
2. diagnose the observed zoomed-out idle continuous repaint before adding more
   Graph hierarchy/render complexity;
3. then resume bounded composition/hierarchy work.

### R10B — node internal composition
Separate silhouette from composition:
`Auto`, `Compact`, `HeaderBody`, `PortRows`, `Custom`.
`PortRows` remains retained geometry; no child Ctrl per row.

### R11 — transient edge activity
One Graph-owned ticker, retained route reuse, bounded old/new marker damage; no runtime semantics in the model.

### R12 — evidence-driven adoption
- ProgressRing: strong stable-presentation cache candidate;
- Slider: retain direct track + cached AA detail unless new evidence changes it;
- standard controls: shared styled facade + direct text/cheap geometry;
- stable 9-slice: final-composition caching where useful;
- Timeline/charts/dense controls: same direct/cache/bounded-layer policy.

## Recovery rule

`REFRESH -> INSPECT -> IMPLEMENT -> REVIEW -> PUBLISH -> VERIFY -> VALIDATE`

Remote `main` is authoritative; preserve concurrent work, review complete dependency/package slices, run `git diff --check`, and publish only coherent fast-forward checkpoints.
