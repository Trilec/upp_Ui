# 09 — Ui Model API + Theme Audit

This audit records the model-ownership simplification performed after R2 model
rendering acceptance, the subsequent UiDoc convergence onto the same public
ownership vocabulary, and the final lifetime hardening of ordinary shared models.

## Decision

`upp_Ui` does not need separate "widget" and "model/view" versions of ordinary
data controls.

A genuine model-backed control owns an internal model from construction and
exposes the same model API whether the data is locally or externally owned:

```cpp
control.Model();
control.SetModel(external);
control.UseInternalModel();
control.IsUsingInternalModel();
control.ClearModel();
```

`Model()` always means the model currently driving the control.

This lets the simple case stay simple:

```cpp
UiList list;
list.Model().Add("One");
list.Model().Add("Two");
```

and grows naturally into shared/external data:

```cpp
UiListModel database;
UiList list;
list.SetModel(database);
list.Model().Add("One");
```

There is no data copy or second semantic store in either case.

The contract is about **model authority and view ownership**, not about forcing
every domain into `UiModelItem` or one common model base class.

## Model-backed control audit

### UiList

Model: `UiListModel`.

- owns `internal_model_`;
- `model_` selects the active model;
- renderer pool remains viewport-bounded;
- selection/rename/reorder remain view interaction state;
- external/internal switching copies no rows.

Theme finding: Minimal List rows may intentionally be transparent. A standalone
List nevertheless owns a viewport. Theme-driven List paint fills missing
viewport faces from semantic `UiPanelRole::Surface` while leaving row renderers
transparent/lightweight. Explicit custom List transparency remains caller-owned.

### UiGallery

Model: `UiListModel`.

- same ownership contract as List;
- uniform grid arithmetic and bounded renderer pool remain unchanged;
- zoom/marquee/selection remain view state.

The Gallery corrective established the semantic Surface fallback for a
transparent Minimal face and removed the stale List row skin from the Gallery
viewport.

### UiTree

Model: `UiTreeModel`.

- same ownership vocabulary;
- the visible-row projection is derived view state, not another semantic model;
- structural changes may rebuild the projection; ordinary scrolling remains
  viewport-bounded;
- rebinding the already-active model is idempotent.

No duplicate Tree widget/model family is required.

### UiTable

Model: `UiTableModel`.

The audit removed one historical exception: a default `UiTable` used to seed its
internal model with a hidden 12 x 6 sample dataset. A model view now starts with
an empty internal model; demos own demo data.

Table retained geometry and bounded renderer pools are unchanged.

Theme finding: Table has explicit domain chrome in addition to a palette.
`UiTable::SyncThemeStyle()` completes those fields from semantic
Surface/Subtle/List theme roles and dark-transforms warning/error semantic fills.
Custom Table styles are not overridden.

### UiDropdown

Model: `UiListModel`.

R2D removed the former parallel Dropdown item mirror. Convenience
`Add/Remove/SetItem...` methods mutate the active model directly and therefore do
not constitute another data authority.

Changing external/internal ownership does not alter popup/rendering semantics.

### UiMenu

Model: `UiMenuModel`.

Menu remains domain-specific because command/check/radio/submenu topology is real
model semantics, not generic List data. Sharing the ownership vocabulary does not
force Menu into `UiListModel`.

Popup content renderers remain bounded to visible rows; Menu retains command and
popup/session chrome.

### UiNodeGraph

Model: `UiGraphModel`.

Graph topology, ports and edges remain domain model state. Geometry caches,
selection, pan/zoom and transient drag/connect/marquee state remain view state.
The canvas is resolved through an explicit semantic dark/light theme surface.

The final Graph hardening keeps one retained world-space spatial hash as the
broad-phase authority. Public `HitTestNode`, `HitTestPort` and `HitTestEdge` now
use the same small spatial-neighborhood queries as live pointer interaction rather
than retaining a second prepared-viewport scan path. Node-style-class preview
rebuilds only currently prepared users of that class plus their prepared incident
edges; it does not rebuild the world index or full prepared viewport. Empty hash
cells are removed when their final occupant leaves.

### UiDoc

Model: `UiDocCore`.

UiDoc now follows the same first-class ownership vocabulary without flattening a
document into generic items:

