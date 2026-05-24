# Ui Controls Guide

Primary guide for using, extending, and reviewing the U++ Ui control set. This consolidates the control guide, theme blueprint, sizing contract, paint hook rules, shared item-action direction, and control-base refactor notes so future developers and AI agents have one control-layer entry point.


---


# Imported Source: archive/u_ui_controls_guide.md

# UI Controls Guide (U++ Ui)

This is the living guide for building, reviewing, and using the repo's `Ui*` control set. It keeps the old checklist role, but also records the practical control-layer and demo-layer rules that new AI agents need before they touch the codebase.

Repo rules:
## What This Guide Covers

Use this file for three things:

- understanding what the current Ui* controls are
- reviewing new/changed controls before merge
- learning the repo-specific gotchas around theme/style/layout work

## Current Ui Controls

Core styled surfaces and infrastructure:

- UiStyle, CtrlStyled, UiDraw, UiTheme, UiIcons
- UiLayoutCursor for lightweight shell/demo placement

Primary controls:

- UiLabel, UiButton, UiCheckBox, UiToggle, UiRadioButton
- UiPanel, UiGroupPanel, UiAccordion, UiScrollPanel, UiStack, UiTitleCard
- UiBaseEdit, UiLineEdit, UiPasswordEdit, UiMaskEdit, UiMultiEdit
- UiIntEdit, UiFloatEdit, UiSlider, UiSliderEdit, UiScrollBar, UiSplitter
- UiColorPicker, UiDropdown, UiMenu, UiTab, UiBreadcrumbs
- UiList, UiTree, UiTable
- UiGridLayout, UiBoxLayout, UiDoc
- UiBezierCurveEditor, UiBezierCurveField
- UiOsFileDialog as a native OS dialog wrapper, not a themed dialog surface

Composite controls:

- UiCompositeLabel, UiCompositeEdit, UiCompositeDropdown
- UiCompositeSlider, UiCompositeToggle, UiCompositeColor
- Source location: `Ui/Composites/`. Include through `<Ui/Ui.h>` for normal
  app/demo code, or through `<Ui/Composites/UiComposite*.h>` for narrow control
  dependencies. New composite rows belong in this folder, not beside leaf
  controls in the root `Ui/` package directory.

Support controls:

- UiToolButton, UiIndicatorBase, UiIndicatorSupport

## Repo-Specific Control Rules

- Demos are tests: if a control API or baseline style changes, update the affected demos and copied usage output immediately.
- Prefer the control/theme default first. Do not add demo-local style overrides unless the demo is intentionally showing a variation.
- Do not restate defaults just because a setter exists. If a call does not materially change behavior, remove it.
- For UiLabel, remember that metrics.text_font is the actual paint path when metrics.use_text_font is enabled. Setting only style.font can be a no-op.
- `GetMinSize()` is the stable floor. `GetContentSize()` is the logical child/content extent. Container controls that host children should expose content extent so callers do not hand-measure panels.
- `UiPanel` reports visible child bounds as content size, but it is still a host surface, not a flow layout. Use `UiBoxLayout` when rows/items need wrapping or responsive sequence layout. Use `UiGridLayout` when children belong to stable row/column cells.
- `UiGroupPanel` is the titled grouping container. It paints group chrome and hosts one child through `SetContent()`. Put a box/grid layout inside that slot when the grouped body needs arrangement; do not add group chrome to layout engines.
- For accordions inside scroll panels, section body heights must be explicit only when a section intentionally contains a bounded viewport. Otherwise, accordion bodies should be able to measure real content through `MeasureHeightForWidth()` / `GetContentSize()`.
- Use UiLayoutCursor for lightweight shell/manual placement, but keep box/grid layouts for repeated stacked content.
- Add comments around meaningful constructor groups, layout groups, and theme groups in demos and new controls. New code should be readable without tracing every line.
- Keep style helpers compact. If a style helper mostly repeats defaults, trim it.
- For custom-painted primitives, use the dedicated part hooks (`WhenPaintTrack`, `WhenPaintActiveTrack`, `WhenPaintThumb`, `WhenPaintArrow`) instead of demo-local overpaint or geometry guessing.
- Track/thumb controls should use consistent vocabulary: track is the long lane/line, thumb is the draggable affordance, and optional icons/labels belong to the thumb unless the control has a separate item/content model.
- Splitters should expose the split as a unitless percentage/proportion by default. Exact pixel values belong on pane constraints such as minimum, preferred, or fixed pane size, and must be DPI-scaled.
- Container controls must keep `GetMinSize()`, `GetContentSize()`, and `MeasureHeightForWidth()` accurate when they host arbitrary child controls. Leaf controls report their intrinsic floor/content; host controls aggregate the visible child bounds or layout-computed extent; scroll/accordion/stack controls consume those measurements instead of guessing. This prevents the common accordion bug where a section reopens with a zero-height or stale body after controls are swapped or hidden.

- No backward-compat naming shims.
- Demos are tests: any API change requires updating `examples/**` and `README.md`.
- Prefer no allocations in `Paint()`; cache where possible.

## Sizing And Content Measurement

The authoritative detailed document is [UiSizing_Contract.md](E:\apps\github\upp_Ui\UPP_GUIDES\UiSizing_Contract.md). This guide carries the short version because every new control and demo needs it during review.

Use these meanings consistently:

- `GetMinSize()` is the stable floor a parent can negotiate against.
- `GetContentSize()` is the current logical extent of a control's own content or visible children.
- `MeasureHeightForWidth(width)` is for controls whose height depends on available width.

Container rules:

- `UiPanel` is a styled host. It reports visible child bounds through `GetContentSize()`, but it does not arrange children.
- `UiGroupPanel` is a styled titled host with one content slot. It reports the title/header and body content extent, but it does not arrange multiple body children by itself.
- `UiBoxLayout` and `UiGridLayout` are measured layout engines, but they have different contracts. `UiBoxLayout` owns wrapping/free-form sequence layout. `UiGridLayout` owns stable row/column placement; resizing may change track sizes, but it must not silently move an item to a different logical cell.
- `UiScrollPanel` is a bounded viewport. Its `GetContentSize()` is the scrollable document extent, not the viewport height.
- `UiAccordion` should measure real section content through width-aware measurement or `GetContentSize()`. Use explicit section body height only when the section intentionally contains a bounded viewport.

Demo and inspector rules:

- Use composite rows for inspector properties: `UiCompositeSlider`, `UiCompositeToggle`, `UiCompositeDropdown`, and `UiCompositeColor`.
- Composite row implementations live under `Ui/Composites/`; keep this
  separation so root `Ui/` remains the leaf-control and infrastructure surface.
- Put repeated inspector rows in `UiBoxLayout`, not in manually positioned `UiPanel` children.
- Avoid hand-maintained magic heights unless the UX is explicitly a fixed viewport.

## Shared spacing vocabulary

Use one naming system across item controls:

- `item_spacing`: spacing between repeated owned items such as accordion sections, list rows, menu rows, tree rows, dropdown popup rows, and tabs
- `content_gap`: primary spacing inside a single item/control surface, for example icon-to-text or check/icon/text lane spacing
- semantic secondary gaps stay explicit where they represent a separate lane, for example `right_gap`, `metadata_gap`, `drag_gap`, `chevron_gap`, or `accessory_gap`
- `content_margin`: outer inset around a control's painted content

Do not add alias setters or compatibility fields when renaming to this vocabulary.
## Gold delta checklist (copy 1:1)

Use this gate before merge for any new/changed `Ui*` control. `UiLabel` + `UiButton` are the gold references.

### Cache lifecycle

- [ ] Keep explicit dirty flags for derived caches (`layout_dirty_`, `metrics_dirty_`, `minsize_dirty_`).
- [ ] Recompute caches only in invalidation paths (`SetText`, `SetStyle`, `SetFont`, `Layout`, size/state changes), not in `Paint()`.
- [ ] Setters that affect geometry/text/style set the correct dirty flags and call `RefreshLayout()` (and `Refresh()` when visuals change).
- [ ] `Layout()` early-outs when caches are valid; otherwise rebuild once and clear dirty flags.
- [ ] `GetMinSize()` uses cached natural size and invalidates only when inputs change.

### Paint contract

- [ ] Paint phase order stays `background -> content -> foreground`.
- [ ] `Paint()` remains render-only: no hidden state mutation, no layout/text measurement recompute, no expensive allocation.
- [ ] If a cache is dirty inside `Paint()`, call `RefreshLayout()` and return (gold pattern).
- [ ] Content draw uses precomputed rects/splits (`layout_`, line sizes, icon rects), not ad-hoc recomputation.
- [ ] State mapping (`normal/hot/pressed/disabled/focus`) is explicit and consistent before drawing.

### Style contract

- [ ] Style structs are data-only (`StyledPalette`, `StyledMetrics`, `StyledSkin` first); behavior stays in control code.
- [ ] Reuse shared primitives/helpers; do not create parallel style/alignment/padding systems.
- [ ] Preserve `frame -> inset -> padding` semantics and shared `UiAlign` conventions.
- [ ] New public style fields satisfy the two-consumer rule (2 controls or 2 real use-cases) or include explicit rationale.
- [ ] Prefer variants/presets/theme tokens over one-off public setters.

### Event/data contract

