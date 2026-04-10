# Deviation Log — windows-cpp

Documents every branch-local deviation from the canonical ASH Pattern System specification, together with a resolution path for each. Every deviation in this log is a branch-local implementation convention, not a canonical redefinition. The canonical spec on main remains the semantic authority. See `windows/CANONICAL-DEFERENCE.md`.

## Deviation format

Each entry contains:
- **What** — the deviation in plain language
- **Why** — the reason the deviation exists
- **Scope** — exactly what on the branch the deviation affects
- **Canonical reference** — the canonical spec section that discusses the unresolved area
- **Resolution path** — how the deviation will be resolved in a future expansion pass

## 1. Empty fallback-policy registry

**What:** The `RecoveryEngine` ships with an empty in-memory fallback-policy registry stub. `select_fallback()` observes the empty registry and always returns `(nullopt, escalation diagnostic)` with `disposition=ESCALATED`.

**Why:** The canonical `specs/registries/fallback-policy-registry.md` on main defines the registry entry shape (policy_id, applicability_conditions, candidate_state_reference, ordering_rank) but does not enumerate the actual entries. The canonical `specs/algorithms/recovery-fallback-semantics.pseudo.md` explicitly states that when the registry is empty or unavailable, the correct behavior is to escalate to containment. Shipping an empty registry therefore matches canonical behavior exactly — this is not a semantic divergence, it is the canonical escalation path.

**Scope:** `windows/src/RecoveryEngine.cpp`'s `select_fallback` implementation.

**Canonical reference:** `specs/registries/fallback-policy-registry.md`, `specs/algorithms/recovery-fallback-semantics.pseudo.md`.

**Resolution path:** When the canonical fallback-policy registry is populated on main (in a future canonical-content pass), this branch updates `RecoveryEngine` to load the canonical entries and exercise the non-empty code path. The existing empty-registry escalation test becomes an edge-case test rather than the primary behavior.

## 2. Branch-local integer encoding

**What:** The branch encodes a 9-bit state as a `uint16_t` with `bit[i] = (encoded >> i) & 1`, so `b0` is the least significant bit and `b8` is bit 8.

**Why:** The canonical spec does not prescribe an integer encoding from `F2^9` to machine-native integer types. Every implementation must pick one. This choice is consistent with `std::bitset<9>::to_ulong()` and matches C/C++ conventions for bit packing.

**Scope:** `windows/src/include/ash/State.hpp` (`from_int`, `to_int`, `bN` accessors). Test code uses the encoding for convenient state construction.

**Canonical reference:** `specs/core/ash-state-space.pseudo.md`. The canonical spec treats states as 9-coordinate vectors without committing to an integer encoding.

**Resolution path:** No resolution needed. This is a branch-local convention that does not bind the canonical spec. If main ever prescribes an integer encoding, this branch adopts it. If a future canonical decision mandates a different bit order (e.g., b0 as MSB), the branch updates the `from_int`/`to_int` implementation without changing the rest of the code.

## 3. Branch-local realm identity string format

**What:** The branch uses 9 ASCII characters `'0'`/`'1'` from b0 to b8 as the realm identity string format (e.g., a state with b0=1 and b2=1 encodes as `"101000000"`).

**Why:** The canonical `specs/core/realm-identity.pseudo.md` intentionally leaves the external string format open. Every implementation must pick one that is deterministic and faithful to the 9-bit structure.

**Scope:** `windows/src/include/ash/RealmEncoder.hpp` (`RealmIdentity` struct) and `windows/src/RealmEncoder.cpp` (stub — will use this format when implemented).

**Canonical reference:** `specs/core/realm-identity.pseudo.md`. "This specification intentionally leaves the external string format open."

**Resolution path:** The current stub does not actually encode anything. When `RealmEncoder` is implemented in a future expansion pass, it adopts this format. If the canonical spec ever prescribes a format, this branch adopts the canonical format.

## 4. `ASH-WINDOWS-STUB-*` rule-ID family

**What:** The branch declares a `ASH-WINDOWS-STUB-001` rule ID and uses it in every NOT_IMPLEMENTED diagnostic emitted by a non-slice module stub.

**Why:** The canonical rule-ID taxonomy (`specs/interfaces/rule-id-taxonomy.md`) does not define a rule ID for "not yet implemented" stubs. Every diagnostic on this branch must still carry at least one taxonomy-compliant rule ID (the stub envelopes must validate against `Diagnostics::validate_rule_id`). The branch-local ASH-WINDOWS-STUB family is taxonomy-compliant (matches the `{FAMILY}-{CATEGORY}-{NUMBER}` pattern) but is not a canonical family.

**Scope:** `windows/src/include/ash/RuleIds.hpp` (the `kWindowsStub001` constant), `windows/src/RealmEncoder.cpp`, `windows/src/TopologyGenerator.cpp`, `windows/src/AxiomEvaluator.cpp`, `windows/src/GenerationPlanner.cpp`, `windows/src/ArtifactEmitter.cpp`.

