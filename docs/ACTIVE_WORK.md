# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; do not force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `3af1ddbeb51134c1a47b75596424a4cf665667fc`
TASK: **UIGRAPH-NODE-LAYOUT-CORE-02 — retained section-aware node layout**
BRANCH: `main`
TOUCHED: `Ui/UiGraph/UiNodeGraph.h`, `UiNodeGraphPresentation.inc`, `UiNodeGraphProjection.inc`, `Utilities/UiGraphRenderTests/PresentationLayout.h`, and UiGraph architecture/recovery docs.
STATUS: **SOURCE IMPLEMENTED + CANONICAL DOCS RECONCILED — WINDOWS DEBUG VALIDATION PENDING**
PUBLISHED: implementation `98d92187e288fc076655207a30da0fe64b083eb9`; documentation follows on `main`.
VALIDATION: source review/tests added; Windows CLANG Debug gate not yet reported for CORE-02.
NEXT ACTION: focused Debug `UiGraphRenderTests` + `UiGraphDesignMatrix`; then implement the shared C++ template/slot layer and Presentation Studio V4 against the retained layout authority.

## READ FIRST

1. `docs/ACTIVE_WORK.md`
2. `docs/08_UIGRAPH_GUIDE.md`
3. `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md`
4. `docs/UIGRAPH_PRESENTATION_AUDIT.md`
5. `docs/06_UI_SCALE_AND_LOD_GUIDE.md`

Current source and these canonical docs override remembered chat/old handovers.

## CORE DECISION

Node layout is part of retained node geometry, not a second runtime subsystem.

`NodeGeometry.presentation` is the retained node-layout result/cache:
- exact rich geometry preparation owns it;
- paint and attached controls consume it directly;
- compatible live pan/zoom projects the same retained rectangles;
- Micro preparation skips rich layout/resolver work;
- no second per-node layout cache;
- no runtime JSON layout compiler;
- no `Ctrl` layout tree per ordinary node.

The internal `NodeLayout` cursor is only cheap rectangle arithmetic during exact
preparation. It is not another retained authority.

## IMPLEMENTED RETAINED STRUCTURE

`UiGraphNodePresentation` now retains:

- `safe`
- `header`
- `body`
  - `body_left`
  - `body_main`
  - `body_right`
- `footer`
- `overlay`
- `center`
- existing leaf slots and physical port-label lanes.

`overlay` / `center` intentionally overlap `body_main` as composition regions.

Body modes:
`Stack`, `Centered`, `Media`, `KeyValue`, `Fields`, `PortRows`, `FlowTags`.

Left/right labelled port lanes can remain full-height or be body-only. Body-only
lanes share `body_left` / `body_right`, preserving Header/Footer width. Top/bottom
lane semantics are unchanged in this tranche.

## PERFORMANCE CONTRACT

Retained layout is the cache; do not calculate it and then duplicate it elsewhere.

Compatible camera motion projects the retained result. Preferred incremental rule:
- retain everything useful;
- invalidate narrowly where practical;
- replay a small section when needed;
- project cached layout while structure/LOD/capacity remain compatible;
- exact rebuild only when required.

Do **not** build a fine-grained dependency graph yet. Per-section dirty/revision
updates are planned direction, not current implementation evidence.

## DESIGN COVERAGE

The retained regions/body modes are intended to cover:
- media card: Header title + Media BodyMain + Overlay state icons + wrapping tags;
- central controller/hub: Center icon/title/subtitle with external focus/ring decoration;
- summary/key-value cards: structured rows + Footer summary;
- status/process cards: subtitle/title + status/progress;
- parameter/operator nodes: fields/controls plus side lanes;
- Blueprint-style port catalogues: labelled typed port rows without one Ctrl per row.

Edge colour/activity remains edge styling. Future active-flow emphasis should reuse
prepared edge routes, not enter node layout.

## NEXT PRODUCTION LAYER

Add a compact shared **C++ template/slot description** mapping named features into
the retained regions/body modes.

Requirements:
- normal UMK/CLANG compilation;
- one immutable/shared template definition for many nodes;
- no per-node copied template tree;
- feature placement is template-owned (Icon may be Header/Center/Overlay etc.);
- eventual Stable/Reflow section policy;
- arbitrary slot placement only through this production mechanism;
- JSON optional for Studio/session interchange or generated C++ only.

Starting presentation intents remain:
Minimal, Identity, Summary, Status, Media, Parameter, Operator.
These are not LOD levels.

## STUDIO V4 DIRECTION

Replace the current wide matrix authoring model with a focused selected-node/template
workbench after the core gate passes:
- one selected shape/template;
- four persistent Normal / LOD1 / LOD2 / LOD3 previews;
- preview cameras independent from LOD thresholds;
- `UiRangeSegments` edits transition thresholds only;
- adjustable Min/Max threshold domain;
- feature policy: Inherit / Force On / Force Off subject to capacity;
- right-side builder edits region/slot placement, body mode, alignment and later
  Stable/Reflow behaviour;
- explicit Reset Cameras;
- production C++ template output;
- no demo-only parallel allocator.

## TEST COVERAGE ADDED

`Utilities/UiGraphRenderTests/PresentationLayout.h` checks:
- `KeyValue` body-mode retention;
- BodyLeft/Main/Right allocation;
- body-only left/right labelled lanes;
- Header width not consumed by body-only lanes;
- Overlay/Center mapping to BodyMain;
- compatible middle-pan projection of new retained regions without layout rebuild.

Existing shape/profile/capacity/text-line-box/Micro tests remain authoritative.

## WINDOWS DEBUG GATE

DEBUG ONLY:
1. fetch/pull current `main`;
2. build/run `UiGraphRenderTests`;
3. build `examples/UiGraphDesignMatrix`;
4. confirm `UIGRAPH_STUDIO_SELECTOR_SMOKE checks=4 failed=0`;
5. launch/leave Studio running for Curt;
6. `git diff --check` PASS.

No Release, broad aggregate suite or 10k benchmark unless the focused gate exposes
a shared compile/API/performance problem.

## BRANCH HYGIENE

Do not create proof/final/published branches for this work.
Steady state is `main` plus only genuinely unfinished/unpublished branches after
content-equivalence review. Continue deleting obsolete supervisor branches.