- [ ] Interactive controls expose `SetData(const Value&)` and `GetData() const`.
- [ ] Action semantics are explicit (`WhenAction`, `WhenPush`, `WhenChange`, etc.) and fire exactly once per user action.
- [ ] User callbacks fire only after the control has committed its public state. Inside `WhenSelect`, `WhenSelectData`, `WhenSelection`, `WhenChange`, or `WhenAction`, callers must be able to read the new value through `GetData()`, `GetSelection()`, `GetCursor()`, or the equivalent public getter.
- [ ] Visual hover/highlight/cursor state is not a substitute for committed selection state. Do not fire callbacks from a transient highlight if `selected_index_`, `selected_ids_`, checked state, active cell, or the control's data token has not already been updated.
- [ ] Single-select and multi-select paths follow the same ordering rule: mutate the complete selection/cursor/anchor state first, then emit one callback for the user action.
- [ ] Disabled rows, group headers, separators, and placeholder rows are not selectable unless the control explicitly documents otherwise. Mouse, keyboard, `SetCursor`, `Select`, `SetData`, range selection, and select-all paths must share the same selectable predicate.
- [ ] Naming symmetry is kept (`SetX`/`GetX`).
- [ ] Async/timer callbacks are lifetime-safe (`Ptr` guard or equivalent), with teardown cancel where needed.
- [ ] GUI work stays on GUI thread; callback paths avoid long/blocking work.

## Selection And Callback Ordering

Selection controls must commit their model state before notifying user code. This rule exists because demos and designer tools commonly update inspectors, generated code, or dependent controls inside `WhenSelection` / `WhenSelectData`. If the callback observes the old state, the UI appears to accept a choice and then snaps back.

Required ordering:

1. Validate that the target is selectable.
2. Mutate the committed state: selected index/id/range, cursor/anchor, checked state, active cell, display text, and data token.
3. Refresh/invalidate visuals as needed.
4. Fire the public callback exactly once for the user action.

Do not use this brittle ordering:

```cpp
highlight_index_ = row;
ClosePopup();
PostCallback([=] { Select(highlight_index_); });
```

The highlight can be lost by popup teardown, focus changes, deactivation, scroll/accordion relayout, or page-stack changes. Prefer direct commit on activation:

```cpp
Select(row);              // updates selected_index_, display text, GetData()
ClosePopupInternal(false); // cleanup only; selection is already committed
```

Control-specific notes:

- `UiDropdown`: clicking or pressing Enter on a popup row must call `Select(idx)` before closing the popup. Popup close should not be the primary mechanism that commits a clicked row.
- `UiList`: `GetData()` must represent the selected row(s), not a stale cursor. `Select`, `SetCursor`, `SetData`, range selection, and keyboard movement must all reject disabled/group-header rows through the same selectable predicate.
- `UiTree`: `SelectSingle` and multi-select updates must set `cursor_id_` / `anchor_id_` before `NotifySelectionChange()`, because single-select `GetData()` and many handlers depend on the current selected node. Disabled/group-header/placeholder nodes must not become selected via mouse, keyboard, or programmatic data binding.
- `UiTable`: active cell and selection range must both be updated before `WhenSelection`; selection callbacks should see the new active cell and full range through `GetData()`.
- `UiMenu` and action rows: action callbacks should run after the menu item checked/radio state has been committed. Row hover/hot state alone is not a selected or checked state.

Programmatic `SetData()` deserves an explicit policy per control. If it fires selection/change events, it must still follow state-before-callback ordering. If it intentionally does not fire user-action events, document that in the header and demo.

## Build smoke tests

Recommended active-demo sweep (CLI). Build output should stay local to this repo so new developer setup does not accidentally use the U++ global output tree:

```powershell
$packages = Get-ChildItem -Path examples -Directory |
    Where-Object { $_.Name -ne 'OLD' } |
    Sort-Object Name |
    ForEach-Object { $_.Name }

foreach($pkg in $packages) {
    & 'E:\upp-18468\umk.exe' GitHubOut "examples/$pkg" CLANGx64 -br +GUI "E:\apps\github\upp_Ui\out\$pkg"
    if($LASTEXITCODE -ne 0) { throw "Build failed: $pkg" }
}
```

Retired prototype demos live under `examples/OLD` and are intentionally excluded from this sweep.

## Icon conversion utility (SVG/PNG)

- Tool: `Utilities/MakeIconFromSVG` (build target:
  `E:\apps\github\upp_Ui\out\MakeIconFromSVG.exe`).
- Default output mode: `iml`.
- Input: vector (`.svg`) and raster (`.png`, other `StreamRaster` formats).
- Shared icon workflow output:
  - `<base>.iml.append` for [UiIcons.iml](E:\apps\github\upp_Ui\Ui\UiIcons.iml)
  - `<base>.icons_h.append` for [UiIcons.h](E:\apps\github\upp_Ui\Ui\UiIcons.h)
- Legacy/local output mode: `uimakeicon`.
- Typical usage:

```bat
E:\apps\github\upp_Ui\out\MakeIconFromSVG.exe designs\search.svg
E:\apps\github\upp_Ui\out\MakeIconFromSVG.exe designs\check.svg designs\radio.svg --size 48x48 --output-base Ui\icon_batch
E:\apps\github\upp_Ui\out\MakeIconFromSVG.exe designs\search.svg --format uimakeicon --output-base Ui\newicons\search_icon
```

- Workflow note: the shared icon pack is IML-backed now. Append the generated `.iml.append` content into `Ui/UiIcons.iml` and the generated `.icons_h.append` content into `Ui/UiIcons.h`.

## Control status

Legend:

- In `Ui/Ui.upp`: included in the Ui package build
- Demo: `examples/<Control>Demo`

| Control | In `Ui/Ui.upp` | Active demo | Status | Notes / blockers | Next |
|---|---:|---:|---|---|---|
| UiStyle / CtrlStyled | yes | n/a | baseline | core styling surface | keep stable; add perf notes as needed |
| UiDraw | yes | n/a | baseline | 9-slice + blur helpers used across controls | ensure `Ui/Ui.upp` dependencies stay aligned |
| UiTheme | yes | UiThemeDemo | builds (umk) | theme resolver and specimen surface | keep demo theme-first; avoid demo-local policy duplication |
| UiIcons | yes | n/a | baseline | icon sources for demos | keep stable |
| UiLabel | yes | UiLabelDemo | builds (umk) | baseline for text metrics, alignment, and margins | run visual checks + refine edge cases |
| UiButton | yes | UiButtonDemo | builds (umk) | baseline for states + animation hooks | migrate remaining duplicated shell helpers to shared demo shell |
| UiToolButton | yes | UiButtonDemo / UiThemeDemo | builds (umk) | icon button variant | keep role policy in theme |
| UiCheckBox | yes | UiCheckBoxDemo | builds (umk) | classic/tri-state/switch/chip/list styles | add more grouping + keyboard behavior checks |
| UiToggle | yes | UiToggleDemo | builds (umk) | boolean switch wrapper over UiCheckBox switch style | migrate remaining duplicated shell helpers to shared demo shell |
| UiRadioButton | yes | UiRadioButtonDemo | builds (umk) | builder-shell demo exposes grouping and indicator/body geometry | add arrow-key group navigation |
| UiPanel | yes | UiPanelDemo | builds (umk) | background-only styled container; reports child content bounds | reduce old local style duplication over time |
| UiGroupPanel | yes | UiDocDemo / Designer | builds (umk) | titled group container with icon/subtitle/side-title header and one body slot | add focused demo if group-panel styling grows further |
| UiAccordion | yes | UiAccordionDemo | builds (umk) | styled collapsible sections with measured body content | add animated open/close + drag reorder if needed |
| UiScrollPanel | yes | UiScrollPanelDemo | builds (umk) | styled scroll container with viewport/content separation | add keyboard scroll navigation checks |
| UiStack | yes | n/a | builds as package dependency | headless page container; intended for designer/inspector page switching | add a focused UiStackDemo or utility test if it becomes public-facing |
| UiTitleCard | yes | UiTitleCardDemo | builds (umk) | header-focused card with title/subtitle/copy/media | add image-top and image-bottom demo variants |
| UiGridLayout | yes | UiDesigner utility / OLD prototypes | builds in Ui package | layout engine; old demos retired to `examples/OLD` | validate through designer and future generated-code tests |
| UiBoxLayout | yes | UiDesigner utility / OLD prototypes | builds in Ui package | layout engine; old demos retired to `examples/OLD` | validate through designer and future generated-code tests |
| UiBaseEdit | yes | edit-family demos | builds (umk) | foundation for edit family; side API stable | keep cursor/selection behavior tested through derived controls |
| UiLineEdit | yes | UiLineEditDemo | builds (umk) | single-line behavior and minimal edit theme | add behavioral checks for enter/tab/focus |
| UiPasswordEdit | yes | UiPasswordEditDemo | builds (umk) | side button/show-hide usage | visual pass on eye/submit compositions |
| UiMaskEdit | yes | UiMaskEditDemo | builds (umk) | masks, validators, formatters, validation feedback | add more semantic validator scenarios |
| UiMultiEdit | yes | UiMultiEditDemo | builds (umk) | multi-line edit behavior, side items, geometry, and color surfaces | add long-text perf checks |
| UiIntEdit / UiFloatEdit | yes | UiIntFloatDemo | builds (umk) | numeric edit spin arrows, ranges, wheel/key support | align style API with other edit controls |
| UiSlider | yes | UiSliderDemo | builds (umk) | ticks + keyboard/wheel + click-drag behavior | add label/tick text variants |
| UiSliderEdit | yes | UiSliderDemo | builds (umk) | slider + field composition | add integer mode and format presets |
| UiScrollBar | yes | UiScrollBarDemo | builds (umk) | styled track/thumb/arrows + hover-expand animation | add behavioral checks for drag, wheel, arrows |
| UiSplitter | yes | UiSplitterDemo | builds (umk) | styled track/thumb splitter with optional label/icon grip | add frame-splitter variant only if a real Ui layout needs it |
| UiColorPicker | yes | UiColorPickerDemo | builds (umk) | technical picker with alpha, slots, formatted values, and swatches | finish shared-shell migration and tighten spacing |
| UiDropdown | yes | UiDropdownDemo | builds (umk) | popup selection with state-before-callback ordering | keep model and callback ordering tests current |
| UiMenu | yes | UiMenuDemo | builds (umk) | menu rows, bar mode, action/check/radio rows | add keyboard/popup interaction checks |
| UiTab | yes | UiTabDemo | builds (umk) | tab placement, indicators, drag/close, body palettes | add keyboard navigation and overflow checks |
| UiBreadcrumbs | yes | UiBreadcrumbsDemo | builds (umk) | path/navigation crumbs with shared shell demo | add compact/overflow behavior if needed |
| UiList | yes | UiListDemo | builds (umk) | selectable rows, group headers, disabled rows, actions | keep selectable predicate shared across all paths |
| UiTree | yes | UiTreeDemo | builds (umk) | tree model, selection, expand/collapse, icons | keep model-selection ordering tested |
| UiTable | yes | UiTableDemo | builds (umk) | tabular model/control surface | add broader editing/navigation checks |
| UiDoc | yes | UiDocDemo | builds (umk) | rich doc base with selection, marks, search, annotations, undo/redo | grow block schema + serializers + screenplay mode policies |
| UiBezierCurveEditor / UiBezierCurveField | yes | n/a | builds in Ui package | curve editor/field controls | add demo when API stabilizes |
| UiOsFileDialog | separate package | UiOsFileDialogDemo | builds via demo (umk) | native OS wrapper; launch controls are themed, native dialog chrome is not | keep platform backends compile-covered through demo |
| UiCompositeLabel | yes | used by demos/designer | builds in Ui package | label/value inspector row | keep label-width/gap controls consistent |
| UiCompositeEdit | yes | used by demos/designer | builds in Ui package | label/edit inspector row | keep edit theme default; avoid custom demo styling |
| UiCompositeDropdown | yes | used by demos/designer | builds in Ui package | label/dropdown inspector row | keep dropdown selection state-before-callback |
| UiCompositeSlider | yes | used by demos/designer | builds in Ui package | label/slider/value inspector row | keep click-to-drag behavior consistent with UiSlider |
| UiCompositeToggle | yes | used by demos/designer | builds in Ui package | label/toggle/value inspector row | keep thumb state visually synchronized |
| UiCompositeColor | yes | used by designer | builds in Ui package | compact color property row | add focused public demo if needed |

