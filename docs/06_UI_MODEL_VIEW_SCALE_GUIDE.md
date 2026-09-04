# 06 — Ui Model-View Scale Guide

High-scale contract for model-backed `upp_Ui` controls.

This guide complements `03_UI_MODEL_GUIDE.md` and
`07_UI_MODEL_RENDERING_PLAN.md`. The model guide defines ownership,
request-first mutation, stable identity and callbacks; the rendering plan defines
shared item presentation; this guide defines the performance rules that keep
those controls usable with very large logical models.

## Core contract

Model-backed controls must treat logical item count and visual object count as
separate things.

1. **The model is authoritative.** Selection, edits and commands resolve to model
   indexes or stable IDs, never to the lifetime of a painted/attached visual.
2. **Ordinary model items are not child `Ctrl`s.** List rows, Gallery tiles, Table
   cells, Tree rows and normal Graph nodes are painted by the owning view. A child
   control is reserved for genuinely interactive embedded content.
3. **Renderers are also virtualized.** `UiItemRender` is a cheap non-`Ctrl`
   presentation object, but high-scale views create/recycle only enough renderer
   instances for visible/overscan surfaces rather than one per logical record.
4. **Viewport work is bounded.** Normal paint, hover and hit testing must be
   proportional to visible/overscan content, not the index of the visible item or
   the total item count.
5. **Geometry has a direct fast path.** Uniform rows/cells use arithmetic mapping
   between logical index, scroll position and screen rectangle. Variable-width or
   spatial views retain geometry/indexes so ordinary interaction does not rescan
   the entire prefix/model.
6. **Renderer layout is prepared outside Paint.** A renderer may cache internal
   image/text/action rectangles, but `Layout()` is reached through dirty-gated
   preparation on layout/model/theme/geometry changes. `Paint()` and `HitTest()`
   consume prepared geometry only.
7. **Model changes carry useful scope.** `UiModelChange` insert/erase/update/move
   ranges should be used to avoid full refresh/rebuild when a narrower response
   is truthful. A structural/reset change may legitimately rebuild a projection.
8. **Lazy assets follow the viewport.** Views may expose visible/overscan ranges;
   owners/providers prepare thumbnails or other expensive data for that bounded
   range and refresh the relevant item when ready.
9. **Bulk user operations may be bulk.** Explicit operations such as Select All,
   a very large marquee, copying/exporting/filtering or rebuilding a changed Tree
   projection can be O(N). Ordinary scrolling and painting may not be O(N).
10. **Large logical extent must not overflow geometry.** Shared extent helpers use
    wide intermediates and saturate where native control geometry remains
    integer-based.
11. **Geometry detail is screen-bounded.** Generated curve detail follows
    `UiGeometry`'s 0.35 final-device-pixel error contract. High-count scenes may
    call `UiGeometry` directly; they must not allocate a `UiShapePath` per
    item merely for abstraction uniformity.

The engineering target for high-scale row/item views is at least 100,000 logical
records with ordinary viewport cost independent of total model size. This is an
invariant target, not a wall-clock threshold tied to one machine.

## Shared model-view geometry

`UiModelView.h` contains small pure helpers shared by high-scale views:

- `UiVisibleRange`
- `UiUniformContentExtent(...)`
- `UiComputeLinearVisibleRange(...)`
- `UiComputeGridVisibleRange(...)`
- `UiComputeUniformInsertBefore(...)`

These helpers deliberately do not create an abstract view hierarchy. The Ui
control family shares a performance/model contract without forcing List, Gallery,
Table, Tree and Graph through one inheritance design.

## Shared item rendering

`UiItemRenderData` is the common presentation payload and `UiItemRender` is the
shared presentation object. Built-in `UiItemRenderBasic` and
`UiItemRenderImage` support horizontal and vertical composition and follow the
normal Ui theme/role/style lifecycle.

A renderer instance belongs to one currently prepared visible surface. List,
Gallery, Table and Tree own bounded renderer pools, rebind those slots as logical
records enter the useful range, and retain their models as the sole semantic
authority. Renderer prototypes can be replaced without changing the view's
selection, hierarchy, scrolling, editing or topology rules.

