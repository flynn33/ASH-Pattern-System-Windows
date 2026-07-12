# Packaging Decision — APS Windows

**Date:** 2026-07-12  
**Branch:** main  
**Decision ID:** PACKAGING-DECISION-v1  
**Status:** ✅ ADOPTED (Owner-approved default)

---

## Executive Summary

This document records the **hybrid packaging model** for APS Windows, approved as the default approach unless owner provides written override.

### Core Principle

> **SDK distribution now; MSIX later when product UI exists.**

Do not thrash between MSIX and classic installer as if the product were already a UI app. The current release is a **native C++ semantic core library**, not an end-user application with UI.

---

## Packaging Model: Hybrid Approach

### Phase 1: Near-term (Ship the Core) — SDK Distribution ✅ CURRENT

**Deliverable:** Multi-arch Release artifacts for SDK consumers

**Contents:**
- Headers (`windows/src/include/ash/`)
- Static library: `ash_core.lib` (x64 and ARM64)
- PDBs per policy (debug symbols)
- Layout ZIP archive
- Optional WiX SDK MSI (later, if requested)
- Checksums (SHA256)
- SBOM (Software Bill of Materials)
- Release manifest (JSON)

**Distribution:**
- Unsigned packages until production signing certificate is available
- `signing-blocked.md` evidence note when unsigned
- Clean install/upgrade/repair/uninstall procedure documented

**Rationale:**
- Core library can be released immediately as static library
- Customers can integrate into their own packaging workflow
- No need for MSIX while there is no product UI
- Reduces complexity and dependency on Windows App SDK

### Phase 2: Later (End-user Product App with UI) — Signed MSIX

**Trigger:** When product UI (WinUI 3) is implemented and owner reopens this scope.

**Deliverable:** Signed MSIX package

**Contents:**
- Full product application bundle
- WinUI 3 UI components
- Accessibility compliance artifacts
- Authenticode-signed MSIX

**Rationale:**
- MSIX appropriate for full product with UI
- Enterprise deployment via MSI/Setup only if required
- Signing credentials required for production release

---

## Signing Strategy

### Current State: Unsigned Packages ✅ OK

Until a production code-signing certificate is provided by the owner:

1. Produce **unsigned** SDK packages
2. Include `signing-blocked.md` evidence note
3. Document clean install/upgrade/repair/uninstall procedures
4. Mark installer matrix as blocked on signing (not fiction)

### Production Signing (Future)

When certificate is available:

1. Configure Authenticode signing in build pipeline
2. Sign SDK MSI/MSIX packages
3. Verify signatures with customer tools
4. Update `packaging-and-signing-baseline.md` with hashes and evidence

---

## Packaging Work Order

### Step 1: Decision Document ✅ COMPLETE

This document captures the hybrid model + rationale.

### Step 2: Implement Packaging Scripts

Create PowerShell scripts under `windows/packaging/`:

- Consume Release x64/ARM64 build outputs
- Stage SDK layout (headers + lib)
- Create ZIP archive
- Generate SHA256 checksums
- Generate SBOM
- Create release manifest JSON
- Optional: MSIX project skeleton (only if product app entrypoint exists)

### Step 3: Document Install Procedures

Document clean install/upgrade/repair/uninstall for chosen artifact type:

- SDK ZIP extraction and registration
- MSI installer procedures (if implemented later)
- MSIX installation (when UI exists)

Execute what is possible without cert; mark rest blocked on signing.

### Step 4: Update Evidence Baseline

Update `completion-evidence/packaging-and-signing-baseline.md` with:

- Commands and paths
- Hashes and checksums
- Gaps and blockers

---

## Clean Install/Upgrade/Repair/Uninstall Procedures

### SDK ZIP Installation

1. Extract ZIP to target directory (e.g., `C:\Program Files\ASH-Pattern`)
2. Copy `ash_core.lib` to project include paths
3. Include headers in project references
4. Link against `ash_core.lib`

### MSI Installer (Future)

1. Run Setup.exe with /quiet for silent install
2. Upgrade existing installation with same version
3. Repair: Setup.exe /repair
4. Uninstall: Control Panel → Programs → Remove

### MSIX Package (Future, when UI exists)

1. Install via Store or MSI wrapper
2. Upgrade via Store update channel
3. Repair: Not applicable (MSIX self-healing)
4. Uninstall: Settings → Apps → Remove

---

## Rationale Summary

| Decision | Reason |
|----------|--------|
| SDK now, MSIX later | Core library doesn't need UI packaging yet |
| Unsigned packages OK | Cannot invent secrets; document gap instead |
| Hybrid model | Flexibility for core release and future product |
| No installer fiction | Don't pretend MSIX is shipping without entrypoint |

---

## Owner Approval Required

The following items require owner input:

1. ✅ **Packaging model decision** — Adopted as hybrid (SDK now / MSIX later)
2. ⏳ **Production signing certificate** — Blocked until provided
3. ⏳ **Canonical corpus source** — Needed for conformance testing
4. ⏳ **Final release approval** — External owner gate

---

## Next Actions

1. ✅ Decision document created (this file)
2. → Implement packaging scripts (`windows/packaging/`)
3. → Generate x64/ARM64 SDK packages (unsigned OK)
4. → Record install validation results or blocked-on-signing notes
5. → Update `packaging-and-signing-baseline.md`

---

*Decision approved: 2026-07-12*  
*Status: ADOPTED (owner-approved default)*  
*Next review: After packaging scripts implemented*
