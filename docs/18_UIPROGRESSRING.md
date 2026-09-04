# UiProgressRing

`UiProgressRing` is the single-value circular companion to `UiProgressBar`.
It is deliberately not a chart control: one semantic value advances toward one
total. Multi-part composition belongs to `UiChartRing`.

## Contract

- `Set(actual, total)`, `SetTotal`, `Get`, `GetTotal`, `GetRatio`, `GetPercent`,
  integer operators, and `SetData` / `GetData` follow the linear progress value
  vocabulary.
- `SetRole(UiRole)` selects the same Standard/Subtle/Accent/Alert semantic family
  used by the rest of `upp_Ui`. With no custom style, live theme revisions are
  inherited automatically. `SetCustomStyle` remains the explicit local override
  boundary.
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

Ring-specific style fields remain separate from rectangular `UiProgressBar`
surface metrics:

- progress colour or along-sweep start/end gradient;
- independent unused-track colour;
- center text colour and preferred font;
- stroke thickness;
- cap roundness from `0` to `100` percent;
- ring inset;
- intro and indeterminate animation durations.

`cap_roundness` is proportional to stroke thickness and defaults to `100`.
`0` is a flat butt end, intermediate values progressively round the endpoint,
and `100` is a true semicircular end whose radius is half the current stroke
thickness.

The progress sweep is computed directly from the `double` ratio. Integer
percentages are a text presentation only; the painted geometry is not quantized
to whole-percent steps.

## Shared circular-arc primitive

Exact arc/gradient/cap painting now lives in `UiDraw` as the generic
`UiPaintCircularArc` primitive. It follows the repository geometry hierarchy:
keep the circular stroke as a native Painter arc rather than pre-flattening it
into a control-owned point loop. `UiProgressRing` owns its progress-specific
track/raster composition locally, while `UiChartRing` owns its multi-segment
composition. There is no separate ring-renderer subsystem and no common ring
control base class.

Both solid and gradient progress use one exact Painter arc. Gradient mode uses
an angular image brush on the same stroke, so the sweep remains continuous.
Partial arcs use native flat/round caps at the endpoints and UiDraw's
thickness-relative intermediate cap geometry. Intermediate custom caps use a
bounded sub-pixel stroke overlap to prevent an antialiasing hairline at the
cap/stroke join. Full `100%` sweeps are closed circles with no cap seam.

`UiShapes::RingSegment` / `UiShapes::Pie` exist for controls that need an
authored **filled radial silhouette**. They are not a reason to replace this
cheaper native stroked-arc path. Normal controls should use the highest shared
layer that fits the job; dense scenes may use `UiGeometry` directly. See
`24_UI_GEOMETRY_CONTRACT.md` and `25_UI_SHAPE_PATH.md`.

## Raster policy

The center text remains direct `Draw` text. Ring geometry follows the accepted
Ui rendering policy:

- stable determinate presentation -> exact `UiRasterCache` entry tagged
  `aa/ui-progress-ring`;
- intro and indeterminate animation -> live bounded `BufferPainter` raster;
- oversized rasters outside the shared AA cache policy -> live raster fallback.

The cache key includes every raster-affecting value: exact viewport size,
radius, start/sweep, thickness, cap roundness, track/progress/gradient colours
and gradient enablement. Theme/role changes therefore naturally resolve to a
new exact key rather than reusing stale pixels.

## Demo

`examples/UiProgressRingDemo` follows the canonical full-demo shell:

- Inspector — progress value/text, semantic role and preview layout;
- Theme Overrides — explicit live-state (Normal/Disabled) progress/gradient/track/text colours,
  geometry, typography and optional motion timing, inherited until individually activated;
- Code — generated C++ from the same authored state;
- Light/Dark, Help, Replay and Exit actions.

The preview remains genuinely theme-driven while no Theme Override is active.

## Validation

Focused package:

```bat
E:\upp-18468\umk.exe GitHubOut Utilities/UiProgressRingRunTests CLANGx64 -br E:\apps\github\upp_Ui\out\UiProgressRingRunTests.exe
```

Expected summary after this tranche:

```text
UIPROGRESSRING_SUMMARY checks=60 failed=0
```

The focused contract now includes semantic-role/theme inheritance, custom-style
preservation and exact stable raster-cache reuse in addition to the existing
value, geometry, cap, gradient, text and animation checks.

Visual demo:

```bat
E:\upp-18468\umk.exe GitHubOut examples/UiProgressRingDemo CLANGx64 -br +GUI E:\apps\github\upp_Ui\out\UiProgressRingDemo.exe
```
