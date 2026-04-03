# Conformance Categories — canonical verification requirements

## Purpose

This specification groups the canonical invariants from `invariant-spec.md` into **5 conformance categories**. Each category represents a distinct verification bucket that downstream implementations must satisfy.

This specification is **canonical** (Phase 3). All 5 categories must be satisfied for an implementation to be considered conformant. See `implementation-acceptance.md` for acceptance criteria.

---

## Category 1: Algebraic/State Conformance

### Scope
Verifies that the implementation correctly represents, normalizes, classifies, and validates ASH states using the locked algebraic definitions.

### Covers
- Deterministic normalization (same input → same output)
- Locked parity formula for control-bit derivation (`b0 XOR ... XOR b7`)
- Locked [8,4,4] normative 16-codeword set for admissibility
- Corrected-core derivation rule for correctable states
- System-state classification (7 canonical classes)
- Recoverability classification
- Control-dimension integrity (no direct mutation, always re-derived)

### Invariant families
- `INV-NORM-001` through `INV-NORM-003`
- `INV-CONTROL-001` through `INV-CONTROL-003`

### Downstream test requirement
A downstream test suite must demonstrate that:
1. Normalization produces identical results for identical inputs across all runs
2. The parity formula is correctly implemented for all 256 possible 8-bit inputs
3. All 16 normative codewords are correctly classified as `ADMISSIBLE`
4. Correctable cores are corrected before control derivation
5. Uncorrectable cores produce diagnostic failures, not silent results
6. The control bit is never directly mutated during ordinary transitions

---

## Category 2: Recovery/Fallback/Containment Conformance

### Scope
Verifies that the implementation correctly executes recovery, fallback, containment, and safe-halt behavior using the locked semantics and the canonical fallback-policy registry.

### Covers
- Deterministic recovery-category mapping (class → action)
- Fallback selection against the canonical registry (no heuristic guessing)
- Monotonic escalation (recovery → fallback → containment → safe halt)
- No silent healing (every recovery action produces a diagnostic)
- SAFE_HALT terminal finality (no transitions from SAFE_HALT)
- Containment behavior (restricted mode, diagnostic preservation)

### Invariant families
- `INV-RECOVERY-001` through `INV-RECOVERY-005`

### Downstream test requirement
A downstream test suite must demonstrate that:
1. Each system-state class maps to the correct recovery category
2. Fallback selection uses only the canonical registry
3. Escalation severity never decreases without external intervention
4. Every recovery action produces at least one diagnostic record
5. Once in SAFE_HALT, no further state transitions succeed

---

## Category 3: Diagnostics Conformance

### Scope
Verifies that the implementation produces diagnostics that conform to the shared diagnostic schema, use taxonomy-compliant rule IDs, and maintain chain integrity.

### Covers
- Diagnostic schema conformance (`DiagnosticEnvelope` required fields)
- Rule-ID taxonomy conformance (`{FAMILY}-{CATEGORY}-{NUMBER}` pattern)
- Diagnostic chain integrity (`parent_diagnostic_reference`, `chain_root_reference`)
- No silent omission (every step produces a diagnostic)
- Axiom evaluation diagnostics (no silent pass/fail, explainable results)

### Invariant families
- `INV-DIAG-001` through `INV-DIAG-004`
- `INV-AXIOM-001` through `INV-AXIOM-002`

### Downstream test requirement
A downstream test suite must demonstrate that:
1. Every diagnostic record has all required schema fields
2. Every `rule_id` matches the canonical taxonomy pattern
3. For any recovery/escalation sequence, the diagnostic chain is complete with valid references
4. No diagnostic is silently omitted, deferred, or suppressed
5. Every axiom evaluation produces an explainable diagnostic record

---

## Category 4: Generation/Materialization-Boundary Conformance

### Scope
Verifies that the implementation respects the locked materialization boundary between `GenerationPlanner` and `ArtifactEmitter`.

### Covers
- Planning produces no side effects (no files, processes, or mutations)
- The plan is self-contained (emitter needs only the plan)
- The planner does not emit artifacts
- The emitter does not invent semantics not present in the plan
- Plan structure is stable and deterministic

### Invariant families
- `INV-PLAN-001` through `INV-PLAN-002`
- `INV-BOUNDARY-001` through `INV-BOUNDARY-002`

### Downstream test requirement
A downstream test suite must demonstrate that:
1. Planning completes without any external side effects
2. The emitter can materialize from the plan without calling back to the planner
3. No artifact exists that is not traceable to a plan element
4. The planner produces no files, modules, or services

---

## Category 5: Contract/Module Conformance

### Scope
Verifies that the implementation satisfies the per-module contracts defined in Phase 2 for modules not fully covered by the other categories.

### Covers
- Realm encoding (deterministic, normalized-state-only input)
- Transition application (deterministic, normalized-state-only input, core-then-re-derive)
- Topology generation (deterministic, stable ordering, stable lineage)
- Per-module input/output conformance as defined in the detailed contract files

### Invariant families
- `INV-REALM-001` through `INV-REALM-002`
- `INV-TRANS-001` through `INV-TRANS-002`
- `INV-TOPO-001` through `INV-TOPO-002`

### Downstream test requirement
A downstream test suite must demonstrate that:
1. Realm encoding is deterministic and rejects unnormalized input
2. Transitions are deterministic, operate only on normalized states, and re-derive control
3. Topology generation is deterministic with stable ordering and lineage
4. Each module satisfies its detailed contract from `specs/interfaces/contracts/`

---

## Category coverage rule

All 5 conformance categories are **required**. A downstream implementation must not skip any category. Partial coverage (e.g., passing Algebraic/State but skipping Diagnostics) is **not** sufficient for conformance.

See `implementation-acceptance.md` for the full acceptance threshold.
