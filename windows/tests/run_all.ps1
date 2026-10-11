[CmdletBinding()]
param(
    [string] $BinDir = '',
    [string] $InventoryPath = '',
    [string] $ReportPath = '',
    [ValidateRange(1, 3600)][int] $TimeoutSeconds = 120,
    [ValidateSet('Native', 'Emulated')][string] $ExecutionRoute = 'Native'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
function Get-ASHFileHash([string] $Path) {
    $stream = [IO.File]::OpenRead($Path)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($algorithm.ComputeHash($stream))).Replace('-', '').ToLowerInvariant() }
    finally { $algorithm.Dispose(); $stream.Dispose() }
}
if (-not $BinDir) { $BinDir = Join-Path $PSScriptRoot '..\build\out\bin\x64-Debug' }
if (-not $InventoryPath) { $InventoryPath = Join-Path $PSScriptRoot 'test-inventory.json' }
$startedAt = [DateTime]::UtcNow.ToString('o')
$sourceRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$sourceCommit = & git -C $sourceRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot determine test source commit.' }
$sourceDirty = @(& git -C $sourceRoot status --porcelain --untracked-files=all).Count -gt 0
if ($LASTEXITCODE -ne 0) { throw 'Cannot determine test source dirty state.' }
$results = New-Object 'Collections.Generic.List[object]'
$overallError = $null
try {
    if (-not (Test-Path -LiteralPath $BinDir -PathType Container)) { throw "Binary directory not found: $BinDir" }
    $BinDir = (Resolve-Path -LiteralPath $BinDir).Path
    $inventory = Get-Content -LiteralPath $InventoryPath -Raw | ConvertFrom-Json
    $tests = @($inventory.tests)
    if ($inventory.schemaVersion -ne '1.0' -or $tests.Count -eq 0) { throw 'Test inventory must be version 1.0 and nonempty.' }
    if (@($tests | Select-Object -Unique).Count -ne $tests.Count) { throw 'Test inventory contains duplicate executables.' }
    foreach ($name in $tests) {
        if ($name -notmatch '^ash-test-[a-z0-9-]+\.exe$') { throw "Unsafe test inventory filename: $name" }
    }
    $hostArchitecture = $env:PROCESSOR_ARCHITECTURE
    if ($env:PROCESSOR_ARCHITEW6432) { $hostArchitecture = $env:PROCESSOR_ARCHITEW6432 }
    foreach ($name in $tests) {
        $path = Join-Path $BinDir $name
        $result = [ordered]@{ name = $name; sha256 = $null; status = 'not_started'; exitCode = $null; startedAtUtc = [DateTime]::UtcNow.ToString('o'); finishedAtUtc = $null; caseCount = $null; caseFailures = $null; output = ''; error = $null }
        $process = $null
        try {
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $result.status = 'missing'; throw "Required executable missing: $name" }
            $result.sha256 = (Get-ASHFileHash $path)
            $stream = [IO.File]::OpenRead($path)
            try {
                $reader = New-Object IO.BinaryReader($stream)
                if ($reader.ReadUInt16() -ne 0x5a4d) { throw 'Invalid executable DOS signature.' }
                $stream.Position = 0x3c
                $peOffset = $reader.ReadInt32()
                if ($peOffset -lt 0 -or $peOffset -gt $stream.Length - 6) { throw 'Invalid executable PE offset.' }
                $stream.Position = $peOffset
                if ($reader.ReadUInt32() -ne 0x4550) { throw 'Invalid executable PE signature.' }
                $machine = $reader.ReadUInt16()
            } finally { $stream.Dispose() }
            if ($machine -notin @(0x8664, 0xaa64)) { $result.status = 'architecture_mismatch'; throw ('Unsupported executable machine 0x{0:x4}; the native test inventory supports x64 and ARM64 only.' -f $machine) }
            if ($machine -eq 0xaa64 -and $hostArchitecture -ne 'ARM64') { $result.status = 'architecture_mismatch'; throw 'ARM64 execution requires an ARM64 host or explicitly available authorized ARM64 execution environment.' }
            if ($machine -eq 0x8664 -and $hostArchitecture -eq 'ARM64' -and $ExecutionRoute -ne 'Emulated') { $result.status = 'architecture_mismatch'; throw 'x64 execution on ARM64 must be explicitly labeled Emulated.' }
            $info = New-Object Diagnostics.ProcessStartInfo
            $info.FileName = $path
            $info.WorkingDirectory = $BinDir
            $info.UseShellExecute = $false
            $info.CreateNoWindow = $true
            $info.RedirectStandardOutput = $true
            $info.RedirectStandardError = $true
            $process = New-Object Diagnostics.Process
            $process.StartInfo = $info
            if (-not $process.Start()) { throw 'Process launch returned false.' }
            $deadline = [Diagnostics.Stopwatch]::StartNew()
            $stdout = $process.StandardOutput.ReadToEndAsync()
            $stderr = $process.StandardError.ReadToEndAsync()
            if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
                $result.status = 'timeout'
                $process.Kill()
                if (-not $process.WaitForExit(1000)) { throw 'Timed-out process did not exit within the cleanup deadline.' }
                throw "Test exceeded $TimeoutSeconds seconds."
            }
            $result.exitCode = $process.ExitCode
            $remainingMilliseconds = [Math]::Max(0, ($TimeoutSeconds * 1000) - [int]$deadline.ElapsedMilliseconds)
            $result.status = 'output_failed'
            if (-not [Threading.Tasks.Task]::WaitAll([Threading.Tasks.Task[]]@($stdout, $stderr), $remainingMilliseconds)) {
                $result.status = 'output_timeout'
                throw "Test output did not close within $TimeoutSeconds seconds; a descendant may still hold a redirected pipe."
            }
            $result.output = $stdout.Result + $stderr.Result
            $matches = [regex]::Matches($result.output, 'Total:\s+(\d+)\s+Failures:\s+(\d+)')
            if ($matches.Count -gt 0) {
                $result.caseCount = [int]$matches[$matches.Count - 1].Groups[1].Value
                $result.caseFailures = [int]$matches[$matches.Count - 1].Groups[2].Value
            }
            $result.status = if ($process.ExitCode -eq 0) { 'passed' } else { 'failed' }
            if ($process.ExitCode -eq 0 -and ($null -eq $result.caseCount -or $result.caseCount -eq 0)) { $result.status = 'empty_suite'; $result.error = 'Executable did not report a nonempty test-case inventory.' }
            if ($process.ExitCode -eq 0 -and $null -ne $result.caseFailures -and $result.caseFailures -ne 0) { $result.status = 'failed'; $result.error = 'Test-case failure report contradicts the process success exit code.' }
        } catch {
            if ($result.status -eq 'not_started') { $result.status = 'launch_failed' }
            $result.error = $_.Exception.Message
        } finally {
            if ($null -ne $process) {
                if ($process.StartInfo.RedirectStandardOutput) {
                    try { $process.StandardOutput.Dispose() } catch { }
                    try { $process.StandardError.Dispose() } catch { }
                }
                $process.Dispose()
            }
            $result.finishedAtUtc = [DateTime]::UtcNow.ToString('o')
        }
        $results.Add([PSCustomObject]$result)
        Write-Host ("=== {0}: {1} ===" -f $name, $result.status)
        if ($result.output) { Write-Host $result.output }
        if ($result.error) { Write-Host $result.error }
    }
} catch { $overallError = $_.Exception.Message; Write-Host $overallError }
$failedRuns = @($results | Where-Object { $_.status -ne 'passed' }).Count
$passedRuns = @($results | Where-Object { $_.status -eq 'passed' }).Count
$passed = $null -eq $overallError -and $results.Count -gt 0 -and $failedRuns -eq 0
$report = [ordered]@{ schemaVersion = '1.0'; status = $(if ($passed) { 'pass' } else { 'fail' }); sourceCommit = $sourceCommit; sourceDirty = $sourceDirty; powershellVersion = $PSVersionTable.PSVersion.ToString(); hostArchitecture = $env:PROCESSOR_ARCHITECTURE; binDir = $BinDir; inventoryPath = $InventoryPath; executionRoute = $ExecutionRoute; startedAtUtc = $startedAt; finishedAtUtc = [DateTime]::UtcNow.ToString('o'); requiredRuns = $results.Count; passedRuns = $passedRuns; failedRuns = $failedRuns; error = $overallError; results = @($results.ToArray()) }
if ($ReportPath) {
    $ReportPath = [IO.Path]::GetFullPath($ReportPath)
    New-Item -ItemType Directory -Path (Split-Path -Parent $ReportPath) -Force | Out-Null
    [IO.File]::WriteAllText($ReportPath, ($report | ConvertTo-Json -Depth 8), (New-Object Text.UTF8Encoding($false)))
}
Write-Host ("Required runs: {0}; passed: {1}; failed: {2}" -f $results.Count, $passedRuns, $failedRuns)
if (-not $passed) { exit 1 }
exit 0