## Release milestone: "v1 green baseline"

Criteria:

- `Ui/Ui.upp` builds cleanly.
- `UiLabelDemo` and `UiButtonDemo` build + run cleanly.
- README + docs match the actual API.
- Checklist shows clear blockers for all not-yet-added controls.

## Recent hardening log

- `UiBaseEdit` side visibility/spin fix:
  - `LayoutSides()` no longer force-shows side controls.
  - `UiIntEdit` / `UiFloatEdit` spin controls now use side visibility APIs so hide/show state and side-space reservation stay in sync.
- `UiTitleCard` paint pipeline + text metric cache:
  - Consolidated to one explicit paint pipeline (background -> content -> foreground/focus).
  - Added cached title/subtitle/copy text metrics with dirty invalidation on text/style changes.
- `UiGridLayout` header paint metric cleanup:
  - Replaced paint-time `GetTextSize(...)` for cluster headers with font metric usage.
- `UiAccordion` destructor/timer teardown + `SetData/GetData`:
  - Added explicit destructor teardown to stop active animation callbacks and drag state.
  - Added explicit `SetData/GetData` open-state contract (`ValueArray` mapping, lock-policy normalization).
- `UiDropdown` / `UiList` / `UiTree` selection ordering:
  - Selection callbacks now follow the state-before-callback rule so `GetData()` / `GetSelection()` are current inside handlers.
  - Dropdown popup activation commits `Select(idx)` before popup cleanup.
  - List/tree selectable predicates are shared across mouse, keyboard, data binding, and multi-select paths so disabled rows and group headers cannot be selected accidentally.
- Demo non-paint text-metric cleanup:
  - `UiGridLayoutDemo`, `UiButtonDemo`, `UiLabelDemo`, `UiPanelDemo`, and `UiDemoBase` updated to avoid paint-time `GetTextSize(...)` where practical.
- Active demo cleanup:
  - Active examples are built through the `examples` directory excluding `examples/OLD`.
  - Retired prototype/test packages such as old box/grid layout demos stay under `examples/OLD`.
  - `UiColorPickerDemo`, `UiIntFloatDemo`, `UiMaskEditDemo`, and `UiScrollPanelDemo` now use the shared builder shell.
  - Active demo source files carry a purpose/hygiene/changelog header so future agents know whether a package is a visual reference, smoke test, or specialized behavior demo.


---


# Imported Source: archive/u_theam_guide.md

# UiTheme Blueprint

## Purpose

Define a global theme system for the U++ UI layer that sits **above** `UiStyle.h` primitives and **below** per-control custom overrides.

This document is the authoritative design intent for `UiTheme.h` and related resolver code.

It is written so a developer or coding agent can understand not only the API shape, but also the architectural rules, responsibilities, precedence, and implementation goals.

---

## Core Intent

The existing styling system already provides a strong low-level foundation:

* `StyledPalette` for face/frame/ink/icon colors by control state
* `StyledMetrics` for geometry, padding, radius, frame width, shadow, highlight
* `StyledSkin` for skin images, 9-slice data, and geometry insets
* `UiDraw.h` helpers that already paint using those common styling primitives

The problem to solve is not lack of styling power.

The problem is **preset-level usability**.

Most developers do not want to tune many separate settings just to get a decent result. They want to say:

* Minimal
* Pill
* Linear
* Solid
* Outline
* Compact
* Layered

and have the entire UI adopt that visual family by default.

So the theme system must provide:

1. **One global theme preset source**
2. **Per-control role-based resolution**
3. **Optional per-instance local override**
4. **One central place where theme looks are authored**
5. **No duplication of visual policy across each control implementation**

---

## Styling Axes

This system must keep the major styling axes separate.

If these axes are blurred together, the API becomes confusing, controls drift apart,
and the theme system slowly turns into a pile of overlapping exceptions.

### 1. Primitive tokens

Defined by shared style primitives such as:

* palette values
* fills
* frame and face enable flags
* radius
* frame width
* spacing and padding
* shadow and highlight
* 9-slice skin data
* content inset

These are raw building blocks.

They are not a theme by themselves.

### 2. Preset

A preset is the global visual family.

Examples:

* `Minimal`
* `Pill`
* `Linear`
* `Solid`
* `Outline`
* `Compact`
* `Layered`

A preset defines broad visual language:

* surface treatment
* density tendency
* corner philosophy
* border emphasis
* elevation behavior
* panel treatment

### 3. Mode

Mode is the environment or palette mapping variant.

Examples:

* `Light`
* `Dark`
* `System`

Mode should mostly influence:

* color mapping
* contrast
* emphasis strength
* shadow/highlight interpretation

Mode should **not** be the primary mechanism for changing geometry language.

### 4. Role

Role is the semantic intent of a control inside the active preset.

Examples:

* button: `Standard`, `Accent`, `Subtle`, `Danger`
* label: `Body`, `Title`, `Caption`, `Badge`

Role answers:

* what this control means
* how strongly it should stand out
* how it should behave visually relative to sibling controls

Role is not a replacement for preset.

### 5. Visual mode

Some controls legitimately need control-specific structural variants.

Examples:

* checkbox: classic, chip, list-check
* radio: classic, pills, list

These are not theme presets.

They are structural rendering modes specific to that control family.

They may affect:

* indicator placement
* structural composition
* selection mark style
* layout model

Visual mode should only exist where the control truly has multiple structural forms.

### 6. Local override

This is the explicit per-instance escape hatch.

It allows a developer to take the fully resolved control style and modify it locally.

This must remain possible, but it must remain the highest-precedence exception layer,
not the primary authoring model.

---

## Architectural Rule

### Styling has three layers

#### 1. Primitive styling vocabulary

Lives in `UiStyle.h`

This layer defines the atoms:

* fills
* palette entries
* frame/face enable flags
* radius
* frame width
* content padding
* shadows
* highlights
* 9-slice skins
* content inset

This layer must remain generic and reusable.

It should **not** become a bag of hardcoded widget theme recipes.

#### 2. Theme + role resolution

Lives in `UiTheme.h` / `UiTheme.cpp`

This layer defines named looks and maps them to concrete `Style` objects for each control type.

Examples:

* `ResolveButtonStyle(UiThemePreset::Pill, UiThemeMode::Light, UiButtonRole::Accent)`
* `ResolveLabelStyle(UiThemePreset::Pill, UiThemeMode::Light, UiLabelRole::Caption)`

This is where the actual visual family is defined.

#### 3. Per-control resolved style and custom override

Lives in each control

Each control still keeps its own `Style` struct because each control has control-specific fields.

