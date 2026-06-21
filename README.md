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

The current acceptance judgment is `NOT_SHIPPABLE`. See [windows/conformance/acceptance-judgment.md](windows/conformance/acceptance-judgment.md).

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
├── windows/
│   ├── README.md
│   ├── CANONICAL-DEFERENCE.md
│   ├── build/                     # Visual Studio solution, projects, and PowerShell wrapper
│   ├── src/                       # C++20 public headers and module implementations
│   ├── tests/                     # C++ test harness and module tests
│   ├── conformance/               # Windows conformance and release-gate documentation
│   └── docs/                      # Windows architecture and build/run documentation
└── completion-evidence/           # Current Windows completion and release-readiness evidence
```

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

## Release Status

The semantic core is implemented, but the Windows product is not release-ready until the remaining gates are complete:

- Visual Studio/MSBuild release matrix on supported Windows hosts;
- static analysis where configured;
- signed packaging or another owner-approved Microsoft-supported distribution artifact;
- clean install, launch, upgrade, repair, and uninstall validation;
- UI and accessibility validation after a product UI exists;
- SBOM, checksum, signing, and release-manifest evidence;
- final owner-controlled release approval.

Release evidence is tracked under [completion-evidence/](completion-evidence/).
