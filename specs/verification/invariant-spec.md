# Invariant Specification — canonical verification requirements

## Purpose

This specification defines the **canonical invariant set** that every downstream implementation of the ASH Pattern System must verify. Each invariant encodes a locked semantic, algebraic, contractual, or behavioral requirement from the agnostic specification repository.

This specification is **canonical** (Phase 3). Downstream implementations are not conformant unless they satisfy these invariants. See `implementation-acceptance.md` for acceptance criteria.

## Invariant structure

Each invariant has:
- **ID**: unique identifier in the form `INV-{FAMILY}-{NUMBER}`
- **Statement**: the property that must hold
- **Rationale**: why this invariant matters
- **Test criterion**: what a test must demonstrate
- **Source**: the specification that locks this requirement

---

## INV-NORM — Deterministic normalization

### INV-NORM-001
**Statement**: Normalization is deterministic — the same candidate state always produces the same normalized output or the same diagnostic failure.
**Rationale**: Non-deterministic normalization would make the entire state model unreliable.
**Test criterion**: For any candidate state, repeated normalization calls produce identical results.
**Source**: `ash-state-space.pseudo.md`, `state-model-contract.md`

### INV-NORM-002
**Statement**: For `INADMISSIBLE_CORRECTABLE` cores, normalization corrects to the nearest admissible codeword before deriving the control bit (corrected-core derivation rule).
**Rationale**: Raw-core derivation would produce a control bit inconsistent with the normalized state.
**Test criterion**: For every correctable core, the normalized output matches correction-then-derivation, not raw derivation.
**Source**: `ash-state-space.pseudo.md`, `core-admissibility.pseudo.md`

### INV-NORM-003
**Statement**: Normalization fails with a diagnostic for `INADMISSIBLE_DETECTABLE` and `INADMISSIBLE_UNRECOVERABLE` cores — it does not silently produce a result.
**Rationale**: Silent normalization of uncorrectable cores would mask errors.
**Test criterion**: Normalization of uncorrectable cores produces a diagnostic failure, not a normalized state.
**Source**: `ash-state-space.pseudo.md`, `state-validity-diagnostics.pseudo.md`

---

## INV-REALM — Deterministic realm encoding

### INV-REALM-001
**Statement**: Realm encoding is deterministic — equal normalized states produce equal realm encodings.
**Rationale**: Non-deterministic encoding would break realm identity.
**Test criterion**: For any normalized state, repeated encoding calls produce identical results.
**Source**: `realm-identity.pseudo.md`, `realm-encoder-contract.md`

### INV-REALM-002
**Statement**: Realm encoding operates only on normalized states — encoding from an unnormalized or invalid state must fail with a diagnostic.
**Rationale**: Encoding invalid states would propagate corruption into realm identity.
**Test criterion**: Encoding an unnormalized state produces a diagnostic failure.
**Source**: `realm-encoder-contract.md`

---

## INV-CONTROL — Control-dimension integrity

### INV-CONTROL-001
**Statement**: The control bit is always derived using the locked parity formula: `b0 XOR b1 XOR b2 XOR b3 XOR b4 XOR b5 XOR b6 XOR b7`.
**Rationale**: Any other formula would break the algebraic lock.
**Test criterion**: For all 256 possible 8-bit cores, `derive_control_bit` produces the correct overall parity.
**Source**: `control-bit-derivation.pseudo.md`

### INV-CONTROL-002
**Statement**: For all 16 admissible codewords, the derived control bit is `0`.
**Rationale**: The locked [8,4,4] code is doubly-even; all codewords have even parity.
**Test criterion**: `derive_control_bit(codeword) == 0` for each of the 16 normative codewords.
**Source**: `control-bit-derivation.pseudo.md`, `core-admissibility.pseudo.md`

### INV-CONTROL-003
**Statement**: Ordinary transitions must not directly mutate the control bit — any change must come through re-derivation from the core.
**Rationale**: Direct mutation would bypass the parity sentinel.
**Test criterion**: After any transition, the control bit equals `derive_control_bit(resulting_core)`.
**Source**: `ash-state-space.pseudo.md`, `transition-registry-contract.md`

---

## INV-TRANS — Deterministic transition outcomes

### INV-TRANS-001
**Statement**: Transition resolution is deterministic — the same state and transition identifier always produce the same result.
**Rationale**: Non-deterministic transitions would make the system unpredictable.
**Test criterion**: Repeated transition applications with the same inputs produce identical outputs.
**Source**: `transition-system.pseudo.md`, `transition-registry-contract.md`

### INV-TRANS-002
**Statement**: Transitions operate only on normalized states — applying a transition to an unnormalized state must fail.
**Rationale**: Transitioning from an invalid state would propagate corruption.
**Test criterion**: Transition application to an unnormalized state produces a diagnostic failure.
**Source**: `transition-registry-contract.md`

---

## INV-TOPO — Deterministic topology generation

### INV-TOPO-001
**Statement**: Topology generation is deterministic — the same input produces the same topology structure.
**Rationale**: Non-deterministic topology would break lineage and ordering guarantees.
**Test criterion**: Repeated generation with the same input produces identical topology.
**Source**: `topology-expansion.pseudo.md`, `topology-generator-contract.md`

### INV-TOPO-002
**Statement**: Topology ordering is stable and does not depend on platform-specific behavior.
**Rationale**: Platform-dependent ordering would break cross-implementation consistency.
**Test criterion**: Topology ordering is identical across implementations for the same input.
**Source**: `topology-generator-contract.md`

---

## INV-AXIOM — Stable axiom diagnostics

