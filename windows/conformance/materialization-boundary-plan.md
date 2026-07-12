# Materialization Boundary Plan — APS Windows

The Windows implementation preserves the APS planner/emitter boundary:

- `GenerationPlanner` produces an inspectable in-memory `GenerationPlan`.
- `GenerationPlanner` does not perform filesystem, process, network, UI, or package side effects.
- `ArtifactEmitter` consumes only `GenerationPlan` and `TargetConfiguration`.
- `ArtifactEmitter` does not call back into the planner.
- Every emitted artifact descriptor traces to a plan element.

## Windows Interfaces

```cpp
PlanResult GenerationPlanner::generate_plan(const Bit9State& state,
                                            const TargetMetadata& target) const;

EmissionResult ArtifactEmitter::materialize_plan(const GenerationPlan& plan,
                                                 const TargetConfiguration& config) const;
```

`GenerationPlan` includes source realm identity, target class, role assignments, artifact descriptions, registry version, and a deterministic SHA-256 plan hash.

## Runtime Verification

`windows/tests/CompletionModules.test.cpp` verifies:

- the planner returns source realm identity and target class;
- the planner returns non-empty artifact descriptions;
- the planner returns a 64-character SHA-256 hex hash;
- the emitter creates one artifact descriptor per plan artifact description;
- every emitted descriptor traces to `artifact_descriptions[index]`;
- incomplete plans are blocked with an error diagnostic.

## Remaining Release Work

This core implementation emits deterministic artifact descriptors. A production Windows materializer that writes files must add staged/atomic file writes, collision handling, permission and low-storage tests, rollback behavior, and clean install/package evidence before release acceptance.
