# Theme Migration Checklist (U++ Ui)

This is the active execution checklist for migrating the `Ui*` control set to the
new theme-driven styling model.

This replaces the previous preset-heavy direction.

## 2026-03 Public API Cleanup

Current canonical public names and contracts after the release-hardening pass:

- `UiList` and `UiTree` now expose selection through `SetData/GetData`.
  - Single selection uses one scalar `Value` token.
  - Multi selection uses `ValueArray`.
  - Token resolution prefers model `data`; fallback is row index for `UiList` and node id for `UiTree`.
- Selection events use `WhenSelection`.
- `UiBaseEdit` naming is normalized to `SetOverwriteMode()`, `IsOverwriteMode()`, `AcceptsNewlines()`, `AcceptsTabs()`, and `AcceptsDrop()`.
- `UiTab` uses `SetActiveTab()` / `GetActiveTab()`.
- `UiDropdown` external binding is explicit with `SetModel(UiListModel&)`; `UseInternalModel()` switches back to the owned model and `GetInternalModel()` exposes it.
- `UiIntEdit` / `UiFloatEdit` keep only the canonical numeric vocabulary (`Min`, `Max`, `MinMax`, `Step`, `Precision`, `NotNull`).
- `UiAccordion` section accessors are `GetSectionContent()`, `GetSectionHeader()`, and `GetSectionBody()`.
- `UiToolButton` remains a distinct public control with its own theme/default-style path, but now shares the button interaction/paint/layout implementation through `UiButton`.

Verification completed for this cleanup slice:

- `examples/UiButtonDemo`
- `examples/UiLabelDemo`
- `examples/UiListDemo`
- `examples/UiTreeDemo`
- `examples/UiTreeRunTests`
- `examples/UiDropdownDemo`
- `examples/UiIntFloatDemo`
- `examples/UiTabDemo`
- `examples/UiSliderDemo`
- `examples/UiDocModelTest`
## Core Policy

- This is a new UI system. We are not preserving old preset families for compatibility.
- Every visual control must have exactly one hand-authored `StyleDefault()`.
- `StyleDefault()` must be rebuilt intentionally to match the `Minimal` design direction.
- The HTML mockup is the visual source of truth for theme intent.
- Controls should be theme-aware by default.
- `SetStyle(const Style&)` remains the explicit local override path.
- If a control cannot express the intended design cleanly, flag the API gap before forcing a weak approximation.
- Demos are tests. After adjusting a control, compile its demo before moving to the next control.
- Layout classes are not primary theme surfaces unless they visibly paint something.

## Design References

Primary visual references:

- `E:\web\github\web_ui_theams\DesignSystemsTheams.html`
- `E:\web\github\web_ui_theams\theme-presets\minimal.js`
- `E:\web\github\web_ui_theams\theme-presets\rounded.js`
- `E:\web\github\web_ui_theams\theme-presets\linear.js`
- `E:\web\github\web_ui_theams\theme-presets\solid.js`
- `E:\web\github\web_ui_theams\theme-presets\outline.js`
- `E:\web\github\web_ui_theams\theme-presets\compact.js`
- `E:\web\github\web_ui_theams\theme-presets\layered.js`

Initial implementation target:

- First theme default source: `Minimal`
- Light and dark must both be tuned explicitly
- First implementation slice after control cleanup: `Minimal` and `Rounded`

## Control Style Policy

Every visual control should converge on this model where applicable:

- `StyleDefault()`
- `SetStyle(const Style&)`
- `GetStyle() const`
- `OnStyleChanged()`
- theme-driven effective style path
- explicit local override
- semantic role only when the control family actually benefits from roles
- control-specific visual mode only when the control has real structural variants

What should be removed over time:

- extra built-in family presets such as `StyleMinimal()`, `StyleSoft()`, `StyleStrong()` when they are only acting as local theme systems
- unusual or inconsistent style naming that breaks API familiarity across controls

## Per-Control Workflow

Do this in order for each control:

1. Inspect the control style API, paint path, layout path, and demo.
2. Rebuild `StyleDefault()` from scratch using the HTML mockup as the guide.
3. Remove superfluous built-in style presets from the control.
4. Normalize naming so the style API feels familiar across controls.
5. Add theme-aware resolution so the control uses `UiTheme` by default.
6. Preserve explicit local `SetStyle(...)` override behavior.
7. Update the control demo so it still makes sense as a test and showcase.
8. Compile the demo.
9. If the demo compiles, move to the next control.
10. If the control cannot match the design cleanly, log the API gap and stop that control slice before papering over it.

## Compile Testing

U++ root:

- `E:\upp-18468`

General example command:

```bat
"E:\upp-18468\umk.exe" "E:\apps\github\upp_Ui,E:\upp-18468\uppsrc" examples/UiThemeDemo CLANGx64 -br "E:\apps\github\upp_Ui\build\UiThemeDemo"
```

Control demo compile rule:

