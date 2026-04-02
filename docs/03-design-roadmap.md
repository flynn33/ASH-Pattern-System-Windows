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

- **control-bit derivation** — define the semantic role, interface, invariants, and diagnostic behavior of the derived control dimension; lock the exact derivation formula (currently an unresolved closure item)
- **core admissibility** — define which 8-bit core vectors are structurally admissible under the [8,4,4] extended Hamming code; lock the exact codeword set / generator matrix (currently an unresolved closure item)
- **state-validity diagnostics** — define a canonical diagnostic record that every implementation must produce for any candidate state

Specifications created for this phase:

- `specs/core/control-bit-derivation.pseudo.md`
- `specs/core/core-admissibility.pseudo.md`
- `specs/core/state-validity-diagnostics.pseudo.md`

**Status**: Specification structure complete. Two unresolved closure items remain:

1. the exact derivation formula for `derive_control_bit`
2. the exact codeword set for core admissibility

These must be resolved before Phase 2 can be considered implementation-ready.

### Phase 2 — lock the implementation contracts

Define the exact capabilities that any implementation must expose, including:

- state normalization
- control derivation
- transition application
- topology generation
- axiom diagnostics
- generation planning
- artifact materialization boundary

### Phase 3 — define invariant-based verification requirements

Describe what downstream test suites must prove, including:

- deterministic normalization
- deterministic realm encoding
- no uncontrolled mutation of the control dimension
- deterministic transition outcomes
- deterministic topology expansion
- stable axiom diagnostics
- stable generation-plan structure

### Phase 4 — create platform build handoff packages

For each target implementation repo, the coding agent should receive:

- this repository
- target platform constraints
- target language constraints
- runtime constraints
- packaging constraints
- performance constraints if applicable

## Immediate next design step

The current design milestone is the **formal closure of Phase 1.5**:

1. **Lock the control-bit derivation formula** — select and record the exact algebraic function `F2^8 -> F2` in `specs/core/control-bit-derivation.pseudo.md`
2. **Lock the core admissibility law** — select and record the exact generator matrix and codeword enumeration for the [8,4,4] extended Hamming code in `specs/core/core-admissibility.pseudo.md`
3. **Verify diagnostic completeness** — confirm that `specs/core/state-validity-diagnostics.pseudo.md` can produce a full diagnostic for every state once the above two items are locked

Once these closure items are resolved, the repository can proceed to Phase 2 (locking implementation contracts) with confidence that downstream repositories will not need to guess foundational semantics.
