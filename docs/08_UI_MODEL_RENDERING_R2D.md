# 08 — Ui Model Rendering R2D: Dropdown + Menu

This note records the implemented R2D convergence of `UiDropdown` and `UiMenu`
on the shared `UiItemRender` architecture defined by
`07_UI_MODEL_RENDERING_PLAN.md`.

## Goals

R2D removes presentation/model duplication without making Dropdown and Menu the
same control.

- `UiDropdown` uses `UiListModel` as its single authoritative row state.
- `UiMenu` keeps its domain-specific `UiMenuModel` and command semantics.
- both controls use `UiItemRender` for ordinary item content presentation;
- popup item renderer instances are bounded by visible popup rows;
- renderer geometry is prepared outside `Paint()`;
- control-specific interaction chrome stays with the owning control.

## UiDropdown

### One model authority

Dropdown no longer keeps a parallel `Vector<Item>` beside its bound
`UiListModel`. There are no model-to-item/item-to-model conversion passes and no
`RefreshFromModel()` mirror synchronization step.

The normal API works directly with `UiModelItem`:

```cpp
UiListModel model;
model.Add(UiModelItem("Draft", 1));
model.Add(UiModelItem("Published", 2));

drop.SetModel(model);
```

The convenience spelling `UiDropdown::Item` is a direct type alias for
`UiModelItem`; it does not define another object type or storage authority. New
code should prefer `UiModelItem` when shared model ownership is important to the
reader.

Dropdown mutation helpers copy/update the authoritative `UiModelItem` through
`UiListModel::Set()`, so model notifications remain the view synchronization
mechanism.

### Shared presentation

`SetItemRender(const UiItemRender&)` installs one renderer prototype. Dropdown
clones only the instances needed for current presentation:

- one collapsed-face renderer while the control has collapsed content;
- a bounded visible popup renderer pool while the popup is open.

The built-in default is a content-only, theme-aware `UiItemRenderBasic`
configuration derived from Dropdown styling.

The renderer presents ordinary content:

- icon;
- title;
- description;
- right text;
- custom text/font attributes carried by `UiModelItem`.

Dropdown remains responsible for:

- popup surface/frame;
- hot/selected row background;
- group-header/separator semantics;
- single/multi selection and check marker slot;
- collapsed checked-count badge;
- dropdown indicator;
- drag handle and insertion marker;
- drag reorder request/mutation policy;
- type-ahead and popup lifetime.

The old item-paint callback is not a second presentation authority.

### Paint discipline

Popup scrollbar/visible-range state and renderer layout are prepared by popup
layout, scrolling, model/style changes and explicit visibility changes.
`PopupWindow::Paint()` only consumes the prepared renderer pool plus Dropdown
chrome. It does not call renderer layout or rebuild popup scrollbar geometry.

`SetItemRender()` requests layout. U++ is allowed to service that layout request
before a caller later invokes `Layout()` explicitly. Therefore instrumentation
such as `GetLastRenderLayoutCount()` must not be used to require renderer
preparation to happen in one particular caller-visible layout turn. The durable
contract is that the replacement prototype/style survives cloning, the bounded
collapsed/popup renderer population is correct, model/selection state is
unchanged, and a subsequent unchanged layout performs no unnecessary renderer
relayout.

## UiMenu

### Domain model stays domain-specific

`UiMenuModel` remains the authoritative model. A menu item is not forced to
inherit from `UiModelItem` because it carries command-domain semantics such as:

- `command_id`;
- checkable/radio state;
- submenu hierarchy;
- shortcut/right text;
- separator/default-item semantics.

`UiMakeItemRenderData(const UiMenuItem&, ...)` projects only the ordinary visual
content into `UiItemRenderData`.

### Shared popup content renderer

`UiMenu::SetItemRender(const UiItemRender&)` installs the popup-content renderer
prototype. Every open popup level owns a bounded pool corresponding to its
visible rows. Closed menus allocate no per-model renderer objects.

The default renderer is a content-only `UiItemRenderBasic` configured from the
current Menu style. It presents:

- icon;
- title;
- optional description;
- shortcut/right text;
- default-item emphasis.

Menu itself continues to paint and own:

- popup/bar surfaces;
- hot and pressed row chrome;
- separator rules;
- check/radio glyphs;
- submenu arrow;
- menu-bar navigation;
- command activation and request-first mutation;
- popup stack/session/focus behavior.

This separation keeps `UiItemRender` reusable without teaching it Menu command
or topology semantics.

## Theme and style

The built-in Dropdown and Menu renderer prototypes are configured from their
resolved control styles rather than hard-coded standalone colours. A style/theme
change invalidates the corresponding prepared renderer pool and causes layout to
be prepared again before normal rendering.

Custom renderers remain explicit caller-owned presentation choices through
`SetItemRender()`; changing a renderer does not change the control model,
selection, command or popup semantics.

## Performance contract

Dropdown and Menu are not part of the 100,000-record hard scale family, but their
popup implementation follows the same architectural invariant:

> logical item count must not determine live presentation-object count.

Only visible popup rows have renderer instances. Scrolling rebinds/reuses the
bounded pool. Popup `Paint()` visits only the prepared visible range.

## Deterministic acceptance

`Utilities/UiDropdownMenuRenderTest` contains 11 focused checks covering:

- direct external `UiListModel` authority in Dropdown;
- external model update without mirror refresh;
- one collapsed renderer instance;
- no relayout on unchanged collapsed layout;
- renderer prototype/style replacement while preserving model and selection state;
- the `UiDropdown::Item` spelling resolving directly to `UiModelItem`;
- `UiMenuItem` to `UiItemRenderData` title/description/right mapping;
- Menu payload/command/default-item mapping;
- common renderer layout from Menu data;
- Paint consuming prepared geometry without relayout;
- Menu renderer prototype replacement while closed without per-model allocation.

The prototype-replacement check deliberately does not require
`GetLastRenderLayoutCount() == 1` after a subsequent explicit `Layout()`: Windows
validation showed that `RefreshLayout()` may already have prepared the new
renderer, making that later call correctly report zero new layouts. Requiring a
second layout would test scheduling rather than renderer/model correctness.

Windows acceptance should additionally build and exercise the existing
`UiDropdownDemo` and `UiMenuDemo`, including popup scrolling, selection/checking,
drag reorder where exposed, submenu navigation, keyboard behavior and Light/Dark
theme changes.
