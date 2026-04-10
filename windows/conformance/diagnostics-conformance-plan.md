# Diagnostics Conformance Plan — windows-cpp

Documents how the windows-cpp branch conforms to the canonical diagnostic schema and rule-ID taxonomy defined in:
- `specs/interfaces/diagnostic-schema.md` on main
- `specs/interfaces/rule-id-taxonomy.md` on main
- `specs/interfaces/contracts/diagnostics-module-contract.md` on main

## Authority boundary

The canonical schema and taxonomy are authoritative. This branch conforms; it does not redefine. Any divergence is a bug to be fixed on this branch, not a proposal to change the canonical spec.

## Envelope schema conformance

Every diagnostic produced by any module on this branch is a `ash::DiagnosticEnvelope` constructed via `ash::DiagnosticEnvelopeBuilder`. The envelope type in `windows/src/include/ash/DiagnosticEnvelope.hpp` mirrors the canonical schema field-for-field:

| Canonical field | Windows type |
|---|---|
| `diagnostic_kind` | `DiagnosticKind` enum: `STATE_VALIDITY`, `RECOVERY`, `FALLBACK`, `CONTAINMENT`, `SAFE_HALT` |
| `severity` | `Severity` enum: `INFO`, `WARNING`, `ERROR`, `CRITICAL` |
| `stage` | `Stage` enum: `DETECTION`, `CLASSIFICATION`, `RECOVERY`, `ESCALATION`, `TERMINAL` |
| `disposition` | `Disposition` enum: `RESOLVED`, `PENDING`, `BLOCKED`, `ESCALATED`, `TERMINAL` |
| `subject_reference` | `std::string` (non-empty required) |
| `parent_diagnostic_reference` | `std::shared_ptr<DiagnosticEnvelope>` (nullable for root only) |
| `chain_root_reference` | `std::shared_ptr<DiagnosticEnvelope>` (never null; root points to self) |
| `rule_ids` | `std::vector<std::string>` (≥1 required, each taxonomy-compliant) |
| `summary` | `std::string` (non-empty required) |
| `notes` | `std::vector<std::string>` (≥1 non-empty required) |

## Builder discipline

`DiagnosticEnvelopeBuilder::build_root()` and `build_child()` both call `validate_required_fields_()` before allocating the envelope. If any required field is missing or empty, the builder throws `std::logic_error` and no envelope is returned.

Tests: `Diagnostics.test.cpp` covers:
- `builder_accepts_complete_envelope` — happy path
- `builder_rejects_missing_kind`
- `builder_rejects_missing_summary`
- `builder_rejects_missing_notes`
- `builder_rejects_missing_rule_ids`

## Rule-ID taxonomy conformance

Every rule ID used in this branch matches the canonical pattern `{FAMILY}-{CATEGORY}-{NUMBER}` where:
- `FAMILY` is a non-empty sequence of uppercase letters and dashes, starting with a letter
- `CATEGORY` is a non-empty sequence of uppercase letters and dashes, starting with a letter
- `NUMBER` is a 3-digit zero-padded integer

`Diagnostics::validate_rule_id()` implements this pattern as a manual parse (no stdlib regex) for speed and compile-time simplicity. Tests:
- `validate_rule_id_accepts_canonical_examples`
- `validate_rule_id_rejects_lowercase`
- `validate_rule_id_rejects_short_number`
- `validate_rule_id_rejects_missing_family`

### Rule IDs declared in this branch

Defined in `windows/src/include/ash/RuleIds.hpp`:

