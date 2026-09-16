# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; do not force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `04ba9ec9d3e9b53080b92a8578a4564463cf995b`
TASK: **UIGRAPH-PRESENTATION-STUDIO-V3 — port approved HTML matrix into U++**
BRANCH: `supervisor/uigraph-presentation-studio-v3-main`
STATUS: **SOURCE COMPLETE — DEBUG VALIDATION PENDING**
PUBLISHED: pending merge to `main`
NEXT ACTION: focused Debug build/launch of `examples/UiGraphDesignMatrix`, then Curt visual acceptance.

## ACCEPTED FOUNDATION

Preserve Eddie's production presentation/execution architecture:
- one prepared `UiGraphNodePresentation` owns production layout/visibility/capacity;
- bounded `WhenResolveNodePresentation` request seam;
- rich paint consumes prepared regions;
- live camera projection reuses prepared regions;
- Micro preparation skips rich presentation callbacks and retains the 10k fast path;
- one world spatial authority and current Micro/Rich execution ownership remain unchanged.

This Studio checkpoint does **not** modify production UiGraph source or silently make proposed Studio policy authoritative inside `UiNodeGraph`.

## IMPLEMENTED VISUAL CONTRACT

The reference is the approved `uigraph_presentation_studio_matrix_v3.html` concept.
The U++ implementation follows its compact engineering-tool structure using repository controls and the production `UiNodeGraph` renderer.

Shell:
- `UiTitleCard` header: `UiGraph / Presentation Studio` / `Design · Compare · Tune · Export`;
- polished light/dark theme support;
- template, shape, authored-size and port-topology selectors via `UiDropdown`;
- `UiRangeSegments` as the real LOD resolution editor;
- compact feature legend, fixed left LOD rail and horizontally scrollable shape matrix;
- Copy JSON / Import / Export actions.

Built-in templates:
1. Minimal
2. Identity
3. Summary
4. Status
5. Media
6. Parameter
7. Operator

These names describe information intent rather than amount of detail; LOD remains a separate axis.

Shapes:
- Rectangle, Ellipse, Diamond, Triangle, Hexagon, Cloud, Document, Database.

Authored-size scenarios:
- Compact 220x145
- Reference 260x170
- Spacious 320x210

Port scenarios:
- None
- 1 IN / 1 OUT
- 3 IN / 2 OUT
- 4 IN / 4 OUT

Cells use real semantic ports plus real production edges from helper nodes so connectivity is rendered rather than implied.

## FEATURE VOCABULARY

`StudioFeature` is a stable editor-side bitmask vocabulary:
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

Feature chips are interactive per template + shape + LOD:
- green = requested and currently visible through production presentation;
- red = disabled by authored policy;
- amber = requested but suppressed by current production LOD/capacity.

Studio-specific examples such as status/progress/fields/actions are painted only inside production-prepared content slots; they do not create a second node renderer or new application semantics in `UiGraphModel`.

## LOD RESOLUTION EDITOR

`UiRangeSegments` domain: 32..300 final-pixel resolution, step 1, minimum segment span 8.
Default boundaries: Normal 160px, LOD1 80px, LOD2 48px.
The reversed visual axis matches the HTML: 300px on the left -> 32px on the right.

Thresholds are authored:
- globally per template; or
- as an explicit per-shape override selected from the shape headers/filter.

The matrix rows sample the midpoint of each authored range while still reporting the **actual** production `UiGraphPresentationLevel`, so disagreements between proposed policy and production remain visible.

## PERSISTENCE

`StudioDocument` uses `schema_version: 2` because the catalogue/feature vocabulary supersedes the earlier six-template editor schema.

JSON stores:
- active template;
- authored-size and port preset;
- all seven built-in templates;
- global thresholds;
- per-shape threshold overrides;
- eight shapes x four LOD requested feature masks using human-readable keys.

Actions:
- Copy JSON to clipboard;
- Export to JSON;
- Import + validate catalogue/schema before applying.

## SOURCE ORGANISATION

The previous 1289-line example monolith is split into ordinary example `.h/.cpp` units:
- `PresentationStudioPolicy.*`
- `PresentationStudioCell.*`
- `PresentationStudio.*`
- small `main.cpp`

This is intentionally conventional example organisation; no change is made to UiNodeGraph's accepted single-TU `.inc` implementation architecture.

## DELIBERATE HTML/U++ DIFFERENCE

The HTML keeps shape-column headers CSS-sticky during vertical matrix scrolling. The first U++ checkpoint keeps the left LOD rail fixed and the shape header row inside the scrolled sheet. This avoids adding bespoke z-order/overlay machinery before the first Windows visual pass. Revisit only if Curt finds vertical header loss materially harms comparison.

## FAST WINDOWS GATE

DEBUG ONLY.

1. Build `examples/UiGraphDesignMatrix`.
2. Launch it and leave it running for Curt.
3. Smoke only:
   - light/dark theme;
   - switch all seven templates;
   - drag all three `UiRangeSegments` boundaries;
   - select a shape header and verify per-shape threshold editing / Use global;
   - toggle at least one feature chip and observe green/red/amber state;
   - switch 1x1 -> 4x4 ports;
   - switch Compact / Reference / Spacious;
   - Copy JSON contains `schema_version: 2` and the 14 feature names;
   - Export then Import restores state.
4. `git diff --check` PASS.

Production graph source is untouched in this checkpoint, so no Release build, 10k benchmark or broad aggregate suite is required unless the demo compile exposes a real production/API problem.
