# Ui Version 1 Control Audit and Remediation Plan

This is the master release-readiness audit for the `Ui*` control library.

It is a planning document, not one implementation task. Divide each workstream into focused assignments containing one to three closely related changes, exact files/functions, and a clear test gate.

## Executive assessment

The library has a credible shared foundation rather than a collection of unrelated controls:

- shared `StyledPalette`, `StyledMetrics`, and `StyledSkin` structures;
- central drawing, measurement, theme, model, layout, and icon infrastructure;
- broad runtime control and demo coverage;
- a Designer that increasingly constructs real runtime controls.

The primary Version 1 risk is inconsistency at subsystem boundaries rather than general code quality:

- API synonyms or different names for the same concept;
- container/content semantics that differ between runtime, preview, and code generation;
- animation and callback lifetime;
- serialized format stability;
- old documentation that describes a different style model;
- incomplete Designer inventory or inspector properties that have no effect.

## Release priorities

| Priority | Meaning | Version 1 rule |
|---|---|---|
| P0 | Control-tree corruption, lifetime use-after-free, data loss, generated-code failure, unsafe persistence | Fix immediately |
| P1 | API contradiction, runtime/Designer mismatch, animation leak, invalid geometry, serious documentation mismatch | Fix before release candidate |
| P2 | Redundant helper, naming rough edge, minor inefficiency, incomplete example | Fix where low risk; otherwise document |
| P3 | Optional enhancement or aesthetic refinement | Not a Version 1 blocker |

## Canonical review checklist for every control

- Public header states purpose, ownership, thread context, usage, and callback semantics.
- Public API uses the canonical project vocabulary from `00_Ui_V1_Engineering_Contract.md`.
- No dead, duplicate, speculative, or accidentally public helper remains.
- Constructor creates a valid control tree with no cycles, duplicate parentage, or hidden-overload traps.
- Destructor/removal paths cancel timers, animations, and delayed callbacks.
- `Paint()` is side-effect free, balances clips, handles tiny/empty geometry, and avoids unnecessary allocation.
- `Layout()`, `GetMinSize()`, `GetContentSize()`, and width-aware measurement agree.
- DPI scaling occurs exactly once.
- Disabled, hidden, empty, focus, hover, pressed, and keyboard states are deterministic.
- Theme role and custom-style paths share the same geometry contract.
- `StyleDefault()` is never mutated.
- Setters are chainable consistently and return the concrete type where appropriate.
- Programmatic setter callback behavior is documented and tested.
- Serialization has safe defaults, invalid-value handling, and an explicit compatibility policy.
- Demo covers normal, edge, disabled, theme, resizing, and destruction behavior.
- Designer preview, save/reload, and generated code match runtime behavior.
- Hard-coded colour, size, timing, or text values are justified by a named default/style field.

## Known findings requiring attention

### P0/P1 findings

1. **Container geometry split**
   
   `UiPanel` / `UiScrollPanel` styled margins and direct-child placement have previously represented different geometry. Preview and generated code must use one resolver.

2. **Group Panel content semantics**
   
   `UiGroupPanel` is a single-content host and fills its content control. Fit/alignment for a directly placed leaf control requires a deliberate internal host rather than applying positions that `UiGroupPanel::Layout()` overwrites.

3. **Control-tree safety**
   
   `UiQuadSplitter` previously encountered an ownership cycle through a hidden `Add` overload. Composite/container constructors need direct-construction and adapter-factory regression tests.

4. **Context-free Designer properties**
   
   Layout properties must be shown only where the parent placement strategy can apply them.

5. **Serialization policy**
   
   Style, model, document, and Designer formats need a clear Version 1 compatibility statement and invalid-load tests.

6. **Animation lifecycle**
   
   Toggle, slider, mask feedback, progress, and other animated controls need common stop/hide/remove/destruction behavior.

7. **Documentation authority**
   
   Imported Chameleon pointer-style examples and generic naming guidance must not override the current control-owned style architecture.

### P2 findings

- Audit `SetStyle` versus `SetCustomStyle` and retain one canonical operation.
- Audit `SetSizeMin` versus `SetMinSize`, `SetValue` versus `Set`/`Get`, and `ShowX` versus `EnableX`.
- Review mixed scoped/unscoped enum conventions before freezing Version 1.
- Consider moving non-trivial inline image/blur implementation out of `UiStyle.h` to reduce public-header weight.
- Automate comparison of stable runtime controls against Designer registrations.

## Workstream 1 — Public API and style vocabulary

### Objective

Freeze the Version 1 naming and behavior table before broad cleanup.

### Required work

- Inventory every public `Ui/*.h` API.
- Group shared concepts: text/title, data/value, range, direction, alignment, spacing, style, selection, and events.
- Classify each method as canonical, domain-specific, redundant alias, misleading, or internal leakage.
- Choose enum conventions and migration candidates.
- Identify public helpers that can become private.
- Update docs and demos with each accepted rename.

### Acceptance gate

- Every public control appears in the inventory.
- Proposed renames include Designer and codegen impact.
- No alias is retained solely because experimental code used it.

## Workstream 2 — Core style, draw, theme, and measurement

