# ACTIVE WORK

Remote main is authoritative. Refresh before editing/publishing; never force-push.

BASE: `4c997824f741f1555269f95aec3462e7674045e0` / main
TASK: **UIGRAPH-NODE-WORKSPACE-03E2 — visible Content underlay and explicit Overlay/fit summaries**
TOUCHED: `examples/UiGraphComponentStudio/{WorkspaceViews.h,WorkspaceViews.cpp,WorkspaceOverlayTests.cpp}`; this file; `docs/UIGRAPH_OVERLAY_CONTRACT.md`.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for accumulated 03E1/03E2. The preceding 03D gate is now reported PASS, not FAIL.
PUBLISHED: the commit containing this record; recover exact SHA with `git log -1 -- examples/UiGraphComponentStudio/WorkspaceOverlayTests.cpp`.
VALIDATION: complete pinned touched originals reconstructed with matching Git blob hashes; local full diff and git diff --check PASS. New C++ tests/Windows GUI not executed here.
NEXT ACTION: Gary validates the accumulated overlay checkpoint and manual on/off test using the recorded executable. Preserve the preceding PASS and regression suites; continue general diagram inventory / V8 ports only after this bounded gate.

## Latest source / 03E2

The Overlay diagram now paints a subdued labelled Content footprint below its
Overlay guides, using the same retained projection/clip. Contain gaps remain
unpainted; hidden Content leaves no invented footprint. This is diagnostic guide
painting only, never another production allocator or thumbnail renderer.
Underlay guides cannot receive drops or hijack Overlay/port hit routing.
Structure summaries expose Cover (crop), Contain (whole), and Overlay membership.
Native tests exercise projected footprint, guide paint, hidden state, hit-routing
and summaries in addition to 03E1 geometry/raster/paint-order tests.

## Retained source / 03E1 — 4c997824


Content/Overlay allocation and paint ordering are already separate in production.
The native Media preview looked like two columns because Thumbnail used left-
aligned Contain, leaving unpainted space where Ready sat. New Media uses Cover
(crop to fill its own allocation), with State right-aligned in OverlayRight.
No saved-family migration, global image-fit change, fake ContentRight column,
new runtime schema or retained geometry cache is introduced.

The new native Overlay gate checks real overlap, unchanged underlying rectangles
and image bytes across overlay edits, paint order with reversed template ordering,
upper-layer selection, Contain invariance and persistence. It is wired into both
Debug startup and --view-tests and fails the process before normal launch.
No new native PASS is claimed. Read UIGRAPH_OVERLAY_CONTRACT.md for exact scope.

## Latest Windows evidence — reported PASS at 57e8d381

Gary tested `57e8d38167cde7cee2bc62b0093979af86ca91ca` on clean main, using
U++18468 / CLANGx64. Required ancestry and origin/main ancestry PASS. No fixes.
RenderTests 9/0; EllipseBands 16/0; Workspace 44/0; native View 45/0;
Band UI 23/0; startup smoke 5/0; WorkspaceComponent 24/0; Component 21/0;
ExecutionPath 8/0; Presentation 87/0; evidence-reader self-tests 12/0.
Generated C++ compiled unchanged with Ui/CtrlLib only, SHA-256:
`F29B56A3C5AB929B43CEE845A606654C2142A848ABC025ABBCA03AC954F57F62`.

Manual PASS: both Media text rows readable on Rectangle/Ellipse; independent
ellipse bands/60% width and outer-band picking; camera/node size stable;
explicit capacity warning at Header20 and recovery at42; save/reload typography
and bands; generated fields; exclusive full-height Code page and persistent mode
selection after resizing; component Delete/Undo and inherited-layout protection.

Executable evidence SHA-256:
`31644228035E31E05EF33EA276368A675AFD7C18EB95D410C3EC1245AF7BB17E`.
Evidence directory:
`C:\Users\admin\AppData\Local\Temp\UiGraphWorkspace-validation\20260921-210253-57e8d38167cd-6f4450`.
Executable: UiGraphComponentStudio.exe in that directory. PID 339420 was reported
running; historical observation only. Diff check PASS; worktree clean; no commits.
Held-button Escape during native drag remains unverified.
Curt separately reported the confusing Overlay/thumbnail composition. The 03D
PASS does not cover that new issue or source added after the tested checkpoint.

## Retained implementation / recovery

Read this file, UIGRAPH_OVERLAY_CONTRACT.md, UIGRAPH_ELLIPSE_BANDS.md,
UIGRAPH_WORKSPACE_AUTHORING.md, UIGRAPH_WORKSPACE_RUNTIME.md and current source.
Active app is examples/UiGraphComponentStudio. DesignMatrix remains retired.

- 35a2cf8 / 03D1: exclusive rail host, selected-mode styling, zoom caption.
- 08f09ba / 03D2: readable per-region height reservation, old failing tests retained.
- 645a6b7 / 03D3: production Ellipse/Circle bands and independent clipping.
- e5a1f19 / 03D4: strict schema v2; conservative v1 migration; C++ band export.
- 8b2e53c / 03D5: band inspector/defaults/picking and native integration gate.
- 57e8d381 / 03D6: evidence parser and executable identity; now Windows PASS.

One NodeGeometry.presentation remains authoritative. Components are bounded
painted C++ descriptions, not Ctrl trees or runtime JSON. Content/Overlay are
sibling layers; Overlay never participates in Content flow. Image allocation and
painted footprint are distinct. Component styling follows layout; node appearance
inherits independently. Camera edits never resize nodes. Preserve 03C performance
and projection work. Do not identify running executables from source HEAD alone.

## Open boundaries

General diagram inventory for hidden/unallocated components; full V8 Body-only /
Full-edge shared post-port Content/Overlay and input/output zones; threshold undo /
compact lifecycle; wider physical DND. No completion claim for these in 03E1/03E2.

## Gary gate

Use E:\apps\github\upp_Ui (literal underscore, no backslash before it), clean main.
Require latest published checkpoint as ancestor. Run the established Debug runner
with -RequiredAncestor <SHA> -Launch. Read the overlay contract for manual checks.
All prior required summaries and the new OVERLAY summary must pass with positive
counts. No fixed generated-code hash: the new preset legitimately changes it.
Record exact HEAD, executable path/SHA-256, all summaries, manual result, evidence
path, PID and clean worktree. Stop at first real failure. Minor mechanical fixes
only, reviewed/documented/published and retested. No weakened tests, retired matrix,
Release/broad benchmark or rich-Micro fallback. Publish coherent reviewed slices.
