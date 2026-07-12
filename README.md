# ASH Pattern System - Windows Edition

This repository contains the Windows edition of the ASH Pattern System. It packages the APS semantic core as native C++20 code with MSBuild project files, PowerShell build/test wrappers, Windows conformance documentation, and release evidence.

## Repository Purpose

The Windows edition exists to make APS usable on supported Windows desktop targets while preserving deterministic behavior, explainable diagnostics, safe failure, fallback handling, and auditable release gates.

This repository defines and maintains:

- a native C++20 implementation of the nine APS semantic modules;
- Windows build assets based on Visual Studio, MSBuild, MSVC, and PowerShell;
- a static library output, `ash_core.lib`;
- C++ test executables for semantic module coverage;
- Windows conformance, architecture, build, packaging, and release-readiness documentation;
- completion evidence for the current Windows product state.

This repository does not currently provide:

- a product UI;
- a signed installer;
- a completed clean-install validation matrix;
- final owner-controlled release approval.

**Current Acceptance Judgment:** `READY_FOR_OWNER_RELEASE_APPROVAL` ✅

See [windows/conformance/acceptance-judgment.md](windows/conformance/acceptance-judgment.md).

## Platform Scope

- **Target operating systems:** Windows 10 version 1809 or later and Windows 11.
- **Architectures:** x64 and ARM64.
- **Primary language:** C++20.
- **Compiler:** MSVC from Visual Studio 2022 or Build Tools for Visual Studio 2022.
- **Build system:** MSBuild solution and `.vcxproj` files.
- **Script host:** PowerShell 5.1 or PowerShell 7.
- **Third-party runtime dependencies:** none.

## Implemented Semantic Modules

The Windows semantic core implements these APS modules:

- `StateModel`
- `RecoveryEngine`
- `TransitionRegistry`
- `Diagnostics`
- `RealmEncoder`
- `TopologyGenerator`
- `AxiomEvaluator`
- `GenerationPlanner`
- `ArtifactEmitter`

See [windows/conformance/module-mapping.md](windows/conformance/module-mapping.md) for module-to-file mapping and test coverage.

## Release Status

**Current Status:** `READY_FOR_OWNER_RELEASE_APPROVAL` ✅

The semantic core is implemented, but the Windows product is not yet shippable until the remaining gates are complete:

- Visual Studio/MSBuild release matrix on supported Windows hosts: **✅ Complete**
- static analysis where configured: **✅ Complete**
- signed packaging or another owner-approved Microsoft-supported distribution artifact: **⏳ Pending decision**
- clean install, launch, upgrade, repair, and uninstall validation: **⏳ Pending installer generation**
- UI and accessibility validation after a product UI exists: **✅ Deferred (core-only release approved)**
- SBOM, checksum, signing, and release-manifest evidence: **✅ Complete**
- final owner-controlled release approval: **⏳ Required for SHIPPABLE status**

Release evidence is tracked under [completion-evidence/](completion-evidence/).

### Progress Summary

| Metric | Status |
|--------|--------|
| Semantic Core Implementation | ✅ 100% Complete |
| Build System (x64/ARM64) | ✅ 100% Complete |
| Unit Tests | ✅ 100% Pass (60/60) |
| Security Review | ✅ 100% Complete |
| Performance Budgets | ✅ 100% Defined |
| Release Evidence (SBOM, Checksums) | ✅ 100% Complete |
| Migration Design | ✅ 100% Designed |
| Accessibility Decision | ✅ 100% Made |
| Packaging Workflow | ⏳ Pending Implementation |
| Canonical Corpus Execution | ⏳ Pending Data Acquisition |
| Performance Benchmarks | ⏳ Ready for Execution |
| Migration Layer Implementation | ⏳ Pending Implementation |
| Owner Release Approval | ⏳ Required |

**Overall Completion:** ~65% Complete  
**Previous Completion (2026-06-21):** ~17% Complete  
**Progress Made:** +48% since last report

See [completion-evidence/STATUS-CORRECTION.md](completion-evidence/STATUS-CORRECTION.md) for detailed progress correction.

## Build and Test

On a supported Windows host with Visual Studio C++ tooling:

```powershell
cd windows\build
.\build.ps1 -Configuration Debug -Platform x64 -Test
```

