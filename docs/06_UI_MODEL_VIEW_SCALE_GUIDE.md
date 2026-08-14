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
2. **Ordinary model items are not child `Ctrl`s.** List rows, gallery tiles, table
   cells, tree rows and normal graph nodes are painted by the owning view. A child
   control is reserved for genuinely interactive embedded content.
3. **Renderers are also virtualized.** `UiItemRender` is a cheap non-`Ctrl`
   presentation object, but high-scale views still create/recycle only enough
   renderer instances for visible/overscan surfaces rather than one per logical
   record.
4. **Viewport work is bounded.** Normal paint, hover and hit testing must be
   proportional to visible/overscan content, not the index of the visible item or
   the total item count.
5. **Geometry has a direct fast path.** Uniform rows/cells use arithmetic mapping
   between logical index, scroll position and screen rectangle. More complex
   views may use retained geometry or spatial indexes, but must not rescan the
   entire model for every pointer move or repaint.
6. **Renderer layout is prepared outside Paint.** A renderer may cache internal
   image/text/action rectangles, but its virtual `Layout()` is reached through
   dirty-gated preparation on layout/model/theme/geometry changes. `Paint()` and
   `HitTest()` consume prepared geometry only.
7. **Model changes carry useful scope.** `UiModelChange` insert/erase/update/move
   ranges should be used to avoid full refresh/rebuild when a narrower response
   is truthful. A reset may legitimately invalidate the whole projection.
8. **Lazy assets follow the viewport.** Views may expose visible/overscan ranges;
   owners/providers prepare thumbnails or other expensive data for that bounded
   range and refresh the relevant item when ready.
9. **Bulk user operations may be bulk.** Explicit operations such as Select All,
   a marquee spanning a very large logical range, copying/exporting/filtering or
   rebuilding a changed tree can be O(N). Ordinary scrolling and painting may not
   be O(N).
10. **Large logical extent must not overflow geometry.** Shared extent helpers use
    wide intermediates and saturate where the native control geometry remains
    integer-based.

The engineering acceptance target for uniform List/Gallery views is at least
100,000 logical items with viewport cost independent of total model size. This is
an invariant target, not a wall-clock benchmark tied to one machine.

## Shared model-view geometry

`UiModelView.h` contains small pure helpers shared by high-scale views:

- `UiVisibleRange`
- `UiUniformContentExtent(...)`
- `UiComputeLinearVisibleRange(...)`
- `UiComputeGridVisibleRange(...)`
- `UiComputeUniformInsertBefore(...)`

These helpers deliberately do not create an abstract view hierarchy. The Ui
control family shares a performance and model contract without forcing List,
Gallery, Table, Tree and Graph through one inheritance design.

## Shared item rendering

`UiItemRenderData` is the common presentation payload and `UiItemRender` is the
shared presentation object. Built-in `UiItemRenderBasic` and
`UiItemRenderImage` support horizontal and vertical composition and follow the
normal Ui theme/role/style lifecycle.

A renderer instance belongs to one currently prepared visible surface. List and
Gallery own bounded renderer pools, rebind those slots as logical indexes enter
the useful range, and retain the model as the sole semantic authority. A renderer
prototype can be replaced with `SetItemRender(...)` without changing the model or
view interaction rules.

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

The geometry vocabulary follows `UiGridLayout` where the concepts are genuinely
the same: item size, gap, inset and width-derived column count. `UiGridLayout`
remains a layout engine for real child controls; `UiGallery` remains a model view.

`SetItemRender(...)` is the presentation extension point. Selection, scrolling,
zoom, marquee selection, model ownership and useful-range calculation remain
Gallery responsibilities. `SetZoom()`/`ZoomBy()` alter the uniform cell geometry;
on Windows, Ctrl+wheel routes to Gallery zoom. Cheap grid arithmetic is not
throttled—expensive thumbnail/data preparation should instead follow the bounded
useful range.

Multi-select background marquee maps the band to row/column spans arithmetically.
Plain drag replaces selection, Ctrl drag toggles against the opening selection,
Shift drag extends/adds from the opening selection, Escape restores the opening
selection, and edge drag autoscrolls. A deliberately huge marquee may select a
huge number of logical items and is therefore an explicit bulk operation.

Variable-height/masonry layout is intentionally outside the current high-scale
contract. If added later it must declare its geometry/cache cost rather than
weakening the uniform fast path.

## UiList

`UiList` uses a uniform row extent. Its normal paint range and drag insertion
position are derived directly from scroll position and row extent; it does not
start at row zero and walk forward until the viewport is reached.

Visible rows are presented through a bounded horizontal `UiItemRender` pool. The
default is `UiItemRenderBasic`; callers can replace the renderer prototype without
changing model or selection ownership. A single visible-row `UI_MODEL_UPDATE`
rebinds/relayouts only the affected prepared renderer while insert/erase/move/reset
may require broader row-position/layout work.

## UiTable

`UiTable` is the explicit row/column data surface. Its high-scale contract is the
same: determine visible row/column spans first and paint that intersection.
Variable column widths may use retained prefix geometry rather than repeated
total-model traversal. The R2 rendering plan also makes cell and column-header
presentation natural future `UiItemRender` slots without turning each cell/header
into a `Ctrl` or independent logical model record.

The current table remains a separate scale-audit target: its model/view design is
already direct-painted, but deep row/column traversal must continue toward the
same bounded-viewport rule before claiming hundred-thousand-row acceptance.

## UiTree

`UiTree` owns a retained `visible_rows_` projection because expansion changes the
linear view of hierarchical data. Rebuilding that projection after a structural
change can legitimately be O(number of visible nodes). Ordinary scrolling and
painting over an unchanged projection should derive the first visible row
arithmetically rather than scanning from projection row zero.

Tree hierarchy chrome remains Tree-owned. Primary row content and additional
columns can become separate renderer surfaces for the same logical node; they do
not become separate model nodes. The current tree remains a separate scale-audit
target for ordinary deep paint traversal and node-id-to-visible-row lookup.

## UiNodeGraph

`UiNodeGraph` already follows the retained-scene philosophy: normal graph objects
are painted rather than represented by one `Ctrl` each, stable graph IDs live in
`UiGraphModel`, and geometry/damage can be rebuilt narrowly for changed nodes and
edges.

Different node classes may later choose different `UiItemRender` presentations
for node content while Graph retains shape, ports, edges, world geometry and
interaction. Very large spatial graphs still require viewport culling/spatial
lookup so pan, zoom, paint and hit testing depend on spatially relevant objects
rather than blindly scanning the entire graph.

## Validation

`Utilities/UiModelViewPerformanceTest` uses deterministic 100,000-item probes.
The tests verify geometry, renderer-pool bounds, renderer-layout counts and paint
visit counts rather than fragile elapsed-time thresholds. They prove that:

- jumping to the final List/Gallery item does not visit the preceding 99,999
  records during ordinary paint;
- live renderer count remains bounded by viewport/overscan rather than logical
  model size;
- unchanged Layout reuses prepared renderer geometry;
- Paint does not invoke renderer layout;
- a narrow visible model update rebinds/relayouts at most the affected visible
  renderer;
- Gallery zoom/reflow preserves bounded renderer counts and final-item reachability.

When a model-view control gains a new interaction path, test whether that path
accidentally reintroduces prefix/full-model traversal. Drag insertion, hover,
selection painting, renderer preparation, lazy preview callbacks, resize and zoom
are common regression points.