- an owned internal `UiDocCore` exists from construction;
- `Model()` is the only public document-model accessor;
- `SetModel(external)` selects an externally owned `UiDocCore` without copying,
  merging or clearing either model;
- `UseInternalModel()` restores retained internal document data;
- multiple UiDoc views may share one `UiDocCore` while retaining independent
  caret, selection, viewport, active-object and layout state;
- model transactions and Undo/Redo notify every actively bound view;
- callbacks from previously bound inactive models are ignored;
- weak `Pte`/`Ptr` lifetime bookkeeping prevents a destroyed inactive model from
  blocking a fresh observer when a later model reuses the same address;
- active annotation/embed/table state is reconciled against the model after
  model notifications, so externally removed objects cannot remain as stale view
  selections;
- history depth is `UiDocCore` policy, not visual `UiDoc::Style` state.

`UiDocCore` deliberately keeps its richer document change contract:
`UiDocCoreTransaction`, `UiDocApplyResult`, revision evidence and
`UiDocPositionMap`. It does not inherit `UiDataModelBase` merely for naming
symmetry and is not mirrored through `UiModelItem`.

See `docs/10_UIDOC_MODEL_BINDING.md` for the programmer-facing contract.

## Shared observer lifetime contract

`UiListModel`, `UiTreeModel`, `UiTableModel`, `UiMenuModel` and `UiGraphModel`
inherit the lightweight `UiDataModelBase` notification contract. Their views may
leave callbacks attached to previously used external models and ignore those
callbacks while the model is inactive.

Raw address identity is insufficient for that pattern: external model A can be
destroyed and a fresh model B can later be allocated at the same address. The
shared model layer therefore carries weak `Pte` identity and views remember
previous bindings through `UiModelObserverSet`. Expired identities are pruned
before deduplication, so B always receives a fresh observer even when its address
matches destroyed A.

Copy/value semantics remain explicit:

- copying a `UiDataModelBase` creates a fresh weak identity and does not copy
  installed callbacks;
- assignment preserves the destination object's identity and callbacks while
  copying revision semantics;
- active external ownership remains non-owning; weak observer bookkeeping does
  not extend a model's lifetime.

This lifetime rule is shared by List, Gallery, Tree, Table, Dropdown, Menu and
NodeGraph and matches the same-address hardening already established for UiDoc.

## Controls deliberately not given models

### UiAccordion

Accordion is a composite container. Its sections contain real child controls and
its open/closed/drag state is the structure of that container. A separate model
would either duplicate child ownership or require an abstraction more complex
than the current direct API.

Result: keep it model-free.

### UiMatrixSelector

MatrixSelector is a small bounded value/preset control. Its cells are the direct
configuration/value of one widget, not a scalable or independently shared data
collection.

Result: keep it model-free.

### UiColorMatrix

ColorMatrix represents one compact 1-8-colour value edited as a unit. A separate
model would add an object/lifetime layer without a sharing or virtualization
benefit.

Result: keep it model-free.

## PropertyEditor

`PropertyEditorModel` remains a specialized schema/transaction model with
validation, normalization, preview/commit/reset and impact semantics. This is not
a competing ordinary control family and its specialized API should not be
mechanically replaced with the collection-control contract.

## Switching contract

Switching models is selection of authority, not data movement.

```text
internal model A ── retained while inactive
        ^
        | UseInternalModel()
        |
control.model_ ───────────────> external model B
                SetModel(B)
```

Rules:

1. Every genuine model-backed view constructs its internal model eagerly.
2. Default `model_` points to that internal model.
3. `SetModel(external)` changes authority and resets only derived view state that
   cannot safely carry across unrelated datasets/documents.
4. Neither model is copied, merged or cleared by a switch.
5. `UseInternalModel()` selects the retained internal model.
6. `ClearModel()` clears the currently active model and does not switch models.
7. Previously installed model callbacks may remain connected, but a view ignores
   notifications whose observed model is not the current model.
8. Binding deduplication is lifetime-aware rather than raw-address-only; a fresh
   object that reuses a destroyed model's address must receive a fresh callback.
9. External models are non-owning and must outlive the period in which they are
   actively used by a view.