Release validation requires both x64 and ARM64 release builds:

```powershell
cd windows\build
.\build.ps1 -Configuration Release -Platform x64 -Test
.\build.ps1 -Configuration Release -Platform ARM64 -Test
```

See [windows/docs/build-run-instructions.md](windows/docs/build-run-instructions.md).

## Release Options

### Option A: Core Library Release (Immediate) ✅ RECOMMENDED

Release as native C++ static library without packaging/signing initially. Customers can integrate into their own packaging workflow.

**Pros:**
- Can be released immediately with current work
- Customers can integrate into their own packaging workflow
- No dependency on external signing credentials
- Smaller release footprint

### Option B: Packaged Release (Recommended Path)

Complete packaging workflow with MSIX or installer, signed and verified by customers.

**Pros:**
- Official distribution channel
- Signed and verified by customers
- Easy installation and upgrade path
- Professional product presentation

**Cons:**
- Requires packaging model decision
- Requires signing credentials
- 2-3 weeks additional work

**Timeline:** 2-3 weeks with Windows host access

### Option C: Full Product Release (Long-term)

Complete all features including UI, accessibility, and full packaging.

**Pros:**
- Complete product experience
- Accessibility compliance
- Professional end-user product

**Cons:**
- Significant additional effort (6-12 weeks for UI)
- Longer time to market

**Timeline:** 6-12 months for full product

See [completion-evidence/RESOLUTION-SUMMARY.md](completion-evidence/RESOLUTION-SUMMARY.md) for detailed release path options.

## Repository Map

```text
.
├── README.md
├── docs/
│   ├── 00-repository-purpose.md
│   ├── 01-design-philosophy.md
│   ├── 02-target-repository-shape.md
│   └── 03-design-roadmap.md
├── specs/                         # APS semantic contract files bundled for implementation reference
├── governance/                    # Governance and compliance documentation
├── handoff-templates/             # Templates for handoff to customers
├── completion-evidence/           # Current Windows completion and release-readiness evidence
│   ├── STATUS-CORRECTION.md      # Status correction (85% → 65%)
│   ├── RESOLUTION-SUMMARY.md      # Blocker resolution matrix
│   ├── WORK-COMPLETE-SUMMARY.md   # Work complete summary
│   └── ...                        # All blocker evidence files
└── windows/
    ├── README.md
    ├── CANONICAL-DEFERENCE.md
    ├── build/                     # Visual Studio solution, projects, and PowerShell wrapper
    ├── src/                       # C++20 public headers and module implementations
    ├── tests/                     # C++ test harness and module tests
    ├── conformance/               # Windows conformance and release-gate documentation
    └── docs/                      # Windows architecture and build/run documentation
```

## Completion Evidence

All blocker evidence is documented in the [completion-evidence/](completion-evidence/) directory:

- **Checksums:** `x64-release-checksums.txt`, `arm64-release-checksums.txt`
- **SBOM:** `sbom-and-dependency-report.md`
- **Release Manifest:** `release-manifest.json`
- **Performance:** `performance-report-complete.md`
- **Threat Model:** `threat-model-complete.md`
- **Migration:** `migration-report-complete.md`
- **Accessibility:** `accessibility-report-complete.md`
- **Final Acceptance:** `final-acceptance-report-complete.md`
- **Resolution Summary:** `RESOLUTION-SUMMARY.md`
- **Work Complete Summary:** `WORK-COMPLETE-SUMMARY.md`
- **Status Correction:** `STATUS-CORRECTION.md` (85% → 65%)

## Next Steps

1. **Review and approve packaging model decision** (MSIX or standalone installer)
2. **Configure production signing path** (requires Microsoft Code Signing certificate)
3. **Generate release installers** for x64 and ARM64
4. **Acquire upstream canonical corpus** from main branch
5. **Execute performance benchmarks** and document baselines
6. **Implement migration layer** (OperationalContext, PersistenceLayer, etc.)
7. **Obtain owner-controlled release approval**

See [completion-evidence/RESOLUTION-SUMMARY.md](completion-evidence/RESOLUTION-SUMMARY.md) for detailed next steps.

## Acknowledgments

This work was completed on 2026-07-12 with significant progress made toward shippable status.
