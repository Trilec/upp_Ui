# ACTIVE WORK

Remote main is authoritative. Refresh before work/publish; never force-push.

BASE: `fbd283aba2c9deb0f02a855ce7b008c0283a12f2` / main
TASK: **UIGRAPH-NODE-WORKSPACE-02D — diagram/table interaction hardening**
TOUCHED: `examples/UiGraphComponentStudio/{WorkspaceViews.h,WorkspaceViews.cpp,WorkspaceViewTests.cpp,UiGraphComponentStudio.upp,main.cpp}`; this file; `docs/UIGRAPH_WORKSPACE_02D_VALIDATE.md`.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for 02D source.
PUBLISHED: the commit containing this update; recover with `git log -1 -- examples/UiGraphComponentStudio/WorkspaceViewTests.cpp`.
VALIDATION: complete touched-source review, exact base-blob matching, declaration/definition and package checks, local git diff --check PASS. U++/Windows compilation and GUI execution NOT run here. Native regression tests are added, not claimed passing.
NEXT ACTION: Gary's accumulated Debug gate in `docs/UIGRAPH_WORKSPACE_02D_VALIDATE.md`: RenderTests, WorkspaceTests, workspace --view-tests, generated C++ compilation and actual workspace interaction. No retired DesignMatrix build.

## Latest bounded changes

- Disarm a palette button before entering native DND; Escape/release cannot also activate its click action.
- Wrap empty-region '+' targets inside narrow diagrams instead of drawing later targets offscreen.
- Reject diagram drops/hits on actual graph-owned port reservations and outside the client area.
- Grey hidden component chips independently from their visible siblings. Keep Stable-hidden chips selectable.
- Scroll the wide structure/LOD table horizontally without changing hit-column meaning.
- Clip headers/placement text; show coloured inclusion badges and an explicit insertion line.
- Clear stale DND feedback after reconstruction; copy identities before re-entrant selection callbacks.
- Placement summaries recognise bindings and all supported component-style overrides.
- Native view regressions run on Debug startup or with `--view-tests` (test-only exit).

These are authoring-view changes. No runtime layout, template schema, Micro backend,
or port topology changes are included. Physical DND and visual quality still need
Windows validation. Threshold-drag undo coalescing and compact-size restoration
remain the next editing-lifecycle refinement; do not claim them fixed by 02D.

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

fbd283aba2c9deb0f02a855ce7b008c0283a12f2: 02C replaces four-preview app with left family/shape/scope/palette, live threshold
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
not validate 02A/02B/02C/02D.

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

Repo E:\apps\github\upp_Ui / main. Require the supervisor's latest published checkpoint
as ancestor of current HEAD. Earlier 02A and 02B must also be ancestors.
Debug only: Utilities/UiGraphRenderTests; Utilities/UiGraphWorkspaceTests;
examples/UiGraphComponentStudio; an exported C++ fixture using only Ui.
Do not build retired UiGraphDesignMatrix. Capture UIGRAPH_WORKSPACE_UI_SMOKE
and UIGRAPH_WORKSPACE_VIEW_SUMMARY; see the complete 02D validation task.

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
