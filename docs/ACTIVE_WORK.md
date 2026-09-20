# ACTIVE WORK

Remote main is authoritative. Fetch before editing/publishing; never force-push.

BASE: `8f9a49a8367f0dd9d95d1085582e9348c5aa91db` / main
TASK: **UIGRAPH-NODE-WORKSPACE-03B — component Delete, preview selection and readable text fitting**
TOUCHED: `Ui/UiGraph/UiGraphNodeComponent.cpp`; `examples/UiGraphComponentStudio/{WorkspaceFiles.cpp,WorkspaceViews.h,WorkspaceViews.cpp,WorkspaceViewTests.cpp}`; this file; `docs/UIGRAPH_WORKSPACE_03B.md`.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for this bounded 03B source slice. Oval band layout is documented direction, NOT implemented.
PUBLISHED: the commit containing this update; recover with `git log -1 -- docs/UIGRAPH_WORKSPACE_03B.md` and inspect current remote main.
VALIDATION: complete pinned touched source reconstructed and original Git blob hashes matched; full local source diff and `git diff --check` PASS. No U++/Windows compilation or native execution in this environment. Added regressions are not a runtime PASS.
NEXT ACTION: Gary's focused Debug gate and manual Delete/text checks in `docs/UIGRAPH_WORKSPACE_03B.md`. Then the outstanding diagram inventory and V8 port-reservation work; resolve shape-aware bands against those contracts rather than moving demo rectangles independently.

## Latest source changes

- Painted structure/region selection takes keyboard focus. Preview selection does
  the same, copies component IDs before callbacks, and adds a three-pixel logical
  hit tolerance for bars/dots, clipped to the component slot and retained safe area.
- Delete on those focused surfaces invokes the existing Remove transaction. It
  does not intercept Delete in property/text editors or delete graph topology.
  Inherited layouts remain read-only; removal is undoable.
- Single-line Ellipsis text tries a bounded font-height fit down to its configured
  final-pixel readable floor before becoming a bar. Width still uses ellipsis;
  a fitting authored font is unchanged. Clip/Wrap are unchanged. No node/slot
  resizing, mask rewriting, extra layout cache or Micro text work is introduced.
- The structure table now prefixes its placement summary with the ACTUAL current
  representation and a named reason (for example Text 12px / Bar: no room).
- Existing native view tests now also exercise focus routing, Delete/undo/scope,
  thin proxy hits, Media title on Rectangle/Ellipse, height-fit/ellipsis/floor,
  unchanged authored fonts and the Micro early-return boundary.

## Latest Windows evidence — Gary's report, 03A only

Tested `8f9a49a8367f0dd9d95d1085582e9348c5aa91db`; ancestry PASS.
Debug builds: RenderTests, WorkspaceTests, generated C++ and Component Studio PASS.
Generated C++ compiled unchanged with Ui/CtrlLib only. RenderTests: 8 suites,
failed_suites=0. WorkspaceTests: checks=33, failed=0. Native view checks=27,
failed=0. Startup smoke checks=1, failed=0. Diff check PASS; worktree clean.
No source fixes/commits. Demo was PID 320416 (historical observation, not a process
that the next agent should kill or assume still exists).

Manual PASS reported: both Left header icons survive Normal/LOD1/Normal; isolated
component typography changes; port-label toggles preserve inspector position;
inherited Ellipse is explained/read-only. Held-mouse-button Escape during native
DND remains unverified. Curt then reported missing Delete and a Normal title bar.
Those were not covered by the passing 03A gate. This 03B source is newer and must
be validated separately. Evidence path supplied by Gary:
`C:\Users\admin\AppData\Local\Temp\UiGraphWorkspace-validation\20260920-203041-8f9a49a8367f-6ea403`.

## Read first / authoritative boundaries

1. This file and `docs/UIGRAPH_WORKSPACE_03B.md`.
2. `docs/UIGRAPH_WORKSPACE_V8_AUDIT.md` (03A audit; pending items remain pending).
3. `docs/UIGRAPH_NODE_WORKSPACE.md`, `docs/UIGRAPH_WORKSPACE_AUTHORING.md`.
4. `docs/UIGRAPH_WORKSPACE_RUNTIME.md`, `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md`.
5. Current complete source, callers and tests.

Active application: `examples/UiGraphComponentStudio`. DesignMatrix remains
retired. V8 is the latest HTML reference; the newer oval sketch is design input,
not a second production allocator. JSON is authoring interchange, not runtime UI.
NodeGeometry.presentation remains the sole evaluated per-node layout cache.
Component identity, inclusion, available capacity and representation are distinct.
Content/Overlay are sibling layers. Ordinary components never allocate Ctrl trees.
One family JSON saves Base layout/appearance plus independent shape snapshots;
component styling travels with layout. Production C++ has no authoring dependency.
Camera edits do not resize nodes. Port identities and connections remain model-owned.

## Remaining work (not claimed complete)

- Template-driven diagram inventory for components lacking a prepared rectangle.
- Complete V8 Body-only/Full-edge post-port interior shared by BOTH layers,
  input/output zone preview controls, and associated JSON/export/anchor tests.
- Shape-aware independently fitted Header/Footer bands: see the explicit proposal
  and containment/cache constraints in 03B. Current production still uses one
  conservative safe rectangle.
- Malformed zoom-caption suffix, palette/gallery glyphs, diagram chip packing,
  threshold gesture undo and compact-size restoration; wider save/open/DND manual
  follow-through. No native screenshot parity or large-graph performance claim.

## Recovery / validation

Refresh current main. Require the latest supervisor commit as an ancestor, never
exact equality. Run `scripts/ValidateUiGraphWorkspace.ps1 -RequiredAncestor <SHA>
-Launch` using established Windows toolchain; full command/task is in 03B.
No Release/broad suite/10k benchmark without a focused reason. No test weakening,
retired-field restoration or rich-Micro fallback. Minor mechanical CLANG fixes
may be reviewed, documented, published and retested. Stop for ownership/architecture
failures. Report exact tested HEAD, ancestry, summaries, first issue, manual findings,
evidence directory, demo PID, diff check, worktree status and any fix SHA.
