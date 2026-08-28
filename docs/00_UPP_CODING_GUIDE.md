# 00 — U++ Coding Guide

Reusable U++ engineering practice for the `upp_Ui` library family. This guide is
the first thing to read before touching any control, demo, or package in this
repository, and it is intended to be copyable into another U++ project.

## Packages and `.upp` membership

- A `.upp` package is the build unit. A **library** package has no `main` and no
  `mainconfig`. An **EXE** package declares `GUI_APP_MAIN` / `CONSOLE_APP_MAIN`
  and `mainconfig "" = "GUI"` (or `"CONSOLE"`).
- The `uses` list names every package the code depends on. Missing uses are a
  build error in the same way a missing include is. Do not rely on transitive
  `uses` to satisfy direct source dependencies.
- Keep `file` lists in the same order as the headers/sources they declare, and
  keep them complete. A header that is included via `#include <Pkg/Header.h>`
  must exist at that package path.
- `plugin/png`, `plugin/jpg`, `plugin/bmp` are required for image file I/O in
  EXEs.
- One package, one responsibility. Do not fold unrelated controls into the `Ui`
  package just to avoid creating a sibling package.

## C++ source/header organisation

- Header: includes, `namespace Upp {`, public API, private members at the
  bottom, `} // namespace Upp`, include guard.
- Use the `_Package_Header_h_` include-guard convention (`_Ui_UiLabel_h_`).
- Keep `Paint()`, `Layout()`, and event handlers in the `.cpp`. Inline tiny
  trivial getters in the header only.
- A control `Ctrl` derived type declares `typedef T CLASSNAME;` so `THISBACK`
  bindings work.
- Document purpose, intent, thread context, and a short usage note in the
  header comment block before the code.

## U++ naming and style

- Public API follows U++ PascalCase: `SetX()` / `GetX()` paired state,
  `WhenX` public callbacks, `SetData()` / `GetData()` for data-binding controls,
  typed `SetValue()` / `GetValue()` only where the family convention exists.
- Use `SetText()`/`GetText()` for primary visible text, `SetTitle()`/`GetTitle()`
  for titled containers, `SetSubTitle()`/`GetSubTitle()` consistently.
- Shared vocabulary enums: `UiAlign`, `UiCrossAlign`, `UiDirection`,
  `UiIconRenderMode`. Do not accumulate aliases for the same operation.
- Use the shared style vocabulary (`StyledPalette`, `StyledMetrics`,
  `StyledSkin`, `UiTheme` role resolvers) rather than local parallel systems.

## Ctrl ownership and lifetime

- Parent controls own child `Ctrl` instances. Never add one control to more than
  one parent; never create parent/child cycles.
- Prefer value members and `One<T>` for unique heap ownership. Avoid raw
  `new`/`delete` in ordinary control code.
- Use `Ptr<>` guards for callbacks that might outlive the call stack.
- Stop timers and animations on hide/remove/destroy. Composite constructors need
  regression coverage for direct construction and destruction.
- Do **not** use arbitrary integers with `Ctrl::SetTimeCallback()` /
  `Ctrl::KillTimeCallback()`. Ctrl timer ids are internal byte-offset identifiers,
  not application-defined handles; large or invented ids can assert in Debug and
  can address invalid Ctrl state.
- Prefer an owned `TimeCallback` member for delayed or one-shot work. Use
  `KillSet()` when replacing a pending callback and `Kill()` during cancellation
  or teardown.
- Repeating custom frame clocks should own their timer state. In `Ui`, use
  `UiFrameTicker` for one-callback-at-a-time animation/frame stepping instead of
  repeatedly scheduling raw Ctrl timer ids. The shared `Animation` package is
  preferred where its interpolation/lifecycle model already fits the control.

## Callbacks and `When...` conventions

- User callbacks fire only after public state is committed. Inside a
  selection/change callback the caller must be able to read the new state
  through the corresponding getter.
