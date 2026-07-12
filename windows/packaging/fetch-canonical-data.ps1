# Fetch canonical corpus from upstream repository
# Usage: .\fetch-canonical-data.ps1 [-Verbose]

[CmdletBinding()]
param(
    [switch] $Verbose,
    [string] $UpstreamRepo = "flynn33/ASH-Pattern-System",
    [string] $UpstreamCommit = "cc253f3d137a27f0eeb471bed62bbdb939e3b6d1"
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$outputDir = Join-Path $scriptDir "..\canonical-data"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Fetching Canonical Corpus from Upstream" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Upstream Repo:  $UpstreamRepo" -ForegroundColor Green
Write-Host "Commit:         $UpstreamCommit" -ForegroundColor Green
Write-Host "Output Dir:     $outputDir" -ForegroundColor Green
Write-Host ""

# Create output directory
if (Test-Path $outputDir) {
    Write-Host "Removing existing canonical-data directory..." -ForegroundColor Yellow
    Remove-Item $outputDir -Recurse -Force
}
New-Item -ItemType Directory -Path $outputDir | Out-Null

Write-Host "Fetching canonical-data/ directory..." -ForegroundColor Cyan
# Fetch canonical-data directory contents
$canonicalDataUrl = "https://api.github.com/repos/$UpstreamRepo/git/trees/$UpstreamCommit?recursive=1"
$response = Invoke-RestMethod -Uri $canonicalDataUrl -Method Get -Headers @{"Accept"="application/vnd.github.v3+json"}

# Filter for canonical-data files
$canonicalFiles = @()
if ($response.items) {
    foreach ($item in $response.items) {
        if ($item.path -like "canonical-data/*") {
            $canonicalFiles += $item
        }
    }
}

foreach ($file in $canonicalFiles) {
    if ($file.type -eq "blob") {
        Write-Host "  Fetching: $($file.path)" -ForegroundColor Gray
        $contentUrl = "https://raw.githubusercontent.com/$UpstreamRepo/$UpstreamCommit/$($file.path)"
        (Invoke-RestMethod -Uri $contentUrl).TrimEnd() | Out-File -FilePath (Join-Path $outputDir $file.path) -Encoding UTF8
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Canonical Data Fetch Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Contents staged in: $outputDir" -ForegroundColor Green
Write-Host ""
