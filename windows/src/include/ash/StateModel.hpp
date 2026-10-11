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
//   - All 9 coordinates participate in state processing.
//   - Operational health requires explicit context. The no-context
//     classifier retained here is a default in-memory helper for the
//     current semantic core tests, not a universal safety judgment.
//
// Postconditions and invariants (canonical):
//   - Structural normalization is deterministic and realm-preserving.
//   - Every Bit9State is a well-formed realm.
//   - Every normalize() produces a diagnostic.
//   - All 9 bits participate equally in algebraic structure
//
// Prohibited behaviors (canonical):
//   - MUST NOT decompose 9-bit state into 8-bit core + derived 9th bit
//   - MUST NOT change realm identity during structural normalization.

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <array>
#include <utility>

#include "DiagnosticEnvelope.hpp"
#include "State.hpp"

namespace ash {

enum class AdmissibilityStatus {
    WELL_FORMED,
    MALFORMED,
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
    TARGET_RESOLUTION_REQUIRED,
    APPLY_CORRECTION,
    FALLBACK_REQUIRED,
    CONTAINMENT_ACTIVE,
    EXTERNAL_ESCALATION_REQUIRED,
    TERMINAL_NO_RECOVERY,
};

// Pair type returned by normalize(): the resulting state plus a
// diagnostic envelope. Neither field is ever silently dropped.
struct NormalizeResult {
    Bit9State state;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

enum class RuntimeMode { NORMAL, CONTAINED, FAILED, SAFE_HALT };

// The canonical registry defines selection rules, not domain-safe states.
// Applications provide verified candidates and the explicit operational policy.
struct FallbackCandidate {
    Bit9State state;
    std::uint64_t configured_rank{0};
    std::uint64_t verification_sequence{0};
    bool applicable{true};
    bool verification_valid{true};
    std::string evidence_reference;
    bool escalate_on_validation_failure{false};
};

struct FallbackPolicyInstance {
    std::string instance_id;
    std::string policy_id;
    std::string registry_version{"1.0"};
    bool available{true};
    std::vector<FallbackCandidate> candidates;
};

struct OperationalContext {
    std::string context_id;
    std::string context_version;
    std::vector<Bit9State> allowed_stable_states;
    std::optional<Bit9State> preferred_state;
    FallbackPolicyInstance fallback_policy_instance;
    std::string codeword_registry_version{"1.0"};
    bool propagation_risk_policy_defined{false};
    bool propagation_risk_permits_mutation{false};
    bool containment_policy_defined{false};
    bool halt_policy_defined{false};
    std::vector<std::string> restricted_operations{"inspect", "diagnostics", "export", "safe_halt"};
};

struct InputNormalizationResult {
    std::optional<Bit9State> state;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

struct StateAssessment {
    std::optional<Bit9State> state;
    std::string operational_context_id;
    std::vector<Bit9State> candidate_targets;
    std::optional<Bit9State> selected_target;
    std::optional<std::size_t> correction_codeword_id;
    RuntimeMode runtime_mode{RuntimeMode::NORMAL};
    std::optional<SystemStateClass> state_class;
    bool blocked{true};
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

struct AveragingResult {
    std::array<double, kStateSpaceSize> values{};
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

class StateModel {
public:
    StateModel() = default;

    // Classify structural input status. Bit9State is always well formed.
    [[nodiscard]] AdmissibilityStatus classify_admissibility(const Bit9State& s) const noexcept;

    // Classify the system-state class of a 9-bit state. Composes
    // admissibility with the canonical class mapping.
    [[nodiscard]] SystemStateClass classify_system_state(const Bit9State& s) const noexcept;

    // Deterministic mapping from system-state class to recovery
    // category per specs/core/recoverability-semantics.pseudo.md.
    [[nodiscard]] RecoveryCategory classify_recoverability(SystemStateClass cls) const noexcept;

    // Normalize a candidate state. Always produces a diagnostic.
    // Structural normalization returns the input unchanged with a
    // RESOLVED diagnostic for every well-formed Bit9State.
    [[nodiscard]] NormalizeResult normalize(const Bit9State& candidate) const;

    // Boundary parsing rejects malformed signatures before an ASH state exists.
    [[nodiscard]] InputNormalizationResult normalize_input(std::string_view candidate) const;
    [[nodiscard]] bool validate_context(const OperationalContext& context) const noexcept;
    [[nodiscard]] StateAssessment assess_state(std::string_view candidate,
                                              const OperationalContext* context,
                                              RuntimeMode mode = RuntimeMode::NORMAL) const;
    [[nodiscard]] StateAssessment assess_state(const Bit9State& state,
                                              const OperationalContext& context,
                                              RuntimeMode mode = RuntimeMode::NORMAL) const;
    // Canonical T f(x) = sum_c f(x XOR c) / 16 over the full state space.
    [[nodiscard]] AveragingResult average(const std::array<double, kStateSpaceSize>& values) const;
};

}  // namespace ash
