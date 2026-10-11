[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $AuthorityCache,
    [Parameter(Mandatory)][string] $PolicyRoot,
    [Parameter(Mandatory)][string] $ProfileRoot,
    [Parameter(Mandatory)][string] $ProfileCommit,
    [Parameter(Mandatory)][string] $ContractRoot,
    [Parameter(Mandatory)][string] $BuildRoot,
    [Parameter(Mandatory)][string] $ConsumerRoot,
    [Parameter(Mandatory)][string] $OutputRoot,
    [Parameter(Mandatory)][string] $EvidenceRoot,
    [string] $IndependentReviewReport = '',
    [string] $CompletedBuildReport = '',
    [string] $Python = 'python'
)
$ErrorActionPreference = 'Stop'
$arguments = @((Join-Path $PSScriptRoot 'verify_sdk_release.py'),
    '--authority-cache', $AuthorityCache, '--policy-root', $PolicyRoot,
    '--profile-root', $ProfileRoot, '--profile-commit', $ProfileCommit,
    '--contract-root', $ContractRoot, '--build-root', $BuildRoot,
    '--consumer-root', $ConsumerRoot, '--output-root', $OutputRoot,
    '--evidence-root', $EvidenceRoot, '--powershell', (Get-Process -Id $PID).Path)
if ($IndependentReviewReport) { $arguments += @('--independent-review-report', $IndependentReviewReport) }
if ($CompletedBuildReport) { $arguments += @('--completed-build-report', $CompletedBuildReport) }
& $Python @arguments
if ($LASTEXITCODE -ne 0) { throw "SDK qualification failed with exit code $LASTEXITCODE. Inspect the generated qualification report under EvidenceRoot." }
exit 0
