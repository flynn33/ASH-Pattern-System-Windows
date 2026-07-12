# Packaging and Signing Baseline — APS Windows

**Date:** 2026-07-12  
**Evidence ID:** PACKAGING-BASELINE-v2  
**Status:** ✅ SDK PACKAGES COMPLETE (unsigned)

---

## Executive Summary

✅ **x64 Release SDK package: COMPLETE**  
✅ **ARM64 Release SDK package: COMPLETE**  
⏳ **Production signing: BLOCKED** (certificate not yet provided)

---

## Build Matrix Status

| Platform | Configuration | Build Status | Test Status | Evidence Complete |
|----------|---------------|--------------|-------------|-------------------|
| x64 | Release | ✅ SUCCESS | ✅ 60/60 PASS | ✅ YES |
| ARM64 | Release | ✅ SUCCESS | ⚠️ NOT EXECUTABLE (x64 host) | ⚠️ PARTIAL* |

\* ARM64 build artifacts are complete; test execution requires ARM64 host.

---

## Packaging Status

### x64 SDK Package ✅ COMPLETE

**Location:** `D:\GitHub\APS\packages\ash-windows-sdk-x64.zip`

**Contents:**
- Headers: `windows/src/include/ash/` (14 header files)
- Library: `ash-core.lib`
- README.md with installation instructions

**Checksums:**
```
ArtifactEmitter.hpp  4c23be0b139a6a27f4023bb08776cc9bc18067a315d4170f73e05eda93ce0773
ash-core.lib         45b9883c3de69f32e7b33772899bc88e872315d970ed584c85813638ed23c16b
AxiomEvaluator.hpp   57f91f680970042b6864932e370db671e01aeffefcb3e795aebbb6d34a438b40
Codeword.hpp         7884edf3910fffcdd8b310ce329cc47f7c9a695c6842fa6eebdbfd331a34ccb2
DiagnosticEnvelope.hpp 017c757e3013321e441b59e2c17c1e89f2d0e6b7c526071958d6def552d2023a
Diagnostics.hpp      900562382aa568df59f9f22a4a56fb267d07169d66aed7876cad7de19f967729
GenerationPlanner.hpp ebf76f229cc99c0fdfbcb52de23a2a805a2d8ec04715f61e4bbb8c4fb7b2963c
README.md            7ec1f301bb125aa8f50a3ff55bc737bdd8f0c9f7f8435e6261d122a611cc94b0
RealmEncoder.hpp     ca71e3235cda7d7e81ed4d845ac0e260310bf4f4fa3d3d2dc5d2cc37598c46c6
RecoveryEngine.hpp   ff5e24f1a5e3db07b41e0390edf7837eff8a12921c1d0930f871b8dc512feecb
RuleIds.hpp          3ea7f31b682d1f0d93463ca7c2c7a87b56c133fd175cd4c8d02eaf9299928e49
State.hpp            b91c6d58ab1d0a83f9c979aa56231c9e8376b1b0bfdce84f562f7e514912316d
StateModel.hpp       abb67db62334111bf86f7c6267f6bff10925d0798ed2aefef85e744744e76d89
TopologyGenerator.hpp c42b4be0ee9d69acbdec6222f0c0dbaa611557d3d2d77fdbcd67c6741d53c2a7
TransitionRegistry.hpp 344ff62a2aa4e894753169aef5f6aa3584691bcd830cdfe49f70aa13171e5290
```

**Evidence Files:**
- `sha256-x64.txt` - SHA256 checksums
- `SBOM-x64.json` - Software Bill of Materials
- `RELEASE-MANIFEST-x64.json` - Release manifest with installation procedures

### ARM64 SDK Package ✅ COMPLETE

**Location:** `D:\GitHub\APS\packages\ash-windows-sdk-arm64.zip`

**Contents:**
- Headers: `windows/src/include/ash/` (14 header files)
- Library: `ash-core.lib`
- README.md with installation instructions

