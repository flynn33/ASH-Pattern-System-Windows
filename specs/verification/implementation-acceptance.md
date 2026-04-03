# Implementation Acceptance — PENDING REVALIDATION

> **PENDING REVALIDATION — Research Math Realignment Package R1**
>
> This acceptance specification was built on the **superseded 8+1 drift formalization**. The acceptance criteria, non-waivable requirements, and judgment language defined here require **research-baseline revalidation** before they can be treated as authoritative.
>
> No downstream implementation may claim conformance or receive a CONFORMANT judgment based on these criteria until they are revalidated against the restored 9D research baseline. Revalidation is planned for realignment packages R2–R3.

---

# Implementation Acceptance — verification requirements (pending revalidation)

## Purpose

This specification defines the **minimum acceptance threshold** for a downstream implementation repository to be considered conformant with the ASH Pattern System.

This specification is **canonical** (Phase 3). No downstream implementation may claim conformance without satisfying these requirements.

---

## Minimum acceptance conditions

A downstream implementation is **accepted as conformant** if and only if all of the following conditions are met:

1. **All invariants pass** — every invariant defined in `invariant-spec.md` must be verified and must pass. No invariant may be skipped.

2. **All conformance categories are covered** — every category defined in `conformance-categories.md` must be represented in the test suite. No category may be omitted entirely.

3. **Locked semantics are preserved exactly** — the locked algebraic definitions (parity formula, [8,4,4] codeword set), corrected-core derivation rule, materialization boundary, fallback-policy registry, diagnostic schema, and rule-ID taxonomy must be implemented exactly as specified. No substitutions, approximations, or local overrides.

4. **Per-module contracts are satisfied** — every module contract defined in `specs/interfaces/contracts/` must be satisfied. Module behavior must match the detailed contract, not just the umbrella summary.

5. **Diagnostics are complete** — the diagnostic chain from state-validity detection through terminal safe halt must be complete, auditable, and conformant with the shared schema and taxonomy.

---

## Failure conditions

A downstream implementation is **non-conformant** if any of the following conditions are true:

1. **Any invariant fails** — a single failing invariant for a locked semantic renders the implementation non-conformant for the associated conformance category.

2. **Any conformance category is missing** — if the test suite does not cover one or more conformance categories, the implementation cannot be assessed and is treated as non-conformant.

3. **A locked semantic is substituted** — if the implementation uses a different derivation formula, codeword set, fallback policy, diagnostic schema, or rule-ID taxonomy than the canonical specification, it is non-conformant regardless of test results.

4. **The materialization boundary is violated** — if the planner emits artifacts or the emitter invents semantics, the implementation is non-conformant for the Generation/Materialization-Boundary category.

5. **Diagnostics are incomplete** — if any step in the recovery/escalation chain fails to produce a diagnostic, or if diagnostics do not conform to the schema/taxonomy, the implementation is non-conformant for the Diagnostics category.

6. **Silent healing occurs** — if recovery actions mutate state without producing diagnostic records, the implementation is non-conformant for the Recovery/Fallback/Containment category.

---

## Non-waivable locked requirements

The following locked requirements **may not be waived, deferred, or locally overridden** by any downstream implementation:

| Requirement | Source | Locked since |
|---|---|---|
| Control-bit derivation formula (overall parity) | `control-bit-derivation.pseudo.md` | Design Package C |
| Core admissibility law (normative 16-codeword set) | `core-admissibility.pseudo.md` | Design Package C |
| Corrected-core derivation rule | `ash-state-space.pseudo.md` | Design Package B |
| Materialization boundary (planner/emitter separation) | `generation-planner-contract.md`, `artifact-emitter-contract.md` | Phase 2 |
| Fallback-policy registry conformance | `fallback-policy-registry.md` | Design Package D |
| Diagnostic schema conformance | `diagnostic-schema.md` | Design Package D |
| Rule-ID taxonomy conformance | `rule-id-taxonomy.md` | Design Package D |
| SAFE_HALT terminal finality | `containment-safe-failure-semantics.pseudo.md` | Design Package B |
| Monotonic escalation | `recoverability-semantics.pseudo.md` | Design Package B |

These requirements are the semantic foundation of the ASH Pattern System. Waiving any of them would compromise the system's guarantees for self-healing, self-correcting, safe-failure, and fallback behavior.

---

## Acceptance judgment language

### CONFORMANT

An implementation receives the judgment **CONFORMANT** when:

- All invariants in `invariant-spec.md` pass
- All 5 conformance categories in `conformance-categories.md` are covered and passing
- All non-waivable locked requirements are satisfied exactly
- All per-module contracts are satisfied
- The diagnostic chain is complete and conformant

### NON-CONFORMANT

An implementation receives the judgment **NON-CONFORMANT** when any acceptance condition is not met.

A non-conformant judgment must include:

- A list of failing invariants (by ID) with the reason for failure
- A list of failing conformance categories
- A list of non-waivable requirements that were violated
- Recommendations for remediation

### PARTIAL — not a valid judgment

There is no "partial conformance" judgment. An implementation is either **CONFORMANT** or **NON-CONFORMANT**. Partial coverage may be useful for development progress tracking, but it does not constitute a conformance claim.

---

## Relation to other specifications

- **invariant-spec.md** — defines the canonical invariant set that must pass
- **conformance-categories.md** — defines the 5 verification buckets that must be covered
- **semantic-contracts.md** — umbrella contract document; Phase 3 verifies its requirements
- **specs/interfaces/contracts/** — detailed module contracts; Phase 3 verifies their satisfaction
- **governance/ai-coding-handoff.md** — instructs downstream coding agents that conformance requires Phase 3 satisfaction
