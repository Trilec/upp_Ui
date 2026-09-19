# Focused Windows Debug gate. No source edits, commits, release builds or benchmarks.
# Output goes to a unique temporary directory unless OutputRoot is provided.
[CmdletBinding()]
param(
    [string]$Repo = (Split-Path -Parent $PSScriptRoot),
    [string]$UppRoot = 'E:\upp-18468',
    [string]$Method = 'CLANGx64',
    [ValidatePattern('^[0-9a-fA-F]{40}$')]
    [string]$RequiredAncestor = '3f9957c7a332312d6d6d3f92bb6206344cd2373e',
    [string]$OutputRoot = '',
    [switch]$Launch
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$Repo = (Resolve-Path -LiteralPath $Repo).Path
$oldLocation = Get-Location
$report = [System.Collections.Generic.List[string]]::new()
$run = $null

function Git-Read([string[]]$Arguments) {
    $result = & git @Arguments
    if ($LASTEXITCODE -ne 0) { throw "git $($Arguments -join ' ') failed" }
    return $result
}
function Native-Checked([string]$Executable, [string[]]$Arguments, [string]$Label) {
    if (!(Test-Path -LiteralPath $Executable -PathType Leaf)) {
        throw "Missing executable for $Label`: $Executable"
    }
    Write-Host "=== $Label ==="
    $log = Join-Path $run ($Label + '.log')
    # In Windows PowerShell, native compiler stderr can be ErrorRecord output
    # even when the native command succeeds. Judge the native exit status.
    $savedPreference = $ErrorActionPreference
    $code = $null
    try {
        $ErrorActionPreference = 'Continue'
        & $Executable @Arguments 2>&1 | Tee-Object -FilePath $log | Out-Host
        $code = $LASTEXITCODE
    }
    finally { $ErrorActionPreference = $savedPreference }
    if ($null -eq $code -or $code -ne 0) { throw "$Label failed (exit $code). Log: $log" }
    if (!(Test-Path -LiteralPath $log)) { throw "$Label did not produce its evidence log" }
    $report.Add("PASS $Label")
}
function Require-Summary([string]$Label, [string]$Pattern) {
    $text = Get-Content -LiteralPath (Join-Path $run ($Label + '.log')) -Raw
    if ($text -notmatch $Pattern) { throw "Missing passing summary in $Label.log" }
    $report.Add($Matches[0])
}

try {
    Set-Location -LiteralPath $Repo
    $runnerHash = (Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash
    if (((Git-Read -Arguments @('rev-parse', '--show-toplevel')) -replace '/', '\') -ne ($Repo -replace '/', '\')) {
        throw 'Repo must identify the repository root'
    }
    if (@(Git-Read -Arguments @('status', '--porcelain')).Count) { throw 'Dirty worktree: preserve existing work and stop' }
    if ((Git-Read -Arguments @('branch', '--show-current')) -ne 'main') { throw 'Use the clean main checkout, not another work branch' }
    Git-Read -Arguments @('fetch', 'origin') | Out-Host
    if ([int](Git-Read -Arguments @('rev-list', '--count', 'origin/main..HEAD')) -ne 0) {
        throw 'Local main has unpublished commits; do not validate an assumed remote snapshot'
    }
    Git-Read -Arguments @('pull', '--ff-only', 'origin', 'main') | Out-Host
    if ((Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash -ne $runnerHash) {
        throw 'Runner advanced during fetch/pull; restart it from the updated checkout'
    }
    foreach ($sha in @($RequiredAncestor, 'fbd283aba2c9deb0f02a855ce7b008c0283a12f2')) {
        Git-Read -Arguments @('merge-base', '--is-ancestor', $sha, 'HEAD') | Out-Null
    }
    $head = [string](Git-Read -Arguments @('rev-parse', 'HEAD'))
    $report.Add("TESTED HEAD: $head")
    $report.Add("RUNNER SHA256: $runnerHash")
    $report.Add("REQUIRED ANCESTOR: $RequiredAncestor PASS")

    $umk = Join-Path $UppRoot 'umk.exe'
    $uppsrc = Join-Path $UppRoot 'uppsrc'
    if (!(Test-Path -LiteralPath $umk) -or !(Test-Path -LiteralPath (Join-Path $uppsrc 'Core\Core.upp'))) {
        throw 'Established U++ toolchain not found; supply UppRoot, do not switch versions silently'
    }
    $parent = Split-Path -Parent $Repo
    $nests = @($Repo, (Join-Path $parent 'upp_statemachine'), (Join-Path $parent 'upp_animation'), $uppsrc)
    foreach ($nest in $nests) {
        if (!(Test-Path -LiteralPath $nest)) { throw "Missing established package nest: $nest" }
    }
    $assembly = $nests -join ','
    if (!$OutputRoot) { $OutputRoot = Join-Path ([IO.Path]::GetTempPath()) 'UiGraphWorkspace-validation' }
    $stamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $head.Substring(0, 12) + '-' + [guid]::NewGuid().ToString('N').Substring(0, 6)
    $run = Join-Path $OutputRoot $stamp
    New-Item -ItemType Directory -Path $run -Force | Out-Null
    $run = (Resolve-Path -LiteralPath $run).Path
    $report.Add("EVIDENCE: $run")
    $report.Add("TOOLCHAIN: $umk / $Method / Debug")
    Write-Host "TESTED HEAD: $head"
    Write-Host "Evidence directory: $run"

    $render = Join-Path $run 'UiGraphRenderTests.exe'
    Native-Checked -Executable $umk -Arguments @($assembly, 'Utilities/UiGraphRenderTests', $Method, '-b', $render) -Label 'render-build'
    Native-Checked -Executable $render -Arguments @() -Label 'render-test'
    Require-Summary 'render-test' 'UIGRAPH_RENDER_TESTS_SUMMARY\s+suites=[1-9]\d*\s+failed_suites=0\b'

    # Export from the actual authoring test executable, then compile the output
    # unchanged against Ui. The temporary package deliberately does not use the
    # authoring or PropertyEditor packages.
    $probeNest = Join-Path $run 'probe-nest'
    $probe = Join-Path $probeNest 'GeneratedWorkspaceProbe'
    New-Item -ItemType Directory -Path $probe -Force | Out-Null
    $generated = Join-Path $probe 'GeneratedFamily.cpp'
    $workspace = Join-Path $run 'UiGraphWorkspaceTests.exe'
    Native-Checked -Executable $umk -Arguments @($assembly, 'Utilities/UiGraphWorkspaceTests', $Method, '-b', $workspace) -Label 'authoring-build'
    Native-Checked -Executable $workspace -Arguments @("--export=$generated") -Label 'authoring-test'
    Require-Summary 'authoring-test' 'UIGRAPH_WORKSPACE_SUMMARY\s+checks=[1-9]\d*\s+failed=0\b'
    if (!(Test-Path -LiteralPath $generated) -or (Get-Item -LiteralPath $generated).Length -eq 0) {
        throw 'Authoring tests did not create the generated C++ fixture'
    }
    $generatedHash = (Get-FileHash -LiteralPath $generated -Algorithm SHA256).Hash
    $report.Add("GENERATED CPP SHA256: $generatedHash")
    $utf8 = [System.Text.UTF8Encoding]::new($false)
    [IO.File]::WriteAllText((Join-Path $probe 'GeneratedWorkspaceProbe.upp'), @'
description "Actual generated UiGraph C++ compile probe";
noblitz;
uses CtrlLib, Ui;
file GeneratedFamily.cpp, main.cpp;
mainconfig "GUI" = "GUI";
'@, $utf8)
    [IO.File]::WriteAllText((Join-Path $probe 'main.cpp'), "#include <Ui/Ui.h>`nusing namespace Upp;`nGUI_APP_MAIN {}`n", $utf8)
    $probeExe = Join-Path $run 'GeneratedWorkspaceProbe.exe'
    Native-Checked -Executable $umk -Arguments @("$probeNest,$assembly", 'GeneratedWorkspaceProbe', $Method, '-b', '+GUI', $probeExe) -Label 'generated-code-build'
    if ((Get-FileHash -LiteralPath $generated -Algorithm SHA256).Hash -ne $generatedHash) {
        throw 'Generated C++ changed during compilation'
    }
    $report.Add('Generated C++ compiled unchanged with Ui/CtrlLib only; host integration is not a runtime claim')

    $studio = Join-Path $run 'UiGraphComponentStudio.exe'
    Native-Checked -Executable $umk -Arguments @($assembly, 'examples/UiGraphComponentStudio', $Method, '-b', '+GUI', $studio) -Label 'workspace-build'
    $p = Start-Process -FilePath $studio -ArgumentList '--view-tests' -WorkingDirectory $run -PassThru
    if (!$p.WaitForExit(120000)) {
        Stop-Process -Id $p.Id -ErrorAction SilentlyContinue
        throw 'View tests exceeded 120 seconds; inspect the native log/assertion'
    }
    $p.Refresh()
    if ($p.ExitCode -ne 0) { throw "Native view tests failed (exit $($p.ExitCode)); inspect the application log" }
    $report.Add('PASS native-view-test exit=0')
    $nativeLog = [IO.Path]::ChangeExtension($studio, '.log')
    if (Test-Path -LiteralPath $nativeLog) {
        Copy-Item -LiteralPath $nativeLog -Destination (Join-Path $run 'native-view-test.log')
        $text = Get-Content -LiteralPath $nativeLog -Raw
        if ($text -notmatch 'UIGRAPH_WORKSPACE_VIEW_SUMMARY\s+checks=[1-9]\d*\s+failed=0\b') {
            throw 'Native view log did not contain its passing test summary'
        }
        $report.Add($Matches[0])
    }
    else { throw 'Native summary log not found beside executable; retrieve the U++ log before claiming the gate passed' }

    Git-Read -Arguments @('diff', '--check') | Out-Null
    if ((Git-Read -Arguments @('rev-parse', 'HEAD')) -ne $head) { throw 'Checkout changed while tests ran; evidence must identify one HEAD' }
    $dirty = @(Git-Read -Arguments @('status', '--porcelain'))
    $report.Add('git diff --check: PASS')
    $report.Add('WORKTREE CLEAN: ' + $(if ($dirty.Count) { 'NO' } else { 'YES' }))
    if ($dirty.Count) { throw 'Validation left worktree changes; inspect before claiming completion' }
    if ($Launch) {
        $demo = Start-Process -FilePath $studio -WorkingDirectory $run -PassThru
        $report.Add("WORKSPACE PID: $($demo.Id) - manual validation pending")
    }
    $report.Add('AUTOMATED DEBUG GATE: PASS - physical drag/drop and visual checks remain manual')
}
catch {
    $report.Add('FAIL: ' + $_.Exception.Message)
    throw
}
finally {
    if ($run) { $report | Set-Content -LiteralPath (Join-Path $run 'summary.txt') -Encoding UTF8 }
    $report | ForEach-Object { Write-Host $_ }
    Set-Location $oldLocation
}
