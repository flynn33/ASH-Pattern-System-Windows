# Semantic Contracts for Future Implementations

## Required semantic modules

Every downstream implementation must provide semantic equivalents of the following modules:

1. `StateModel`
2. `RealmEncoder`
3. `TransitionRegistry`
4. `TopologyGenerator`
5. `AxiomEvaluator`
6. `GenerationPlanner`
7. `ArtifactEmitter`
8. `Diagnostics`

## Required behaviors

### `StateModel`
Must:

- normalize states deterministically where normalization is defined
- distinguish the 8-bit core from the derived control dimension
- derive the control bit deterministically using the function defined in `control-bit-derivation.pseudo.md`
- classify core admissibility using the rules defined in `core-admissibility.pseudo.md`
- validate state consistency and produce a `StateValidityDiagnostic` as defined in `state-validity-diagnostics.pseudo.md`
- report diagnosable failure when normalization cannot proceed (e.g., inadmissible core beyond correctable distance, or derivation formula not yet locked)
- never silently invent missing semantics — if a specification marks a rule as an unresolved closure item, the implementation must not guess a value and must report the gap in diagnostics

### `RealmEncoder`
Must:

- encode realm identity from normalized state
- produce deterministic results
- represent control semantics explicitly

### `TransitionRegistry`
Must:

- resolve transitions deterministically
- apply transitions to normalized states
- preserve the rule that ordinary transitions operate on the core and then re-derive control

### `TopologyGenerator`
Must:

- generate deterministic ternary topology
- preserve stable ordering and lineage

### `AxiomEvaluator`
Must:

- return the full diagnostic record
- explain failure states in diagnostic notes

### `GenerationPlanner`
Must:

- produce an abstract generation plan before any side effects occur
- include topology, role assignment, axiom diagnostics, and artifact descriptions

### `ArtifactEmitter`
Must:

- materialize a generation plan for a target platform
- preserve the meaning of the plan rather than invent new semantics

### `Diagnostics`
Must:

- expose explainable validation information for states, transitions, topology, and axioms

## Prohibited shortcuts

A downstream implementation must not:

- treat the 9th coordinate as an ordinary unrestricted peer bit during ordinary transitions
- treat syntax as the source of truth over semantics
- skip normalization before encoding or transition application
- collapse planning and materialization into one opaque semantic step
- replace semantic validation with superficial metadata checks
- silently accept an inadmissible core as valid
- produce a control bit by any means other than the canonical derivation function
- skip admissibility classification before normalization

## Prohibited guessing of unresolved semantics

Some specifications in this repository contain explicitly marked **unresolved closure items** — foundational design decisions that have not yet been locked. These include (but are not limited to):

- the exact derivation formula for the control bit (see `control-bit-derivation.pseudo.md`)
- the exact codeword set / generator matrix for core admissibility (see `core-admissibility.pseudo.md`)

A downstream implementation **must not**:

- invent a derivation formula where the specification has not locked one
- invent a codeword set where the specification has not locked one
- use a placeholder formula or codeword set in production without explicit acknowledgment
- treat a guessed value as canonical
- assume that any particular formula or codeword set is correct based on general knowledge of coding theory — the ASH Pattern System may choose a specific variant for semantic reasons

A downstream implementation **must**:

- structure its code so that unresolved closure items are single replaceable points of definition
- report `UNABLE_TO_DERIVE` or equivalent in diagnostics when a closure item is not yet locked
- refuse to normalize or validate states when required closure items are missing, rather than silently producing incorrect results

## Portability rule

Implementations may differ in syntax, packaging, runtime model, memory layout, and tooling.
They may not differ in the semantic behavior defined by this repository.
