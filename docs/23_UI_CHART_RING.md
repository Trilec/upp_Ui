# 23 — UiChartRing

`UiChartRing` is the proportional multi-value companion to `UiProgressRing`.

The semantic boundary is intentional:

- `UiProgressRing` = one current amount against one total;
- `UiChartRing` = several authored values composing a ring.

Do not add chart segments to `UiProgressRing` merely to reuse circular geometry.
Both controls instead use the generic `UiPaintCircularArc` primitive in `UiDraw`.

## Data contract

`UiChartRingSegment` stores:

- non-negative `value`;
- optional `label` for host/legend use;
- optional explicit `color`; `Null` means use the themed series palette.

`AddSegment`, `SetSegments` and `ClearSegments` own composition data.
Values do not need to be percentages.

With `SetTotal(0)`, the visible total is the sum of positive segment values.
A larger explicit total leaves the unused track visible as remainder. If an
explicit total is smaller than the segment sum, the resolved total expands to
the sum rather than clipping later segments.

## Geometry

The chart remains circular inside any rectangular allocation. Segments preserve
authored order and start at 12 o'clock, clockwise.

`segment_gap` is authored as the visible pixel gap between neighbouring painted
segments. Geometry converts it to an angular centreline gap at the current
radius and automatically adds the two cap extensions, so rounded caps do not
overlap or silently consume the requested gap. A local gap is bounded for very
small segments so a positive segment is not erased solely by spacing.

Useful combinations:

- gap `0`, cap `0` -> contiguous classic donut;
- positive gap, cap `100` -> separated rounded segments;
- intermediate cap roundness -> softer but still visibly bounded sections.

`SetCenterText` is presentation-only and uses the same shrink-without-grow text
policy as `UiProgressRing`.

## Theme/style

`UiChartRing` follows the normal Ui lifecycle:

- semantic `UiRole`;
- theme revision cache while inherited;
- `SetCustomStyle` / `ClearCustomStyle` local override boundary;
- theme-derived track, center text and eight series palette slots;
- explicit per-segment colour overrides the corresponding series slot only for
  that datum.

No `UiTheme::ResolveChartRing` public family is introduced yet. The control
composes the established `UiProgressBar` semantic role family into chart-ring
track/text/series colours. `Standard` preserves the normal multicolour series
palette; `Accent` and `Alert` derive tonal families from their semantic primary
colour; `Subtle` uses a deliberately muted tonal family. Explicit per-segment
colours remain authored overrides.

## Rendering

`UiChartRing` uses the `UiPaintCircularArc` primitive from `UiDraw` for every
segment. Each visible segment is one exact Painter arc; there is no short-arc
segmentation approximation.

The chart has no animation in V1, so its ring geometry is an ideal exact
`UiRasterCache` workload under tag `aa/ui-chart-ring`. Center text remains direct.
The raster key includes exact geometry, track colour and every visible segment's
start/sweep/colour.

V1 deliberately omits legends, selection, exploded slices, nested rings and
hover popups. Labels are retained in the data model for hosts or a future legend
without forcing those features into the base chart.

## Validation

Focused package:

```bat
E:\upp-18468\umk.exe GitHubOut Utilities/UiChartRingRunTests CLANGx64 -br E:\apps\github\upp_Ui\out\UiChartRingRunTests.exe
```

Expected:

```text
UICHARTRING_SUMMARY checks=39 failed=0
```

Demo:

```bat
E:\upp-18468\umk.exe GitHubOut examples/UiChartRingDemo CLANGx64 -br +GUI E:\apps\github\upp_Ui\out\UiChartRingDemo.exe
```
