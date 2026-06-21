# run_all.ps1
#
# Runs all Windows module test executables and aggregates exit
# codes. Intended to be invoked after a successful build via:
#
#   .\build\build.ps1 -Test
#
# or directly from a Developer PowerShell prompt:
#
#   .\tests\run_all.ps1 -BinDir ..\build\bin\x64-Debug

[CmdletBinding()]
param(
    [string] $BinDir = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not $BinDir) {
    # Default to the Debug x64 output next to the build directory.
    $BinDir = Join-Path (Split-Path -Parent $scriptDir) 'build\bin\x64-Debug'
}

if (-not (Test-Path $BinDir)) {
    Write-Error "Binary directory not found: $BinDir. Build the solution first (run .\build\build.ps1)."
    exit 1
}

$tests = @(
    'ash-test-statemodel.exe',
    'ash-test-transitionregistry.exe',
    'ash-test-diagnostics.exe',
    'ash-test-recoveryengine.exe',
    'ash-test-completionmodules.exe'
)

$totalFailures = 0
$testsRun = 0

foreach ($exeName in $tests) {
    $exePath = Join-Path $BinDir $exeName
    if (-not (Test-Path $exePath)) {
        Write-Warning "Test executable not found: $exePath (skipping)"
        continue
    }

    Write-Host ''
    Write-Host "=== $exeName ===" -ForegroundColor Cyan
    & $exePath
    $exitCode = $LASTEXITCODE
    $testsRun++
    if ($exitCode -ne 0) {
        Write-Host "  exit code: $exitCode (failures)" -ForegroundColor Red
        $totalFailures += $exitCode
    } else {
        Write-Host "  exit code: 0 (pass)" -ForegroundColor Green
    }
}

Write-Host ''
Write-Host '========================================'
Write-Host "Ran $testsRun test executable(s)"
Write-Host "Total failures: $totalFailures"

if ($totalFailures -ne 0) {
    exit $totalFailures
}
Write-Host 'All tests passed.' -ForegroundColor Green
exit 0
