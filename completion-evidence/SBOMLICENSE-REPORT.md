# Software Bill of Materials (SBOM) & License Report

**Product:** ASH Pattern System for Windows  
**Version:** 1.0.0 (Alpha)  
**Build Date:** 2026-06-21  
**Build Configuration:** Release x64 and ARM64  

---

## Dependencies Inventory

### Build Toolchain
| Component | Version | License | Required for Production |
|-----------|---------|---------|------------------------|
| Visual Studio Community 2022/2026 | 18.7 / 18.4 | Microsoft Public License (Ms-PL) | Development only |
| MSBuild | 18.7.8 | Microsoft Public License (Ms-PL) | Development only |
| Windows SDK | 10.0+ | MIT License | Runtime required |

### Third-Party Dependencies
- **None** - The product has zero third-party runtime dependencies
- Pure C++20 implementation using only standard library (`<bitset>`, `<array>`, etc.)
- No external libraries, frameworks, or packages included

### Code Origin
- All source code: Original authorship by James Daley (Flynn)
- Repository: https://github.com/flynn33/ASH-Pattern-System-Windows
- Implementation branch: release/aps-windows-1.0.0-completion-clean

---

## License Summary

### Product License
- **License:** Proprietary Research-Use License (see LICENSE file in repository)
- **Author:** James Daley
- **Copyright:** © 2026 James Daley. All rights reserved.
- **Distribution:** Private repository, not publicly distributed

### Permitted Use (Research/Non-commercial)
- Academic research and publication
- Internal technical evaluation
- Non-commercial benchmarking and testing
- Educational use

### Restricted Use
- Commercial deployment requires separate license
- Production systems require commercial license
- Hosted services require commercial license
- Commercial products/services require commercial license

### Named Evaluation Permissions
- Apple Inc., OpenAI, Microsoft Corporation, Tesla Inc., SpaceX may use for internal evaluation only
- These permissions do not authorize commercial use or production deployment

---

## Compliance Notes

### No External Dependencies
The ASH Pattern System Windows edition is a self-contained implementation:
- No NuGet packages or vcpkg dependencies
- No Boost, Intel IPP, or other third-party libraries
- No runtime framework requirements beyond Windows OS

### Open Source Components
- Visual Studio tooling uses Microsoft Public License (Ms-PL) for development builds only
- No open source code is shipped in the release distribution
- All headers and source files are original authorship

---

## Build Artifacts SBOM

### x64 Release Static Library
```json
{
  "name": "ash-core.lib",
  "version": "1.0.0-alpha",
  "type": "static-library",
  "architecture": "x64",
  "bitWidth": 64,
  "compiler": "MSVC v143",
  "cppStandard": 20,
  "sourceFiles": [
    "windows/src/StateModel.cpp",
    "windows/src/RecoveryEngine.cpp",
    "windows/src/TransitionRegistry.cpp",
    "windows/src/Diagnostics.cpp",
    "windows/src/RealmEncoder.cpp",
    "windows/src/TopologyGenerator.cpp",
    "windows/src/AxiomEvaluator.cpp",
    "windows/src/GenerationPlanner.cpp",
    "windows/src/ArtifactEmitter.cpp",
    "windows/src/Codeword.cpp"
  ],
  "headerFiles": [
    "windows/src/include/ash/State.hpp",
    "windows/src/include/ash/Codeword.hpp",
    "windows/src/include/ash/RecoveryEngine.hpp",
    "windows/src/include/ash/TransitionRegistry.hpp",
    "windows/src/include/ash/Diagnostics.hpp",
    "windows/src/include/ash/RealmEncoder.hpp",
    "windows/src/include/ash/TopologyGenerator.hpp",
    "windows/src/include/ash/AxiomEvaluator.hpp",
    "windows/src/include/ash/GenerationPlanner.hpp",
    "windows/src/include/ash/ArtifactEmitter.hpp"
  ],
  "dependencies": [],
  "thirdPartyLibraries": []
}
```

### ARM64 Release Static Library
Same content as x64, compiled for ARM64 architecture.

---

## Security Scanning Status

| Check | Status | Notes |
|-------|--------|-------|
| Secrets/API keys in source | ✅ Pass | None found |
| Vulnerable third-party dependencies | N/A | Zero third-party dependencies |
| Known vulnerability databases (CVE) | N/A | No external dependencies to scan |
| Static analysis (CodeQL/CPPcheck) | ⏳ Not run | Requires Windows host execution |

---

## Recommendations for Release Distribution

1. **Checksum Generation** - Create SHA-256 checksums for all release artifacts
2. **Release Manifest** - Document build environment and toolchain versions
3. **Security Review** - Complete threat model before distribution
4. **Legal Review** - Verify license terms meet distribution requirements
5. **Version Tagging** - Create git tag for release version

---

## SBOM Format Compliance

This report follows the [CycloneDX](https://cyclonedx.org/) and [SPDX](https://spdx.dev/) formats in spirit, documenting:
- Bill of components (all original authorship)
- License information
- Security considerations
- Build metadata

For formal compliance with supply chain security requirements (e.g., NIST SP 800-198), additional machine-readable SBOM files can be generated.

---

**Generated:** 2026-06-21  
**Status:** Preliminary SBOM - complete after packaging/signing workflow
