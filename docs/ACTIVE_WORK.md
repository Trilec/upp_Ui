# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; do not force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `3af1ddbeb51134c1a47b75596424a4cf665667fc`
TASK: **UIGRAPH-NODE-LAYOUT-CORE-02 — retained section-aware node layout**
BRANCH: `main`
STATUS: **SOURCE IMPLEMENTED — WINDOWS DEBUG VALIDATION PENDING**
PUBLISHED: `98d92187e288fc076655207a30da0fe64b083eb9`
NEXT ACTION: focused Debug `UiGraphRenderTests` + `UiGraphDesignMatrix`; then wire Presentation Studio V4 to the retained layout regions rather than the old matrix sampling model.

## CORE DECISION

Node layout is not a second subsystem/cache after geometry.

`NodeGeometry.presentation` is the retained node-layout cache:
- exact node geometry preparation owns it;
- it stores the prepared rectangles consumed by paint/attached controls;
- compatible live pan/zoom projects the same retained rectangles;
- Micro preparation keeps skipping rich presentation/layout work;
- no runtime JSON compiler or Ctrl layout tree is introduced.

The small internal `NodeLayout` cursor remains only an allocation primitive used while exact rich geometry is prepared.

## SECTION-AWARE RETAINED LAYOUT

`UiGraphNodePresentation` now represents an explicit retained layout hierarchy:
- `safe`
- `header`
- `body`
  - `body_left`
  - `body_main`
  - `body_right`
- `footer`
- `overlay`
- `center`
- leaf slots: title / subtitle / icon / badge / description / media / control
- physical port label lanes: left / right / top / bottom

`overlay` and `center` intentionally overlap `body_main`; they are composition regions, not sibling reservations.

## BODY MODES

Added structural body-mode metadata:
- `Stack`
- `Centered`
- `Media`
- `KeyValue`
- `Fields`
- `PortRows`
- `FlowTags`

These modes do not create heavy runtime engines. They describe how a host/template intends to use `body_main`; specialised row/tag/field painters remain bounded inside the prepared region.

This covers the concrete families discussed with Curt:
- media card: title + media body + overlay state icons + wrapping footer/tag content;
- central controller/hub: centred body content + icon/title/subtitle + external state/focus decoration;
- summary/key-value nodes: structured two/three-column rows + footer summary;
- status/process nodes: title/subtitle + progress/status content;
- parameter/operator nodes: body fields/controls plus labelled side lanes;
- Blueprint-style port catalogue nodes: labelled output/input rows without turning every row into a Ctrl.

## PORT-LANE OWNERSHIP

Default behaviour remains backward compatible: labelled left/right lanes reserve full node height.

A presentation request may now bind either labelled side lane to `Body` only:
- `left_port_lane_body_only`
- `right_port_lane_body_only`

Body-only lanes participate in `body_left` / `body_right` and no longer steal width from Header/Footer.
Explicit `body_left_width` / `body_right_width` reservations may be larger than the label demand; the lane shares the reserved body column.

Top/bottom lane semantics are unchanged in this tranche.

## PERFORMANCE CONTRACT

No second per-node cache is added.

Compatible camera motion continues to project prepared geometry rather than relayout. The live-projection path now projects all new retained regions:
- body_left
- body_main
- body_right
- overlay
- center

Therefore enlarging/shrinking within a compatible LOD/visibility interval remains a transform of cached layout, not a new layout pass.

Do not build a fine-grained dependency graph yet. Preferred rule remains:
- retain everything;
- invalidate narrowly where practical;
- replay a small section when necessary;
- rely on camera projection for compatible scaling;
- exact rebuild only when structural/LOD/capacity inputs really change.

## CURRENT LIMITS / NEXT CORE STEP

This tranche establishes the retained section vocabulary and projection contract. It does NOT yet implement arbitrary drag/drop slot reassignment or per-feature Inherit/Force-On/Force-Off.

Next production tranche should add a compact template/slot description that maps named features into these retained regions without adding a general-purpose UI layout tree. That description should compile as normal C++/UMK code. JSON, if retained in Studio, is design/session interchange only.

## TEST COVERAGE ADDED

`UiGraphRenderTests/PresentationLayout.h` adds checks that:
- `KeyValue` body-mode metadata survives preparation;
- BodyLeft/Main/Right are retained;
- body-only labelled left/right lanes share BodyLeft/BodyRight;
- Header keeps more width than BodyMain when side lanes are body-only;
- overlay/center map to BodyMain;
- compatible middle-pan projects all new retained regions without a geometry/layout rebuild.

Existing matrix/shape/capacity/text-line-box tests remain authoritative.

## WINDOWS DEBUG GATE

DEBUG ONLY.

1. Fetch/pull current `main`.
2. Build/run `UiGraphRenderTests`.
3. Existing prepared-presentation tests plus new section-aware checks must PASS.
4. Build `examples/UiGraphDesignMatrix`.
5. Confirm Debug startup still reports:
   `UIGRAPH_STUDIO_SELECTOR_SMOKE checks=4 failed=0`
6. Launch and leave Studio running for Curt.
7. `git diff --check` PASS.

No Release build.
No broad aggregate suite unless this focused gate exposes a shared API/compile problem.
No 10k benchmark unless a measured regression appears.

## STUDIO V4 DIRECTION AFTER GATE

Replace the current matrix authoring model with a focused selected-node/template editor:
- one selected shape/template;
- four persistent Normal / LOD1 / LOD2 / LOD3 previews;
- preview cameras independent from LOD threshold editing;
- `UiRangeSegments` edits transition thresholds only;
- feature policy becomes Inherit / Force On / Force Off subject to real capacity;
- right-side builder edits region/slot placement and body mode;
- Reset Cameras is explicit;
- Min/Max LOD editor range becomes adjustable.

Built-in starting templates remain:
Minimal, Identity, Summary, Status, Media, Parameter, Operator.

## BRANCH / CHECKPOINT HYGIENE

Do not create proof/final/published branches for this work.
Steady state remains `main` plus only genuinely unfinished/unpublished branches after content-equivalence review.
