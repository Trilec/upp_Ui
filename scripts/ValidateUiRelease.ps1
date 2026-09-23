# Read-only validation driver. Build recipes come from the checked-in/supplied
# assembly, not a remembered E: drive layout. Default scope is deliberately small.
# This runner does not grant source/API, generated-code or visual acceptance.
[CmdletBinding()]
param(
    [string]$Repo = '',
    [string]$AssemblyFile = '',
    [string]$Umk = '',
    [string]$Method = 'CLANGx64',
    [ValidateSet('Surgical','Headers','Demos','Full')][string]$Profile = 'Surgical',
    [ValidateSet('Debug','Release','Both')][string]$Configuration = 'Debug',
    [ValidatePattern('^$|^[0-9a-fA-F]{40}$')][string]$RequiredAncestor = '',
    [string]$OutputRoot = '',
    [ValidateRange(5,3600)][int]$TimeoutSeconds = 120,
    [switch]$Blitz,
    [switch]$SelfTest
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:evidence = ''
$script:records = New-Object System.Collections.Generic.List[string]

function Record([string]$Text) {
    Write-Host $Text
    $script:records.Add($Text)
}
function Git-Read([string[]]$Arguments) {
    $result = @(& git -C $Repo @Arguments)
    if ($LASTEXITCODE) { throw "git failed: $($Arguments -join ' ')" }
    return (($result -join "`n").Trim())
}
function Write-Utf8([string]$Path, [string]$Text) {
    [IO.File]::WriteAllText($Path, $Text, (New-Object Text.UTF8Encoding($false)))
}
function Read-Summaries([string]$Text, [string]$RequiredName = '', [long]$MinimumCount = 1) {
    $names = if ($RequiredName) { [regex]::Escape($RequiredName) } else {
        '[A-Z][A-Z0-9_]*(?:SUMMARY|SMOKE)|UITAB_THEME_PAINT'
    }
    $lines = @($Text -split "`r?`n" | Where-Object { $_ -match "^\s*(?:$names)\b" })
    if (!$lines.Count) { throw "Missing mandatory summary: $RequiredName" }
    foreach ($line in $lines) {
        $m = [regex]::Match($line, "^\s*(?:$names)\s+(checks|suites)=([0-9]+)\s+(failed|failed_suites)=([0-9]+)\s*$")
        if (!$m.Success) { throw "Malformed summary: $line" }
        $count = [long]$m.Groups[2].Value
        $failed = [long]$m.Groups[4].Value
        $pair = ($m.Groups[1].Value -eq 'checks' -and $m.Groups[3].Value -eq 'failed') -or
                ($m.Groups[1].Value -eq 'suites' -and $m.Groups[3].Value -eq 'failed_suites')
        if (!$pair -or $count -lt [Math]::Max(1, $MinimumCount) -or $failed -ne 0) { throw "Failing/empty summary: $line" }
    }
    return $lines
}
function Test-SummaryReader {
    $cases = @(
        @{Text='UI_RELEASE_SMOKE checks=58 failed=0'; Name='UI_RELEASE_SMOKE'; Pass=$true},
        @{Text='UI_RELEASE_SMOKE checks=0 failed=0'; Name='UI_RELEASE_SMOKE'; Pass=$false},
        @{Text='UI_RELEASE_SMOKE checks=58 failed=1'; Name='UI_RELEASE_SMOKE'; Pass=$false},
        @{Text=''; Name='UI_RELEASE_SMOKE'; Pass=$false},
        @{Text='OTHER_SUMMARY checks=2 failed=0'; Name='UI_RELEASE_SMOKE'; Pass=$false},
        @{Text='UI_RELEASE_SMOKE checks=x failed=0'; Name='UI_RELEASE_SMOKE'; Pass=$false},
        @{Text='UI_RELEASE_SMOKE checks=58 failed=0 trailing'; Name='UI_RELEASE_SMOKE'; Pass=$false},
        @{Text="UI_RELEASE_SMOKE checks=58 failed=0`nUI_RELEASE_SMOKE checks=2 failed=1"; Name='UI_RELEASE_SMOKE'; Pass=$false},
        @{Text='UI_GRAPH_RENDER_TESTS_SUMMARY suites=9 failed_suites=0'; Name=''; Pass=$true},
        @{Text='UI_GRAPH_RENDER_TESTS_SUMMARY suites=0 failed_suites=0'; Name=''; Pass=$false},
        @{Text='UI_RELEASE_SMOKE checks=58 failed_suites=0'; Name='UI_RELEASE_SMOKE'; Pass=$false},
        @{Text="log text`nUI_RELEASE_SMOKE checks=58 failed=0`n"; Name='UI_RELEASE_SMOKE'; Pass=$true}
    )
    foreach ($case in $cases) {
        $ok = $true
        try { $null = Read-Summaries $case.Text $case.Name } catch { $ok = $false }
        if ($ok -ne $case.Pass) { throw "Summary parser self-test failed: $($case.Text)" }
    }
    Record 'UI_RELEASE_EVIDENCE_SELFTEST checks=12 failed=0'
}
function Assert-Inventory($Inventory) {
    $seen = @{}
    foreach ($item in $Inventory.controls) {
        if ($seen.ContainsKey($item.type)) { throw "Duplicate control: $($item.type)" }
        $seen[$item.type] = $true
        if (!(Test-Path -LiteralPath (Join-Path $Repo $item.header) -PathType Leaf)) { throw "Missing header: $($item.header)" }
        if ($item.example) {
            $name = Split-Path -Leaf $item.example
            if (!(Test-Path -LiteralPath (Join-Path $Repo "$($item.example)/$name.upp") -PathType Leaf)) { throw "Missing demo package: $($item.example)" }
        }
    }
    if (!$seen.Count) { throw 'Empty control inventory' }
    $active = Join-Path $Repo 'docs/ACTIVE_WORK.md'
    if (@(Get-Content -LiteralPath $active).Count -gt 100) { throw 'ACTIVE_WORK exceeds 100 lines' }
    $expected = @(@($Inventory.guides | ForEach-Object { Split-Path -Leaf $_ }) + @('ACTIVE_WORK.md') | Sort-Object)
    $actual = @(Get-ChildItem -LiteralPath (Join-Path $Repo 'docs') -File -Filter '*.md' | ForEach-Object { $_.Name } | Sort-Object)
    if (@(Compare-Object $expected $actual).Count) { throw 'Canonical guide inventory differs from docs/*.md' }
    $readerFiles = @('README.md','GETTING_STARTED.md','CHANGELOG.md') + @($Inventory.guides)
    foreach ($relative in $readerFiles) {
        $file = Join-Path $Repo $relative
        $text = Get-Content -Raw -LiteralPath $file
        foreach ($match in [regex]::Matches($text, '!?\[[^\]]*\]\(([^)]+)\)')) {
            $link = ($match.Groups[1].Value -split '#')[0]
            if (!$link -or $link -match '^[a-zA-Z][a-zA-Z0-9+.-]*:') { continue }
            $target = [IO.Path]::GetFullPath((Join-Path (Split-Path $file) $link))
            if (!(Test-Path -LiteralPath $target)) { throw "Broken reader link in ${relative}: $link" }
        }
    }
    $pending = @($Inventory.controls | Where-Object { $_.source_review -eq 'pending' }).Count
    Record "INVENTORY controls=$($seen.Count) guides=$($Inventory.guides.Count) source_reviews_pending=$pending"
}
function Build-Target([string]$Package, [string]$Config, [string]$VarFile = $AssemblyFile) {
    $tag = ($Package -replace '[^a-zA-Z0-9_.-]', '_') + '-' + $Config
    $exe = Join-Path $script:evidence ($tag + '.exe')
    $args = @($VarFile, $Package, $Method)
    $flags = ''
    if ($Blitz) { $flags += 'b' }
    if ($Config -eq 'Release') { $flags += 'r' }
    if ($flags) { $args += ('-' + $flags) }
    $args += $exe
    Record "BUILD $Package $Config"
    # Native stderr is compiler output; process exit and resulting file decide
    # success. Preserve the complete first failing build log without continuing.
    $oldPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { & $Umk @args 2>&1 | Tee-Object -FilePath (Join-Path $script:evidence ($tag + '.build.log')) | Out-Host; $code = $LASTEXITCODE }
    finally { $ErrorActionPreference = $oldPreference }
    if ($code -ne 0 -or !(Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Build failed ($code): $Package / $Config" }
    Record "BUILD PASS $Package $Config SHA256=$((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash)"
    return $exe
}
function Run-Test([string]$Exe, [string]$Summary, [long]$MinimumCount = 1) {
    $stdout = $Exe + '.stdout.log'; $stderr = $Exe + '.stderr.log'
    $p = Start-Process -FilePath $Exe -WorkingDirectory $Repo -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru
    if (!$p.WaitForExit($TimeoutSeconds * 1000)) {
        # Only our own exact process is terminated, never an already open demo.
        $p.Kill()
        $null = $p.WaitForExit(5000)
        throw "Test timed out: $Exe"
    }
    $p.WaitForExit(); $p.Refresh()
    $text = (Get-Content -Raw -LiteralPath $stdout) + "`n" + (Get-Content -Raw -LiteralPath $stderr)
    if ($p.ExitCode -ne 0) { throw "Test failed ($($p.ExitCode)): $Exe" }
    foreach ($line in @(Read-Summaries $text $Summary $MinimumCount)) { Record $line }
}
function Build-Headers($Inventory, [string]$Config) {
    $nest = Join-Path $script:evidence 'header-probe'
    $package = Join-Path $nest 'UiPublicHeadersProbe'
    $null = New-Item -ItemType Directory -Path $package -Force
    $headers = @(@($Inventory.controls | ForEach-Object { $_.header }) + @($Inventory.support_headers) + @('Ui/Ui.h','Ui/UiVersion.h') | Sort-Object -Unique)
    $files = New-Object System.Collections.Generic.List[string]
    for ($i = 0; $i -lt $headers.Count; $i++) {
        $name = 'Header{0:D3}.cpp' -f $i
        Write-Utf8 (Join-Path $package $name) ("#include <$($headers[$i])>`nvoid UiHeaderProbe$i() {}`n")
        $files.Add($name)
    }
    Write-Utf8 (Join-Path $package 'main.cpp') "#include <CtrlLib/CtrlLib.h>`nGUI_APP_MAIN { Upp::SetExitCode(0); }`n"
    $files.Add('main.cpp')
    Write-Utf8 (Join-Path $package 'UiPublicHeadersProbe.upp') ("noblitz;`nuses Core, Draw, Painter, CtrlCore, CtrlLib, Ui;`nfile`n    " + ($files -join ",`n    ") + ";`nmainconfig `"GUI`" = `"GUI`";`n")
    $probeVar = Join-Path $script:evidence 'Headers.var'
    $upp = ((@($nest) + $script:nests) -join ';') -replace '\\','/'
    Write-Utf8 $probeVar ("UPP = `"$upp`";`nOUTPUT = `"$($script:assemblyOutput -replace '\\','/')`";`n")
    $null = Build-Target 'UiPublicHeadersProbe' $Config $probeVar
    Record "HEADERS PASS count=$($headers.Count) configuration=$Config"
}

try {
    Test-SummaryReader
    if ($SelfTest) { exit 0 }
    if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT) { throw 'Native runner requires Windows; parser -SelfTest is portable.' }
    if (!$Repo) { $Repo = Split-Path -Parent $PSScriptRoot }
    $Repo = (Resolve-Path -LiteralPath $Repo).Path
    if (Git-Read @('status','--porcelain')) { throw 'Dirty worktree: preserve local work before validation.' }
    if ((Git-Read @('branch','--show-current')) -ne 'main') { throw 'Use the current main branch.' }
    $null = Git-Read @('fetch','origin')
    $head = Git-Read @('rev-parse','HEAD')
    if ($head -ne (Git-Read @('rev-parse','origin/main'))) { throw 'Update clean main with git pull --ff-only; do not validate stale or unpublished local code.' }
    if ($RequiredAncestor) { $null = Git-Read @('merge-base','--is-ancestor',$RequiredAncestor,$head) }
    if (!$AssemblyFile) { $AssemblyFile = Join-Path $Repo 'GitHubOut.var' }
    $AssemblyFile = (Resolve-Path -LiteralPath $AssemblyFile).Path
    $assembly = Get-Content -Raw -LiteralPath $AssemblyFile
    $upp = [regex]::Match($assembly, '(?m)^\s*UPP\s*=\s*"([^"]+)"\s*;')
    $output = [regex]::Match($assembly, '(?m)^\s*OUTPUT\s*=\s*"([^"]+)"\s*;')
    if (!$upp.Success -or !$output.Success) { throw 'Assembly must contain quoted UPP and OUTPUT assignments.' }
    $script:nests = @($upp.Groups[1].Value -split ';' | Where-Object { $_.Trim() } | ForEach-Object { $_.Trim() })
    foreach ($nest in $script:nests) {
        if (![IO.Path]::IsPathRooted($nest) -or !(Test-Path -LiteralPath $nest -PathType Container)) { throw "Missing/nonabsolute assembly nest: $nest" }
    }
    if (!(@($script:nests | Where-Object { (Resolve-Path -LiteralPath $_).Path.TrimEnd('\','/') -ieq $Repo.TrimEnd('\','/') }).Count)) { throw 'Assembly does not include this checkout; refuse to compile another copy.' }
    $script:assemblyOutput = $output.Groups[1].Value
    if (![IO.Path]::IsPathRooted($script:assemblyOutput)) { throw 'Use an absolute assembly OUTPUT folder.' }
    if (!$Umk) {
        $uppsrc = @($script:nests | Where-Object { (Split-Path -Leaf $_.TrimEnd('\','/')) -ieq 'uppsrc' })
        if ($uppsrc.Count -ne 1) { throw 'Cannot uniquely derive UMK from uppsrc; provide -Umk explicitly.' }
        $Umk = Join-Path (Split-Path -Parent $uppsrc[0]) 'umk.exe'
    }
    $Umk = (Resolve-Path -LiteralPath $Umk).Path
    if (!$OutputRoot) { $OutputRoot = Join-Path $env:TEMP 'Ui-release-validation' }
    $script:evidence = Join-Path $OutputRoot ((Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $head.Substring(0,12) + '-' + [Guid]::NewGuid().ToString('N').Substring(0,6))
    $null = New-Item -ItemType Directory -Path $script:evidence -Force
    Record "TESTED_HEAD $head"
    Record "UMK $Umk SHA256=$((Get-FileHash -LiteralPath $Umk -Algorithm SHA256).Hash) FILE_VERSION=$((Get-Item -LiteralPath $Umk).VersionInfo.FileVersion) METHOD=$Method"
    Record "ASSEMBLY $AssemblyFile SHA256=$((Get-FileHash -LiteralPath $AssemblyFile -Algorithm SHA256).Hash)"
    Record "EVIDENCE $script:evidence"
    Write-Utf8 (Join-Path $script:evidence 'assembly.var.txt') $assembly
    $inventory = Get-Content -Raw -LiteralPath (Join-Path $Repo 'tests/ui_release_inventory.json') | ConvertFrom-Json
    Assert-Inventory $inventory
    $version = [regex]::Match((Get-Content -Raw -LiteralPath (Join-Path $Repo $inventory.version_header)), '#define\s+UPP_UI_VERSION\s+"([^"]+)"')
    if (!$version.Success) { throw 'Missing central Ui version' }
    Record "UI_VERSION $($version.Groups[1].Value)"
    if ($Profile -eq 'Full' -and !$PSBoundParameters.ContainsKey('Configuration')) { $Configuration = 'Both' }
    $configs = if ($Configuration -eq 'Both') { @('Debug','Release') } else { @($Configuration) }
    $targets = @($inventory.surgical_targets)
    if ($Profile -eq 'Demos' -or $Profile -eq 'Full') {
        # Only tracked packages are retained targets; local evidence/build folders
        # must never accidentally join the validation matrix.
        $packages = (Git-Read @('ls-files','*.upp')) -split "`n"
        $targets = @()
        foreach ($path in $packages) {
            $isDemo = $path -like 'examples/*' -or $path -match '^Utilities/[^/]*Demo/[^/]+\.upp$'
            $isTest = $path -match '^(Utilities|tests)/[^/]*(Test|Probe)[^/]*/[^/]+\.upp$'
            if ($isDemo -or ($Profile -eq 'Full' -and $isTest)) {
                $targets += [pscustomobject]@{ package=($path -replace '/[^/]+$',''); run=($Profile -eq 'Full' -and $isTest); summary=''; minimum_checks=1 }
            }
        }
        if ($Profile -eq 'Full') { $targets += @($inventory.surgical_targets | Where-Object { $_.run }) }
        $byPackage = @{}
        foreach ($target in $targets) { $byPackage[$target.package] = $target }
        $targets = @($byPackage.Values | Sort-Object package)
    }
    foreach ($config in $configs) {
        if ($Profile -eq 'Headers' -or $Profile -eq 'Full') { Build-Headers $inventory $config }
        if ($Profile -eq 'Headers') { continue }
        foreach ($target in $targets) {
            $exe = Build-Target $target.package $config
            if ($target.run) { Run-Test $exe $target.summary $target.minimum_checks }
        }
    }
    $null = Git-Read @('diff','--check')
    if ((Git-Read @('rev-parse','HEAD')) -ne $head -or (Git-Read @('status','--porcelain'))) { throw 'Checkout changed during validation; retain evidence but do not call it a clean acceptance.' }
    Record "UI_RELEASE_VALIDATION PASS profile=$Profile configuration=$Configuration (requested profile only; not visual/generated-code/all-source acceptance)"
    Write-Utf8 (Join-Path $script:evidence 'summary.txt') ($script:records -join "`r`n")
    exit 0
}
catch {
    Record ('FAIL ' + $_.Exception.Message)
    if ($script:evidence) { Write-Utf8 (Join-Path $script:evidence 'summary.txt') ($script:records -join "`r`n") }
    exit 1
}
