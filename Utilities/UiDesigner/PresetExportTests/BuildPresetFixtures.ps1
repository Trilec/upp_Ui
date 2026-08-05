param(
    [string]$UmkPath = 'E:\upp-18468\umk.exe',
    [string]$Assembly = 'GitHubOut',
    [string]$Config = 'CLANGx64',
    [string]$OutputRoot = 'E:\apps\github\upp_Ui\out'
)

$ErrorActionPreference = 'Stop'
$fixtureExe = Join-Path $OutputRoot 'UiDesignerPresetExportTests.exe'
$generatedRoot = Join-Path $PSScriptRoot '.generated-presets'

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
New-Item -ItemType Directory -Path $generatedRoot -Force | Out-Null
New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null

try {
    Invoke-Checked 'Build PresetExportTests' {
        & $UmkPath $Assembly 'Utilities/UiDesigner/PresetExportTests' $Config '-br' $fixtureExe
    }
    Invoke-Checked 'Export blank, three-pane and dialog fixtures' {
        & $fixtureExe '--export-all' $generatedRoot
    }

    $packages = @(
        'BlankFormGenerated',
        'ThreePaneGenerated',
        'DialogGenerated'
    )
    foreach($package in $packages) {
        $relative = "Utilities/UiDesigner/PresetExportTests/.generated-presets/$package"
        $output = Join-Path $OutputRoot ($package + '.exe')
        Invoke-Checked "Build generated preset $package" {
            & $UmkPath $Assembly $relative $Config '-br' '+GUI' $output
        }
        if(-not (Test-Path -LiteralPath $output -PathType Leaf)) {
            throw "Generated preset executable was not produced: $output"
        }
    }

    Write-Host 'Blank Form, Three Pane and Dialog generated-package builds passed.'
    Remove-Item -LiteralPath $generatedRoot -Recurse -Force
}
catch {
    Write-Host "Generated preset fixtures retained for diagnosis: $generatedRoot"
    throw
}
