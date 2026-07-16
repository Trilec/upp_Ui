param(
    [string]$UmkPath = 'E:\upp-18468\umk.exe',
    [string]$Assembly = 'GitHubOut',
    [string]$Config = 'CLANGx64',
    [string]$OutputRoot = 'E:\apps\github\upp_Ui\out'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$fixtureExe = Join-Path $OutputRoot 'UiDesignerFoundationTests.exe'
$generatedRoot = Join-Path $PSScriptRoot '.generated-smoke'
$packageDir = Join-Path $generatedRoot 'GeneratedPackage'
$generatedExe = Join-Path $OutputRoot 'GeneratedPackage.exe'
$packageRelative = 'Utilities/UiDesigner/FoundationTests/.generated-smoke/GeneratedPackage'

function Invoke-Checked([string]$label, [scriptblock]$command) {
    Write-Host "== $label =="
    & $command
    if($LASTEXITCODE -ne 0) {
        throw "$label failed with exit code $LASTEXITCODE"
    }
}

if(-not (Test-Path -LiteralPath $UmkPath -PathType Leaf)) {
    throw "umk was not found at $UmkPath"
}

Remove-Item -LiteralPath $generatedRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $packageDir -Force | Out-Null

try {
    Invoke-Checked 'Build FoundationTests' {
        & $UmkPath $Assembly 'Utilities/UiDesigner/FoundationTests' $Config '-br' $fixtureExe
    }
    Invoke-Checked 'Run FoundationTests' {
        & $fixtureExe
    }
    Invoke-Checked 'Export generated fixture' {
        & $fixtureExe '--export-fixture' $packageDir
    }

    $required = @(
        'GeneratedPackage.upp',
        'GeneratedUiWindow.generated.h',
        'GeneratedUiWindow.generated.cpp',
        'GeneratedUiWindow.h',
        'GeneratedUiWindow.cpp',
        'main.cpp',
        'design.json',
        'project.uidesign.json',
        'theme.json'
    )
    foreach($file in $required) {
        $path = Join-Path $packageDir $file
        if(-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Generated fixture is missing $file"
        }
    }

    Invoke-Checked 'Build generated package' {
        & $UmkPath $Assembly $packageRelative $Config '-br' '+GUI' $generatedExe
    }

    if(-not (Test-Path -LiteralPath $generatedExe -PathType Leaf)) {
        throw "Generated executable was not produced: $generatedExe"
    }

    $process = Start-Process -FilePath $generatedExe -PassThru
    Start-Sleep -Milliseconds 1500
    if($process.HasExited) {
        throw "Generated executable exited early with code $($process.ExitCode)"
    }
    Stop-Process -Id $process.Id -Force
    $process.WaitForExit()

    Write-Host 'Generated package build and process smoke passed.'
    Remove-Item -LiteralPath $generatedRoot -Recurse -Force
}
catch {
    Write-Host "Generated fixture retained for diagnosis: $generatedRoot"
    throw
}
