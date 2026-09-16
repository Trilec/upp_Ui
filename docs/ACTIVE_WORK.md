# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; do not force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `7ad86d003b5974ad7c4fe1abe8b5985c95a22bc8`
TASK: **UIGRAPH-PRESENTATION-STUDIO-V3-FIX01 — selector propagation + representative LOD sampling**
BRANCH: `main`
STATUS: **SOURCE FIX PUBLISHED — WINDOWS DEBUG / VISUAL VALIDATION PENDING**
PUBLISHED: commit containing this recovery record
NEXT ACTION: focused Debug build/launch of `examples/UiGraphDesignMatrix`; confirm selector propagation, LOD2/LOD3 distinction and schema-v2 import/export, then finish remote branch cleanup.

## ACCEPTED FOUNDATION

Preserve Eddie's production presentation/execution architecture:
- one prepared `UiGraphNodePresentation` owns production layout/visibility/capacity;
- bounded `WhenResolveNodePresentation` request seam;
- rich paint consumes prepared regions;
- live camera projection reuses prepared regions;
- Micro preparation skips rich presentation callbacks and retains the 10k fast path;
- one world spatial authority and current Micro/Rich execution ownership remain unchanged.

The Presentation Studio remains an authoring/diagnostic example. It does not silently make Studio thresholds/features authoritative inside production `UiNodeGraph`.

## PRESENTATION STUDIO V3

Reference: approved `uigraph_presentation_studio_matrix_v3.html` concept.

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

Feature vocabulary:
`TLE SUB ICO BGE STA PRG DES MED FLD CONT ACT PLAB PSUM FOOT`.
Green = requested+visible; red = disabled; amber = requested but production/capacity suppresses it.

LOD editor:
- `UiRangeSegments` domain 32..300 final-pixel resolution;
- defaults Normal 160px / LOD1 80px / LOD2 48px;
- global per-template thresholds plus explicit per-shape overrides;
- cell metadata always reports actual production `UiGraphPresentationLevel`.

Persistence:
- `StudioDocument` schema version 2;
- Copy JSON / Export / Import;
- stores all seven templates, all eight shapes, four LOD feature masks and threshold overrides.

## FIX01 — ROOT CAUSE AND CHANGES

Gary's first Windows smoke found selector text changed while the matrix did not.
Root cause: `UiDropdown` commits selection through `WhenSelect`; the Studio incorrectly subscribed to inherited `WhenAction`, which `UiDropdown` does not fire for selection commits.

Fixed:
- Template, Shape, Authored Size and Ports now subscribe to `UiDropdown::WhenSelect(int)`;
- `ConfigureFromDocument()` uses `SetDataSilently()` so document restore/import does not recursively fire selector callbacks;
- Debug startup runs an internal four-part selector->matrix projection smoke:
  1. template selection changes matrix template state;
  2. authored-size selection reaches matrix cells;
  3. port selection rebuilds 4x4 topology;
  4. shape selection reaches matrix selection/filter state;
- expected Debug log on success: `UIGRAPH_STUDIO_SELECTOR_SMOKE checks=4 failed=0`.

Gary's local CLANG fixes are folded into source:
- local frame helper renamed `StudioDrawFrame` to avoid ambiguity;
- two conditional text expressions construct `String` explicitly.

LOD representative sampling is also corrected:
- LOD2 samples close to the upper edge of its authored band so retained detail is easier to judge;
- LOD3 samples at <=36px when the band permits it, so the default row actually crosses production's current Micro threshold instead of frequently showing a second LOD2 specimen.

This changes only Studio sampling, not production LOD thresholds or renderer semantics.
If LOD2 still lacks useful identity after this correction, treat that as design evidence for a separate production-policy decision rather than faking it in the Studio.

## SOURCE ORGANISATION

Studio example remains split into normal `.h/.cpp` units:
- `PresentationStudioPolicy.*`
- `PresentationStudioCell.*`
- `PresentationStudio.*`
- small `main.cpp`.

Do not use this as a reason to refactor UiNodeGraph's accepted single-TU `.inc` organisation during presentation closure.

## FAST WINDOWS GATE

DEBUG ONLY.

1. `git fetch origin --prune && git checkout main && git pull --ff-only`.
2. Build `examples/UiGraphDesignMatrix`.
3. Launch and leave running for Curt.
4. Confirm Debug log: `UIGRAPH_STUDIO_SELECTOR_SMOKE checks=4 failed=0`.
5. Smoke:
   - Template visibly changes matrix (Minimal -> Operator);
   - Shape filter visibly changes shown matrix column;
   - Authored Size visibly rebuilds specimens;
   - Ports visibly changes 1x1 -> 4x4 topology;
   - LOD2 and LOD3 are not accidentally the same production level at default Reference sampling;
   - light/dark;
   - drag all three LOD boundaries;
   - feature chip green/red/amber interaction;
   - Copy JSON has schema_version 2 and 14 feature keys;
   - Export then Import restores state.
6. `git diff --check` PASS.

No Release, 10k benchmark or broad suite unless this focused gate exposes a production/API problem.

## REMOTE BRANCH CLEANUP

Gary reduced remote branches from 20 to 10 but retained nine supervisor branches because simple ancestry checks reported commits not reachable from main.
Do not keep them merely because squash/merge history makes ancestry non-linear.
For each retained branch compare its actual tree/diff against current `origin/main`; preserve only genuinely unpublished content.

Retained list to resolve:
- `supervisor/uigraph-arrow-published`
- `supervisor/uigraph-arrow-tee-square`
- `supervisor/uigraph-design-matrix-rectangle-proof6`
- `supervisor/uigraph-ellipse-published`
- `supervisor/uigraph-ellipse-raster-fix`
- `supervisor/uigraph-presentation-studio-published`
- `supervisor/uigraph-presentation-studio-v3`
- `supervisor/uigraph-presentation-studio-v3-main`
- `supervisor/uigraph-ring-test-small`

Desired steady state: `origin/main` plus only a branch containing genuinely unfinished/unpublished work. Delete merged/duplicated proof, published, validation and temporary branches after verifying their content is already represented on main.
