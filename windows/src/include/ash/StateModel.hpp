// ash/StateModel.hpp
//
// StateModel — canonical module.
//
// Canonical source of truth:
//   specs/interfaces/contracts/state-model-contract.md on main
//   specs/core/ash-state-space.pseudo.md on main
//   specs/core/state-admissibility.pseudo.md on main
//   specs/core/system-state-classification.pseudo.md on main
//   specs/core/recoverability-semantics.pseudo.md on main
//
// Canonical purpose (paraphrased from the contract):
//   The foundational module owning the 9-bit state representation,
//   normalization, validity diagnostics, system-state classification,
//   and recoverability classification.
//
// Preconditions (canonical):
//   - Access to canonical codeword set C ⊂ F2^9
//   - All 9 coordinates participate in state processing (no 8+1
//     decomposition for canonical classification)
//
// Postconditions and invariants (canonical):
//   - Normalization is deterministic (INV-STATE-002)
//   - Classification is total: every state maps to exactly one class
//     (INV-ADMISSIBILITY-002)
//   - No silent normalization; every normalize() produces a diagnostic
//     (INV-STATE-003)
//   - All 9 bits participate equally in algebraic structure
//
// Prohibited behaviors (canonical):
//   - MUST NOT decompose 9-bit state into 8-bit core + derived 9th bit
//   - MUST NOT silently treat transformation-incompatible state as valid

#pragma once

#include <memory>
#include <utility>

#include "DiagnosticEnvelope.hpp"
#include "State.hpp"

namespace ash {

// Four-way canonical admissibility enum — specs/core/state-admissibility.pseudo.md
enum class AdmissibilityStatus {
    VALID,                        // structurally valid, in C
    TRANSFORMATION_COMPATIBLE,    // in same orbit as a VALID state
    TRANSFORMATION_INCOMPATIBLE,  // codeword set not fully specified OR outside all orbits
    UNCLASSIFIED,                 // malformed input
};

// Seven-class canonical system-state enum — specs/core/system-state-classification.pseudo.md
enum class SystemStateClass {
    STABLE,
    UNSTABLE,
    CORRECTABLE,
    DEGRADED,
    CONTAINED,
    FAILED,
    SAFE_HALT,
};

// Seven-category canonical recovery enum — specs/core/recoverability-semantics.pseudo.md
enum class RecoveryCategory {
    NO_ACTION,
    NORMALIZE_STATE,
    APPLY_CORRECTION,
    FALLBACK_REQUIRED,
    CONTAINMENT_REQUIRED,
    ESCALATION_REQUIRED,
    TERMINAL_NO_RECOVERY,
};

// Pair type returned by normalize(): the resulting state plus a
// diagnostic envelope. Neither field is ever silently dropped.
struct NormalizeResult {
    Bit9State state;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

class StateModel {
public:
    StateModel() = default;

    // Classify the admissibility of a 9-bit state. Linear scan over
    // the canonical codeword set. Deterministic and total.
    [[nodiscard]] AdmissibilityStatus classify_admissibility(const Bit9State& s) const noexcept;

    // Classify the system-state class of a 9-bit state. Composes
    // admissibility with the canonical class mapping.
    [[nodiscard]] SystemStateClass classify_system_state(const Bit9State& s) const noexcept;

    // Deterministic mapping from system-state class to recovery
    // category per specs/core/recoverability-semantics.pseudo.md.
    [[nodiscard]] RecoveryCategory classify_recoverability(SystemStateClass cls) const noexcept;

    // Normalize a candidate state. Always produces a diagnostic.
    // Conservative implementation on this branch: for VALID states,
    // returns the input unchanged with a RESOLVED diagnostic. For
    // TRANSFORMATION_COMPATIBLE states, returns the input unchanged
    // with a PENDING diagnostic pointing at the future normalization
    // path algorithm (see deviation-log.md item 5).
    [[nodiscard]] NormalizeResult normalize(const Bit9State& candidate) const;
};

}  // namespace ash
