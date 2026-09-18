# ACTIVE WORK

Remote main is authoritative. Refresh before work/publish. Never force-push.

BASE: `14551a3aa933a1e6e65296e7acb1d82bb401ac0f` / main
TASK: **UIGRAPH-NODE-WORKSPACE-02B — V7 family document, edit transactions and export**
TOUCHED: `Utilities/UiGraphWorkspace/`, `Utilities/UiGraphWorkspaceTests/`, `docs/UIGRAPH_WORKSPACE_AUTHORING.md`, this file.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for authoring library 02B. Replacement V7 UI remains PARTIAL.
PUBLISHED: the commit containing this update; recover via `git log -1 -- Utilities/UiGraphWorkspace/WorkspaceDocument.cpp`.
VALIDATION: complete new-source review and local git diff --check PASS. U++/Windows compiler and GUI are unavailable here. Added tests are NOT claimed passing.
NEXT ACTION: publish the native V7 workspace shell, real PropertyEditor, region/table DND, file actions and generated-code panel. Retire DesignMatrix with that replacement, then Gary validates the accumulated runtime/authoring/UI checkpoint.

## Read first

1. `docs/UIGRAPH_WORKSPACE_AUTHORING.md` — V7 document/inheritance/save/export contract.
2. `docs/UIGRAPH_NODE_WORKSPACE.md` — workspace intent; V7 extends its family and rail design.
3. `docs/UIGRAPH_WORKSPACE_RUNTIME.md` — implemented 02A runtime API/limits.
4. `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md` — retained geometry contract.
5. Current full source; older four-preview/01A prose is historical.

## Recovery

Runtime 02A was already published at 14551a3: registered shared templates, optional
outer-width thresholds, seven painted component families, per-component style,
native bounded Micro hints and eight RenderTests suites. Its Windows validation
remains pending. Do not reimplement or erase it from a stale chat snapshot.

02B now provides a separate AUTHORING package (never linked into runtime JSON
layout): one family per document, Base plus independent layout/style shape
snapshots, portable bounded static assets, strict candidate import, atomic file
replacement, deterministic C++ export and common transactional drop operations.
The authoring UI can use those operations instead of a second document/allocator.
Neither existing demo is retired by this library checkpoint alone.

## Last Windows evidence — Gary's report, 01A only

Tested d9754c76d7be0b37d951653f3f643c3e3963782a: RenderTests seven suites PASS;
Components 21/0; ExecutionPath 8/0; DesignMatrix selector 4/0; ComponentStudio Debug
build/startup PASS; diff check PASS; clean worktree; no fixes. Title-edit automation
was interrupted by physical Escape, not a completed interaction PASS.
This does not validate 02A, 02B or the next V7 UI.

## Architecture constraints

NodeGeometry.presentation is the ONE retained evaluated layout. Body owns sibling
Content/Overlay layers, each L/M/R. Ports retain semantic IDs/anchors. No per-node
Ctrl tree, runtime JSON compiler, alternate demo allocator or retired field aliases.
Styles and layout save together but resolve independently. Preview data/camera stay
out of generated production templates. Real Ctrl uses SetNodeCtrl, not Actions.

## Validation after replacement UI

Debug: Utilities/UiGraphRenderTests (eight suites), Utilities/UiGraphWorkspaceTests,
and examples/UiGraphComponentStudio. Compile an exported C++ fixture. GUI checks:
new/clone/open/save/save-as, independent shape overrides, real DND/cancel/stale-drop,
PropertyEditor selection/style/code, thresholds versus camera and Micro hints.
Minor mechanical CLANG fixes allowed after review; no test weakening/redesign.
Report exact tested HEAD/ancestry, first failure, summaries, GUI evidence and clean
worktree. No Release/broad suite/10k unless focused checks justify it.