`UiItemRenderImage` treats vertical Gallery presentation as a genuinely scalable
tile presentation. Theme-provided text fonts are resolved during renderer
`Layout()` and can step down through a small bounded ladder when the allocated tile
becomes compact. Horizontal List presentation keeps the normal theme fonts, and an
explicit custom title font from model data is not silently rescaled. The resolved
fonts are cached with the item geometry so `Paint()` never performs font sizing or
layout work.

## UiGallery

`UiGallery` is a fluid, wrapping visual item view backed by `UiListModel`.

Its high-scale geometry is intentionally uniform:

- every logical item occupies one uniform cell;
- content inside the cell may have different intrinsic dimensions;
- column count resolves from available width;
- resize changes column count without constructing item controls;
- index-to-cell, hit testing, scrolling and visible-range calculation are
  arithmetic;
- only viewport-intersecting items are painted;
- visible plus overscan items have prepared/recycled `UiItemRender` instances;
- a small configurable row overscan is reported through `WhenVisibleRange` for
  lazy thumbnail/data preparation.

`SetItemRender(...)` is the presentation extension point. Selection, scrolling,
zoom, marquee selection, model ownership and useful-range calculation remain
Gallery responsibilities. `SetZoom()`/`ZoomBy()` alter uniform cell geometry and
emit `WhenZoom` only when the resolved semantic zoom actually changes; on Windows,
Ctrl+wheel routes to Gallery zoom. Expensive preview preparation should follow the
bounded useful range rather than delaying cheap grid arithmetic.

Multi-select background marquee maps the band to row/column spans arithmetically.
Plain drag replaces selection, Ctrl drag toggles against the opening selection,
Shift drag extends/adds from the opening selection, Escape restores the opening
selection, and edge drag autoscrolls.

Gallery interaction chrome is view-owned. The marquee fill is drawn behind tile
content, its frame is drawn above the tiles, and selected tiles receive an
explicit theme-derived interaction frame independent of the renderer's own
selected-state surface. This keeps thumbnails/text readable while making both
marquee and selected-state boundaries obvious in Light and Dark modes.

Gallery marquee capture has one explicit ownership rule: only the active Gallery
marquee path may release capture, and that ownership flag is cleared before the
release call. `CancelMode()` never calls `ReleaseCapture()`. This is required on
Win32/U++ because capture release can synchronously re-enter `CancelMode()`; a
release from inside that callback would recurse until stack overflow.

The theme-driven Gallery viewport derives row/content palette information from the
current List role but is itself a complete visual surface. The Minimal List role
may intentionally resolve its normal row face to `UiFill::None()` so a List can
blend into an owning surface. Gallery must not convert that intentional
transparency into platform `SColorPaper()`; when its List-derived face is `NONE`,
Gallery resolves that face through the current theme's Panel/Surface role. This
preserves lightweight embedded List semantics while giving a standalone Gallery a
real dark viewport in Dark mode. Gallery also drops List row/nine-slice skin for
its viewport so a stale light raster cannot survive a theme switch. Custom Gallery
styles may still provide an explicit skin/fill.

Theme-aware demos and hosts must also distinguish **content setters** from
**style-mutating setters**. Calling a setter that enters `StyleEdit()` can
legitimately create a local custom style and therefore freeze the currently
resolved theme snapshot. The Gallery showcase keeps its `UiGroupPanel` header on
its theme-driven style and places transparent layout regions on a theme-aware
`UiPanel` surface; it does not hardcode a separate dark-demo palette.

Variable-height/masonry layout is intentionally outside the current high-scale
contract. If added later it must declare its retained geometry/cache cost rather
than weakening the uniform fast path.

## UiList

`UiList` uses a uniform row extent. Normal paint and drag insertion positions are
derived directly from scroll position and row extent; it does not start at row
zero and walk forward until the viewport is reached.

