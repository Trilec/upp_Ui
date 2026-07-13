$ErrorActionPreference = 'Stop'

$bundleDir = $PSScriptRoot
$repoRoot = (Resolve-Path (Join-Path $bundleDir '..\..')).Path
$parts = Get-ChildItem -LiteralPath $bundleDir -Filter 'part*.b64' | Sort-Object Name
if ($parts.Count -eq 0) {
    throw 'No PropertyEditor bundle parts were found.'
}

$base64 = [string]::Concat(($parts | ForEach-Object { (Get-Content -LiteralPath $_.FullName -Raw).Trim() }))
$zipBytes = [Convert]::FromBase64String($base64)
$tempZip = Join-Path $env:TEMP 'PropertyEditorUtility.zip'
[IO.File]::WriteAllBytes($tempZip, $zipBytes)

$expected = '122a533d783e79daeb621c9f8f0072269947c8f04d18e8c42f947eed86268281'
$actual = (Get-FileHash -LiteralPath $tempZip -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -ne $expected) {
    Remove-Item -LiteralPath $tempZip -Force -ErrorAction SilentlyContinue
    throw "PropertyEditor bundle checksum mismatch. Expected $expected, got $actual."
}

Expand-Archive -LiteralPath $tempZip -DestinationPath $repoRoot -Force
Remove-Item -LiteralPath $tempZip -Force

Write-Host 'PropertyEditor source extracted successfully:'
Write-Host '  Utilities/PropertyEditor'
Write-Host '  Utilities/PropertyEditorDemo'
Write-Host '  Utilities/PropertyEditorTests'
