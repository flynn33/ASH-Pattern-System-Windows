# Verification Plan — windows-cpp

Maps each of the 28 canonical invariants from `specs/verification/invariant-spec.md` on main to a test case on this branch, plus a section mapping the 5 canonical conformance categories to the test files that cover them.

## Authority boundary

The canonical invariant definitions and acceptance criteria are authoritative on main. This document is a mapping. If the canonical invariant spec changes, this document must be re-aligned; it does not redefine invariants.

## Invariant-by-invariant coverage

Status values:
- **Covered** — a test case in this branch dynamically verifies the invariant
- **Covered (architectural)** — the invariant is enforced at the header signature level in this branch; runtime verification is pending
- **Deferred (non-slice)** — the invariant applies to a non-slice module that currently ships as a stub; the test is owed in a future expansion pass

| Invariant ID | One-line meaning | Test file | Test case name | Status |
|---|---|---|---|---|
| INV-STATE-001 | State uses all 9 coordinates in F2^9 | `StateModel.test.cpp` | `classify_admissibility_is_total_over_512_states` | Covered |
| INV-STATE-002 | Normalization is deterministic | `StateModel.test.cpp` | `normalize_is_deterministic` | Covered |
| INV-STATE-003 | Normalization fails with diagnostic; no silent result | `StateModel.test.cpp` | `normalize_always_produces_a_diagnostic` | Covered |
| INV-CODEWORD-001 | x' = x XOR c on all 9 bits | `TransitionRegistry.test.cpp` | `apply_transition_operates_on_full_9_bits` | Covered |
| INV-CODEWORD-002 | Transformations are deterministic and pure | `TransitionRegistry.test.cpp` | `apply_transition_is_deterministic` | Covered |
| INV-CODEWORD-003 | Involution: (x XOR c) XOR c = x | `TransitionRegistry.test.cpp` | `apply_transition_is_involutive` | Covered |
| INV-CODEWORD-004 | Codeword set matches research baseline [9,4,4] | `TransitionRegistry.test.cpp` | `codeword_set_has_exactly_16_members`, `codeword_set_is_closed_under_xor`, `codeword_set_weights_are_in_0_4_8` | Covered |
| INV-ADMISSIBILITY-001 | Admissibility classification is deterministic | `StateModel.test.cpp` | `classify_admissibility_is_deterministic` | Covered |
| INV-ADMISSIBILITY-002 | Every 9-bit vector maps to exactly one status | `StateModel.test.cpp` | `classify_admissibility_is_total_over_512_states` | Covered |
| INV-REALM-001 | Realm encoding is deterministic | — | — | Deferred (non-slice) |
| INV-REALM-002 | Realm encoding operates only on valid states | — | — | Deferred (non-slice) |
| INV-TRANS-001 | Equal state + transition = equal output | `TransitionRegistry.test.cpp` | `apply_transition_is_deterministic`, `resolve_transition_by_id_is_deterministic` | Covered |
| INV-TRANS-002 | Transitions operate on full 9-bit states via XOR | `TransitionRegistry.test.cpp` | `apply_transition_operates_on_full_9_bits` | Covered |
| INV-TOPO-001 | Topology generation is deterministic with stable ordering | — | — | Deferred (non-slice) |
| INV-AXIOM-001 | Every axiom evaluation produces a diagnostic | — | — | Deferred (non-slice) |
| INV-RECOVERY-001 | Recovery classification is deterministic | `RecoveryEngine.test.cpp`, `StateModel.test.cpp` | `execute_normalize_state_is_deterministic`, `classify_recoverability_maps_all_7_classes` | Covered |
| INV-RECOVERY-002 | Fallback uses only canonical registry | `RecoveryEngine.test.cpp` | `select_fallback_empty_registry_escalates_to_containment` | Covered |
| INV-RECOVERY-003 | Escalation is monotonic | `RecoveryEngine.test.cpp` | `post_halt_enter_containment_rejected` (post-halt rejection demonstrates monotonic terminal state) | Covered |
| INV-RECOVERY-004 | No silent healing | `RecoveryEngine.test.cpp`, `StateModel.test.cpp` | `execute_normalize_state_produces_diagnostic`, `select_fallback_never_silent`, `normalize_always_produces_a_diagnostic` | Covered |
| INV-RECOVERY-005 | SAFE_HALT is terminal; no transitions out | `RecoveryEngine.test.cpp` | `post_halt_normalize_rejected_with_error_diagnostic`, `post_halt_apply_correction_rejected`, `post_halt_select_fallback_rejected`, `post_halt_enter_containment_rejected` | Covered |
| INV-PLAN-001 | Planning produces no side effects | `GenerationPlanner.hpp` (architectural) | — | Covered (architectural) + Deferred (runtime) |
| INV-PLAN-002 | Plan is self-contained (emitter needs only plan) | `ArtifactEmitter.hpp` (architectural) | — | Covered (architectural) + Deferred (runtime) |
| INV-BOUNDARY-001 | No file/module/service created during planning | `GenerationPlanner.cpp` (architectural: no I/O headers included) | — | Covered (architectural) + Deferred (runtime) |
| INV-BOUNDARY-002 | Every emitted artifact traceable to plan element | `ArtifactEmitter.hpp` (architectural: materialize_plan accepts only plan and config) | — | Covered (architectural) + Deferred (runtime) |
| INV-DIAG-001 | Every diagnostic conforms to envelope schema | `Diagnostics.test.cpp` | `validate_envelope_accepts_well_formed`, `validate_envelope_rejects_empty_subject`, `validate_envelope_rejects_null_chain_root`, builder-field tests | Covered |
| INV-DIAG-002 | Every rule_id conforms to taxonomy pattern | `Diagnostics.test.cpp` | `validate_rule_id_accepts_canonical_examples`, `validate_rule_id_rejects_lowercase`, `validate_rule_id_rejects_short_number`, `validate_rule_id_rejects_missing_family` | Covered |
| INV-DIAG-003 | Chain is complete; references intact | `Diagnostics.test.cpp` | `validate_chain_root_is_self_reference`, `validate_chain_accepts_parent_child_link`, `builder_rejects_escalation_child_of_detection_parent`, `expose_chain_returns_ordered_root_to_leaf` | Covered |
| INV-DIAG-004 | No silent omission | Implicit across slice tests + `apply_transition_emits_diagnostic` | `apply_transition_emits_diagnostic`, `normalize_always_produces_a_diagnostic`, `select_fallback_never_silent` | Covered |

