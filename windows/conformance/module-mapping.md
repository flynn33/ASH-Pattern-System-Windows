# Module Mapping — windows-cpp

This document maps each of the 9 canonical ASH Pattern System semantic modules to its concrete file location on the `windows-cpp` branch. This is the first of the six downstream conformance deliverables required by `handoff-templates/common-downstream-handoff-requirements.md` on main.

## Authority boundary

The canonical agnostic main repository is the semantic authority for the ASH Pattern System. This document is a literal mapping — it names files; it does not add interpretation. Where any discrepancy exists between this document and the canonical contracts on main, the canonical contracts win. See `windows/CANONICAL-DEFERENCE.md`.

## Mapping table

| # | Canonical module | Canonical contract on `main` | Windows header | Windows implementation | Slice? | Status |
|---|---|---|---|---|---|---|
| 1 | `StateModel` | `specs/interfaces/contracts/state-model-contract.md` | `windows/src/include/ash/StateModel.hpp` | `windows/src/StateModel.cpp` | ✓ | Implemented |
| 2 | `RecoveryEngine` | `specs/interfaces/contracts/recovery-engine-contract.md` | `windows/src/include/ash/RecoveryEngine.hpp` | `windows/src/RecoveryEngine.cpp` | ✓ | Implemented |
| 3 | `TransitionRegistry` | `specs/interfaces/contracts/transition-registry-contract.md` | `windows/src/include/ash/TransitionRegistry.hpp` | `windows/src/TransitionRegistry.cpp` | ✓ | Implemented |
| 4 | `Diagnostics` | `specs/interfaces/contracts/diagnostics-module-contract.md` | `windows/src/include/ash/Diagnostics.hpp` | `windows/src/Diagnostics.cpp` | ✓ | Implemented |
| 5 | `RealmEncoder` | `specs/interfaces/contracts/realm-encoder-contract.md` | `windows/src/include/ash/RealmEncoder.hpp` | `windows/src/RealmEncoder.cpp` |   | Stub (NOT_IMPLEMENTED) |
| 6 | `TopologyGenerator` | `specs/interfaces/contracts/topology-generator-contract.md` | `windows/src/include/ash/TopologyGenerator.hpp` | `windows/src/TopologyGenerator.cpp` |   | Stub (NOT_IMPLEMENTED) |
| 7 | `AxiomEvaluator` | `specs/interfaces/contracts/axiom-evaluator-contract.md` | `windows/src/include/ash/AxiomEvaluator.hpp` | `windows/src/AxiomEvaluator.cpp` |   | Stub (NOT_IMPLEMENTED) |
| 8 | `GenerationPlanner` | `specs/interfaces/contracts/generation-planner-contract.md` | `windows/src/include/ash/GenerationPlanner.hpp` | `windows/src/GenerationPlanner.cpp` |   | Stub (NOT_IMPLEMENTED) |
| 9 | `ArtifactEmitter` | `specs/interfaces/contracts/artifact-emitter-contract.md` | `windows/src/include/ash/ArtifactEmitter.hpp` | `windows/src/ArtifactEmitter.cpp` |   | Stub (NOT_IMPLEMENTED) |

## Shared types and cross-cutting files

| Purpose | File on this branch |
|---|---|
| 9-bit state type (`Bit9State`) and coordinate accessors | `windows/src/include/ash/State.hpp` |
| Canonical 16-codeword set and generators | `windows/src/include/ash/Codeword.hpp` + `windows/src/Codeword.cpp` |
| `DiagnosticEnvelope` schema and builder | `windows/src/include/ash/DiagnosticEnvelope.hpp` + `windows/src/DiagnosticEnvelope.cpp` |
| Rule-ID constants (taxonomy-compliant) | `windows/src/include/ash/RuleIds.hpp` |

## Slice modules — functional

The four minimal conformance slice modules (`StateModel`, `RecoveryEngine`, `TransitionRegistry`, `Diagnostics`) have working C++ implementations. They share the shared types above and compose without cycles: `StateModel` is foundational, `TransitionRegistry` provides XOR-by-codeword, `Diagnostics` is a cross-cutting validator, and `RecoveryEngine` composes all three. Each slice module has a dedicated test executable under `windows/tests/` that covers its canonical invariants.

## Non-slice modules — stubs

The five non-slice modules (`RealmEncoder`, `TopologyGenerator`, `AxiomEvaluator`, `GenerationPlanner`, `ArtifactEmitter`) ship on this branch as:

- A header file (`.hpp`) containing the full interface declaration with doxygen-style contract comments that quote the canonical contract from main.
- A `.cpp` stub whose every method returns a well-formed `NOT_IMPLEMENTED` `DiagnosticEnvelope`. The envelope carries the branch-local `ASH-WINDOWS-STUB-001` rule ID (documented in `deviation-log.md` item 4).

`GenerationPlanner.hpp` and `ArtifactEmitter.hpp` carry a boxed LOCKED MATERIALIZATION BOUNDARY comment quoting `specs/interfaces/semantic-contracts.md` on main. Architectural enforcement of the boundary is at the header signature level: `GenerationPlanner::generate_plan` accepts no side-effecting inputs; `ArtifactEmitter::materialize_plan` takes only the plan and target configuration, with no reference to the planner. See `materialization-boundary-plan.md`.

## Expansion path

The non-slice module stubs are structured so that implementation work in future passes fills in `.cpp` bodies without changing headers. Each stub's method signature is already final per the canonical contract; only the behavior is deferred.
