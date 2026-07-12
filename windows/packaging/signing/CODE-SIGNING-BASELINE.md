# Code Signing Baseline — ASH Pattern System Windows

**Date:** 2026-07-13  
**Status:** `READY_FOR_CERTIFICATE_CONFIGURATION`  

---

## Overview

This document defines the code-signing capability for the ASH Pattern System Windows edition. The implementation provides:

- ✅ **Signing scripts** (PowerShell + C++ tooling)
- ✅ **Certificate configuration templates**
- ✅ **Signature verification utilities**
- ✅ **Build integration hooks**
- ⏳ **Production certificate placeholder** (requires Microsoft Code Signing Certificate)

---

## Current Status

| Capability | Status | Notes |
|------------|--------|-------|
| Signing scripts | ✅ Complete | PowerShell + C++ tooling ready |
| Certificate configuration | ✅ Template ready | Requires production cert |
| Signature verification | ✅ Complete | SHA256 + timestamp validation |
| Build integration | ✅ Complete | Automated signing hooks |
| Production certificate | ⏳ Pending | Requires Microsoft Code Signing CA |

---

## Implementation Components

### 1. Signing Scripts (`signing/sign.ps1`)

Automated signing workflow for release builds:

```powershell
# Usage:
#   .\sign.ps1 -Path "D:\GitHub\APS\packages" -CertThumbprint "YOUR_THUMBPRINT"
#   .\sign.ps1 -Path "D:\GitHub\APS\packages" -CertFile "cert.pfx" -PfxPassword "password"
```

**Capabilities:**
- Signs ZIP archives with timestamp
- Signs individual files (lib, exe)
- Creates code-signing manifest
- Validates certificate chain
- Supports Windows Code Signing Certificate (EV or OV)

### 2. Certificate Configuration (`signing/cert-config.json`)

Template for certificate management:

```json
{
  "certificate": {
    "type": "windows-code-signing",
    "provider": "microsoft-ca",
    "thumbprint": "",
    "pfxPath": "",
    "pfxPassword": "",
    "validFrom": "2026-01-01T00:00:00Z",
    "validTo": "2031-01-01T00:00:00Z"
  },
  "timestampServer": "http://timestamp.digicert.com",
  "ocspResponder": "http://ocsp.digicert.com"
}
```

### 3. Signature Verification (`signing/verify-signature.ps1`)

Validate signed artifacts:

```powershell
# Usage:
#   .\verify-signature.ps1 -Path "D:\GitHub\APS\packages\ash-windows-sdk-x64-release.zip"
```

**Outputs:**
- Certificate chain validation
- Timestamp verification
- Signature integrity check
- Trust status (trusted/untrusted)

### 4. Build Integration (`build/build.ps1`)

Signing hook in build workflow:

```powershell
# After build completes, optionally sign artifacts
if ($env:CODE_SIGNING_ENABLED -eq "true") {
    & "$repoRoot\windows\packaging\signing\sign.ps1" `
        -Path "$buildDir\bin\$Platform-$Configuration" `
        -CertThumbprint $env:CODE_SIGNING_CERT_THUMBPRINT
}
```

---

## Production Certificate Requirements

### Microsoft Code Signing Certificate

To enable production signing, obtain a **Microsoft Code Signing Certificate**:

1. **Certificate Authority:** Microsoft Code Signing CA (or DigiCert via Microsoft)
2. **Certificate Type:** Extended Validation (EV) or Organization Validation (OV)
3. **Key Usage:** Code Signing only
4. **Validity:** Minimum 5 years recommended
5. **Cost:** ~$1,000-$2,000/year for EV

### Certificate Acquisition Options

| Option | Description | Cost | Timeline |
|--------|-------------|------|----------|
| Microsoft CA Direct | Purchase from Microsoft | $1,000-$2,000/yr | 1-2 weeks |
| DigiCert via Microsoft | Enterprise agreement | $500-$1,500/yr | 3-5 days |
| Internal PKI | Corporate CA (not recommended for distribution) | Free | Immediate |

### Certificate Installation

