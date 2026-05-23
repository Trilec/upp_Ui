# Ui Demo Guide

Primary guide for building and reviewing demos. Demos are treated as executable documentation and regression tests. This consolidates the normalized shell/template guidance, showcase intent, migration gates, and current demo expectations.


---


# Imported Source: archive/UiDemoTemplate_guide.md

# Ui Demo Template Guide

## Purpose

`UiDemoBase` is the baseline shell for Ui demo executables. It exists so each control demo feels related without forcing the center content to look identical.

## Current baseline

- Flat split-shell layout with a left workspace and right inspector rail
- Minimal header with icon, title, and subtitle instead of heavy card chrome
- Right-aligned version badge, theme segment, and exit utility action
- Large neutral preview canvas on the left for the real control surface
- Inspector sections on the right for `Usage`, `State`, and live `Properties`
- Light grey / blue visual language with a matching dark-mode pass

## Layout contract

Use this composition unless a demo has a strong reason to diverge:

- Top left: icon, title, and short subtitle
- Top right: version badge, theme segment, and utility action
- Left body: one large workspace / preview canvas
- Right body: `Usage`, `State`, and `Properties` inside one shared multi-open accordion
- `Properties` may contain nested accordions for grouped control areas like `Size`, `Frame`, `Face`, or `Shadow`

## Reuse rules

- Keep the shell familiar across demos
- Change the header title, subtitle, and inspector copy per control
- Keep the inspector rail concise and interaction-aware
- Use the left workspace for the real control, not decorative filler
- Prefer a neutral preview canvas over themed chrome inside the stage
- Use `PreviewGrid()` as the default host for demo controls in the left workspace; utility demos may split the preview area into a selector pane and a live preview pane
- Use `PropertyBox()` as the default host for live inspector controls in the right rail
- Utility-style demos are preferred when they provide real value beyond appearance alone
- A demo should ideally generate or expose copyable state/code that reproduces the current result
- Use `UiTitleCard`, `UiPanel`, `UiLabel`, `UiButton`, `UiSlider`, and `UiToggle` only where they add structure, not ornament

## Property Row Rule

All numeric demo properties should use the same row pattern:

- left: property name
- center: editor control, usually a slider
- right: current value

This row should be provided by the template/property-row helper, not invented separately in each demo.

### Layout

- Keep the label on the left so the property remains scannable in a stacked list
- Let the slider or editor take the flexible center width
- Keep the value right-aligned so rows line up cleanly
- Use one row per property; avoid a separate label row above the slider unless a control genuinely needs more explanation

### Value formatting

- Use integers for discrete values such as font size, radius, border width, and item count
- Use decimals only for genuinely continuous values such as opacity, scale, or shadow strength
- Add a suffix only when it helps readability, for example `px`, `%`, or `x`
- If the value is already obvious from the control context, a bare integer is preferred over noisy formatting

### Ownership

- The control itself should not invent its own label or value badge
- The demo shell/property-row helper owns the surrounding label/value presentation
- The control only owns the interactive widget and its actual state

## Inspector Composition Rule

Inspector content should be composed from measured controls:

- use `UiCompositeSlider`, `UiCompositeToggle`, `UiCompositeDropdown`, `UiCompositeColor`, or equivalent shared property rows
- composite row sources live under `Ui/Composites/`; prefer `<Ui/Ui.h>` in demos unless a narrow include is genuinely useful
- place repeated property rows in `UiBoxLayout`
- use `UiPanel` as a visual host only when child positions are already known
- use `UiScrollPanel` as the outer bounded viewer for a rail or document-like area
- if a scroll panel is embedded inside an accordion section as a viewport, set that section body height explicitly

Do not manually line up label/edit pairs in each demo. That breaks typography,
spacing, and future theme changes. The composite controls are the default
property inspector surface.

## Styling direction

- Soft, clean, modern surfaces
- Pill-shaped chrome for theme and utility actions
- Blue headline emphasis
- Blue slider thumbs in both themes for visibility and consistency
- Soft grey/blue framing rather than heavy borders
- Subtle gradient window background
- Resolve demo colors from one palette block near the top of the demo file
- Avoid repeating per-control light/dark `if` branches when palette lookup can do it once

## Checklist

- Header is visually light and does not dominate the demo
- Version is visible as a pill badge
- Theme switching is available from the shell
- Exit action is always easy to reach
- Right rail supports `Usage`, `State`, and `Properties` inside one shared accordion
- Nested property accordions should reuse the same soft-corner language with a lighter framed treatment so they read as secondary groups
- Right rail follows `Usage -> State -> Properties`
- Section headings (`Usage`, `State`, `Properties`) use the shared heavy label face: `Font().FaceName("Arial Black").Height(15)` when available, with fallback to the demo sans helper
- Numeric properties follow the standard property row: `label | editor | value`
- Left workspace remains the visual focus
- Layout scales cleanly from the baseline window size upward

