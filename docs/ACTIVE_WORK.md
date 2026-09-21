# ACTIVE WORK

Remote main is authoritative. Refresh before editing/publishing; never force-push.

BASE: `08f09ba1f1b8a02d24a5f6301acc371ebef307af` / main
TASK: **UIGRAPH-NODE-WORKSPACE-03D3 — bounded production ellipse bands**
TOUCHED: `Ui/UiGraph/{UiNodeGraphPresentation.inc,UiGraphNodeTemplate.h,UiGraphNodeComponent.h,UiGraphNodeComponentPaint.cpp}`; `Utilities/UiGraphRenderTests/{EllipseBands.cpp,main.cpp,UiGraphRenderTests.upp}`; this file; `docs/UIGRAPH_ELLIPSE_BANDS.md`.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for the runtime slice only. Authoring/JSON/export/picking integration is the immediate next checkpoint.
PUBLISHED: commit containing this record; recover with `git log -1 -- docs/UIGRAPH_ELLIPSE_BANDS.md`.
VALIDATION: full pinned dependency slice reviewed; runtime allocator upload matches locally reviewed Git blob `8eb964eabc4d21babed29d851c3602adb81466bc`; complete candidate diff review. Native U++/Windows builds and new tests not run here. Baseline gate remains FAIL until an actual newer report.
NEXT ACTION: wire ellipse_bands and width percentage into Template inspector, new Media defaults, JSON versioned round-trip, production C++ export and shared clip-based preview picking. Publish/verify that coherent slice, then Gary runs accumulated Debug gate.

## 03D3 source

Shared template bool ellipse_bands (default false) and bounded width percent80.
Only identified templates opt in. Ellipse/Circle Header/Footer use independently
validated narrower/outward rectangles with unchanged height; central safe is NOT
inflated. Body remains within conservative capacity. Existing labelled reservations
and all graph topology are preserved. Native Micro/other shapes stay conservative.
Runtime component painting uses the matching region clip. Existing projection
already carries these retained rectangles; no second layout cache was added.
RenderTests has a new ellipse suite (nine total), including a real paint pixel
outside the old safe rectangle. See UIGRAPH_ELLIPSE_BANDS.md for exact boundaries.

## Retained concurrent checkpoints

35a2cf89665a4800d3a706466cf7c26c0661083b (03D1): single ParentCtrl rail host fixes
hidden inspector allocation; checked mode feedback and zoom caption; startup smoke5.
08f09ba1f1b8a02d24a5f6301acc371ebef307af (03D2): resolve named rich inputs once,
reverse per-region minimum-height pass before natural Ellipsis rows consume height.
Preserve both: earlier parallel drafts were NOT published over newer main.
The failing Media assertions and the 03C camera-admission/performance work remain
unchanged. No native PASS or named-component 10k performance acceptance is claimed.

## Latest Windows evidence — FAIL at 1fa8793

Gary tested `1fa8793c1b3ad440d86be9a2007f0825d0650275`, required ancestry PASS.
Render8 suites/0, Workspace33/0, generated C++ Ui/CtrlLib-only compile PASS, hash
`5058C4DB324F6D2A2F30DD889E823FF9AABFDAC6606E8168353828F06DD868D3`.
Startup1/0. Native view45/2 FAIL: Media title readable with enlarged Subtitle and
two icons failed on Rectangle and Ellipse. Delete/Undo/focus/proxy checks passed.
Stopped before new manual checks/normal launch. No source fixes/new PID; diff check
PASS, worktree clean. Evidence:
`C:\Users\admin\AppData\Local\Temp\UiGraphWorkspace-validation\20260921-124117-1fa8793c1b3a-208e3d`.
03A at8f9a49 was previously validated; held-button Escape during DND remains unverified.
Do not identify a screenshot executable from a source SHA alone.

## Read / scope

Read this file, UIGRAPH_ELLIPSE_BANDS.md, UIGRAPH_WORKSPACE_03B.md,
UIGRAPH_WORKSPACE_V8_AUDIT.md, UIGRAPH_PERFORMANCE_AUDIT.md and current complete
layout/runtime/authoring source. Active app: examples/UiGraphComponentStudio.
DesignMatrix remains retired. One NodeGeometry.presentation authority, shared
bounded C++ templates, no runtime JSON compiler or ordinary per-node Ctrl tree.
Family layout/style inheritance stays separate. Camera never resizes authored nodes.

Still pending: authoring integration listed above; template-driven diagram inventory;
complete V8 Body-only/Full-edge shared post-port Content/Overlay and zone controls;
threshold undo/compact lifecycle; full Save/Open/physical DND checks. Do not bundle
unrelated redesign or claim these done by ellipse bands.

## Gary gate

Clean current main at E:\apps\github\upp_Ui. Require latest supervisor SHA as an
ancestor, not exact equality. Run scripts/ValidateUiGraphWorkspace.ps1 with
-RequiredAncestor <SHA> -Launch, established U++18468 / CLANGx64 / Debug only.
All RenderTests (now nine suites), WorkspaceTests, generated C++, native view and
startup smoke must pass. Preserve the currently failing title checks. After PASS,
manual code full-height/active indicator, Normal Media text, ellipse toggle/width,
outer-band picking, Delete/Undo, inspector-scroll and save/reload/export checks.
Minor mechanical CLANG fixes may be reviewed/documented/published and retested;
stop on ownership/architecture failures. No test weakening, rich-Micro fallback,
retired DesignMatrix, Release or broad benchmark. Report HEAD, ancestry, summaries,
first blocker, evidence, manual findings, PID, diff check/tree and fix SHA.
