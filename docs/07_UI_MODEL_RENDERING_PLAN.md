# 07 — Ui Model Rendering Plan

Architecture plan for a unified, theme-aware, high-performance presentation layer
across model-backed `upp_Ui` controls.

This plan complements:

- `00_UPP_CODING_GUIDE.md` — repository engineering and paint/layout rules;
- `02_UI_THEME_GUIDE.md` — theme roles, palettes, metrics, skins and runtime theme revisions;
- `03_UI_MODEL_GUIDE.md` — model ownership and request-first mutation;
- `06_UI_MODEL_VIEW_SCALE_GUIDE.md` — bounded viewport work and 100,000-item acceptance.

The objective is not compatibility with the current first-pass APIs. The objective
is one clean architecture that is easy to use, easy to explain, and remains fast
when a logical model contains tens or hundreds of thousands of records.

## Decisions locked by this plan

1. `UiItemRenderData` is the shared presentation payload.
2. `UiItemRender` is the shared non-`Ctrl` presentation object.
3. `UiItemRender` owns its prepared per-visible-item layout privately. There is no
   public `UiItemRenderSpec` or `UiItemRenderLayout` abstraction.
4. `Layout()` is explicit and virtual. It runs only when layout-affecting inputs
   change. `Paint()` never performs layout.
5. Views own geometry, selection, scrolling, hierarchy/topology and interaction.
   Renderers own presentation inside the rectangle assigned by the view.
6. Logical model count is independent of live renderer count. High-scale views
   create/recycle renderers only for visible/overscan surfaces.
7. Headers, cells, list rows, gallery tiles, tree columns and graph-node content
   use the same renderer concept. One logical model record may produce several
   render surfaces without becoming several logical model records.
8. Built-in renderers are theme-aware first-class citizens. They use
   `StyledPalette`, `StyledMetrics`, `StyledSkin`, `UiRole` and `UiTheme`; they do
   not hard-code a parallel colour/style system in `Paint()`.
9. Geometry generation follows the shared final-pixel contract:
   normal controls/renderers may use `UiShapes`/`UiShapePath`; dense scenes may
   use `UiGeometry` directly when the authored-path layer would be overhead.
9. Real child `Ctrl`s are an exceptional escape hatch for genuinely interactive
   embedded content. They are never the default representation of large models.
10. Existing APIs may be replaced rather than shimmed when the new architecture
    is implemented. Update production callers, demos and tests in the same pass.

## 1. UiItemRenderData — common presentation data

`UiItemRenderData` describes information that can be presented in many views. It
is not a model by itself and it does not own view interaction.

The initial payload should remain deliberately small and semantic, approximately:

```cpp
struct UiItemRenderData : Moveable<UiItemRenderData> {
    String title;
    String subtitle;
    String description;
    String right_text;

    Image image;
    Image icon;
    UiIconRenderMode icon_render_mode = UiIconRenderMode::PreserveColor;

    Value value;
    Value data;

    UiRole role = UiRole::Standard;
    bool enabled = true;

    bool has_metadata = false;
    Color metadata_color = Null;
};
```

The exact initial fields are finalized during implementation after sweeping the
current List/Gallery/Tree/Menu/Table/Graph needs. Do not turn the structure into a
kitchen-sink replacement for every domain model.

Domain-only state remains in the domain model/view:

- Tree: parent/children, lazy state, expansion and hierarchy;
- Table: row/column address, editability, validation state and sorting;
- Menu: command id, submenu, radio/check semantics and action policy;
- Graph: position, size, shape, ports, edges and topology;
- List/Gallery: selection/reorder policy, grouping or view-specific interaction.

The current broad `UiModelItem` should be reviewed as part of this migration. Its
presentation fields naturally move toward `UiItemRenderData`; view/domain-specific
flags should move back to the owning model/view instead of surviving only for
compatibility.

### Composition, not inheritance

Domain records contain or expose render data. They do not inherit a universal
model-item base merely to share presentation.

```text
UiListItem    ─┐
UiTreeNode    ─┤
UiTableCell   ─┤
UiTableHeader ─┼─> UiItemRenderData -> UiItemRender
UiMenuItem    ─┤
UiGraphNode   ─┘
```

This keeps each model meaningful while making its ordinary visual content
interchangeable.