Visible rows are presented through a bounded horizontal `UiItemRender` pool. The
default is `UiItemRenderBasic`; callers can replace the renderer prototype without
changing model or selection ownership. A single visible-row `UI_MODEL_UPDATE`
rebinds/relayouts only the affected prepared renderer while structural changes may
require broader row-position work.

## UiTable

`UiTable` is the explicit row/column data surface. R2C gives it both a direct deep
row path and retained variable-column geometry:

- visible rows are derived arithmetically from vertical scroll position and
  uniform row height;
- `int64` retained column prefix offsets are rebuilt when column geometry changes;
- visible/deep columns are found with binary lookup rather than summing widths from
  column zero on every paint/hit test;
- Paint iterates only the visible row × visible column intersection;
- visible plus overscan cells use a bounded renderer pool;
- column headers and row headers use separate bounded renderer pools;
- default cell/header/row-header presentation uses theme-aware
  `UiItemRenderBasic` configuration;
- `SetColumnCellRender(...)` can override one column's cell presentation without
  changing Table model/selection/editing authority;
- one transient editor remains the active editing mechanism.

Table still owns alternate/read-only/warning/error/custom cell backgrounds,
selection/hot/active chrome, grid, sort markers and resize guides. Those are view
semantics rather than generic item-render content.

A narrow cell update rebinds at most its currently prepared renderer and does not
rebuild retained column geometry. Column resize rebuilds prefix geometry once and
then reuses it for paint, hit testing and deep horizontal navigation.

## UiTree

`UiTree` owns a retained `visible_rows_` projection because expansion changes the
linear view of hierarchical data. Rebuilding that projection after a structural
change can legitimately be O(number of visible nodes); ordinary scrolling and
painting over an unchanged projection are viewport-bounded.

R2C adds:

- arithmetic visible-row range calculation from `scroll_y_` and uniform row
  height;
- a retained node-id → exact visible-projection-row map rebuilt with the
  projection;
- placeholder-safe lookup: lazy-loading placeholder rows are visible but are not
  inserted into the node-id lookup, so later real rows retain correct indexes;
- direct `ScrollTo`, selection-range, drag-order, drop-target and accessory-control
  positioning through that lookup instead of repeated prefix scans;
- a bounded primary-row renderer pool plus bounded renderer surfaces for visible
  data columns;
- `SetColumnRender(...)` for column-specific presentation overrides;
- Paint beginning at the computed visible projection row rather than row zero;
- lazy completion clears loading state before the model notification rebuilds the
  projection, so a legitimate zero-child completion cannot leave a stale
  `Loading...` placeholder.

Tree continues to own hierarchy depth, connector lines, disclosure/loading glyphs,
expansion, selection, drag/drop, focus/drop chrome, lazy lifecycle and attached
exceptional `Ctrl` accessories. A logical Tree node with several columns remains
one node even though it may use several visible renderer surfaces.

`GetContentSize()` is an explicit measurement operation and may inspect the
projection; that does not weaken the ordinary scrolling/painting contract.

## UiNodeGraph

`UiNodeGraph` uses two deliberately separate retained layers:

1. **world-space broad phase** — one persistent spatial hash containing logical
   node/edge bounds;
2. **screen-space prepared scene** — only spatial candidates relevant to the
   current viewport/overscan plus transient drag dependencies.

Normal graph objects are painted rather than represented by one `Ctrl` each, and
stable graph IDs live only in `UiGraphModel`. Exceptional embedded interactive
content may attach an externally-owned `Ctrl`; that does not change the ordinary
virtual-node contract.

The final high-scale rules are:

- pan/zoom changes screen preparation but reuses the world-space index;
- node geometry mutation removes/reinserts that node's spatial record and updates
  its incident edges rather than rebuilding the hash;
- nested `BeginBatchUpdate()` / `EndBatchUpdate()` coalesces multiple authoritative
  model mutations into one retained-view response at the outer commit;
- empty spatial hash cells are deleted when their final node/edge leaves so long
  editing sessions do not accumulate vacant buckets;
- public `HitTestNode`, `HitTestPort` and `HitTestEdge` use the same tiny
  world-space spatial-neighborhood queries as live pointer interaction and then
  exact-test only those candidates;
