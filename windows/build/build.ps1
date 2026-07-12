# ASH Pattern System - Windows C++ branch build script.
#
# Discovers MSBuild via vswhere.exe (both Microsoft-produced), builds the
# ash-windows.sln solution, and optionally runs the test suite after a
# successful build.
#
# Usage:
#   .\build.ps1
#   .\build.ps1 -Configuration Release
#   .\build.ps1 -Platform ARM64
#   .\build.ps1 -Test
#   .\build.ps1 -Clean

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Debug',

    [ValidateSet('x64', 'ARM64')]
    [string] $Platform = 'x64',

    [switch] $Test,

    [switch] $Clean
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$solutionPath = Join-Path $scriptDir 'ash-windows.sln'

if (-not (Test-Path $solutionPath)) {
    Write-Error "Solution file not found: $solutionPath"
    exit 1
}

# Locate vswhere (ships with Visual Studio Installer)
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere.exe not found at '$vswhere'. Install Visual Studio 2022 or Build Tools for Visual Studio 2022 with the C++ workload."
    exit 1
}

# Locate MSBuild via vswhere
$msbuild = & $vswhere `
    -latest `
    -products '*' `
    -requires 'Microsoft.Component.MSBuild' `
    -find 'MSBuild\**\Bin\MSBuild.exe' `
    | Select-Object -First 1

if (-not $msbuild -or -not (Test-Path $msbuild)) {
    Write-Error 'MSBuild.exe not found. Install the C++ workload in Visual Studio 2022 or Build Tools for Visual Studio 2022.'
    exit 1
}

Write-Host "MSBuild:       $msbuild"
Write-Host "Solution:      $solutionPath"
Write-Host "Configuration: $Configuration"
Write-Host "Platform:      $Platform"
Write-Host ''

$target = if ($Clean) { 'Clean' } else { 'Build' }

& $msbuild $solutionPath `
    "/t:$target" `
    "/p:Configuration=$Configuration" `
    "/p:Platform=$Platform" `
    /nologo `
    /m `
    /verbosity:minimal

if ($LASTEXITCODE -ne 0) {
    Write-Error "MSBuild $target failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

if ($Clean) {
    Write-Host "Clean complete." -ForegroundColor Green
    exit 0
}

$binDir = Join-Path $scriptDir ("bin\{0}-{1}" -f $Platform, $Configuration)
Write-Host ''
Write-Host "Build complete. Artifacts in: $binDir" -ForegroundColor Green

if ($Test) {
    $runAllPath = Join-Path (Split-Path -Parent $scriptDir) 'tests\run_all.ps1'
    if (-not (Test-Path $runAllPath)) {
        Write-Error "Test runner not found: $runAllPath"
        exit 1
    }

    Write-Host ''
    Write-Host 'Running tests...' -ForegroundColor Cyan
    & $runAllPath -BinDir $binDir
    exit $LASTEXITCODE
}