## 2. UiItemRender — one small reusable presentation object

`UiItemRender` is not a `Ctrl`. It is a cheap, recyclable presentation object for
one currently bound visible surface.

The base contract should remain compact:

```cpp
class UiItemRender {
public:
    virtual ~UiItemRender() {}

    virtual One<UiItemRender> Clone() const = 0;

    virtual void SetData(const UiItemRenderData& data);
    virtual void Layout(Rect rect, UiDirection direction) = 0;

    virtual Size GetContentSize() const = 0;
    virtual Size GetMinSize() const = 0;

    virtual void Paint(Draw& w, const UiItemRenderState& state) const = 0;
    virtual UiItemRenderHit HitTest(Point p) const;
};
```

The exact names/signatures may be adjusted for U++ conventions during coding, but
these responsibilities are the contract. Do not add another public geometry
object unless a concrete implementation proves it necessary.

### Renderer instances own prepared layout

A renderer such as `UiItemRenderImage` privately retains whatever rectangles it
needs:

```text
bounds
image rect
icon rect
title rect
subtitle/description rect
right text rect
painted action hit rects
```

`Layout()` calculates them. `Paint()` and `HitTest()` consume them.

No view duplicates renderer-internal hit geometry, and no renderer calculates
layout from inside `Paint()`.

### Layout invalidation

A visible renderer is relaid out when a layout-affecting input changes, including:

- it is rebound to a different model record;
- its allocated rectangle changes;
- List/Tree/Table column geometry changes;
- Gallery tile size or orientation changes;
- Graph node content rectangle or zoom-dependent presentation changes;
- renderer class/configuration changes;
- font, metric, skin/content inset or another geometry-affecting style changes;
- relevant model data changes enough to affect measurement/layout.

The following normally repaint without relayout:

- hot;
- pressed;
- selected;
- focused;
- palette-only state changes.

Theme changes must distinguish paint-only palette changes from geometry-affecting
font/metric/skin changes when practical. Correctness wins over micro-optimization:
a theme revision may conservatively relayout the bounded visible renderer pool,
never the entire logical model.

### GetContentSize and GetMinSize

The distinction follows the wider Ui layout contract:

- `GetContentSize()` reports the renderer's natural presentation size for its
  current data/configuration;
- `GetMinSize()` reports the smallest useful presentation;
- the owning view still decides the rectangle actually allocated to a row, tile,
  cell or node content area.

Uniform high-scale views must not call either method for all 100,000 records as a
side effect of normal scrolling. Uniform List/Gallery geometry remains arithmetic.
Measurement is for visible content, explicit fit/auto-size modes, or deliberately
non-uniform future layouts with a declared retained-geometry strategy.

## 3. Renderer pooling and the 100,000-item rule

A renderer is lightweight, but high-scale views still do not allocate one per
logical record.

```text
100,000 model records
        |
        v
view computes visible + overscan range
        |
        v
~40–100 live UiItemRender instances
        |
        v
Layout only when dirty -> Paint / HitTest
```

The view owns a recycled renderer pool. When a surface leaves the useful range its
renderer is rebound/reused for another visible surface.

The configured renderer supplied to a view acts as a prototype. `Clone()` (or an
equivalent U++-native creation mechanism chosen during implementation) creates
cheap per-visible instances while preserving configured renderer style/options.

No ordinary caller should have to understand the pool.

## 4. Built-in renderers and default experience

Initial built-ins:

- `UiItemRenderBasic` — compact icon/text/subtitle/description/right-content
  composition, naturally suited to horizontal rows and simple tiles;
- `UiItemRenderImage` — image/thumbnail-forward composition, naturally suited to
  vertical Gallery tiles but also usable horizontally.

Both support `UiDirection::H` and `UiDirection::V`. Orientation changes the
composition inside the same renderer class; it does not require separate
Horizontal/Vertical subclasses.

The normal experience must "just work": every model-backed control receives a
sensible built-in renderer without requiring explicit setup. Advanced callers
replace only the render slot they care about.

Representative API intent:

```cpp
list.SetItemRender(my_render);

gallery.SetItemRender(my_render);

table.SetCellRender(cell_render);
table.SetHeaderRender(header_render);

tree.SetItemRender(primary_render);
tree.SetColumnRender(1, status_render);

graph.SetNodeRender(default_render);
graph.SetNodeRender("image", image_render);
```