## Changelog

- 2026-03: established the first shared demo-shell baseline in `UiDemoBase`
- 2026-04: aligned the template shell with `UiTitleCard::GetMinSize()` and clipped title-card paint to control bounds
- 2026-04: simplified the shell into a flatter split workspace with a right-side inspector and neutral preview canvas
- 2026-04: moved `UiDemoBase` shell styling to a single resolved `DemoPalette` so theme colors are defined once and reused throughout
- 2026-04: added reusable `PreviewGrid()` and `PropertyBox()` hosts so future demos can add controls without editing low-level inner layout coordinates
- 2026-04: normalized the inspector structure to `Usage`, `State`, and `Properties`, with a reusable `StateBox()` host for control-specific metadata
- 2026-04: added UiFontSelectorDemo as a live font-enumeration and preview utility using the shared demo shell pattern
- 2026-04: formalized the shared numeric property row pattern as `label | editor | value`, with integer formatting preferred for discrete properties like font size
- 2026-04: shifted the recommended demo approach toward utility-style builders/generators where the user can copy the current configuration, not just inspect a static showcase
- 2026-04: moved the inspector template toward multi-open accordions, with nested accordions available for grouped property areas

---


# Imported Source: archive/UiDemo_ShowcasePlan.md

# Ui Demo Showcase Plan

## Purpose

This guide defines what each Ui demo should show, how much variation each demo should contain, and which inspector patterns remain common across the whole demo family.

The goal is not to make every demo look identical.

The goal is:

- one familiar shell
- one familiar inspector rhythm
- strong control-specific showcases
- no duplicate showcase tiles that do not teach anything new

## Shared shell contract

All demos should use the `UiDemoBase` shell and follow the same broad structure:

- left: main preview/work area
- right: inspector rail
- header: icon, title, subtitle, version, theme switch, exit

The preview area should hold the real showcase content.
The inspector should explain what is selected, what state it is in, and what properties are being changed.

## Shared inspector contract

Every demo should follow the same inspector order.

### 1. Usage

Short, practical explanation of:

- what the control is for
- what the current showcase is demonstrating
- what the user should try

When useful, include a compact copyable code sample for the selected showcase.

### 2. State

Always show current live state near the top.

Common fields where applicable:

- enabled / disabled
- focused
- hovered
- selected item / row / cell / tab
- checked / on / open / active
- value / text
- current theme preset
- current visual preset name

Control-specific fields should appear below the common fields.

Examples:

- button: pressed count, last action
- toggle: on/off state
- list/tree: selected id, hovered id, drag source/target
- dropdown: selected key/value
- table: active cell, selection range, edit cell
- menu: open path, armed item, last command
- document: selection length, paragraph style, current inline style

### 3. Common properties

These should be reused across as many demos as practical.
Do not force a property where the control does not support it cleanly.

Recommended common property group:

- scale
- enabled
- pill / square / brutal visual preset
- frame on/off
- background on/off
- gradient on/off
- shadow on/off
- accent color choice
- frame thickness

Optional common properties for controls that support them:

- icon choice
- font size
- dense / normal spacing
- animation on/off

### Property row standard

Numeric properties should use one consistent row shape across demos:

- left: property label
- center: slider or other editor
- right: current value

Formatting rules:

- discrete values: integer, optionally with a small suffix like `px`
- continuous values: decimal only when needed
- right-align the value so stacked property rows stay visually stable

The demo shell owns this row treatment. Individual controls should not be responsible for inventing their own surrounding label/value chrome.

### 4. Control-specific properties

These are the interesting knobs unique to the control.

Examples:

- accordion: single-open, drag reorder, animation speed
- list: icon mode, checkbox mode, right-text alignment, editable rows
- tree: drag/drop, connector lines, rename, lazy children
- menu: context vs menu-bar mode, submenu arrows, check/radio items
- table: row headers, column resize, selection mode, alternating rows

## Preview layout rules

The preview area should be intentionally sized per demo.

Use these general limits:

- 1 large showcase when the control is inherently document-like or editor-like
- 2 columns when the control is structurally complex and needs room
- 3 to 6 tiles when the control is small and style-oriented

Do not add another tile unless it clearly demonstrates a different capability.

## Recommended showcase counts by control

### Single-workspace demos

Use one dominant preview only:

- `UiDoc`
- `UiTable`
- `UiMultiEdit` when used as editor-focused showcase
- `UiBaseEdit` if converted into a â€œform editingâ€ showcase

