# UiProgressRing

`UiProgressRing` is the circular companion to `UiProgressBar`.

## Contract

- `Set(actual, total)`, `SetTotal`, `Get`, `GetTotal`, `GetRatio`, `GetPercent`,
  integer operators, and `SetData` / `GetData` follow the linear progress value
  vocabulary.
- Percentage text is shown in the center by default. `SetText` replaces it and
  `NoPercent` hides the automatic readout.
- `SetIndeterminate(true)` preserves the unknown-total convention used by
  `UiProgressBar`: total `<= 0` means indeterminate.
- `AnimateOnShow(true)` is enabled by default. The first visible presentation
  eases from zero to the current target value without changing semantic state.
  Later value changes are immediate; `RestartIntroAnimation()` explicitly
  replays the entry motion.

## Circular geometry

The control accepts any allocated rectangle. Painting uses a centered square
based on the shorter axis, so the ring always remains circular rather than
stretching into an ellipse.

Ring-specific style fields are deliberately separate from the rectangular
`UiProgressBar` surface metrics:

- progress colour or along-sweep start/end gradient;
- independent unused-track colour;
- center text colour and preferred font;
- stroke thickness;
- cap radius;
- ring inset;
- intro and indeterminate animation durations.

Center text uses the authored font as its preferred maximum. If the inner ring
area becomes too small, the font is reduced only as much as needed; when even
its configured minimum height cannot fit, text is omitted rather than painted
illegibly.

The progress sweep is computed directly from the `double` value ratio. It is not
quantized to whole percentages; integer percentages are only the default text
presentation and test cases use representative values to protect boundary and
arbitrary-ratio behaviour.

## Rendering

The ring is a transparent control rendered into a bounded alpha `ImageBuffer`
using anti-aliased `BufferPainter`, then composited over its parent. The buffer
is only the centered square ring viewport, not the full rectangular allocation.
Solid progress is a single stroked arc. Gradient progress uses bounded short arc
segments with interpolated colour, capped at 96 segments.

## Validation

Focused test package:

```bat
E:\upp-18468\umk.exe GitHubOut Utilities/UiProgressRingRunTests CLANGx64 -br E:\apps\github\upp_Ui\out\UiProgressRingRunTests.exe
```

Expected summary:

```text
UIPROGRESSRING_SUMMARY checks=47 failed=0
```

Visual demo:

```bat
E:\upp-18468\umk.exe GitHubOut examples/UiProgressRingDemo CLANGx64 -br +GUI E:\apps\github\upp_Ui\out\UiProgressRingDemo.exe
```

The demo exposes value/total, determinate/indeterminate behavior, opening
animation, independent track/progress/text colours, progress gradient,
thickness, cap radius, inset, font face/size/style, and allocated rectangle
width/height through the production `PropertyEditor`.
