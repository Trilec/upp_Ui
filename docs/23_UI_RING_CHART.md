# 23 — UiRingChart

`UiRingChart` is the proportional multi-value companion to `UiProgressRing`.

The semantic boundary is intentional:
- `UiProgressRing` = one current amount against one total;
- `UiRingChart` = several authored values composing a ring.

Both controls share internal `UiRingDraw` geometry rather than mixing chart semantics into the progress control.

## Data contract

`UiRingSegment` stores a non-negative `value`, optional `label`, and optional explicit `color`. `Null` colour uses the themed series palette.

Composition APIs are `AddSegment`, `SetSegments` and `ClearSegments`. Values do not need to be percentages.

With no explicit total, the chart normalizes against the sum of positive segments. A larger explicit total leaves unused track as a remainder. An explicit total below the segment sum never clips authored data; the resolved total expands to the sum.

## Geometry

The chart stays circular in any rectangular allocation. Segments preserve authored order and begin at 12 o'clock, clockwise.

`segment_gap` means the visible pixel gap between painted segment ends. Geometry converts this to an angular centerline separation and compensates for cap extension, so rounded caps do not overlap or silently consume the requested gap.

Useful combinations:
- gap `0`, cap `0` -> contiguous classic donut;
- positive gap, cap `100` -> separated rounded segments;
- intermediate cap roundness -> softer bounded sections.

`SetCenterText` is presentation-only and uses the same shrink-without-grow fitting policy as `UiProgressRing`.

## Theme/style

`UiRingChart` follows the normal Ui lifecycle:
- semantic `UiRole`;
- live theme revision inheritance while no custom style is active;
- `SetCustomStyle` / `ClearCustomStyle` local override boundary;
- themed track, center text and eight series palette slots;
- an explicit per-segment colour overrides the palette only for that datum.

V1 composes the established `UiProgressBar` semantic role family rather than introducing a separate public `ResolveRingChart` theme family.

## Rendering

Every visible segment is one exact Painter arc through `UiRingDraw`; there is no short-arc segmentation approximation.

V1 has no animation, so stable ring geometry uses exact `UiRasterCache` entries tagged `aa/ui-ring-chart`. Center text remains direct Draw. The cache key includes exact geometry, track colour and every visible segment's start/sweep/colour.

V1 deliberately omits legends, selection, exploded slices, nested rings and hover popups. Labels remain in the data model for host/future legend use.

## Demo

`examples/UiRingChartDemo` uses the canonical Inspector / Theme Overrides / Code shell with Light/Dark, Help and Exit actions. The example exposes four proportional values, automatic/explicit total, semantic role, center text, thickness, cap roundness, segment gap, inset, typography and series palette overrides.

## Validation

Focused package:

```bat
E:\upp-18468\umk.exe GitHubOut Utilities/UiRingChartRunTests CLANGx64 -br E:\apps\github\upp_Ui\out\UiRingChartRunTests.exe
```

Expected:

```text
UIRINGCHART_SUMMARY checks=35 failed=0
```

Demo:

```bat
E:\upp-18468\umk.exe GitHubOut examples/UiRingChartDemo CLANGx64 -br +GUI E:\apps\github\upp_Ui\out\UiRingChartDemo.exe
```
