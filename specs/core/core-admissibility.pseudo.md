# Core Admissibility — agnostic specification

## Purpose

The admissibility layer defines **which 8-bit core vectors are structurally valid** within the ASH state space.

Not all 256 possible vectors in F2^8 are meaningful ASH core states. The 8-bit stabilizing core is modeled against the **[8,4,4] extended Hamming code**, which imposes algebraic structure on the space. Admissibility determines whether a given core vector belongs to the structured subset or lies outside it.

Admissibility is the foundation for:

- **normalization** — only admissible cores can be normalized into valid ASH states
- **state validity** — a state cannot be valid if its core is inadmissible (unless recovery succeeds)
- **self-correction** — the system's ability to detect and recover from corrupted states
- **safe-failure behavior** — the system's ability to recognize unrecoverable states and fail safely
- **fallback behavior** — the system's ability to choose a known-good state when recovery is not possible
- **deterministic recovery planning** — the system's ability to plan recovery paths based on distance from admissible states

## Admissibility concept

### The structured subset

The [8,4,4] extended Hamming code defines a **16-element subset** of F2^8 (the codewords). These codewords have minimum Hamming distance 4 from each other, which means:

- **1-bit errors** are always detectable
- **2-bit errors** are always detectable
- **1-bit errors** are correctable (the corrupted vector is distance 1 from exactly one codeword)
- **3-bit errors** are detectable but not reliably correctable

### Admissibility classification

An 8-bit core vector falls into exactly one of these categories:

```text
ENUM AdmissibilityStatus
    ADMISSIBLE
    INADMISSIBLE_CORRECTABLE
    INADMISSIBLE_DETECTABLE
    INADMISSIBLE_UNRECOVERABLE
END ENUM
```

| Classification | Meaning | Hamming distance from nearest codeword | Recovery path |
|---|---|---|---|
| `ADMISSIBLE` | The core is a valid codeword | 0 | None needed |
| `INADMISSIBLE_CORRECTABLE` | The core is not a codeword but is within correctable distance | 1 | Correction to nearest codeword |
| `INADMISSIBLE_DETECTABLE` | The core is not a codeword, error is detected but correction is ambiguous | 2 | Fallback required (cannot correct with certainty) |
| `INADMISSIBLE_UNRECOVERABLE` | The core is not a codeword and is beyond reliable detection/correction | 3+ | Safe failure required |

## State classifications (full 9-coordinate state)

When the admissibility of the core is combined with the status of the derived control bit, the full ASH state falls into one of these categories:

| Core admissibility | Control bit status | Full state classification | System behavior |
|---|---|---|---|
| `ADMISSIBLE` | Correctly derived | **Stable** | Normal operation |
| `ADMISSIBLE` | Incorrect or missing | **Unstable** | Recoverable via control-bit re-derivation |
| `INADMISSIBLE_CORRECTABLE` | Any | **Correctable** | Recover core to nearest codeword, then re-derive control |
| `INADMISSIBLE_DETECTABLE` | Any | **Degraded** | Error detected; fallback to known-good state required |
| `INADMISSIBLE_UNRECOVERABLE` | Any | **Failed** | Safe failure; no reliable correction path |

## Relation to normalization

Normalization requires:

1. The core must be admissible (or corrected to an admissible state).
2. The control bit must be derived from the (possibly corrected) admissible core.

If the core is inadmissible and uncorrectable, normalization **cannot proceed**. The system must report a diagnostic failure rather than produce a silently invalid normalized state.

```text
FUNCTION normalize_requires_admissibility(core_bits[8]) -> NormalizationPrecondition
    status = classify_core_admissibility(core_bits)

    IF status == ADMISSIBLE THEN
        RETURN { can_normalize: TRUE, core: core_bits }

    ELSE IF status == INADMISSIBLE_CORRECTABLE THEN
        corrected_core = correct_to_nearest_codeword(core_bits)
        RETURN { can_normalize: TRUE, core: corrected_core, correction_applied: TRUE }

    ELSE
        RETURN { can_normalize: FALSE, reason: status }
    END IF
END FUNCTION
```

## Relation to state validity

A state is **valid** if and only if:

1. its 8-bit core is admissible (classification = `ADMISSIBLE`)
2. its control bit matches the value produced by `derive_control_bit(core_bits)`

A state may be **recoverable** even if not currently valid:

