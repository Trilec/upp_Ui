# ACTIVE WORK

Remote main is authoritative. Refresh before editing/publishing; never force-push.
Keep this recovery index <=100 lines. Contracts belong in the nine reader guides.

## Release hygiene — UI-RC-HYGIENE-01

BASE: `297beabdea87e3cc2c32282968e262ef68392abb` / main.
TASK: all-controls release-readiness implementation and evidence coverage.
TOUCHED: root reader docs; nine canonical guides; `Ui/UiVersion.h`, `Ui.h`, `Ui.upp`;
`UiRangeSegmentsPaint.cpp`; two `Ui/srcdoc.tpp` references;
`scripts/ValidateUiRelease.ps1`; `tests/ui_release_inventory.json`; this file.
STATUS: PARTIAL overall; documentation/version/validation infrastructure implemented.
PUBLISHED: containing commit; recover with `git log -1 -- scripts/ValidateUiRelease.ps1`.
VALIDATION: complete source originals matched Git blobs; reviewed source/diff;
standalone version-header compile/run and local manifest/guide-link checks PASS.
PowerShell/native U++/Windows not executed here. No whole-library acceptance claim.
NEXT ACTION: continue remaining per-control source/demo/generated-code gates from the
inventory. Gary runs the small Surgical profile on latest main; not the Full profile.

UiVersion.h owns Ui `1.0.0-rc.1` as an UNRELEASED candidate identity, not certification.
Sibling package versions and persistence schemas are unchanged. The inventory lists
50 concrete controls and distinct source, demo, code-generation and platform evidence.
It does not mark unreviewed controls PASS. Historical tests/demos are retained until
a replacement proves the same useful coverage. Remote branch inventory is main only.

Published source fix: `297beabdea87e3cc2c32282968e262ef68392abb`.
Paths: `Ui/UiRangeSegments*.{h,cpp}`, `Ui/UiDirectContentHost.{h,cpp}`,
`Utilities/UiReleaseSmoke`. Distinct role ramps, finite inputs, minimum-span
normalization, callback/capture safety, linear projection, bounded AA/cache,
borrowed-child lifetime/parent guards. Extracted normalization: 20,002/0 via Clang.
Native focused smoke: expected 58 checks, zero failures; not yet run on Windows.
The additional percent-format arithmetic fix divides before multiplying by 100.

Documentation: nine guides plus this file. RangeSegments is in Controls; UiDoc is
in Models; generic scale/LOD is in Drawing; Graph contracts and open limitations
are in `09_UIGRAPH_DEVELOPMENT.md`. Superseded checkpoint pages remain in Git history.
Do not resurrect their obsolete build tasks or convert old PASS into current PASS.

## Parallel reusable Theme fix — UI-TAB-THEME-01

BASE: `11783c34ade07ceb8bf03b760eb58b314b2edd59` / main.
TASK: Theme-owned active UiTab caps and strip fills, including explicit None.
TOUCHED: `Ui/UiTab.cpp`; `Utilities/UiTabThemePaintTest`.
STATUS: IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.
PUBLISHED: `8114269abd91cc33569f68117bef4fd4d537897a`.
VALIDATION: source/API/diff review PASS; native pixel test not compiled/run here.
NEXT ACTION: existing Designer gate: UiTabThemePaintTest + ThemeStudioRoleTest,
canonical Designer compile/open; Curt owns visual checks. No broad Graph matrix.

## Parallel UiGraph — UIGRAPH-NODE-WORKSPACE-03E2

BASE: `4c997824f741f1555269f95aec3462e7674045e0` / main.
TASK: Content footprint underlay in Overlay diagram; explicit Overlay/fit summaries.
TOUCHED: `examples/UiGraphComponentStudio/{WorkspaceViews.h,WorkspaceViews.cpp,WorkspaceOverlayTests.cpp}`.
STATUS: accumulated 03E1/03E2 IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.
PUBLISHED: recover with `git log -1 -- examples/UiGraphComponentStudio/WorkspaceOverlayTests.cpp`.
VALIDATION: pinned full source/diff review PASS; newer native tests not executed here.
NEXT ACTION: existing `scripts/ValidateUiGraphWorkspace.ps1` Debug gate with required
ancestor and -Launch; require positive existing summaries AND OVERLAY summary.
See Graph Development for manual checks. Do not restart old matrix work.

The preceding 03D gate remains reported PASS at
`57e8d38167cde7cee2bc62b0093979af86ca91ca` (U++18468 / CLANGx64).
Reported suites: Render 9/0; EllipseBands 16/0; Workspace 44/0; native View 45/0;
Band UI 23/0; startup 5/0; WorkspaceComponent 24/0; Component 21/0;
ExecutionPath 8/0; Presentation 87/0; evidence-reader 12/0. Generated C++ compiled.
This earlier PASS does NOT validate source added afterwards. Full earlier evidence
and historical executable identity remain in Git at `8114269abd91cc33569f68117bef4fd4d537897a`.
Preserve one NodeGeometry.presentation authority, immutable camera baseline,
independent Content/Overlay, native component/Micro budgets and 03C performance work.
Active authoring app: UiGraphComponentStudio. DesignMatrix is retired.
Open: held-button Escape/physical DND, diagram inventory, full V8 post-port zones,
threshold undo and compact lifecycle. No closure claimed by release hygiene.
