# ASH State Space — agnostic specification

## Design decision

The ASH state space is **F2^9**.

An ASH state is therefore a 9-coordinate binary state vector:

```text
S = (b0, b1, b2, b3, b4, b5, b6, b7, b8)
where each bi ∈ F2
```

## Structural interpretation

### Coordinates `b0..b7`

The first 8 coordinates form the **stabilizing algebraic core**.

This core is modeled against the structure of the **[8,4,4] extended Hamming code**.
That means the 8-bit layer is the primary substrate for:

- stabilization
- detection
- consistency checking
- structured state interpretation

### Coordinate `b8`

The 9th coordinate is the **derived control/parity dimension**.
Its role is not to behave as an ordinary peer bit under ordinary state evolution.

Its purposes include:

- control-state signaling
- stabilization support
- consistency checking
- self-referential state semantics

## Canonical state record

```text
TYPE AshState
    core_bits: Vector[8] over F2
    control_bit: Bit
END TYPE
```

Equivalent expanded form:

```text
TYPE AshStateExpanded
    b0: Bit
    b1: Bit
    b2: Bit
    b3: Bit
    b4: Bit
    b5: Bit
    b6: Bit
    b7: Bit
    b8: Bit
END TYPE
```

## Validity rule

A valid ASH state must satisfy both conditions:

1. the 8-bit core is structurally admissible under the admissibility rules defined in `core-admissibility.pseudo.md`
2. the control bit is consistent with the derivation rule defined in `control-bit-derivation.pseudo.md`

If either condition fails, the state-validity diagnostic system (defined in `state-validity-diagnostics.pseudo.md`) must be able to explain why.

## Normal form

The preferred semantic normal form is:

```text
AshState = { core_bits, control_bit }
```

This is preferred because it makes the special role of the 9th coordinate explicit.

## Pseudocode

```text
FUNCTION make_state(core_bits[8]) -> AshState
    -- derive_control_bit is defined in control-bit-derivation.pseudo.md
    state.core_bits = core_bits
    state.control_bit = derive_control_bit(core_bits)
    RETURN state
END FUNCTION
```

```text
FUNCTION normalize_state(candidate_state) -> AshState
    -- derive_control_bit is defined in control-bit-derivation.pseudo.md
    -- admissibility must be checked per core-admissibility.pseudo.md
    core = extract_core_bits(candidate_state)
    admissibility = classify_core_admissibility(core)

    IF admissibility == INADMISSIBLE_CORRECTABLE THEN
        core = correct_to_nearest_codeword(core)
    ELSE IF admissibility IN [INADMISSIBLE_DETECTABLE, INADMISSIBLE_UNRECOVERABLE] THEN
        FAIL with diagnostic (see state-validity-diagnostics.pseudo.md)
    END IF

    control = derive_control_bit(core)
    RETURN AshState(core_bits = core, control_bit = control)
END FUNCTION
```

## Prohibition

Ordinary transition logic must not directly mutate the control bit as if it were just another free coordinate.
Any change in the control bit during ordinary evolution must come through re-derivation from the core state.

## Required invariants

1. normalization is deterministic
2. equal 8-bit cores produce equal derived control bits
3. state validity can be explained diagnostically (see `state-validity-diagnostics.pseudo.md`)
4. the 9th coordinate remains semantically distinguished from the first 8
5. normalization must not proceed on inadmissible cores that cannot be corrected (see `core-admissibility.pseudo.md`)

## Related specifications

- `specs/core/control-bit-derivation.pseudo.md` — defines `derive_control_bit`, the function that computes the 9th coordinate from the 8-bit core
- `specs/core/core-admissibility.pseudo.md` — defines `classify_core_admissibility`, the function that determines whether an 8-bit core is a valid codeword under the [8,4,4] extended Hamming code structure
- `specs/core/state-validity-diagnostics.pseudo.md` — defines the diagnostic record produced when state validity is evaluated
