# Ui Version 1 Engineering Contract

This document is the project-specific authority for the `Trilec/upp_Ui` control library as it approaches Version 1.

It overrides older imported notes and generic U++ examples wherever those sources conflict with the current `Ui*` architecture. The larger coding standards guide remains useful for U++ ownership, threading, package, drawing, serialization, and safety caveats, but it is not the authority for the public `Ui*` style API.

## 1. Release intent

The library began as experimental work, but Version 1 is not required to preserve experimental naming or accidental behavior.

Before Version 1:

- remove misleading or redundant public APIs rather than adding compatibility shims;
- prefer one clear contract over several historical variants;
- keep runtime controls, demos, Designer preview, serialization, and generated code aligned;
- treat crashes, lifetime faults, data loss, generated-code failures, and silent property no-ops as release blockers.

## 2. Canonical style architecture

New and updated `Ui*` controls should use the shared style vocabulary from `UiStyle.h`:

- `StyledPalette` for state-aware face, frame, ink, and icon values;
- `StyledMetrics` for geometry and visual metrics;
- `StyledSkin` for image-backed skin drawing and geometry insets;
- `UiTheme` role resolvers for semantic defaults;
- shared `UiDraw` and geometry helpers rather than local parallel systems.

The standard control-owned style lifecycle is:

```cpp
static const Style& StyleDefault();

const Style& GetEffectiveStyle() const;
UiControl& SetCustomStyle(const Style& style);
UiControl& ClearCustomStyle();
bool HasCustomStyle() const;
void OnStyleChanged();
```

A control may expose `StyledPaletteRef()`, `StyledMetricsRef()`, or `StyledSkinRef()` when that matches the existing family pattern.

Do not use a borrowed `const Style*` as the primary style state for new `Ui*` controls. Do not mutate `StyleDefault()`.

## 3. Theme and explicit overrides

Theme roles provide the default style. Explicit appearance overrides create or modify control-owned custom style state.

A theme revision change should update controls that are still theme-driven. It must not overwrite an explicit custom style.

Use semantic roles consistently, normally:

- Standard
- Subtle
- Accent
- Alert

Do not hard-code application-specific RGB values in paint paths. Default colours belong in `StyleDefault()` or `UiTheme` role construction.

## 4. Canonical geometry vocabulary

These concepts are not interchangeable:

- **frame**: painted frame around a surface;
- **skin content inset**: geometry reserved by the skin;
- **content margin**: space between the styled surface and its content;
- **container inset**: explicit host/layout space applied around children;
- **gap**: space between peer content blocks or layout items;
- **item spacing**: space between repeated owned items;
- **content gap**: primary spacing inside one item or control surface.

The shared geometry order is:

```text
outer rectangle
→ frame / shadow / skin geometry
→ content margin
→ content rectangle
```

A property must not mean style margin in one path and direct-child positioning in another. Preview and generated code must use the same geometry interpretation.

## 5. Containers and layouts

Every container must be classified explicitly:

- styled host;
- single-content host;
- page container;
- multi-child layout;
- fixed-pane container.

Current intended semantics:

- `UiPanel`: styled host; it does not perform flow layout;
- `UiGroupPanel`: titled single-content host;
- `UiScrollPanel`: bounded viewport with a scrollable content host;
- `UiStack`, `UiTab`, and accordion section bodies: page/body hosts;
- `UiBoxLayout`: ordered row/column flow layout;
- `UiGridLayout`: stable logical row/column placement;
- `UiSplitter` and `UiQuadSplitter`: fixed pane-count containers.

Use Box or Grid layouts when several children require automatic arrangement.

Single-content containers must reject, replace, or deliberately wrap a second direct content root. They must not silently overlap or discard content.

## 6. Sizing and alignment

Use one vocabulary:

- **Fit**: natural/minimum size on that axis;
- **Fixed**: exact requested size, subject to safety constraints;
- **Expand**: consume available parent-distributed space;
- **cell alignment**: position a non-expanded item inside allocated space;
- **cross alignment**: alignment on the non-flow axis of a box layout.

