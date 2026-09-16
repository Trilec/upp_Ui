# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; do not force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `3fd8a5cd171a5ffbb0bb4ec8b23e6c5a4b8e6a23`
TASK: **UIGRAPH-NODE-LAYOUT-CORE-01 — unify presentation allocation inside node geometry preparation**
BRANCH: `main`
STATUS: **SOURCE COMPLETE — WINDOWS DEBUG VALIDATION PENDING**
PUBLISHED: `09c8de77cc1e0a60e8504fb33b65009c3d74bb8c`
NEXT ACTION: focused Debug presentation tests + Design Matrix build, then implement Studio V4 against this geometry-owned layout path.

## DECISION

Do NOT introduce a public `UiGraphLayout` subsystem or a second layout cache.

The existing architecture already has the correct cache owner:
- `NodeGeometry` is the retained prepared geometry record;
- `NodeGeometry.presentation` stores the finished `UiGraphNodePresentation` rectangles;
- exact preparation computes them;
- compatible camera projection reuses/projects them;
- reusable middle-pan performs no presentation/layout preparation;
- Micro preparation skips rich presentation allocation entirely.

Node layout therefore belongs inside the existing node-geometry preparation path.

## INTERNAL NODE LAYOUT

`UiNodeGraphPresentation.inc` now contains one tiny internal `NodeLayout` rectangle cursor.
It is an implementation helper only, not a public graph/layout framework.

Properties:
- stack/value object;
- no heap allocation;
- no Ctrl children;
- no virtual dispatch;
- no independent cache;
- no Paint-time work;
- only integer rectangle arithmetic;
- nested composition by constructing another `NodeLayout` over an allocated slot.

Core operations are intentionally minimal:
- `Take(amount, side [, gap])`
- `Remaining()`
- static `Center(rect, size)`
- `fits` state

This replaces the private ad-hoc `take` lambda and centralises the rectangle allocation mechanism without creating another authority.

## CURRENT PRODUCTION INTEGRATION

The same internal `NodeLayout` now allocates:
- port-label lanes;
- badge/footer reservations;
- header region;
- nested header icon lane;
- centred title/subtitle group;
- control slot;
- description slot;
- remaining media/body region.

The prepared result is still written directly into `NodeGeometry.presentation`.
No cache/invalidation semantics change.

No changes to:
- spatial ownership;
- routes/edges;
- Micro/Rich backend ownership;
- production LOD thresholds;
- current LOD visibility policy;
- model semantics;
- host request callback authority.

## WHY THIS MATTERS

This creates the low-level execution primitive needed for future presentation templates without turning graph nodes into Ctrl layout trees.

Future template authoring should define WHAT regions/slots a node wants and how they are composed; exact node geometry preparation should execute that definition through this same internal rectangle allocator and cache only the finished `UiGraphNodePresentation` result.

Do not add a second per-node layout cache. If template definitions later need preprocessing, cache/compile the TEMPLATE definition once, never duplicate prepared per-node geometry.

## STUDIO V4 DIRECTION

After validation, rebuild the Presentation Studio authoring model around four independent concepts:

1. persistent specimen/camera size;
2. LOD transition thresholds;
3. per-LOD feature policy;
4. template/node layout.

Moving `UiRangeSegments` boundaries must NOT resize the specimen cameras.
Feature policy should support Inherit / Force On / Force Off, subject only to real shape capacity.

Template editing should remain constrained and fast rather than becoming a full general UI Designer. Likely concepts:
- nested sections/regions;
- Top / Bottom / Left / Right / Fill / Center;
- inset/gap;
- named slots such as Title, Subtitle, Icon, Badge, Status, Progress, Description, Media, Fields, Controls, Actions, PortSummary, Footer;
- Subtitle may be positioned as an overline/kicker by template layout rather than requiring another feature enum.

Built-in starting templates remain:
Minimal, Identity, Summary, Status, Media, Parameter, Operator.

Production definitions should compile as ordinary C++ with UMK/CLANG. JSON may remain a Studio/session/export representation but must not become a runtime layout compiler requirement.

## WINDOWS DEBUG GATE

DEBUG ONLY.

1. Fetch/pull current `main`.
2. Build and run `UiGraphRenderTests`.
   Existing prepared-presentation checks must remain PASS, especially:
   - region containment/non-overlap;
   - measured Windows text line boxes;
   - Normal enlargement stability;
   - all 8 canonical shapes / 3 profiles / real projected sizes;
   - reusable middle-pan performs no resolver/layout work;
   - batched invalidation refreshes once;
   - Micro skips rich presentation callbacks.
3. Build `examples/UiGraphDesignMatrix`.
4. Confirm Debug startup still reports:
   `UIGRAPH_STUDIO_SELECTOR_SMOKE checks=4 failed=0`
5. Launch and leave the Studio running for Curt.
6. `git diff --check` PASS.

No Release build.
No 10k benchmark unless a regression is observed.
No Studio V4 implementation in this validation pass.

## BRANCH HYGIENE

Do not create proof/final/published branches for this work.
Continue deleting obsolete supervisor branches after content-equivalence review.
Steady state should be `main` plus only genuinely unfinished/unpublished work.