### Coverage summary

- **Covered (dynamic):** 20 invariants
- **Covered (architectural):** 4 invariants (INV-PLAN-001/002, INV-BOUNDARY-001/002) — enforcement is at the header signature level; runtime tests are a future expansion item
- **Deferred (non-slice):** 4 invariants (INV-REALM-001/002, INV-TOPO-001, INV-AXIOM-001) — awaiting non-slice module implementations

## Conformance category coverage

From `specs/verification/conformance-categories.md`:

| # | Category | Covered by |
|---|---|---|
| 1 | Algebraic / State Conformance | `StateModel.test.cpp` + `TransitionRegistry.test.cpp` (codeword set, admissibility, transformation, involution) |
| 2 | Recovery / Fallback / Containment Conformance | `RecoveryEngine.test.cpp` (normalize, correction, empty-registry fallback, containment, safe halt, post-halt rejection) |
| 3 | Diagnostics Conformance | `Diagnostics.test.cpp` (envelope builder, rule-ID taxonomy, chain integrity, stage ordering) |
| 4 | Generation / Materialization-Boundary Conformance | Architectural enforcement in `GenerationPlanner.hpp` + `ArtifactEmitter.hpp`; runtime tests deferred |
| 5 | Contract / Module Conformance | Per-module headers carry canonical contract comments; tests verify the four slice modules' interface behavior |

All 5 categories have at least architectural coverage. Categories 1, 2, and 3 have full runtime test coverage on the slice modules. Categories 4 and 5 have partial runtime coverage and will expand as the non-slice modules are implemented.

## How to run the verification

```powershell
cd windows\build
.\build.ps1 -Test
```

or individually:

```powershell
cd windows\build
.\build.ps1
cd ..\tests
.\run_all.ps1
```

Each test executable exits with the number of failed test cases. A total exit code of 0 indicates the dynamically covered invariants all hold on the current build.
