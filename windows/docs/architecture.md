# Windows Edition - Architecture

## Target Environment

The Windows edition targets supported Windows desktop systems.

- **Operating system:** Windows 10 version 1809 or later and Windows 11, 64-bit x64 and ARM64.
- **Runtime environment:** Native Win32 / Universal C Runtime. The core library does not depend on Windows Runtime components.
- **UI framework:** none in the current repository state. The current deliverable is a static library (`ash_core.lib`) and a set of test executables. A product UI remains a release blocker until implemented and accessibility-tested.
- **File system access model:** none in the semantic core. `GenerationPlanner` returns an in-memory plan, and `ArtifactEmitter` returns in-memory descriptors that trace to plan elements; neither performs filesystem writes in the current implementation.
- **Concurrency / threading model:** the semantic core is single-threaded. Module instances are externally synchronized if shared from multiple threads. The implementation does not introduce background work or mutable global state.

## Compiler And Language

- **Compiler:** MSVC `cl.exe` from Visual Studio 2022 version 17.8 or later, or Build Tools for Visual Studio 2022 with the C++ workload installed.
- **C++ standard:** C++20 (`/std:c++20`).
- **Warning level:** `/W4 /WX`.
- **Exception handling:** `/EHsc`.
- **Runtime library:** `/MT` for the core library and test executables.

## Build System

MSBuild assets live in `windows/build/`. The PowerShell wrapper `build.ps1` locates `msbuild.exe` through Visual Studio tooling, invokes the configured build or clean target, and can run tests after a successful build.

The solution contains six projects:

1. `ash-core` - static library, compiles all module `.cpp` files into `ash_core.lib`.
2. `ash-test-statemodel` - application, links `ash_core.lib`, runs StateModel tests.
3. `ash-test-transitionregistry` - application, links `ash_core.lib`, runs TransitionRegistry tests.
4. `ash-test-diagnostics` - application, links `ash_core.lib`, runs Diagnostics tests.
5. `ash-test-recoveryengine` - application, links `ash_core.lib`, runs RecoveryEngine tests.
6. `ash-test-completionmodules` - application, links `ash_core.lib`, runs RealmEncoder, TopologyGenerator, AxiomEvaluator, GenerationPlanner, and ArtifactEmitter tests.

See `windows/docs/build-run-instructions.md` for step-by-step build instructions.

## Module Layout

All nine APS modules are declared as classes in `windows/src/include/ash/`. Each class is instantiable, non-virtual, and operates on value types.

```text
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

  RealmEncoder  TopologyGenerator  AxiomEvaluator
  GenerationPlanner  ---[locked boundary]--->  ArtifactEmitter
```

The arrows show data and dependency flow: `StateModel` is the foundation, `TransitionRegistry` provides the XOR-by-codeword primitive, `Diagnostics` is a cross-cutting validator used by every module, and `RecoveryEngine` composes the state and transition primitives. Realm encoding, topology, axiom evaluation, planning, and emission are implemented as module-local services.

The locked materialization boundary between `GenerationPlanner` and `ArtifactEmitter` is enforced by interface discipline and runtime tests: `GenerationPlanner` returns a `GenerationPlan` value and takes no callable or I/O arguments; `ArtifactEmitter` takes only `const GenerationPlan&` and `const TargetConfiguration&`, then traces every emitted descriptor to a plan element.

## Shared Types

Four shared header files under `windows/src/include/ash/` define the types used by every module:

- **`State.hpp`** - `using Bit9State = std::bitset<9>;` plus helpers for bit access and integer encoding.
- **`Codeword.hpp`** - `using Codeword = std::bitset<9>;` plus the 16-element codeword set and the 4 generators.
- **`DiagnosticEnvelope.hpp`** - the `DiagnosticEnvelope` struct and its supporting enums (`DiagnosticKind`, `Severity`, `Stage`, `Disposition`), plus a fluent `DiagnosticEnvelopeBuilder` that enforces all required fields before emitting an envelope.
- **`RuleIds.hpp`** - compile-time constants for every rule ID used by the semantic core modules.

## Performance / Resource Constraints

The semantic core has no hard performance targets beyond deterministic and diagnosable behavior. Concretely:

- **Memory budget:** the core library has zero heap allocations in the common path for `classify_admissibility`, `apply_transition`, and `validate_envelope`. `DiagnosticEnvelope` uses `std::shared_ptr` for parent and chain root references, which implies heap allocations per envelope; current tests treat that as acceptable for correctness and keep allocation-free diagnostics as a later optimization topic.
- **Startup time:** sub-millisecond. The library is a static library with no global initialization other than the `kCanonicalCodewords` table.
- **CPU budget:** core operations are constant, linear in the codeword count of 16, or bounded across the 512-state APS space.
- **Storage:** not applicable. The semantic core does not persist state.

## Platform Boundary

This architecture records the Windows edition's implementation choices: thread model, memory model, CRT linkage, compiler flags, project layout, and test harness behavior. These choices are local to the Windows edition and are tracked here and in `windows/conformance/deviation-log.md` when they affect release readiness.
