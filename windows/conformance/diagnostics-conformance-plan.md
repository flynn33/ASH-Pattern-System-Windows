# Diagnostics Conformance Plan — APS Windows

Diagnostics are represented by `ash::DiagnosticEnvelope` and constructed through `ash::DiagnosticEnvelopeBuilder`.

## Envelope Fields

| Field | Windows type |
|---|---|
| `diagnostic_kind` | `DiagnosticKind` enum |
| `severity` | `Severity` enum |
| `stage` | `Stage` enum |
| `disposition` | `Disposition` enum |
| `subject_reference` | `std::string` |
| `parent_diagnostic_reference` | `std::shared_ptr<DiagnosticEnvelope>` |
| `chain_root_reference` | `std::shared_ptr<DiagnosticEnvelope>` |
| `rule_ids` | `std::vector<std::string>` |
| `summary` | `std::string` |
| `notes` | `std::vector<std::string>` |

The builder rejects missing required fields before returning an envelope. Child diagnostics preserve chain-root references and enforce stage-order rules.

## Diagnostic Kinds

The Windows enum includes semantic-core diagnostic kinds used by the implemented modules:

- `STATE_VALIDITY`
- `TRANSITION`
- `TOPOLOGY`
- `AXIOM`
- `GENERATION_PLAN`
- `EMISSION`
- `RECOVERY`
- `FALLBACK`
- `CONTAINMENT`
- `SAFE_HALT`

## Rule IDs

Rule ID constants live in `windows/src/include/ash/RuleIds.hpp` and use taxonomy-compliant families for state, codeword, transition, realm, topology, axiom, generation, emission, recovery, fallback, containment, halt, and diagnostics.

The former branch-local temporary family is no longer used by production module behavior.

## Verification

`windows/tests/Diagnostics.test.cpp` validates:

- complete envelope construction;
- rejection of missing kind, summary, notes, and rule IDs;
- rule ID format acceptance/rejection;
- envelope validation failures for missing subject and chain root;
- root and child chain integrity;
- invalid stage-order rejection;
- root-to-leaf chain exposure.

Module tests additionally verify that state normalization, transitions, recovery, fallback, safe halt, realm encoding, topology generation, axiom evaluation, generation planning, and artifact emission all return diagnostics.
