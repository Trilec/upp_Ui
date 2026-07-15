$ErrorActionPreference = 'Stop'

$uiDesignerRoot = $PSScriptRoot
$repoRoot = (Resolve-Path (Join-Path $uiDesignerRoot '..\..')).Path
$deliveryDir = Join-Path $uiDesignerRoot '.integrated'
$parts = Get-ChildItem -LiteralPath $deliveryDir -Filter 'part*.b64' | Sort-Object Name
if($parts.Count -eq 0) {
    throw 'No integrated UiDesigner source parts were found.'
}

$base64 = [string]::Concat(($parts | ForEach-Object {
    (Get-Content -LiteralPath $_.FullName -Raw).Trim()
}))
$zipBytes = [Convert]::FromBase64String($base64)
$tempZip = Join-Path $env:TEMP 'UiDesignerIntegratedSource.zip'
[IO.File]::WriteAllBytes($tempZip, $zipBytes)

$expected = 'a01b813c0a238da99066c105102971304e0aae5d8e42f79ffbae4caeea9d27d6'
$actual = (Get-FileHash -LiteralPath $tempZip -Algorithm SHA256).Hash.ToLowerInvariant()
if($actual -ne $expected) {
    Remove-Item -LiteralPath $tempZip -Force -ErrorAction SilentlyContinue
    throw "UiDesigner source checksum mismatch. Expected $expected, got $actual."
}

Expand-Archive -LiteralPath $tempZip -DestinationPath $repoRoot -Force
Remove-Item -LiteralPath $tempZip -Force

Write-Host 'Integrated UiDesigner source extracted successfully.'
Write-Host 'Files:        69'
Write-Host 'Application:  Utilities/UiDesigner/UiDesigner'
Write-Host 'Tests:        Utilities/UiDesigner/Tests'
Write-Host 'CLI:          Utilities/UiDesigner/CLI'
Write-Host 'MCP:          Utilities/UiDesigner/MCP'
Write-Host 'Architecture: Utilities/UiDesigner/DESIGN.md'
Write-Host 'No commit or push was performed.'