### Objective

Prove that all controls share one safe geometry and style foundation.

### Scope

`UiStyle`, `UiDraw`, `UiTheme`, `UiMeasure`, indicator support, icons, and model primitives.

### Required work

- Audit frame, skin inset, content margin, content rectangle, and DPI behavior.
- Remove or consolidate overlapping geometry/drawing helpers.
- Review expensive inline public-header helpers.
- Verify theme revision and custom-style ownership.
- Validate enum serialization and missing-field behavior.
- Add empty/tiny/high-DPI geometry tests.

## Workstream 3 — Basic controls and indicators

### Scope

Label, Button, ToolButton, SplitButton, CheckBox, RadioButton, Toggle, Slider, ProgressBar, and shared indicator support.

### Required work

- Compare state, icon, text, alignment, role, and style APIs side by side.
- Standardize keyboard, focus, disabled behavior, and callback ordering.
- Audit animation ownership and interruption.
- Validate minimum sizes and narrow geometry.
- Remove duplicated indicator/button-family helpers.

## Workstream 4 — Editor family

### Scope

BaseEdit, LineEdit, IntEdit, FloatEdit, PasswordEdit, MaskEdit, MultiEdit, and SliderEdit.

### Required work

- Separate inherited and specialized API.
- Audit selection, clipboard, undo/redo, caret, placeholder, and flank ownership.
- Define validation/clamping/rejection semantics.
- Guard formatter and validator re-entrancy.
- Verify PasswordEdit display paths do not expose plain text unexpectedly.
- Validate malformed MaskEdit masks safely.

## Workstream 5 — Containers, layouts, and child ownership

### Scope

Panel, GroupPanel, ScrollPanel, Stack, Accordion, Tab, BoxLayout, GridLayout, Splitter, and QuadSplitter.

### Required work

- Classify each container type.
- Define direct-child capacity and second-child behavior.
- Unify Fit/Fixed/Expand and alignment semantics.
- Use one direct-content rectangle resolver in preview and codegen.
- Add ownership-cycle, duplicate-parent, empty-slot, remove, and reinsert tests.
- Ensure measurement and generated construction agree.

## Workstream 6 — Data views and navigation

### Scope

List, Tree, Table, Dropdown, Breadcrumbs, and Menu.

### Required work

- Define internal/external model ownership.
- Validate selection after insert, remove, clear, sort, and model replacement.
- Test empty, large, and invalid-index behavior.
- Normalize selected/current/active terminology only where concepts match.
- Audit popup closure, capture, focus restoration, and keyboard navigation.
- Detect obvious quadratic rebuild paths.

## Workstream 7 — Advanced controls and composites

### Scope

UiDoc, ColorPicker, Bezier controls, SliderEdit, and `Ui/Composites`.

### Required work

- Separate model, rendering, commands, and composition responsibilities.
- Audit callback re-entrancy and transaction boundaries.
- Review resource ownership and embedded controls.
- Measure paint/layout hot paths before optimizing.
- Ensure composites delegate rather than clone child behavior.
- Limit Designer exposure to stable, serializable Version 1 properties.

## Workstream 8 — Designer completeness and parity

### Required work

- Compare `Ui.upp` / `Ui.h` inventory with Designer registrations.
- Map every inspector property to a real runtime API/style field.
- Disable inapplicable layout properties contextually.
- Add round-trip fixtures for every control using non-default values.
- Compile generated code from a complete control gallery.
- Avoid preview-only simulations except explicitly marked Designer-only behavior.

### Acceptance gate

- No stable runtime control is accidentally absent.
- No editable property silently does nothing.
- Preview, save/reload, and generated code agree.

## Workstream 9 — Defensive and malicious-code review

### Required work

- Search raw allocation/deallocation, unsafe captures/casts, shell/process calls, file writes, and hidden network access.
- Review parsing, masks, document resources, serialization, and generated code for unbounded or injectable input.
- Check integer overflow and allocation limits for images, tables, documents, and layout dimensions.
- Remove stale experimental diagnostics and debug backdoors.
- Run available compiler warnings/static analysis.

Classify every finding as exploitability, crash, data loss, denial of service, or hygiene.

## Workstream 10 — Documentation, demos, and release closure

### Required work

- Remove or label conflicting historical guidance.
- Ensure every stable control has a concise example and focused demo.
- Document intentional differences from original U++ controls.
- Update control inventory and Designer coverage.
- Freeze version, U++ baseline, persistence statement, and known limitations.
- Run clean builds for library, demos, tests, Designer, and generated gallery.

## Recommended execution order

1. Vocabulary and core foundation.
2. Containers/layouts, because their contracts affect many controls and Designer code.
3. Basic controls and editors.
4. Data views and advanced controls.
5. Designer parity.
6. Defensive review and release closure.

## Task sizing rule

Do not assign a whole workstream as one coding task.

A normal implementation task should contain:

- one narrow objective;
- one subsystem;
- one to three closely related changes;
- exact files/functions;
- a short manual and automated acceptance gate;
- a report before the next task set.

Stability and ownership defects come first, then API/behavior corrections, then Designer/codegen, then documentation.
