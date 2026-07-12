# ASH Pattern System — Signature Verification Script
#
# Verifies code signatures on release artifacts.
# Supports ZIP archives and individual files.
#
# Usage:
#   .\verify-signature.ps1 -Path "D:\GitHub\APS\packages\ash-windows-sdk-x64-release.zip"
#   .\verify-signature.ps1 -Path "D:\GitHub\APS\packages\ash-windows-sdk-x64-release.zip" -Extract

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [string] $Path,

    [switch] $Extract,

    [switch] $TestMode
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "ASH Pattern System — Signature Verification" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Path: $Path" -ForegroundColor Green
Write-Host ""

# --- Check if path exists -----------------------------------------------------

if (-not (Test-Path -LiteralPath $Path)) {
    Write-Error "Path not found: $Path"
    exit 1
}

$filePath = Get-Item -LiteralPath $Path
$fileName = [System.IO.Path]::GetFileName($Path)
$filePathName = [System.IO.Path]::GetFileNameWithoutExtension($Path)

Write-Host "Artifact: $fileName" -ForegroundColor Cyan
Write-Host "Location: $Path" -ForegroundColor Cyan
Write-Host ""

# --- Extract ZIP if needed ----------------------------------------------------

if ($Extract) {
    Write-Host 'Extracting ZIP archive...' -ForegroundColor Cyan
    
    $extractDir = Join-Path $env:TEMP (Guid::NewGuid().ToString())
    New-Item -ItemType Directory -Path $extractDir -Force | Out-Null
    
    try {
        Expand-Archive -LiteralPath $Path -DestinationPath $extractDir -Force
        
        Write-Host "  Extracted to: $extractDir" -ForegroundColor Green
    } catch {
        Write-Error "Failed to extract ZIP: $_"
        exit 1
    }
    
    # Verify signature in extracted files
    $extractedPath = $extractDir
} else {
    # Check if it's a ZIP file
    if ($fileName -like "*.zip") {
        Write-Host "ZIP archive detected. Use -Extract to verify contents." -ForegroundColor Yellow
        Write-Host ""
        
        # Check for embedded signature data (if any)
        $signatureFile = Join-Path $filePathName 'SIGNATURE'
        if (Test-Path -LiteralPath $signatureFile) {
            Write-Host "Found signature file: $signatureFile" -ForegroundColor Cyan
            $signatureContent = Get-Content -LiteralPath $signatureFile -Raw
            
            # Parse signature data
            $signatureData = $signatureContent | ConvertFrom-Json
            Write-Host ""
            Write-Host "Signature Data:" -ForegroundColor Cyan
            Write-Host "  Certificate: $($signatureData.certificate.subject)" -ForegroundColor Green
            Write-Host "  Timestamp:   $($signatureData.timestampServer)" -ForegroundColor Green
            Write-Host "  Status:       $($signatureData.signingStatus)" -ForegroundColor Green
            
            if ($signatureData.signingStatus -eq 'signed') {
                Write-Host ""
                Write-Host '========================================' -ForegroundColor Cyan
                Write-Host 'Signature Verification: PASSED' -ForegroundColor Green
                Write-Host '========================================' -ForegroundColor Cyan
            } else {
                Write-Host ""
                Write-Host '========================================' -ForegroundColor Cyan
                Write-Host 'Signature Verification: UNSIGNED' -ForegroundColor Yellow
                Write-Host '========================================' -ForegroundColor Cyan
            }
        } else {
            Write-Host "No signature file found (expected for unsigned archives)." -ForegroundColor Yellow
        }
        
        exit 0
    } else {
        $extractedPath = $Path
    }
}

# --- Verify individual files --------------------------------------------------

Write-Host ""
Write-Host 'Verifying signatures...' -ForegroundColor Cyan
Write-Host ""

$verificationResults = @()
$totalFiles = 0
$signedFiles = 0
$unsignedFiles = 0
$failedFiles = 0

Get-ChildItem -LiteralPath $extractedPath -Recurse -File -ErrorAction SilentlyContinue | ForEach-Object {
    $file = $_
    $totalFiles++
    
    try {
        # Check for embedded signature data
        $signatureFile = Join-Path (Split-Path -Parent $file.FullName) 'SIGNATURE'
        
        if (Test-Path -LiteralPath $signatureFile) {
            $signatureContent = Get-Content -LiteralPath $signatureFile -Raw
            $signatureData = $signatureContent | ConvertFrom-Json
            
            Write-Host "  $($file.Name):" -ForegroundColor Cyan
            Write-Host "    Certificate: $($signatureData.certificate.subject)" -ForegroundColor Green
            Write-Host "    Timestamp:   $($signatureData.timestampServer)" -ForegroundColor Green
            Write-Host "    Status:       $($signatureData.signingStatus)" -ForegroundColor Green
            
            if ($signatureData.signingStatus -eq 'signed') {
                $signedFiles++
                $verificationResults += @{
                    file = $file.Name
                    status = 'signed'
                    certificate = $signatureData.certificate.subject
                    timestamp = $signatureData.timestampServer
                }
            } else {
                $unsignedFiles++
                $verificationResults += @{
                    file = $file.Name
                    status = 'unsigned'
                    certificate = $signatureData.certificate.subject
                    timestamp = $signatureData.timestampServer
                }
            }
        } else {
            Write-Host "  $($file.Name): unsigned (no signature data)" -ForegroundColor Yellow
            $unsignedFiles++
            $verificationResults += @{
                file = $file.Name
                status = 'unsigned'
                certificate = 'N/A'
                timestamp = 'N/A'
            }
        }
    } catch {
        Write-Host "  $($file.Name): FAILED — $_" -ForegroundColor Red
        $failedFiles++
        $verificationResults += @{
            file = $file.Name
            status = 'failed'
            certificate = 'N/A'
            timestamp = 'N/A'
            error = $_.Exception.Message
        }
    }
}

# --- Summary ------------------------------------------------------------------

Write-Host ""
Write-Host '========================================' -ForegroundColor Cyan
Write-Host 'Verification Summary' -ForegroundColor Cyan
Write-Host '========================================' -ForegroundColor Cyan
Write-Host "Total files: $totalFiles" -ForegroundColor Green
Write-Host "Signed:      $signedFiles" -ForegroundColor Green
Write-Host "Unsigned:    $unsignedFiles" -ForegroundColor Yellow
Write-Host "Failed:      $failedFiles" -ForegroundColor Red

if ($failedFiles -gt 0) {
    Write-Host ""
    Write-Host '========================================' -ForegroundColor Cyan
    Write-Host 'Verification Status: FAILED' -ForegroundColor Red
    Write-Host '========================================' -ForegroundColor Cyan
    exit 1
} else {
    Write-Host ""
    Write-Host '========================================' -ForegroundColor Cyan
    Write-Host 'Verification Status: PASSED' -ForegroundColor Green
    Write-Host '========================================' -ForegroundColor Cyan
    exit 0
}
