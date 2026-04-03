# Design Roadmap

## Goal

Move from the ASH research math baseline to implementation-ready handoff materials without collapsing the design into any single language or platform.

> **Research Math Realignment R1**: The foundational math has been reset to the full 9D ASH research baseline. Phases and packages built on the superseded 8+1 formalization are noted as such below. Later layers require revalidation before platform handoff can proceed.

## Pre-realignment history

### Phase 1 through Phase 3 (superseded foundation)

The following phases were completed under the original 8+1 formalization. Their structural contributions (resilient software semantics, diagnostic schemas, contract framework, verification framework) remain valuable, but their **mathematical assumptions** (8-bit core + derived 9th control bit, [8,4,4] admissibility, parity formula) are **superseded** by the research math realignment.

- **Phase 1** — semantic foundation (state space, realm identity, transitions, topology, axioms, generation planning)
- **Phase 1.5 / Design Package A** — formal state layers (control-bit derivation, core admissibility, state-validity diagnostics) — **mathematical basis superseded by R1**
- **Phase 1.75 / Design Package B** — resilient software semantics (system-state classification, recoverability, recovery/fallback, containment/safe-failure) — **pending revalidation**
- **Phase 1.85 / Design Package C** — algebraic locks (parity formula, [8,4,4] codeword set) — **superseded by R1**
- **Phase 1.9 / Design Package D** — registry and diagnostics layer (fallback-policy registry, diagnostic schema, rule-ID taxonomy) — **structural concepts preserved, specific invariants pending revalidation**
- **Phase 2** — implementation contracts (9 module contracts, materialization boundary) — **pending revalidation**
- **Phase 3** — invariant-based verification requirements — **pending revalidation**

## Research math realignment sequence

### R1 — Foundational Math Reset (current)

Reset the canonical mathematical foundation to the full 9D ASH research baseline:

- **State space** — full F2^9 with 512 vertices/realms
- **Transformation** — XOR-by-codeword: `x' = x ⊕ c` for `c ∈ C ⊂ F2^9`
- **Averaging operator** — `T f(x) = (1/|C|) Σ f(x ⊕ c)` with `T² = T`
- **Branching** — first-class canonical capability

De-authorize the 8+1 drift formalization. Suspend later-layer authority pending revalidation.

New specifications created:

- `specs/algorithms/codeword-transformation-semantics.pseudo.md`
- `specs/algorithms/averaging-operator-semantics.pseudo.md`
- `specs/algorithms/branching-semantics.pseudo.md`

Rewritten to research baseline:

- `specs/core/ash-state-space.pseudo.md`
- `specs/core/realm-identity.pseudo.md`
- `specs/algorithms/transition-system.pseudo.md`

**Status**: Complete.

### R2 — Rewrite state/recovery semantics against restored 9D baseline

Rewrite the state-layer and resilient-software semantics to operate on the full 9D model:

- Define validity and admissibility for full 9-bit states under the research codeword structure
- Rewrite state-validity diagnostics for the 9D model
- Rewrite system-state classification, recoverability, recovery/fallback/containment against the 9D baseline
- Formalize the specific codeword set `C ⊂ F2^9` from the research baseline

**Status**: Not started. This is the immediate next phase.

### R3 — Rebuild contracts and verification after math realignment

Rebuild the contract and verification layers on the revalidated 9D foundation:

- Revalidate or rewrite all 9 module contracts against the 9D model
- Revalidate or rewrite the diagnostic schema and rule-ID taxonomy
- Revalidate or rewrite the invariant specification, conformance categories, and acceptance criteria
- Confirm that the materialization boundary, fallback-policy registry, and other structural contributions from the original phases remain valid or are updated

**Status**: Not started. Blocked until R2 is complete.

### Phase 4 — create platform build handoff packages

For each target implementation repo, the coding agent should receive:

- this repository (post-R3, fully revalidated)
- target platform constraints
- target language constraints
- runtime constraints
- packaging constraints
- performance constraints if applicable

**Status**: Paused. Platform build handoff work cannot proceed until R1–R3 are complete and the repository is fully revalidated against the research baseline.

## Immediate next design step

R1 is complete. The immediate next step is **R2 — rewrite state/recovery semantics against the restored 9D baseline**.

R2 will define validity, admissibility, diagnostics, classification, and recovery semantics for the full 9-bit state model, grounded in the research codeword structure rather than the superseded 8+1 formalization.
