# 06 — Large-scale Views and LOD Guide

This is the scale/performance architecture for `upp_Ui` controls that present
large logical datasets or large spatial scenes.

It applies to List, Gallery, Tree, Table, Graph and future controls such as
timelines, diagram editors, canvases and dense charting surfaces.

Read with:

- `03_UI_MODEL_GUIDE.md` — semantic model ownership and mutation;
- `07_UI_DRAWING_GUIDE.md` — final-pixel drawing and geometry;
- `08_UIGRAPH_GUIDE.md` — the current dense-scene reference implementation.

## 1. Core rule: logical size is not live visual size

A model may contain 100,000 rows or a graph may contain 10,000 nodes without
creating one `Ctrl`, renderer or expensive paint object per logical item.

The view owns a bounded projection:

```text
semantic model
    -> visible / overscan / spatial candidates
    -> bounded prepared presentation
    -> Paint / HitTest
```

Ordinary paint and interaction work must scale with the useful viewport, not the
total model.

Explicit whole-model operations such as Select All, export, filtering or a
structural projection rebuild may be O(N) when that is semantically truthful.

## 2. One semantic authority

The model owns semantic records. The control owns derived view state:

- viewport/scroll/pan/zoom;
- selection, hot, focus and transient gestures;
- visible projection;
- renderer pool;
- prepared geometry;
- spatial indexes;
- raster/cache handles.

Do not add a duplicate item collection just to make a view faster. Optimize the
projection, notification scope, renderer pool or spatial lookup instead.

## 3. Choose the cheapest spatial strategy that matches the layout

### Uniform sequential layouts

List, Gallery and ordinary Table rows/cells should use direct arithmetic:

- visible range from scroll offset and item extent;
- row/cell hit location from division;
- direct jump to deep indexes;
- no spatial tree for a regular grid.

### Hierarchical sequential layouts

Tree may retain a flattened visible projection and direct node-id -> visible-row
lookup. Structural mutations may rebuild that projection; ordinary scrolling
must not.

### Irregular spatial layouts

Graph and future free-form timelines/canvases require a retained spatial broad
phase. UiNodeGraph uses a world-space spatial hash because ordinary graph objects
have broadly similar extents and local mutation is common.

Do not add a quadtree/R-tree/BVH merely because it sounds more advanced. Change
the broad phase only when measured workloads prove the existing structure is the
bottleneck.

## 4. Shared item-render architecture

Model-backed collection views share presentation through
`UiItemRenderData -> UiItemRender`.

`UiItemRenderData` carries ordinary presentation such as title, subtitle,
description, icon/image, right text, value/data and semantic role. It is not a
universal domain model.

`UiItemRender` is a lightweight non-`Ctrl` presentation object. The view owns:

- row/tile/cell/node content rectangle;
- scrolling and viewport;
- hierarchy/topology;
- selection and interaction;
- headers/columns;
- Graph ports/edges and other domain chrome.

The renderer owns prepared presentation **inside the rectangle assigned by the
view**.

Built-in renderers such as Basic/Image can be reused across List, Gallery, Tree,
Table, Dropdown/Menu content and Graph node-content surfaces without forcing
those controls to share domain models.

A renderer prototype is cloned/recycled only for visible useful surfaces.
Render class describes structural composition; semantic role/style describes
appearance. Do not create four renderer classes merely to represent Standard,
Subtle, Accent and Alert.

### Domain chrome remains with the view

Shared content rendering does not absorb control semantics:

- Tree keeps disclosure/hierarchy/drag chrome;
- Table keeps row/column geometry, sorting and editing;
- Menu keeps check/radio/submenu/command behavior;
- Dropdown keeps popup/selection/check/reorder behavior;
- Graph keeps shape, ports, routes and topology.

This keeps the renderer reusable without turning it into another universal view
framework.

## 5. Renderer pooling and prepared layout

`UiItemRender` is a recyclable non-`Ctrl` presentation object.

Large item views create only enough renderer instances for visible/overscan
surfaces. Renderer `Layout()` happens when geometry-affecting inputs change;
`Paint()` and `HitTest()` consume prepared geometry.

Normal paint must not:

- measure every logical row;
- rebuild renderer layout;
- mutate model state;
- decode/load assets;
- start timers;
- emit semantic callbacks.

A genuinely interactive embedded `Ctrl` remains an escape hatch, but it is
attached only for the small useful visible set. Never restore one child
`Ctrl` per ordinary logical item.

## 6. Mutation work must match mutation scope

A model revision is not permission to rebuild the whole view.

Use the narrowest correct reaction:

- presentation-only update -> rebind/invalidate affected prepared renderers;
- uniform List/Gallery item update -> no grid geometry rebuild;
- Tree ordinary item update -> no flattened projection rebuild;
- Graph local node/style update -> affected prepared node and incident routes;
- structural insert/erase/move/reset -> rebuild only the derived structure that
  can actually change.

Bulk semantic operations should publish bounded bulk notifications instead of a
large stream of tiny events where the model already knows the affected range.

## 7. Identity under mutation

Different models have different truthful identities:

- List/Gallery/Dropdown — sequential index state, with stable application keys in
  item data where identity must survive a complete reorder/projection reset;
- Tree/Menu/Graph — stable node/edge references;
- Table — current public selection is coordinate/range based;
- UiDoc — document positions/anchors remapped by document transactions.

Do not force one identity scheme onto every control.

## 8. LOD is a view policy, not model data

LOD reduces presentation work as projected information disappears. It must not
mutate semantic model content.

Think in three separate layers:

