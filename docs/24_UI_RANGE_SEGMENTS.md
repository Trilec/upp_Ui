# 24 — UiRangeSegments

`UiRangeSegments` is the multi-boundary scalar range editor in the `Ui` control
family. It divides one fixed numeric domain into contiguous labelled segments and
lets the user move the internal boundaries without moving the outer endpoints.

It is intended for cases such as LOD thresholds, quality bands, classification
ranges, processing stages, allocation bands and similar ordered numeric domains.
It is **not** a chart and it is **not** a continuous colour-gradient editor.

## Semantic contract

The control owns one domain `[min, max]` and an ordered vector of
`UiRangeSegment` records:

```cpp
struct UiRangeSegment {
    double span;
    String label;
    Color  color; // Null = automatic palette
    Value  data;  // optional application payload
};
```

`SetSegments()` treats supplied spans as proportional weights and normalizes them
so the segments fill the current domain exactly. The normalized span values then
become the authoritative segment extents.

For N segments there are N-1 internal boundaries. Moving boundary `i` changes
only segment `i` and segment `i + 1`; all other boundaries and the fixed domain
remain unchanged.

```cpp
UiRangeSegments lod;
lod.SetRange(0, 100)
   .SetStep(1)
   .SetSegmentCount(4);

lod.SetBoundaryValue(0, 18);
lod.SetBoundaryValue(1, 48);
lod.SetBoundaryValue(2, 76);
```

For threshold-oriented callers, the complete boundary vector can be applied and
read directly:

```cpp
Vector<double> thresholds;
thresholds << 18 << 48 << 76;
lod.SetBoundaryValues(thresholds); // derives four contiguous segments
Vector<double> current = lod.GetBoundaryValues();
```

The domain can be any finite scalar range; it is not intrinsically percentage
based. A `0..1`, `-5..5`, frame-number or distance domain uses the same API.
`ValueDisplay::Percent` only changes presentation labels. `ValueDisplay::Domain`
shows the actual scalar values.

## Structure editing

`SetSegmentCount(n)` creates `n` equal spans. `SplitSegment()` inserts a new
boundary inside one existing segment while preserving the total. `RemoveSegment()`
merges the removed span into a neighbour so the fixed domain remains covered.

`SetMinimumSegmentSpan()` prevents a user boundary from collapsing either
adjacent segment below the requested scalar extent. `SetStep()` snaps boundary
editing in domain units.

## Direction and reverse presentation

The same semantic model supports horizontal and vertical presentation through
`SetDirection(UiDirection::H|V)`.

`SetReverse(true)` reverses only the visual projection. Scalar ordering, segment
indexes, labels, payloads and returned boundaries are unchanged. This is useful
for presentations such as LOD where a high/full-size value may be shown on the
left and a low/micro value on the right.

Resize, DPI changes, direction changes and reverse presentation never mutate the
semantic scalar values.

## UiGraph LOD authoring use

In the UiGraph Presentation Studio, `UiRangeSegments` is a **threshold editor**.
It must not own or modify preview-camera size.

A Studio preview keeps its current projected size while the segment boundaries
move. If a threshold crosses that fixed projected size, the preview changes its
actual Normal/LOD1/LOD2/LOD3 policy; the specimen itself does not jump to a new
camera scale.

The control's outer endpoints remain fixed during direct boundary dragging by
design. A host that needs editable LOD Min/Max should expose explicit Min/Max
fields and call `SetRange(min, max)`; this changes the scalar editing domain, not
the preview camera.

For UiGraph specifically, keep these concepts independent:

1. preview/camera size;
2. LOD transition thresholds edited by `UiRangeSegments`;
3. per-LOD feature policy;
4. retained node/template layout.

See `UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md` and `08_UIGRAPH_GUIDE.md` for the
production retained-layout and Studio V4 direction.

## Colour policy

Each segment can provide an explicit colour. `Null` uses the control palette.
The style owns up to eight deterministic series colours; additional segments
cycle through deterministic lighter/darker variants rather than random colours.

`PaletteMode::Series` uses the palette as discrete series colours.
`PaletteMode::Gradient` samples deterministic colours between the authored
palette anchors across the segment sequence. This remains a segmented range
control: it does not edit continuous gradient stops or make colour itself the
semantic value. A true colour-gradient editor should remain a separate control.

Semantic roles follow the same family policy as `UiChartRing`: Standard keeps a
multi-colour series; Subtle, Accent and Alert produce role-derived tonal series.
An explicit segment colour always wins.

## Interaction and visuals

Internal boundaries use a compact circular thumb centred on the divider, with a
small centre dot. Hover/active boundaries receive a stronger visual treatment.
Hovering a segment lightly emphasizes that segment; clicking it establishes the
selected segment independently of the active boundary.

Optional presentation includes:

- segment labels inside available segment space;
- boundary value badges;
- outer endpoint values;
- divider lines;
- selected-segment frame;
- horizontal or vertical layout;
- reversed visual direction.

`WhenChanging` fires during live user boundary movement and `WhenAction` fires
on committed movement. Programmatic setters do not emit user-action callbacks.
`WhenSegmentSelect` and `WhenBoundarySelect` report user selection changes.

## Value binding

`GetData()` exports a `ValueArray`. Each entry is a `ValueMap` containing:

- `span`;
- `label`;
- optional `color`;
- optional `data`.

`SetData()` accepts the same form. This keeps labels and application payloads
round-trippable without introducing a second internal model.

## Rendering/performance contract

The control deliberately follows the current `07_UI_DRAWING_GUIDE.md` and the
performance lessons captured by `06_UI_SCALE_AND_LOD_GUIDE.md` / UiGraph:

- coloured segment bodies are cheap direct `Draw` rectangles;
- rounded outer ends and boundary thumbs use native `DrawEllipse` primitives;
- divider lines and text are direct `Draw`;
- no full-control `BufferPainter` is rebuilt during drag;
- no timer or animation loop is owned by the control;
- static/idle state is genuinely idle;
- pixel geometry is a projection of semantic scalar values, never a second
  source of truth.

The control therefore does not use a raster cache merely for uniformity. If a
future visual option introduces genuinely expensive stable AA composition, it
should be evaluated independently under the shared raster-cache rules rather
than changing the baseline drag path.

## Demo and focused validation

Canonical demo:

```text
examples/UiRangeSegmentsDemo
```

The demo follows the repository Inspector / Theme Overrides / Data / Code shell.
Its Data page edits the same active `UiRangeSegment` collection that drives the
preview, while live boundary dragging writes normalized spans back to that view.

Focused tests:

```text
Utilities/UiRangeSegmentsRunTests
```

Expected focused summary after Windows validation:

```text
UIRANGESEGMENTS_SUMMARY checks=60 failed=0
```

The focused suite covers proportional normalization, arbitrary domains, adjacent
boundary redistribution, step/minimum-span constraints, boundary-array input,
split/remove operations, data round-trip, horizontal/vertical/reversed geometry,
palette interpolation, theme/custom-style lifecycle, selection and paint smoke.
