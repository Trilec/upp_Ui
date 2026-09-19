# ACTIVE WORK

Remote main is authoritative. Refresh before editing/publishing; never force-push.

BASE: `0520f452aab5f2b7028fc0ebc597d7cfcdfe1f5f` / main
TASK: **UIGRAPH-NODE-WORKSPACE-03A — user-reported authoring regressions**
TOUCHED: `Utilities/UiGraphWorkspace/WorkspaceDocument.cpp`; `examples/UiGraphComponentStudio/{WorkspaceInspector.cpp,WorkspaceWindow.h,WorkspaceViewTests.cpp}`; this file; `docs/UIGRAPH_WORKSPACE_V8_AUDIT.md`.
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING** for 03A only.
PUBLISHED: the commit containing this update; recover with `git log -1 -- examples/UiGraphComponentStudio/WorkspaceInspector.cpp`.
VALIDATION: complete touched files reconstructed from pinned GitHub and original blob hashes matched; full local diff reviewed; git diff --check PASS. Windows compilation/new tests NOT run here.
NEXT ACTION: continue the bounded diagram-inventory/diagnostic fix, then validate current main with Gary. Read the V8 audit before changing port reservation semantics.

## Latest changes / boundaries

03A fixes the authoring default which stacked new icons as 28-unit Top rows in a
42-unit Media header. New icons default Left; existing saved/moved placement is
not silently migrated. Ordinary property commits now update dependent values in
the existing PropertyEditor model instead of detaching/recreating its rows. Only
a renderer-kind/schema change needs a guarded rebuild. Typography is immediately
after component identity, with explicit inherited-scope guidance and disabled-edit
rejection. Native regressions cover two icons Normal/LOD1/Normal, repeated label
commits, typography and inheritance. Tests are added, not yet native PASS.

Still pending: diagram tags for unallocated/reflow-hidden components, readable
capacity/missing-data diagnostics, malformed zoom-label formatting, V8 left-panel
port controls and the Body-only/Full-edge reservation refinement. Do not claim
these fixed by 03A. See `docs/UIGRAPH_WORKSPACE_V8_AUDIT.md`.

## Latest Windows validation — reported by Gary

Tested `0520f452aab5f2b7028fc0ebc597d7cfcdfe1f5f`; current remote matched that HEAD.
RenderTests: 8 suites, failed_suites=0. WorkspaceTests: checks=33, failed=0.
Generated C++ compiled unchanged with Ui/CtrlLib only, SHA-256
`5058C4DB324F6D2A2F30DD889E823FF9AABFDAC6606E8168353828F06DD868D3`.
Component Studio Debug build PASS. View tests: checks=18, failed=0.
UI smoke: checks=1, failed=0. Diff check PASS; worktree clean; retired DesignMatrix
not built. Mechanical fixes 94597a9, 7347212, 97d36b2, 0520f45 are included.

Manual evidence: shape change and Thumbnail selection synchronized guides, table,
preview and inspector; LOD3 reachable. Physical drag negotiation and full Save/Open
workflows were NOT completed. Curt subsequently reported header icons missing at
Normal, missing diagram inventory, hard-to-find typography and inspector scroll
reset. Passing automated tests did not cover those interactions. The 03A source
is newer than this Windows evidence and requires validation.

## Read first

1. This file and `docs/UIGRAPH_WORKSPACE_V8_AUDIT.md`.
2. `docs/UIGRAPH_NODE_WORKSPACE.md` — active app and current limitations.
3. `docs/UIGRAPH_WORKSPACE_AUTHORING.md` — family inheritance, strict JSON/export.
4. `docs/UIGRAPH_WORKSPACE_RUNTIME.md` and current full source.
5. `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md` — retained layout authority.

Active app: `examples/UiGraphComponentStudio`. DesignMatrix is retired. V8 is the
latest user reference; V7/four-preview notes are historical direction, not a reason
to restore the old app. HTML remains a design reference, not runtime source/schema.

## Architecture

NodeGeometry.presentation remains the sole evaluated per-node layout cache.
Content and Overlay are sibling layers; component identity, inclusion and actual
representation are separate. No per-node Ctrl tree or runtime JSON compiler.
Seven palette kinds are painted. Actions are not arbitrary live controls; a real
Ctrl uses the existing host-owned SetNodeCtrl contract.

One family JSON saves Base layout and appearance plus independent shape snapshots.
Component styling currently travels with its layout definition; family appearance
has separate inheritance. Generated C++ separates factories and uses only Ui.
Camera edits never resize authored nodes. Port topology/IDs remain model-owned.

## Durable source checkpoints

- 14551a3aa933a1e6e65296e7acb1d82bb401ac0f: 02A registered templates, painted kinds, Micro hints.
- acb1b8b5b8029996316ba391bac8830e07c63ca9: 02B authoring JSON/inheritance/export.
- fbd283aba2c9deb0f02a855ce7b008c0283a12f2: 02C V7 workspace, old Studio retirement.
- 3f9957c7a332312d6d6d3f92bb6206344cd2373e: 02D diagram/table interaction hardening.
- 4873b1965d97751a5671162d85b439cc0f8748bc: repeatable Debug runner.
- 0520f452aab5f2b7028fc0ebc597d7cfcdfe1f5f: Gary's mechanically corrected, validated baseline.

## Gary gate

Clean current main at E:\apps\github\upp_Ui. Require latest supervisor SHA as an
ancestor, not exact equality. Run `scripts/ValidateUiGraphWorkspace.ps1
-RequiredAncestor <SHA> -Launch`; full task in UIGRAPH_WORKSPACE_02D_VALIDATE.md.
Debug only. RenderTests, WorkspaceTests, unchanged generated C++, workspace and
--view-tests. The native view check count grows; require all checks passing.

Specifically retest Media + two new icons at Normal/LOD1/Normal; selected text
font/colour; repeated Show port labels edits without selection/scroll loss;
keyboard and physical drag cancellation; inherited scope. Do not weaken tests or
change authored LOD masks to conceal missing capacity. Minor mechanical fixes may
be reviewed/committed/published; stop for ownership/architecture failures.

Report exact HEAD, ancestry, summaries, first blocker, manual evidence, PID,
evidence directory, diff check, clean worktree and any fix SHA. No Release, broad
suite or 10k benchmark without a focused reason. Continue coherent publications
with this recovery record updated each time.