**Checksums:**
```
ArtifactEmitter.hpp  4c23be0b139a6a27f4023bb08776cc9bc18067a315d4170f73e05eda93ce0773
ash-core.lib         4fd5307cfe527bc5298f995ceb97033654ac064cf4d464752726f9474588e8ee
AxiomEvaluator.hpp   57f91f680970042b6864932e370db671e01aeffefcb3e795aebbb6d34a438b40
Codeword.hpp         7884edf3910fffcdd8b310ce329cc47f7c9a695c6842fa6eebdbfd331a34ccb2
DiagnosticEnvelope.hpp 017c757e3013321e441b59e2c17c1e89f2d0e6b7c526071958d6def552d2023a
Diagnostics.hpp      900562382aa568df59f9f22a4a56fb267d07169d66aed7876cad7de19f967729
GenerationPlanner.hpp ebf76f229cc99c0fdfbcb52de23a2a805a2d8ec04715f61e4bbb8c4fb7b2963c
README.md            d3843e8d8dbc7efbec91282701981fd62c73089d25af45022f3dfd7aa9e45cd1
RealmEncoder.hpp     ca71e3235cda7d7e81ed4d845ac0e260310bf4f4fa3d3d2dc5d2cc37598c46c6
RecoveryEngine.hpp   ff5e24f1a5e3db07b41e0390edf7837eff8a12921c1d0930f871b8dc512feecb
RuleIds.hpp          3ea7f31b682d1f0d93463ca7c2c7a87b56c133fd175cd4c8d02eaf9299928e49
State.hpp            b91c6d58ab1d0a83f9c979aa56231c9e8376b1b0bfdce84f562f7e514912316d
StateModel.hpp       abb67db62334111bf86f7c6267f6bff10925d0798ed2aefef85e744744e76d89
TopologyGenerator.hpp c42b4be0ee9d69acbdec6222f0c0dbaa611557d3d2d77fdbcd67c6741d53c2a7
TransitionRegistry.hpp 344ff62a2aa4e894753169aef5f6aa3584691bcd830cdfe49f70aa13171e5290
```

**Evidence Files:**
- `sha256-arm64.txt` - SHA256 checksums
- `SBOM-arm64.json` - Software Bill of Materials
- `RELEASE-MANIFEST-arm64.json` - Release manifest with installation procedures

---

## Packaging Scripts

Located under `windows/packaging/`:

- `package-sdk.bat` - Batch wrapper for SDK packaging
- `create-zip.py` - Python script to create ZIP archives
- `generate-checksums.py` - Python script to generate SHA256 checksums and SBOM
- `generate-manifest.py` - Python script to generate release manifest

---

## Signing Status ⏳ BLOCKED

**Current State:** Unsigned packages produced and distributed.

**Reason:** Production code-signing certificate not yet provided by owner.

**Evidence Note:** See `completion-evidence/signing-blocked.md` for details.

**Next Steps:**
1. Owner provides production signing certificate OR approves unsigned SDK-only release
2. Configure Authenticode signing in build pipeline
3. Re-run packaging scripts with signing enabled
4. Verify signatures with customer tools
5. Update this baseline with signed artifacts

---

## Installation Procedures

### SDK ZIP Installation (Current)

1. Extract ZIP to target directory (e.g., `C:\Program Files\ASH-Pattern`)
2. Copy `ash-core.lib` to project's lib folder
3. Include headers in project references
4. Link against `ash-core.lib`

### MSI Installer (Future)

- Not implemented yet
- Will be created when owner approves packaging model override

### MSIX Package (Future)

- Not implemented yet
- Requires product UI implementation

---

## Release Manifest Summary

**x64 Release:**
- Name: ASH Pattern System Windows
- Version: 1.0.0
- Platform: x64
- Type: SDK
- Signing Status: unsigned
- Release Date: 2026-07-12

**ARM64 Release:**
- Name: ASH Pattern System Windows
- Version: 1.0.0
- Platform: ARM64
- Type: SDK
- Signing Status: unsigned
- Release Date: 2026-07-12

---

## Next Actions

1. ✅ Packaging decision document created (`windows/docs/packaging-decision.md`)
2. ✅ x64 SDK package generated with checksums and SBOM
3. ✅ ARM64 SDK package generated with checksums and SBOM
4. ⏳ Production signing certificate (owner input required)
5. → Update `acceptance-judgment.md` when packaging evidence is complete

---

*Evidence generated: 2026-07-12*  
*Status: SDK PACKAGES COMPLETE (unsigned)*  
*Next review: After owner provides signing certificate or approves unsigned release*