Examples:

* button press offset
* focus margin
* icon layout
* icon margin
* text margin
* label nowrap
* label transparency
* rich text span support

The control-specific `Style` remains necessary.

But it should become the **resolved result**, not the primary location where the main global theme family is authored.

---

## High-Level Goal

A programmer should be able to do this:

```cpp
UiTheme::SetPreset(UiThemePreset::Pill);
UiTheme::SetMode(UiThemeMode::Light);
```

Then instantiate controls normally and get a coherent UI.

They may optionally refine a control semantically:

```cpp
ok.SetRole(UiButtonRole::Accent);
cancel.SetRole(UiButtonRole::Subtle);
title.SetRole(UiLabelRole::Title);
note.SetRole(UiLabelRole::Caption);
```

And only if needed should they locally override the fully resolved style:

```cpp
UiButton::Style s = ok.GetResolvedStyle();
s.metrics.radius = DPI(12);
ok.SetStyle(s);
```

That is the intended precedence and usage model.

---

## Precedence Rules

These rules must be deterministic.

### Style source precedence

1. **Explicit local style override**
2. **Explicit local role + current global preset**
3. **Default role + current global preset**
4. **Hardcoded safe fallback**

This means:

* a programmer can still fully override any control
* most programmers do not need to override anything
* controls remain coherent when nothing special is set

### Legacy preset compatibility

During migration, existing per-control preset APIs such as:

* `StyleMinimal()`
* `StyleSoft()`
* `StyleStrong()`
* control-local named convenience styles

may remain temporarily for backward compatibility.

However, they must no longer be treated as independent theme-authoring surfaces.

They should become one of the following:

* wrappers over `UiTheme` resolver output, or
* deprecated compatibility conveniences

They must not become a second competing policy layer beside `UiTheme`.

No new control should introduce fresh family-level preset authorship once the
theme resolver path exists.

---

## What a Theme Preset Means

A theme preset is a **visual language preset**, not just a color palette.

A preset defines things such as:

* corner radius
* border strength
* fill treatment
* control density
* shadow/elevation behavior
* panel treatment
* selection emphasis
* light/dark palette mapping

A preset does **not** define a specific widget by itself.

Instead, the preset is combined with a **control role**.

For example:

* Pill + Button(Standard)
* Pill + Button(Accent)
* Pill + Label(Body)
* Pill + Label(Caption)

This avoids exploding the API into dozens of unrelated style names.

---

## Default Theme Preset Family

Recommended initial preset family:

* `Minimal`
* `Pill`
* `Linear`
* `Solid`
* `Outline`
* `Compact`
* `Layered`

### Intent of each preset

#### Minimal

Very low-noise modern UI.
Thin borders, restrained fills, light separators, flat presentation.

#### Pill

Soft modern UI with larger radii and friendlier control shapes.

#### Linear

Sharp rectangular geometry with crisp boundaries and strong structure.

#### Solid

Stronger filled surfaces and heavier visual weight.

#### Outline

Line-driven controls with restrained fills.

#### Compact

Dense professional layout with tighter spacing and smaller controls.

#### Layered

Modern surface hierarchy using cards, subtle elevation, and panel depth.
Not retro bevel. Not pure neumorphism.

---

## Theme Modes

At minimum:

* `Light`
* `Dark`

Potential future additions:

* `System`
* `HighContrast`

Theme mode must affect palette resolution, and may also affect certain emphasis values such as border visibility or shadow strength.

But geometry semantics should remain preset-driven rather than mode-driven.

---

## Control Roles

Roles give semantic meaning to a control inside the current preset.

### Button roles

Suggested initial button roles:

* `Standard`
* `Accent`
* `Subtle`
* `Icon`
* `Danger` (optional, but useful)

### Label roles

Suggested initial label roles:

* `Body`
* `Headline`
* `Subheadline`
* `Title`
* `Caption`
* `Badge`
* `Footnote`

These roles are not separate themes.
They are semantic variants within the active preset.

Roles should remain compact and semantically meaningful.

They should not turn into large visual catalogs such as:

* `PillPrimaryLargeGhostButton`
* `SoftSecondaryPanelLabel`

That kind of naming is a sign that preset, role, and local override concerns are
being mixed together.

---

## Global Theme Context

The framework needs one current theme context.

This context must be cheap to query and safe to use across controls.

Minimum contents:

* current preset
* current mode
* optional scale/density override later

Controls that do not have explicit local style overrides should resolve their appearance from this global context.

---

## Local Control Model

Each control should conceptually support:

* optional explicit local style override
* optional semantic role
* optional control-specific visual mode where structurally required
* ability to resolve its effective style from global theme if no explicit override exists

That means controls should not have to store every global decision themselves.

Instead they should expose something like:

* `SetStyle(const Style&)`
* `ClearStyleOverride()`
* `SetRole(...)`
* `GetResolvedStyle() const`
* `GetEffectiveStyle() const`

`GetResolvedStyle()` may mean the style built from global preset + role.
`GetEffectiveStyle()` may mean resolved style unless there is a local explicit override.

Either naming is fine as long as the semantics are consistent.

Controls should not need to care in `Paint()` whether the effective style came from:

* preset + mode + role
* preset + mode + role + visual mode
* explicit local override
* fallback default

They should paint from one final style object.

---

## Why Per-Control Style Still Exists

Even with a global theme system, `UiButton::Style` and `UiLabel::Style` still need to exist.

Reason:

Each control has fields that are specific to its behavior and layout.

A button needs things like:

* press offset
* focus margin
* icon images
* icon render mode (`UiIconRenderMode`)
* content alignment

A label needs things like:

* transparency
* nowrap
* span/rich text support assumptions
* icon/text spacing behavior

So per-control `Style` remains the correct runtime payload.

The change is this:

### Old mental model

Control style object is where the preset itself is authored.

### New mental model

Control style object is the final resolved payload built by the theme resolver.

That is the key architectural shift.

---

## Resolution Responsibility

`UiTheme` owns the recipes.

Each resolver function should create a fully usable control style for the requested:

* preset
* mode
* role

Example:

```cpp
UiButton::Style ResolveButtonStyle(UiThemePreset preset,
                                   UiThemeMode mode,
                                   UiButtonRole role);

UiLabel::Style ResolveLabelStyle(UiThemePreset preset,
                                 UiThemeMode mode,
                                 UiLabelRole role);
```

These functions should:

1. start from a sensible default style object
2. apply preset-level geometry/surface rules
3. apply light/dark palette mapping
4. apply role-specific semantic adjustments
5. return the final style

This is the single authoritative place where the look of each theme is defined.

Resolver code should be the only place where family-level visual policy is authored.

If multiple controls need the same family-level visual behavior, that should be
expressed through shared recipe helpers inside the theme layer, not by repeating
similar `StyleMinimal/Soft/Strong` logic in every control.

---

## What Must Not Happen

The following are anti-patterns and should be avoided:

### 1. Every control manually hardcodes theme policy

Bad because behavior drifts.

### 2. `UiStyle.h` becomes filled with per-widget preset recipes

Bad because it mixes primitives with theme policy.

### 3. Every UI instance must manually set every control’s style preset

Bad because adoption becomes tedious and inconsistent.

### 4. Theme presets are just color swaps

Bad because style families should differ in geometry, density, borders, and depth as well.

### 5. Controls lose the ability to override locally

Bad because custom UI work still needs escape hatches.

### 6. Theme family concepts are duplicated across controls

Bad because each control will slowly diverge in tone, naming, and behavior.

If `Minimal`, `Pill`, `Outline`, or similar family ideas exist, they should be
centrally authored once and resolved outward.

### 7. Roles become a dumping ground for visual special cases

Bad because semantic roles must stay understandable.

If a use-case is truly one-off, it belongs in a local style override.

If it is structural and reusable within one control family, it may belong in a
control-specific visual mode.

If it is a whole-UI family rule, it belongs in the preset layer.

### 8. "Anything is possible" is implemented as "everything is public"

Bad because raw flexibility without boundaries leads to bloat.

The system should provide broad expressive power through:

* shared primitives
* centralized resolvers
* semantic roles
* local override

not through unbounded growth of tiny public setters and ad hoc style variants.

---

## Practical Resolution Pattern

### Control paint path should use effective style only

A control should paint using one final style reference.

Pseudo-flow:

```cpp
const Style& s = GetEffectiveStyle();
UiPaintStyledSurface(... s.palette, s.metrics, s.skin ...);
```

That means a control’s paint code should not need to know whether the style came from:

* explicit local override
* global preset resolution
* fallback default

It just paints the final resolved style.

This keeps paint code simple and stable.

---

## Serialization Intent

The global theme preset itself may be process-global rather than per-control serialized.

A local explicit style override, if used, may still be serializable exactly as the control already supports.

That means the theme system should not break existing style serialization logic.

Local explicit styles are durable.
Global theme context is ambient.

If explicit local style serialization already exists for controls, the introduction
of `UiTheme` should preserve that behavior rather than replacing it.

---

## Recommended Implementation Order

### Stage 1

Introduce `UiThemePreset`, `UiThemeMode`, and role enums.

### Stage 2

Add `UiTheme` API for global preset and mode.

### Stage 3

Implement resolver functions for button and label.

### Stage 4

Add per-control role and explicit-style-override flags.

### Stage 5

Make controls paint from `GetEffectiveStyle()`.

### Stage 6

Migrate old `StyleMinimal()`, `StyleSoft()`, etc. into either:

* compatibility wrappers over the resolver, or
* deprecated convenience presets

### Stage 7

Extend to other controls such as panels, tabs, tree/list, inputs, and toolbars.

---

## Agent Guidance: Design Intent in Plain Terms

If an implementation agent reads this document, it should understand the following:

1. The project already has strong styling primitives.
2. The goal is not to replace that low-level system.
3. The goal is to add a clean, curated preset layer above it.
4. The preset layer must be global by default.
5. Controls must still support local explicit override.
6. Control-specific style structs remain valid and necessary.
7. Theme presets should be authored centrally, not scattered across widgets.
8. The resolver must output concrete control styles composed from shared primitives.
9. The system must optimize for fast adoption by ordinary developers.
10. Most users should get a coherent modern UI simply by selecting one preset.

---

## Reference Header Sketch

```cpp
#ifndef _Ui_UiTheme_h_
#define _Ui_UiTheme_h_

#include <Ui/UiStyle.h>
#include <Ui/UiButton.h>
#include <Ui/UiLabel.h>

namespace Upp {

// -----------------------------------------------------------------------------
// Global theme preset family
// -----------------------------------------------------------------------------

enum class UiThemePreset {
    Minimal,
    Pill,
    Linear,
    Solid,
    Outline,
    Compact,
    Layered
};

enum class UiThemeMode {
    Light,
    Dark,
    System
};

// -----------------------------------------------------------------------------
// Semantic roles per control family
// -----------------------------------------------------------------------------

enum class UiButtonRole {
    Standard,
    Accent,
    Subtle,
    Icon,
    Danger
};

enum class UiLabelRole {
    Body,
    Headline,
    Subheadline,
    Title,
    Caption,
    Badge,
    Footnote
};

// -----------------------------------------------------------------------------
// Optional future extension point for broader context
// -----------------------------------------------------------------------------

struct UiThemeContext {
    UiThemePreset preset = UiThemePreset::Minimal;
    UiThemeMode   mode   = UiThemeMode::Light;
};

// -----------------------------------------------------------------------------
// Theme manager / resolver entry points
// -----------------------------------------------------------------------------

class UiTheme {
public:
    static void SetPreset(UiThemePreset preset);
    static UiThemePreset GetPreset();

    static void SetMode(UiThemeMode mode);
    static UiThemeMode GetMode();

    static void SetContext(const UiThemeContext& ctx);
    static UiThemeContext GetContext();

    // Resolve concrete styles for controls from the active global context.
    static UiButton::Style ResolveButton(UiButtonRole role = UiButtonRole::Standard);
    static UiLabel::Style  ResolveLabel(UiLabelRole role = UiLabelRole::Body);

    // Resolve styles explicitly, without depending on global state.
    static UiButton::Style ResolveButton(UiThemePreset preset,
                                         UiThemeMode mode,
                                         UiButtonRole role = UiButtonRole::Standard);

    static UiLabel::Style ResolveLabel(UiThemePreset preset,
                                       UiThemeMode mode,
                                       UiLabelRole role = UiLabelRole::Body);
};

// -----------------------------------------------------------------------------
// Optional helper namespace if you prefer recipe-style organization
// -----------------------------------------------------------------------------

namespace UiThemeDefaults {
    UiButton::Style MakeButton(UiThemePreset preset,
                               UiThemeMode mode,
                               UiButtonRole role = UiButtonRole::Standard);

    UiLabel::Style MakeLabel(UiThemePreset preset,
                             UiThemeMode mode,
                             UiLabelRole role = UiLabelRole::Body);
}

} // namespace Upp

#endif
```

---

## Notes About This Header

This is intentionally a sketch, not final law.

The important architectural points are:

* the theme preset is global by default
* the resolver is central
* roles are semantic
* control style structs remain the final payload
* local override remains possible

The exact static-vs-instance approach for `UiTheme` can still be adjusted to fit the rest of the U++ architecture.

One implementation caution matters early: avoid creating unnecessary header coupling
between controls and `UiTheme`.

If roles, context, or notification hooks become shared dependencies, it is usually
cleaner to separate lightweight theme enums/context from resolver implementations
rather than making every control include heavy theme headers directly.

---

## Recommended Per-Control Integration Pattern

For controls like `UiButton` and `UiLabel`, the intended future shape is approximately:

```cpp
class UiButton : public Ctrl, public CtrlStyled<UiButton> {
public:
    UiButton& SetRole(UiButtonRole role);
    UiButtonRole GetRole() const;

    UiButton& SetStyle(const Style& s);          // explicit local override
    UiButton& ClearStyleOverride();
    bool HasStyleOverride() const;

    const Style& GetEffectiveStyle() const;

private:
    UiButtonRole role_ = UiButtonRole::Standard;
    bool         has_style_override_ = false;
    Style        style_;
    mutable Style resolved_style_cache_;
};
```

Equivalent pattern for `UiLabel`.

The control can cache the resolved style if useful, but cache invalidation must be clear whenever:

* global theme changes
* local role changes
* local visual mode changes
* explicit local style changes

If global theme notifications are introduced, they should be narrow and predictable.
The theme system should not require controls to poll or manually rebuild every paint.

Cache invalidation must remain explicit and cheap.

---

## Review Lens for Future Changes

When reviewing theme-related changes, ask:

1. Is this adding a new primitive token, or is it actually sneaking in theme policy?
2. Is this a global family concern, a semantic role concern, a structural control-mode concern, or a one-off local override?
3. Will this naming still make sense when more controls are added?
4. Does this change centralize policy, or spread it wider?
5. Does this make common usage simpler, or only make internals more configurable?

If a change cannot be placed cleanly on one layer, it likely needs redesign.

---

## Final Principle

The styling engine is already the orchestra.

`UiTheme` is the conductor and the score.

The goal is to let ordinary developers press one good preset and get a polished UI, without taking away the ability for experts to fine-tune anything later.

---


# Imported Source: archive/UiSizing_Contract.md

# UI Sizing Contract

## Purpose

Define one stable sizing contract for nested controls so layouts, accordions,
and scroll containers do not enter width/height feedback loops.

This document is the working rule for:
- `UiAccordion`
- `UiScrollPanel`
- `UiBoxLayout`
- `UiGridLayout`
- `UiTree`
- `UiLabel`
- other item/document controls added later

## Core Rules

### 1. `GetMinSize()` is the stable floor

`GetMinSize()` answers:
- "What is the smallest safe size this control can accept?"

Rules:
- must be conservative
- must be stable
- must not depend on transient scrollbar visibility
- must not oscillate when parent width changes by a few pixels

`GetMinSize()` is for parent layout negotiation, not for exact content extent.

### 2. `GetContentSize()` is actual content extent

`GetContentSize()` answers:
- "What size is the control's content currently occupying or logically needs?"

Rules:
- may depend on current layout/content
- may be larger than `GetMinSize()`
- is appropriate for scroll containers, inspectors, and diagnostics

Use it for:
- `UiScrollPanel`
- `UiTree`
- `UiBoxLayout`
- `UiGridLayout`
- `UiPanel` as a host/container fallback
- text/document-like display controls where actual content extent matters

`GetContentSize()` must not be treated as optional plumbing for container
controls. If a control can own visible children, a parent should be able to ask
for the logical extent of those children without the application manually
measuring them. This is the practical safeguard that prevents accordion,
scroll-panel, and inspector code from depending on hand-maintained magic
heights.

### 3. Width-aware measurement must be explicit

If a parent needs height as a function of width, do not abuse `GetMinSize()`.

Use:
- `MeasureHeightForWidth(int width)`

This is appropriate for:
- wrapping layouts
- rich/wrapping labels
- document-like controls
- accordion bodies that depend on available width

### 4. Parent layout must be one-way

The contract is:
1. parent determines available rect
2. child lays itself out within that rect
3. child reports stable size contracts upward

Avoid:
- measuring child from transient rects
- using width-sensitive `GetMinSize()` to drive parent height
- scrollbar visibility changing parent width and immediately re-triggering a different child "minimum"

### 5. Scroll containers need bounded convergence

`UiScrollPanel` must:
- measure content
- decide scrollbars
- re-evaluate viewport once if bars changed
- stop there

It must not enter an unbounded viewport/content oscillation loop.

### 6. Accordion bodies need explicit policy

Accordion body sizing must support:
- fixed height
- auto height from actual content

Policy:
- if section body height is explicitly set, use it
- otherwise prefer actual content extent
- use width-aware measurement only when a child explicitly supports it
- fall back to `GetMinSize()` only as a conservative fallback

For inspector-style panes, fixed body height is valid and often preferable.

### 7. Host panels report child bounds, layouts report flow

`UiPanel` is a styled host, not a layout engine. Its `GetContentSize()` reports
the bounding extent of visible child controls, including its own styled outer
metrics. This makes simple panel hosting work correctly in accordions and scroll
containers.

Use `UiBoxLayout` when content must be arranged, wrapped, or remeasured as a
sequence for a new width. Use `UiGridLayout` when content belongs to stable
row/column cells. A panel can report where children currently are; it does not
decide where they should go.

Practical rule:
- simple painted host: `UiPanel`
- stacked/repeated property rows and responsive wrapping sequences: `UiBoxLayout`
- stable row/column placement: `UiGridLayout`
- bounded scrolling viewport: `UiScrollPanel`

## Box vs Grid Responsive Contract