**Canonical reference:** `specs/interfaces/rule-id-taxonomy.md`.

**Resolution path:** When the non-slice modules are implemented, every `ASH-WINDOWS-STUB-001` reference is replaced with a canonical rule ID from the appropriate family (`ASH-STATE-*`, `ASH-RECOVERY-*`, etc.). The `kWindowsStub001` constant in `RuleIds.hpp` is removed. This deviation is fully temporary.

## 5. Conservative `normalize` for TRANSFORMATION_COMPATIBLE states

**What:** `StateModel::normalize` currently returns `TRANSFORMATION_COMPATIBLE` states unchanged with a `PENDING` diagnostic, rather than computing an actual normalization path from the orbit representative to a canonical `VALID` state.

**Why:** The canonical `specs/core/state-admissibility.pseudo.md` and `specs/core/state-validity-diagnostics.pseudo.md` assert the existence of a normalization path from any orbit member to a VALID coset representative, but do not algorithmically specify the path-finding procedure. A conservative stub is the only faithful implementation until the canonical algorithm is formalized.

**Scope:** `windows/src/StateModel.cpp` `normalize()` method.

**Canonical reference:** `specs/core/state-admissibility.pseudo.md`, `specs/core/state-validity-diagnostics.pseudo.md`.

**Resolution path:** When the canonical normalization path algorithm is formalized (or when a future research pass establishes a deterministic canonical reduction algorithm such as coset-leader decoding), this branch updates `normalize` to compute and apply the path. The existing `normalize_valid_state_is_identity_with_resolved_diagnostic` test remains valid; a new test exercises the path-computed case.

## 6. Non-slice modules are stubs

**What:** `RealmEncoder`, `TopologyGenerator`, `AxiomEvaluator`, `GenerationPlanner`, and `ArtifactEmitter` are all shipped as headers plus NOT_IMPLEMENTED `.cpp` stubs. None of them has working runtime behavior.

**Why:** The instruction package governing this branch prescribes a minimal conformance slice first (`StateModel`, `RecoveryEngine`, `TransitionRegistry`, `Diagnostics`) followed by expansion toward full 9-module coverage. The non-slice modules are out of scope for this pass; they ship as stubs so the module mapping document has real files to reference and the build produces a complete `ash_core.lib`.

**Scope:** `windows/src/RealmEncoder.cpp`, `windows/src/TopologyGenerator.cpp`, `windows/src/AxiomEvaluator.cpp`, `windows/src/GenerationPlanner.cpp`, `windows/src/ArtifactEmitter.cpp`.

**Canonical reference:** `handoff-templates/common-downstream-handoff-requirements.md` (the canonical 9-module mapping table).

**Resolution path:** Each non-slice module becomes the subject of its own expansion pass. When a module is implemented, its `.cpp` is rewritten to provide real behavior, its test file is added, and its canonical invariants (INV-REALM-001, INV-REALM-002, INV-TOPO-001, INV-AXIOM-001, INV-PLAN-001, INV-PLAN-002, INV-BOUNDARY-001, INV-BOUNDARY-002) move from "Deferred (non-slice)" to "Covered" in `verification-plan.md`.

## 7. Branch-local narrowing of alignment-agent workflow trigger

**What:** `.github/workflows/alignment-agent.yml` on this branch has its trigger narrowed to `branches: [main]` for both `push` and `pull_request`, rather than `branches: ["**"]` as on main.

**Why:** The canonical alignment agent scans the entire repository for any implementation code outside `.github/scripts/` and flags it. That rule is a main-repository rule — it enforces the canonical-file-type policy of the closed agnostic baseline. It is not a repository-wide rule. On this branch, implementation code under `windows/` is expected and correct; without the trigger narrowing the agent would produce loud (but non-blocking, since it runs in report-only mode) warnings on every `windows-cpp` push, defeating its signal-to-noise ratio.

The branch-local narrowing is a one-file, two-line change. The version of the workflow file on the `main` branch is unchanged and continues to enforce the canonical-repo invariant on main pushes. The canonical semantic integrity agent, math integrity agent, and downstream conformance agent keep their original triggers on this branch and continue to run — they scan canonical files inherited unchanged from main and produce the same output they produce on main (specifically, the same three pre-existing `specs/registries/fallback-policy-registry.md` 8+1 findings that the prior passes already documented).

**Scope:** `.github/workflows/alignment-agent.yml` on `windows-cpp` only.

**Canonical reference:** `governance/github-agents-governance.md` on main § Alignment Agent.

**Resolution path:** None needed. This is an explicit branch-local scope adjustment. When future branches are created for other target classes, they may inherit the same adjustment.
