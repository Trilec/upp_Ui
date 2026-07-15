$ErrorActionPreference = 'Stop'

$uiDesignerRoot = $PSScriptRoot
$repoRoot = (Resolve-Path (Join-Path $uiDesignerRoot '..\..')).Path
$deliveryDir = Join-Path $uiDesignerRoot '.integrated'
$manifestPath = Join-Path $deliveryDir 'manifest.json'
$tempZip = Join-Path $env:TEMP 'UiDesignerIntegratedSource.zip'

function Get-TextSha256([string] $text) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes($text)
        return ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

if(-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Integrated UiDesigner manifest is missing: $manifestPath"
}

$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$expectedParts = @($manifest.parts)
if($expectedParts.Count -eq 0) {
    throw 'Integrated UiDesigner manifest contains no archive parts.'
}

$publishedParts = @(Get-ChildItem -LiteralPath $deliveryDir -Filter 'part*.b64' -File)
if($publishedParts.Count -ne $expectedParts.Count) {
    throw "Integrated UiDesigner part count mismatch. Expected $($expectedParts.Count), found $($publishedParts.Count)."
}

foreach($published in $publishedParts) {
    if(-not ($expectedParts.name -contains $published.Name)) {
        throw "Unexpected integrated UiDesigner archive part: $($published.Name)"
    }
}

$segments = New-Object 'System.Collections.Generic.List[string]'
foreach($part in $expectedParts) {
    $path = Join-Path $deliveryDir ([string]$part.name)
    if(-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing integrated UiDesigner archive part: $($part.name)"
    }

    $content = (Get-Content -LiteralPath $path -Raw).Trim()
    if($content.Length -ne [int]$part.length) {
        throw "Length mismatch for $($part.name). Expected $($part.length), got $($content.Length)."
    }

    $partHash = Get-TextSha256 $content
    $expectedPartHash = ([string]$part.sha256).ToLowerInvariant()
    if($partHash -ne $expectedPartHash) {
        throw "Checksum mismatch for $($part.name). Expected $expectedPartHash, got $partHash."
    }

    $segments.Add($content)
}

$base64 = [string]::Concat($segments)
if($base64.Length -ne [int]$manifest.base64_length) {
    throw "Integrated UiDesigner Base64 length mismatch. Expected $($manifest.base64_length), got $($base64.Length)."
}

try {
    try {
        $zipBytes = [Convert]::FromBase64String($base64)
    }
    catch {
        throw "Integrated UiDesigner Base64 decoding failed: $($_.Exception.Message)"
    }

    if($zipBytes.Length -ne [int]$manifest.archive_size) {
        throw "Integrated UiDesigner archive size mismatch. Expected $($manifest.archive_size), got $($zipBytes.Length)."
    }

    [IO.File]::WriteAllBytes($tempZip, $zipBytes)

    $actualArchiveHash = (Get-FileHash -LiteralPath $tempZip -Algorithm SHA256).Hash.ToLowerInvariant()
    $expectedArchiveHash = ([string]$manifest.archive_sha256).ToLowerInvariant()
    if($actualArchiveHash -ne $expectedArchiveHash) {
        throw "UiDesigner source checksum mismatch. Expected $expectedArchiveHash, got $actualArchiveHash."
    }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [IO.Compression.ZipFile]::OpenRead($tempZip)
    try {
        if($zip.Entries.Count -ne [int]$manifest.file_count) {
            throw "UiDesigner source entry count mismatch. Expected $($manifest.file_count), got $($zip.Entries.Count)."
        }
    }
    finally {
        $zip.Dispose()
    }

    Expand-Archive -LiteralPath $tempZip -DestinationPath $repoRoot -Force
}
finally {
    Remove-Item -LiteralPath $tempZip -Force -ErrorAction SilentlyContinue
}

Write-Host 'Integrated UiDesigner source extracted successfully.'
Write-Host "Archive SHA: $($manifest.archive_sha256)"
Write-Host "Parts:       $($expectedParts.Count)"
Write-Host "Files:       $($manifest.file_count)"
Write-Host 'Application: Utilities/UiDesigner/UiDesigner'
Write-Host 'Tests:       Utilities/UiDesigner/Tests'
Write-Host 'CLI:         Utilities/UiDesigner/CLI'
Write-Host 'MCP:         Utilities/UiDesigner/MCP'
Write-Host 'Architecture: Utilities/UiDesigner/DESIGN.md'
Write-Host 'No commit or push was performed.'