`UiBoxLayout` is the responsive wrapping layout. A horizontal box with wrap may
move later children onto a new row when the available width changes. That is
expected because the contract is ordered sequence placement, not stable cell
addressing.

`UiGridLayout` is a stable cell layout. A child added to row 1, column 2 remains
assigned to that logical cell when the parent resizes. Grid responsiveness comes
from row/column track sizing, child fit/fixed/expand behavior, minimum cell
targets, and scroll/content-size reporting. It must not reflow explicit cells
like a gallery.

Grid auto helpers are allowed only when they preserve this contract:

```cpp
int cols = UiGridLayout::ComputeColumns(parent_width, approx_cell_width, gap);
int rows = UiGridLayout::ComputeRows(parent_height, approx_cell_height, gap);
grid.SetGridSize(cols, rows);
```

Those helpers compute an initial or caller-requested grid size. They do not make
cell ids fluid. `Add(ctrl)` means "next free cell"; `Add(ctrl, row, col, ...)`
means "this explicit cell".

## Proposed Public API Signatures

### Shared / recurring pattern

```cpp
Size GetContentSize() const;
int  MeasureHeightForWidth(int width) const;
```

Not every control needs both.

### `UiScrollPanel`

```cpp
Size GetContentSize() const;
Rect GetViewportRect() const;
```

### `UiTree`

```cpp
Size GetContentSize() const;
```

### `UiBoxLayout`

```cpp
Size GetContentSize() const;
int  MeasureHeightForWidth(int width) const; // when wrap auto-resize is relevant
```

### `UiGridLayout`

Stable grid surface:

```cpp
Size GetContentSize() const;
int  MeasureHeightForWidth(int width);
UiGridLayout& SetGridSize(int columns, int rows);
UiGridLayout& SetMinCellSize(Size sz);
static int  ComputeColumns(int available_width, int approx_cell_width, int gap = 0);
static int  ComputeRows(int available_height, int approx_cell_height, int gap = 0);
static Size ComputeGrid(Size available, Size approx_cell, Size gap = Size(0, 0));
```

### `UiLabel`

Already present:

```cpp
Size GetContentSize() const;
```

If width-aware wrapping measurement is formalized later:

```cpp
int MeasureHeightForWidth(int width) const;
```

### `UiAccordion`

Public policy stays simple:

```cpp
UiAccordion& SetSectionBodyHeight(int index, int px);
```

Internal measurement rule should prefer:
1. fixed body height
2. child `MeasureHeightForWidth(width)` where width-aware flow is supported
3. child `GetContentSize()`
4. child `GetMinSize()`

## Container Measurement Policy

### `UiAccordion`

When measuring a section body:
- use explicit section body height if present
- otherwise inspect visible child controls in section content
- for each child, prefer width-aware layout measurement where available, then actual content extent over guessed min-size heuristics
- compute a stable union height

Do not let width-sensitive children cause oscillation through naive `GetMinSize()`.

If a section contains a `UiScrollPanel`, remember that its content size is the
scrollable document extent. That is correct for an outer inspector scroll, but
not automatically correct for a bounded accordion section. Use an explicit
section body height when the scroll panel is meant to be a viewport inside the
accordion.

### `UiScrollPanel`

`GetMinSize()` remains stable and conservative.

`GetContentSize()` exposes:
- current logical content size after layout

Parent containers can use it when they care about actual document/content extent.

## Demo Guidance

Builder demos should follow these rules:
- `MODEL DATA` trees in inspector accordions use fixed section body height
- do not rely on width-sensitive auto body height for inspector tools
- keep editor sync guarded so programmatic refresh does not mutate model state
- put repeated inspector controls in `UiBoxLayout` and use composite rows
- use `UiPanel` for visual surfaces, not as the primary property-row layout

This is not a hack. It is an explicit UI decision:
- inspector panes are bounded viewers
- document panes are auto/flexible viewers

## Rollout Order

### Phase 1
- `UiScrollPanel`: expose `GetContentSize()`
- `UiTree`: expose `GetContentSize()`
- `UiBoxLayout`: expose `GetContentSize()`
- `UiAccordion`: measure section bodies via stable content rules

### Phase 2
- formalize width-aware measurement on controls that truly need it
- audit `UiLabel`, `UiDoc`, `UiTable`, `UiMenu`, `UiTab`

### Phase 3
- normalize demos to fixed-height inspector sections where appropriate
- remove layout oscillation repros from active builder demos

## Current Lessons

- A control inside an accordion inside a scroll panel is the stress test for the library sizing contract.
- The failure pattern is usually not one bad control. It is a feedback loop between:
  - body measurement
  - viewport width
  - scrollbar visibility
  - width-sensitive size reporting
- Fix the contract at the container boundary first, then refine child controls.

---


# Imported Source: archive/UiCustomPaint_Hooks_Proposal.md

# Ui Custom Paint Hooks Proposal

## Purpose

Define a clean, reusable paint-hook contract for controls that need caller-owned
visual surfaces without forcing hacks into demo code or control-local paint
workarounds.

The immediate driver is `UiSlider`, where color-adjustor tracks such as:

- hue ramps
- alpha/checker tracks
- gain/exposure ramps

need to paint inside the control's real geometry, not in a vague outer
background callback.

This proposal is intentionally broader than slider. The same pattern should be
used whenever a control exposes a visually meaningful sub-surface that callers
may need to replace or augment.

## Problem

Today `UiSlider` already has:

- `WhenPaintBackground`
- `WhenPaintForeground`

Those are useful for broad decoration, but they are not the right abstraction
for control-part painting because:

- they do not expose the control's real sub-rects
- they do not communicate which part is being painted
- they do not provide a clean "handled" path for replacing default paint
- they force callers to reverse-engineer geometry or overpaint defaults

That is the wrong contract for a color track.

## Design Goals

- Keep default control painting intact by default.
- Let callers replace or augment a specific control part cleanly.
- Expose exact geometry for the part being painted.
- Preserve a consistent hook vocabulary across controls.
- Avoid callback soup with one-off names that do not generalize.
- Avoid forcing callers to subclass controls just to paint one surface.

## Core Rule

Use generic background/foreground events for shell decoration only.

Use part-aware paint hooks for real control surfaces.

Examples of real control surfaces:

- slider track
- slider active track
- slider thumb
- accordion chevron lane
- list row action lane
- dropdown indicator lane
- table cell overlay

## Proposed Hook Pattern

For any control with paintable sub-parts:

1. Expose the geometry of the relevant parts.
2. Expose a paint hook that targets those parts directly.
3. Allow the hook to suppress default paint for that part.

## Proposed Slider API

Add a small paint context struct in
[E:\apps\github\upp_Ui\Ui\UiSlider.h](E:\apps\github\upp_Ui\Ui\UiSlider.h):

```cpp
struct UiSliderPaintContext {
    Rect outer;
    Rect track;
    Rect active_track;
    Rect thumb;
    const UiSlider::Style* style = nullptr;
    StyledState state = ST_NORMAL;
    bool has_focus = false;
    UiDirection direction = UiDirection::H;
    double min = 0.0;
    double max = 0.0;
    double value = 0.0;
};
```

Then add dedicated hooks:

```cpp
Event<Draw&, const UiSliderPaintContext&, bool&> WhenPaintTrack;
Event<Draw&, const UiSliderPaintContext&, bool&> WhenPaintActiveTrack;
Event<Draw&, const UiSliderPaintContext&, bool&> WhenPaintThumb;
```

Contract:

- `handled` starts as `false`
- callback may paint the part and set `handled = true`
- if `handled` remains `false`, the control paints its built-in default

This keeps the control deterministic and avoids double-paint hacks.

## Why This Is Better Than Only Using Style

The style system already allows image-based track fill through `UiFill`, which
is useful and should remain supported.

That is good for:

- static hue ramp image
- static alpha checker image
- simple gradient tile fill

But style alone is not enough for all cases:

- caller may want generated per-size painting
- caller may want value-aware active-track visuals
- caller may want a non-image procedural track
- caller may want to replace only one part while leaving others default

So the right model is:

- style for declarative defaults
- part hooks for explicit custom rendering

Not one or the other.

## Default Paint Order For Slider

The default order should remain:

1. `WhenPaintBackground`
2. track default or `WhenPaintTrack`
3. active-track default or `WhenPaintActiveTrack`
4. ticks
5. thumb default or `WhenPaintThumb`
6. `WhenPaintForeground`

That preserves current shell behavior while making the meaningful visual parts
replaceable.

## Geometry Rules

The control, not the caller, owns geometry.

The caller must not be expected to infer:

- track rect
- active segment rect
- thumb rect

from the outer bounds.

If the control is painting a sub-part, the hook should receive that exact rect.

## Naming Policy

Use explicit part naming:

- `WhenPaintTrack`
- `WhenPaintActiveTrack`
- `WhenPaintThumb`

Do not use vague names like:

- `WhenPaintCustom`
- `WhenPaintExtra`
- `WhenPaintOverlay`

because they do not scale across controls.

## Generalization Pattern For Other Controls

When another control needs caller-owned painting of a real sub-surface, follow
the same structure:

1. define a small paint context
2. expose exact geometry
3. expose a part-aware `WhenPaint...`
4. allow `handled = true` to suppress default paint

Examples:

`UiAccordion`
- `WhenPaintChevronLane`
- `WhenPaintDragLane`

`UiDropdown`
- `WhenPaintIndicator`
- `WhenPaintPopupRowOverlay`

`UiList`
- `WhenPaintRowAccessory`
- `WhenPaintDragHandle`

