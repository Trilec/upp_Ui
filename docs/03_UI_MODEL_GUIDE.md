# 03 — Models and Data

Models remove duplicated semantic state and support shared views. A programmer
should not need a separate model object for a simple control. PropertyEditor
schema, adapters and editing transactions are in its [guide](05_UI_PROPERTY_EDITOR_GUIDE.md).

## The active-model contract

`Model()` always returns the model currently driving a genuine model-backed
control. Each such control owns an internal model by default:

```cpp
UiList list;
list.Model().Add("Apple");
list.Model().Add("Banana");
```

External binding is a non-owning switch, not a data copy or merge:

```cpp
UiListModel fruit; // outlives the active binding
UiList list;
list.SetModel(fruit);
list.Model().Add("Apple", 100);
ASSERT(&list.Model() == &fruit);
list.UseInternalModel(); // retained internal data is still present
```

The shared vocabulary is Model (const/non-const), SetModel, UseInternalModel,
IsUsingInternalModel and ClearModel. ClearModel clears the **currently active**
model; it does not switch to the internal model. Model changes publish the
notification that updates bound views; no RefreshFromModel call is necessary.

| Control | Model | Identity |
| --- | --- | --- |
| UiList / UiGallery / UiDropdown | UiListModel | sequential index; optional stable application data key |
| UiTree | UiTreeModel | stable node reference |
| UiTable | UiTableModel | row/column coordinate and range |
| UiMenu | UiMenuModel | stable menu node and command semantics |
| UiNodeGraph | UiGraphModel | graph IDs/references |
| UiDoc | UiDocCore | document positions/anchors and transaction mapping |

Sharing ownership vocabulary does not force every domain into UiModelItem.
There is no separate widget-only List/Tree/Table family: the internal model
already supplies that experience. Small bounded value controls such as
UiMatrixSelector, UiColorMatrix and UiRangeSegments need no extra model object.
UiAccordion is a real-child composition, not a hidden list-view alternative.

## Authority and lifetime

Semantic records belong to the active model. Views own viewport, hover, focus,
selection visuals, gestures, transient editors and derived presentation. Shared
renderers own only prepared content inside the rectangle the view gives them.
Do not keep another item collection in a control merely to synchronize it.

External models must outlive active use. Bound mutations run on the GUI thread
unless the host provides synchronization. Rebinding reconciles interaction state
without clearing/copying either dataset. Notifications from inactive models are
ignored. Weak observer identity distinguishes a fresh model from an old object
that occupied the same address. See the coding guide for Ctrl parenting, which
does not by itself transfer C++ ownership.

## Request-first user mutation

A control first computes and emits intent. Rejected means no change; handled
means the host performed/scheduled the edit; unhandled plus enabled internal
mutation permits the control to edit the active model. Model notification then
updates every bound view. Observations are not authorization.

Examples: UiReorderRequest, UiTreeMoveRequest, UiMenuActionRequest,
UiTableEditRequest and Graph move/connect/delete/route requests. Simple local use
may EnableInternalMutation(true). Command-driven hosts disable it and implement
the corresponding When...Request. Do not mutate first and ask permission later.

UiDoc's positional transaction model is already authoritative; a richer generic
request interception layer for all user edits remains a separate future policy,
not an implemented List-style request contract. It must preserve UiDocCoreTransaction.

## Mutable records and notification scope

Publish mutable record edits through the model's mutation/Touch API:

```cpp
// Examples from the relevant model families:
// list.Model().Touch(first, count);
// tree.Model().Touch(node);
// table.Model().TouchCell(row, column);
// table.Model().TouchHeader(axis, index);
// menu.Model().Touch(node);
// graph.Model().TouchNode(node_id);
// graph.Model().TouchEdge(edge_id);
```

Prefer one truthful ranged/bulk event per semantic batch. Presentation-only edits
do not justify rebuilding an entire projection. Guard feedback loops rather than
recursively mutating a property from its own notification.

Do not use display labels as application identity. Sequential views remap indices
on insertion/removal/move; application data keys can restore identity after a full
reset. Tree/Menu/Graph keep their native stable references. Table remains coordinate
based unless its public model is deliberately changed. UiDoc uses position maps.

## Scale boundary

Logical size is independent from live Ctrl/renderer count. List/Gallery/Table use
visible-range arithmetic; Tree may retain a visible hierarchy projection. Normal
scroll/paint/hit work is bounded to useful visible/overscan content. Explicit
Select All/export/filter/structural rebuild can legitimately be O(N). Model switches
must not copy N records just to display them. Images are prepared for visible
items and signalled through bounded model notifications, not eagerly decoded for
all records. See [Drawing and Performance](07_UI_DRAWING_GUIDE.md).

## UiDoc: document-specific contract

UiDocCore owns positional text, sparse style runs, blocks, annotations, resources,
embeds/inline images, tables, anchors/metadata, revisioned transactions, position
maps, undo/redo and import/export. UiDoc is its Ctrl view, with independent caret,
selection, viewport, active object and paragraph/layout caches. Several views can
share one model without copying the document.

UiDocCoreTransaction, UiDocApplyResult and UiDocPositionMap express actual
positional edits. A committed transaction remaps each active view's transient
state and invalidates deleted active objects. History depth is model policy;
changing a theme must never change shared document history.

Agents edit against an expected revision, apply a bounded transaction, inspect
its result and allow bound views to consume it. Pixel positions are disposable
view geometry, not durable document identity. Resources remain model/provider
semantics rather than a second editor-owned store. Preserve the current sparse,
paragraph-cached/viewport-driven implementation; do not allocate a Ctrl per character.

The existing engine is not U++ RichText. A **future**, separately authorized
extraction may separate UiDocView (layout/geometry/mapping), UiDocRenderer (painting)
and UiDocEditSession (caret/semantic commands) from UiDoc's platform input/focus/
clipboard/capture host. Those names describe direction, not shipped replacement
APIs. Extract from proven implementation without inventing another document model,
then validate the full document suite before using it in a Timeline or other dense
view. Do not build a second compact document engine or one live UiDoc per card.

## PropertyEditor: specialized schema, not application authority

PropertyEditorModel stores typed property values, defaults, mixed/inherited state,
help/group/unit metadata, validation and refresh-impact information. It emits
WhenStructureChanged, WhenValueChanged, WhenPreview, WhenCommit, WhenReset and
WhenGroupMetadataChanged. The visual editor owns editing lifetime and delegates
application commands/undo to the host. Core stores custom adapter/provider IDs
without importing concrete GUI or domain implementations.

Use one active semantic collection for a demo Data page. PropertyEditor rows may
project that collection; they must not become a competing editable collection
with a separate synchronization protocol. Clear ownership is more important than
making unlike domain model classes inherit the same base.
