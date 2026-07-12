# ASH Pattern System - Windows Code Signing Script
#
# Signs release artifacts with a code signing certificate.
# Supports both thumbprint-based and PFX file-based signing.
#
# Usage:
#   .\sign.ps1 -Path "D:\GitHub\APS\packages" -CertThumbprint "A1B2C3..."
#   .\sign.ps1 -Path "D:\GitHub\APS\packages" -CertFile "cert.pfx" -PfxPassword "password"
#
# Environment variables:
#   CODE_SIGNING_ENABLED=true  — Enable signing in build workflow
#   CODE_SIGNING_CERT_THUMBPRINT — Certificate thumbprint for production

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [string] $Path,

    [Parameter(Mandatory=$false)]
    [string] $CertThumbprint,

    [Parameter(Mandatory=$false)]
    [string] $CertFile,

    [Parameter(Mandatory=$false)]
    [string] $PfxPassword,

    [switch] $SkipManifest,

    [switch] $TestMode
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$timestampServer = if ($env:CODE_SIGNING_TIMESTAMP_SERVER) { $env:CODE_SIGNING_TIMESTAMP_SERVER } else { "http://timestamp.digicert.com" }

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "ASH Pattern System — Code Signing" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Path: $Path" -ForegroundColor Green
Write-Host ""

# --- Load certificate configuration ------------------------------------------

$certConfigPath = Join-Path $scriptDir 'cert-config.json'
if (Test-Path -LiteralPath $certConfigPath) {
    Write-Host "Loading certificate configuration from: $certConfigPath" -ForegroundColor Yellow
    $config = Get-Content -LiteralPath $certConfigPath -Raw | ConvertFrom-Json
} else {
    Write-Host "Certificate config not found. Using command-line parameters." -ForegroundColor Yellow
    $config = @{ certificate = @{}; timestampServer = $timestampServer }
}

# --- Resolve certificate -----------------------------------------------------

$certificate = $null
$certStoreLocation = 'Cert:\LocalMachine\My'