- normal local marquee drag queries only intersecting cells for transient preview;
  semantic selection commits on release;
- a marquee spanning more than 256 cells suppresses live candidate hints and
  performs one final spatial query on release rather than making zoomed-out drag
  expensive;
- node-style-class preview rebuilds only currently prepared users of that class
  and their prepared incident edges; it does not rebuild the spatial hash or the
  complete prepared viewport;
- edge/global style changes may legitimately invalidate broader routing/spatial
  state because route extent can change;
- dirty-region paint queries the spatial hash for the dirty world area and paints
  only intersecting prepared nodes/edges;
- selection chrome is a final independent approximately 2px shape-following
  Accent overlay, so node-shape frame styling does not define selection identity.

The spatial hash is the chosen broad phase for this editor workload because
ordinary nodes have broadly similar extents and insert/remove/move operations are
simple and local. Do not add a parallel quadtree/R-tree/BVH unless measured graph
workloads demonstrate a real limitation of the hash. A future GPU renderer should
consume the same model/spatial/prepared-scene boundaries rather than moving model
semantics into a rendering backend.

For shape construction, normal controls can use `UiShapes`, but Graph is the
canonical example of a dense scene that may go directly to `UiGeometry` after
projection to screen pixels. That avoids per-node authored-path allocation while
preserving exactly the same geometry accuracy contract. See
`24_UI_GEOMETRY_CONTRACT.md` and `25_UI_SHAPE_PATH.md`.

## Validation

`Utilities/UiModelViewPerformanceTest` covers the renderer foundation plus List,
Gallery and Table. R2C Windows acceptance established **52 deterministic checks**
in both Debug and Release with `Fails: 0`. Its Table coverage includes:

- 100,000 logical rows and direct row 99,999 reachability;
- bounded cell/header renderer pools;
- Paint-without-renderer-layout;
- narrow cell update without retained-column rebuild;
- 2,000 variable-width columns with direct deep horizontal reach;
- bounded deep horizontal Paint;
- exactly one retained-prefix rebuild for one column resize;
- per-column renderer override without losing bounded pooling.

`Utilities/UiTreeScaleTest` is deliberately separate and contains **11
deterministic checks**. R2C Windows acceptance established all 11 in both Debug
and Release with `Fails: 0`. It covers:

- a flat 100,000-node projection and exact final-row id lookup;
- bounded primary/column renderer pools;
- direct final-node scrolling and viewport-sized Paint;
- renderer layout remaining outside Paint;
- projection/lookup correctness after model change;
- per-column renderer override;
- lazy-loading placeholder lookup correctness;
- zero-child lazy completion removing its placeholder in the same projection
  rebuild.

`Utilities/UiGalleryRegressionTest` is the focused corrective suite for Gallery
capture/theme/presentation behavior. It contains **11 checks** covering bounded
renderers, explicit selection/marquee frames, semantic zoom notification,
Dark-mode viewport styling, Paint-without-relayout and passive `CancelMode()` when
no Gallery marquee is active. At checkpoint `d78f161...`, Windows Debug and Release
both reached 10/11: the sole failure proved that a transparent Minimal List face
was not resolving to a concrete dark Gallery surface. The Panel/Surface fallback
was subsequently corrected and now requires Windows revalidation.

`Utilities/UiNodeGraphScaleTest` now contains **51 deterministic checks**. In
addition to the 10,000-node / 19,800-edge topology and existing bounded prepared
scene, marquee and batch evidence, it verifies that public node/port/edge hit
queries remain spatially bounded and that a live update of the actually-used
`soft` node style class increments neither the full spatial-build serial nor the
full geometry-build serial.

These tests verify geometry, renderer-pool bounds, renderer-layout counts and paint
visit counts rather than fragile elapsed-time thresholds. Windows Debug/Release
build/run results remain the platform acceptance authority.

When a model-view control gains a new interaction path, test whether that path
accidentally reintroduces prefix/full-model traversal. Drag insertion, hover,
selection painting, renderer preparation, lazy preview callbacks, resize and zoom
are common regression points.
