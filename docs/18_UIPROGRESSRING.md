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
- cap roundness from `0` to `100` percent;
- ring inset;
- intro and indeterminate animation durations.

`cap_roundness` is proportional to stroke thickness and defaults to `100`.
It is not an independent pixel radius. `0` produces a flat butt end. As the
percentage increases, two proportional quarter-circle corner arcs grow while
the central flat end face shrinks. At `100` the flat face reaches zero and the
two corner arcs form a true semicircular end whose radius is half the current
stroke thickness. Increasing thickness therefore keeps the authored cap shape
proportional instead of exposing a small detached bump. The public API is
`SetCapRoundness(int percent)` / `GetCapRoundness()` and clamps to `0..100`.

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

Both solid and gradient progress use one exact Painter arc path. Solid progress
strokes that path directly. Gradient progress builds a small angular colour
brush for the same viewport and uses Painter's image-brush stroke support, so
colour follows the sweep continuously without dividing the geometry into short
segments.

For partial sweeps, `0%` cap roundness uses Painter's native butt cap and `100%`
uses the native round cap. Intermediate values keep the same butt-ended arc body
and construct the endpoint from two thickness-relative quarter-circle curves
plus the remaining central flat face. The endpoint uses the same angular
gradient brush as the arc, so changing cap roundness does not change gradient
continuity. A full `100%` sweep is painted as a closed circle, where end-cap
shape is irrelevant and there is no start/end cap seam.

The angular brush is generated only when gradient mode is active. This keeps the
ordinary solid path minimal while avoiding the visible radial seams and cap
artifacts produced by the retired segmented-gradient implementation.

## Validation

Focused test package:

```bat
E:\upp-18468\umk.exe GitHubOut Utilities/UiProgressRingRunTests CLANGx64 -br E:\apps\github\upp_Ui\out\UiProgressRingRunTests.exe
```

Expected summary:

```text
UIPROGRESSRING_SUMMARY checks=51 failed=0
```

Visual demo:

```bat
E:\upp-18468\umk.exe GitHubOut examples/UiProgressRingDemo CLANGx64 -br +GUI E:\apps\github\upp_Ui\out\UiProgressRingDemo.exe
```

The demo exposes value/total, determinate/indeterminate behavior, opening
animation, independent track/progress/text colours, progress gradient,
thickness, cap roundness `0..100%`, inset, font face/size/style, and allocated
rectangle width/height through the production `PropertyEditor`.

For rendering acceptance, explicitly compare solid and gradient modes at low,
mid and high percentages. Test cap roundness at `0`, an intermediate value such
as `50`, and `100`, including a very thick stroke. The progress body must remain
a continuous smooth arc with a full-width endpoint profile: no visible segment
spokes, radial wedges, holes, detached cap shapes, or small nipple-like caps as
stroke thickness increases.