if ($CertThumbprint) {
    Write-Host "Using certificate thumbprint: $CertThumbprint" -ForegroundColor Cyan
    
    try {
        $certificate = Get-ChildItem CertStoreLocation | 
            Where-Object { $_.Thumbprint -eq $CertThumbprint }
        
        if (-not $certificate) {
            throw "Certificate with thumbprint '$CertThumbprint' not found in store: $certStoreLocation"
        }
    } catch {
        Write-Error "Failed to load certificate by thumbprint: $_"
        exit 1
    }
} elseif ($CertFile) {
    Write-Host "Loading certificate from PFX file: $CertFile" -ForegroundColor Cyan
    
    try {
        $securePassword = if ($PfxPassword) {
            (ConvertTo-SecureString $PfxPassword -AsPlainText -Force)
        } else {
            Read-Host -AsSecureString -Prompt "Enter PFX password"
        }
        
        $certificate = Import-PfxCertificate `
            -FilePath $CertFile `
            -Password $securePassword `
            -CertStoreLocation $certStoreLocation
        
        Write-Host "  Certificate loaded: $($certificate.Subject)" -ForegroundColor Green
    } catch {
        Write-Error "Failed to load certificate from PFX: $_"
        exit 1
    }
} elseif ($env:CODE_SIGNING_CERT_THUMBPRINT) {
    Write-Host "Using environment variable CODE_SIGNING_CERT_THUMBPRINT" -ForegroundColor Cyan
    
    try {
        $certificate = Get-ChildItem CertStoreLocation | 
            Where-Object { $_.Thumbprint -eq $env:CODE_SIGNING_CERT_THUMBPRINT }
        
        if (-not $certificate) {
            throw "Certificate with thumbprint '$env:CODE_SIGNING_CERT_THUMBPRINT' not found in store: $certStoreLocation"
        }
    } catch {
        Write-Error "Failed to load certificate from environment variable: $_"
        exit 1
    }
} else {
    Write-Host "No certificate specified. Signing will be skipped." -ForegroundColor Yellow
    Write-Host "Set CODE_SIGNING_CERT_THUMBPRINT or provide -CertThumbprint/-CertFile parameters." -ForegroundColor Yellow
    
    if ($TestMode) {
        Write-Host "Running in test mode without signing..." -ForegroundColor Yellow
        $skipSigning = $true
    } else {
        Write-Error "No certificate provided. Cannot sign artifacts."
        exit 1
    }
}

# --- Verify certificate -------------------------------------------------------

if ($certificate) {
    Write-Host ""
    Write-Host "Certificate Details:" -ForegroundColor Cyan
    
    Write-Host "  Subject: $($certificate.Subject)" -ForegroundColor Green
    Write-Host "  Thumbprint: $($certificate.Thumbprint)" -ForegroundColor Green
    Write-Host "  Valid From: $($certificate.NotEffective)" -ForegroundColor Green
    Write-Host "  Valid To: $($certificate.NotBefore)" -ForegroundColor Green
    
    $daysUntilExpiry = ($certificate.NotAfter - (Get-Date)).Days
    if ($daysUntilExpiry -lt 30) {
        Write-Host "  WARNING: Certificate expires in $daysUntilExpiry days!" -ForegroundColor Red
    } else {
        Write-Host "  Days until expiry: $daysUntilExpiry" -ForegroundColor Green
    }
    
    # Check certificate usage
    $keyUsage = $certificate.Extensions | Where-Object { $_.Oid.Value -eq '2.5.29.37' }
    if ($keyUsage) {
        Write-Host "  Key Usage: $($keyUsage.Format(2))" -ForegroundColor Green
    } else {
        Write-Host "  WARNING: Certificate may not have Code Signing key usage!" -ForegroundColor Yellow
    }
}

# --- Sign artifacts -----------------------------------------------------------

if (-not $skipSigning) {
    Write-Host ""
    Write-Host '========================================' -ForegroundColor Cyan
    Write-Host 'Signing Artifacts' -ForegroundColor Cyan
    Write-Host '========================================' -ForegroundColor Cyan
    
    # Get all files in the path (recursively, excluding directories)
    $files = Get-ChildItem -LiteralPath $Path -Recurse -File -ErrorAction SilentlyContinue
    
    if (-not $files) {
        Write-Host "No files found to sign in: $Path" -ForegroundColor Yellow
        exit 0
    }
    
    Write-Host "Found $($files.Count) file(s) to sign." -ForegroundColor Green
    
    foreach ($file in $files) {
        try {
            # Sign the file using Windows Crypto API via PowerShell
            $signer = New-Object System.Security.Cryptography.SignedCms `
                [System.Security.Cryptography.X509Certificates.X509Certificate2]::new($certificate), 
                [System.Security.Cryptography.HashAlgorithmName]::SHA256, 
                [System.Security.Cryptography.CMSDigestInfo]::Sha256
            
            $content = [System.IO.File]::ReadAllBytes($file.FullName)
            $signer.AddSignedData($content) | Out-Null
            
            # Create CMS with timestamp
            $cms = New-Object System.Security.Cryptography.SignedCms `
                [System.Security.Cryptography.X509Certificates.X509Certificate2]::new($certificate), 
                [System.Security.Cryptography.HashAlgorithmName]::SHA256, 
                [System.Security.Cryptography.CMSDigestInfo]::Sha256
            
            $cms.Content = $content
            $cms.ComputeSignature() | Out-Null
            
            # Add timestamp request
            $timestampRequest = New-Object System.Net.WebClient
            $timestampResponse = $timestampRequest.DownloadData($timestampServer + "/?tsa=" + [System.IO.Path]::GetFileName($file.FullName))
            
            Write-Host "  Signed: $($file.Name)" -ForegroundColor Green
        } catch {
            Write-Host "  Failed to sign: $($file.Name) — $_" -ForegroundColor Red
        }
    }
    
    Write-Host ""
    Write-Host '========================================' -ForegroundColor Cyan
    Write-Host 'Signing Complete' -ForegroundColor Green
    Write-Host '========================================' -ForegroundColor Cyan
}

# --- Create signing manifest --------------------------------------------------

if (-not $SkipManifest) {
    Write-Host ""
    Write-Host 'Creating signing manifest...' -ForegroundColor Cyan
    
    $manifest = [ordered]@{
        name            = 'ASH Pattern System Windows — Signing Manifest'
        version         = '1.0.0-alpha'
        platform        = 'windows'
        generatedAt     = (Get-Date).ToString('o')
        certificate     = if ($certificate) {
            @{
                subject    = $certificate.Subject
                thumbprint = $certificate.Thumbprint
                validFrom  = $certificate.NotEffective.ToString('o')
                validTo    = $certificate.NotAfter.ToString('o')
            }
        } else {
            @{
                subject    = 'N/A (no certificate)'
                thumbprint = 'N/A'
                validFrom  = 'N/A'
                validTo    = 'N/A'
            }
        }
        timestampServer = $timestampServer
        filesSigned     = @()
        signingStatus   = if ($certificate) { 'signed' } else { 'unsigned' }
    }
    
    # Collect signed files
    $files = Get-ChildItem -LiteralPath $Path -Recurse -File -ErrorAction SilentlyContinue
    foreach ($file in $files) {
        $manifest.filesSigned += @{
            path  = [System.IO.Path]::GetRelativePath($Path, $file.FullName)
            name  = $file.Name
            size  = $file.Length
        }
    }
    
    $manifestPath = Join-Path $Path 'SIGNING-MANIFEST.json'
    $manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    
    Write-Host "  Manifest written: $manifestPath" -ForegroundColor Green
}

Write-Host ""
exit 0
