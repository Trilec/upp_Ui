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

## Gary task

Repo: `E:\apps\github\upp_Ui`, branch `main`.
Require the supervisor's latest published checkpoint as an ancestor of tested
HEAD; also require `fbd283aba2c9deb0f02a855ce7b008c0283a12f2` (02C).
Preserve unrelated work; stop on a dirty checkout or failed fast-forward.

Use existing U++ installation and build method. Debug only (no `-r`).
From the repository root in PowerShell:

```powershell
$ErrorActionPreference = 'Stop'
$umk = 'E:\upp-18468\umk.exe'
$assembly = 'E:\apps\github\upp_Ui,E:\apps\github\upp_statemachine,E:\apps\github\upp_animation,E:\upp-18468\uppsrc'
$out = Join-Path (Get-Location) 'build\workspace-debug'
New-Item -ItemType Directory -Force $out | Out-Null

& $umk $assembly Utilities/UiGraphRenderTests CLANGx64 -b "$out\UiGraphRenderTests.exe"
if ($LASTEXITCODE) { throw 'RenderTests build failed' }
& "$out\UiGraphRenderTests.exe"
if ($LASTEXITCODE) { throw 'RenderTests failed' }

& $umk $assembly Utilities/UiGraphWorkspaceTests CLANGx64 -b "$out\UiGraphWorkspaceTests.exe"
if ($LASTEXITCODE) { throw 'WorkspaceTests build failed' }
& "$out\UiGraphWorkspaceTests.exe" "--export=$out\GeneratedFamily.cpp"
if ($LASTEXITCODE) { throw 'WorkspaceTests failed' }

& $umk $assembly examples/UiGraphComponentStudio CLANGx64 -b +GUI "$out\UiGraphComponentStudio.exe"
if ($LASTEXITCODE) { throw 'Workspace build failed' }
$p = Start-Process "$out\UiGraphComponentStudio.exe" -ArgumentList '--view-tests' -PassThru -Wait
if ($p.ExitCode) { throw 'Native view tests failed; inspect application log' }
$p = Start-Process "$out\UiGraphComponentStudio.exe" -PassThru
"Workspace PID: $($p.Id)"
```

Check `UIGRAPH_WORKSPACE_VIEW_SUMMARY ... failed=0` in the executable's log.
Debug startup also runs these tests and the existing camera/threshold smoke.
RenderTests should have eight passing suites; WorkspaceTests must report failed=0.

Compile the exported `GeneratedFamily.cpp` as a temporary U++ package depending
only on Ui and CtrlLib; do not copy Utilities/UiGraphWorkspace into its dependencies.
Use a separate nest under the build output and a minimal main. The output file
must be the actual `--export` result, not a hand-edited equivalent. Do not publish
the temporary package or generated fixture.

Manual checks: drag palette -> diagram and table; cancel with Escape and release
over the source (no duplicate addition); scroll table horizontally and edit LOD3;
resize and reach every '+' target; move/reorder; grey hidden Stable chips remain
selectable; port-lane drops reject without document mutation. Also run the 02C
save/open, independent inheritance, property-editor and live-LOD checks recorded
in ACTIVE_WORK. Do not build retired UiGraphDesignMatrix.

Run `git diff --check`. Report exact HEAD, ancestry, build/test summaries, first
real blocker, generated-code compile result, visual findings, PID and worktree
clean YES/NO. Minor mechanical CLANG fixes may be reviewed/committed/published.
Stop for architecture/ownership failures; no weakening tests or enabling rich
Micro fallbacks. No Release/broad suite/10k benchmark without a focused reason.
