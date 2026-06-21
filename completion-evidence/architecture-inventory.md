# Architecture Inventory

Date: 2026-06-21

## Windows Tree

| Area | Paths | Role |
|---|---|---|
| Public headers | `windows/src/include/ash/*.hpp` | APS module contracts and data types |
| Core implementation | `windows/src/*.cpp` | Native C++ module behavior |
| Build metadata | `windows/build/*.vcxproj`, `windows/build/ash-windows.sln` | Visual Studio build graph |
| Test code | `windows/tests/*.test.cpp`, `windows/tests/test_runner.hpp` | Unit and module conformance checks |
| Windows docs | `windows/docs/*.md`, `windows/conformance/*.md` | Platform status, architecture, and conformance evidence |

## APS Modules

| Module | Status | Primary files |
|---|---|---|
| StateModel | Implemented | `StateModel.hpp`, `StateModel.cpp` |
| TransitionRegistry | Implemented | `TransitionRegistry.hpp`, `TransitionRegistry.cpp` |
| Diagnostics | Implemented | `Diagnostics.hpp`, `Diagnostics.cpp`, `DiagnosticEnvelope.hpp` |
| RecoveryEngine | Implemented | `RecoveryEngine.hpp`, `RecoveryEngine.cpp` |
| RealmEncoder | Implemented | `RealmEncoder.hpp`, `RealmEncoder.cpp` |
| TopologyGenerator | Implemented | `TopologyGenerator.hpp`, `TopologyGenerator.cpp` |
| AxiomEvaluator | Implemented | `AxiomEvaluator.hpp`, `AxiomEvaluator.cpp` |
| GenerationPlanner | Implemented | `GenerationPlanner.hpp`, `GenerationPlanner.cpp` |
| ArtifactEmitter | Implemented | `ArtifactEmitter.hpp`, `ArtifactEmitter.cpp` |

## Boundaries

- Core modules expose value-based inputs and outputs.
- Generation planning returns an inspectable plan and does not perform materialization.
- Artifact emission returns in-memory descriptors and does not perform filesystem, process, network, or UI work.
- Recovery and diagnostics preserve structured diagnostic envelopes.
