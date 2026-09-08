# UIGRAPH-EXECUTION-CONSOLIDATION — Windows validation task

OBJECTIVE: Validate the latest published consolidation without losing 10k speed.
REPO/PATH: Trilec/upp_Ui; run from the local repository root.
BRANCH: performance/uigraph-execution-consolidation-20260908
REQUIRED SHA: Fetch this branch and record its current HEAD as the validation SHA.
Confirm 866fedf0b76f50448feaf356b64b608bb7d0f6a5 is an ancestor of HEAD; also require
this task and UiGraphDemoObservation.cpp at HEAD. Validate latest, not a stale snapshot.
IN SCOPE: Compile/run this Graph dependency slice and report regressions.
OUT OF SCOPE: Architecture changes, new optimisations, API redesign or merging main.

PREPARATION: Start from a clean checkout. Stop if there is unrelated local work.
Use the installed, previously validated U++ compiler/build method. Supply its actual
umk executable, uppsrc directory and build-method name/path when prompted below;
these machine-specific paths are deliberately not guessed.

PowerShell commands (from repo root):

```powershell
$ErrorActionPreference = 'Stop'
if (git status --porcelain) { throw 'Working tree is not clean' }
git fetch origin
if ($LASTEXITCODE) { throw 'Fetch failed' }
git switch performance/uigraph-execution-consolidation-20260908
if ($LASTEXITCODE) { throw 'Branch switch failed' }
git merge --ff-only origin/performance/uigraph-execution-consolidation-20260908
if ($LASTEXITCODE) { throw 'Fast-forward failed' }
git merge-base --is-ancestor 866fedf0b76f50448feaf356b64b608bb7d0f6a5 HEAD
if ($LASTEXITCODE) { throw 'Required structural checkpoint missing' }
if (!(Test-Path examples/UiGraphDemo/UiGraphDemoObservation.cpp)) { throw 'Final checkpoint missing' }
$validationSha = git rev-parse HEAD
$repo = (Get-Location).Path
$umk = Read-Host 'Full path to the installed umk.exe'
$uppsrc = Read-Host 'Full path to the installed uppsrc directory'
$method = Read-Host 'Previously validated U++ build-method name or .bm path'
if (!(Test-Path $umk) -or !(Test-Path "$uppsrc/Core/Core.upp")) { throw 'Toolchain paths invalid' }
$assembly = "$repo,$repo/Utilities,$repo/examples,$uppsrc"
$output = Join-Path $env:TEMP "uigraph-validation-$validationSha"
New-Item -ItemType Directory -Force $output | Out-Null
$tests = @('UiGraphModelTests','UiGraphViewTests','UiGraphRenderTests',
           'UiGraphScaleTests','UiNodeGraphPanProfileTest',
           'UiNodeGraphPerformanceTest','UiNodeGraphPresentationTest')
foreach ($config in @('Debug','Release')) {
    $flags = if ($config -eq 'Release') { '-br' } else { '-b' }
    foreach ($package in ($tests + @('UiGraphDemo'))) {
        $exe = Join-Path $output "$package-$config.exe"
        & $umk $assembly $package $method $flags $exe
        if ($LASTEXITCODE) { throw "Build failed: $package $config" }
        if ($package -ne 'UiGraphDemo') {
            & $exe
            if ($LASTEXITCODE) { throw "Tests failed: $package $config" }
        }
    }
}
Write-Host "Validated build/test SHA: $validationSha; binaries: $output"
```

UMK flags and comma-separated assembly syntax follow the official
[UMK documentation](https://www.ultimatepp.org/app$ide$umk$en-us.html).
Capture compiler output and every test summary. Existing suites must report zero
failures; the new UIGRAPH_EXECUTION_PATH_SUMMARY must report checks=8 failed=0.
These commands have not been run in the implementation environment.

MANUAL CHECKS: Launch UiGraphDemo-Release.exe from the output directory.
- Compare BASE cc386585d0eb5287c0f096d01a70a79e7df42fd7 and this branch using the same
  compiler, window size, DPI and 10k fixture. Use a separate clean checkout for BASE.
- At L3 around zoom 0.20, record idle/zoom/middle-pan paint, geometry, node phases and
  micro/cache counters. Middle-pan must keep micro paint, details/ports=0 and
  content/text=0; valid retained coverage must avoid exact geometry rebuilding.
  No return to the former 550–700 ms rich node paint. Report repeated samples, not
  one favourable frame. Stop on a repeatable material slowdown versus BASE.
- Confirm stock silhouettes, corners, colours and all three route styles at overview
  and detailed scale; input port circles complete/tangent, edge Circle arrows intact.
- Smoke-test reference/10k switching, scope enter/exit, Fit, selection and embedded
  controls. Semantic drag/connect/marquee fallback is expected; pan fallback is not.
- Zoom/pan with diagnostics enabled, disabled, and hidden. After roughly 200 ms quiet,
  status must show current zoom in every case. Disabled/hidden diagnostics must not
  sample; visible enabled diagnostics may sample once after settling.
- Disable/hide diagnostics while an observation is pending: normal status must still
  update. After 2 seconds quiet, observe two 10-second CPU windows; no continuing
  diagnostics sample/repaint activity. Report paint path and readable fallback reason.

STOP CONDITIONS: Missing dependencies/toolchain, build error, assertion/test failure,
visual corruption, incorrect geometry/selection or repeatable performance regression.
Do not weaken tests or change architectural decisions to obtain a pass.

EDITS/COMMIT/PUSH: Very minor compile/typo fixups on this branch are allowed. Review
full diff, run git diff --check, update ACTIVE_WORK, commit/publish and verify remote.
No major coding, performance redesign, force-push, main changes or merge. Report the
first substantive issue with source location and reproduction instead.

EVIDENCE: Branch + validated SHA, compiler/configuration/DPI, build and test logs,
new suite summary, 10k BASE-versus-branch samples, visual/status/idle results, any minor
fixup SHA, remaining failures. Report PASS only for checks actually performed.
