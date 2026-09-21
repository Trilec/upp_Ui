# Focused Windows Debug gate. No source edits, commits, release builds or benchmarks.
# Output goes to a unique temporary directory unless OutputRoot is provided.
[CmdletBinding()]
param(
    [string]$Repo = '',
    [string]$UppRoot = 'E:\upp-18468',
    [string]$Method = 'CLANGx64',
    [ValidatePattern('^[0-9a-fA-F]{40}$')]
    [string]$RequiredAncestor = '8b2e53cb8229c49c5e42aa1a068dbeb46b0ce50a',
    [string]$OutputRoot = '',
    [switch]$Launch,
    [switch]$EvidenceSelfTest
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
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
# Every emitted record for the requested suite must pass. One passing record
# must not conceal a failure from another fixture/window in the same native log.
function Read-TestSummary([string]$Text, [string]$Name,
                          [string]$CountField = 'checks', [string]$FailureField = 'failed',
                          [int]$MinimumCount = 1) {
    $namePattern = [regex]::Escape($Name)
    $records = [regex]::Matches($Text, '\b' + $namePattern + '\b[^\r\n]*')
    if ($records.Count -eq 0) { throw "Missing summary: $Name" }
    $pattern = '^' + $namePattern + '[ \t]+' + [regex]::Escape($CountField) +
               '=([0-9]+)[ \t]+' + [regex]::Escape($FailureField) + '=([0-9]+)(?=[ \t]|$)'
    # Validate all records before writing any successful evidence to the pipeline.
    $valid = [System.Collections.Generic.List[string]]::new()
    foreach ($record in $records) {
        $match = [regex]::Match($record.Value, $pattern)
        if (!$match.Success) { throw "Malformed summary: $($record.Value)" }
        if ([long]$match.Groups[1].Value -lt $MinimumCount -or [long]$match.Groups[2].Value -ne 0) {
            throw "Failed or incomplete summary: $($record.Value)"
        }
        $valid.Add($match.Value)
    }
    return $valid.ToArray()
}
function Require-Summary([string]$Label, [string]$Name,
                         [string]$CountField = 'checks', [string]$FailureField = 'failed',
                         [int]$MinimumCount = 1) {
    $text = Get-Content -LiteralPath (Join-Path $run ($Label + '.log')) -Raw
    foreach ($line in @(Read-TestSummary $text $Name $CountField $FailureField $MinimumCount)) {
        $report.Add($line)
    }
}
function Test-EvidenceReader {
    $cases = @(
        @{ Text = 'SUITE checks=7 failed=0'; Pass = $true },
        @{ Text = "prefix SUITE checks=7 failed=0`r`nSUITE checks=9 failed=0"; Pass = $true },
        @{ Text = ''; Pass = $false },
        @{ Text = 'OTHER checks=7 failed=0'; Pass = $false },
        @{ Text = 'SUITE checks=0 failed=0'; Pass = $false },
        @{ Text = 'SUITE checks=7 failed=2'; Pass = $false },
        @{ Text = "SUITE checks=7 failed=0`nSUITE checks=7 failed=2"; Pass = $false },
        @{ Text = "SUITE checks=7 failed=2`nSUITE checks=7 failed=0"; Pass = $false },
        @{ Text = 'SUITE checks=7 failed=0x'; Pass = $false },
        @{ Text = "SUITE checks=7 failed=0`nSUITE checks=? failed=0"; Pass = $false }
    )
    foreach ($case in $cases) {
        $accepted = $true
        try { Read-TestSummary $case.Text 'SUITE' | Out-Null }
        catch { $accepted = $false }
        if ($accepted -ne $case.Pass) { throw 'Evidence reader self-test failed' }
    }
    Read-TestSummary 'SUITE suites=9 failed_suites=0' 'SUITE' 'suites' 'failed_suites' 9 | Out-Null
    $accepted = $true
    try { Read-TestSummary 'SUITE suites=8 failed_suites=0' 'SUITE' 'suites' 'failed_suites' 9 | Out-Null }
    catch { $accepted = $false }
    if ($accepted) { throw 'Evidence reader accepted an incomplete render suite' }
    return 'UIGRAPH_EVIDENCE_READER_SUMMARY checks=12 failed=0'
}

# Parser-only fixtures: no repository lookup, network, compiler or GUI process.
if ($EvidenceSelfTest) {
    Write-Output (Test-EvidenceReader)
    return
}

try {
    $report.Add((Test-EvidenceReader))
    if ([string]::IsNullOrWhiteSpace($Repo)) {
        $Repo = Split-Path -Parent $PSScriptRoot
    }
    $Repo = (Resolve-Path -LiteralPath $Repo).Path
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
    Require-Summary 'render-test' 'UIGRAPH_RENDER_TESTS_SUMMARY' 'suites' 'failed_suites' 9
    Require-Summary 'render-test' 'UIGRAPH_ELLIPSE_BANDS_SUMMARY'

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
    Require-Summary 'authoring-test' 'UIGRAPH_WORKSPACE_SUMMARY'
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
    $studioHash = (Get-FileHash -LiteralPath $studio -Algorithm SHA256).Hash
    $report.Add("WORKSPACE EXE: $studio")
    $report.Add("WORKSPACE SHA256: $studioHash")
    $nativeLog = [IO.Path]::ChangeExtension($studio, '.log')
    $p = Start-Process -FilePath $studio -ArgumentList '--view-tests' -WorkingDirectory $run -PassThru
    $timedOut = !$p.WaitForExit(120000)
    if ($timedOut) {
        # Only this runner's test process is stopped; existing demos are untouched.
        Stop-Process -Id $p.Id -ErrorAction SilentlyContinue
        $p.WaitForExit(5000) | Out-Null
    }
    $p.Refresh()
    $exitCode = if ($p.HasExited) { $p.ExitCode } else { $null }
    $report.Add("NATIVE TEST EXIT: $exitCode; TIMEOUT: $timedOut")

    # Preserve evidence BEFORE judging exit/summary status. The first failing
    # Media check and its capacity diagnostics must survive a stopped gate.
    if (Test-Path -LiteralPath $nativeLog) {
        Copy-Item -LiteralPath $nativeLog -Destination (Join-Path $run 'native-view-test.log')
        $text = Get-Content -LiteralPath $nativeLog -Raw
        foreach ($m in [regex]::Matches($text, '\bUIGRAPH_[A-Z0-9_]*(?:SUMMARY|SMOKE)\b[^\r\n]*')) {
            $report.Add('OBSERVED: ' + $m.Value) # observed is not a passing verdict
        }
        $firstFailure = [regex]::Match($text, '(?m)^[^\r\n]*(?:FAIL:|UIGRAPH_[A-Z0-9_]*FAILURE)[^\r\n]*')
        if ($firstFailure.Success) { $report.Add('FIRST NATIVE FAILURE: ' + $firstFailure.Value) }
    }
    if ($timedOut) { throw 'View tests exceeded 120 seconds; inspect native-view-test.log/native application log' }
    if ($null -eq $exitCode -or $exitCode -ne 0) { throw "Native view tests failed (exit $exitCode); inspect native-view-test.log/native application log" }
    if (!(Test-Path -LiteralPath $nativeLog)) {
        throw 'Native summary log not found beside executable; retrieve the U++ log before claiming the gate passed'
    }
    Require-Summary 'native-view-test' 'UIGRAPH_WORKSPACE_VIEW_SUMMARY'
    Require-Summary 'native-view-test' 'UIGRAPH_WORKSPACE_BAND_UI_SUMMARY'
    Require-Summary 'native-view-test' 'UIGRAPH_WORKSPACE_UI_SMOKE'
    $report.Add('PASS native-view-test exit=0; view, band integration and startup evidence present')
    if ((Get-FileHash -LiteralPath $studio -Algorithm SHA256).Hash -ne $studioHash) {
        throw 'Workspace executable changed during validation'
    }

    Git-Read -Arguments @('diff', '--check') | Out-Null
    if ((Git-Read -Arguments @('rev-parse', 'HEAD')) -ne $head) { throw 'Checkout changed while tests ran; evidence must identify one HEAD' }
    $dirty = @(Git-Read -Arguments @('status', '--porcelain'))
    $report.Add('git diff --check: PASS')
    $report.Add('WORKTREE CLEAN: ' + $(if ($dirty.Count) { 'NO' } else { 'YES' }))
    if ($dirty.Count) { throw 'Validation left worktree changes; inspect before claiming completion' }
    if ($Launch) {
        $demo = Start-Process -FilePath $studio -WorkingDirectory $run -PassThru
        $report.Add("WORKSPACE PID: $($demo.Id) - manual validation pending")
        $report.Add("MANUAL REVIEW EXE: $studio / SHA256 $studioHash")
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
