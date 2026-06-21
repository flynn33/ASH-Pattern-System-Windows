# ASH Pattern System — Windows C++ Implementation Branch

> **Downstream implementation branch.** This branch (`windows-cpp`) is a Windows C++ implementation of the ASH Pattern System. The canonical agnostic main repository remains the semantic authority. This branch consumes the canonical specifications and implements them for a desktop Windows target class. It does not redefine, override, or extend canonical semantics.

## Role of this branch

The canonical main repository (closed as the agnostic specification baseline in commit `6310479`) is the source of truth for the ASH Pattern System. Main defines what the system means. This branch defines how one specific downstream target — Windows desktop, C++ — implements it. The two are not equal; main wins on every semantic question.

See [`CANONICAL-DEFERENCE.md`](CANONICAL-DEFERENCE.md) for the explicit boundary statement.

## Target class

This branch treats Windows as a **desktop target class** implementation of the ASH Pattern System, as defined in `handoff-templates/desktop-implementation-handoff-template.md` on main. The nine required sections of the desktop template are addressed across this branch:

| Desktop template section | Where it lives on this branch |
|---|---|
| 1. Target environment | `windows/docs/architecture.md` |
| 2. Semantic-module mapping | `windows/conformance/module-mapping.md` |
| 3. Invariant / conformance verification inputs | `windows/conformance/verification-plan.md` |
| 4. Diagnostics integration | `windows/conformance/diagnostics-conformance-plan.md` |
| 5. Materialization-boundary expectations | `windows/conformance/materialization-boundary-plan.md` |
| 6. Packaging / build / deployment decisions | `windows/docs/build-run-instructions.md` |
| 7. Performance / resource constraints | `windows/docs/architecture.md` (performance section) |
| 8. Caveat / deviation tracking | `windows/conformance/deviation-log.md` |
| 9. Proof-of-conformance deliverables | `windows/conformance/` (all six files) |

## Language and tooling policy

Per the instruction package governing this branch:

- **Primary language:** C++20, compiled with MSVC `cl.exe`.
- **Secondary languages (allowed, justified):** JSON for configuration or structured data where useful. TypeScript only where strictly needed and only as a secondary/supporting language. Neither is used in the current Windows implementation.
- **Build system:** MSBuild (`.sln` / `.vcxproj`) plus a PowerShell wrapper. All Microsoft-produced.
- **Third-party libraries:** none. C++ standard library only.
- **Testing:** a minimal single-header homegrown test harness in `windows/tests/test_runner.hpp`. Built as plain C++ executables with MSVC. No external test framework.

The full policy is: **Windows-native tools and libraries may be used only if they are produced and supported by Microsoft.**

## Directory layout

```
windows/
├── README.md                    # this file
├── CANONICAL-DEFERENCE.md       # explicit deference statement
├── build/                       # MSBuild solution, project files, PowerShell wrapper
├── src/
│   ├── include/ash/             # public headers (all 9 canonical modules + shared types)
│   └── *.cpp                    # module implementations for all 9 modules
├── tests/                       # test_runner.hpp + per-module tests
├── conformance/                 # 6 downstream conformance deliverables
└── docs/                        # architecture and build/run instructions
```

## Semantic core coverage

This branch implements the nine APS semantic modules in native C++:
`StateModel`, `TransitionRegistry`, `Diagnostics`, `RecoveryEngine`,
`RealmEncoder`, `TopologyGenerator`, `AxiomEvaluator`, `GenerationPlanner`,
and `ArtifactEmitter`.

See `windows/conformance/module-mapping.md` for the complete mapping and
`windows/conformance/verification-plan.md` for the current test coverage.

## Current acceptance status

**`NOT_SHIPPABLE`** — see `windows/conformance/acceptance-judgment.md`.
The local semantic core tests pass, but release-critical Windows gates remain:
MSVC/MSBuild release matrix, packaging, production signing, clean install,
accessibility, SBOM, and final owner-controlled release approvals.

## How to build and run

See `windows/docs/build-run-instructions.md`. In short:

```powershell
cd windows\build
.\build.ps1
..\tests\run_all.ps1
```

Prerequisites: Visual Studio 2022 (or Build Tools for Visual Studio 2022) with the C++ workload, and PowerShell 5.1+ or PowerShell 7.