`UiDoc` is the main exception: it should feel like a compact word processor, not a tiled component gallery.

### Two-panel demos

Use two larger previews when the control needs space and interaction depth:

- `UiTree`
- `UiMenu`
- `UiDropdown`
- `UiTab`
- `UiScrollPanel`
- `UiScrollBar`

### Four-tile demos

Use a quad when the control has 4 distinct visual/behavior variants:

- `UiAccordion`
- `UiList`
- `UiPanel`
- `UiSlider`
- `UiTitleCard`

### Three-to-six tile demos

Use more smaller tiles when the control is compact and style-driven:

- `UiButton`
- `UiToolButton`
- `UiCheckBox`
- `UiRadioButton`
- `UiToggle`
- `UiLabel`

## Control-specific showcase intent

### UiButton / UiToolButton

Show:

- subtle / accent / brutal
- icon-only, text-only, icon+text
- gradient vs flat
- dense vs roomy
- hover/press/disabled
- optional pulse or accent animation on one sample only

Useful inspector properties:

- icon pick
- radius
- frame thickness
- gradient on/off
- shadow on/off
- animation on/off

### UiCheckBox / UiRadioButton

Show:

- standard form usage
- list-style usage
- icon + label usage
- different alignment / indicator side
- compact vs roomy sizing
- disabled / mixed state where supported

Useful inspector properties:

- indicator side
- indicator size
- font size
- checked/mixed state
- icon pick

### UiToggle

Show:

- compact toggle
- larger pill toggle
- label-left / label-right usage
- accent color variants
- animated state change

Useful inspector properties:

- on/off
- track thickness
- thumb size
- animation on/off
- accent color

### UiLabel

Show:

- title / subtitle / body / caption / badge
- icon + text
- alignment
- wrapping
- muted vs accent emphasis

Useful inspector properties:

- font size
- wrap on/off
- align
- icon pick

### UiPanel

Show:

- flat
- framed
- gradient
- brutal
- shadowed
- animated color panel

Useful inspector properties:

- frame on/off
- background on/off
- gradient on/off
- radius
- shadow on/off
- accent / frame color

### UiAccordion

Show:

- compact settings accordion
- richer content accordion
- drag reorder
- single-open policy

Useful inspector properties:

- single-open
- animation speed
- drag reorder
- header icon on/off
- divider thickness

### UiList

Show:

- icon rows
- checkbox rows
- right-text alignment
- editable rows
- drag reorder if supported
- underline / pill / square / brutal themes

Useful inspector properties:

- icon set
- checkbox on/off
- editable on/off
- right-text alignment
- row density
- drag reorder

### UiTree

Show:

- simple tree
- styled tree
- drag/drop tree
- lazy-load or rename example

Useful inspector properties:

- show connectors
- drag/drop
- rename
- row density
- icon set
- line thickness

### UiDropdown

Show:

- plain text dropdown
- icon dropdown
- checkbox/state dropdown if supported
- style preset contrast

Useful inspector properties:

- icon set
- row density
- editable/read-only if relevant
- popup max rows
- frame on/off

### UiMenu

Show:

- menu-bar mode
- context popup mode
- submenu chain
- check/radio items

Useful inspector properties:

- pill / square preset
- icon set
- check/radio visibility
- submenu arrows
- animation if added later

### UiTable

Show:

- one strong live table
- different columns demonstrating icons, read-only, right-align, active cell, selection

Useful inspector properties:

- alternating rows
- row headers
- column headers
- grid lines
- resize on/off
- editor enable

### UiTab

Show:

- standard tabs
- pill tabs
- brutal tabs
- icon tabs

Useful inspector properties:

- orientation
- close buttons
- icon set
- underline vs filled style

### UiTitleCard

Show:

- media-left
- media-right
- compact title card
- documentation-style title card

Useful inspector properties:

- rule on/off
- media side
- title size
- subtitle visibility

### Edit controls

Applies to:

- `UiBaseEdit`
- `UiLineEdit`
- `UiPasswordEdit`
- `UiMaskEdit`
- `UiMultiEdit`
- `UiIntEdit`
- `UiFloatEdit`
- `UiSliderEdit`

Show:

- normal
- error/validation
- icon or assist affordance
- read-only
- disabled

Useful inspector properties:

- placeholder
- read-only
- error state
- assist icon
- font size
- frame thickness

### UiDoc

This should not be tiled.

It should be presented as a compact word-processor workspace:

- toolbar across the top of the preview
- bold / italic / underline
- text color
- bullets
- image insert
- table insert if already supported
- paragraph alignment basics

Inspector should focus on:

- current selection info
- active styles
- document metadata

## Default visual preset set

