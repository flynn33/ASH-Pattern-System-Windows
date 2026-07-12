# Signing Blocked — APS Windows

**Date:** 2026-07-12  
**Evidence ID:** SIGNING-BLOCKED-v1  
**Status:** ⏳ BLOCKED ON OWNER INPUT

---

## Current State

Production **code-signing certificate** is not yet available. Until a production Authenticode signing certificate is provided by the owner:

- ✅ Unsigned SDK packages are produced and distributed
- ✅ Checksums and SBOM generated for unsigned artifacts
- ⏳ Signed installers cannot be produced (blocked)
- ⏳ Clean install validation on Windows Store blocked (requires signing)

---

## What's Available Now

### Unsigned SDK Packages ✅ OK

The following can be produced and distributed without signing:

- **SDK ZIP archives** (x64 and ARM64)
  - Headers (`windows/src/include/ash/`)
  - Static library (`ash_core.lib`)
  - PDBs (debug symbols, if available)
  - README with installation instructions
  - SHA256 checksums
  - SBOM (Software Bill of Materials)
  - Release manifest JSON

### Build Artifacts ✅ OK

- x64 Release: `D:\GitHub\APS\windows\build\bin\x64-Release\ash_core.lib`
- ARM64 Release: `D:\GitHub\APS\windows\build\bin\ARM64-Release\ash_core.lib`
- All test executables built and verified (x64)

---

## What's Blocked ⏳

### Production Signing

Until owner provides a production code-signing certificate:

- ❌ Signed MSIX packages (requires Authenticode signing)
- ❌ Signed MSI installers (requires Authenticode signing)
- ❌ Windows Store submission (requires valid certificate)
- ❌ Signature verification by enterprise customers

### Install Validation

- ❌ Clean install validation on Windows target (requires signed installer)
- ❌ Upgrade path validation (requires signed installer)
- ❌ Repair functionality validation (requires signed installer)
- ❌ Uninstall validation (requires signed installer)

---

## Owner Input Required

To unblock signing:

1. **Provide production code-signing certificate** (or approve unsigned SDK-only release)
2. **Configure signing identity** in build pipeline
3. **Approve packaging model** (hybrid SDK now / MSIX later)

---

## Next Steps

1. ⏳ Owner provides signing certificate OR approves unsigned SDK release
2. → Configure Authenticode signing in build pipeline
3. → Re-run packaging scripts with signing enabled
4. → Verify signatures with customer tools
5. → Update `packaging-and-signing-baseline.md` with signed artifacts

---

*Evidence generated: 2026-07-12*  
*Status: BLOCKED ON OWNER INPUT (signing certificate)*
