# Materialization Boundary Plan — windows-cpp

Documents how the windows-cpp branch enforces the LOCKED materialization boundary between `GenerationPlanner` and `ArtifactEmitter` defined in `specs/interfaces/semantic-contracts.md` on main.

## Canonical source (verbatim)

From `specs/interfaces/semantic-contracts.md` § Materialization boundary (locked):

> The boundary between `GenerationPlanner` and `ArtifactEmitter` is a locked design decision:
> - GenerationPlanner produces an abstract, target-aware but non-materialized plan. It must not emit artifacts or perform side effects.
> - ArtifactEmitter materializes that plan for a target runtime/platform. It must not invent semantics not present in the plan.
> - The plan is the sole interface between planner and emitter.
> - Planning and materialization must not be collapsed into a single opaque step.

## Canonical invariants enforced by the boundary

- **INV-PLAN-001** Planning produces no side effects
- **INV-PLAN-002** Plan is self-contained — emitter needs only the plan
- **INV-BOUNDARY-001** GenerationPlanner does not emit artifacts
- **INV-BOUNDARY-002** Every emitted artifact is traceable to a plan element

## Architectural enforcement on this branch

The five non-slice modules — including `GenerationPlanner` and `ArtifactEmitter` — ship on this branch as headers plus NOT_IMPLEMENTED stubs. The boundary is therefore enforced *architecturally* at the header file level; runtime enforcement is a future expansion item.

### `GenerationPlanner.hpp`

The `generate_plan` method signature accepts only pure-data inputs:

```cpp
PlanResult generate_plan(const Bit9State& state, const TargetMetadata& target) const;
```

There is no:
- Filesystem path parameter
- Output stream parameter
- Callback parameter
- Shared mutable state dependency
- Permission to include `<filesystem>`, `<fstream>`, `<cstdio>` for write operations, or any I/O-capable header

The method returns a `PlanResult` — a value type containing a `GenerationPlan` and a `DiagnosticEnvelope`. The plan itself is a plain struct with vectors of strings describing artifacts and role assignments. By construction, the method cannot perform side effects — it would have to include an I/O header to do so, and the stub's `#include` list deliberately contains only the envelope types and rule-ID constants.

### `GenerationPlanner.cpp`

The stub `.cpp` file includes exactly these headers:

```cpp
#include "ash/GenerationPlanner.hpp"
#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"
```

No `<filesystem>`, no `<fstream>`, no `<cstdio>` used for write, no `<process>`, no Windows header that would permit side effects. This is architectural enforcement of INV-PLAN-001 at the translation-unit level. Any future expansion that implements `generate_plan` must maintain this discipline — any PR adding an I/O header to `GenerationPlanner.cpp` must be reviewed as a potential boundary violation.

### `ArtifactEmitter.hpp`

The `materialize_plan` method signature takes only the plan and a target configuration:

```cpp
EmissionResult materialize_plan(const GenerationPlan& plan,
                                const TargetConfiguration& config) const;
```

There is no `GenerationPlanner&` parameter. By construction the emitter cannot call back to the planner; the plan is the sole input from the planning side. This is architectural enforcement of INV-PLAN-002 and the "no callback to planner" rule from the canonical contract.

### `ArtifactEmitter.cpp`

The stub `.cpp` file currently includes only the envelope types and rule-ID constants. When `materialize_plan` is implemented in a future expansion pass, the implementation may include I/O headers — that is expected and correct, because the emitter is the materialization side of the boundary. The prohibition is on:

- Calling back to a `GenerationPlanner` instance
- Inventing semantics not present in the plan (every emitted artifact must carry a `plan_element_reference` pointing at the plan element that justified it)
- Skipping plan elements

These are enforced by future runtime tests and code review; the header signature enforces the no-callback rule architecturally today.

## Runtime enforcement (future expansion)

Runtime enforcement of the boundary requires:

1. **INV-PLAN-001 runtime test:** invoke `GenerationPlanner::generate_plan` in a test harness that observes filesystem, process, and network state before and after, asserting no change. This requires a small sandbox or filesystem snapshot; it is deferred until `GenerationPlanner` is implemented.
2. **INV-PLAN-002 runtime test:** invoke `ArtifactEmitter::materialize_plan` with only the plan and config, verifying that the emitter does not attempt to access a planner (this is already enforced by the lack of planner reference in the signature — the test would only re-verify at runtime).
3. **INV-BOUNDARY-001 runtime test:** same observer as INV-PLAN-001; verify no artifacts created during planning.
4. **INV-BOUNDARY-002 runtime test:** after calling `materialize_plan`, iterate the resulting `Artifact` list and assert every `plan_element_reference` corresponds to an element in the input plan.

These tests land with the real implementations of `GenerationPlanner` and `ArtifactEmitter` in a future expansion pass. Until then, the `acceptance-judgment.md` placeholder explicitly notes Categories 4 invariants as "covered (architectural) + deferred (runtime)".

## Review guidance

Any PR on this branch that modifies `GenerationPlanner.cpp` or `ArtifactEmitter.cpp` must be reviewed with the following checklist:

- Does `GenerationPlanner.cpp` include any I/O header? If yes, reject.
- Does `GenerationPlanner::generate_plan`'s signature still take only pure data? If it now takes a filesystem path, output stream, or callback, reject.
- Does `ArtifactEmitter::materialize_plan` take any reference to a `GenerationPlanner`? If yes, reject.
- Does the emitter invent any semantics not present in the plan? If yes, reject.
- Does every emitted `Artifact` carry a `plan_element_reference` that maps back to the input plan? If no, reject.

These are not optional. The boundary is LOCKED at the canonical level and must remain LOCKED at the implementation level.
