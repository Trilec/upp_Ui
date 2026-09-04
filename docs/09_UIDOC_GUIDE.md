# 09 — UiDoc Guide

This is the canonical architecture guide for `UiDocCore` and `UiDoc`.

It defines model binding, document/view ownership and the intended reusable
non-Ctrl document-engine boundary.

## 1. Authority

`UiDocCore` is the single semantic document authority.

It owns:

- positional text;
- sparse style runs;
- semantic blocks;
- annotations/comments;
- resources and embeds;
- inline runs/images;
- tables;
- anchors and metadata;
- revisioned transactions;
- position mapping;
- Undo/Redo history;
- serialization/import/export semantics.

Do not mirror a document into `UiModelItem`, List or Tree merely to obtain a
common model type.

## 2. Normal model binding

`UiDoc` follows the same ownership vocabulary as other genuine model-backed
controls:

```cpp
UiDocCore& Model();
UiDoc& SetModel(UiDocCore& model);
UiDoc& UseInternalModel();
bool IsUsingInternalModel() const;
UiDoc& ClearModel();
```

A new UiDoc owns an internal UiDocCore.

`SetModel()` switches authority without copying, merging or clearing either
model. External models are non-owning and must outlive the period in which they
are active.

Previously bound inactive models are tracked with weak lifetime identity so a new
object reusing an old address still receives a fresh observer binding.

## 3. One model, many views

Several UiDoc controls may bind the same UiDocCore.

The model remains semantic authority; each view owns independent:

- caret;
- selection;
- scroll/viewport;
- active annotation/embed/table state;
- layout caches;
- hit-test and paint state.

A transaction produces the authoritative change/position mapping. Each bound
active view remaps its own transient state and drops stale active objects that no
longer exist.

## 4. Change contract

UiDocCore deliberately keeps its richer document-specific contract rather than
inheriting `UiDataModelBase` for spelling symmetry.

Core synchronization types include:

- `UiDocCoreTransaction`;
- `UiDocApplyResult`;
- `UiDocPositionMap`.

These carry the range/revision/position information needed for real document
editing.

## 5. History

History depth is UiDocCore policy, not UiDoc visual style.

An external application-owned document therefore chooses its own history limit;
theme/style changes must never overwrite shared semantic history policy.

## 6. Model state versus view state

UiDocCore:

- document content;
- structure;
- resources;
- annotations;
- transaction/history semantics.

UiDoc:

- focus/input;
- caret/selection;
- viewport;
- active object;
- transient drag/resize;
- derived layout;
- drawing;
- clipboard/GUI integration.

Pixel geometry is derived view state and never durable document identity.

## 7. Agent editing

Agents should operate on semantic transactions, not GUI pixels:

1. read model/revision;
2. construct a bounded UiDocCoreTransaction;
3. apply against the expected revision;
4. inspect UiDocApplyResult;
5. let bound views consume the committed change.

Do not create a second agent-only document mirror.

## 8. Current reusable-engine direction

UiDoc already has a custom document engine; it is not built on U++ RichText.

The useful future extraction is therefore to separate the mature non-semantic
view/editor mechanics from the Ctrl host rather than replace the model.

Desired layers:

```text
UiDocCore
    semantic document authority
        |
UiDocView
    retained layout + geometry + position mapping
        |
UiDocRenderer
    non-Ctrl document paint adapter
        |
UiDocEditSession
    caret/selection + semantic edit commands
        |
UiDoc : Ctrl
    focus/input/clipboard/scroll/capture host
```

Exact public APIs should be extracted from proven current implementation rather
than invented in parallel.

## 9. UiDocView responsibility

A reusable non-Ctrl view should own derived/disposable presentation such as:

- glyph measurement cache;
- paragraph height/index cache;
- retained line/glyph/table/embed geometry;
- visible/overscan layout preparation;
- document-position <-> screen-position mapping;
- caret geometry;
- table/embed geometry queries.

It does not own mutation/history.

## 10. UiDocRenderer responsibility

A renderer consumes UiDocView + transient paint state.

Paint state may include:

- selection;
- optional caret;
- search matches;
- active annotation/embed/table cell;
- enabled/read-only presentation.

Resource/image resolution should remain a provider seam rather than file-path
semantics in the document model.

## 11. UiDocEditSession responsibility

A reusable edit session may own:

- caret/selection anchor;
- typing style;
- active table/embed edit state;
- semantic insert/delete/move/format commands;
- creation/application of UiDocCore transactions;
- position reconciliation after model changes.

It must not own platform clipboard, focus, mouse capture or scrollbars.

## 12. Why reuse matters

A reusable document view/renderer can support:

- rich Timeline cards/blocks without one child UiDoc Ctrl per item;
- read-only previews;
- inspectors/search results;
- compact document fields;
- headless/agent document inspection.

For a dense Timeline, only visible documents would have prepared view state.
Live editing can activate one transient/shared editor/session for the current
item.

## 13. Extraction order

If/when this work resumes:

1. extract shared retained view records with no behavior change;
2. extract layout/index/cache into UiDocView;
3. move geometry queries onto UiDocView;
4. move painting to UiDocRenderer;
5. move semantic editing session state/commands;
6. run complete UiDoc regression/visual acceptance;
7. only then consume the engine from Timeline or another control.

Do not combine the extraction with unrelated document feature redesign.

## 14. Non-goals

Do not:

- introduce U++ RichText as a new authority;
- create a second compact document model;
- fork a Timeline-specific rich-text engine;
- make UiDocRenderer own history/mutation;
- make every rich Timeline item a child UiDoc Ctrl;
- move focus/clipboard/capture into UiDocCore;
- make pixel geometry durable document semantics.
