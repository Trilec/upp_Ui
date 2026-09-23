# ACTIVE WORK

Remote main is authoritative. Refresh before editing/publishing; never force-push.
This file is a recovery pointer, not an implementation history. Keep <=100 lines.

## Release hygiene — UI-RC-HYGIENE-01

BASE: `8114269abd91cc33569f68117bef4fd4d537897a` / main.
TASK: release-readiness audit and coherent source/documentation cleanup.
TOUCHED: `Ui/UiRangeSegments*.{h,cpp}`; `Ui/UiDirectContentHost.{h,cpp}`;
`Utilities/UiReleaseSmoke/{main.cpp,UiReleaseSmoke.upp}`; this file.
STATUS: PARTIAL overall; this source slice IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.
PUBLISHED: commit containing this entry; recover with `git log -1 -- Utilities/UiReleaseSmoke/main.cpp`.
VALIDATION: complete touched originals reconstructed with matching Git blob hashes;
full diff and git diff --check PASS. Extracted normalization algorithm compiled with
Clang and passed 20,002 deterministic cases; this is not a U++/Windows test result.
New native regression gate and existing focused suite have not been run here.
NEXT ACTION: continue catalogue/docs/demo coverage and release tooling. Gary's final
surgical task must use latest main and the checked-in GitHubOut.var. Build/run
UiReleaseSmoke plus UiRangeSegmentsRunTests; compile the RangeSegments demo.

Source changes: distinct inherited Accent/Subtle/Alert ramps; finite input guards;
order-independent minimum-span normalization; callback lifetime guards and capture
cancellation; linear range projection; bounded antialiased track/thumb rasters;
borrowed-child lifetime/parent checks. No persistence-schema or Graph layout change.
UiReleaseSmoke checks actual pixels/cache reuse as well as model/lifetime behavior.
Do not infer all-controls audit completion or visual acceptance from this smoke.

## Parallel reusable Theme fix — UI-TAB-THEME-01

BASE: `11783c34ade07ceb8bf03b760eb58b314b2edd59` / main.
TASK: preserve Theme-owned active UiTab caps and strip fills, including explicit None.
TOUCHED: `Ui/UiTab.cpp`; `Utilities/UiTabThemePaintTest/{main.cpp,UiTabThemePaintTest.upp}`.
STATUS: IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.
PUBLISHED: `8114269abd91cc33569f68117bef4fd4d537897a`.
VALIDATION: source/API/diff review PASS; native pixel test not compiled/run here.
NEXT ACTION: existing Designer gate: UiTabThemePaintTest + ThemeStudioRoleTest,
canonical Designer compile/open; Curt owns visual checks. No broad Graph matrix.

## Parallel UiGraph — UIGRAPH-NODE-WORKSPACE-03E2

BASE: `4c997824f741f1555269f95aec3462e7674045e0` / main.
TASK: Content footprint underlay in Overlay diagram; explicit Overlay/fit summaries.
TOUCHED: `examples/UiGraphComponentStudio/{WorkspaceViews.h,WorkspaceViews.cpp,WorkspaceOverlayTests.cpp}`;
`docs/UIGRAPH_OVERLAY_CONTRACT.md`.
STATUS: accumulated 03E1/03E2 IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.
PUBLISHED: recover with `git log -1 -- examples/UiGraphComponentStudio/WorkspaceOverlayTests.cpp`.
VALIDATION: pinned full source/diff review PASS; newer native tests not executed here.
NEXT ACTION: existing `scripts/ValidateUiGraphWorkspace.ps1` Debug gate with required
ancestor and -Launch; retain positive counts/all prior summaries plus OVERLAY.
Read the overlay contract for physical/manual checks. Do not restart old matrix work.

The preceding 03D gate remains reported PASS at
`57e8d38167cde7cee2bc62b0093979af86ca91ca` (U++18468 / CLANGx64).
Reported suites: Render 9/0; EllipseBands 16/0; Workspace 44/0; native View 45/0;
Band UI 23/0; startup 5/0; WorkspaceComponent 24/0; Component 21/0;
ExecutionPath 8/0; Presentation 87/0; evidence-reader 12/0. Generated C++ compiled.
This earlier PASS does NOT validate source added afterwards. Full evidence and
historical executable identity remain in Git at the BASE commit above.

Preserve one NodeGeometry.presentation authority, immutable camera baseline,
independent Content/Overlay allocation, prepared native components/Micro budgets,
and 03C projection/performance work. Active authoring app: UiGraphComponentStudio.
General graph demos/tests remain supported; DesignMatrix is retired.
Open: held-button Escape/physical DND, general diagram inventory, proposed V8
post-port zones, threshold undo and compact lifecycle. No closure claimed here.
