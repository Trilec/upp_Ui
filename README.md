# U++ Ui

Modern, style-first controls and UI helpers for Ultimate++.

This repository is building a cohesive `Ui*` layer that sits alongside CtrlLib and focuses on three things:

- consistent styling through shared palette / metrics / skin structures
- practical reusable controls, layouts, and drawing helpers
- demos that are useful in a WYSIWYG sense, not just showcase windows

The library is moving toward a Version 1 release. The main direction is stable: clean themed controls, predictable layout behavior, clear ownership, and demo/Designer applications that help users and coding agents understand how to apply the library in real code.

![UiButton demo screenshot](Snapshot_Button.jpg)

![General UI demo screenshot](Snapshot_Controls.jpg)

![UiAccordion demo screenshot](Snapshot_Accordion.jpg)

![UiColorPicker demo screenshot](Snapshot_Colorpicker.jpg)

![UiFontSelector demo screenshot](Snapshot_FontSelector.jpg)

![UiTheme demo screenshot](Snapshot_Theme.jpg)

![Designer screenshot](Snapshot_Designer.jpg)

## Quick links

- `GETTING_STARTED.md` - fast ramp-up
- `UPP_GUIDES/00_Ui_V1_Engineering_Contract.md` - current Version 1 API, style, geometry, lifetime, and Designer authority
- `UPP_GUIDES/Ui_V1_Control_Audit.md` - release-readiness audit and remediation workstreams
- `UPP_GUIDES/Ui_V1_Documentation_Cleanup.md` - documentation retention, cleanup, consolidation, and archive plan
- `UPP_GUIDES/README.md` - guide reading order and documentation authority
- `CHANGELOG.md`

## Repo layout

- `Ui/` - the `Ui` package: controls, theme/style infrastructure, layout helpers, icons, and draw utilities
- `examples/` - demos and small test apps; these also act as a manual regression suite
- `Utilities/Designer/` - visual layout/control designer for building and saving UI designs, inspecting generated code, and testing layout behavior
- `UPP_GUIDES/` - current engineering contracts, detailed guides, feature references, audits, and archived design history

## What the repository provides

The runtime source of truth is `Ui/Ui.upp` together with the public umbrella header `Ui/Ui.h`. Documentation and Designer coverage are expected to follow that inventory.

The `Ui` package currently covers:

- Theme and style infrastructure
  - `UiStyle`, `UiTheme`, `UiDraw`, `UiIcons`
  - shared palette/metrics/skin styling, icon catalog, and paint helpers

- Layout and composition
  - `UiBoxLayout` - lightweight row/column layout with fit, fixed, and expand semantics
  - `UiGridLayout` - structured grid-style placement for denser UI surfaces
  - `UiSplitter` - two-pane splitter layout with themed thumb and pane sizing
  - `UiQuadSplitter` - four-pane splitter layout for editor-style workspaces
  - `UiStack` - page stack/container for switching between hosted child pages
  - `UiLayoutCursor` - small incremental placement helper for readable manual layout code
  - `UiMeasureLayout` - helper for asking a control how much space it wants

- Text and display controls
  - `UiLabel` - styled text display with selection, alignment, icon/media, and richer presentation options
  - `UiTitleCard` - structured title/subtitle/media header surface
  - `UiBreadcrumbs` - path/navigation display with optional icons and dividers
  - `UiDoc` - rich document display/editor surface for larger formatted text content

- Buttons and toggles
  - `UiButton` - general action button with icon/text layout support
  - `UiToolButton` - compact toolbar/action button
  - `UiCheckBox` - themed boolean box control
  - `UiRadioButton` - themed single-choice option control
  - `UiToggle` - themed switch/toggle control
  - `UiSplitButton` - action button with a secondary popup lane
  - `UiMenu` - themed popup action surface

- Panels, containers, and scrolling
  - `UiPanel` - general styled host surface with frame, fill, radius, and shadow support
  - `UiGroupPanel` - titled single-content container with icon/subtitle/side-title header chrome
  - `UiAccordion` - collapsible section container with themed headers and bodies
  - `UiScrollPanel` - themed scrollable content host
  - `UiScrollBar` - standalone themed scrollbar
  - `UiTab` - tabbed navigation surface
  - `UiStack` - stacked page container for multi-view surfaces
  - `UiSliderEdit` - a slider paired with a text field

- Edit and text-entry controls
  - `UiBaseEdit` - shared base for edit controls
  - `UiLineEdit` - single-line text entry
  - `UiPasswordEdit` - masked password entry with optional visibility control
  - `UiMaskEdit` - masked formatted text entry
  - `UiMultiEdit` - multi-line text editing
  - `UiIntEdit` - integer entry
  - `UiFloatEdit` - floating-point entry
  - `UiColorPicker` - reusable color picker with numeric channels and swatch workflow

- Selection, data, and navigation controls
  - `UiDropdown` - themed selection dropdown
  - `UiList` - styled flat list with selection and metadata support
  - `UiTree` - styled hierarchical item browser
  - `UiTable` - themed tabular presentation/edit surface
  - `UiDataModels` - reusable data/model helpers used across list, tree, dropdown, and table controls

- Slider, picker, and curve utilities
  - `UiSlider` - themed scalar slider
  - `UiBezierCurveEditor` - interactive curve editor for four-point bezier curves
  - `UiBezierCurveField` - composite field around the curve editor for direct use in apps and demos

