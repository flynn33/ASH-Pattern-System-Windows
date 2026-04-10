# Acceptance Judgment — windows-cpp

> **STATUS: NON-CONFORMANT (IN PROGRESS)**
>
> This file is a **placeholder**, not a final judgment. The `windows-cpp` branch currently implements the minimal conformance slice (4 of 9 canonical modules). Full conformance requires all 9 modules to be implemented and all 28 canonical invariants to pass dynamic verification.

## Canonical acceptance language

From `specs/verification/implementation-acceptance.md` on main, line 69:

> "There is no 'partial conformance.' An implementation is CONFORMANT, CONFORMANT WITH CAVEATS, or NON-CONFORMANT."

The three canonical acceptance tiers are:

- **CONFORMANT** — all 28 invariants pass dynamic verification, all 5 conformance categories have runtime coverage, all 9 module contracts are satisfied, diagnostics are complete.
- **CONFORMANT WITH CAVEATS** — all invariants pass, but the implementation carries documented caveats that do not affect core conformance (e.g., platform-specific performance characteristics, optional feature gaps).
- **NON-CONFORMANT** — any invariant fails, any conformance category is uncovered, any failure condition from `implementation-acceptance.md` lines 29–35 is triggered, or diagnostics are incomplete.

## Current judgment for windows-cpp

**NON-CONFORMANT (IN PROGRESS)**

## Why not CONFORMANT

The current branch state does not satisfy the CONFORMANT criteria for these reasons:

1. **Non-slice modules are stubs.** `RealmEncoder`, `TopologyGenerator`, `AxiomEvaluator`, `GenerationPlanner`, and `ArtifactEmitter` ship as NOT_IMPLEMENTED stubs. Their canonical invariants cannot be dynamically verified:
   - `INV-REALM-001` (realm encoding deterministic) — deferred
   - `INV-REALM-002` (realm encoding valid-state only) — deferred
   - `INV-TOPO-001` (topology deterministic, stable ordering) — deferred
   - `INV-AXIOM-001` (every axiom evaluation produces diagnostic) — deferred
   - `INV-PLAN-001` (planning no side effects) — architectural only
   - `INV-PLAN-002` (plan self-contained) — architectural only
   - `INV-BOUNDARY-001` (no artifacts during planning) — architectural only
   - `INV-BOUNDARY-002` (emitted artifacts trace to plan) — architectural only

2. **Conformance Category 4 (Materialization Boundary) lacks runtime coverage.** The boundary is enforced at the header signature level (see `materialization-boundary-plan.md`), but no runtime test observes side effects during planning or verifies plan/artifact traceability during emission. Category 4 therefore has only architectural coverage on this branch.

3. **Conformance Category 5 (Contract/Module) has partial coverage.** The four slice modules have runtime contract coverage; the five non-slice modules have only header-level contract comments without runtime verification.

4. **Normalization path computation is conservative.** `StateModel::normalize` currently returns `TRANSFORMATION_COMPATIBLE` states unchanged with a PENDING diagnostic rather than computing an actual normalization path (see `deviation-log.md` item 5).

These are all expected gaps at this stage of the expansion roadmap. They are not defects relative to the current branch scope; they are the scope.

## Why not NON-CONFORMANT with failure conditions

None of the seven explicit failure conditions from `specs/verification/implementation-acceptance.md` is triggered by the current branch:

| # | Failure condition | Status on windows-cpp |
|---|---|---|
| 1 | Any non-codeword-dependent invariant fails | ✓ None fails — 20 of 28 are dynamically covered and pass; 8 are architectural or deferred |
| 2 | Any conformance category is missing | ✓ All 5 categories have at least architectural coverage |
| 3 | 9-bit state decomposed into 8-bit core + derived 9th bit | ✓ `Bit9State` is a `std::bitset<9>`; no decomposition anywhere |
| 4 | Materialization boundary violated | ✓ Enforced architecturally in GenerationPlanner.hpp/ArtifactEmitter.hpp |
| 5 | Diagnostics incomplete or non-conformant | ✓ All envelopes built via validated builder; rule IDs taxonomy-compliant; chain integrity tested |
| 6 | Silent healing occurs (recovery without diagnostics) | ✓ Every RecoveryEngine path returns a diagnostic |
| 7 | Codewords invented outside research baseline | ✓ `kCanonicalCodewords` transcribed literally from `specs/core/codeword-set.pseudo.md`; `is_canonical_codeword` enforces membership |

The branch is in the "not yet complete" sense of NON-CONFORMANT, not the "violates a failure condition" sense. The distinction matters for the expansion roadmap: there is no corrective work required for the slice modules; there is only additive work required for the non-slice modules.

## Path to CONFORMANT

1. Implement `GenerationPlanner` and `ArtifactEmitter` with runtime boundary verification tests. Moves Category 4 to runtime coverage. Resolves INV-PLAN-001, INV-PLAN-002, INV-BOUNDARY-001, INV-BOUNDARY-002.
2. Implement `RealmEncoder` with deterministic 9-char realm identity encoding. Resolves INV-REALM-001, INV-REALM-002.
3. Implement `TopologyGenerator` with deterministic ordering and lineage. Resolves INV-TOPO-001. Note that the canonical branching algorithms are pending R2 on main; this step depends on that canonical resolution.
4. Implement `AxiomEvaluator` with per-axiom diagnostic emission. Resolves INV-AXIOM-001.
5. Implement `StateModel::normalize` path-finding for `TRANSFORMATION_COMPATIBLE` states once the canonical path algorithm is specified. Resolves deviation item 5.
6. Populate the fallback-policy registry once canonical entries are defined on main. Resolves deviation item 1.

When these six steps are complete and all tests pass, this document is updated to a final **CONFORMANT** or **CONFORMANT WITH CAVEATS** judgment, signed and dated.

## Placeholder disclaimer

This file is a placeholder. It does not constitute a formal acceptance judgment. A formal judgment will be issued only after:

1. All 28 canonical invariants have dynamic runtime coverage (not just architectural)
2. All 9 canonical modules have working implementations
3. The full test suite has been run against the current build and all tests pass
4. A human reviewer has signed off on the branch implementation

Until those conditions are met, the branch operates in the "minimal conformance slice" mode and the judgment remains `NON-CONFORMANT (IN PROGRESS)`.