- Once a control is adjusted, compile its demo immediately.
- Do not move to the next control until the current control demo compiles.
- Once the entire control list is complete, begin the dedicated `UiThemeDemo`.

## Familiarity / Consistency Gate

For every control update, verify:

- The default look reads as part of the same minimalist system.
- Light and dark behavior are both considered.
- The style API uses familiar naming and concepts.
- Similar concepts are named similarly across controls.
- The control does not expose a strange one-off style vocabulary unless genuinely necessary.
- Behavior and appearance remain properly separated.

## API Gap Rule

Flag the control if any of the following happens:

- The mockup requires a visual treatment the control cannot express cleanly.
- The control needs new one-off public knobs just to imitate the theme.
- The control's paint/layout model fights the intended design direction.
- A composite control hardcodes child styling in a way that prevents theme consistency.

When flagged:

- note the specific gap
- do not hide it with ad hoc styling hacks
- resolve the API/design issue before continuing that control

## Execution Order

Primary order:

1. `UiButton`
2. `UiLabel`
3. `UiPanel`
4. `UiBaseEdit`
5. `UiLineEdit`
6. `UiPasswordEdit`
7. `UiMaskEdit`
8. `UiMultiEdit`
9. `UiIntEdit / UiFloatEdit`
10. `UiCheckBox`
11. `UiRadioButton`
12. `UiToggle`
13. `UiSlider`
14. `UiScrollBar`
15. `UiDropdown`
16. `UiTab`
17. `UiTitleCard`
18. `UiAccordion`
19. `UiScrollPanel`

Secondary / mostly non-theme surface:

- `UiBoxLayout`
- `UiGridLayout`
- `UiDataModels`
- `UiDoc`

These should only be touched for theme work if they visibly paint theme surfaces or block the migration.

## Control Queue

Legend:

- `Default`: new `StyleDefault()` authored from scratch against the mockup
- `Cleanup`: extra presets removed / API normalized
- `Theme`: theme-aware default path added
- `Demo`: demo updated to remain a useful control test
- `Compile`: demo compile verified
- `Gap`: API gap found that blocks clean design implementation

| Control | Demo | Default | Cleanup | Theme | Demo | Compile | Gap | Notes |
|---|---|---|---|---|---|---|---|---|
| UiButton | `examples/UiButtonDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; theme-driven default, demo verified |
| UiLabel | `examples/UiLabelDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; theme-driven default, demo verified |
| UiPanel | `examples/UiPanelDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; surface default rebuilt, accordion dependency updated |
| UiBaseEdit | `examples/UiBaseEditDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; theme-driven edit base verified |
| UiLineEdit | `examples/UiLineEditDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | verified through inherited edit theme path + demo compile |
| UiPasswordEdit | `examples/UiPasswordEditDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | verified; eye-button styling now follows theme button roles |
| UiMaskEdit | `examples/UiMaskEditDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | verified through edit-base inheritance and demo compile |
| UiMultiEdit | `examples/UiMultiEditDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | verified; demo side buttons switched to theme button roles |
| UiIntEdit / UiFloatEdit | `examples/UiIntFloatDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | verified; numeric edits inherit themed base and updated demo action button |
| UiCheckBox | `examples/UiCheckBoxDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; theme-driven default, visual mode kept separate from theme |
| UiRadioButton | `examples/UiRadioButtonDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; theme-driven default, visual mode kept separate from theme |
| UiToggle | `examples/UiToggleDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; thin switch-wrapper over themed checkbox |
| UiSlider | `examples/UiSliderDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; themed default plus slider-edit demo verification |
| UiScrollBar | `examples/UiScrollBarDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; themed chrome default, popup dependency updated, demo verified |
| UiDropdown | `examples/UiDropdownDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; field default/theme path cleaned up and popup demo verified |
| UiTab | `examples/UiTabDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; theme-driven default with visual mode kept as structural axis |
| UiTitleCard | `examples/UiTitleCardDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; theme-driven default, resolver added, demo verified |
| UiAccordion | `examples/UiAccordionDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; theme-driven composite default, demo updated to resolver APIs |
| UiScrollPanel | `examples/UiScrollPanelDemo` | [x] | [x] | [x] | [x] | [x] | [ ] | complete; themed container default, demo verified |
| UiBoxLayout | `examples/UiBoxLayoutDemo` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | touch only if visible theme surface is needed |
| UiGridLayout | `examples/UiGridLayoutDemo` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | touch only if visible theme surface is needed |

## Theme Demo Phase

Do not start `UiThemeDemo` until the control queue above has been worked through.

When the control queue is complete:

- [x] create `examples/UiThemeDemo`
- use the HTML design system mockup as the structure guide
- add sections for each preset
- include a day/night switch
- show the controls in a coherent specimen layout
- [x] begin with `Minimal` and `Rounded`

## Operator Rule

Once this workflow starts:

- adjust one control
- compile its demo
- if clean, move to the next
- continue through the queue without needing user interaction
- stop only for a real blocker or when the control queue is complete and the `UiThemeDemo` phase begins