Where a control supports visual presets, use a familiar preset set across demos:

- Pill
- Square
- Brutal

Optional fourth preset when it materially helps:

- Underline

Do not invent one-off preset names per demo if they mean the same thing.

## Default icon strategy

Where icon swapping is valuable, use the shared icon catalog and expose a small curated set first.

Recommended demo icon sets:

- none
- document/file
- navigation
- status/info
- action/edit

Do not dump the full icon catalog into the UI unless the control is specifically an icon showcase.

## Default animation strategy

Animation should be present, but controlled.

Use animation to show:

- hover transitions
- accent pulse
- accordion open/close
- toggle motion
- panel color shifts

Do not animate every showcase tile.
Each demo should have one or two clearly intentional animation examples.

## Conversion order

Recommended rollout order:

1. `UiButtonDemo`
2. `UiCheckBoxDemo`
3. `UiRadioButtonDemo`
4. `UiToggleDemo`
5. `UiPanelDemo`
6. `UiLabelDemo`
7. `UiAccordionDemo`
8. `UiListDemo`
9. `UiTreeDemo`
10. `UiDropdownDemo`
11. `UiMenuDemo`
12. `UiTabDemo`
13. `UiSliderDemo`
14. edit-control demos
15. `UiTableDemo`
16. `UiDocDemo`

This order proves the shell on small controls first, then on structural controls, then on the large document/table surfaces.

## Immediate template follow-up

Before mass conversion, the demo shell should expose three clean insertion seams:

- preview content host
- state summary host
- property control host

`PreviewGrid()` and `PropertyBox()` now exist.
The next shared addition should be a state-summary host so each demo can add current control metadata without hand-positioning that block every time.

---


# Imported Source: archive/UiDemo_MigrationChecklist.md

# Ui Demo Migration Checklist

Track conversion of every demo onto the shared builder-shell path derived from `UiDemoBase`.

Status key:

- `[ ]` not started
- `[-]` in progress
- `[x]` converted and reviewed

## Foundation

- [x] `UiDemoBase` shell established
- [x] Shared inspector structure normalized to `Usage -> State -> Properties`
- [x] Shared preview/property hosts added
- [x] Shared inspector state host patterns proven in converted demos
- [x] Theme toggle / version pill / red exit pill normalized across active builder demos
- [x] Builder demos use the direct logo asset path without demo-local backdrop hacks

## Converted active builder demos

- [x] `UiAccordionDemo`
- [x] `UiButtonDemo`
- [x] `UiCheckBoxDemo`
- [x] `UiDropdownDemo`
- [x] `UiFontSelectorDemo`
- [x] `UiLabelDemo`
- [x] `UiLineEditDemo`
- [x] `UiListDemo`
- [x] `UiMenuDemo`
- [x] `UiMultiEditDemo`
- [x] `UiPanelDemo`
- [x] `UiPasswordEditDemo`
- [x] `UiRadioButtonDemo`
- [x] `UiSliderDemo`
- [x] `UiTabDemo`
- [x] `UiTableDemo`
- [x] `UiTitleCardDemo`
- [x] `UiToggleDemo`
- [x] `UiTreeDemo`

## Remaining demos still outside the normalized builder-shell path

- [ ] `UiBaseEditDemo`
- [ ] `UiBoxLayoutDemo`
- [ ] `UiColorPickerDemo`
- [ ] `UiDocDemo`
- [ ] `UiGridLayoutDemo`
- [ ] `UiIntFloatDemo`
- [ ] `UiMaskEditDemo`
- [ ] `UiScrollBarDemo`
- [ ] `UiScrollPanelDemo`
- [ ] `UiTabCapDemo`
- [ ] `UiThemeDemo`

## Legacy / utility / lower-priority examples

- [ ] `UiAllControlsDemo`
- [ ] `UiDataModelsTest`
- [ ] `UiIndicatorPaintTest`
- [ ] `UiChoiceRunTests`
- [ ] `UiMenuRunTests`
- [ ] `UiTableRunTests`
- [ ] `UiTreeRunTests`

## Review gates per converted demo

- [x] Uses shared shell
- [x] Has compact header title/subtitle
- [x] Has `Usage`
- [x] Has `State`
- [x] Has `Properties`
- [x] Demonstrates distinct showcase variants only
- [x] Common properties applied where relevant
- [x] Control-specific properties added
- [x] Light/dark behavior checked
- [x] Rebuilt and reviewed visually

## Current note

`UiMultiEditDemo`, `UiRadioButtonDemo`, `UiMenuDemo`, and `UiTabDemo` are now part of the active builder-demo set and should be maintained against the same shell contract and documentation expectations as the earlier converted demos.
