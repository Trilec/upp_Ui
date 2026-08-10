# U++ Ui

Modern, style-first controls and UI helpers for Ultimate++.

This repository contains the reusable **Ui library family**:

- `Ui` — the control library: controls, theme/style infrastructure, layout
  helpers, icons, and draw utilities.
- `Utilities/PropertyEditorCore` — headless property schema and value model.
- `Utilities/PropertyEditor` — the `Ui`-backed property inspector/editor.

Important controls include `UiMatrixSelector` (compact styled matrix selector)
and `UiRangeSlider` (two-handle interval slider), alongside the rest of the
control family documented in the controls guide.

![UiButton demo screenshot](Snapshot_Button.jpg)

![General UI demo screenshot](Snapshot_Controls.jpg)

![UiAccordion demo screenshot](Snapshot_Accordion.jpg)

![UiColorPicker demo screenshot](Snapshot_Colorpicker.jpg)

![UiFontSelector demo screenshot](Snapshot_FontSelector.jpg)

![UiTheme demo screenshot](Snapshot_Theme.jpg)

## Documentation

The canonical, portable documentation set lives in `docs/`:

- `docs/00_UPP_CODING_GUIDE.md` — reusable U++ engineering practice
- `docs/01_UI_CONTROLS_GUIDE.md` — the current control catalogue
- `docs/02_UI_THEME_GUIDE.md` — theme and style system
- `docs/03_UI_MODEL_GUIDE.md` — model-driven architecture and PropertyEditor
- `docs/04_UI_DEMO_GUIDE.md` — the intended demo structure
- `docs/05_UI_PROPERTY_EDITOR_GUIDE.md` — complete PropertyEditor integration,
  adapters, transactions, providers, layout, and performance contract

Start with `00_UPP_CODING_GUIDE.md`, then read the controls guide before
touching any control or demo.

## Repository layout

- `Ui/` — the `Ui` package (controls, themes, layouts, icons, draw helpers).
- `Utilities/` — sibling reusable packages: `PropertyEditor`,
  `PropertyEditorCore`, `IconExportCore`, `MakeIconFromSVG`, plus control and
  model test packages.
- `examples/` — demos; these also act as a manual regression suite.
- `tests/` — control test packages.
- `docs/` — the canonical guide set above.

## Building

Use the local assembly (`GitHubOut.var`) with `umk`:

```bat
E:\upp-18468\umk.exe GitHubOut Ui CLANGx64 -br E:\apps\github\upp_Ui\build\Ui.exe
E:\upp-18468\umk.exe GitHubOut Utilities/PropertyEditorCore CLANGx64 -br E:\apps\github\upp_Ui\build\PropertyEditorCore.exe
E:\upp-18468\umk.exe GitHubOut Utilities/PropertyEditor CLANGx64 -br E:\apps\github\upp_Ui\build\PropertyEditor.exe
```

Demos build from the `examples` nest, e.g.:

```bat
E:\upp-18468\umk.exe GitHubOut examples/UiLabelDemo CLANGx64 -br +GUI E:\apps\github\upp_Ui\build\UiLabelDemo.exe
```

## Licence

Apache License 2.0 — see `LICENSE`.
