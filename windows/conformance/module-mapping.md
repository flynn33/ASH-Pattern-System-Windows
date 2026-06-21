# Module Mapping — APS Windows

This document maps the nine APS semantic modules to their Windows C++ source files and test coverage. The Windows edition implements these modules for supported native Windows desktop targets.

## Mapping Table

| # | Canonical module | Windows header | Windows implementation | Primary tests | Status |
|---|---|---|---|---|---|
| 1 | `StateModel` | `windows/src/include/ash/StateModel.hpp` | `windows/src/StateModel.cpp` | `windows/tests/StateModel.test.cpp` | Implemented |
| 2 | `RecoveryEngine` | `windows/src/include/ash/RecoveryEngine.hpp` | `windows/src/RecoveryEngine.cpp` | `windows/tests/RecoveryEngine.test.cpp` | Implemented |
| 3 | `TransitionRegistry` | `windows/src/include/ash/TransitionRegistry.hpp` | `windows/src/TransitionRegistry.cpp` | `windows/tests/TransitionRegistry.test.cpp` | Implemented |
| 4 | `Diagnostics` | `windows/src/include/ash/Diagnostics.hpp` | `windows/src/Diagnostics.cpp` | `windows/tests/Diagnostics.test.cpp` | Implemented |
| 5 | `RealmEncoder` | `windows/src/include/ash/RealmEncoder.hpp` | `windows/src/RealmEncoder.cpp` | `windows/tests/CompletionModules.test.cpp` | Implemented with realm and orbit IDs |
| 6 | `TopologyGenerator` | `windows/src/include/ash/TopologyGenerator.hpp` | `windows/src/TopologyGenerator.cpp` | `windows/tests/CompletionModules.test.cpp` | Implemented |
| 7 | `AxiomEvaluator` | `windows/src/include/ash/AxiomEvaluator.hpp` | `windows/src/AxiomEvaluator.cpp` | `windows/tests/CompletionModules.test.cpp` | Implemented |
| 8 | `GenerationPlanner` | `windows/src/include/ash/GenerationPlanner.hpp` | `windows/src/GenerationPlanner.cpp` | `windows/tests/CompletionModules.test.cpp` | Implemented |
| 9 | `ArtifactEmitter` | `windows/src/include/ash/ArtifactEmitter.hpp` | `windows/src/ArtifactEmitter.cpp` | `windows/tests/CompletionModules.test.cpp` | Implemented |

## Shared Types

| Purpose | File |
|---|---|
| 9-bit state type, canonical integer encoding, signatures, realm IDs | `windows/src/include/ash/State.hpp` |
| Canonical 16-codeword set and generators | `windows/src/include/ash/Codeword.hpp`, `windows/src/Codeword.cpp` |
| Diagnostic envelope schema and builder | `windows/src/include/ash/DiagnosticEnvelope.hpp`, `windows/src/DiagnosticEnvelope.cpp` |
| Rule-ID constants | `windows/src/include/ash/RuleIds.hpp` |

## Implementation Notes

The Windows core is a native C++ static library. The semantic core remains independent of UI, packaging, signing, and deployment concerns. The planner/emitter boundary is preserved: `GenerationPlanner` returns an inspectable in-memory plan and deterministic hash; `ArtifactEmitter` consumes only that plan plus target configuration and traces each emitted descriptor to a plan element.

## Test Coverage

The solution now builds five test executables:

- `ash-test-statemodel`
- `ash-test-transitionregistry`
- `ash-test-diagnostics`
- `ash-test-recoveryengine`
- `ash-test-completionmodules`

`windows/tests/run_all.ps1` runs all five after a successful MSBuild build. Local non-Windows syntax and behavior checks can compile the same test files with a C++20 compiler, but the release gate remains MSVC/MSBuild on Windows.
