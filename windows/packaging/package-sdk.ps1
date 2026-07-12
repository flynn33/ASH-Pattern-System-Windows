# ASH Pattern System - Windows SDK Packaging Script
#
# Consumes Release x64/ARM64 build outputs and creates SDK distribution packages.
# Produces: staged SDK folder, ZIP archive, SHA256 checksums, simple SBOM/manifest.
#
# Artifact layout (from Directory.Build.props):
#   windows/build/bin/{Platform}-{Configuration}/
#     ash-core.lib
#     ash-core.pdb  (optional)
#     ash-test-*.exe
#
# Usage (from any cwd):
#   cd D:\GitHub\APS\windows\packaging
#   .\package-sdk.ps1 -Platform x64
#   .\package-sdk.ps1 -Platform ARM64
#   .\package-sdk.ps1 -Platform x64 -OutputPath "D:\GitHub\APS\packages"

[CmdletBinding()]
param(
    [ValidateSet('x64', 'ARM64')]
    [string] $Platform = 'x64',

    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Release',

    [string] $OutputPath = '',

    [switch] $SkipChecksums,

    [switch] $SkipSBOM
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path   # .../windows/packaging
$windowsDir = Split-Path -Parent $scriptDir                   # .../windows
$repoRoot = Split-Path -Parent $windowsDir                   # .../APS
$buildDir = Join-Path $windowsDir 'build'
$binDir = Join-Path $buildDir ("bin\{0}-{1}" -f $Platform, $Configuration)

if (-not $OutputPath) {
    $OutputPath = Join-Path $repoRoot 'packages'
}

# --- Resolve required inputs -------------------------------------------------

if (-not (Test-Path -LiteralPath $binDir)) {
    Write-Error @"
Build output not found: $binDir

Expected layout from Directory.Build.props:
  windows\build\bin\$Platform-$Configuration\

Build first:
  cd `"$buildDir`"
  .\build.ps1 -Configuration $Configuration -Platform $Platform
"@
    exit 1
}

# MSVC TargetName comes from ProjectName "ash-core" → ash-core.lib (hyphen)
$libCandidates = @(
    (Join-Path $binDir 'ash-core.lib'),
    (Join-Path $binDir 'ash_core.lib')
)
$libFile = $libCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $libFile) {
    Write-Error @"
Library not found in: $binDir
Looked for: ash-core.lib, ash_core.lib

Contents of bin dir:
$((Get-ChildItem -LiteralPath $binDir -ErrorAction SilentlyContinue | ForEach-Object { '  ' + $_.Name }) -join "`n")
"@
    exit 1
}

$headersDir = Join-Path $windowsDir 'src\include\ash'
if (-not (Test-Path -LiteralPath $headersDir)) {
    Write-Error "Headers not found: $headersDir"
    exit 1
}

Write-Host '========================================' -ForegroundColor Cyan
Write-Host 'ASH Pattern System - SDK Packaging' -ForegroundColor Cyan
Write-Host '========================================' -ForegroundColor Cyan
Write-Host "Repo root:     $repoRoot"
Write-Host "Platform:      $Platform"
Write-Host "Configuration: $Configuration"
Write-Host "Bin dir:       $binDir"
Write-Host "Library:       $libFile"
Write-Host "Headers:       $headersDir"
Write-Host "Output path:   $OutputPath"
Write-Host ''

# --- Stage package -----------------------------------------------------------

if (-not (Test-Path -LiteralPath $OutputPath)) {
    New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null
}

$packageName = "ash-windows-sdk-{0}-{1}" -f $Platform, $Configuration.ToLowerInvariant()
$stageDir = Join-Path $OutputPath $packageName
if (Test-Path -LiteralPath $stageDir) {
    Write-Host "Removing existing stage: $stageDir" -ForegroundColor Yellow
    Remove-Item -LiteralPath $stageDir -Recurse -Force
}

$stageInclude = Join-Path $stageDir 'include\ash'
$stageLib = Join-Path $stageDir 'lib'
New-Item -ItemType Directory -Path $stageInclude -Force | Out-Null
New-Item -ItemType Directory -Path $stageLib -Force | Out-Null

Write-Host 'Staging SDK layout...' -ForegroundColor Cyan
Copy-Item -Path (Join-Path $headersDir '*') -Destination $stageInclude -Recurse -Force
Write-Host '  Headers staged -> include\ash\' -ForegroundColor Green

Copy-Item -LiteralPath $libFile -Destination (Join-Path $stageLib (Split-Path -Leaf $libFile)) -Force
Write-Host "  Library staged -> lib\$(Split-Path -Leaf $libFile)" -ForegroundColor Green

$pdbName = [System.IO.Path]::ChangeExtension((Split-Path -Leaf $libFile), '.pdb')
$pdbFile = Join-Path $binDir $pdbName
if (Test-Path -LiteralPath $pdbFile) {
    Copy-Item -LiteralPath $pdbFile -Destination (Join-Path $stageLib $pdbName) -Force
    Write-Host "  PDB staged -> lib\$pdbName" -ForegroundColor Green
} else {
    Write-Host '  PDB not found (optional)' -ForegroundColor Yellow
}

