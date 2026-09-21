# ACTIVE WORK

Remote main is authoritative. Fetch before editing/publishing; never force-push.

BASE: `35a2cf89665a4800d3a706466cf7c26c0661083b` / main
TASK: **UIGRAPH-NODE-WORKSPACE-03D2 — readable capacity across natural text rows**
TOUCHED: `Ui/UiGraph/UiNodeGraphPresentation.inc`; this file.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for D2 source. Baseline native gate was FAIL; no newer native PASS is claimed.
PUBLISHED: the commit containing this update; recover via `git log -1 -- Ui/UiGraph/UiNodeGraphPresentation.inc`.
VALIDATION: full pinned original matched blob `87007890a90036cd406f9236cbe8dbeaa90106a4`; complete diff reviewed and git diff --check PASS. Local capacity-arithmetic cases PASS (4,950 combinations), NOT U++ or font-backend validation. Existing failing native integration assertions are unchanged. No Windows execution here.
NEXT ACTION: implement the approved ellipse-band capacity and its clipping/export/authoring integration, then run Gary's accumulated Debug correctness gate. Preserve the D2 and 03B assertions; do not accept a proxy to hide a text failure.

## D2 source correction

03B fitted a Title only after earlier natural Top rows had consumed the header.
Its two Media integration cases still failed, although isolated fitting tests passed.
The allocator now resolves each participating named rich component once and makes
one reverse, eight-region minimum-height pass. Natural single-line Ellipsis
Top/Bottom rows yield excess height to later readable text in the same region.
Minimum heights use each component's actual font at its existing final-pixel floor;
explicit extents, Clip/Wrap and component identities/order remain authored.
Hidden Reflow and missing data do not request capacity. Stable reservations still
participate. Outer node/header dimensions and LOD masks are unchanged.

The fixed 16-entry preparation scratch is transient, not retained state. Micro
keeps its lazy source resolution and never enters this font-metric/budget pass.
Paint still consumes retained output. If combined minima do not fit, ordinary
capacity/proxy diagnostics remain; no unreadable font or overlapping slot is forced.
Native font behaviour remains the validation boundary, not a claimed PASS.

## D1 source, already published

`35a2cf89665a4800d3a706466cf7c26c0661083b`: code/inspector alternatives share one
ParentCtrl host. UiBoxLayout can no longer resurrect the hidden PropertyEditor and
split the rail in half. Code fills the host below Copy/Save. Checked mode buttons
have distinct face/frame/icon; zoom caption formatting is repaired. Startup smoke
now checks repeated rail page switching, visibility and code bounds (5 checks).

## Latest Windows evidence — Gary report at 1fa8793

`1fa8793c1b3ad440d86be9a2007f0825d0650275`: ancestry PASS; RenderTests 8 suites/0;
WorkspaceTests 33/0; unchanged generated C++ compiled with Ui/CtrlLib only, hash
`5058C4DB324F6D2A2F30DD889E823FF9AABFDAC6606E8168353828F06DD868D3`.
Startup smoke 1/0. Native views **45 checks / 2 FAIL**, both Media title readability
cases (Rectangle/Ellipse, enlarged subtitle, two icons). Other Delete/Undo/focus/
proxy-hit checks passed. Gate stopped: no manual checks or normal launch/new PID.
Diff check PASS; tree clean; no fixes. Evidence:
`C:\Users\admin\AppData\Local\Temp\UiGraphWorkspace-validation\20260921-124117-1fa8793c1b3a-208e3d`.
Do not identify a screenshot's executable version solely from the tested source SHA.

## Architecture / recovery

Active app: examples/UiGraphComponentStudio. DesignMatrix stays retired.
NodeGeometry.presentation is the sole retained layout authority. Shared bounded
C++ templates; no runtime JSON compiler, second layout cache or per-node Ctrl tree.
Content/Overlay are siblings. Port identities/connections stay model-owned.
Family JSON saves layout/style with independent shape inheritance; generated C++
requires Ui, not authoring. Camera changes never resize authored nodes.

03A `8f9a49a8367f0dd9d95d1085582e9348c5aa91db` was Windows-validated; held-button
Escape DND remains unverified. 03B `e02bbd0845f154e417f242088db93cf1f75da446`
contains Delete/text-fit/tests; its Media failure motivates D2. 03C `1fa8793...`
contains camera-admission correction and matched 10k diagnostics: preserve it;
performance acceptance is still pending. Read UIGRAPH_PERFORMANCE_AUDIT.md.

Read current source and docs/UIGRAPH_WORKSPACE_03B.md, UIGRAPH_WORKSPACE_V8_AUDIT.md,
UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md, UIGRAPH_WORKSPACE_AUTHORING.md and runtime guide.
Latest user explicitly requests implementation of previously deferred ellipse bands.
Outstanding: those bands, complete V8 Body-only/Full-edge post-port interior and
zone controls, template-driven diagram inventory, palette glyphs, threshold undo/
compact-size lifecycle and full Save/Open/DND manual follow-through.

## Validation

Clean current main at E:\apps\github\upp_Ui; require latest supervisor SHA as an
ancestor. Run scripts/ValidateUiGraphWorkspace.ps1 -RequiredAncestor <SHA> -Launch
with established U++ 18468 / CLANGx64 / Debug. All existing assertions stay enabled.
Require passing RenderTests, WorkspaceTests, unchanged generated C++, native view
and startup summaries before manual Media text, code rail/highlight and geometry.
Report exact HEAD, first blocker, summaries, evidence, PID and clean tree.
Gary may make reviewed mechanical fixes only; document/publish/verify/retest them.
No architectural redesign, guard removal, weakened tests or rich-Micro fallback.