- if the core is `INADMISSIBLE_CORRECTABLE`, correction restores admissibility
- if the control bit is wrong but the core is admissible, re-derivation restores consistency

## Relation to self-correction and fallback

The admissibility classification directly determines the recovery strategy:

| Classification | Recovery strategy |
|---|---|
| **Stable** | No action required |
| **Unstable** | Re-derive control bit from admissible core |
| **Correctable** | Correct core to nearest codeword, then re-derive control bit |
| **Degraded** | Cannot correct with certainty; fall back to a known-good state from the fallback registry |
| **Failed** | Cannot recover; invoke safe-failure protocol |

This mapping must be deterministic. The same admissibility status must always lead to the same category of recovery behavior.

## Invariants

1. **Determinism** — `classify_core_admissibility` always returns the same result for the same input.
2. **Completeness** — every vector in F2^8 maps to exactly one admissibility classification.
3. **Consistency with code structure** — the classification must be consistent with the [8,4,4] extended Hamming code distance properties.
4. **No silent acceptance** — an inadmissible core must never be silently treated as admissible.
5. **Purity** — classification has no side effects.

## Pseudocode interface shape

```text
FUNCTION classify_core_admissibility(core_bits[8]) -> AdmissibilityStatus
    PRECONDITION: length(core_bits) == 8
    PRECONDITION: all elements of core_bits are in F2

    distance = hamming_distance_to_nearest_codeword(core_bits)

    IF distance == 0 THEN
        RETURN ADMISSIBLE
    ELSE IF distance == 1 THEN
        RETURN INADMISSIBLE_CORRECTABLE
    ELSE IF distance == 2 THEN
        RETURN INADMISSIBLE_DETECTABLE
    ELSE
        RETURN INADMISSIBLE_UNRECOVERABLE
    END IF
END FUNCTION
```

```text
FUNCTION correct_to_nearest_codeword(core_bits[8]) -> Vector[8] over F2
    PRECONDITION: classify_core_admissibility(core_bits) == INADMISSIBLE_CORRECTABLE

    nearest = find_unique_nearest_codeword(core_bits)
    RETURN nearest
END FUNCTION
```

## Diagnostics boundary

When admissibility classification results in anything other than `ADMISSIBLE`, the diagnostic system must be able to report:

- the input core as received
- the classification result
- the Hamming distance to the nearest codeword (if computable)
- the nearest codeword (if unique and computable)
- whether correction was applied
- which specification rules were evaluated

This information feeds into the `StateValidityDiagnostic` record defined in `state-validity-diagnostics.pseudo.md`.

## Relation to other specifications

- **ash-state-space.pseudo.md** — defines the state structure; references admissibility in its validity rule
- **control-bit-derivation.pseudo.md** — defines the derivation that applies after admissibility is confirmed
- **state-validity-diagnostics.pseudo.md** — consumes admissibility classification to populate diagnostic records

---

## Unresolved closure item — admissibility law

> **STATUS: REQUIRED DESIGN DECISION — NOT YET LOCKED**

The exact set of admissible codewords has not been formally enumerated and locked in this repository.

### What is known

- The admissible set is the set of codewords of the **[8,4,4] extended Hamming code**.
- This code has exactly **16 codewords** in F2^8.
- The minimum Hamming distance between any two codewords is **4**.
- The code is fully characterized by its generator matrix or parity-check matrix.

### What is not yet decided

- The specific generator matrix (or equivalently, the specific parity-check matrix) that defines the canonical codeword set for the ASH Pattern System.
- Whether the standard textbook form of the [8,4,4] code is adopted as-is, or whether a specific permutation/variant is chosen for ASH-specific semantic reasons.
- The explicit enumeration of all 16 codewords.

### What downstream implementations must do

Until this closure item is resolved:

1. Implementations must **not invent** a codeword set.
2. Implementations must structure their code so the codeword set (or generator/parity-check matrix) is a single replaceable point of definition.
3. Implementations must be able to report `admissibility-law-not-provided` in diagnostics if the codeword set is not yet defined.
4. Implementations must not ship a production system that silently uses a placeholder codeword set.

### Required resolution path

This closure item must be resolved by an explicit design decision recorded in this file.
When the codeword set is locked, this section must be replaced with:

- the generator matrix (or parity-check matrix)
- the full enumeration of all 16 codewords
- the justification for the chosen form
- the distance distribution of the code
