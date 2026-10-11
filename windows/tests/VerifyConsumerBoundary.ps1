[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $ConsumerRoot,
    [Parameter(Mandatory)][string] $SDKRoot,
    [Parameter(Mandatory)][string] $ReportPath
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$problems = New-Object 'Collections.Generic.List[string]'
$ConsumerRoot = [IO.Path]::GetFullPath($ConsumerRoot)
$SDKRoot = [IO.Path]::GetFullPath($SDKRoot)
$selfBytes = [IO.File]::ReadAllBytes($PSCommandPath)
try {
    foreach ($root in @($ConsumerRoot, $SDKRoot)) {
        if (-not (Test-Path -LiteralPath $root -PathType Container)) { throw "Missing boundary root: $root" }
    }
    foreach ($file in Get-ChildItem -LiteralPath $ConsumerRoot -Recurse -Force) {
        if ($file.Attributes -band [IO.FileAttributes]::ReparsePoint) { $problems.Add('Reparse points are not permitted in the acceptance tree.'); continue }
        $relative = $file.FullName.Substring($ConsumerRoot.Length).TrimStart('\').Replace('\', '/')
        if ($relative -match '(^|/)(windows/(src|tests)|Vendor/ASH-Pattern-System|ThirdParty/ASH-Pattern-System|External/ASH-Pattern-System)(/|$)' -or $file.Name -eq '.gitmodules') {
            $problems.Add("Prohibited repository acquisition: $relative")
        }
        $sameGuard = $false
        if (-not $file.PSIsContainer -and $file.Name -eq 'VerifyConsumerBoundary.ps1') {
            $guardBytes = [IO.File]::ReadAllBytes($file.FullName)
            $sameGuard = [Convert]::ToBase64String($guardBytes) -eq [Convert]::ToBase64String($selfBytes)
        }
        if (-not $file.PSIsContainer -and -not $sameGuard -and $file.Extension -in @('.vcxproj','.props','.targets','.sln','.cmake','.ps1','.bat')) {
            $content = [IO.File]::ReadAllText($file.FullName)
            if ($content -match '(?i)ash-test-|<ProjectReference\b|github\.com/flynn33/ASH-Pattern-System(?:-Windows)?(?:\.git|/archive|/raw)|windows[\\/]src|windows[\\/]tests|FetchContent|ExternalProject_Add') {
                $problems.Add("Prohibited source/test dependency or acquisition in $relative")
            }
        }
    }
    foreach ($relative in @('include\ash\Version.hpp','msbuild\ASHPatternSystem.props','msbuild\ASHPatternSystem.targets')) {
        if (-not (Test-Path -LiteralPath (Join-Path $SDKRoot $relative) -PathType Leaf)) { $problems.Add("SDK input missing: $relative") }
    }
    if (-not (Test-Path -LiteralPath (Join-Path $ConsumerRoot 'app\src\ConsumerApp.cpp') -PathType Leaf)) { $problems.Add('Consumer-owned application implementation is missing.') }
    if (-not (Test-Path -LiteralPath (Join-Path $ConsumerRoot 'app\StandaloneConsumer.vcxproj') -PathType Leaf)) { $problems.Add('Standalone consumer target is missing.') }
} catch { $problems.Add($_.Exception.Message) }
$passed = $problems.Count -eq 0
$report = [ordered]@{ schemaVersion = '1.0'; status = $(if ($passed) { 'pass' } else { 'fail' }); kind = 'static-consumer-boundary'; consumerRoot = $ConsumerRoot; sdkRoot = $SDKRoot; errors = @($problems.ToArray()); evaluatedDependencyGraphVerified = $false }
$ReportPath = [IO.Path]::GetFullPath($ReportPath)
[IO.Directory]::CreateDirectory((Split-Path -Parent $ReportPath)) | Out-Null
[IO.File]::WriteAllText($ReportPath, ($report | ConvertTo-Json -Depth 5), (New-Object Text.UTF8Encoding($false)))
if (-not $passed) { $problems | ForEach-Object { Write-Host $_ }; exit 1 }
Write-Host 'Static consumer boundary passed. Evaluated build and link inspection is a separate required check.'
exit 0
