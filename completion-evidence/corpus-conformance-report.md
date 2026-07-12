# Canonical Corpus Conformance Report — APS Windows

**Date:** 2026-07-12  
**Evidence ID:** CORPUS-CONFORMANCE-v1  
**Status:** ✅ CANONICAL DATA VENDORED, VERIFICATION FRAMEWORK READY

---

## Executive Summary

The canonical ASH Pattern System corpus has been successfully acquired from the upstream specification repository and vendored into the Windows implementation branch. A verification framework has been implemented to validate the corpus contents.

### Gap Resolution Status

| Blocker ID | Capability | Status | Evidence |
|------------|------------|--------|----------|
| APSWIN-GAP-001 | Canonical upstream JSON corpus vendored | ✅ RESOLVED | `windows/canonical-data/` contains 8 canonical JSON files |
| APSWIN-GAP-002 | Corpus verification framework implemented | ✅ READY FOR EXECUTION | `windows/packaging/run-canonical-conformance.py` validated all files |

---

## Canonical Corpus Acquisition

### Source Repository

- **Repository:** `flynn33/ASH-Pattern-System`
- **Commit:** `cc253f3d137a27f0eeb471bed62bbdb939e3b6d1`
- **Description:** Canonical agnostic specification repository for the ASH Pattern System

### Acquisition Method

**Script:** `windows/packaging/fetch-canonical-data.py`

The Python script fetches all files from the `canonical-data/` directory of the upstream repository using the GitHub API.

**Command Used:**
```bash
python windows/packaging/fetch-canonical-data.py flynn33/ASH-Pattern-System cc253f3d137a27f0eeb471bed62bbdb939e3b6d1
```

### Vendored Files

**Location:** `D:\GitHub\APS\windows\canonical-data\canonical-data\1.0\`

| File | Description | Size (approx) |
|------|-------------|---------------|
| `codewords.json` | 16-member codeword set definition | ~2 KB |
| `realms.json` | 512 realm identities with b0 MSB ordering | ~4 KB |
| `orbits.json` | 32 orbit partitioning for [9,4,4] doubly-even code | ~3 KB |
| `transitions.json` | All 8,192 state/codeword transition pairs | ~50 KB |
| `fallback-policy-definitions.json` | Fallback policy instance model | ~2 KB |
| `rule-registry.json` | Diagnostic rule family registry | ~3 KB |
| `normative-artifact-index.json` | Normative artifact index | ~2 KB |
| `mathematical-properties.json` | Mathematical properties of codeword set | ~1 KB |

**Total Files:** 8  
**Total Size:** ~70 KB (estimated)

---

## Conformance Verification Framework

### Purpose

The verification framework validates that the vendored canonical corpus files:
1. Exist in the expected location
2. Contain valid JSON data
3. Match the expected structure for each file type

### Implementation

**Location:** `D:\GitHub\APS\windows\packaging\run-canonical-conformance.py`

**Usage:**
```bash
python windows/packaging/run-canonical-conformance.py
```

### Verification Results

All 8 canonical corpus files have been validated:

| File | Status | Notes |
|------|--------|-------|
| codewords.json | ✅ PASS | Valid JSON loaded |
| realms.json | ✅ PASS | Valid JSON loaded |
| orbits.json | ✅ PASS | Valid JSON loaded |
| transitions.json | ✅ PASS | Valid JSON loaded |
| fallback-policy-definitions.json | ✅ PASS | Valid JSON loaded |
| rule-registry.json | ✅ PASS | Valid JSON loaded |
| normative-artifact-index.json | ✅ PASS | Valid JSON loaded |
| mathematical-properties.json | ✅ PASS | Valid JSON loaded |

---

## Integration with Windows Implementation

### Next Steps for Full Conformance Testing

To execute full conformance testing:

1. **Wire corpus to build/test commands:**
   - Add corpus verification step to `windows/build/build.ps1`
   - Run after Release build completes
   - Execute on both x64 and ARM64 platforms

2. **Execute corpus verification:**
   ```bash
   python windows/packaging/run-canonical-conformance.py
   ```

3. **Document results:**
   - Update this report with execution timestamps
   - Record any discrepancies between canonical and implementation
   - Note platform-specific behaviors

### Protected Surface Considerations

Per the branch divergence report (APSWIN-DIV-002):
- Do not merge upstream wholesale into Windows branch
- Preserve Windows implementation tree under `windows/`
- Keep canonical corpus as reference data, not replacement
- Governance surfaces remain protected

---

## Deviation Log Updates Required

The following deviation entries should be updated in `windows/conformance/deviation-log.md`:

### D-003 — Operational Context Model Is Incomplete

**Current Status:** DESIGN COMPLETE  
**Update:** Add note that canonical corpus is now vendored and available for reference.

### New Deviation Entry (Optional)

Consider adding:
```
## D-XXX — Canonical Corpus Execution Pending

**What:** Full corpus verification against Windows implementation not yet executed.

**Why:** Canonical data vendored but not wired to build/test commands; execution requires Release builds.

**Scope:** `windows/canonical-data/`, `windows/packaging/run-canonical-conformance.py`

**Resolution:** Execute corpus verification on Release x64/ARM64 builds; document results in this report.
```

---

## Evidence Files

- `windows/canonical-data/` - Vendored canonical corpus
- `windows/packaging/fetch-canonical-data.py` - Acquisition script
- `windows/packaging/run-canonical-conformance.py` - Verification framework
- `completion-evidence/canonical-conformance-gap-analysis.md` - Gap analysis (updated)
- `completion-evidence/corpus-conformance-report.md` - This report

---

*Evidence generated: 2026-07-12*  
*Status: Canonical corpus vendored, verification framework ready*  
*Next review: After full corpus execution on Release builds*
