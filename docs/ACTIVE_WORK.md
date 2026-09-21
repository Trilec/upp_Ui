# ACTIVE WORK

Remote main is authoritative. Fetch before editing/publishing; never force-push.

BASE: `1fa8793c1b3ad440d86be9a2007f0825d0650275` / main
TASK: **UIGRAPH-NODE-WORKSPACE-03D1 — exclusive code rail and selected mode**
TOUCHED: `examples/UiGraphComponentStudio/WorkspaceWindow.{h,cpp}`; this file.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for the rail slice. **The Media title regression remains FAIL at the validated baseline.**
PUBLISHED: the commit containing this record; recover with `git log -1 -- examples/UiGraphComponentStudio/WorkspaceWindow.cpp`.
VALIDATION: complete touched source fetched at BASE and reconstructed with matching Git blob hashes; complete local diff / git diff --check PASS. Native U++/Windows builds and new smoke checks not run here.
NEXT ACTION: repair the actual Media header capacity regression without weakening its two failing tests; implement the user-approved ellipse bands in production, then run the accumulated Debug gate. See the boundaries below.

## 03D1 — implemented source

The rail previously registered PropertyEditor and code as two expanding children
of UiBoxLayout. RebuildInspector hid one, but UiBoxLayout::Layout explicitly shows
participating children again. Both reappeared and split the height. A single
ParentCtrl page host now owns those alternatives. Code occupies the complete host
below its Copy/Save toolbar; the hidden PropertyEditor cannot consume flow space.
No PropertyEditor library behaviour is changed.

The four mode buttons retain the existing checked-state contract and now have a
distinct pressed face/frame/icon under Minimal, with no pressed content offset.
Selection is independent of keyboard focus. Debug startup smoke exercises repeated
Code/Inspector/Style switches, exclusive visibility, code bounds and checked state.
The malformed zoom suffix is separated from the numeric Format conversion.

## Latest Windows evidence — user/Gary report, 03B + 03C

Tested `1fa8793c1b3ad440d86be9a2007f0825d0650275`. Required ancestry PASS.
RenderTests: eight suites/zero failures. WorkspaceTests: 33 checks/zero failures.
Generated C++ compiled unchanged with Ui/CtrlLib only; SHA-256:
`5058C4DB324F6D2A2F30DD889E823FF9AABFDAC6606E8168353828F06DD868D3`.
Startup smoke 1/0. Native views **45 checks, 2 FAIL**: Media title readability with
an enlarged subtitle and two icons failed for both Rectangle and Ellipse.
Other native Delete/Undo/focus/topology/proxy-hit checks passed. The gate stopped;
manual acceptance and normal launch were not performed. No fix commits or new PID.
Diff check PASS, tree clean. Evidence supplied by Gary:
`C:\Users\admin\AppData\Local\Temp\UiGraphWorkspace-validation\20260921-124117-1fa8793c1b3a-208e3d`.

Curt's screenshot also shows the code rail/selection defect and unchanged ellipse
bands. Do not infer which executable produced that screenshot from the source SHA;
the failed gate did not launch a new normal workspace. The source defects still
need repair, not an assumption that a stale running process explains them all.

## Architecture and previous checkpoints

03A `8f9a49a8367f0dd9d95d1085582e9348c5aa91db`: side-default icon insertion,
persistent inspector updates; Gary passed Debug and manual icon/typography/scope
checks. Held-button Escape during native DND remains unverified.
03B `e02bbd0845f154e417f242088db93cf1f75da446`: focused Delete/Undo, proxy picking,
height-fit text and outcome summaries. Its two Media integration tests now FAIL.
03C `1fa8793c1b3ad440d86be9a2007f0825d0650275`: camera-baseline admission fix and
matched 10k component diagnostics. Preserve that work; no performance acceptance
is implied by the correctness results. Read UIGRAPH_PERFORMANCE_AUDIT.md.

NodeGeometry.presentation remains the only evaluated layout authority. Shared
bounded C++ templates, no runtime JSON compiler and no ordinary per-node Ctrl tree.
Content and Overlay are sibling layers; labelled ports are graph-owned reservations.
Camera edits never resize authored nodes. Generated C++ uses Ui without authoring.
Family JSON saves layout/style with independent shape inheritance.

## Next source slices / acceptance

1. Diagnose Media allocation and actual font metrics; preserve both failing
   integration assertions, the configured readable floor and authored-font tests.
   Do not merely accept a proxy or silently enlarge the specimen to pass.
2. Implement the approved shape-aware bands rather than only documenting them.
   Audit clipping, picking, projection, ports, JSON/export and containment. Keep
   conservative fallback and the Micro/performance boundary; no extra layout cache.
   The latest user request explicitly asks for this previously deferred capability.
3. Continue the outstanding template-driven diagram inventory and V8 Body-only /
   Full-edge post-port interior/zone controls. Do not claim these delivered by D1.

Read: this file, UIGRAPH_WORKSPACE_03B.md, UIGRAPH_WORKSPACE_V8_AUDIT.md,
UIGRAPH_PERFORMANCE_AUDIT.md, UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md and current source.
The active app is examples/UiGraphComponentStudio; DesignMatrix stays retired.

## Validation

Use clean current main at E:\apps\github\upp_Ui and require the latest supervisor
checkpoint as ancestor, not exact equality. Run scripts/ValidateUiGraphWorkspace.ps1
-RequiredAncestor <SHA> -Launch with U++ 18468 / CLANGx64 / Debug. Do not weaken gates.
Until the text failure is repaired, a rail-only publish does NOT imply gate PASS.
Capture all summaries and first blocker; manual code-page full height/highlight,
Media text, ellipse geometry, Delete/Undo and inspector-scroll checks follow PASS.
Minor mechanical fixes only for Gary, with reviewed diff / docs / publish / retest.
No force-push, broad suite or new benchmark without the existing focused reason.