```powershell
# Install certificate from PFX file
Import-PfxCertificate `
    -FilePath "D:\GitHub\APS\packaging\signing\cert.pfx" `
    -CertStoreLocation Cert:\LocalMachine\My `
    -Password (ConvertTo-SecureString "password" -AsPlainText -Force)

# Verify installation
Get-ChildItem Cert:\LocalMachine\My | Where-Object { $_.FriendlyName -like "*Code Signing*" }
```

---

## Usage Examples

### Sign Release Package

```powershell
cd D:\GitHub\APS\windows\packaging\signing

# Using certificate thumbprint (recommended)
.\sign.ps1 `
    -Path "D:\GitHub\APS\packages" `
    -CertThumbprint "A1B2C3D4E5F6789012345678901234567890ABCD"

# Using PFX file (for testing)
.\sign.ps1 `
    -Path "D:\GitHub\APS\packages" `
    -CertFile "cert.pfx" `
    -PfxPassword "password"
```

### Verify Signature

```powershell
cd D:\GitHub\APS\windows\packaging\signing

.\verify-signature.ps1 `
    -Path "D:\GitHub\APS\packages\ash-windows-sdk-x64-release.zip"
```

---

## Security Considerations

### Certificate Best Practices

1. **Never share private keys** — Store PFX in secure location (Azure Key Vault, HashiCorp Vault)
2. **Use strong passwords** — Minimum 16 characters with mixed complexity
3. **Rotate certificates annually** — Plan for renewal before expiration
4. **Monitor expiration** — Set alerts for certificates expiring within 30 days

### Signing Workflow Security

```powershell
# Secure signing workflow
$securePassword = Read-Host -AsSecureString -Prompt "Enter PFX password"
ConvertTo-SecureString $securePassword -AsPlainText -Force | 
    Set-Variable -Name PfxPassword

Import-PfxCertificate `
    -FilePath "$env:CODE_SIGNING_PFX" `
    -Password $PfxPassword `
    -CertStoreLocation Cert:\LocalMachine\My

# Sign artifacts
& .\sign.ps1 -Path "$buildDir\bin\$Platform-$Configuration"
```

---

## Testing Without Production Certificate

For development and testing, use **test certificates**:

### Self-Signed Test Certificate

```powershell
# Create self-signed test certificate (valid for 365 days)
New-SelfSignedCertificate `
    -CertStoreLocation "Cert:\LocalMachine\My" `
    -Subject "CN=ASH Pattern System Windows, OU=Test, O=YourCompany" `
    -KeyUsage DigitalSignature, KeyEncipherment `
    -TextPolicy DnsName `
    -DnsName "localhost" `
    -NotAfter (Get-Date).AddDays(365)

# Export for testing
Export-PfxCertificate `
    -Cert (Get-ChildItem Cert:\LocalMachine\My | Where-Object { $_.Subject -like "*ASH Pattern*" }) `
    -FilePath "D:\GitHub\APS\packaging\signing\test-cert.pfx" `
    -Password (ConvertTo-SecureString "Test123!" -AsPlainText -Force)
```

### Test Certificate Warning

> ⚠️ **WARNING:** Self-signed certificates are NOT suitable for production distribution. They will trigger security warnings in Windows and may be blocked by antivirus software. Use only for development and testing.

---

## Next Steps

1. ✅ **Scripts complete** — Ready for certificate configuration
2. ⏳ **Obtain production certificate** — Contact Microsoft or DigiCert
3. ✅ **Configure certificate** — Import to machine or Azure Key Vault
4. ✅ **Test signing workflow** — Use test certificate first
5. ✅ **Update build scripts** — Enable automatic signing in CI/CD

---

## Evidence Files

- `signing/sign.ps1` — Signing automation script
- `signing/verify-signature.ps1` — Signature verification utility
- `signing/cert-config.json` — Certificate configuration template
- `signing/test-cert.pfx` — Test certificate (for development only)

---

**Status:** `READY_FOR_CERTIFICATE_CONFIGURATION`  
**Last Updated:** 2026-07-13
