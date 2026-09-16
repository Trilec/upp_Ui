# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; do not force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `dd15265fa75f0e7b33731aa2f071be9059fac8fa`
TASK: **UIGRAPH-PRESENTATION-STUDIO-V3 — approved HTML matrix port**
BRANCH: `main`
STATUS: **PUBLISHED — WINDOWS DEBUG / VISUAL VALIDATION PENDING**
PUBLISHED: `dd15265fa75f0e7b33731aa2f071be9059fac8fa`
NEXT ACTION: one focused Debug build/launch of `examples/UiGraphDesignMatrix`; Curt visually reviews the running Studio.

## ACCEPTED FOUNDATION

Preserve Eddie's production presentation/execution architecture:
- one prepared `UiGraphNodePresentation` owns production layout/visibility/capacity;
- bounded `WhenResolveNodePresentation` request seam;
- rich paint consumes prepared regions;
- live camera projection reuses prepared regions;
- Micro preparation skips rich presentation callbacks and retains the 10k fast path;
- one world spatial authority and current Micro/Rich execution ownership remain unchanged.

The Studio does **not** modify production UiGraph source or silently make proposed Studio policy authoritative inside `UiNodeGraph`.

## PUBLISHED STUDIO

Reference: approved `uigraph_presentation_studio_matrix_v3.html` concept.

Shell / interaction:
- `UiTitleCard`: `UiGraph / Presentation Studio` / `Design · Compare · Tune · Export`;
- polished light/dark mode;
- `UiDropdown` selectors for Template, Shape, Authored size and Ports;
- real `UiRangeSegments` 32..300 final-pixel LOD editor;
- fixed left LOD rail + horizontally scrollable shape matrix;
- Copy JSON / Import / Export.

Templates:
1. Minimal
2. Identity
3. Summary
4. Status
5. Media
6. Parameter
7. Operator

Shapes: Rectangle, Ellipse, Diamond, Triangle, Hexagon, Cloud, Document, Database.

Authored sizes: Compact 220x145, Reference 260x170, Spacious 320x210.
Ports: None, 1 IN / 1 OUT, 3 IN / 2 OUT, 4 IN / 4 OUT.
Cells use real semantic ports and real production edges from helper nodes.

## FEATURE POLICY

`StudioFeature` is an editor-side bitmask:
- `TLE` Title
- `SUB` Subtitle
- `ICO` Icon
- `BGE` Badge
- `STA` Status
- `PRG` Progress
- `DES` Description
- `MED` Media
- `FLD` Fields / parameter rows
- `CONT` Interactive controls
- `ACT` Actions / buttons
- `PLAB` Individual port labels
- `PSUM` Port summary
- `FOOT` Footer

Chip states:
- green = requested and visible through current production presentation;
- red = disabled by authored policy;
- amber = requested but suppressed by current production LOD/capacity.

Status/progress/fields/actions/port-summary examples are drawn only inside production-prepared content slots; they do not create a second node renderer or add app semantics to `UiGraphModel`.

## LOD / PERSISTENCE

`UiRangeSegments` defaults: Normal 160px, LOD1 80px, LOD2 48px, lower bound 32px, upper bound 300px. The visual axis is reversed to match the HTML: 300px left -> 32px right.

Thresholds are global per template or explicit per-shape overrides. Matrix rows sample each authored range while reporting the **actual** production `UiGraphPresentationLevel`, exposing disagreement instead of hiding it.

`StudioDocument` schema version is `2`. JSON stores active template, authored size, port preset, all seven templates, global/per-shape thresholds, and 8 shapes x 4 LOD feature masks with human-readable keys.

## SOURCE ORGANISATION

The old 1289-line example monolith is split into ordinary example units:
- `PresentationStudioPolicy.*`
- `PresentationStudioCell.*`
- `PresentationStudio.*`
- small `main.cpp`

This does not change UiNodeGraph's accepted single-TU `.inc` implementation architecture.

## DELIBERATE FIRST-PASS DIFFERENCE

The HTML keeps shape-column headers CSS-sticky during vertical scrolling. This U++ checkpoint keeps the left LOD rail fixed while the shape header row remains in the scrolled sheet. Revisit only if Curt finds that materially harms comparison; avoid bespoke z-order machinery before visual evidence.

## FAST WINDOWS GATE

DEBUG ONLY.

1. Build `examples/UiGraphDesignMatrix`.
2. Launch it and leave it running for Curt.
3. Smoke:
   - light/dark;
   - all seven templates;
   - drag all three LOD boundaries;
   - shape header -> per-shape threshold override -> Use global;
   - toggle feature chips and observe green/red/amber;
   - 1x1 -> 4x4 ports;
   - Compact / Reference / Spacious;
   - Copy JSON contains `schema_version: 2` and 14 feature keys;
   - Export then Import restores state.
4. `git diff --check` PASS.

No Release build, 10k benchmark or broad aggregate suite unless the demo compile exposes a real production/API problem.
