# UiProgressRing

`UiProgressRing` is the single-value circular companion to `UiProgressBar`. It is deliberately not a chart control: one semantic value advances toward one total. Multi-value composition belongs to `UiRingChart`.

## Contract

- `Set(actual, total)`, `SetTotal`, `Get`, `GetTotal`, `GetRatio`, `GetPercent`, integer operators, and `SetData` / `GetData` follow the linear progress vocabulary.
- `SetRole(UiRole)` uses the Standard/Subtle/Accent/Alert semantic family. With no custom style, live theme revisions are inherited automatically. `SetCustomStyle` is the explicit local override boundary.
- Percentage text is centered by default. `SetText` replaces it and `NoPercent` hides the automatic readout.
- `SetIndeterminate(true)` uses total `<= 0`, matching `UiProgressBar` unknown-total semantics.
- `AnimateOnShow(true)` is enabled by default. Intro animation changes presentation only; semantic value remains authoritative.

## Geometry and caps

Painting uses a centered square based on the shorter allocated axis, so the ring remains circular in rectangular layouts.

`cap_roundness` is thickness-relative and defaults to `100`:
- `0` = flat butt end;
- intermediate values progressively round the two end corners while retaining a central face;
- `100` = a true semicircular end with radius equal to half the current stroke thickness.

The sweep is calculated from the exact `double` ratio. Integer percentages are text presentation only.

## Shared renderer

Exact arc/gradient/cap painting lives in `Ui/UiRingDraw.h/.cpp`. `UiRingDraw` is an internal geometry/render helper, not a common control base. It is shared by `UiProgressRing` and `UiRingChart`.

Solid and gradient progress use one exact Painter arc. Gradient mode uses an angular image brush on the same stroke. Full 100% sweeps are closed circles with no cap seam.

## Raster policy

Center text remains direct `Draw` text.

- stable determinate presentation -> exact `UiRasterCache` entry tagged `aa/ui-progress-ring`;
- intro and indeterminate animation -> live bounded `BufferPainter` raster;
- oversized rasters outside shared AA-cache policy -> live fallback.

The stable cache key includes all raster-affecting geometry and colours, so role/theme changes cannot reuse stale pixels.

## Demo

`examples/UiProgressRingDemo` follows the canonical full-demo shell:
- Inspector — value/text, semantic role and preview layout;
- Theme Overrides — Normal/Disabled progress, gradient, track, text, geometry, typography and motion settings, inherited until individually activated;
- Code — generated C++ from the same authored state;
- Light/Dark, Help, Replay and Exit actions.

## Validation

Focused package:

```bat
E:\upp-18468\umk.exe GitHubOut Utilities/UiProgressRingRunTests CLANGx64 -br E:\apps\github\upp_Ui\out\UiProgressRingRunTests.exe
```

Expected:

```text
UIPROGRESSRING_SUMMARY checks=60 failed=0
```

Visual demo:

```bat
E:\upp-18468\umk.exe GitHubOut examples/UiProgressRingDemo CLANGx64 -br +GUI E:\apps\github\upp_Ui\out\UiProgressRingDemo.exe
```