- Composite controls
  - `Composites/UiCompositeColor` - color-oriented composite helpers
  - `Composites/UiCompositeDropdown` - dropdown composition helpers
  - `Composites/UiCompositeEdit` - edit composition helpers
  - `Composites/UiCompositeLabel` - label composition helpers
  - `Composites/UiCompositeSlider` - slider composition helpers
  - `Composites/UiCompositeToggle` - toggle composition helpers

- Supporting utility pieces
  - `UiIndicatorBase`, `UiIndicatorSupport` - shared indicator helpers used by several controls
  - `Ui.h` - package umbrella include

Controls still being added or closed out for Version 1 should be described as in progress until their runtime implementation, tests, demo, documentation, and Designer integration have all been accepted.

## Designer

`Utilities/Designer` is the visual designer for this library. It lets you build a model using layouts, containers, controls, composites, and presets; inspect the hierarchy and sizing modes; save/load designs as JSON; and view generated U++ code while testing layout behavior in the preview.

The Designer uses the same fit, fixed, and expand sizing concepts as the runtime controls, together with box/grid placement, spacers, splitters, panel-style hosting, page containers, theme roles, and reusable presets.

Designer coverage is a Version 1 parity gate rather than a separate control inventory:

- adapters should construct real runtime controls;
- inspector properties should map to real runtime APIs or style fields;
- inapplicable properties should not remain silently editable;
- preview, save/reload, and generated code should agree.

![Designer screenshot](Snapshot_Designer.jpg)

## Intended role of the demos

The demos are practical reference and manual regression tools, not only visual showcases.

Examples:

- `UiPanelDemo` acts like a small panel builder that lets a user tune properties and copy generated code
- `UiFontSelectorDemo` previews installed fonts and shows the corresponding font-construction call
- `UiDemoBase` is the shell/template reference for how demo windows are structured and styled

The active builder demos share a common shell contract: header/title/subtitle/logo, version pill, theme toggle, exit control, dotted preview canvas, and Usage -> State -> Properties inspectors with live code output.

## Example/demo inventory

The `examples/` directory includes focused demos and shared demo infrastructure, including:

- `UiDemoBase`
- panel, accordion, theme, label, title-card, breadcrumb, button, and toggle demos
- box/grid/splitter/scroll demos
- base, line, password, mask, multi-line, integer, and floating-point edit demos
- dropdown, list, tree, table, menu, and tab demos
- color picker, document, and OS file-dialog examples

The checked-in package list is the source of truth. Retired prototype demos should remain outside the active build sweep or under the documented archive/OLD location.

## Build and run

Important:

- `Ui` is a library package, not a runnable main package.
- Do not select `Ui` itself as the main package in TheIDE.
- Use one of the packages under `examples/` as the runnable main package.

### TheIDE

1. Open the repository in TheIDE.
2. Ensure the selected assembly can see:
   - this repository;
   - the external `Animation` package used by `Ui/Ui.upp`;
   - U++ `uppsrc`.
3. Select a demo package or `Utilities/Designer` as the main package.

For this repository, prefer the checked-in `GitHubOut.var` assembly for local development. It contains the project nest configuration and keeps intermediates and runnable outputs under the repository `out/` directory rather than the global U++ output tree.

Do not place current demo or Designer outputs in `build/` or `bin/`.

## Public API direction

This is a new codebase with no backward-compatibility naming shim layer. Experimental names may be corrected before Version 1.

The canonical project rules are in `UPP_GUIDES/00_Ui_V1_Engineering_Contract.md`.

Current conventions include:

- public control methods use normal U++-style PascalCase names such as `SetText`, `GetData`, and `SetCustomStyle`;
- callbacks use `WhenX`;
- interactive selection controls expose `SetData()` / `GetData()` where data binding is natural;
- selection notifications use the accepted family callback, normally `WhenSelection`;
- `UiTab` uses `SetActiveTab()` / `GetActiveTab()`;
- `UiDropdown` uses explicit model binding through `SetModel(UiListModel&)`, `UseInternalModel()`, and `GetInternalModel()`;
- numeric edits use `Min`, `Max`, `MinMax`, `Step`, `Precision`, and `NotNull`;
- `UiAccordion` section accessors are `GetSectionContent()`, `GetSectionHeader()`, and `GetSectionBody()`;
- custom-painted primitive parts use dedicated hooks rather than outer overpaint.

Spacing vocabulary:

- `item_spacing` - spacing between repeated owned items;
- `content_gap` - primary spacing inside one item/control surface;
- semantic secondary gaps use explicit names such as `right_gap`, `metadata_gap`, `drag_gap`, `chevron_gap`, or `accessory_gap`;
- `content_margin` - margin inside a styled surface around painted content;
- container/layout inset remains a distinct host-placement concept.

Minimal button usage:

```cpp
UiButton button;
button.SetText("Run")
      .SetIcon(CtrlImg::go_forward())
      .SetIconSide(UiAlign::LEFT)
      .SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
```

## Current status

The project is in Version 1 hardening:

- runtime API and style unification;
- container/layout stability;
- complete Designer coverage and parity;
- focused tests and demos;
- documentation and release cleanup.

The goal is a solid styled UI layer for real Ultimate++ applications, without preserving accidental experimental APIs or hidden behavior.

## License

Intended to live alongside Ultimate++.
