param(
    [Parameter(Mandatory = $true)]
    [string]$Executable
)

$ErrorActionPreference = 'Stop'

function Start-Mcp([string]$payload, [bool]$framed) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $Executable
    $psi.UseShellExecute = $false
    $psi.RedirectStandardInput = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $psi.StandardOutputEncoding = [Text.Encoding]::UTF8
    $psi.StandardErrorEncoding = [Text.Encoding]::UTF8

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $psi
    if(-not $process.Start()) { throw 'Unable to start UiDesigner MCP process.' }

    if($framed) {
        $bytes = [Text.Encoding]::UTF8.GetByteCount($payload)
        $process.StandardInput.Write("Content-Length: $bytes`r`n`r`n$payload")
    }
    else {
        $process.StandardInput.WriteLine($payload)
    }
    $process.StandardInput.Close()
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    if($process.ExitCode -ne 0) {
        throw "MCP process failed ($($process.ExitCode)): $stderr"
    }
    if([string]::IsNullOrWhiteSpace($stdout)) {
        throw 'MCP process returned no response.'
    }
    return $stdout
}

function Parse-Mcp([string]$text, [bool]$framed) {
    if($framed) {
        $separator = $text.IndexOf("`r`n`r`n")
        if($separator -lt 0) { throw 'Framed MCP response has no header terminator.' }
        $header = $text.Substring(0, $separator)
        if($header -notmatch '^Content-Length:\s*(\d+)$') {
            throw "Invalid MCP Content-Length header: $header"
        }
        $json = $text.Substring($separator + 4)
        if([Text.Encoding]::UTF8.GetByteCount($json) -ne [int]$Matches[1]) {
            throw 'MCP framed response length does not match its header.'
        }
    }
    else {
        $json = $text.Trim()
    }
    return $json | ConvertFrom-Json
}

$initialize = '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{},"clientInfo":{"name":"uidesigner-smoke","version":"1"}}}'
$tools = '{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}'
$resources = '{"jsonrpc":"2.0","id":3,"method":"resources/list","params":{}}'

foreach($framed in @($false, $true)) {
    $mode = if($framed) { 'Content-Length' } else { 'newline' }
    $initResult = Parse-Mcp (Start-Mcp $initialize $framed) $framed
    if($initResult.result.serverInfo.name -ne 'upp-ui-designer') {
        throw "$mode initialize returned unexpected server information."
    }
    $toolResult = Parse-Mcp (Start-Mcp $tools $framed) $framed
    $names = @($toolResult.result.tools | ForEach-Object { $_.name })
    foreach($required in @('uidesigner_apply_drop', 'uidesigner_set_behavior', 'uidesigner_export')) {
        if($names -notcontains $required) { throw "$mode tools/list is missing $required" }
    }
    $resourceResult = Parse-Mcp (Start-Mcp $resources $framed) $framed
    $uris = @($resourceResult.result.resources | ForEach-Object { $_.uri })
    foreach($required in @('uidesigner://document', 'uidesigner://theme', 'uidesigner://catalog', 'uidesigner://behaviors')) {
        if($uris -notcontains $required) { throw "$mode resources/list is missing $required" }
    }
    Write-Host "UiDesigner MCP $mode framing: PASS"
}
