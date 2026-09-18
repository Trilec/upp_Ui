# ACTIVE WORK

Remote main is authoritative. Refresh before work/publish. Never force-push.

BASE: `d9754c76d7be0b37d951653f3f643c3e3963782a` / main
TASK: **UIGRAPH-NODE-WORKSPACE-02 — V6 authoring workspace and production components**
TOUCHED: this file; `docs/UIGRAPH_NODE_WORKSPACE.md`
STATUS: **PARTIAL — direction recorded; source implementation in progress**
PUBLISHED: documentation checkpoint containing this update; inspect current remote main.
VALIDATION: Gary reported Windows Debug PASS at d9754c7 (details below). This checkpoint has no source changes.
NEXT ACTION: implement the coherent production/workspace slices below and publish
with current recovery state. Do not report planned capabilities as implemented.

## Read first

1. `docs/UIGRAPH_NODE_WORKSPACE.md` — current accepted V6 workspace direction.
2. `docs/UIGRAPH_NODE_COMPONENTS.md` — implemented 01A API and boundaries.
3. `docs/UIGRAPH_NODE_COMPONENT_CONTRACT_DRAFT.md` — wider accepted component contract.
4. `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md` — retained layout authority.
5. Current complete source, callers, tests and .upp membership.

The latest user request replaces the four-preview authoring UI with one main live
preview plus explicit LOD jump/reset controls and colour-coded region/overlay
editors. Old four-preview documentation is historical UI guidance, not authority.

## Last Windows validation — validator-provided evidence

Gary tested `d9754c76d7be0b37d951653f3f643c3e3963782a`:
- Required d9754c7 and f12a255 ancestry PASS.
- UiGraphRenderTests Debug build/runtime PASS.
- UIGRAPH_COMPONENT_SUMMARY checks=21 failed=0.
- UIGRAPH_EXECUTION_PATH_SUMMARY checks=8 failed=0.
- UIGRAPH_RENDER_TESTS_SUMMARY suites=7 failed_suites=0.
- UiGraphDesignMatrix Debug build PASS; selector smoke checks=4 failed=0.
- UiGraphComponentStudio Debug build PASS; four previews rendered, no obvious
  clipping or crash in visual startup inspection.
- git diff --check PASS; clean worktree; only origin/main; no source fixes.

Title-edit interaction was interrupted by a physical Escape stopping the Windows
UI helper. No application failure was observed, but that interaction is not a
completed acceptance PASS. Historical demo PIDs are not recovery authority.

## Implementation direction

Evolve `examples/UiGraphComponentStudio` into the single Node Design Workspace.
Retire the old DesignMatrix Studio at the replacement source checkpoint; preserve
unrelated graph demos and automated rendering/performance coverage.

Use the attached V6 HTML as visual guidance: left family/shape/palette management,
central live thresholds and region/overlay/production preview, lower wide
hierarchy/placement/LOD table, right actual PropertyEditor with Inspector / Style
Overrides / generated C++ modes. Native DND must route every surface through one
validated add/move/reorder operation. Named selection is shared across surfaces.

Authoring families have a base design and explicit optional shape overrides.
JSON is versioned authoring interchange; generated shared C++ template/style
factories use the same validated definitions as the production preview.

Painted components are not arbitrary real controls. Only supported component
families are enabled. Embedded controls use the existing explicit host binding
and useful-scale activation contract; no per-node Ctrl trees.

## Current production boundary

01A implements named repeated Text/Icon slots, typed existing-data text bindings,
per-slot alignment/ink/font height/LOD overrides/Stable-Reflow, prepared rich bars
and dots, retained component output, pan reuse and exact named-component wheel
fallback. It does NOT yet implement native physical-Micro hints, rich collection
components, DND, JSON/code export or the V6 shell. Source code overrides this
summary when subsequent checkpoints advance main.

`NodeGeometry.presentation` remains the single evaluated layout cache. Body has
sibling Content and Overlay layers, each with L/M/R columns. Port anchors remain
semantic silhouette geometry. No retired body_left/body_main/center aliases.
No parallel demo allocator, runtime JSON compiler or per-node UI layout tree.

## Checkpoint / validation discipline

Publish only coherent reviewed source. Each publish updates BASE/TASK/TOUCHED/
STATUS/PUBLISHED/VALIDATION/NEXT ACTION here. Verify remote commit and branch after
writes. Fresh-fetch main before publishing; preserve concurrent work.

Gary validates latest main with required checkpoint ancestry. Debug only:
UiGraphRenderTests, focused workspace tests and the current native workspace.
Minor mechanical CLANG fixes may be reviewed/committed/published; no architecture
redesign, disabled tests or rich-Micro shortcuts. New checkpoints remain platform
validation pending until actual compile/runtime evidence is supplied.