### A. Population LOD

Decides **which semantic objects need presentation**.

Examples:

- visible/overscan List rows;
- spatial Graph candidates;
- Graph overview connector representatives at extreme zoom.

Population LOD never deletes or rewrites model objects.

### B. Presentation LOD

Decides **which visual information remains useful**.

Examples:

- rich text -> title only -> no text;
- ports/labels disappear at different projected scales;
- shadows and decorative detail disappear before topology;
- connector labels/arrows disappear before connector identity.

Thresholds should be staggered so the scene simplifies progressively rather than
popping between two complete modes.

### C. Geometry LOD

Decides **how much explicit curve detail can affect final pixels**.

This is not a per-control zoom threshold. It is the shared final-device-pixel
contract from `07_UI_DRAWING_GUIDE.md` / `UiGeometry`.

A semantic route can remain present while its explicit projected curve geometry
uses fewer vertices automatically.

## 9. Screen-space significance

The decisive quantity is projected pixels.

A control may use authored world units and DPI-scaled metrics, but expensive
visual decisions happen after projection:

```text
authored/model coordinates
    -> DPI / view transform
    -> final device pixels
    -> presentation + geometry decision
```

For a 10-pixel object, drawing 100-pixel-quality internal detail is wasted work.

LOD identity must remain stable: a diamond should remain recognisably a diamond,
a connector should remain attached to the same endpoints, and selection must
continue to represent the same semantic object.

## 10. Retained scene and camera reuse

A camera/view change is not automatically a scene rebuild.

For a spatial control, retain semantic/spatial/prepared scene state where it is
safe and move the camera cheaply:

- live pan may translate already prepared projected geometry while retained
  world coverage remains valid;
- live wheel zoom may project prepared geometry about the pointer anchor while
  LOD/coverage constraints remain compatible;
- after interaction quiet, perform one exact settle rebuild;
- unsafe coverage or LOD transition -> immediate exact fallback.

Programmatic APIs such as `SetZoom`, `SetPan`, Fit and host-driven setup should
remain synchronous/exact unless their public contract explicitly says otherwise.

This retained-scene/camera split is also the correct boundary for any future GPU
backend.

## 11. Dirty regions and hit testing

Paint and hit testing should use the same retained broad-phase authority.

- dirty-region paint queries only the intersecting useful world/screen area;
- point hit tests query a small candidate neighborhood, then exact-test;
- marquee selection queries intersecting spatial cells;
- very large marquee previews may defer expensive candidate presentation until
  release.

Do not maintain a second full prepared-viewport scan merely for public hit-test
APIs.

## 12. Expensive assets

Prepare expensive images/resources at the visible-range seam.

Typical Gallery flow:

```text
domain/catalog
    -> cheap model rows with stable key
    -> visible + overscan notification
    -> bounded image preparation/cache
    -> update visible rows
    -> one ranged model notification
```

Do not eagerly decode thousands of assets.

## 13. Dense-scene drawing rule

Normal controls should use `UiShapes`/`UiShapePath` for reusable authored
silhouettes.

Dense scenes such as Graph may go directly to `UiGeometry` after projection to
final pixels when creating an intermediate authored-command object per item would
only add allocation/work.

The error contract is identical either way. See `07_UI_DRAWING_GUIDE.md`.

## 14. UiNodeGraph as the current reference

UiNodeGraph demonstrates the complete dense-scene pattern:

- 10,000-node deterministic fixture;
- retained world spatial hash;
- bounded prepared node/edge population;
- direct micro-node Draw scene when projected nodes are physically tiny;
- rich details/ports omitted in micro mode;
- overview connector population reduction at extreme zoom;
- adaptive final-pixel route geometry;
- one exact geometry settle after reusable live camera movement;
- profiling counters for candidates, prepared, painted, geometry and phase work.

The lesson is generic; Graph-specific topology and editing are documented in
`08_UIGRAPH_GUIDE.md`.

## 15. Idle means idle

A static view with no animation, mutation or invalidation should not continuously
repaint.

Continuous idle CPU is a defect even when each individual paint is optimized.
Before adding caches or a new renderer, find who is issuing Refresh/repaint or
keeping a frame/timer loop alive.

Animation is the exception and must have explicit ownership and stop conditions.

## 16. Profiling rules

Profiling must be observer-only.

When disabled, diagnostics should return before constructing strings, comparing
large state, scheduling UI updates or performing extra geometry work.

Prefer structural evidence over machine-specific timing thresholds:

- candidate/prepared/painted counts;
- geometry-build serials;
- spatial-index build/update counts;
- renderer-layout counts;
- LOD counts;
- path-vertex counts.

Elapsed microseconds remain useful evidence but are not a portable correctness
contract.

## 17. New high-scale control checklist

Before publishing a large-data control, answer:

1. What owns semantic data?
2. What is the view projection?
3. Is layout uniform arithmetic, hierarchical projection or irregular spatial?
4. What is the bounded visible/overscan/prepared population?
5. What work happens on a local update?
6. What work happens on a structural update?
7. Which presentation details disappear as projected pixels shrink?
8. Does explicit geometry use the shared final-pixel contract?
9. Can pan/zoom/scroll reuse retained preparation?
10. Does Paint perform layout, model work, loading or allocation that can move
    outside Paint?
11. Do hit tests use the same bounded broad phase?
12. Does the view become truly idle when nothing changes?
13. Can diagnostics prove all of the above without changing behavior?

If those answers are clear, a timeline, graph, tree, gallery or another
large-scale view can share the same architecture without copying UiNodeGraph
internals.