10. Derived interaction state must remain valid after external mutation; when a
    selected model object disappears, the view clears or clamps that transient
    state rather than retaining a stale semantic reference.

## Performance result

The API convergence adds no new semantic data layer.

- switching is O(1) model authority selection plus legitimate derived-view reset;
- no record/document copy is introduced;
- no child-Ctrl-per-item allocation is introduced;
- List/Gallery/Table/Tree renderer pools remain bounded by useful viewport
  content;
- Tree projection and Graph geometry retain their domain-specific cache contracts;
- Graph point hit tests query retained spatial cells before exact geometry tests;
- Graph local style-class updates do not rebuild the full spatial/prepared scene;
- UiDoc paragraph/layout caches remain derived view state and are invalidated from
  `UiDocApplyResult`, not mirrored document storage.

Existing scale tests remain required after the API migration.

## Theme audit result

### Corrected

- standalone theme-driven `UiList` viewport receives a semantic Surface face
  even when Minimal row presentation is intentionally transparent;
- `UiTable` resolves ordinary Table domain chrome coherently in Dark mode rather
  than retaining selected Light-only defaults;
- NodeGraph committed selection uses the semantic Accent path rather than the
  darker pressed Accent, with a consistent approximately 2px shape-following
  overlay independent of node-shape frame styling.

### Already coherent

- Gallery: semantic viewport Surface corrective previously Windows accepted;
- Tree: explicit theme-derived surface/selection/glyph colours;
- Dropdown: collapsed/popup theme path previously Windows accepted;
- Menu: bar/popup theme path previously Windows accepted;
- NodeGraph: explicit semantic canvas/node/edge dark/light resolver;
- Accordion: theme-live Surface + Accent title-card composition;
- MatrixSelector: semantic Panel/Button roles with revision tracking;
- ColorMatrix: semantic Panel/Button roles with revision tracking.

### Styling rule

Do not fix Dark mode by sprinkling independent hard-coded dark RGB values through
controls. Resolve a semantic theme role first. Domain-specific semantic colours
may be transformed from established defaults when no general theme role
represents the state.

Avoid configuration setters that enter `StyleEdit()` merely to repeat an
existing default: that turns a theme-derived style into an intentional custom
style and freezes the current theme snapshot.

## Deterministic validation

`Utilities/UiModelBindingContractTest` now provides **55 checks**: the existing
seven ownership/switching checks for each of List, Gallery, Tree, Table, Dropdown,
Menu and NodeGraph, plus six weak-identity tests covering independent copy
identity, assignment identity preservation, expiration, same-address helper reuse,
List rebinding and NodeGraph rebinding.

`Utilities/UiDocModelBindingTest` provides **22 checks** covering:

- default internal ownership and retained internal data;
- exact external identity and no-copy switching;
- direct external mutation notifications;
- idempotent same-model rebinding;
- two views sharing one document with independent positional remapping;
- inactive-model callback suppression;
- destroyed-model / same-address lifetime reuse;
- active-only `ClearModel()`;
- model-owned history policy;
- shared model editing and Undo authority;
- editing through the currently active external model;
- reconciliation of active image state after direct external model removal.

`Utilities/UiThemeSurfaceRegressionTest` provides **13 checks** covering focused
Light/Dark surface corrections.

Existing scale/regression gates remain authoritative:

- `UiModelViewPerformanceTest` — 52 checks;
- `UiTreeScaleTest` — 11 checks;
- `UiGalleryRegressionTest` — 11 checks;
- `UiDropdownMenuRenderTest` — 11 checks;
- `UiNodeGraphScaleTest` — **51 checks** after final spatial/local-update hardening;
- the existing UiDoc model/interaction/geometry/image/metadata suites.

Windows validation must run the focused binding suites in Debug and Release and
retain the relevant existing regression gates.

## Legacy spelling cleanup

`Model()` is now the canonical and only ordinary model-backed accessor spelling
in the converged controls. Repository callers were mechanically migrated and the
transitional List/Gallery/Tree/Table/Dropdown/Menu/NodeGraph
`GetInternalModel()` / `GetModel()` aliases and UiDoc `Core()` alias are removed.

Do not restore those aliases or introduce a second model store for compatibility.
Specialized controls such as PropertyEditor may retain their own domain API where
that API represents genuinely different semantics.
