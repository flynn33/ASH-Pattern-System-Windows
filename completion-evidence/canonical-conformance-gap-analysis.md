<<<<<<< HEAD
# Canonical Conformance Gap Analysis

Date: 2026-06-21

## Satisfied Locally

| Requirement | Local evidence |
|---|---|
| 512 realm identities | `StateModel.test.cpp` enumerates 512 identities; `CompletionModules.test.cpp` checks realm ID, orbit ID, and signature behavior |
| b0 most-significant ordering | `State.hpp` conversion and transition tests |
| 16 codewords | Existing `Codeword` tests through transition and recovery paths |
| Transition behavior | `TransitionRegistry.test.cpp` checks all 8,192 state/codeword pairs and noncanonical rejection |
| Diagnostics without branch-local temporary rule family | `Diagnostics.test.cpp`, `CompletionModules.test.cpp` |
| Nine module presence | `CompletionModules.test.cpp`, `windows/conformance/module-mapping.md` |
| Materialization boundary | `GenerationPlanner.cpp`, `ArtifactEmitter.cpp`, completion-module tests |

## Remaining Gaps

| ID | Severity | Gap | Required closure |
|---|---|---|---|
| APSWIN-GAP-001 | Blocker | Canonical upstream JSON corpus is not yet vendored into this Windows branch. | Add a platform-safe canonical data source or consume upstream release artifacts without touching protected governance surfaces. |
| APSWIN-GAP-002 | Blocker | Local tests cover all 512 realm identities and all 8,192 state/codeword pairs, but the full upstream JSON/vector corpus is not wired to Windows build/test commands. | Add and run corpus verification covering every upstream canonical vector category. |
| APSWIN-GAP-003 | Blocker | Native Windows build, package, signing, and clean-install evidence is absent. | Run the Visual Studio/MSBuild matrix and release packaging on a Windows build host. |
| APSWIN-GAP-004 | Blocker | Production UI/accessibility scope is not represented in this branch. | Either implement the product UI/accessibility surface or record an owner-approved scope decision that the Windows repo is a native core package. |

