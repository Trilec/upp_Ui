# 03 — Ui Model Guide

`upp_Ui` uses models where they remove duplicated state or allow data to be
shared, switched, virtualized, or presented by more than one view. A programmer
should not need to manage a separate model object for the simple case.

For PropertyEditor schema/factory/transaction details, continue with
`05_UI_PROPERTY_EDITOR_GUIDE.md`.

## The normal model-backed control contract

The genuine model views share one programmer-facing rule:

> `Model()` always returns the model currently driving the control.

The control always owns an internal model. If the application never supplies a
model, that internal model is active automatically.

```cpp
UiList list;
list.Model().Add("Apple");
list.Model().Add("Banana");
list.Model().Add("Orange");
```

There is no separate non-model `UiList` variant and no model refresh call. Model
mutation emits the change notification that keeps the view synchronized.

When application code needs to own or share the data, bind an external model:

```cpp
UiListModel fruit;
UiList list;

list.SetModel(fruit);
list.Model().Add("Apple", 100);
list.Model().Add("Banana", 200);
```

After `SetModel(fruit)`, `list.Model()` and `fruit` are the same object:

```cpp
ASSERT(&list.Model() == &fruit);
```

The same ownership vocabulary is used by:

- `UiList` -> `UiListModel`
- `UiGallery` -> `UiListModel`
- `UiTree` -> `UiTreeModel`
- `UiTable` -> `UiTableModel`
- `UiDropdown` -> `UiListModel`
- `UiMenu` -> `UiMenuModel`
- `UiNodeGraph` -> `UiGraphModel`
- `UiDoc` -> `UiDocCore`

Each exposes:

```cpp
ModelType& Model();
const ModelType& Model() const;
Control& SetModel(ModelType& model);
Control& UseInternalModel();
bool IsUsingInternalModel() const;
Control& ClearModel();
```

`UiDocCore` deliberately remains a document-specific model rather than inheriting
`UiDataModelBase` or flattening document state into `UiModelItem`. Its
`UiDocCoreTransaction`, `UiDocApplyResult` and position-map notifications carry
richer positional-edit semantics needed by document views. The ownership
vocabulary is shared; the domain model and change payload remain appropriate to
the domain. See `10_UIDOC_MODEL_BINDING.md`.

## Switching models

Switching changes which model drives the view. It never copies, merges, or
implicitly clears data.

```cpp
UiList list;
list.Model().Add("Internal A");
list.Model().Add("Internal B");

UiListModel database_a;
database_a.Add("Database A / 1");
list.SetModel(database_a);      // List now presents database_a

UiListModel database_b;
database_b.Add("Database B / 1");
list.SetModel(database_b);      // O(1) ownership switch; no item copy

list.UseInternalModel();        // Internal A/B are still there
```

This makes switching between datasets/databases natural while preserving a
simple local-data mode.

`IsUsingInternalModel()` reports which ownership mode is active:

```cpp
if(list.IsUsingInternalModel())
    Cout() << "local data";
```

`ClearModel()` means exactly one thing:

> clear the currently active model, without changing which model is active.

```cpp
list.SetModel(database_a);
list.ClearModel();              // clears database_a
ASSERT(&list.Model() == &database_a);
```

To clear the retained internal model after using an external model, switch to it
explicitly and clear it:

```cpp
list.UseInternalModel().ClearModel();
```

There is deliberately no implicit transfer from one model to another.

## Why there is no separate widget-only family

The internal model already gives the simple control experience:

```cpp
UiList list;
list.Model().Add("One");
```

An external model is an ownership/sharing option, not a different widget type.
Maintaining a second set of non-model widgets would create parallel state,
duplicate APIs, and different code paths for the same interaction.

The model also enables high-scale views to keep logical record count independent
of live `Ctrl`/renderer count. See `06_UI_MODEL_VIEW_SCALE_GUIDE.md`.

## Controls that deliberately do not need a model

Not every control becomes better by adding a model object.

- `UiAccordion` is a composite container whose sections own real child controls.
- `UiMatrixSelector` is a small bounded value/preset selector.
- `UiColorMatrix` is one compact multi-colour value/editor.

These are not "widget versions" of hidden model controls. Their state is already
the direct value/composition the application is editing. Creating separate model
classes for them would add indirection without sharing or scale benefit.

## One authoritative state

For model-backed views:

- the active model owns semantic record state;
- the control owns interaction state: viewport, hover, pressed state, local
  selection visuals, drag threshold, insertion/drop chrome and transient editor
  lifetime;
- renderer instances own prepared presentation geometry only;
- no control maintains a parallel item mirror;
- switching models changes the active pointer and resets view state as needed,
  not record state;
- callbacks from previously bound inactive models are ignored by the view.

For `UiDoc`, caret, selection, scroll position, active object and paragraph/layout
caches are view state. Text, style runs, blocks, annotations, resources, embeds,
anchors, revisions and Undo/Redo history are `UiDocCore` state. Multiple UiDoc
views can therefore share one document model while retaining independent view
state.

