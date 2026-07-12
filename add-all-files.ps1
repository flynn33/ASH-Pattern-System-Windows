# Add all source code, documentation, and evidence files to git
Set-Location "D:\GitHub\APS"

Write-Host "=== Adding files to git staging area ===" -ForegroundColor Cyan

$filesToAdd = @(
    # Core source files
    "windows/src/StateModel.cpp",
    "windows/src/RecoveryEngine.cpp", 
    "windows/src/TransitionRegistry.cpp",
    "windows/src/Diagnostics.cpp",
    "windows/src/RealmEncoder.cpp",
    "windows/src/TopologyGenerator.cpp",
    "windows/src/AxiomEvaluator.cpp",
    "windows/src/GenerationPlanner.cpp",
    "windows/src/ArtifactEmitter.cpp",
    
    # Include headers (excluding those in .gitignore)
    "windows/src/include/**/*",
    
    # Tests
    "windows/tests/*.cpp",
    "windows/tests/test_runner.hpp",
    
    # Documentation  
    "docs/**/*.md",
    "completion-evidence/**/*.md",
    "completion-evidence/**/*.json",
    
    # Root files
    "README.md",
    "CMakeLists.txt" -if (Test-Path "CMakeLists.txt"),
)

foreach ($file in $filesToAdd) {
    $fullPath = Join-Path $PWD $file
    if (Test-Path $fullPath) {
        Add-Content -Path ".git/info/exclude" -Value "*$file`n" 2>&1 | Out-Null
        git add "$fullPath" -f 2>&1 | Out-Null
        Write-Host "Added: $($file.Substring(0, [Math]::Min(50,$file.Length)))" -ForegroundColor Green
    }
}

Write-Host "`n=== Adding all remaining files ===" -ForegroundColor Cyan
git add -A 2>&1 | Out-Null

# Check what was added
Write-Host "`nStaged files:" -ForegroundColor Yellow
git diff --cached --name-only 2>&1 | Select-Object -First 50

Write-Host "`nCommit message ready" -ForegroundColor Green