### INV-AXIOM-001
**Statement**: Every axiom evaluation produces a diagnostic record — no bare pass/fail without diagnostics.
**Rationale**: Silent pass/fail makes the system opaque and unauditable.
**Test criterion**: Every axiom evaluation call returns a diagnostic record with rule IDs and explanation.
**Source**: `axiom-evaluation.pseudo.md`, `axiom-evaluator-contract.md`

### INV-AXIOM-002
**Statement**: Axiom evaluation is deterministic — the same inputs produce the same results.
**Rationale**: Non-deterministic evaluation would make axiom diagnostics unreliable.
**Test criterion**: Repeated evaluation with the same inputs produces identical results.
**Source**: `axiom-evaluator-contract.md`

---

## INV-RECOVERY — Stable recovery/fallback/containment behavior

### INV-RECOVERY-001
**Statement**: Recovery is deterministic — the same system-state classification always produces the same recovery category.
**Rationale**: Non-deterministic recovery would make the system unpredictable under failure.
**Test criterion**: For each system-state class, `classify_recoverability` always returns the same category.
**Source**: `recoverability-semantics.pseudo.md`, `recovery-engine-contract.md`

### INV-RECOVERY-002
**Statement**: Fallback selection uses only the canonical fallback-policy registry — no heuristic, ad hoc, or invented fallback states.
**Rationale**: Non-registry fallback would violate the locked policy design.
**Test criterion**: Every fallback selection is traceable to a `policy_id` in the canonical registry.
**Source**: `fallback-policy-registry.md`, `recovery-engine-contract.md`

### INV-RECOVERY-003
**Statement**: Escalation is monotonic — recovery failure always escalates to a more severe category, never de-escalates without external intervention.
**Rationale**: De-escalation without intervention would mask escalating failures.
**Test criterion**: The severity of recovery categories in a chain is monotonically non-decreasing.
**Source**: `recoverability-semantics.pseudo.md`, `recovery-engine-contract.md`

### INV-RECOVERY-004
**Statement**: No silent healing — every recovery action produces a diagnostic record.
**Rationale**: Silent healing makes the system unauditable.
**Test criterion**: Every recovery/fallback/containment/halt action produces at least one diagnostic.
**Source**: `recovery-fallback-semantics.pseudo.md`, `recovery-engine-contract.md`

### INV-RECOVERY-005
**Statement**: `SAFE_HALT` is terminal — no transitions from `SAFE_HALT` to any other state.
**Rationale**: Terminal finality is the foundation of safe-failure semantics.
**Test criterion**: Once a system enters `SAFE_HALT`, no subsequent state transition succeeds.
**Source**: `containment-safe-failure-semantics.pseudo.md`, `system-state-classification.pseudo.md`

---

## INV-PLAN — Stable generation-plan structure

### INV-PLAN-001
**Statement**: Generation planning produces no side effects — no files, no processes, no observable mutations.
**Rationale**: Side effects during planning would violate the materialization boundary.
**Test criterion**: Planning completes without creating, modifying, or deleting any external resource.
**Source**: `generation-planning.pseudo.md`, `generation-planner-contract.md`

### INV-PLAN-002
**Statement**: The generation plan is self-contained — the emitter needs only the plan to materialize.
**Rationale**: A non-self-contained plan would force the emitter to call back to the planner, violating the boundary.
**Test criterion**: The emitter can materialize from the plan without any additional planner interaction.
**Source**: `generation-planner-contract.md`

---

## INV-BOUNDARY — Materialization-boundary conformance

### INV-BOUNDARY-001
**Statement**: `GenerationPlanner` does not emit artifacts — it produces only a plan.
**Rationale**: Artifact emission by the planner would collapse the materialization boundary.
**Test criterion**: No file, module, or service is created during planning.
**Source**: `generation-planner-contract.md`

### INV-BOUNDARY-002
**Statement**: `ArtifactEmitter` does not invent semantics not present in the plan.
**Rationale**: Invented semantics would diverge from the canonical specification.
**Test criterion**: Every emitted artifact is traceable to a plan element; no artifact exists without a plan source.
**Source**: `artifact-emitter-contract.md`

---

## INV-DIAG — Diagnostic schema and rule-ID taxonomy conformance

### INV-DIAG-001
**Statement**: Every diagnostic record conforms to the shared `DiagnosticEnvelope` schema.
**Rationale**: Non-conformant diagnostics would break the unified diagnostic chain.
**Test criterion**: Every diagnostic has all required fields: `diagnostic_kind`, `severity`, `stage`, `disposition`, `rule_ids`, `summary`, `notes`.
**Source**: `diagnostic-schema.md`, `diagnostics-module-contract.md`

### INV-DIAG-002
**Statement**: Every `rule_id` in every diagnostic conforms to the canonical taxonomy pattern `{FAMILY}-{CATEGORY}-{NUMBER}`.
**Rationale**: Non-conformant rule IDs would break traceability.
**Test criterion**: Every `rule_id` matches the taxonomy pattern and references a defined rule family.
**Source**: `rule-id-taxonomy.md`, `diagnostics-module-contract.md`

### INV-DIAG-003
**Statement**: The diagnostic chain is complete — every step from detection through terminal halt produces a diagnostic, and chain references are intact.
**Rationale**: Gaps in the chain would make post-mortem analysis impossible.
**Test criterion**: For any recovery/escalation sequence, every step has a diagnostic with valid `parent_diagnostic_reference` and `chain_root_reference`.
**Source**: `diagnostic-schema.md`, `diagnostics-module-contract.md`

### INV-DIAG-004
**Statement**: No silent omission — diagnostics are never deferred, batched, or conditionally suppressed.
**Rationale**: Silent omission defeats the purpose of the diagnostic system.
**Test criterion**: Every required diagnostic is emitted at the point of the action, not later.
**Source**: `diagnostic-schema.md`, `diagnostics-module-contract.md`
