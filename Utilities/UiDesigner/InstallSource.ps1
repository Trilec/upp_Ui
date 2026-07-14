$ErrorActionPreference = 'Stop'

$uiDesignerRoot = $PSScriptRoot
$repoRoot = (Resolve-Path (Join-Path $uiDesignerRoot '..\..')).Path
$bundleDir = Join-Path $uiDesignerRoot '.bundle'
$parts = Get-ChildItem -LiteralPath $bundleDir -Filter 'part*.b64' | Sort-Object Name
if ($parts.Count -eq 0) {
    throw 'No UiDesigner source bundle parts were found.'
}

$base64 = [string]::Concat(($parts | ForEach-Object {
    (Get-Content -LiteralPath $_.FullName -Raw).Trim()
}))
$zipBytes = [Convert]::FromBase64String($base64)
$tempZip = Join-Path $env:TEMP 'UiDesignerSource.zip'
[IO.File]::WriteAllBytes($tempZip, $zipBytes)

$expected = 'e871f213f92bf6a23f4552a7d6618109b3157510323b235145744d828cc6ea41'
$actual = (Get-FileHash -LiteralPath $tempZip -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -ne $expected) {
    Remove-Item -LiteralPath $tempZip -Force -ErrorAction SilentlyContinue
    throw "UiDesigner source checksum mismatch. Expected $expected, got $actual."
}

Expand-Archive -LiteralPath $tempZip -DestinationPath $repoRoot -Force
Remove-Item -LiteralPath $tempZip -Force

Write-Host 'UiDesigner source extracted successfully.'
Write-Host 'Application: Utilities/UiDesigner/UiDesigner'
Write-Host 'Tests:       Utilities/UiDesigner/Tests'
Write-Host 'Blueprint:   UPP_GUIDES/DesignerNext_GreenfieldSystemArchitectureBlueprint.md'
