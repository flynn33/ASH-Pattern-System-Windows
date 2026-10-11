# Module Mapping — APS Windows

This document maps the nine APS semantic modules to their Windows C++ source files and test coverage. The Windows edition implements these modules for supported native Windows desktop targets.

Current authority is APS `1.0.0-rc.3` at `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8`; SDK product remains `1.0.0-rc.1`. [Canonical deference](../CANONICAL-DEFERENCE.md) records the separate immutable policy/profile bindings. Implemented status below identifies source coverage, not final release qualification. Release acceptance must be established by the exact released `qualification.json`, bound to its clean source and asset bytes.

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

The explicit planner takes the seven-field `GenerationRequest` plus separate target/optional-measurement inputs. It retains complete topology, leaf indices, node roles, all five axiom diagnostics, artifact records, warnings and versioned registry metadata. `aps-windows-plan-2` binds all request/evidence fields and replaces earlier incomplete plan hashes. The emitter audits consistency as well as identity before copying all declared descriptor fields; no planner callback or filesystem generator is introduced. Supported native profile/naming/target references and the legacy overload's defaults are documented in [planning integration](../docs/sdk-integration.md#planning-and-descriptor-emission). These plan/artifact record layouts require clean rebuilds.

`Diagnostics` validates complete records and recomputes the nine-field semantic identity; its serializers cover inner records, diagnostic transports, typed containment/safe-halt transports and one complete causal-chain array. `RecoveryEngine` owns runtime restrictions and exposes typed safety projections with original/action diagnostics, ordered ancestry, triggers and UTC entry/adoption times. Prevented attempts remain blocked containment evidence; confirmed breach enters frozen terminal halt. Invalid imported graph/action evidence rejects before adoption, and earlier independent histories remain accessible for separate audit exports. Those public record layouts require consumer rebuilds.

`SupportDiagnostics.hpp` is a separate support responsibility in `ash-core`, tested by `SupportDiagnostics.test.cpp`. Debug-only source contains the actual rich collector; Release-only source contains minimal essential collection. Its immutable bounded snapshots, local paired export and session concurrency contract do not change semantic decisions or make mutable semantic module instances thread safe.

## Test Coverage

The solution builds seven test executables:

- `ash-test-statemodel`
- `ash-test-transitionregistry`
- `ash-test-diagnostics`
- `ash-test-recoveryengine`
- `ash-test-completionmodules`
- `ash-test-canonicalconformance`
- `ash-test-supportdiagnostics`

`windows/tests/run_all.ps1` requires and runs the complete seven-executable inventory on a compatible host. `CanonicalConformance.test.cpp` executes native comparisons against immutable external authority data; `SupportDiagnostics.test.cpp` checks the separately compiled Development and Release support backends. Local non-Windows syntax and behavior checks do not replace the MSVC/MSBuild release gate.
