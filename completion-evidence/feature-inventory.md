# Feature Inventory

Date: 2026-06-21

| Capability | Status | Evidence |
|---|---|---|
| Canonical state conversion | Implemented | `windows/src/include/ash/State.hpp`, `windows/tests/StateModel.test.cpp` |
| Structural state normalization | Implemented | `windows/src/StateModel.cpp`, `windows/tests/StateModel.test.cpp` |
| Transition registry | Implemented | `windows/src/TransitionRegistry.cpp`, `windows/tests/TransitionRegistry.test.cpp` |
| Diagnostics chain construction | Implemented | `windows/src/Diagnostics.cpp`, `windows/tests/Diagnostics.test.cpp` |
| Recovery planning | Implemented | `windows/src/RecoveryEngine.cpp`, `windows/tests/RecoveryEngine.test.cpp` |
| Realm and orbit identity encoding | Implemented | `windows/src/RealmEncoder.cpp`, `windows/tests/CompletionModules.test.cpp` |
| Ordered ternary topology generation | Implemented | `windows/src/TopologyGenerator.cpp`, `windows/tests/CompletionModules.test.cpp` |
| Axiom evaluation | Implemented | `windows/src/AxiomEvaluator.cpp`, `windows/tests/CompletionModules.test.cpp` |
| Side-effect-free generation planning | Implemented | `windows/src/GenerationPlanner.cpp`, `windows/tests/CompletionModules.test.cpp` |
| Descriptor-only artifact emission | Implemented | `windows/src/ArtifactEmitter.cpp`, `windows/tests/CompletionModules.test.cpp` |
| Production UI | Not implemented in this branch | Recorded as release blocker |
| Signed installer/package | Not implemented in this branch | Recorded as release blocker |
| Full upstream vector corpus execution | Not complete | Local exhaustive realm/transition tests exist; full upstream JSON/vector corpus remains a release blocker |
