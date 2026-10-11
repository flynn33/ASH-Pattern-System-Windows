[CmdletBinding()]
param(
    [ValidateSet('x64','ARM64')][string]$Platform = 'x64',
    [ValidateSet('Release')][string]$Configuration = 'Release',
    [Parameter(Mandatory=$true)][string]$BuildRoot,
    [Parameter(Mandatory=$true)][string]$AuthorityCache,
    [Parameter(Mandatory=$true)][string]$GateReport,
    [Parameter(Mandatory=$true)][string]$OutputPath,
    [Parameter(Mandatory=$true)][string]$ReportPath,
    [string]$Python = 'python'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
& $Python (Join-Path $PSScriptRoot 'sdk_release.py') package --repo-root $repoRoot --build-root $BuildRoot --authority-cache $AuthorityCache --gate-report $GateReport --platform $Platform --output $OutputPath --report $ReportPath
if ($LASTEXITCODE -ne 0) { throw "SDK packaging failed (exit $LASTEXITCODE); see $ReportPath" }
