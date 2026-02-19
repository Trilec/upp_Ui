# U++ Ui (experimental)

Modern, style-first controls for Ultimate++.

This is intentionally forward-looking work: new `Ui*` controls designed to coexist with CtrlLib (not replace it overnight). The code is optimized for a clean, consistent styling surface (palette/metrics/skin), predictable layout (icon+text blocks with per-block margins), and optional animation hooks.

![UiButton demo screenshot](Snapshot_Button.jpg)

![General UI demo screenshot](Snapshot_Controls.jpg)

![UiAccordion demo screenshot](Snapshot_Accordion.jpg)

## Quick links

- `GETTING_STARTED.md` (fast ramp-up)
- `ARCHITECTURE.md` (style/draw/layout/9-slice/blur/animation concepts)
- `CHECKLIST.md` (living status + next steps)
- `CHANGELOG.md`

## Repo layout

- `Ui/` - the `Ui` package (controls + styling and drawing helpers)
- `examples/` - demo packages (these act as a manual regression suite)

## What currently builds in the `Ui` package

As of today, `Ui/Ui.upp` compiles these controls:

- Core: `UiStyle.h`, `UiDraw.h`, `UiIcons.h`
- Layout: `UiBoxLayout`, `UiGridLayout`
- Text/edit: `UiBaseEdit`, `UiLineEdit`, `UiPasswordEdit`, `UiMaskEdit`, `UiMultiEdit`, `UiIntEdit`, `UiFloatEdit`
- Value/scroll: `UiSlider`, `UiSliderEdit`, `UiScrollBar`
- Core controls: `UiLabel`, `UiButton`, `UiCheckBox`, `UiToggle`, `UiRadioButton`, `UiPanel`, `UiAccordion`, `UiScrollPanel`, `UiTitleCard`

## Build and run (TheIDE)

1) Open the repository in TheIDE.
2) Ensure your assembly/nests can see both:
   - this repo (for `Ui` + `examples`)
   - U++ `uppsrc` (for `Core/Draw/CtrlLib/...`)
3) Run a demo, e.g. `examples/UiLabelDemo` or `examples/UiButtonDemo`.

Dependencies used by `Ui/Ui.upp` include `Painter` and `Animation`.

Note: `Animation/` in this repo is a copy of the animation/easing package from `E:\apps\github\upp_AnimationEasing`.

## Build from CLI (umk)

Example (Windows):

```bat
"E:\upp-18182\umk.exe" "E:\apps\github\upp_Ui,E:\upp-18182\uppsrc" examples/UiButtonDemo CLANGx64 -br +GUI "E:\apps\github\upp_Ui\build\UiButtonDemo"
```

## API note

This is a new codebase with no backward-compat naming shims. If an API name changes, we update demos + docs to match.

Minimal button usage (current naming):

```cpp
UiButton b;
b.SetText("Run")
 .SetIcon(CtrlImg::go_forward())
 .SetIconLayout(UiAlign::LEFT)
 .SetAccentStyle();
```

## License

Intended to live alongside Ultimate++.
