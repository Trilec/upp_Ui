# Workspace 02D — recovery and focused validation

The V7 workspace was published at `fbd283aba2c9deb0f02a855ce7b008c0283a12f2`
before the interrupted session ended. This checkpoint hardens its diagram/table
interaction; it does not restart or replace that implementation.

## What to inspect

Empty-region targets now wrap into an authoring-only shelf. The shelf does not
allocate node content; node-region rectangles still come from production. A
port-label lane is not a diagram drop target even when it shares a Content column.
Hidden Stable chips stay selectable but are individually grey. The table retains
readable column widths and gains a native horizontal scrollbar, including LOD3
hit testing after scrolling. LOD cells do not initiate component moves. Palette
DND explicitly disarms UiButton's click state before starting its modal drag.

The coloured On/Off/Auto cells describe authored inclusion, not proof that a
component has visible pixels. Actual representation still comes from the preview.
Native view tests cover these coordinates, selection and routing contracts.
They do NOT simulate Windows drag negotiation or prove screenshot parity.

## Gary task — accumulated gate, runner added in 02V

Repo: `E:\apps\github\upp_Ui`, branch `main`.
Require the supervisor's latest published checkpoint as an ancestor of tested
HEAD, not exact HEAD equality. The runner also requires the V7 source checkpoint
`fbd283aba2c9deb0f02a855ce7b008c0283a12f2` (02C).
Preserve unrelated work; stop on a dirty checkout or failed fast-forward.

First fetch/pull clean main so the new runner exists. Then run in PowerShell:

```powershell
Set-Location E:\apps\github\upp_Ui
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ValidateUiGraphWorkspace.ps1 `
    -RequiredAncestor '3f9957c7a332312d6d6d3f92bb6206344cd2373e' -Launch
if ($LASTEXITCODE) { throw 'Workspace Debug gate failed; inspect its evidence directory' }
```

The example requires the 02D source checkpoint. Use the supervisor's newer
published SHA when supplied. The runner's default ancestor is also 02D. It permits current main
to advance as long as the requested ancestor remains present.

Established defaults: `E:\upp-18468\umk.exe`, `CLANGx64`, repository/sibling
`upp_statemachine` and `upp_animation` nests plus the installed `uppsrc`.
`-UppRoot` and `-Method` accept the already validated local equivalents; do not
switch compiler/framework versions silently. Debug only: `-b`, no `-r`.
The per-process execution-policy option does not change machine policy.

The script performs:

1. Clean-main check, fetch/fast-forward, no unpublished local commits, ancestry
   verification and exact tested HEAD capture.
2. Build/run `Utilities/UiGraphRenderTests`; require its passing suite summary.
3. Build/run `Utilities/UiGraphWorkspaceTests` with `--export=<fixture path>`;
   require its passing summary and nonempty generated C++.
4. Build that unchanged generated C++ in a temporary package depending on only Ui
   and CtrlLib. It does NOT reproduce the export manually or include the authoring
   package. Record and recheck the generated-source SHA-256.
5. Build `examples/UiGraphComponentStudio` and run `--view-tests` with a 120-second
   timeout. Only a test process started by this invocation can be stopped.
6. Require exit=0 and `UIGRAPH_WORKSPACE_VIEW_SUMMARY ... failed=0` in the native
   log. Missing evidence stops the gate rather than silently passing it.
7. Check `git diff --check`, unchanged HEAD and clean worktree; with `-Launch`,
   open the workspace for manual checks and report its PID.

Evidence is retained under a unique directory in
`%TEMP%\UiGraphWorkspace-validation`, printed at startup. `-OutputRoot` changes
the parent evidence directory. Read `summary.txt`, individual build/test logs,
`native-view-test.log` and the actual generated fixture. No files are automatically
committed. Nothing from this temporary package should be published.

RenderTests currently has eight suites. All suites must pass; later added suites
are permitted. WorkspaceTests and native view checks must report failed=0 with
positive check counts. A build-only PASS is not a runtime PASS. Compiling the export
is not proof of host integration. Debug workspace startup additionally logs
`UIGRAPH_WORKSPACE_UI_SMOKE`; record that result during launch inspection.

### Manual acceptance

Drag palette -> region diagram, overlay diagram and structure row; cancel with
Escape and release over the source (no duplicate addition). Move and reorder
components. Scroll the table horizontally and edit LOD3. Resize to reach all '+'
targets. Hidden Stable chips stay individually grey and selectable. Port-lane and
inherited-scope drops must reject without document mutation.

Check selection across diagrams/table/preview; PropertyEditor commit/cancel and
style overrides; real colours and all seven painted kinds; Open/Save/Save As/Clone;
invalid import preserving work; independent layout/style detachment/reset; threshold
versus camera independence; 1:1/LOD jumps and native Micro hints. Compare actual
Windows rendering to the user's V7 mockup. Do not build retired DesignMatrix.

### Stop, edits and evidence

Stop at the first real compile/assertion/runtime failure or missing required test
evidence. First Windows execution also validates this new PowerShell runner; it
has only been source-reviewed in the implementation environment.

Minor mechanical CLANG/script fixes may be reviewed, documented, committed and
published. Refresh main first, include only your fixes, verify publication, and
rerun the gate on the resulting SHA. Stop for architecture/ownership failures;
no weakened tests, restored retired fields, or rich-Micro fallback. No Release,
broad suite or 10k benchmark without a focused reason.

Report exact tested HEAD and ancestry; each build/test summary; first real blocker;
generated-code compile/hash result; native view and startup smoke; manual findings;
evidence directory; demo PID; git diff --check; worktree clean YES/NO; any fix SHA.