Exact fluent names are finalized against the existing control vocabulary. Avoid
aliases and compatibility duplicates.

## 5. Multiple columns and headers

One logical record may create several render surfaces. This is a presentation
fact, not a reason to split the model record.

### Tree

Tree owns:

- row and column rectangles;
- indentation and hierarchy depth;
- connector lines;
- disclosure/loading glyphs;
- expansion, selection and drag/drop.

The primary content rectangle and each data-column rectangle are handed to one
renderer instance each. A visible node with three columns may therefore use three
recycled renderers while remaining one logical Tree node.

Tree hierarchy chrome stays outside `UiItemRender`; otherwise the common renderer
would become contaminated with parent/sibling/topology rules.

### Table / matrix-like tabular views

Headers and cells are both render surfaces.

Table owns row/column geometry, selection, resize, sort and editing. It supplies:

- a default cell renderer;
- a default column-header renderer;
- a default row-header renderer where used;
- optional per-column renderer overrides.

A header is not a special paint subsystem. It is `UiItemRenderData` presented by
a configured header renderer. The supplied default header renderer can be
`UiItemRenderBasic` with header-appropriate theme/style configuration.

`UiMatrixSelector` remains a small direct-painted selector rather than a large
model view, but its individual cells may reuse the item-render content/presentation
contract where that removes duplicated icon/text layout cleanly. Matrix still owns
pair arrows, default indication and matrix selection semantics.

### Multi-column List

If List exposes columns/headers, it follows the same rule as Tree/Table: the row
is one logical item, the view creates one render surface per visible column, and
selection normally remains row-level unless an explicit cell-selection mode is
introduced.

## 6. Graph nodes and heterogeneous render classes

`UiNodeGraph` keeps topology and spatial geometry authoritative:

- node position/size/shape;
- ports and connection anchors;
- edges/routes;
- z-order;
- graph hit paths;
- node drag, pan, zoom and marquee.

The node's content region is rendered through `UiItemRender`.

Different node categories may use genuinely different render classes:

```text
standard process node -> UiItemRenderBasic
image/asset node       -> UiItemRenderImage
value node             -> specialized custom renderer
technical node         -> specialized custom renderer
```

A graph node may carry a small presentation key such as `render_class`. The graph
maps that key to configured renderer prototypes. Keep renderer class and style
class separate:

- render class = structural composition;
- style class / `UiRole` = appearance/semantic emphasis.

Thus an `image` renderer may still be `Standard`, `Subtle`, `Accent` or `Alert`
without duplicating four render classes.

Graph ports remain Graph-owned interaction/topology, not generic item actions.
Graph's retained node geometry may contain/use the renderer's prepared internal
geometry lifecycle, but Graph should not build a competing generic item-layout
system.

## 7. Painted interaction before real child controls

Ordinary tile/row actions do not require a `Ctrl`.

`UiItemRender::HitTest()` can return a small `UiItemRenderHit` describing a
painted action part such as:

- star/favourite;
- overflow/menu icon;
- close/remove affordance;
- status/action glyph;
- other renderer-owned clickable decoration.

The owning view keeps transient hot/pressed item+part state and emits the
appropriate request/action event. Painted parts use the same prepared rectangles
as hit testing.

Editing follows the existing List/Table precedent: one transient editor is placed
over the active visible target, committed/cancelled, then reused/removed. Do not
allocate an editor per logical item.

### Real Ctrl escape hatch

A real embedded `Ctrl` is reserved for content that cannot reasonably be expressed
as paint + hit regions + transient editing, for example a rich multi-control
editor.

When this is required, add one visible-control recycling provider that binds only
the visible/overscan targets. Do not restore a parallel "widget version" of List,
Gallery or Tree. The model/render path remains the one normal architecture.

Implement the control provider only when a concrete client requires it; document
the contract now so the renderer design does not close the door.

## 8. Theme and styling are first-class

Built-in item renderers must use the existing Ui style system rather than invent a
renderer-specific palette language.

### Shared primitives

Renderer styles use:

- `StyledPalette` for normal/hot/pressed/disabled face/frame/ink/icon state;
- `StyledMetrics` for radius, frame width, content margin, focus, shadow and
  geometry-affecting settings;
