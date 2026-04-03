# Semantic Contracts for Future Implementations

> **PENDING REVALIDATION — Research Math Realignment Package R1**
>
> This contract layer was built on the **superseded 8+1 drift formalization**. It requires **research-baseline revalidation** before it can be treated as authoritative.
>
> No downstream implementation may claim canonical conformance based on contracts that assume the 8+1 model (8-bit "stabilizing algebraic core" + derived 9th "control/parity bit"). The canonical mathematical foundation is now the **full 9-dimensional ASH research baseline**.
>
> This file and the detailed contract files it references are pending rewrite in realignment packages R2–R3. Platform build handoff work is paused until post-realignment contract rebuild is complete.

## Umbrella contract document

This file is the **umbrella contract document** for the ASH Pattern System. It lists the required semantic modules and references the detailed contract files that define module-level implementation behavior.

The detailed contract files in `specs/interfaces/contracts/` are **canonical** for module-level implementation behavior (Phase 2 — implementation-contract lock). This umbrella document provides the high-level structure; the contract files provide the authoritative detail.

## Required semantic modules

Every downstream implementation must provide semantic equivalents of the following modules:

| # | Module | Detailed contract |
|---|--------|------------------|
| 1 | `StateModel` | `contracts/state-model-contract.md` |
| 2 | `RealmEncoder` | `contracts/realm-encoder-contract.md` |
| 3 | `TransitionRegistry` | `contracts/transition-registry-contract.md` |
| 4 | `TopologyGenerator` | `contracts/topology-generator-contract.md` |
| 5 | `AxiomEvaluator` | `contracts/axiom-evaluator-contract.md` |
| 6 | `GenerationPlanner` | `contracts/generation-planner-contract.md` |
| 7 | `ArtifactEmitter` | `contracts/artifact-emitter-contract.md` |
| 8 | `Diagnostics` | `contracts/diagnostics-module-contract.md` |
| 9 | `RecoveryEngine` | `contracts/recovery-engine-contract.md` |

Each detailed contract file defines: purpose, canonical responsibility, required inputs, required outputs, required behaviors, required diagnostics, invariants, prohibited shortcuts, and relation to other contracts/specs.

## Materialization boundary (locked)

The boundary between `GenerationPlanner` and `ArtifactEmitter` is a **locked design decision**:

- `GenerationPlanner` produces an abstract, target-aware but non-materialized plan. It must not emit artifacts or perform side effects.
- `ArtifactEmitter` materializes that plan for a target runtime/platform. It must not invent semantics not present in the plan.
- The plan is the sole interface between planner and emitter. The emitter must not call back to the planner.
- Planning and materialization must not be collapsed into a single opaque step.

See `contracts/generation-planner-contract.md` and `contracts/artifact-emitter-contract.md` for the authoritative boundary definition.

## Mandatory algebraic conformance

The control-bit derivation formula and the core admissibility law are **locked design decisions** (Design Package C). They are not open choices.

A downstream implementation **must**:

- implement the exact locked parity formula: `b0 ⊕ b1 ⊕ b2 ⊕ b3 ⊕ b4 ⊕ b5 ⊕ b6 ⊕ b7`
- implement the exact locked normative 16-codeword set from `core-admissibility.pseudo.md`
- preserve the corrected-core derivation rule: for correctable cores, derive expected control from the corrected admissible core
- not substitute a different derivation formula, codeword set, or generator matrix

## Mandatory registry, schema, and taxonomy conformance

The following are **locked design decisions** (Design Package D):

- Fallback selection must use the canonical fallback-policy registry (`specs/registries/fallback-policy-registry.md`)
- All diagnostics must conform to the shared diagnostic schema (`specs/interfaces/diagnostic-schema.md`)
- All rule IDs must conform to the canonical taxonomy (`specs/interfaces/rule-id-taxonomy.md`)

A downstream implementation must not invent local fallback policy, diagnostic structures, or rule-ID formats.

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
- invent module behavior not defined in the canonical contract files

## Portability rule

Implementations may differ in syntax, packaging, runtime model, memory layout, and tooling.
They may not differ in the semantic behavior defined by this repository and its contract files.
