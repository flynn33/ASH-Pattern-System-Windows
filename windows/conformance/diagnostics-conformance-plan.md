# Diagnostics Conformance Plan — APS Windows

Diagnostics are represented by `ash::DiagnosticEnvelope` and constructed through `ash::DiagnosticEnvelopeBuilder`.

Current comparisons use canonical APS `1.0.0-rc.3` at `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8` with the separate bindings in [canonical deference](../CANONICAL-DEFERENCE.md). Rc.3 preserves the corrected rc.2 semantic diagnostic IDs and machine-rejects blank explanations and multiline summaries. Original rc.1 contradictions and actual rc.2 repair/integration evidence remain historical. This plan defines required checks; the exact released `qualification.json` establishes actual acceptance for its source and assets.

## Envelope Fields

| Field | Windows type |
|---|---|
| `schema_version`, `diagnostic_id`, `parent_diagnostic_id`, `chain_root_diagnostic_id` | `std::string` identity/transport fields; absent parent serializes null |
| `sequence_number` | `std::size_t` |
| `diagnostic_kind` | `DiagnosticKind` enum |
| `severity` | `Severity` enum |
| `stage` | `Stage` enum |
| `disposition` | `Disposition` enum |
| `subject_reference` | `std::string` |
| `parent_diagnostic_reference` | `std::shared_ptr<DiagnosticEnvelope>` |
| `chain_root_reference` | `std::weak_ptr<DiagnosticEnvelope>` |
| `rule_ids` | `std::vector<std::string>` |
| `summary` | `std::string` |
| `notes` | `std::vector<std::string>` |
| `details` | Owning `DiagnosticObject` |
| `occurrence_metadata` | `std::optional<DiagnosticObject>`; absent serializes null |

The builder rejects missing or meaningless required fields before returning an envelope. Children own their parent ancestry; roots are observed weakly. Explicit causal phases govern chaining, while ordinary operation stages inherit the prior phase. Sequence and parent/root identity must agree, severity cannot fall across a link, containment restricts further work, and terminal disposition freezes the chain.

Identity covers exactly nine semantic fields: kind, disposition, rule IDs, severity, stage, subject, summary, notes and details. UTF-8 JSON has sorted object keys, preserved array order and signed 64-bit integers. Parent/root IDs, sequence, schema version and occurrence metadata do not participate. Structured values own their content and reject floating-point and out-of-range unsigned values.

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
- `INPUT_VALIDATION`
- `STATE_ASSESSMENT`
- `CLASSIFICATION`
- `SCHEMA_VALIDATION`
- `CONFORMANCE`

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
- full semantic payload, structured details and occurrence metadata;
- deterministic UTF-8 identity under changed locale;
- explicit causal phases, containment restrictions and terminal freeze;
- allocation-failure containment in both native validation functions.

Module tests additionally verify that state normalization, transitions, recovery, fallback, safe halt, realm encoding, topology generation, axiom evaluation, generation planning, and artifact emission all return diagnostics.

`windows/tests/verify-native-diagnostic-records.py` independently consumes real serialized records from both x64 runner reports. It validates actual immutable schema bytes, recomputes semantic SHA-256 identities and causal chains, checks the required native record inventory and rejects corruption controls. Support capture/export tests separately exercise privacy, bounds, lifetime, profile-specific collectors and paired exports. Compiler success and self-validation alone do not satisfy these transport checks.
