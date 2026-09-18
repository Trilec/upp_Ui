# ACTIVE WORK

Remote main is authoritative. Refresh before work/publish. Never force-push.

BASE: `57988643b83573636ab85731442637eee6f2cab4` / main
TASK: **UIGRAPH-NODE-WORKSPACE-02A — runtime families, thresholds and native Micro hints**
TOUCHED: `Ui/Ui.upp`; `Ui/UiGraph/UiGraphNodeTemplate.h`, `UiNodeGraph.h`, presentation/geometry/projection/rich/Micro source; new `UiGraphNodeComponent.h/.cpp`, `UiGraphNodeComponentPaint.cpp`, `UiNodeGraphTemplates.cpp`; `Utilities/UiGraphRenderTests/{main.cpp,UiGraphRenderTests.upp,WorkspaceComponents.cpp}`; this file; `docs/UIGRAPH_WORKSPACE_RUNTIME.md`.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for runtime 02A only. V6 authoring UI remains PARTIAL.
PUBLISHED: source commit containing this update; recover with `git log -1 --format=%H -- Ui/UiGraph/UiNodeGraphTemplates.cpp`.
VALIDATION: complete touched-source/diff review, git diff --check PASS, uploaded content hashes checked. New tests added; Windows compilation/runtime NOT run here.
NEXT ACTION: finish the V6 native workspace, DND, persistence/code generation and focused authoring tests. Retire DesignMatrix only with the replacement source publish; then Gary validates the accumulated checkpoint.

## Read first

1. `docs/UIGRAPH_NODE_WORKSPACE.md` — accepted V6 workspace specification.
2. `docs/UIGRAPH_WORKSPACE_RUNTIME.md` — implemented 02A API and limits.
3. `docs/UIGRAPH_NODE_COMPONENTS.md` — historical 01A baseline; newer runtime supersedes it.
4. `docs/UIGRAPH_NODE_COMPONENT_CONTRACT_DRAFT.md` — wider accepted direction.
5. `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md` and complete current source.

The user now wants ONE live preview plus explicit LOD jump/reset controls and
colour-coded region/overlay diagrams. Earlier four-preview UI prose is historical.
Neither existing demo is removed by this runtime checkpoint.

## Last Windows evidence — Gary's report, baseline only

Tested `d9754c76d7be0b37d951653f3f643c3e3963782a`:
- d9754c7 and f12a255 required ancestry PASS.
- RenderTests Debug build/runtime PASS: Components 21/0, ExecutionPath 8/0,
  UIGRAPH_RENDER_TESTS_SUMMARY suites=7 failed_suites=0.
- DesignMatrix Debug build PASS; selector smoke checks=4 failed=0.
- ComponentStudio Debug build/startup PASS; four previews, no obvious clipping/crash.
- git diff --check PASS, clean worktree, origin/main only, no fixes.

Title-edit UI automation was interrupted by physical Escape, not an observed app
failure. That interaction is not a completed PASS. Old PIDs are not recovery state.
This report does NOT validate 02A or the forthcoming workspace.

## Runtime source now implemented

Validated owned template-class registration keyed by node.style_class; optional
projected-outer-width LOD policy; bounded Text/Icon/Image/Progress/Fields/Tags/painted
Actions; per-component typography/role/state style and overflow; retained native
Micro bars/dots/tiny-ready images with budget/collision reasons; cold overview
parity; pan projection of group rectangles. Named-component wheel scaling remains
exact fallback. RenderTests has eight suites; no old tests were disabled.

The component preparation/painter now use ordinary .h/.cpp units. The existing
production region cursors still allocate all component capacity. No demo allocator.

## Not yet in this checkpoint

V6 shell, drag/drop, family/shape authoring, JSON and C++ export are the NEXT slice.
Arbitrary live controls are not a painted palette family; SetNodeCtrl remains the
explicit host escape hatch. Actions currently paint cues, not clickable commands.
Image resources are bounded prepared rasters, not a general image-pyramid system.
Dynamic mosaics require host-supplied small assets. Micro text occupancy is an
approximate presence cue, not measured glyph spans. No region auto-collapse,
animated fades or fine-grained dependency engine.

## Architecture and workflow

NodeGeometry.presentation is the ONE retained evaluated layout result. Body owns
sibling Content and Overlay layers spanning the same Body, each with independent
L/M/R columns. Ports retain semantic IDs and silhouette anchors. No retired aliases,
second layout cache, runtime JSON compiler or per-node Ctrl tree.

REFRESH -> INSPECT -> IMPLEMENT -> REVIEW -> PUBLISH -> VERIFY -> VALIDATE.
Publish coherent source with current recovery state. Refresh main before publish,
preserve parallel work, verify commit/branch afterward. No force or proof branches.
Gary: latest-main ancestry, Debug RenderTests plus focused authoring tests/current
workspace. Minor mechanical CLANG fixes allowed after review; no disabled tests,
architecture redesign or rich-Micro shortcuts. Report exact HEAD, first real
failure, summaries, GUI evidence and clean worktree status.