- `StyledSkin` for image-backed/nine-slice surfaces where appropriate;
- `UiFill` and existing gradient/image helpers rather than a new Gallery/List
  gradient mechanism;
- `UiIconRenderMode` for icon tint/preserve-colour behaviour.

### Semantic role

`UiItemRenderData::role` uses the existing universal `UiRole`:

```text
Standard
Subtle
Accent
Alert
```

Built-in renderer styles resolve these roles through `UiTheme`. Roles express
semantic emphasis, not renderer structure.

Do not store arbitrary application RGB styling in ordinary model data when a
semantic role or renderer style is sufficient. Explicit colour data is retained
only when the colour itself is semantic content (for example a colour swatch or
metadata colour).

### Theme lifecycle

The owning view already observes `UiTheme` revision changes. It is responsible for
synchronizing its configured renderer prototypes/live pool with the current theme.

- theme-driven renderer -> re-resolve from current preset/mode;
- explicit renderer custom style -> preserve authored override;
- palette-only change -> repaint bounded live pool;
- metric/font/skin geometry change -> relayout bounded live pool then repaint.

There is no theme resolution or style mutation inside `Paint()`.

Light and Dark modes must be covered by deterministic/default-style tests and by
the visual demo. Built-in renders should require no application changes when the
theme switches.

### Slot-specific defaults

A control may configure the same renderer class differently for different slots:

- ordinary item/cell: Standard body style;
- header: stronger font and header-appropriate surface/role;
- selected item: state palette selected by view state;
- Graph node: node-surface/content style plus semantic role.

This keeps "header render" and "cell render" explicit without creating separate
header-only rendering infrastructure.

## 9. Control adoption plan

### Stage R2A — foundation

Introduce the shared renderer/data layer and deterministic headless/layout tests:

- `UiItemRenderData`;
- `UiItemRender`;
- `UiItemRenderState` / `UiItemRenderHit` as small value structs;
- `UiItemRenderBasic`;
- `UiItemRenderImage`;
- theme/style resolution for built-ins;
- renderer clone/rebind/layout invalidation tests.

Do not migrate every control in this first commit. Publish the foundation as one
recoverable checkpoint.

### Stage R2B — List + Gallery reference implementation

Migrate List and Gallery completely onto the shared renderer architecture.

- same model data can be displayed by both controls;
- List defaults to horizontal Basic presentation;
- Gallery defaults to a sensible tile presentation and can switch to Image;
- no per-logical-item renderer allocation;
- existing direct viewport geometry remains bounded;
- remove superseded one-off item painters rather than keeping compatibility
  aliases;
- retain a narrow custom renderer hook rather than a competing paint callback if
  both would provide the same authority.

Update the demo to show one 10,000-item model in List and Gallery simultaneously,
using the same render data and configurable renderer. Generate a tiny deterministic
pool (about 64) of small reusable shape images and cycle them through the model;
do not allocate 10,000 unique demo images or fake disk I/O.

Keep deterministic scale acceptance at 100,000 logical items.

Gallery R2 interaction follows on this foundation: marquee selection and semantic
zoom must preserve viewport-bounded ordinary work. Rapid zoom may coalesce
expensive preview preparation, but cheap grid geometry should remain responsive.

### Stage R2C — Tree + Table scale/render integration

Tree:

- use the shared renderer for primary content and additional visible columns;
- retain hierarchy/disclosure/drag chrome in Tree;
- start ordinary Paint directly at the visible projection row;
- add/retain a node-id -> visible-row lookup so deep operations do not repeatedly
  scan the projection;
- add column/header render slots where the control exposes headers.

Table:

- use shared header/cell render slots;
- support per-column renderer overrides;
- compute visible rows directly;
- retain/prefix column geometry so deep horizontal paint does not rescan from
  column zero;
- keep one transient editor for the active cell.

Both gain 100,000-row deterministic viewport tests before claiming high-scale
acceptance.

### Stage R2D — Dropdown + Menu model cleanup

Dropdown should converge on one authoritative model rather than maintaining a
parallel item collection plus model mirror. Reuse the List/item renderer for popup
rows and the collapsed presentation where appropriate.

