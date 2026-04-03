# Design Roadmap

## Goal

Move from a corrected semantic baseline to implementation-ready handoff materials without collapsing the design into any single language or platform.

## Sequence

### Phase 1 — lock the semantic foundation

Complete and stabilize:

- ASH state-space specification
- realm identity specification
- transition semantics
- topology expansion semantics
- axiom evaluation semantics
- generation-planning semantics

### Phase 1.5 — close the formal layers

Complete the formal layers that sit between the semantic kernel and implementation contracts:

- **control-bit derivation** — define the semantic role, interface, invariants, and diagnostic behavior of the derived control dimension (formula locked in Design Package C)
- **core admissibility** — define which 8-bit core vectors are structurally admissible under the [8,4,4] extended Hamming code (codeword set locked in Design Package C)
- **state-validity diagnostics** — define a canonical diagnostic record that every implementation must produce for any candidate state

Specifications created for this phase:

- `specs/core/control-bit-derivation.pseudo.md`
- `specs/core/core-admissibility.pseudo.md`
- `specs/core/state-validity-diagnostics.pseudo.md`

**Status**: Complete. The two algebraic closure items (derivation formula and codeword set) were resolved in Design Package C.

### Phase 1.75 — define resilient software semantics (Design Package B)

Build the system-behavior layer on top of the state-validity foundation:

- **system-state classification** — define the 7 canonical state classes (STABLE, UNSTABLE, CORRECTABLE, DEGRADED, CONTAINED, FAILED, SAFE_HALT) and the deterministic class-to-action mapping
- **recoverability semantics** — define the recovery category for each state class and blocked-recovery conditions
- **recovery/fallback semantics** — define the algorithmic flow for deterministic correction, fallback selection, and escalation
- **containment/safe-failure semantics** — define when containment overrides correction, when safe failure is mandatory, and what SAFE_HALT means as a terminal state
- **corrected-core derivation rule** — harmonize all state-layer specs so expected-control semantics for correctable cores are defined on the corrected admissible core

Specifications created for this phase:

- `specs/core/system-state-classification.pseudo.md`
- `specs/core/recoverability-semantics.pseudo.md`
- `specs/algorithms/recovery-fallback-semantics.pseudo.md`
- `specs/algorithms/containment-safe-failure-semantics.pseudo.md`

**Status**: Complete. Design Package B is formally closed.

**Status**: Complete. The fallback-policy registry was defined in Design Package D.

### Phase 1.85 — lock the algebraic foundation (Design Package C)

Lock the two remaining foundational algebraic items:

- **control-bit derivation formula** — locked to overall parity: `b0 ⊕ b1 ⊕ b2 ⊕ b3 ⊕ b4 ⊕ b5 ⊕ b6 ⊕ b7`
- **core admissibility law** — locked to the exact [8,4,4] extended Hamming code with normative 16-codeword set and generator matrix G

Consequence: all admissible normalized cores derive `control_bit = 0`. The control dimension is a deterministic parity sentinel.

**Status**: Complete. Design Package C is the algebraic-lock package. Both items are now locked.

### Phase 1.9 — registry and diagnostics layer (Design Package D)

Close the remaining structural gaps to achieve end-to-end diagnostic completeness:

- **fallback-policy registry** — define the canonical registry governing deterministic fallback selection (`specs/registries/fallback-policy-registry.md`)
- **unified diagnostic schema** — define the shared diagnostic envelope for all diagnostic contexts (`specs/interfaces/diagnostic-schema.md`)
- **rule-ID taxonomy** — define the canonical rule identifier structure and governance (`specs/interfaces/rule-id-taxonomy.md`)
- **end-to-end diagnostic alignment** — ensure all diagnostic-bearing specs conform to the shared schema and use taxonomy-compliant rule IDs

**Status**: Complete. Design Package D is the registry and diagnostics closure package. All items are now canonical.

### Phase 2 — lock the implementation contracts

Create detailed, canonical contract files for all 9 required semantic modules:

- `StateModel` — normalization, locked derivation/admissibility, diagnostics, classification
- `RecoveryEngine` — recovery, fallback, containment, safe halt, monotonic escalation
- `RealmEncoder` — deterministic encoding from normalized state
- `TransitionRegistry` — deterministic transition resolution, core/control preservation
- `TopologyGenerator` — deterministic topology generation, stable ordering/lineage
- `AxiomEvaluator` — explainable evaluation, no silent pass/fail
- `GenerationPlanner` — abstract planning, no side effects, explicit handoff to emitter
- `ArtifactEmitter` — plan materialization only, no invention of semantics
- `Diagnostics` — schema conformance, taxonomy compliance, chain exposure

Lock the materialization boundary: `GenerationPlanner` plans, `ArtifactEmitter` materializes, neither crosses the other's boundary.

Update `semantic-contracts.md` as the umbrella contract document referencing the detailed contract files.

**Status**: Complete. Phase 2 is the implementation-contract lock package.

### Phase 3 — define invariant-based verification requirements

Define the canonical verification layer that downstream implementations must satisfy:

- **invariant specification** — canonical invariant set covering normalization, control integrity, transitions, topology, axiom diagnostics, recovery/fallback/containment, generation-plan structure, materialization boundary, and diagnostic schema/taxonomy conformance
- **conformance categories** — 5 verification buckets: algebraic/state, recovery/fallback/containment, diagnostics, generation/materialization-boundary, contract/module
- **implementation acceptance** — minimum acceptance threshold, failure conditions, non-waivable locked requirements, and acceptance judgment language (CONFORMANT / NON-CONFORMANT)

Specifications created:

- `specs/verification/invariant-spec.md`
- `specs/verification/conformance-categories.md`
- `specs/verification/implementation-acceptance.md`

**Status**: Complete. Phase 3 is the invariant-based verification requirements package. The verification layer is canonical.

### Phase 4 — create platform build handoff packages

For each target implementation repo, the coding agent should receive:

- this repository
- target platform constraints
- target language constraints
- runtime constraints
- packaging constraints
- performance constraints if applicable

## Immediate next design step

Design Packages A–D, Phase 2, and Phase 3 are complete. The semantic, algebraic, diagnostic, implementation-contract, and verification foundation is fully locked. The immediate next step is **Phase 4 — create platform build handoff packages**.

Phase 4 will create target-specific handoff packages that combine this repository with platform constraints, language constraints, and runtime constraints for each downstream implementation.
