# Branch Divergence Report

## Identity

- Repository: `https://github.com/flynn33/ASH-Pattern-System-Windows.git`
- Canonical main commit: `cc253f3d137a27f0eeb471bed62bbdb939e3b6d1`
- Canonical main tag: none present in the local clone
- Resolved Windows branch/commit: `origin/main` at `b832f6e9b76a8c08038ba9dfdaf76f64691405e5`
- Merge base: `631047989227fbf938d45ed73c5287f292591904`
- Completion branch: `release/aps-windows-1.0.0-completion-clean`
- Audit date: 2026-06-21

## Candidate branches considered

| Ref | HEAD | Platform evidence | Unique work | Disposition |
|---|---|---|---|---|
| `origin/main` | `b832f6e9b76a8c08038ba9dfdaf76f64691405e5` | Windows tree, Visual Studio solution/projects, C++ source, PowerShell build, tests, conformance docs | Preserved Windows implementation line | Selected platform base |
| `upstream/main` | `cc253f3d137a27f0eeb471bed62bbdb939e3b6d1` | Canonical agnostic product baseline | Canonical data, conformance corpus, release evidence | Reference only |
| `upstream/release/aps-1.0.0-completion-clean` | `44a28980639e79bd68161422ce53c50e0791356f` | Canonical release-candidate line | APS 1.0 release candidate | Reference only |
| `upstream/release/aps-1.0.0-release-corrections` | `e620c1207b3cd05307371e5d25a89b4bd472ec33` | Canonical release-correction line | Release correction work | Reference only |

## Divergence summary

| Category | Canonical-only | Platform-only | Conflicting | Resolution |
|---|---:|---:|---:|---|
| Canonical data and corpus | Present on upstream main | Not yet vendored or executed by Windows tests | Yes | Keep as unresolved release blocker until Windows consumes and runs the corpus |
| Windows implementation tree | Absent from upstream main | Present under `windows/` | Yes | Preserve Windows tree and synchronize semantics in platform modules |
| Governance surfaces | Present in both lines | Present in platform base | No product-branch edits permitted | Verify protected surface instead of editing governance |
| Native build metadata | Absent from upstream main | Present under `windows/build/` | Yes | Preserve Visual Studio solution and add missing completion-module test project |

## Canonical changes synchronized

| Requirement/version | Files/assets | Platform adapters affected | Tests | Result |
|---|---|---|---|---|
| 512 canonical realms with b0 as most-significant bit | `windows/src/include/ash/State.hpp`, `windows/src/RealmEncoder.cpp` | Realm encoder, state model | `StateModel.test.cpp`, `CompletionModules.test.cpp` | Implemented in Windows core |
| 16 codewords and 8,192 state/codeword transformations | `windows/src/TransitionRegistry.cpp`, `windows/tests/TransitionRegistry.test.cpp` | Transition registry | `TransitionRegistry.test.cpp` | Existing registry retained and b8 ordering corrected |
| Nine APS modules present and executable | Windows core headers/sources, `windows/tests/CompletionModules.test.cpp` | Realm, topology, axiom, planning, emission modules | `CompletionModules.test.cpp` | Implemented as deterministic library behavior |
| Diagnostics taxonomy for completed modules | `DiagnosticEnvelope.hpp`, `RuleIds.hpp`, diagnostics tests | Diagnostics and module results | `Diagnostics.test.cpp`, `CompletionModules.test.cpp` | Stub family removed from changed Windows code |

## Platform work preserved

| Capability | Commits/files | Reason preserved | Verification |
|---|---|---|---|
| Native Windows build graph | `windows/build/*.vcxproj`, `windows/build/ash-windows.sln` | Required for the Windows repo | Added completion module test project |
| Windows C++ core library | `windows/src/**`, `windows/src/include/**` | Product implementation surface | Compiled with local C++20 compiler |
| Windows conformance docs | `windows/conformance/**`, `windows/docs/**` | Required platform evidence and status reporting | Updated to current implementation and `NOT_SHIPPABLE` blockers |

## Conflicts and decisions

| ID | Evidence | Root cause | Decision | Owner | Tests |
|---|---|---|---|---|---|
| APSWIN-DIV-001 | `git diff --stat origin/main..upstream/main` shows upstream main would delete `windows/` | Platform repo split from canonical agnostic repo | Do not merge upstream wholesale into Windows branch | Windows platform | Local C++20 test executables |
| APSWIN-DIV-002 | Canonical corpus exists upstream but not in this repo branch | Product data was finalized after the Windows branch split | Record as release blocker until corpus is consumed and run | Windows platform | Not complete |
| APSWIN-DIV-003 | MSBuild and PowerShell not present in local environment | Local host is not a Windows build host | Use local C++20 compiler for syntax/behavior; keep native build/signing unverified | Windows platform | Local C++20 test executables |

## Sibling branch isolation

- Windows implementation code merged: No
- macOS implementation code merged: No
- iOS implementation code merged: No
- Exceptions: None
