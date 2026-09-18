# ACTIVE WORK

Remote main is authoritative. Refresh before work/publish; never force-push.

BASE: `acb1b8b5b8029996316ba391bac8830e07c63ca9` / main
TASK: **UIGRAPH-NODE-WORKSPACE-02C — native V7 workspace and old Studio retirement**
TOUCHED: `examples/UiGraphComponentStudio/`; retired `examples/UiGraphDesignMatrix/`; `GETTING_STARTED.md`; `docs/{ACTIVE_WORK,UIGRAPH_NODE_WORKSPACE,UIGRAPH_NODE_COMPONENTS,UIGRAPH_NODE_LAYOUT_ARCHITECTURE}.md`.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for 02C source. Visual parity/usability is not claimed validated.
PUBLISHED: the commit containing this update; recover with `git log -1 -- examples/UiGraphComponentStudio/WorkspaceWindow.cpp`.
VALIDATION: complete touched-source review, declaration/definition and package checks, uploaded-file hash matching, local git diff --check PASS. No native compile or GUI execution here. Tests are source-added, not runtime PASS.
NEXT ACTION: Gary's accumulated Debug gate: current RenderTests + WorkspaceTests + new workspace + exported C++ compilation. Fix first real blocker, then visual/interaction refinement against V7.

## Read first

1. This file.
2. `docs/UIGRAPH_NODE_WORKSPACE.md` — current V7 app and explicit limits.
3. `docs/UIGRAPH_WORKSPACE_AUTHORING.md` — family inheritance, strict JSON, code generation.
4. `docs/UIGRAPH_WORKSPACE_RUNTIME.md` — actual registered-template/component runtime.
5. `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md` — ONE retained layout authority.

Old V4/four-preview and 01A notes are historical. The current authoring UI is
examples/UiGraphComponentStudio. DesignMatrix is no longer an active/buildable
package; general graph tests and demos were not retired.

## Recovered checkpoints

14551a3aa933a1e6e65296e7acb1d82bb401ac0f: 02A runtime — registered shared templates,
width thresholds, seven painted families, native bounded Micro hints, eight
RenderTests suites. Windows validation pending.

acb1b8b5b8029996316ba391bac8830e07c63ca9: 02B authoring — Base + independent
layout/style shape snapshots, strict candidate JSON, portable bounded static
assets, atomic file replacement, C++ factories/registration, common edit
transactions and WorkspaceTests. Windows validation pending.

02C: replaces four-preview app with left family/shape/scope/palette, live threshold
control, region/overlay diagrams, ONE production preview, wide structure/LOD table,
real PropertyEditor Inspector/Template/Style and generated C++ rail. Native DND
uses scope/revision/ID validation and transactional edits. Save/Open/Clone/New,
separate shape-section detachment/reset, confirmed copy-all, image/icon/font
adapters and compact/expand are wired. Geometry guides come from production.

## Last Windows evidence — 01A ONLY

Gary tested d9754c76d7be0b37d951653f3f643c3e3963782a: RenderTests seven suites PASS;
Components21/0; ExecutionPath8/0; DesignMatrix selector4/0; ComponentStudio Debug
build/startup PASS; diff check PASS; clean tree; no fixes. Title-edit automation
was interrupted by physical Escape, not an interaction PASS. That evidence does
not validate 02A/02B/02C.

## Architecture / scope

NodeGeometry.presentation is the one retained layout cache. Content and Overlay
are sibling Body layers. No per-node Ctrl tree or runtime JSON layout compiler.
Seven palette kinds are painted; Actions are NOT arbitrary live controls/accordions.
True Ctrl uses host SetNodeCtrl. Component-local style travels with component
layout; family node appearance detaches independently. JSON saves both; production
C++ has no authoring package dependency. Camera edits never resize the node.

Remaining refinements: gallery/palette glyphs, tighter high-DPI/small-window layout,
diagram chip packing, richer proxy diagnostics, more granular undo/redo, optional
component-style resources and live-control authoring. No native visual parity or
performance claim without measurement. See current workspace page for limitations.

## Gary focused gate

Repo E:\apps\github\upp_Ui / main. Require the supervisor's published 02C commit
as ancestor of current HEAD. Earlier 02A and 02B must also be ancestors.
Debug only: Utilities/UiGraphRenderTests; Utilities/UiGraphWorkspaceTests;
examples/UiGraphComponentStudio; an exported C++ fixture using only Ui.
Do not build retired UiGraphDesignMatrix. Capture UIGRAPH_WORKSPACE_UI_SMOKE.

Manual: palette-to-diagram/table drops and Escape cancellation; move/reorder;
inherited scope rejection/detach/reset; select same component across three views;
PropertyEditor commit/cancel/override; real colours and seven component types;
Open/Save/SaveAs/Clone with invalid import preserving work; independent style/layout
shape inheritance; threshold vs camera independence; 1:1/LOD jumps; Micro hints.

Minor mechanical CLANG fixes may be reviewed/committed/published. Stop and report
architecture or ownership failures; no test weakening or rich-Micro fallback.
Report exact tested HEAD, ancestry, summaries, first blocker, visual evidence,
demo PID and clean worktree. No Release/broad suite/10k unless a focused failure
requires it. Publish coherent checkpoints and update this recovery log each time.
