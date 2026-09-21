# ACTIVE WORK

Remote main is authoritative. Refresh before editing/publishing; never force-push.

BASE: `645a6b774a47c62ab0955621fb81cf39efe9e717` / main
TASK: **UIGRAPH-NODE-WORKSPACE-03D4 — persist and export ellipse-band policy**
TOUCHED: `Utilities/UiGraphWorkspace/{WorkspaceJson.cpp,WorkspaceCode.cpp}`; `Utilities/UiGraphWorkspaceTests/main.cpp`; this file; `docs/UIGRAPH_WORKSPACE_AUTHORING.md`.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for the persistence/export slice. Workspace controls/defaults/picking are next.
PUBLISHED: the commit containing this record; recover with `git log -1 -- Utilities/UiGraphWorkspace/WorkspaceJson.cpp`.
VALIDATION: full pinned touched files reconstructed, original Git blob hashes matched, full local diff reviewed and git diff --check PASS. No Windows/U++ build or native execution here. Gary's last gate remains FAIL until a newer run.
NEXT ACTION: wire runtime ellipse_bands/width into Template inspector, new Media defaults and shared clip-based preview picking. Keep current title and rail corrections. Publish that integration, then run accumulated Debug validation.

## Current source

03D4 writes strict workspace schema v2. Base and every detached shape preserve
ellipse_bands and ellipse_band_width_percent. V1 imports retain false/80 and all
previous authored geometry; saving upgrades explicitly to v2. Missing/wrong/unknown
v2 fields reject transactionally. C++ emits both fields without runtime JSON or
an authoring dependency. Tests include v1 migration, v2 round-trip, independent
layout/style inheritance, failed-import preservation and the actual exported
compilation fixture. These tests are not a native PASS yet.

Retained preceding checkpoints:
- 35a2cf89665a4800d3a706466cf7c26c0661083b / 03D1: one ParentCtrl rail host;
  code consumes the rail instead of sharing with hidden inspector. Checked mode
  feedback and zoom caption repaired; startup smoke has five checks.
- 08f09ba1f1b8a02d24a5f6301acc371ebef307af / 03D2: one rich input-resolution pass
  and bounded reverse per-region readable-height reservation. A natural Subtitle
  must leave a readable minimum for later Title Fill. Existing failing assertions
  remain; no font-floor reduction, fixed-header inflation or LOD-mask rewriting.
- 645a6b774a47c62ab0955621fb81cf39efe9e717 / 03D3: real opt-in Ellipse/Circle bands;
  whole-band containment, unchanged conservative safe, per-region paint clipping,
  no new per-node cache. Nine RenderTests suites. Read UIGRAPH_ELLIPSE_BANDS.md.

## Latest Windows evidence — FAIL at 1fa8793

Gary tested `1fa8793c1b3ad440d86be9a2007f0825d0650275`, ancestry PASS.
Render8/0, Workspace33/0, generated C++ Ui/CtrlLib-only compile PASS, SHA-256
`5058C4DB324F6D2A2F30DD889E823FF9AABFDAC6606E8168353828F06DD868D3`.
Startup1/0. Native view45/2 FAIL: Media title with enlarged Subtitle and two icons
failed on Rectangle and Ellipse. Other Delete/Undo/focus/proxy checks passed.
Stopped before manual checks/normal launch; no fix/new PID; diff PASS; tree clean.
Evidence: `C:\Users\admin\AppData\Local\Temp\UiGraphWorkspace-validation\20260921-124117-1fa8793c1b3a-208e3d`.
03A was previously validated; held-button Escape DND remains unverified.
New export fixtures legitimately have a different hash. Do not assert the old hash.
Do not identify the screenshot executable solely from source HEAD.

## Architecture / recovery

Active app: examples/UiGraphComponentStudio. DesignMatrix stays retired.
One NodeGeometry.presentation; bounded shared C++ templates; no runtime JSON
compiler, ordinary per-node Ctrl tree or competing preview allocator. Component
styling follows layout; family appearance inheritance remains separate. Camera
changes never resize authored nodes. Preserve prior 03C performance/camera work.
Read this file, UIGRAPH_ELLIPSE_BANDS.md, UIGRAPH_WORKSPACE_AUTHORING.md,
UIGRAPH_WORKSPACE_03B.md, UIGRAPH_WORKSPACE_V8_AUDIT.md and complete current source.

Pending after current integration: diagram inventory for unallocated components;
complete V8 Body-only/Full-edge shared post-port Content/Overlay and zone controls;
threshold undo/compact lifecycle; physical DND and full Save/Open checks. Neither
bands nor this persistence checkpoint claims to finish those tasks.

## Gary gate

Clean current main: E:\apps\github\upp_Ui. Require latest published SHA as ancestor.
Run scripts/ValidateUiGraphWorkspace.ps1 -RequiredAncestor <SHA> -Launch with
established U++18468 / CLANGx64, Debug only. RenderTests now nine suites;
WorkspaceTests, unchanged exported C++, native view and startup smoke must pass.
Keep failed Media checks. After PASS: code full-height/selected icon, Normal text,
ellipse toggle/width, outer-band picking, Delete/Undo and save/reload/export.
Minor mechanical fixes may be reviewed/documented/published and retested; stop for
architecture failures. No weakened tests, rich-Micro fallback, retired matrix,
Release/broad benchmark. Report exact HEAD, summaries, first issue and clean tree.