$readme = @"
# ASH Pattern System Windows SDK

- Platform: $Platform
- Configuration: $Configuration
- Source bin: ``$binDir``

## Contents

- ``include/ash/`` — public C++ headers
- ``lib/$(Split-Path -Leaf $libFile)`` — static library
- ``lib/*.pdb`` — debug symbols when present

## Integrate

1. Add ``include`` to additional include directories.
2. Link ``lib\$(Split-Path -Leaf $libFile)``.
3. Toolchain: MSVC C++20, matching architecture ($Platform).

## Build artifacts source

Produced by:

``````powershell
cd windows\build
.\build.ps1 -Configuration $Configuration -Platform $Platform
``````

Signing: this package is **unsigned** unless a production cert is applied externally.
"@
Set-Content -LiteralPath (Join-Path $stageDir 'README.md') -Value $readme -Encoding UTF8

# --- ZIP ---------------------------------------------------------------------

Write-Host ''
Write-Host 'Creating ZIP archive...' -ForegroundColor Cyan
$zipFile = Join-Path $OutputPath ("{0}.zip" -f $packageName)
if (Test-Path -LiteralPath $zipFile) {
    Remove-Item -LiteralPath $zipFile -Force
}
Compress-Archive -Path $stageDir -DestinationPath $zipFile -Force
Write-Host "  Created: $zipFile" -ForegroundColor Green

# --- Checksums ---------------------------------------------------------------

$checksumFile = Join-Path $OutputPath ("sha256-{0}-{1}.txt" -f $Platform, $Configuration.ToLowerInvariant())
if (-not $SkipChecksums) {
    Write-Host ''
    Write-Host 'Generating SHA256 checksums...' -ForegroundColor Cyan
    $lines = New-Object System.Collections.Generic.List[string]
    Get-ChildItem -LiteralPath $stageDir -Recurse -File | ForEach-Object {
        $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        $rel = $_.FullName.Substring($stageDir.Length).TrimStart('\')
        $lines.Add("$hash  $rel") | Out-Null
    }
    $zipHash = (Get-FileHash -LiteralPath $zipFile -Algorithm SHA256).Hash
    $lines.Add("$zipHash  $(Split-Path -Leaf $zipFile)") | Out-Null
    $lines | Set-Content -LiteralPath $checksumFile -Encoding UTF8
    Write-Host "  Checksums: $checksumFile" -ForegroundColor Green
}

# --- SBOM / manifest (simple JSON) -------------------------------------------

if (-not $SkipSBOM) {
    Write-Host ''
    Write-Host 'Generating SBOM + release manifest...' -ForegroundColor Cyan
    $files = @()
    Get-ChildItem -LiteralPath $stageDir -Recurse -File | ForEach-Object {
        $rel = $_.FullName.Substring($stageDir.Length).TrimStart('\').Replace('\', '/')
        $files += [ordered]@{
            name   = $_.Name
            path   = $rel
            size   = $_.Length
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
    }

    $sbom = [ordered]@{
        name          = 'ASH Pattern System Windows SDK'
        version       = '1.0.0-alpha'
        platform      = $Platform
        configuration = $Configuration
        type          = 'SDK'
        sourceBinDir  = $binDir
        library       = (Split-Path -Leaf $libFile)
        dependencies  = @()
        files         = $files
        signingStatus = 'unsigned'
        buildDate     = (Get-Date).ToString('o')
    }
    $sbomPath = Join-Path $stageDir 'SBOM.json'
    $sbom | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $sbomPath -Encoding UTF8
    Copy-Item -LiteralPath $sbomPath -Destination (Join-Path $OutputPath ("SBOM-{0}-{1}.json" -f $Platform, $Configuration.ToLowerInvariant())) -Force

    $manifest = [ordered]@{
        name            = 'ASH Pattern System Windows SDK'
        version         = '1.0.0-alpha'
        platform        = $Platform
        configuration   = $Configuration
        packageZip      = (Split-Path -Leaf $zipFile)
        stageDirectory  = $packageName
        signingStatus   = 'unsigned'
        signingBlocked  = 'Production signing certificate not configured'
        artifactRoot    = $binDir
        releaseDate     = (Get-Date).ToString('yyyy-MM-dd')
    }
    $manifestPath = Join-Path $stageDir 'RELEASE-MANIFEST.json'
    $manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $OutputPath ("RELEASE-MANIFEST-{0}-{1}.json" -f $Platform, $Configuration.ToLowerInvariant())) -Force
    Write-Host '  SBOM + manifest written' -ForegroundColor Green
}

Write-Host ''
Write-Host '========================================' -ForegroundColor Cyan
Write-Host 'Packaging complete' -ForegroundColor Green
Write-Host '========================================' -ForegroundColor Cyan
Write-Host "Stage: $stageDir"
Write-Host "ZIP:   $zipFile"
if (-not $SkipChecksums) { Write-Host "SHA256: $checksumFile" }
exit 0