```
ASH-STATE-VALIDITY-001     (INV-STATE-001 equivalent)
ASH-STATE-VALIDITY-002     (INV-STATE-002 equivalent)
ASH-STATE-VALIDITY-003     (INV-STATE-003 equivalent)
ASH-ADMISSIBILITY-001
ASH-ADMISSIBILITY-002
ASH-CLASSIFICATION-001
ASH-CODEWORD-001           (XOR-by-codeword rule)
ASH-CODEWORD-002           (determinism/purity)
ASH-CODEWORD-003           (involution)
ASH-CODEWORD-004           (canonical set membership)
ASH-TRANS-001
ASH-TRANS-002
ASH-RECOVERY-001 .. 005
ASH-FALLBACK-001
ASH-CONTAINMENT-001
ASH-HALT-001
ASH-DIAG-001 .. 004
ASH-WINDOWS-STUB-001       (branch-local; see deviation-log.md item 4)
```

The `ASH-WINDOWS-STUB-*` family is a branch-local extension for "not yet implemented" diagnostics emitted by non-slice module stubs. It is taxonomy-compliant (matches the canonical pattern) but is not defined in the canonical rule-ID taxonomy on main. The extension is documented in `deviation-log.md` item 4 and will be removed when the non-slice modules receive real implementations.

## Chain integrity

The canonical schema requires:
- Every diagnostic's `chain_root_reference` is non-null
- Every non-root diagnostic's `parent_diagnostic_reference` is non-null
- Stage ordering: ESCALATION child requires parent stage ∈ {RECOVERY, CLASSIFICATION}; TERMINAL child requires parent stage ∈ {ESCALATION, RECOVERY}
- The chain is reconstructable by walking `parent_diagnostic_reference` links back to the root

`DiagnosticEnvelopeBuilder::build_child()` enforces the stage ordering at construction time (throws `std::logic_error` on violation). `Diagnostics::validate_chain()` re-verifies the chain at runtime by walking parent links back to the root and checking `chain_root_reference` consistency at every step.

Tests:
- `validate_chain_root_is_self_reference`
- `validate_chain_accepts_parent_child_link`
- `builder_rejects_escalation_child_of_detection_parent`
- `expose_chain_returns_ordered_root_to_leaf`

## No silent omission

The canonical contract prohibits silent diagnostics: every module operation that produces or consumes state must also produce a diagnostic. On this branch:

- `StateModel::normalize` always returns a diagnostic (tested in `normalize_always_produces_a_diagnostic`)
- `TransitionRegistry::apply_transition` always returns a diagnostic (tested in `apply_transition_emits_diagnostic`)
- `RecoveryEngine::execute_normalize_state`, `execute_apply_correction`, `select_fallback` all always return diagnostics (tested in `execute_normalize_state_produces_diagnostic` and `select_fallback_never_silent`)
- Post-SAFE_HALT rejection paths also return diagnostics; no method silently no-ops

## Post-halt behavior

`RecoveryEngine` transitions to a terminal state on `enter_safe_halt`. After halt, `execute_normalize_state`, `execute_apply_correction`, `select_fallback`, and `enter_containment` all return an ERROR diagnostic citing `ASH-RECOVERY-005` (SAFE_HALT terminal rule) and `ASH-HALT-001` (canonical halt entry). No method silently accepts post-halt input. Tests in `RecoveryEngine.test.cpp` verify each rejection path.

## Open items

- Non-slice modules (`RealmEncoder`, `TopologyGenerator`, `AxiomEvaluator`, `GenerationPlanner`, `ArtifactEmitter`) emit NOT_IMPLEMENTED diagnostics that are schema-conformant but structurally minimal. When these modules receive real implementations, the diagnostic output must be expanded to reflect real recovery/emission chains.
- `AxiomEvaluator::evaluate_axioms` currently returns a single NOT_IMPLEMENTED diagnostic covering the full axiom list rather than one diagnostic per axiom. When the module is implemented, it must produce one diagnostic per axiom per canonical contract (`INV-AXIOM-001`).
- Runtime verification of `INV-PLAN-001` and `INV-BOUNDARY-001` (planner side-effect freedom) is architectural on this branch (enforced by refusal to include I/O headers in `GenerationPlanner.cpp` and by the signature of `generate_plan`). Runtime verification via instrumentation is a future expansion item.