This is why retired synchronization APIs such as `RefreshFromModel()` must not
return. If model data changes through its public mutation API, its change event is
the synchronization path.

## Request-first mutation contract

For user operations that may change application-owned data, the control computes
and reports intent before mutation:

1. The user performs an operation.
2. The control computes the proposed target.
3. The control emits a request event, such as `WhenReorderRequest`.
4. Rejected -> nothing changes.
5. Handled -> the owner performed or scheduled the change.
6. Unhandled + internal mutation enabled -> the control may mutate the active
   model directly.
7. The model notification updates every bound view.

Shared request structs include:

- `UiReorderRequest` — List/Dropdown reorder.
- `UiTreeMoveRequest` — Tree reparent/move.
- `UiMenuActionRequest` — Menu semantic action.
- `UiTableEditRequest` — Table edit.
- Graph request structures for node moves, connections and deletion.

Simple/local use can allow internal mutation:

```cpp
control.EnableInternalMutation(true);
```

Command-driven applications can disable it and handle the request:

```cpp
control.EnableInternalMutation(false);
control.WhenReorderRequest = [=](UiReorderRequest& r) {
    r.handled = true;
    Dispatch(MoveItemCommand(r.from, r.before));
};
```

"Mutate silently, notify afterward" is not the target architecture.

`UiDoc` now shares the model-ownership contract, but its richer request-first
user-edit interception is deliberately a separate future policy layer. Any such
layer must carry or produce `UiDocCoreTransaction`; it must not introduce a
second document store or weaken the existing revision/position-map contract.

## Stable identity and values

Use model IDs/data payloads for program logic rather than display labels.
Display text can change, localize, or be rendered differently without changing
identity.

For shared `UiModelItem` records, `data` is the normal application payload while
`text`, `description`, `right_text`, image/icon, check state and columns are
presentation-oriented record fields.

Graph, Menu, Tree, Table and UiDoc keep their domain-specific model structures
where those structures carry real semantics. Sharing the ownership vocabulary
does not force every domain into `UiListModel` or `UiModelItem`.

## Model lifetime

An external model passed to `SetModel(model)` is non-owning from the control's
point of view. It must outlive the period during which the control uses it.

The internal model is owned by the control and always exists. `UseInternalModel()`
is therefore lifetime-safe and requires no allocation.

Mutate bound models on the GUI thread unless the application provides its own
synchronization.

## PropertyEditor is a specialized model consumer

`PropertyEditorModel` is a headless property schema/value model with additional
normalization, validation, preview/commit/reset and impact semantics. It is not a
reason to duplicate the normal List/Tree/Table control family.

`PropertyEditorItem` has a stable `id`, human `label`, optional grouping/help/unit
metadata, current/default value, validation hooks and refresh-impact information.

`PropertyEditorModel` emits specialized events including:

- `WhenStructureChanged`
- `WhenValueChanged(id)`
- `WhenPreview(id, value)`
- `WhenCommit(id, value)`
- `WhenReset(id)`
- `WhenGroupMetadataChanged`

`PropertyEditor` forwards interaction through its preview/commit/reset/override
and selection events. See `05_UI_PROPERTY_EDITOR_GUIDE.md` for the full contract.

## Avoiding feedback loops

- Do not mutate the same model property recursively from its own change handler
  without a guard.
- Model notifications are observations; request events are authorization.
- Do not keep a second array/view-model mirror merely to feed a control.
- Do not call refresh/sync helpers after normal model mutation; the model event is
  authoritative.

## Scale rules

Model ownership simplicity must not weaken virtualization:

- ordinary List/Gallery/Table/Tree viewport work remains proportional to visible
  or overscan content;
- model switching never copies N records merely to display them;
- renderer pools remain bounded independently of logical record count;
- explicit full-model operations such as Select All may be O(N), but scrolling,
  painting, hover and hit testing may not become O(N).

UiDoc uses a different scale shape: document state remains sparse and paragraph
layout stays viewport-driven/cached rather than allocating a child control or
persistent geometry object per character. Binding an external UiDocCore does not
copy document records or create another layout model.

The deterministic scale tests remain the authority for these invariants.

## Common mistakes

- Exposing internal-model implementation details in ordinary usage instead of
  using `Model()`.
- Clearing or copying the old model when `SetModel()` is called.
- Assuming `ClearModel()` switches ownership.
- Keeping a parallel item container in the control.
- Restoring retired `RefreshFromModel()`-style synchronization.
- Matching application records by display label instead of stable data/ID.
- Using a view as the source of application-owned semantic truth.
- Flattening a domain model such as UiDocCore or UiGraphModel into UiModelItem
  merely for type uniformity.
- Freezing a theme-derived style accidentally while configuring a model view;
  use semantic theme defaults unless a local custom style is intentional.
