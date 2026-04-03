# AI Coding Handoff

> **REALIGNED — Research Math Realignment Package R1**
>
> The ASH Pattern System has been realigned to the **full 9-dimensional research baseline**. The previous 8+1 formalization (8-bit core + derived control bit) is **superseded and not canonical**.
>
> Platform build handoff work is **paused** until post-realignment rebuild (R2–R3) is complete. The contract, verification, and conformance layers built on the 8+1 model require revalidation before downstream implementation may proceed.

## Purpose

This document tells a coding agent how to use this repository when building a target implementation.

The ASH Pattern System is a **resilient software semantics framework** grounded in the **full 9D ASH research math**. The canonical state space is F2^9 with 512 vertices, XOR-by-codeword transformations, an averaging operator with T²=T, and first-class branching semantics.

## Handoff rule

Treat this repository as the semantic authority.
Do not infer core semantics from convenience, local idiom, or language defaults.
The **research baseline** is canonical — not the superseded 8+1 drift formalization.

## Required coding-agent workflow

1. read `README.md`
2. read all files in `docs/`
3. read all files in `specs/core/`, paying particular attention to:
   - `ash-state-space.pseudo.md` — the canonical state definition
   - `control-bit-derivation.pseudo.md` — the control-bit derivation spec and its closure status
   - `core-admissibility.pseudo.md` — the admissibility rules and their closure status
   - `state-validity-diagnostics.pseudo.md` — the canonical diagnostic model
   - `system-state-classification.pseudo.md` — canonical system-state classes and class-to-action mapping
   - `recoverability-semantics.pseudo.md` — recoverability categories and recovery mapping
4. read all files in `specs/algorithms/`, paying particular attention to:
   - `recovery-fallback-semantics.pseudo.md` — deterministic recovery and fallback selection
   - `containment-safe-failure-semantics.pseudo.md` — containment and safe-failure behavior
5. read `specs/interfaces/semantic-contracts.md` and all files in `specs/interfaces/contracts/`
6. read `governance/repository-governance.md`
7. confirm that the locked algebraic definitions (parity formula, 16-codeword set) match the canonical specs
8. check for any remaining unresolved items (e.g., fallback-policy registry)
9. only then begin target-specific design and implementation planning

## What the coding agent must preserve

The coding agent must preserve:

- the ASH state space as **F2^9**
- the special role of the first 8 coordinates as the stabilizing algebraic core
- the special role of the 9th coordinate as a derived control/parity dimension
- deterministic normalization
- deterministic realm identity
- deterministic transition behavior
- deterministic topology expansion
- full axiom diagnostics
- explicit separation between generation planning and materialization
- system-state classification (all 7 canonical classes)
- deterministic recoverability mapping (class-to-recovery-category)
- deterministic recovery and fallback behavior
- containment and safe-failure behavior
- the corrected-core derivation rule (expected control from corrected admissible core for correctable states)
- diagnosable recovery — no silent healing

## What the coding agent must not do

The coding agent must not:

- reinterpret the 9th coordinate as a normal peer bit for ordinary transitions
- silently collapse the core/control distinction
- make one platform's file structure into the engine's identity
- replace semantic planning with direct side effects
- treat convenience behavior as canonical if the specs do not say so
- substitute a different control-bit derivation formula for the locked parity formula
- substitute a different admissibility codeword set for the locked normative 16-codeword set
- treat the locked algebraic definitions as configurable, optional, or open to local variation
- guess any foundational semantic that is explicitly marked as unresolved in this repository (e.g., the fallback-policy registry)
- silently heal or mutate state without producing a diagnostic record
- skip containment when the recovery/fallback specifications require it
- allow a `FAILED` state to continue normal operations without escalation
- allow transitions from `SAFE_HALT` to any other state

## Canonical design decisions (Research Baseline — R1)

The following are canonical after Research Math Realignment R1:

- **State space** — full F2^9, 512 vertices, 9-bit states
- **Transformation** — XOR-by-codeword: `x' = x ⊕ c` where `c ∈ C ⊂ F2^9`
- **Averaging operator** — `T f(x) = (1/|C|) Σ f(x ⊕ c)` with `T² = T`
- **Branching** — first-class canonical capability
- **No derived 9th bit** — the 9th coordinate is not canonically a derived control/parity bit computed from the first 8

## Superseded design decisions (8+1 formalization)

The following are **superseded and not canonical** after R1:

- Control-bit derivation formula (`b8 = b0 ⊕ ... ⊕ b7`) — superseded
- 8-bit [8,4,4] core admissibility law — superseded
- Corrected-core derivation rule — superseded
- All contracts, verification, and conformance layers built on the 8+1 model — **pending revalidation**

The coding agent must not treat the superseded 8+1 formalization as authoritative for implementation.

## Layers pending revalidation

The following layers require research-baseline revalidation before they can be used as authority:

- Contract layer (`specs/interfaces/semantic-contracts.md` and `specs/interfaces/contracts/`)
- Verification layer (`specs/verification/`)
- Recovery/fallback/containment semantics built on the 8+1 model
- Diagnostic schema and rule-ID taxonomy (structural concepts may survive, but 8+1-specific invariants must be revalidated)

Platform build handoff work is **paused** until R2–R3 revalidation is complete.

## Design package status

- **Design Package A** — complete, then **partially superseded by R1**
- **Design Package B** — formally closed, then **partially superseded by R1**
- **Design Package C** — complete, then **superseded by R1** (algebraic locks based on 8+1 model)
- **Design Package D** — complete, **pending revalidation after R1**
- **Phase 2** — complete, **pending revalidation after R1**
- **Phase 3** — complete, **pending revalidation after R1**
- **R1 — Foundational Math Reset** — complete (current)

## Required delivery shape for implementation repos

A downstream implementation handoff should include, at minimum:

- mapping from spec modules to implementation modules
- invariant-based test plan
- materialization boundary design
- diagnostics design
- target-runtime constraints
- packaging and build decisions for that target repo
