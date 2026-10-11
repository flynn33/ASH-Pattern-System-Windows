# Materialization Boundary Plan — APS Windows

The Windows implementation preserves the APS planner/emitter boundary:

- `GenerationPlanner` produces an inspectable in-memory `GenerationPlan`.
- `GenerationPlanner` does not perform filesystem, process, network, UI, or package side effects.
- `ArtifactEmitter` consumes only `GenerationPlan` and `TargetConfiguration`.
- `ArtifactEmitter` does not call back into the planner.
- Every emitted artifact descriptor traces to a plan element.

## Windows Interfaces

```cpp
PlanResult GenerationPlanner::generate_plan(const GenerationRequest& request,
                                            const GenerationPlanningInputs& inputs) const;

PlanResult GenerationPlanner::generate_plan(const Bit9State& state,
                                            const TargetMetadata& target) const;

EmissionResult ArtifactEmitter::materialize_plan(const GenerationPlan& plan,
                                                 const TargetConfiguration& config) const;
```

`GenerationRequest` contains exactly project name, seed state, ordered transition references, signed topology depth, profile ID, naming policy ID and emission target kind. `GenerationPlanningInputs` separately contains target metadata, optional measured `AxiomSubject` and its fixed-predicate profile label. The native registry supports `aps-windows-descriptor-1.0`, `aps-windows-realm-names-1.0` or `aps-windows-project-names-1.0`, descriptor output for desktop/mobile/service and topology depths 0 through 6. Project names use nonempty ASCII letters, digits, dot, underscore and hyphen. Invalid references/inputs return blocked diagnostics and an empty plan.

`GenerationPlan` stores request/inputs, normalized and fully transformed state, source/destination realm identities, all ordered topology nodes and leaves, every typed node role, all five explanatory axiom results and aggregate, warnings, declared artifact records and registry metadata. Missing measured evidence remains `INDETERMINATE`; known failures remain explicit. The legacy state/target overload uses project `ash-plan`, no transitions, depth zero, realm naming, descriptor output and no measured subject.

Its identity is lowercase SHA256 over the exposed `generation_plan_identity_payload(plan)` in native format `aps-windows-plan-2`. Named fields bind byte lengths, arrays bind ordered counts, optional evidence binds presence and doubles bind exact IEEE-754 bits. Every request, stored evidence and registry field participates; `plan_hash` itself is excluded. Earlier incomplete hashes require regeneration with old provenance retained. These public plan/artifact layout changes require a consumer rebuild/relink.

The emitter audits the complete stored plan with fixed semantic modules, validates its hash and compatible target class, then copies every declared artifact's ID, kind, name, summary and role references with its plan-element reference. An empty configured target class uses the plan's class; a conflicting nonempty class is blocked. It rejects incomplete or inconsistent evidence even when the caller rehashes it. It does not call the planner or reconstruct missing application evidence.

## Runtime Verification

`windows/tests/CompletionModules.test.cpp` verifies:

- full-state transitions, normalized/transformed realm identity, complete topology and role ordering;
- all five explanatory axiom results, missing/nonfinite measurements, known failures and warnings;
- supported references, invalid request/target inputs and legacy defaults;
- deterministic identity and changes for behavior-affecting request/evidence/registry fields;
- unambiguous length prefixes and exact measurement bits;
- one descriptor per declaration, all declared fields and `artifact_descriptions[index]` traceability;
- stale hashes, tampering and rehashed incomplete/inconsistent evidence blocked before output.

This list describes source test coverage; the final clean-source qualification report must prove actual execution against the selected artifacts.

## SDK scope and acceptance

The authorized SDK emits deterministic in-memory artifact descriptors. `TargetConfiguration::output_directory` is a reserved caller value; the emitter performs no filesystem writes. The application owns any later file or runtime materialization. A production filesystem generator is outside this SDK work order and is not a release prerequisite.

Final acceptance still requires the complete planner/emitter contract, native comparisons, isolated SDK consumers and artifact-bound review against the exact clean source. Release acceptance must be established by the exact released `qualification.json`, bound to its clean source and asset bytes. Current canonical APS `1.0.0-rc.3` and separate engineering-policy/profile bindings are recorded in [canonical deference](../CANONICAL-DEFERENCE.md); historical rc.1 contradictions and actual rc.2 repair/integration evidence remain separate.