Alignment is not sizing. Centering a Fit item must not silently stretch it.

`GetMinSize()`, `GetContentSize()`, width-aware measurement, `Layout()`, Designer preview, and generated code must agree.

DPI conversion must occur exactly once.

## 7. Public API naming

Public control APIs follow established U++-style PascalCase naming:

- `SetX()` / `GetX()` for paired state;
- `WhenX` for public callbacks;
- `SetData()` / `GetData()` for controls that participate naturally in data binding;
- typed `SetValue()` / `GetValue()` or `Set()` / `Get()` only where that family has a clear established convention.

Use:

- `SetText()` / `GetText()` for primary visible text;
- `SetTitle()` / `GetTitle()` for titled containers;
- `SetSubTitle()` / `GetSubTitle()` consistently;
- one orientation/direction vocabulary per shared concept;
- shared `UiAlign`, `UiCrossAlign`, and `UiDirection` where suitable.

Do not accumulate aliases such as `SetStyle`, `SetCustomStyle`, `Style`, and `UseStyle` for the same operation. Version 1 should retain the clearest canonical form.

## 8. Events and state commitment

User callbacks fire only after public state is committed.

Inside a selection/change callback, callers must be able to read the new state immediately through the corresponding getter.

Document whether programmatic setters fire user-action callbacks. Do not let transient hover/highlight state masquerade as committed selection.

Callbacks run on the GUI thread unless explicitly documented otherwise.

## 9. Ownership and lifetime

Use deterministic ownership and RAII.

- parent controls own child `Ctrl` instances;
- avoid raw `new` / `delete` in ordinary control code;
- use `One<T>` for unique heap ownership;
- use `Ptr<>` guards for callbacks that might outlive the immediate call stack;
- stop timers and animations during hide/remove/destruction as appropriate;
- never create parent/child cycles or add one control to more than one parent.

Composite and container constructors require regression coverage for direct construction and destruction.

## 10. Paint and layout discipline

`Paint()` is render-only:

- no model mutation;
- no event emission;
- no timer startup;
- no expensive recurring allocation where a cache or shared helper is appropriate;
- balanced clipping and draw state;
- valid behavior for empty and tiny rectangles.

Geometry-affecting setters call `RefreshLayout()` and `Refresh()` as appropriate. Visual-only setters call `Refresh()`.

## 11. Animation discipline

Animated controls must:

- avoid blocking loops and `ProcessEvents()` inside the control;
- start only when needed;
- stop when determinate/inactive, hidden, removed, or destroyed;
- guard callback lifetime;
- avoid forcing full Designer model rebuilds for each animation frame.

## 12. Serialization

Any format intended to survive Version 1 requires an explicit compatibility policy.

- use safe defaults when fields are absent;
- validate enum values while loading;
- reject or clamp dangerous dimensions and counts;
- version persistent formats where future evolution is likely;
- distinguish temporary Designer JSON from promised runtime persistence formats.

## 13. Designer parity

The Designer is a release-quality parity check, not a separate simulated UI system.

For every stable runtime control:

- toolbox registration must be intentional;
- adapters must construct the real runtime control;
- inspector properties must map to real runtime APIs or style fields;
- inapplicable properties must be hidden or disabled with a reason;
- save/reload must preserve supported properties;
- generated code must compile and match preview behavior;
- preview-only behavior must be explicitly identified as Designer-only.

No editable inspector property may silently do nothing.

## 14. Documentation hierarchy

Read project guidance in this order:

1. `00_Ui_V1_Engineering_Contract.md` — current project authority;
2. `02_Ui_Controls_Guide.md` — detailed control architecture and review guidance;
3. `02_Upp_Coding_Standards.md` — broad U++ safety reference and imported historical material;
4. feature-specific design documents.

When documents conflict, this contract and current code win. Update the conflicting documentation rather than preserving ambiguity.