`UiTable`
- `WhenPaintCellOverlay`

This should only be added when there is a real need for caller-owned paint of a
meaningful control part. Do not add paint hooks speculatively.

## Current Implementation Scope

This pattern is now implemented in:

- `UiSlider`
- `UiScrollBar`
- `UiToggle`
- `UiColorPicker` as a consumer of the slider hooks for:
  - hue track
  - gain/value track
  - alpha track

Reason:

- `UiSlider` was the initial concrete driver
- `UiScrollBar` is the other core `track + thumb` primitive
- `UiToggle` should follow the same rule for switch-style controls
- `UiColorPicker` proves the contract in a real utility control rather than only in an isolated demo

## Example Usage

Hue track:

```cpp
slider.WhenPaintTrack = [&](Draw& w, const UiSliderPaintContext& ctx, bool& handled) {
    DrawHueRamp(w, ctx.track);
    handled = true;
};
```

Alpha track:

```cpp
slider.WhenPaintTrack = [&](Draw& w, const UiSliderPaintContext& ctx, bool& handled) {
    DrawChecker(w, ctx.track);
    DrawAlphaRamp(w, ctx.track, current_rgb);
    handled = true;
};
```

Gain / exposure track:

```cpp
slider.WhenPaintTrack = [&](Draw& w, const UiSliderPaintContext& ctx, bool& handled) {
    DrawSolidTrack(w, ctx.track, Color(92, 98, 104));
    handled = true;
};
```

Custom thumb with default active fill retained:

```cpp
slider.WhenPaintThumb = [&](Draw& w, const UiSliderPaintContext& ctx, bool& handled) {
    PaintColorKnob(w, ctx.thumb, current_rgb, ctx.has_focus);
    handled = true;
};
```

## What Not To Do

- Do not require subclassing just to paint a track.
- Do not require callers to overpaint after default rendering.
- Do not expose raw internal geometry helpers publicly unless they are part of
  the stable API contract.
- Do not route geometry-sensitive paint through only
  `WhenPaintBackground/Foreground`.

## Rollout Steps

Completed:

1. Added `UiSliderPaintContext` to
   [E:\apps\github\upp_Ui\Ui\UiSlider.h](E:\apps\github\upp_Ui\Ui\UiSlider.h)
2. Added `WhenPaintTrack`, `WhenPaintActiveTrack`, and `WhenPaintThumb`
3. Updated `UiSlider::Paint()` in
   [E:\apps\github\upp_Ui\Ui\UiSlider.cpp](E:\apps\github\upp_Ui\Ui\UiSlider.cpp)
4. Preserved existing background/foreground hooks unchanged
5. Normalized the same contract onto:
   - [E:\apps\github\upp_Ui\Ui\UiScrollBar.h](E:\apps\github\upp_Ui\Ui\UiScrollBar.h)
   - [E:\apps\github\upp_Ui\Ui\UiToggle.h](E:\apps\github\upp_Ui\Ui\UiToggle.h)
6. Used the contract in:
   - [E:\apps\github\upp_Ui\Ui\UiColorPicker.cpp](E:\apps\github\upp_Ui\Ui\UiColorPicker.cpp)
   for hue/gain/alpha track rendering

Next:

1. Extend demo coverage so `UiSliderDemo` can explicitly showcase custom painted hue and alpha tracks
2. Reuse the same contract only where another control exposes a real caller-meaningful part surface

## Recommendation

This is the best fit for the current system:

- it respects the existing `WhenPaint...` model
- it avoids hacks
- it keeps geometry ownership inside the control
- it creates a reusable contract for future controls

The next controls should only adopt this pattern when they expose a real,
caller-meaningful sub-surface. It should not be added speculatively.

---


# Imported Source: archive/UiItemAction_System_Proposal.md

# Ui Item Action System Proposal

## Goal
Add a clean, shared side-action concept for item-driven controls without
turning rows or headers into arbitrary child-control hosts.

Target controls:
- `UiAccordion`
- `UiList`
- `UiTree`
- `UiDropdown`

This proposal is intentionally aligned with the current control architecture:
- built-in structural affordances keep precedence
- user-added actions are control-owned and paint-driven
- the public API stays small and explicit

## Scope
This is not a generic embedded-widget system.

It is a lightweight painted affordance system for:
- per-item or per-header side icons
- optional drag reorder where the control supports it
- consistent side placement and hit-testing rules

## Core Design Rules
1. Built-in structural affordances always keep edge precedence.
2. User-added item actions occupy the remaining side lane.
3. Actions are laid out inward from the chosen side in insertion order.
4. Main content uses whatever width remains after structural affordances and
   actions are reserved.
5. Default action icon rendering preserves source color unless explicitly
   overridden.

Examples of structural affordances that keep precedence:
- accordion chevron
- accordion drag handle if exposed later
- dropdown indicator
- tree expander glyph
- any control-native required glyph

## Proposed Public API
Canonical addition call:

```cpp
UiItemActionHandle AddItemAction(const Image& icon,
                                 UiAlign side,
                                 Function<void(int)> action);
```

Where the callback `int` is:
- accordion section index
- list row index
- tree visible-row or node-mapped index
- dropdown popup row index

This API shape is intended to be consistent across the four controls.

## Proposed Handle API
Returned handle should be fluent and minimal:

```cpp
handle.Visible(bool on = true);
handle.Enabled(bool on = true);
handle.Size(int px);
handle.RenderMode(UiIconRenderMode mode);
handle.Ink(Color c);
handle.Gap(int px);
handle.Tooltip(const String& tip);
handle.Remove();
```

Default behavior:
- visible: `true`
- enabled: `true`
- render mode: `UiIconRenderMode::PreserveColor`
- ink: `Null`
- size: control default
- gap: control default

## Naming Direction
Use `ItemAction` as the shared noun.

Reason:
- works naturally for list/tree/dropdown rows
- still works for accordion sections as header items
- keeps naming aligned without inventing control-specific synonyms

Preferred shared names:
- `AddItemAction(...)`
- `SetItemActionSize(...)`
- `SetItemActionGap(...)`

Avoid:
- `AddToSideActionIcon(...)`
- `SetActionThingSide(...)`
- generic `Action` names with unclear scope

## Placement And Ordering
For a given side:
1. reserve built-in structural affordances first
2. reserve user-added item actions next
3. layout content in the remaining span

So if the right side contains:
- dropdown indicator
- then two added item actions

the order is:
- rightmost: indicator
- next inward: newest or earlier-added action depending on chosen insertion rule
- next inward: next action
- then text/content

Implementation should document and preserve one insertion rule consistently.
Recommendation: insertion order from the edge inward.

## Drag Reorder
Drag reorder is separate from item actions, but it follows the same side-lane
precedence logic when a visible drag affordance exists.

Recommended control-level API:
- `EnableDragReorder(bool on = true)`

Optional later:
- `ShowDragHandle(bool on = true)`

Do not fold drag behavior into `AddItemAction(...)`.

## Control-Specific Notes

### UiAccordion
- actions apply to section headers
- chevron remains outermost structural affordance
- if drag handles are later added visually, they also outrank item actions

### UiList
- good first implementation target
- current row layout is paint-driven and already reserves multiple row lanes
- add hit zones and action-lane reservation instead of child controls

### UiTree
- apply actions to visible rows/nodes
- expander glyph keeps precedence
- node identity and visible-row mapping must be handled carefully

### UiDropdown
- apply actions to popup rows only
- collapsed face stays a separate layout contract
- dropdown indicator remains outermost structural affordance

## Internal Structure Recommendation
Do not expose a large public abstraction immediately.

Internally, use one small side-action descriptor for these controls, such as:
- stable id
- icon
- side
- size
- gap
- visibility
- enabled state
- render mode
- ink override
- tooltip
- callback

Each control should own:
- its action vector
- its per-item action hit rects
- hover/pressed state for actions

## Behavior Rules
- clicking an item action should trigger only the action callback
- it should not automatically trigger row selection or normal item activation
- hover/pressed state for the action should be independent of row selection

## Non-Goals
- arbitrary embedded child controls per row/header
- full action menus in the first pass
- model-level arbitrary action arrays as the initial design
- multi-line action labels
- separate one-off APIs for every side

## Implementation Checklist
1. Add a shared `UiItemActionHandle` public type or equivalent control-local
   handle with the same API contract.
2. Add `AddItemAction(...)` to:
   - `UiAccordion`
   - `UiList`
   - `UiTree`
   - `UiDropdown`
3. Add control-level default sizing/gap knobs where needed:
   - `SetItemActionSize(int px)`
   - `SetItemActionGap(int px)`
4. Implement hit-testing and paint for item actions in each target control.
5. Preserve structural-affordance precedence in each control.
6. Ensure row/header content layout consumes only the remaining span.
7. Keep default action icon rendering on `PreserveColor`.
8. Add demo coverage only after the control API is real.
9. Do not add demo-side hacks to simulate the feature before the controls own it.

## Required Cleanup When Implementing
When the API is actually added, the implementation pass must also do this:

1. Update header changelog blocks in:
   - `Ui/UiAccordion.h`
   - `Ui/UiList.h`
   - `Ui/UiTree.h`
   - `Ui/UiDropdown.h`
2. Add brief contract comments above the new public API in those headers.
3. Update any relevant guide/changelog docs if the implementation changes
   rollout status or usage guidance.
4. Rebuild the active builder demos that exercise affected controls.

