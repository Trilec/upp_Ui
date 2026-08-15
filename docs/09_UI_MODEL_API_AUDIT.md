# 09 — Ui Model API + Theme Audit

This audit records the model-ownership simplification performed after R2 model
rendering acceptance.

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

There is no data copy or second item store in either case.

## Model-backed control audit

### UiList

Model: `UiListModel`.

- owns `internal_model_`;
- `model_` selects the active model;
- renderer pool remains viewport-bounded;
- selection/rename/reorder remain view interaction state;
- external/internal switching copies no rows.

Theme finding: Minimal List rows may intentionally be transparent. A standalone
List nevertheless owns a viewport. Theme-driven List paint now fills missing
viewport faces from semantic `UiPanelRole::Surface` while leaving row renderers
transparent/lightweight. Explicit custom List transparency remains caller-owned.

### UiGallery

Model: `UiListModel`.

- same ownership contract as List;
- uniform grid arithmetic and bounded renderer pool remain unchanged;
- zoom/marquee/selection remain view state.

The earlier Gallery corrective already established the semantic Surface fallback
for a transparent Minimal face and removed the stale List row skin from the
Gallery viewport.

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

Theme finding: Table has explicit domain chrome in addition to a palette. The
central resolver covered the main table/header/selection colours but several
fields still inherited Light defaults in Dark mode. `UiTable::SyncThemeStyle()`
now completes those fields from semantic Surface/Subtle/List theme roles and
dark-transforms warning/error semantic fills. Custom Table styles are not
overridden.

### UiDropdown

Model: `UiListModel`.

R2D already removed the former parallel Dropdown item mirror. Convenience
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
The canvas is already resolved through an explicit semantic dark/light theme
surface.

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
a competing model/view family for List/Tree/Table controls.

Do not generalize PropertyEditor's specialized lifecycle into ordinary data
controls.

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

1. Every genuine model view constructs its internal model eagerly.
2. Default `model_` points to that internal model.
3. `SetModel(external)` changes the active pointer and resets only derived view
   state that cannot safely carry across datasets.
4. Neither model is copied, merged or cleared by a switch.
5. `UseInternalModel()` is exactly `SetModel(internal_model_)`.
6. `ClearModel()` calls `Clear()` on the currently active model and does not
   switch models.
7. Model callbacks installed on previously used models may remain connected, but
   the view ignores notifications whose observed model is not the current
   `model_`.
8. The external model is non-owning and must outlive the period in which it is
   active.

## Performance result

The API convergence does not add a new data layer.

- switching is O(1) model authority selection plus legitimate derived-view reset;
- no record copy is introduced;
- no child-Ctrl-per-item allocation is introduced;
- List/Gallery/Table/Tree renderer pools remain bounded by useful viewport
  content;
- Tree projection and Graph geometry retain their existing domain-specific cache
  contracts.

The existing R2 scale tests remain required after the API migration.

## Theme audit result

### Corrected

- standalone theme-driven `UiList` viewport now receives a semantic Surface face
  even when Minimal row presentation is intentionally transparent;
- `UiTable` now resolves all ordinary Table domain chrome coherently in Dark
  mode rather than retaining selected Light-only defaults.

### Already coherent

- Gallery: semantic viewport Surface corrective already Windows accepted;
- Tree: explicit theme-derived surface/selection/glyph colours;
- Dropdown: collapsed/popup theme path Windows accepted in R2D;
- Menu: bar/popup theme path Windows accepted in R2D;
- NodeGraph: explicit semantic canvas/node/edge dark/light resolver;
- Accordion: theme-live Surface + Accent title-card composition;
- MatrixSelector: semantic Panel/Button roles with revision tracking;
- ColorMatrix: semantic Panel/Button roles with revision tracking.

### Styling rule

Do not fix Dark mode by sprinkling independent hard-coded dark RGB values through
controls. Resolve a semantic theme role first. Domain-specific semantic colours
(e.g. Table warning/error cells) may be transformed from established defaults
when no general theme role represents the state.

Avoid innocent-looking configuration setters that enter `StyleEdit()` merely to
repeat an existing default: that turns a theme-derived style into an intentional
custom style and freezes the current theme snapshot.

## Deterministic validation

New package `Utilities/UiModelBindingContractTest` provides 49 checks: seven model
ownership/switching checks for each of List, Gallery, Tree, Table, Dropdown, Menu
and NodeGraph. In addition to internal/external ownership, it explicitly switches
from external dataset A to external dataset B and verifies that A is neither
copied nor cleared.

New package `Utilities/UiThemeSurfaceRegressionTest` provides 13 checks covering:

- standalone List Dark/Light viewport paint;
- complete Table Dark surfaces/domain chrome;
- Tree, Dropdown, Menu and NodeGraph dark surfaces;
- Accordion, MatrixSelector and ColorMatrix semantic dark theme state.

Windows validation must run these in Debug and Release together with the existing
R2 scale/render suites after this convergence pass.

## Legacy spelling cleanup

`Model()` is the canonical API. Existing repository/application source using
`GetInternalModel()`/`GetModel()` should migrate mechanically to `Model()` (using
`UseInternalModel()` first when internal ownership itself is intentional).

Those old spellings are thin aliases only; they do not represent or maintain a
second model. They should not appear in new documentation, examples or new code.
