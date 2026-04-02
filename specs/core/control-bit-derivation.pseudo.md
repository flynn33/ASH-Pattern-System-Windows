# Control-Bit Derivation — agnostic specification

## Purpose

The 9th coordinate of the ASH state space (`b8`) is a **derived control/parity dimension**.
It is not a free coordinate. Its value must be deterministically derived from the 8-bit stabilizing core (`b0..b7`).

The control-bit derivation function exists to:

- enforce consistency between the stabilizing core and the full 9-coordinate state
- enable detection of state corruption or unauthorized mutation
- support self-referential state semantics (the state can verify its own integrity)
- provide a foundation for normalization, validity checking, and diagnostic reporting
- prevent downstream implementations from treating the 9th coordinate as an ordinary peer bit

## Inputs

```text
core_bits: Vector[8] over F2
```

The 8-bit stabilizing algebraic core of the ASH state.

### Input preconditions

1. `core_bits` must be exactly 8 elements
2. each element must be in F2 (that is, exactly 0 or 1)
3. the input must be provided — derivation from an absent or partial core is not defined

## Outputs

```text
control_bit: Bit (element of F2)
```

A single derived bit representing the 9th coordinate of the full ASH state.

### Output postconditions

1. the output is deterministic: the same `core_bits` always produce the same `control_bit`
2. the output is in F2 (exactly 0 or 1)
3. equal 8-bit cores produce equal derived control bits

## Invariants

1. **Purity** — derivation has no side effects. It reads the core and returns a bit.
2. **Determinism** — identical inputs always produce identical outputs, regardless of platform, language, runtime, or execution context.
3. **Referential transparency** — the function may be called any number of times with the same input and must return the same output.
4. **Independence from external state** — the derivation must not depend on mutable global state, configuration, randomness, time, or any context outside the input vector.
5. **Algebraic closure** — the derivation operates over F2 and produces a value in F2. No intermediate computation may leave the field.

## Prohibited shortcuts

1. Do not allow any code path to set the control bit directly without going through derivation.
2. Do not allow platform-specific optimization to change the derivation semantics.
3. Do not allow lazy or deferred derivation that could produce a state with an unset control bit.
4. Do not allow a fallback value (such as always returning 0) in place of proper derivation.
5. Do not derive from a subset of the core — all 8 bits must participate as defined by the derivation formula.

## Pseudocode interface shape

```text
FUNCTION derive_control_bit(core_bits[8]) -> Bit
    PRECONDITION: length(core_bits) == 8
    PRECONDITION: all elements of core_bits are in F2

    control_bit = APPLY derivation_formula(core_bits)

    POSTCONDITION: control_bit is in F2
    POSTCONDITION: derivation is deterministic

    RETURN control_bit
END FUNCTION
```

## Diagnostic behavior when derivation cannot proceed

If derivation is invoked with invalid input, the implementation must not silently produce a default value.

The following conditions must produce a diagnosable failure:

| Condition | Required behavior |
|---|---|
| `core_bits` is absent or null | Derivation must fail with a diagnostic indicating missing input |
| `core_bits` has fewer than 8 elements | Derivation must fail with a diagnostic indicating insufficient length |
| `core_bits` has more than 8 elements | Derivation must fail with a diagnostic indicating excess length |
| Any element of `core_bits` is not in F2 | Derivation must fail with a diagnostic indicating domain violation |

In all failure cases, the diagnostic must include:

- the reason derivation could not proceed
- the input as received (or a safe representation of it)
- a reference to this specification

The implementation must not return a control bit when derivation fails.

## Relation to other specifications

- **ash-state-space.pseudo.md** — defines the full 9-coordinate state and references this derivation
- **core-admissibility.pseudo.md** — defines which 8-bit cores are admissible; derivation applies to both admissible and inadmissible cores but only produces a meaningful control bit when the core is admissible
- **state-validity-diagnostics.pseudo.md** — uses the result of derivation to populate the `control_derivation_status` and `expected_control_dimension` fields

---

## Unresolved closure item — derivation formula

> **STATUS: REQUIRED DESIGN DECISION — NOT YET LOCKED**

The exact algebraic or parity function that computes the control bit from the 8-bit core has not been formally selected and locked in this repository.

### What is known

- The derivation formula must be a function `F2^8 -> F2`.
- The 8-bit core is modeled against the structure of the **[8,4,4] extended Hamming code**.
- The derivation is expected to be an algebraic function consistent with that coding-theoretic structure.
- Candidate formula families include:
  - overall parity (XOR of all 8 bits)
  - a specific syndrome-derived function
  - a function tied to the generator or parity-check matrix of the [8,4,4] code

### What is not yet decided

- Which specific function from the candidate family is canonical.
- Whether the derivation formula varies by realm or is universal across all ASH states.
- Whether additional algebraic constraints (beyond parity) apply.

### What downstream implementations must do

Until this closure item is resolved:

1. Implementations must **not invent** a derivation formula.
2. Implementations must structure their code so the derivation formula is a single replaceable point of definition.
3. Implementations must be able to report `unable-to-derive` in diagnostics if the formula is not yet provided.
4. Implementations must not ship a production system that silently uses a placeholder formula without explicit acknowledgment.

### Required resolution path

This closure item must be resolved by an explicit design decision recorded in this file.
When the formula is locked, this section must be replaced with the formula definition, its justification, and its relation to the [8,4,4] code structure.
