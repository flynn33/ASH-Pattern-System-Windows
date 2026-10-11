[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string] $BuildRoot,
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Debug',
    [ValidateSet('x64', 'ARM64')][string] $Platform = 'x64'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$version = (Get-Content -LiteralPath (Join-Path $repositoryRoot 'VERSION') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$') { throw 'VERSION must contain one valid product version.' }
$commit = & git -C $repositoryRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $commit -notmatch '^[0-9a-f]{40}$') { throw 'Cannot determine source commit.' }
$status = @(& git -C $repositoryRoot status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Cannot determine source dirty state.' }
$dirty = if ($status.Count -gt 0) { 1 } else { 0 }
$profile = if ($Configuration -eq 'Debug') { 'development' } else { 'release' }
$includeRoot = Join-Path ([IO.Path]::GetFullPath($BuildRoot)) ("generated\{0}-{1}\include\ash" -f $Platform, $Configuration)
New-Item -ItemType Directory -Path $includeRoot -Force | Out-Null
$headerPath = Join-Path $includeRoot 'Version.hpp'
$header = @"
#pragma once
#include <string_view>
#define ASH_SDK_VERSION "$version"
#define ASH_SOURCE_COMMIT "$commit"
#define ASH_SOURCE_DIRTY $dirty
#define ASH_DIAGNOSTICS_PROFILE "$profile"
namespace ash {
inline constexpr std::string_view sdk_version = ASH_SDK_VERSION;
inline constexpr std::string_view source_commit = ASH_SOURCE_COMMIT;
inline constexpr bool source_dirty = ASH_SOURCE_DIRTY != 0;
inline constexpr std::string_view diagnostics_profile = ASH_DIAGNOSTICS_PROFILE;
}
"@
if (-not (Test-Path -LiteralPath $headerPath) -or (Get-Content -LiteralPath $headerPath -Raw) -ne $header) {
    [IO.File]::WriteAllText($headerPath, $header, (New-Object Text.UTF8Encoding($false)))
}
Write-Output $headerPath
