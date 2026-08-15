# 10 — UiDoc Model Binding

`UiDoc` follows the same model-ownership contract as the other genuine model-backed Ui controls while retaining a document-specific model.

## Canonical API

```cpp
UiDocCore& Model();
const UiDocCore& Model() const;
UiDoc& SetModel(UiDocCore& model);
UiDoc& UseInternalModel();
bool IsUsingInternalModel() const;
UiDoc& ClearModel();
```

A newly constructed `UiDoc` owns an internal `UiDocCore` and uses it immediately. Applications that do not need shared or externally owned document state therefore need no separate model object.

```cpp
UiDoc doc;
doc.Model().Replace(UiDocRange(0, 0), WString("Hello"));
```

An application, tool or agent that owns document state can bind it directly:

```cpp
UiDocCore document;
UiDoc editor;
editor.SetModel(document);
```

`SetModel()` switches authority. It does not copy, merge or clear either the previous or the newly supplied model. `UseInternalModel()` restores the retained internal document. `ClearModel()` clears only whichever model is currently active and does not switch ownership.

External models are non-owning and must outlive the period in which a `UiDoc` uses them. UiDoc keeps only weak lifetime bookkeeping for models that were bound previously, so an inactive external model may be destroyed normally; a later `UiDocCore` created at the same address is treated as a new model and receives a fresh binding.

## Why UiDocCore remains domain-specific

The shared model contract is an ownership/view contract, not a requirement that every control use `UiModelItem`.

`UiDocCore` owns document semantics that do not map naturally to a homogeneous item list:

- positional text;
- sparse style runs;
- semantic blocks;
- annotations;
- resources and embeds;
- tables containing rich inline runs;
- document metadata and anchors;
- revisioned transactions;
- position mapping;
- Undo/Redo history.

For the same reason that `UiTable` uses `UiTableModel` and `UiNodeGraph` uses `UiGraphModel`, `UiDoc` uses `UiDocCore`.

Do not mirror a document into `UiListModel`, `UiTreeModel`, `UiTableModel` or `UiModelItem` merely to satisfy a superficial common type.

## One authority, many views

Multiple `UiDoc` controls may bind the same `UiDocCore`:

```cpp
UiDocCore document;
UiDoc editor;
UiDoc preview;

editor.SetModel(document);
preview.SetModel(document);
```

The model is the single semantic authority. Each view retains its own caret, selection, scroll position, active object and layout caches. A model transaction emits `UiDocApplyResult`; each active bound view remaps its own positional state from the same position map and refreshes its derived layout.

Derived view state is also reconciled against the model after notifications. If an external mutation removes the annotation, embed or table that a view currently has active, that stale transient selection is cleared rather than being allowed to intercept later keyboard or command handling. Valid active objects remain active across ordinary edits.

This is particularly useful for agent-driven workflows: an agent can apply a deterministic `UiDocCoreTransaction` directly to the shared model and every bound view observes the same committed result without a second synchronization API.

## Change contract

`UiDocCore` deliberately does **not** inherit `UiDataModelBase` merely for spelling consistency.

Its existing change contract is richer than `UiModelChange`:

```cpp
UiDocCoreTransaction
UiDocApplyResult
UiDocPositionMap
```

It carries document revision, changed range and exact position mapping needed for text/range edits. That remains the authoritative synchronization path.

Previously bound but inactive live models may still contain callbacks. `UiDoc` ignores those notifications unless the observed model is the model currently selected by `Model()`. Weak lifetime tracking ensures a destroyed inactive model no longer occupies a binding identity merely because another object later reuses its address.

## Model state versus view state

`UiDocCore` owns semantic document state and history.

`UiDoc` owns transient presentation/interaction state:

- caret and selection;
- viewport/scroll;
- active table/embed/annotation;
- search presentation;
- paragraph/layout caches;
- hit testing and paint state;
- transient drag/resize interactions.

Switching models resets view state that cannot safely carry across unrelated documents. It does not alter either model. Direct model mutations may remap positional state and invalidate only transient object selections that are no longer valid in the resulting model.

## History policy

Undo history depth belongs to `UiDocCore`, not visual `UiDoc::Style`.

An externally owned model may therefore choose its own history policy:

```cpp
UiDocCore document;
document.SetHistoryLimit(256);
editor.SetModel(document);
```

Changing editor style must not overwrite shared model history policy.

## Derived model views

A document application may project part of `UiDocCore` into another model-backed control when useful. For example, a Review panel can expose annotations through a `UiListModel` whose item payload stores annotation IDs.

That projection is view/navigation data, not another document authority. Mutations still return to `UiDocCore`.

Likewise, a document table may be adapted to a standalone `UiTable` for specialized editing, but `UiDocTable` remains the serialized document representation unless the application explicitly transfers authority.

## Agent-oriented rule

For automated editing, mutate the model rather than driving the GUI:

1. read the current model/revision;
2. construct a bounded `UiDocCoreTransaction`;
3. apply it against the expected revision;
4. inspect `UiDocApplyResult`;
5. let bound views observe the committed model notification.

Do not maintain a second document mirror solely for an agent.

A future request-first user-edit interception layer may sit above this model contract for command-driven applications, but it must not create another semantic data store or replace `UiDocCoreTransaction`.
