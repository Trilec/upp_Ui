param(
    [string]$UmkPath = 'E:\upp-18468\umk.exe',
    [string]$Assembly = 'GitHubOut',
    [string]$Config = 'CLANGx64',
    [string]$OutputRoot = 'E:\apps\github\upp_Ui\out'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

function Invoke-Checked([string]$label, [scriptblock]$command) {
    Write-Host "`n== $label =="
    & $command
    if($LASTEXITCODE -ne 0) {
        throw "$label failed with exit code $LASTEXITCODE"
    }
}

function Build-Package([string]$package, [string]$output, [bool]$gui = $false) {
    Invoke-Checked "Build $package" {
        if($gui) {
            & $UmkPath $Assembly $package $Config '-br' '+GUI' $output
        }
        else {
            & $UmkPath $Assembly $package $Config '-br' $output
        }
    }
}

if(-not (Test-Path -LiteralPath $UmkPath -PathType Leaf)) {
    throw "umk was not found at $UmkPath"
}
New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null

Invoke-Checked 'Architecture guard' {
    powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'ValidateArchitecture.ps1')
}

$probe = Join-Path $OutputRoot 'PropertyEditorCoreProbe.exe'
$propertyTests = Join-Path $OutputRoot 'PropertyEditorTests.exe'
$designerTests = Join-Path $OutputRoot 'UiDesignerTests.exe'
$regressionTests = Join-Path $OutputRoot 'UiDesignerRegressionTests.exe'
$foundationTests = Join-Path $OutputRoot 'UiDesignerFoundationTests.exe'
$cli = Join-Path $OutputRoot 'uidesigner_cli.exe'
$mcp = Join-Path $OutputRoot 'uidesigner_mcp.exe'
$app = Join-Path $OutputRoot 'UiDesigner.exe'

Build-Package 'Utilities/PropertyEditorCoreProbe' $probe
Build-Package 'Utilities/PropertyEditorTests' $propertyTests $true
Build-Package 'Utilities/UiDesigner/Tests' $designerTests $true
Build-Package 'Utilities/UiDesigner/RegressionTests' $regressionTests $true
Build-Package 'Utilities/UiDesigner/FoundationTests' $foundationTests
Build-Package 'Utilities/UiDesigner/CLI' $cli
Build-Package 'Utilities/UiDesigner/MCP' $mcp
Build-Package 'Utilities/UiDesigner/UiDesigner' $app $true

Invoke-Checked 'PropertyEditorCoreProbe' { & $probe }
Invoke-Checked 'PropertyEditorTests' { & $propertyTests }
Invoke-Checked 'UiDesignerTests' { & $designerTests }
Invoke-Checked 'UiDesignerRegressionTests' { & $regressionTests }
Invoke-Checked 'UiDesignerFoundationTests' { & $foundationTests }

Invoke-Checked 'CLI list-controls' { & $cli 'list-controls' 'spacer' }
Invoke-Checked 'CLI schema Spacer' { & $cli 'schema' 'Spacer' }

Invoke-Checked 'MCP newline and Content-Length smoke' {
    powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'MCP\Smoke.ps1') -Executable $mcp
}

Invoke-Checked 'Generated package build smoke' {
    powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'FoundationTests\BuildGeneratedFixture.ps1') `
        -UmkPath $UmkPath -Assembly $Assembly -Config $Config -OutputRoot $OutputRoot
}

Write-Host "`nUiDesigner supervisor validation sequence completed."
Write-Host "GUI executable: $app"
Write-Host 'Interactive design, drag/drop and dialog validation still requires a visible desktop session.'