## Recommended Rollout Order
1. `UiList`
2. `UiAccordion`
3. `UiTree`
4. `UiDropdown`

Reason:
- `UiList` is the cleanest proving ground
- `UiAccordion` is conceptually straightforward after that
- `UiTree` and `UiDropdown` have more built-in row affordance logic and should
  follow once the shared behavior is validated

## Acceptance Standard
The feature is acceptable only if:
- the API remains small and explicit
- control-owned structural affordances keep precedence
- row/header layout remains deterministic
- no demo hacks are required to prove the concept
- the implementation reads as a natural extension of the current controls, not
  as a parallel mini-framework

---


# Imported Source: archive/UiControlBase_refactor_plan.md

# Ui Control Base Refactor Plan

## Purpose

This plan defines the path toward a stable shared-control architecture built around:

- a final `UiButtonBase`
- a final `UiIndicatorBase`
- shared indicator paint helpers
- shared layout usage built on the existing `UiBlocksLayout` helpers

The goal is not to force all controls into one inheritance tree.

The goal is to identify the real common behavior, extract it in safe phases, and keep:

- style freedom
- clean APIs
- stable behavior
- deterministic paint paths
- low-risk incremental refactors

---

## Architectural direction

### What should stay separate

These control families are not the same and should not be flattened:

- push-style command controls
  - `UiButton`
  - `UiToolButton`
- indicator-style choice controls
  - `UiCheckBox`
  - `UiRadioButton`
- track/thumb controls
  - `UiToggle`
- menu/list/dropdown row renderers
  - `UiMenu`
  - `UiList`
  - `UiDropdown`

### What should become shared

The shared layers should be:

1. `UiButtonBase`
   - push/click/press/focus behavior for button-like controls only

2. `UiIndicatorBase`
   - hover/pressed/focus/data/layout behavior for indicator-bearing controls only

3. indicator paint helpers
   - shared check/radio/indeterminate rendering
   - shared buffered small-glyph rendering rules

4. shared layout helpers
   - reuse the existing `UiBlocksLayout` system
   - extend with small wrappers where needed, but do not replace it

---

## Core rules

### Rule 1: shared painters are more important than shared inheritance

If a rendering rule is needed by multiple control families, it belongs in a helper layer first.

Do not create a base class just to share a few draw functions.

### Rule 2: base classes should only exist for real state/interaction overlap

A new base is justified only when it removes meaningful duplicated logic such as:

- hover/pressed/focus state management
- keyboard activation behavior
- data binding and action commit rules
- layout cache invalidation

### Rule 3: layout should continue to use `UiBlocksLayout`

Existing helpers in `UiStyle.h` already cover:

- support vs main content
- gaps and margins
- styled inner/content rect
- min-size stability

We should build on these:

- `UiStyledInnerRect`
- `UiStyledOuterSizeFromContent`
- `UiMeasureBlocksContent`
- `UiComputeBlocksLayout`
- `UiBlock`
- `UiBlocksLayout`

### Rule 4: small antialiased indicator marks must use buffered composition

The `UiMenu` submenu regression showed that tiny antialiased primitives can behave differently on popup surfaces.

Project rule:

- radio dots and similar tiny marks should be rendered through small buffered alpha composition helpers
- these helpers should be reused by menus and indicator controls

---

## Current codebase observations

### Already good

- `UiButton` already acts like a behavior base for `UiToolButton`
- `UiCheckBox` and `UiRadioButton` already use very similar layout/cache/state patterns
- `UiBlocksLayout` is already working well for button and indicator layouts

### Current duplication

- indicator mark painting logic is duplicated
- indicator control focus/hover/press handling is duplicated
- indicator control layout cache structure is duplicated
- menu/list-style indicator rendering is not yet using the same indicator paint seam

### Current non-goals

Do not do these in the first refactor pass:

- do not fold `UiToggle` into `UiIndicatorBase`
- do not make `UiMenu` inherit from a button or indicator base
- do not redesign theme roles broadly before shared painters/layout are stabilized
- do not rewrite working controls wholesale

---

## Refactor phases

## Phase 1: shared indicator paint helpers

### Goal

Extract indicator drawing into shared helpers with no semantic control changes.

### Scope

- shared radio dot painter
- shared check mark painter
- shared indeterminate mark painter
- shared centered glyph/image placement helpers where useful
- buffered composition for tiny antialiased marks

### Consumers for first migration

- `UiCheckBox`
- `UiRadioButton`
- `UiMenu`

### Deliverables

- helper functions in `UiDraw.h` or a small internal helper header used by `UiDraw`
- `UiCheckBox` switched to helpers
- `UiRadioButton` switched to helpers
- `UiMenu` switched to helpers for check/radio marks

### Tests

- dedicated indicator paint regression test binary
- verify helper output is non-empty and colored as expected
- include a checked-radio path
- keep `UiMenuRunTests` and `UiMenuDemo` working

### Completion checklist

- [x] radio dot painter extracted
- [x] check mark painter extracted
- [x] indeterminate mark painter extracted
- [x] `UiCheckBox` migrated
- [x] `UiRadioButton` migrated
- [x] `UiMenu` migrated
- [x] regression test added
- [x] demo/tests rebuilt and run

---

## Phase 2: shared indicator layout helpers

### Goal

Reduce duplicated indicator/text layout code without changing public control APIs.

### Scope

- build thin wrappers over `UiBlocksLayout`
- shared support/main block helpers for:
  - indicator + text
  - optional indicator side
  - common gap handling
- keep actual control style structs separate

### Deliverables

- shared internal helpers for indicator-bearing controls
- `UiCheckBox` and `UiRadioButton` layout paths simplified
- menu/list row code can optionally reuse indicator geometry helpers

### Completion checklist

- [x] indicator/text layout wrapper added
- [x] `UiCheckBox` layout path simplified
- [x] `UiRadioButton` layout path simplified
- [x] no public API regression
- [x] tests still pass

---

## Phase 3: introduce `UiIndicatorBase`

### Goal

Only after phases 1 and 2 prove real overlap, extract a small shared base for indicator controls.

### Intended scope

- hover state
- pressed state
- focus state
- action firing seam
- shared text-size/layout-cache invalidation
- `SetData`/`GetData` support hooks where appropriate

### Intended consumers

- `UiCheckBox`
- `UiRadioButton`

### Explicit exclusions

- `UiToggle`
- `UiMenu`
- `UiButton`
- `UiToolButton`

### Completion checklist

- [x] duplicated state/interaction logic measured and justified
- [x] `UiIndicatorBase` introduced
- [x] checkbox migrated
- [x] radio migrated
- [x] no styling seam lost
- [x] tests expanded and passing

### Phase 3 notes

- `UiIndicatorBase` intentionally remains narrow:
  - text storage/caching
  - hover/press/focus refresh behavior
  - indicator block layout caching
  - min-size plumbing
  - activation-key helpers
- style ownership remains in `UiCheckBox` and `UiRadioButton`
- the new base regression tests caught a real cache-key issue:
  - layout cache cannot depend only on the content rect
  - it must also key on support geometry, side/alignment, gap, and min-support inputs
- this confirms the value of extracting the base only after adding direct tests for it
- `UiToggle` was reviewed after the extraction and remains outside this base:
  - it is a track/thumb control, not an indicator-mark control
  - its geometry, animation, and paint pipeline are materially different
  - it can reuse small helper seams later, but not the current `UiIndicatorBase`

---

## Phase 4: evaluate `UiButtonBase`

### Goal

Formalize `UiButton`’s existing base-like role only if there is a second real consumer beyond `UiToolButton` that benefits from the same internal behavior.

### Rule

Do not split `UiButtonBase` prematurely.

`UiButton` already works as a stable behavior host. Extract a named base only if:

- there is proven duplication
- the split improves clarity
- the split does not destabilize button styling

### Completion checklist

- [ ] duplication justified
- [ ] base extraction improves clarity
- [ ] `UiToolButton` still chains cleanly
- [ ] tests expanded and passing

### Current assessment

- inspected `UiButton` / `UiToolButton` after phase 3
- current conclusion:
  - `UiButton` already is the effective behavior base
  - `UiToolButton` is already the thin specialization we want
  - a separate named `UiButtonBase` would currently be a mechanical split, not a clarity win
- phase 4 therefore remains gated on a future second real consumer or a clearer button-family duplication case

---

## Test strategy

### Paint regressions

- offscreen helper rendering tests
- checked radio mark
- check mark
- indeterminate mark
- different colors and sizes

### Control regressions

- checkbox click and keyboard toggle
- radio exclusive behavior
- toggle key/mouse/data behavior
- disabled choice controls ignore activation input
- programmatic `SetData` does not emit user-action events
- menu checked-radio first-row submenu case
- demo smoke builds for affected demos
- dedicated `UiChoiceRunTests` harness for checkbox/radio/toggle functional checks

### Stability checks

- no public API changes unless intentional
- no theme regression in default presets
- no layout regression in `UiBlocksLayout` consumers
- keep behavior regressions ahead of new abstraction work

---

## Final completion gate

The refactor is only considered complete when:

- [ ] indicator rendering is shared and stable
- [ ] indicator controls share real behavior without losing style flexibility
- [ ] button controls share real behavior without overreaching into other families
- [ ] menu/list/dropdown rows can reuse indicator painters without inheritance coupling
- [ ] layout reuse is centered on the existing `UiBlocksLayout` system
- [ ] demos and regression tests clearly cover the shared seams
