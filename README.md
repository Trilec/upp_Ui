# U++ Ui

Modern, style-first controls and UI helpers for Ultimate++.

This repository is building a cohesive `Ui*` layer that sits alongside CtrlLib and focuses on three things:

- consistent styling through shared palette / metrics / skin structures
- practical reusable controls, layouts, and drawing helpers
- demos that are useful in a WYSIWYG sense, not just showcase windows

The library is still evolving, but it is now far enough along that the main direction is stable: clean themed controls, predictable layout behavior, and demo applications that help both users and AI agents understand how to apply the library in real code.

![UiButton demo screenshot](Snapshot_Button.jpg)

![General UI demo screenshot](Snapshot_Controls.jpg)

![UiAccordion demo screenshot](Snapshot_Accordion.jpg)

## Quick links

- `GETTING_STARTED.md` - fast ramp-up
- `UPP_GUIDES/README.md` - engineering guides, architecture notes, and active roadmaps
- `CHECKLIST.md` - living status + next steps
- `CHANGELOG.md`

## Repo layout

- `Ui/` - the `Ui` package: controls, theme/style infrastructure, layout helpers, icons, and draw utilities
- `examples/` - demos and small test apps; these also act as a manual regression suite
- `Animation/` - legacy local copy retained only as reference material; the active dependency is the external `upp_AnimationEasing` nest

## What the repository provides

The `Ui` package currently covers:

- Theme and style infrastructure
  - `UiStyle`, `UiTheme`, `UiDraw`, `UiIcons`
  - Shared palette/metrics/skin driven styling and helper paint routines

- Layout and composition
  - `UiBoxLayout` - lightweight row/column layout with fit, fixed, and expand semantics
  - `UiGridLayout` - structured grid-style placement for denser UI surfaces
  - `UiLayoutCursor` - small incremental placement helper for readable manual layout code

- Text and display controls
  - `UiLabel` - styled text display with selection, alignment, icon/media, and richer presentation options
  - `UiTitleCard` - structured title/subtitle/media header surface
  - `UiDoc` - rich document display/editor surface for larger formatted text content

- Buttons and toggles
  - `UiButton` - general action button with icon/text layout support
  - `UiToolButton` - compact toolbar/action button
  - `UiCheckBox` - themed boolean box control
  - `UiRadioButton` - themed single-choice option control
  - `UiToggle` - themed switch/toggle control

- Panels, containers, and scrolling
  - `UiPanel` - general styled surface with frame, fill, radius, and shadow support
  - `UiAccordion` - collapsible section container with themed headers and bodies
  - `UiScrollPanel` - themed scrollable content host
  - `UiScrollBar` - standalone themed scrollbar
  - `UiTab` - tabbed navigation surface

- Edit and text-entry controls
  - `UiBaseEdit` - shared base for edit controls
  - `UiLineEdit` - single-line text entry
  - `UiPasswordEdit` - masked password entry
  - `UiMaskEdit` - masked formatted text entry
  - `UiMultiEdit` - multi-line text editing
  - `UiIntEdit` - integer entry
  - `UiFloatEdit` - floating-point entry

- Selection, data, and navigation controls
  - `UiDropdown` - themed selection dropdown
  - `UiList` - styled flat list with selection and metadata support
  - `UiTree` - styled hierarchical item browser
  - `UiTable` - themed tabular presentation/edit surface
  - `UiMenu` - themed menu and action surface
  - `UiDataModels` - reusable data/model helpers used across list, tree, dropdown, and table controls

- Slider, picker, and curve utilities
  - `UiSlider` - themed scalar slider
  - `UiSliderEdit` - slider + edit composition surface
  - `UiColorPicker` - reusable color selection utility
  - `UiBezierCurveEditor` - interactive curve editor for four-point bezier curves
  - `UiBezierCurveField` - composite field around the curve editor for direct use in apps and demos

- Supporting utility pieces
  - `UiIndicatorBase`, `UiIndicatorSupport` - shared indicator helpers used by several controls
  - `Ui.h` - package umbrella include

## Intended role of the demos

The demos are meant to be practical reference tools, not only visual showcases.

Examples:

- `UiPanelDemo` acts like a small panel builder that lets a user tune properties and copy the generated code
- `UiFontSelectorDemo` is a useful font helper that previews installed fonts and shows the exact font-construction call
- `UiDemoBase` is the shell/template reference for how demo windows are structured and styled

The current demo direction is to make each demo useful on its own while still exposing the implementation patterns behind the control.

## Example/demo inventory

The `examples/` directory currently includes:

- `UiDemoBase` - shared demo shell/template reference
- `UiAllControlsDemo` - broad visual overview of the control set
- `UiAccordionDemo`, `UiPanelDemo`, `UiFontSelectorDemo`, `UiThemeDemo`
- `UiButtonDemo`, `UiToolButton` behavior is represented in the button-oriented demos
- `UiLabelDemo`, `UiTitleCardDemo`
- `UiBoxLayoutDemo`, `UiGridLayoutDemo`, `UiScrollPanelDemo`, `UiScrollBarDemo`
- `UiBaseEditDemo`, `UiLineEditDemo`, `UiPasswordEditDemo`, `UiMaskEditDemo`, `UiMultiEditDemo`, `UiIntFloatDemo`
- `UiDropdownDemo`, `UiListDemo`, `UiTreeDemo`, `UiTableDemo`, `UiMenuDemo`, `UiTabDemo`, `UiTabCapDemo`
- `UiCheckBoxDemo`, `UiRadioButtonDemo`, `UiToggleDemo`
- `UiColorPickerDemo`, `UiDocDemo`
- `UiIndicatorPaintTest`, `UiDataModelsTest`, `UiDocModelTest`
- `UiChoiceRunTests`, `UiMenuRunTests`, `UiTableRunTests`, `UiTreeRunTests`

## Build and run

Important:

- `Ui` is a library package, not a runnable main package.
- Do not select `Ui` itself as the main package in TheIDE.
- Use one of the packages under `examples/` as the runnable main package.

### TheIDE

1. Open the repository in TheIDE.
2. Ensure your assembly/nests can see both:
   - this repo, for `Ui` and `examples`
   - U++ `uppsrc`, for `Core`, `Draw`, `CtrlLib`, and related packages
3. Set a demo package as the main package and run that package, for example `examples/UiPanelDemo`, `examples/UiFontSelectorDemo`, or `examples/UiButtonDemo`.

Dependencies used by `Ui/Ui.upp` include `Painter` and `Animation`.

Note: `Animation` should resolve from the external `E:\apps\github\upp_AnimationEasing` nest, not from a local package inside this repository.

### CLI with `umk`

Example on Windows:

```bat
"E:\upp-18468\umk.exe" "E:\apps\github\upp_Ui,E:\upp-18468\uppsrc" examples/UiButtonDemo CLANGx64 -br +GUI "E:\apps\github\upp_Ui\build\UiButtonDemo"
```

## Public API direction

This is a new codebase with no backward-compat naming shim layer. Public names are being kept explicit and direct, and docs/demos are updated with the code.

Current conventions used across the controls and demos:

- interactive selection controls expose `SetData()` / `GetData()`
- selection notifications use `WhenSelection`
- `UiTab` uses `SetActiveTab()` / `GetActiveTab()`
- `UiDropdown` uses explicit model binding via `SetModel(UiListModel&)`, `UseInternalModel()`, and `GetInternalModel()`
- numeric edits keep one vocabulary: `Min`, `Max`, `MinMax`, `Step`, `Precision`, `NotNull`
- `UiAccordion` section accessors are `GetSectionContent()`, `GetSectionHeader()`, and `GetSectionBody()`

Minimal button usage:

```cpp
UiButton b;
b.SetText("Run")
 .SetIcon(CtrlImg::go_forward())
 .SetIconLayout(UiAlign::LEFT)
 .SetStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
```

## Current status

The project is still under active refinement, especially around:

- demo quality and usefulness
- final polish of default styles
- ensuring new controls stay clean, documented, and low-bloat in usage

The overall direction is no longer just exploratory. The controls, helpers, and demos are being shaped into a usable styled UI layer for real Ultimate++ applications.

## License

Intended to live alongside Ultimate++.
