# Windows Branch — Architecture

## Target environment

The `windows-cpp` branch targets **Windows desktop** as a downstream implementation of the ASH Pattern System.

- **Operating system:** Windows 10 (1809 or later) and Windows 11, 64-bit (x64 and ARM64).
- **Runtime environment:** Native Win32 / Universal C Runtime. No dependency on Windows Runtime (WinRT) components for the core library.
- **UI framework:** none in this pass. The current deliverable is a static library (`ash_core.lib`) and a set of test executables. A UI layer is a future expansion.
- **File system access model:** none in the slice modules. The RecoveryEngine, StateModel, TransitionRegistry, and Diagnostics modules are purely in-memory. Future modules (GenerationPlanner → ArtifactEmitter) will define file system behavior only at the ArtifactEmitter boundary, per the locked materialization boundary.
- **Concurrency / threading model:** the initial slice is single-threaded. All four slice modules are thread-unsafe by default and must be externally synchronized if used from multiple threads. This is a deliberate choice for the first slice; the locked canonical semantics are determinism and diagnosability, neither of which is affected by the threading model.

## Compiler and language

- **Compiler:** MSVC `cl.exe` from Visual Studio 2022 (17.8 or later) or Build Tools for Visual Studio 2022 with the C++ workload installed.
- **C++ standard:** C++20 (`/std:c++20`).
- **Warning level:** `/W4 /WX` (warnings as errors).
- **Exception handling:** `/EHsc` (standard C++ exceptions).
- **Runtime library:** `/MT` (static CRT) for the core library, `/MT` for test executables, so the compiled artifacts have no external DLL dependencies beyond the OS.

## Build system

MSBuild with `.sln` and `.vcxproj` files in `windows/build/`. A PowerShell wrapper script `build.ps1` locates `msbuild.exe` via the Visual Studio `vswhere` tool, invokes it with `/p:Configuration=Debug` or `Release`, and optionally runs the tests after a successful build.

The solution contains five projects:

1. `ash-core` — StaticLibrary, compiles all module `.cpp` files into `ash_core.lib`.
2. `ash-test-statemodel` — Application, links `ash_core.lib`, runs StateModel tests.
3. `ash-test-transitionregistry` — Application, links `ash_core.lib`, runs TransitionRegistry tests.
4. `ash-test-diagnostics` — Application, links `ash_core.lib`, runs Diagnostics tests.
5. `ash-test-recoveryengine` — Application, links `ash_core.lib`, runs RecoveryEngine tests.

See `windows/docs/build-run-instructions.md` for step-by-step build instructions.

## Module layout

All nine canonical ASH modules are declared as classes in `windows/src/include/ash/`. Each class is instantiable, non-virtual, and operates on value types.

```
          +-------------+
          |  StateModel |  classify_admissibility
          +------+------+  classify_system_state
                 |         classify_recoverability
                 |         normalize
                 v
        +--------+--------+
        | RecoveryEngine  |  execute_normalize_state
        +--------+--------+  execute_apply_correction
                 |            select_fallback
                 |            enter_containment
                 v            enter_safe_halt
     +-----------+-----------+
     |  TransitionRegistry   |  resolve_transition
     +-----------+-----------+  apply_transition
                 |
                 v
          +------+------+
          | Diagnostics |  validate_envelope
          +-------------+  validate_rule_id
                         validate_chain
                         build_root / build_child

  (non-slice, stubs)

  RealmEncoder  TopologyGenerator  AxiomEvaluator
  GenerationPlanner  ---[locked boundary]--->  ArtifactEmitter
```

The arrows show data and dependency flow in the slice modules: `StateModel` is the foundation, `TransitionRegistry` provides the XOR-by-codeword primitive, `Diagnostics` is a cross-cutting validator used by every module, and `RecoveryEngine` composes all three. The five non-slice modules sit below a conceptual line and currently ship as stubs that return `NOT_IMPLEMENTED` diagnostics.

The **locked materialization boundary** between `GenerationPlanner` and `ArtifactEmitter` is enforced architecturally in this pass via header file interface discipline: `GenerationPlanner`'s declared method signature returns a `GenerationPlan` value and takes no callable or I/O arguments; `ArtifactEmitter`'s declared method signature takes only `const GenerationPlan&` and `const TargetConfiguration&`. Runtime enforcement of the boundary is a future expansion item tracked in `windows/conformance/materialization-boundary-plan.md`.

## Shared types

Four shared header files under `windows/src/include/ash/` define the types used by every module:

- **`State.hpp`** — `using Bit9State = std::bitset<9>;` plus helpers for bit access and integer encoding.
- **`Codeword.hpp`** — `using Codeword = std::bitset<9>;` plus the canonical 16-element set and the 4 generators.
- **`DiagnosticEnvelope.hpp`** — the `DiagnosticEnvelope` struct and its supporting enums (`DiagnosticKind`, `Severity`, `Stage`, `Disposition`), plus a fluent `DiagnosticEnvelopeBuilder` that enforces all required fields before emitting an envelope.
- **`RuleIds.hpp`** — compile-time constants for every canonical rule ID used by the slice modules.

## Performance / resource constraints

The initial slice has no hard performance targets beyond "deterministic and diagnosable." Concretely:

- **Memory budget:** the core library has zero heap allocations in the common path for `classify_admissibility`, `apply_transition`, and `validate_envelope`. `DiagnosticEnvelope` uses `std::shared_ptr` for parent and chain root references, which implies heap allocations per envelope; the slice tests assume this is acceptable for correctness and defer a free-store-free implementation to a future optimization pass.
- **Startup time:** sub-millisecond. The library is a static library with no global initialization other than the `kCanonicalCodewords` table.
- **CPU budget:** N/A for the first slice. The algorithms are linear in the codeword count (16) or constant.
- **Storage:** N/A. The slice does not persist any state.

## Deference statement

This architecture is the Windows branch's interpretation of the canonical agnostic ASH Pattern System specification. The canonical specifications in `specs/` on main remain the authority on every semantic question. Where this document describes a Windows-specific detail that the canonical spec leaves open (thread model, memory model, CRT linkage, compiler flags), the choice is branch-local and recorded here and in `windows/conformance/deviation-log.md`. See `windows/CANONICAL-DEFERENCE.md` for the authority boundary.