Menu keeps its menu-specific model semantics but uses shared item-render content
for ordinary icon/text/description/right-content composition. Menu owns
check/radio/submenu and command interaction chrome.

### Stage R2E — Graph render classes + spatial scale

Integrate item renders into graph node content, preserving existing node-surface,
port and edge authority. Add named render-class resolution for heterogeneous node
content.

Separately harden very large graphs with spatial culling/indexing so paint, pan,
zoom and hit testing depend on spatially relevant graph objects rather than total
node count. Do not claim the List/Gallery 100,000 invariant for Graph until that
spatial acceptance exists.

### Stage R2F — selective small-control reuse

Review `UiMatrixSelector`, `UiTitleCard` and `UiAccordion` for shared content
rendering where it removes duplicated icon/text layout cleanly.

Do not force large-model virtualization into controls whose purpose is a small
composite:

- Matrix remains one lightweight selector Ctrl;
- Accordion remains a rich small/medium composite with real section bodies;
- Accordion/TitleCard headers may reuse item-render presentation without turning
  arbitrary accordion bodies into virtual model rows.

## 10. Public usability target

Normal callers should not need renderer machinery to obtain a good control:

```cpp
UiList list;
list.SetModel(model);       // immediately useful with default render

UiGallery gallery;
gallery.SetModel(model);    // immediately useful with default render
```

Customization should be local and obvious:

```cpp
gallery.SetItemRender(image_render);

table.SetCellRender(cell_render);
table.SetHeaderRender(header_render);

tree.SetColumnRender(2, status_render);
```

The implementation may expose advanced render-class registration for Graph and
other heterogeneous views, but this must not make the common List/Gallery/Table
case feel like a framework configuration exercise.

Documentation and demos must show:

1. default, no-configuration usage;
2. replacing one renderer;
3. horizontal and vertical presentation;
4. multiple columns/header render slots;
5. custom painted action hit regions;
6. theme switch Light/Dark with no model changes;
7. 10,000-item visual example and 100,000-item deterministic acceptance.

## 11. Performance and correctness acceptance

For high-scale model views:

- logical model count does not determine live `Ctrl` count;
- logical model count does not determine live `UiItemRender` count;
- normal paint visits only visible/overscan render surfaces;
- ordinary hit testing does not scan the total model;
- `Paint()` performs no layout, model mutation, event emission, loading or image
  generation;
- renderer layout runs only for dirty visible/overscan surfaces;
- selection/hot/pressed changes do not relayout unless the renderer explicitly
  declares geometry-changing state, which built-in defaults should avoid;
- theme/style changes invalidate only what is required and never require
  constructing renderers for off-screen logical items;
- direct jump to the final item remains bounded;
- multiple columns multiply only visible render work, not total-model work;
- custom renderer and per-column/per-node render-class paths receive the same
  scale tests as defaults.

Explicit bulk operations such as Select All, export, filter/projection rebuild or
Tree structural rebuild may remain O(N) where that is semantically truthful.

## Geometry and silhouette construction

Renderer architecture does not own a second geometry system.

- direct `Draw`/native Painter remains first choice for simple paint;
- reusable normal-control silhouettes come from `UiShapes`;
- custom authored silhouettes use `UiShapePath`;
- explicit adaptive geometry comes from `UiGeometry`;
- dense/high-count views may bypass `UiShapePath` and use `UiGeometry`
  directly;
- no renderer or control may introduce a private sample-count/curve-quality
  policy.

This keeps presentation reuse and scale compatible. See
`24_UI_GEOMETRY_CONTRACT.md` and `25_UI_SHAPE_PATH.md`.

## 12. Implementation discipline

Before each stage:

1. refresh remote `main` and record exact HEAD;
2. inspect complete touched headers/sources/tests/package files;
3. make the smallest coherent clean migration, not a compatibility shim;
4. review full diff and package dependency direction;
5. publish a recoverable checkpoint and verify remote;
6. update `docs/ACTIVE_WORK.md`;
7. run deterministic tests available in the current environment;
8. hand Windows/U++ Debug+Release validation to Gary only after static review.

The architecture is considered successful when model-backed controls are easier to
construct than child-widget equivalents, default styling follows UiTheme in Light
and Dark modes, custom presentation is replaced through a small renderer slot API,
and high logical item counts remain an ordinary supported case rather than a
special demo trick.
