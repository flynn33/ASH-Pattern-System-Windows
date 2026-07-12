# Generate SHA-256 checksums for Release (or Debug) build artifacts.
#
# Layout (Directory.Build.props):
#   windows\build\bin\{Platform}-{Configuration}\
#
# Usage:
#   cd D:\GitHub\APS\windows\build
#   .\generate-checksums.ps1
#   .\generate-checksums.ps1 -Configuration Release
#   .\generate-checksums.ps1 -Platform x64 -Configuration Release

[CmdletBinding()]
param(
    [ValidateSet('x64', 'ARM64')]
    [string[]] $Platform = @('x64', 'ARM64'),

    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Release',

    [string] $OutputDir = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$binRoot = Join-Path $scriptDir 'bin'
if (-not $OutputDir) {
    $OutputDir = Join-Path (Split-Path -Parent (Split-Path -Parent $scriptDir)) 'completion-evidence'
}

if (-not (Test-Path -LiteralPath $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

foreach ($plat in $Platform) {
    $dir = Join-Path $binRoot ("{0}-{1}" -f $plat, $Configuration)
    $outFile = Join-Path $OutputDir ("{0}-{1}-checksums.txt" -f $plat.ToLowerInvariant(), $Configuration.ToLowerInvariant())

    if (-not (Test-Path -LiteralPath $dir)) {
        Write-Warning "Skip $plat $Configuration — missing $dir"
        continue
    }

    Write-Host "Checksums for $dir" -ForegroundColor Cyan
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("# SHA256 — $plat $Configuration") | Out-Null
    $lines.Add("# Source: $dir") | Out-Null
    $lines.Add("") | Out-Null

    Get-ChildItem -LiteralPath $dir -File | Sort-Object Name | ForEach-Object {
        $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        $lines.Add("$hash  $($_.Name)") | Out-Null
        Write-Host "  $($_.Name)"
    }

    $lines | Set-Content -LiteralPath $outFile -Encoding UTF8
    Write-Host "  -> $outFile" -ForegroundColor Green
}

Write-Host 'Done.' -ForegroundColor Green
