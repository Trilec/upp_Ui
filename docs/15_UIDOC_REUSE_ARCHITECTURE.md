# UiDoc reusable document-engine architecture

## Status

Architecture investigation only. This document records a verified extraction boundary for future work; it does **not** claim that the existing `UiDoc : Ctrl` implementation has already been refactored.

Audit base:

```text
cccda9916e32fd9bd8d3a0d041cf10e3848546f7
```

Remote `main` remains authoritative. Refresh before implementation because unrelated Ui work may advance concurrently.

## 1. Finding: UiDoc is not built on U++ RichText

The current Ui package does not depend on the U++ RichText/RichEdit packages. `Ui/Ui.upp` uses:

```text
Core
Draw
Painter
CtrlCore
Animation
CtrlLib
```

The rich-document authority is already custom:

```text
UiDocCore
```

`UiDocCore` is explicitly a non-visual rich-document model shared by `UiDoc` and headless consumers. It owns logical document state and deterministic semantic mutation rather than screen/control state.

This means the useful direction is **not** to replace RichText with another editor. The useful direction is to finish separating the existing UiDoc document engine from the `Ctrl` host so its mature layout, rendering and editing capabilities can be reused by other Ui controls and headless/agent workflows.

## 2. Existing separation that must be preserved

### `UiDocCore` — document authority

Keep `UiDocCore` as the single logical document authority. It already covers the important reusable state and operations, including:

- text and sparse style runs;
- semantic blocks;
- annotations/comments and metadata;
- resources and embeds;
- inline runs and images;
- tables;
- anchors;
- transactions and revision/history behavior;
- serialization/import/export surfaces;
- deterministic range-based mutations usable by UI commands, importers and agents.

Do not create a second document model for a renderer or compact field.

### `UiDoc : Ctrl` — current combined host/view/editor

The current `UiDoc` control still owns several concerns that are logically reusable but private to the Ctrl today:

- retained paragraph/glyph/table/embed geometry;
- paragraph height/index caches and glyph measurement caches;
- visible-only layout preparation;
- document-position <-> screen-position mapping;
- caret geometry and hit testing;
- document painting;
- caret/selection/typing-style/table-selection editing session state;
- semantic text/table editing commands;
- Ctrl-specific focus, keyboard/mouse dispatch, clipboard, capture, scrolling and repaint scheduling.

The extraction should separate these concerns without discarding the existing implementation.

## 3. Verified source boundaries

The audit inspected the complete current slices below.

### Layout and retained geometry

`UiDocLayout.cpp` and `UiDocParagraphLayout.cpp` already behave like a reusable view engine:

- glyph measurement cache;
- content-width and paragraph-height estimates;
- paragraph index and retained top offsets;
- binary paragraph lookup by Y;
- paragraph line wrapping;
- font/style resolution;
- block-role presentation such as lists, quote and screenplay forms;
- inline image/embed layout;
- table layout;
- metadata-card layout;
- visible-window preparation.

These operations fundamentally need `UiDocCore`, style/measurement inputs and a viewport/content width. They do not inherently require focus, mouse capture or a child `Ctrl` per document.

### Geometry and hit testing

`UiDocGeometry.cpp` provides reusable document-view operations:

- visible layout window resolution;
- point -> document position;
- document position -> point;
- caret rectangle;
- table/embed hit testing;
- viewport/page offset transforms.

The reusable version should receive viewport/page origin explicitly instead of reading control-owned scroll state implicitly.

### Painting

`UiDocPaint.cpp` consumes the retained visual records and `UiDocCore` to paint:

- text and styled runs;
- selection;
- search hits;
- annotations;
- inline images/embeds;
- tables and document chrome.

Most of this needs `Draw`, prepared layout and presentation state, not `Ctrl` ownership. Resource/image decoding should become a host/provider callback so the renderer does not make a particular codec/storage policy authoritative.

### Editing

`UiDocInput.cpp` contains substantial semantic editing code that is already expressed as `UiDocCoreTransaction` operations, including:

- selection deletion;
- text insertion;
- paragraph break behavior;
- backspace/delete;
- table-cell edits;
- inline-image removal.

This logic is reusable. The genuinely control-specific layer is focus/key dispatch, clipboard, mouse capture, scrolling caret into view and repaint/event delivery.

`UiDocInteraction.cpp` should therefore remain primarily a host/input adapter and delegate semantic editing to a non-Ctrl edit session.

## 4. Target architecture

### 4.1 `UiDocCore`

Existing non-visual authoritative document model. Preserve its role.

### 4.2 `UiDocView`

New non-Ctrl retained layout/geometry object.

Responsibilities:

- bind/read a `UiDocCore`;
- own paragraph/glyph/table/embed retained geometry;
- own glyph/measurement caches;
- prepare only the required visible/overscan document region;
- expose document height and prepared-range evidence;
- expose position/point/caret/table/embed geometry queries;
- invalidate incrementally from `UiDocCore` changes;
- accept explicit content width, viewport/page geometry and style/measurement inputs.

It must not own focus, clipboard, mouse capture, scrollbars or semantic document state.

Suggested public direction:

```cpp
UiDocView view;
view.SetDocument(core);
view.SetStyle(style);
view.SetContentWidth(width);
view.SetViewport(viewport);
view.Prepare();

int GetDocumentHeight() const;
Point PositionToPoint(int position) const;
int PointToPosition(Point point) const;
Rect GetCaretRect(int position) const;
```

Exact APIs should be derived from the existing implementation rather than invented independently.

### 4.3 `UiDocRenderer`

New non-Ctrl paint adapter over `UiDocView`.

Responsibilities:

```cpp
Paint(Draw&, const UiDocView&, const UiDocPaintState&);
```

`UiDocPaintState` can carry transient view/editor presentation such as:

- selection range;
- caret visibility if requested;
- search matches;
- active annotation/embed/table cell;
- enabled/disabled or read-only presentation flags.

Resource rendering should use a provider seam, for example:

```cpp
Function<Image(const UiDocResource&)> image_provider;
```

or an equivalent resource resolver. Do not make file paths or a specific raster codec part of document semantics.

### 4.4 `UiDocEditSession`

New non-Ctrl editing-session state and semantic commands.

Responsibilities:

- caret and selection anchor;
- typing style;
- active table/embed state where required;
- semantic insert/delete/move/format commands;
- build/apply `UiDocCoreTransaction` operations;
- reconcile session positions after authoritative document changes.

It must not own clipboard, focus, mouse capture or scrollbars. A host can translate platform/UI gestures into edit-session commands.

### 4.5 `UiDoc : Ctrl`

Retain the mature public control as the normal editor host.

Refactor it incrementally so it composes:

```text
UiDocCore authority
+ UiDocView
+ UiDocRenderer
+ UiDocEditSession
+ Ctrl-specific focus/input/clipboard/scrolling
```

The purpose is to preserve current behavior while moving reusable logic underneath the control, not to create a reduced replacement editor.

## 5. Why this matters outside UiDoc

A reusable document renderer/view solves several existing architecture pressures.

### UiTimeline

Timeline must remain one owning `UiTimeline : Ctrl`; normal timeline items must not become child controls.

`UiTimelineItem::document_key` can resolve a `UiDocCore` or application-owned document reference. Visible Timeline cards can then use `UiDocView` + `UiDocRenderer` directly inside the Timeline paint pass.

This enables formatted document-rich Blocks/Sketch/cards while preserving:

- one Timeline control;
- bounded visible preparation;
- stable Timeline item IDs/logical coordinates;
- no per-item child Ctrl explosion.

For live editing, Timeline can activate one transient/shared editor host or one `UiDocEditSession` for the currently edited document item rather than allocating a `UiDoc` Ctrl for every card.

### Other Ui controls

The same primitives can support read-only rich previews, compact document fields, inspectors, cards, search results and agent/headless document inspection without duplicating document layout/rendering logic.

### Human and agent parity

Agents should operate on `UiDocCore` semantic transactions and stable document positions/anchors. They must not automate pixel coordinates or depend on a live `UiDoc` Ctrl.

The extracted view/renderer is presentation only; the extracted edit session must build the same `UiDocCoreTransaction` operations as human commands.

## 6. Required invariants

- `UiDocCore` remains the single logical document authority.
- No second RichText-like parallel model.
- No control pointer stored in document semantics.
- Layout caches are derived and disposable.
- Pixel geometry never becomes durable document identity.
- Existing UiDoc serialization/history semantics remain compatible unless a separately reviewed migration explicitly changes them.
- `UiDoc : Ctrl` keeps its current feature set while extraction proceeds.
- Visible/overscan preparation remains bounded; do not regress to whole-document layout on ordinary scrolling/paint.
- Resource decoding/resolution stays host/provider policy.
- Semantic editing remains transaction-based and suitable for both human and agent callers.

## 7. Safe extraction order

Do not rewrite UiDoc in one pass. Use recoverable stages:

### Stage A — shared view records

Move/rename the retained visual structs currently private inside `UiDoc` into an internal/publicly reusable view header with no behavioral change:

```text
VisualGlyph
VisualLine
TableUnitVisual
TableCellVisual
TableVisual
EmbedVisual
ParagraphCache
```

Keep `UiDoc` using the exact same records immediately.

### Stage B — layout engine

Extract paragraph index, measurement cache, paragraph preparation and visible-layout code into `UiDocView`. Add deterministic layout/geometry tests before changing the Ctrl behavior.

### Stage C — geometry API

Move point/position, caret, table and embed geometry queries onto `UiDocView`. Keep UiDoc forwarding existing behavior.

### Stage D — renderer

Move retained painting to `UiDocRenderer`; `UiDoc::Paint()` becomes a thin host call. Add image/resource provider seam before any external control begins using it.

### Stage E — edit session

Move caret/selection state and semantic editing commands into `UiDocEditSession`, preserving the same `UiDocCoreTransaction` behavior. `UiDocInput.cpp` and `UiDocInteraction.cpp` become gesture/platform adapters.

### Stage F — UiDoc acceptance

Run existing UiDoc core, layout, table, image/embed, annotation, undo/redo, search, mouse/keyboard and visual demos with no feature regression before declaring the extraction accepted.

### Stage G — external reuse

Only after UiDoc itself is accepted should Timeline or another control consume `UiDocView`/`UiDocRenderer` directly.

## 8. Rejected approaches

Do not:

- introduce U++ RichText/RichEdit as a new authority;
- replace `UiDocCore` with a new compact document model;
- fork a separate Timeline-specific rich-text implementation;
- make `UiDocRenderer` own mutation/history;
- make every rich Timeline item a child `UiDoc` Ctrl;
- move clipboard/focus/capture into the reusable document model/view;
- combine the extraction with unrelated UiDoc feature redesign.

## 9. Recommended first implementation task

A bounded first task should be **view-record + layout extraction only**:

```text
UIDOC-ENGINE-R1

Extract the retained UiDoc visual/layout records and paragraph layout/index/cache
logic into a non-Ctrl UiDocView while keeping UiDoc behavior and public APIs
unchanged. Add deterministic equivalence tests proving the existing Ctrl and the
new view engine produce the same paragraph positions, document height, caret
geometry, point/position mapping, table/embed geometry and visible preparation
for representative fixtures. Do not move input/clipboard/edit-session behavior
in this task.
```

This is the safest proof that the current mature UiDoc implementation can become reusable without discarding or duplicating it.