- Document whether programmatic setters fire user-action callbacks.
- Callbacks run on the GUI thread unless documented otherwise.
- Bind with `THISBACK`, capture by value or with `Ptr<>`; never capture a
  possibly-dead `this` raw.
- Model-driven controls use the request-first pattern (`WhenXRequest`) when the
  application owns semantic state — see `03_UI_MODEL_GUIDE.md`.

## Value / ValueArray / ValueMap

- `Value` is the U++ variant for data-binding and property values; use it for
  `SetData()`/`GetData()`, property models, and control payloads where the type
  is not statically fixed.
- Validate enum values and ranges when loading persisted `Value` payloads.
- `ValueArray`/`ValueMap` are the serializable container forms used in models
  and JSON round-trips.

## U++ containers

- `Vector<T>` for arrays, `Array<T>` for objects with ownership, `VectorMap<K,V>`
  for ordered key/value maps, `Index<T>` for sets, `One<T>` for unique heap
  ownership. Use `pick()` to transfer ownership where appropriate.
- Prefer `Array` + `One` over raw owning pointers in control-owned member
  collections.

## Dependency direction

- Library packages depend only on other libraries, never on EXE packages.
- Headless/model packages must not `#include <Ui/...>` or `<CtrlLib/...>`.
  Only packages that legitimately build GUI may reference `Ui`/`CtrlLib`.
- Tests depend on production packages; production packages never depend on tests.

## DPI-aware GUI code

- Apply `DPI(...)` exactly once per value; never double-scale.
- Geometry setters call `RefreshLayout()` + `Refresh()`; visual-only setters call
  `Refresh()`.
- `GetMinSize()`, `GetContentSize()`, width-aware measurement, `Layout()`,
  preview, and generated code must agree on the same geometry vocabulary:
  frame, skin content inset, content margin, container inset, gap, item spacing,
  content gap.

## State ownership and avoiding duplicated authorities

- One authoritative source of state per concern. A view model is a projection,
  never the source of truth for application-owned data.
- Avoid "mutate silently, notify afterward" APIs. Use request-first mutation
  when a command/undo/validation layer exists.
- Do not let hover/pressed visuals masquerade as committed selection.

## Assertions and error handling

- Favor status-returning APIs for expected failures; log errors.
- Use `ASSERT` for programmer invariants in Debug builds; do not rely on it for
  runtime data validation.
- `Paint()` is render-only: no model mutation, no event emission, no timer
  startup, balanced clipping/draw state, valid for empty and tiny rectangles.

## Testing philosophy

- Control/test packages should protect current public behaviour and current
  regression coverage. Deterministic smoke tests verify exact output.
- Do not delete legitimate tests merely to reduce file count.
- Keep tests deterministic: no timestamps, no environment-dependent output in
  golden comparisons.

## Debug/Release expectations

- Debug builds include full assertions and checks; Release builds compile clean
  with optimizations and no asserts.
- Both configurations must compile; do not introduce Debug-only APIs that break
  Release.
- Generated/build output never lives in source directories; it goes to the
  assembly output folder (`build/`, `out/`) which is git-ignored.

## Clean, reviewable implementations

- Keep controls small; split helpers into sibling packages when they grow.
- Review style code for redundant default-setting calls; setting a field that
  does not change behaviour is bloat.
- When removing or replacing a public API, sweep demos and sibling controls for
  stale calls in the same pass.

## Reading order

1. `00_UPP_CODING_GUIDE.md` (this guide);
2. `01_UI_CONTROLS_GUIDE.md` — the control catalogue;
3. `02_UI_THEME_GUIDE.md` — theme and style system;
4. `03_UI_MODEL_GUIDE.md` — model-driven control architecture;
5. `04_UI_DEMO_GUIDE.md` — the demo structure;
6. `05_UI_PROPERTY_EDITOR_GUIDE.md` — PropertyEditor integration and extension.

When this guide and an older document conflict, current code and this guide win.
