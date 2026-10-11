[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Debug',
    [ValidateSet('x64', 'ARM64')][string] $Platform = 'x64',
    [string] $BuildRoot = '',
    [Alias('AuthorityCache')][string] $CanonicalAuthorityRoot = '',
    [string] $Python = 'python',
    [string] $MSBuildPath = '',
    [switch] $Test,
    [switch] $Clean,
    [switch] $Rebuild,
    [string] $ReportPath = ''
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
function Get-ASHFileHash([string] $Path) {
    $stream = [IO.File]::OpenRead($Path)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($algorithm.ComputeHash($stream))).Replace('-', '').ToLowerInvariant() }
    finally { $algorithm.Dispose(); $stream.Dispose() }
}
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$solutionPath = Join-Path $PSScriptRoot 'ash-windows.sln'
if ($Clean -and ($Rebuild -or $Test)) { throw '-Clean cleans only and cannot be combined with -Rebuild or -Test.' }
if (-not $BuildRoot) { $BuildRoot = Join-Path $PSScriptRoot 'out' }
$BuildRoot = [IO.Path]::GetFullPath($BuildRoot).TrimEnd('\')
if ($repositoryRoot.StartsWith($BuildRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or $BuildRoot -eq $repositoryRoot) { throw 'BuildRoot cannot be the repository root or an ancestor.' }
$localOutputRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'out'))
if ($BuildRoot.StartsWith($repositoryRoot + '\', [StringComparison]::OrdinalIgnoreCase) -and $BuildRoot -ne $localOutputRoot -and -not $BuildRoot.StartsWith($localOutputRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'A product-local BuildRoot must stay in windows/build/out. Select an external directory for release builds.' }
if ($CanonicalAuthorityRoot) {
    $authorityPath = [IO.Path]::GetFullPath($CanonicalAuthorityRoot).TrimEnd('\')
    if ($BuildRoot -eq $authorityPath -or $BuildRoot.StartsWith($authorityPath + '\', [StringComparison]::OrdinalIgnoreCase) -or $authorityPath.StartsWith($BuildRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'BuildRoot and the canonical authority checkout must be separate.' }
}
if ($BuildRoot -match '[;"\r\n]') { throw 'BuildRoot contains characters unsupported by the native build route.' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'vswhere.exe is required to discover the installed Microsoft C++ toolchain.' }
$installation = & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.Tools.ARM64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installation) { throw 'Visual Studio 2022 with v143 x64 and ARM64 tools was not found.' }
$installation = @($installation)[0]
$installedMSBuild = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
if (-not $MSBuildPath) { $MSBuildPath = $installedMSBuild }
if (-not $Clean -and [IO.Path]::GetFullPath($MSBuildPath) -ne [IO.Path]::GetFullPath($installedMSBuild)) { throw 'Non-clean builds require the discovered installed Microsoft MSBuild. MSBuildPath overrides are limited to clean-command regression fixtures.' }
if (-not (Test-Path -LiteralPath $MSBuildPath)) { throw 'Discovered MSBuild executable is missing.' }
$toolsVersion = (Get-Content -LiteralPath (Join-Path $installation 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
if ($toolsVersion -notmatch '^14\.4\d\.') { throw "The selected installed toolset is not v143: $toolsVersion" }
$targetTools = Join-Path $installation ("VC\Tools\MSVC\{0}\bin\Hostx64\{1}" -f $toolsVersion, $Platform.ToLowerInvariant())
foreach ($name in @('cl.exe','link.exe','lib.exe')) { if (-not (Test-Path -LiteralPath (Join-Path $targetTools $name))) { throw "Target tool missing: $name for $Platform" } }
$sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
$sdkVersions = @(Get-ChildItem -LiteralPath (Join-Path $sdkRoot 'Include') -Directory | Where-Object { $_.Name -match '^10\.0\.\d+\.0$' } | Sort-Object { [version]$_.Name } -Descending)
$sdkVersion = $null
foreach ($sdk in $sdkVersions) {
    $targetArchitecture = $Platform.ToLowerInvariant()
    if ((Test-Path -LiteralPath (Join-Path $sdk.FullName 'um\Windows.h')) -and (Test-Path -LiteralPath (Join-Path $sdkRoot ("Lib\{0}\um\{1}\kernel32.lib" -f $sdk.Name, $targetArchitecture))) -and (Test-Path -LiteralPath (Join-Path $sdkRoot ("Lib\{0}\ucrt\{1}\libucrt.lib" -f $sdk.Name, $targetArchitecture)))) { $sdkVersion = $sdk.Name; break }
}
if (-not $sdkVersion) { throw "An installed Windows SDK with $Platform headers and libraries is required." }
$properties = @("/p:Configuration=$Configuration", "/p:Platform=$Platform", "/p:ASHBuildRoot=$BuildRoot", "/p:VCToolsVersion=$toolsVersion", "/p:WindowsTargetPlatformVersion=$sdkVersion")
$binDir = Join-Path $BuildRoot ("bin\{0}-{1}" -f $Platform, $Configuration)
$metadataPath = Join-Path $binDir 'build-metadata.json'
if (Test-Path -LiteralPath $metadataPath) { Remove-Item -LiteralPath $metadataPath }
if (-not $Clean) {
    & (Join-Path $PSScriptRoot 'generate-version.ps1') -BuildRoot $BuildRoot -Configuration $Configuration -Platform $Platform
    if ($CanonicalAuthorityRoot) {
        $generator = Join-Path $PSScriptRoot '..\tests\generate-canonical-fixtures.py'
        if (-not (Test-Path -LiteralPath $generator)) { throw 'Canonical fixture generator is missing.' }
        & $Python $generator --authority-cache $CanonicalAuthorityRoot --output (Join-Path $BuildRoot 'generated\tests\CanonicalFixtures.hpp')
        if ($LASTEXITCODE -ne 0) { throw "Canonical fixture generation failed: $LASTEXITCODE" }
    }
}
function Get-SourceSnapshot {
    $paths = @(& git -c core.quotePath=false -C $repositoryRoot ls-files --cached --others --exclude-standard)
    if ($LASTEXITCODE -ne 0) { throw 'Cannot inventory source snapshot.' }
    $files = @($paths | Sort-Object -Unique | ForEach-Object {
        $path = Join-Path $repositoryRoot $_
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing source snapshot file: $_" }
        [PSCustomObject]@{ path = $_.Replace('\','/'); sha256 = (Get-ASHFileHash $path) }
    })
    $content = ($files | ForEach-Object { $_.path + "`t" + $_.sha256 }) -join "`n"
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $digest = ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($content)))).Replace('-','').ToLowerInvariant() } finally { $sha.Dispose() }
    return [PSCustomObject]@{ fingerprint = $digest; files = $files }
}
$before = if (-not $Clean) { Get-SourceSnapshot } else { $null }
$startedAt = [DateTime]::UtcNow.ToString('o')
$target = if ($Clean) { 'Clean' } elseif ($Rebuild) { 'Rebuild' } else { 'Build' }
Write-Host "MSBuild: $MSBuildPath"
Write-Host "Target: $target; configuration: $Configuration; platform: $Platform; output: $BuildRoot"
& $MSBuildPath $solutionPath "/t:$target" @properties /nologo /m /verbosity:minimal
if ($LASTEXITCODE -ne 0) { throw "MSBuild $target failed with exit code $LASTEXITCODE" }
if ($Clean) { Write-Host 'Clean complete; no build or tests were run.'; exit 0 }
$after = Get-SourceSnapshot
if ($before.fingerprint -ne $after.fingerprint) { throw 'Source changed during the build; outputs have no valid completed-build metadata. Rebuild from a stable source snapshot.' }
$libraryPath = Join-Path $binDir 'ash-core.lib'
if (-not (Test-Path -LiteralPath $libraryPath -PathType Leaf)) { throw 'MSBuild returned success without the required ash-core.lib.' }
$evaluationRaw = & $MSBuildPath (Join-Path $PSScriptRoot 'ash-core.vcxproj') @properties -nologo '-getProperty:WindowsTargetPlatformVersion,VCToolsVersion,PlatformToolset,OutDir,IntDir,ASHGeneratedInclude' '-getItem:ClCompile,ProjectReference'
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect evaluated native build settings.' }
$evaluation = ($evaluationRaw -join "`n") | ConvertFrom-Json
$compileItems = @($evaluation.Items.ClCompile)
foreach ($item in $compileItems) {
    $expectedRuntime = if ($Configuration -eq 'Release') { 'MultiThreaded' } else { 'MultiThreadedDebug' }
    if ($item.RuntimeLibrary -ne $expectedRuntime -or $item.LanguageStandard -ne 'stdcpp20' -or $item.WholeProgramOptimization -ne 'false' -or $item.DebugInformationFormat -ne 'OldStyle') { throw 'Evaluated compile settings do not match the native binary contract.' }
}
$commit = (& git -C $repositoryRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot read built source identity.' }
$dirty = @(& git -C $repositoryRoot status --porcelain --untracked-files=all).Count -gt 0
if ($LASTEXITCODE -ne 0) { throw 'Cannot read built source dirty state.' }
$identityHeaderPath = Join-Path $evaluation.Properties.ASHGeneratedInclude 'ash\Version.hpp'
$identityHeader = Get-Content -LiteralPath $identityHeaderPath -Raw
$expectedDirty = if ($dirty) { '1' } else { '0' }
if ($identityHeader -notmatch ('#define ASH_SOURCE_COMMIT "' + [regex]::Escape($commit) + '"') -or $identityHeader -notmatch ('#define ASH_SOURCE_DIRTY ' + $expectedDirty + '(?:\r?\n|$)')) { throw 'Generated native identity differs from completed-build provenance. Rebuild from a stable checkout.' }
$profile = if ($Configuration -eq 'Release') { 'release' } else { 'development' }
$metadata = [ordered]@{
    schemaVersion = '1.0'; version = (Get-Content -LiteralPath (Join-Path $repositoryRoot 'VERSION') -Raw).Trim(); sourceCommit = $commit; sourceDirty = $dirty;
    sourceSnapshot = $after; platform = $Platform; configuration = $Configuration; diagnosticsProfile = $profile;
    startedAtUtc = $startedAt; finishedAtUtc = [DateTime]::UtcNow.ToString('o'); generatedInclude = $evaluation.Properties.ASHGeneratedInclude;
    generatedIdentity = @{ relativeName = "generated/$Platform-$Configuration/include/ash/Version.hpp"; sha256 = (Get-ASHFileHash $identityHeaderPath) };
    library = @{ name = 'ash-core.lib'; sha256 = (Get-ASHFileHash $libraryPath) };
    binaries = @(Get-ChildItem -LiteralPath $binDir -Filter 'ash-test-*.exe' -File | Sort-Object Name | ForEach-Object { @{ name = $_.Name; sha256 = (Get-ASHFileHash $_.FullName) } });
    toolchain = @{ msbuild = (Get-Item -LiteralPath $MSBuildPath).VersionInfo.FileVersion; compiler = (Get-Item -LiteralPath (Join-Path $targetTools 'cl.exe')).VersionInfo.FileVersion; linker = (Get-Item -LiteralPath (Join-Path $targetTools 'link.exe')).VersionInfo.FileVersion; librarian = (Get-Item -LiteralPath (Join-Path $targetTools 'lib.exe')).VersionInfo.FileVersion; windowsSdk = $evaluation.Properties.WindowsTargetPlatformVersion; toolset = $evaluation.Properties.PlatformToolset; toolsVersion = $toolsVersion };
    settings = @{ runtimeLibrary = $(if ($Configuration -eq 'Release') { '/MT' } else { '/MTd' }); languageStandard = 'C++20'; wholeProgramOptimization = $false; debugInformationFormat = '/Z7' };
    evaluatedCompileItems = $compileItems;
    tests = @{ status = 'not_run'; report = $null }
}
if ($Test) {
    if (-not $ReportPath) { $ReportPath = Join-Path $binDir 'test-results.json' }
    & (Join-Path $PSScriptRoot '..\tests\run_all.ps1') -BinDir $binDir -ReportPath $ReportPath
    if ($LASTEXITCODE -ne 0) { throw "Native tests failed; report: $ReportPath" }
    $metadata.tests = @{ status = 'pass'; report = [IO.Path]::GetFullPath($ReportPath); sha256 = (Get-ASHFileHash $ReportPath) }
}
[IO.File]::WriteAllText($metadataPath, ($metadata | ConvertTo-Json -Depth 12), (New-Object Text.UTF8Encoding($false)))
Write-Host "Build complete. Provenance: $metadataPath"
exit 0
