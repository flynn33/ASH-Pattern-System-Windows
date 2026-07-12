# Checksum Generation Instructions

**Purpose:** Generate SHA-256 checksums for ASH Pattern System Windows release artifacts.

---

## Prerequisites

- Windows host with PowerShell 5.1+ or PowerShell 7+
- Access to Release build outputs:
  - `D:\GitHub\APS\windows\build\bin\x64-Release\`
  - `D:\GitHub\APS\windows\build\bin\ARM64-Release\`

---

## Quick Command (PowerShell)

```powershell
# Navigate to Release builds directory
cd D:\GitHub\APS\windows\build\bin

# Generate checksums for x64 release artifacts
Get-ChildItem -Path "x64-Release" -File | ForEach-Object {
    $hash = Get-FileHash $_.FullName -Algorithm SHA256
    Write-Host "$($hash.Hash)  `$_"
} > x64-release-checksums.txt

# Generate checksums for ARM64 release artifacts  
Get-ChildItem -Path "ARM64-Release" -File | ForEach-Object {
    $hash = Get-FileHash $_.FullName -Algorithm SHA256
    Write-Host "$($hash.Hash)  `$_"
} > arm64-release-checksums.txt
```

---

## Manual Checksum Command (CMD)

```cmd
cd D:\GitHub\APS\windows\build\bin
for %f in ("x64-Release\*.exe" "x64-Release\*.lib") do @certutil -hashfile "%f" SHA256 >> x64-release-checksums.txt
for %f in ("ARM64-Release\*.exe" "ARM64-Release\*.lib") do @certutil -hashfile "%f" SHA256 >> arm64-release-checksums.txt
```

---

## Expected Output Format

**x64-release-checksums.txt:**
```
3a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1b2c3d4e5f6a7b  D:\GitHub\APS\windows\build\bin\x64-Release\ash-core.lib
8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5e6f7a8b9  D:\GitHub\APS\windows\build\bin\x64-Release\ash-test-statemodel.exe
... (etc)
```

**ARM64-release-checksums.txt:**
Similar format with ARM64 binaries.

---

## Verification Usage

```powershell
# Verify x64 artifacts integrity
Get-Content -Path "x64-release-checksums.txt" | ForEach-Object {
    $parts = $_.Split('  ', 2)
    $expectedHash = $parts[0].Trim()
    $filePath = $parts[1].Trim()
    
    if (Test-Path $filePath) {
        $actualHash = Get-FileHash $filePath -Algorithm SHA256 | Select-Object -ExpandProperty Hash
        if ($expectedHash -eq $actualHash) {
            Write-Host "✓ PASS: $filePath"
        } else {
            Write-Host "✗ FAIL: $filePath"
            Write-Host "  Expected: $expectedHash"
            Write-Host "  Actual:   $actualHash"
        }
    } else {
        Write-Host "✗ MISSING: $filePath"
    }
}
```

---

## Checksum Files Location

Generated checksum files will be stored in:
- `D:\GitHub\APS\windows\build\bin\x64-release-checksums.txt`
- `D:\GitHub\APS\windows\build\bin\arm64-release-checksums.txt`

These should be copied to the release evidence directory:
- `D:\GitHub\APS\completion-evidence\x64-release-checksums.txt`
- `D:\GitHub\APS\completion-evidence\arm64-release-checksums.txt`

---

**Generated:** 2026-06-21  
**Status:** Script documentation - execute on Windows host to generate checksums
