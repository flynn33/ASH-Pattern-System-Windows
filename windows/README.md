# ASH Pattern System - Windows Edition

This directory contains the Windows-native implementation of the ASH Pattern System semantic core. It is written in C++20, built with Visual Studio/MSBuild, and verified through native C++ test executables.

## Platform Role

The Windows edition implements APS behavior for a Windows desktop target. Platform-specific decisions such as compiler flags, CRT linkage, solution layout, PowerShell wrappers, and release packaging evidence are documented here and under `windows/conformance/`.

See [`CANONICAL-DEFERENCE.md`](CANONICAL-DEFERENCE.md) for the semantic boundary between APS behavior and Windows-specific implementation decisions.

## Target Environment

- **Operating systems:** Windows 10 version 1809 or later and Windows 11.
- **Architectures:** x64 and ARM64.
- **Language:** C++20.
- **Compiler:** MSVC `cl.exe`.
- **Build system:** MSBuild (`.sln` and `.vcxproj`).
- **Script host:** PowerShell 5.1 or PowerShell 7.
- **Runtime dependency policy:** no third-party runtime dependencies.

## Windows Documentation Map

| Area | Document |
|---|---|
| Target environment and architecture | `windows/docs/architecture.md` |
| Semantic module mapping | `windows/conformance/module-mapping.md` |
| Verification plan | `windows/conformance/verification-plan.md` |
| Diagnostics integration | `windows/conformance/diagnostics-conformance-plan.md` |
| Materialization boundary | `windows/conformance/materialization-boundary-plan.md` |
| Build and test instructions | `windows/docs/build-run-instructions.md` |
| Caveat and deviation tracking | `windows/conformance/deviation-log.md` |
| Acceptance status | `windows/conformance/acceptance-judgment.md` |

## Directory Layout

```text
windows/
├── README.md
├── CANONICAL-DEFERENCE.md
├── build/                       # Visual Studio solution, project files, PowerShell wrapper
├── src/
│   ├── include/ash/             # public C++ headers
│   └── *.cpp                    # module implementations
├── tests/                       # test_runner.hpp and module tests
├── conformance/                 # Windows conformance and release-gate documentation
└── docs/                        # architecture and build/run instructions
```

## Semantic Core Coverage

The Windows semantic core implements:

- `StateModel`
- `TransitionRegistry`
- `Diagnostics`
- `RecoveryEngine`
- `RealmEncoder`
- `TopologyGenerator`
- `AxiomEvaluator`
- `GenerationPlanner`
- `ArtifactEmitter`

See `windows/conformance/module-mapping.md` for complete file mapping and `windows/conformance/verification-plan.md` for test coverage.

## Current Acceptance Status

`NOT_SHIPPABLE`

The semantic core exists, but release-critical Windows gates remain:

- MSVC/MSBuild release matrix;
- packaging model and signed distribution evidence;
- production signing verification;
- clean install, launch, upgrade, repair, and uninstall validation;
- product UI and accessibility validation;
- SBOM, checksums, release manifest, and final owner-controlled approvals.

## Build And Run

```powershell
cd windows\build
.\build.ps1
..\tests\run_all.ps1
```

Prerequisites: Visual Studio 2022 or Build Tools for Visual Studio 2022 with the C++ workload, plus PowerShell 5.1 or PowerShell 7.
