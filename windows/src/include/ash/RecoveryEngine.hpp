// ash/RecoveryEngine.hpp
//
// RecoveryEngine — canonical module.
//
// Canonical source of truth:
//   specs/interfaces/contracts/recovery-engine-contract.md on main
//   specs/algorithms/recovery-fallback-semantics.pseudo.md on main
//   specs/algorithms/containment-safe-failure-semantics.pseudo.md on main
//
// Canonical purpose (paraphrased):
//   Deterministic recovery, fallback, containment, and safe-halt
//   executor, consuming StateModel classifications and executing
//   monotonic escalation through recovery → fallback → containment →
//   halt.
//
// Canonical invariants and rules:
//   - INV-RECOVERY-001: Recovery classification is deterministic
//   - INV-RECOVERY-002: Fallback uses only canonical registry
//   - INV-RECOVERY-003: Escalation is monotonic
//   - INV-RECOVERY-004: No silent healing (every action diagnosable)
//   - INV-RECOVERY-005: SAFE_HALT is terminal — no transitions out
//
// Prohibited behaviors (canonical):
//   - MUST NOT silently heal without producing diagnostics
//   - MUST NOT skip containment when specifications require it
//   - MUST NOT select fallback states outside the canonical registry
//   - MUST NOT allow transitions from SAFE_HALT
//   - MUST NOT decompose recovery into 8-bit core + control-bit derivation
//
// Operational recovery takes an explicit validated context/assessment.
// The legacy codeword overload uses the documented built-in test context;
// it does not infer domain safety for an arbitrary application.

#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "Codeword.hpp"
#include "DiagnosticEnvelope.hpp"
#include "State.hpp"
#include "StateModel.hpp"
#include "TransitionRegistry.hpp"

namespace ash {

enum class RecoveryOutcome {
    RECOVERED,
    RECOVERED_VIA_FALLBACK_REPLACEMENT,
    TARGET_RESOLUTION_BLOCKED,
    RECOVERY_FAILED,
    ESCALATE_TO_CONTAINMENT,
    NOT_APPLICABLE,
    TERMINAL_NO_RECOVERY,
};

enum class ContainmentTrigger { FALLBACK_FAILURE, PROPAGATION_RISK, OPERATOR_REQUEST, RECOVERY_VALIDATION_FAILURE };
enum class SafeHaltTrigger { ESCALATION_FROM_FAILED, CONTAINMENT_BREACH, OPERATOR_HALT_REQUEST, POLICY_HALT_REQUEST, UNRESOLVABLE_BLOCKED_RECOVERY };
enum class ContainmentResolution { PENDING, OPERATOR_DIRECTED, POLICY_DIRECTED, SAFE_HALT };

// Value projections of the canonical containment-record and safe-halt-record.
// Owning record references keep the complete causal chain alive for export.
// Times describe this engine's entry/adoption and do not affect semantic IDs.
struct ContainmentDiagnostic {
    ContainmentTrigger trigger{ContainmentTrigger::OPERATOR_REQUEST};
    std::shared_ptr<const DiagnosticEnvelope> original_diagnostic;
    std::shared_ptr<const DiagnosticEnvelope> diagnostic;
    SystemStateClass system_state_class{SystemStateClass::CONTAINED};
    std::vector<std::shared_ptr<const DiagnosticEnvelope>> full_diagnostic_chain;
    std::vector<std::string> restricted_operations;
    std::string containment_time;
    bool awaiting_resolution{true};
    ContainmentResolution resolution{ContainmentResolution::PENDING};
};
struct SafeHaltDiagnostic {
    SafeHaltTrigger trigger{SafeHaltTrigger::OPERATOR_HALT_REQUEST};
    std::shared_ptr<const DiagnosticEnvelope> original_diagnostic;
    std::shared_ptr<const DiagnosticEnvelope> diagnostic;
    SystemStateClass system_state_class{SystemStateClass::SAFE_HALT};
    std::vector<std::shared_ptr<const DiagnosticEnvelope>> full_diagnostic_chain;
    std::string halt_time;
    bool is_terminal{true};
    bool frozen{true};
};

struct RecoveryResult {
    Bit9State state;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
    RecoveryOutcome outcome{RecoveryOutcome::RECOVERY_FAILED};
};

struct FallbackResult {
    std::optional<Bit9State> state;  // empty => no candidate chosen, escalating
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
    std::string policy_id;
    RecoveryOutcome outcome{RecoveryOutcome::ESCALATE_TO_CONTAINMENT};
    std::shared_ptr<DiagnosticEnvelope> containment_diagnostic;
};

class RecoveryEngine {
public:
    // The engine composes StateModel and TransitionRegistry. They are
    // passed by const reference and not owned.
    RecoveryEngine(const StateModel& state_model,
                   const TransitionRegistry& transition_registry) noexcept
        : state_model_(&state_model),
          transition_registry_(&transition_registry) {}

    // Returns true iff the engine has been placed in SAFE_HALT.
    [[nodiscard]] bool is_halted() const noexcept { return halted_; }
    [[nodiscard]] bool is_contained() const noexcept { return contained_; }
    [[nodiscard]] std::shared_ptr<const DiagnosticEnvelope> diagnostic_chain() const noexcept { return last_diagnostic_; }
    // Earlier independent chains are retained when external runtime evidence
    // is adopted. Separate observer histories are never spliced into one chain.
    [[nodiscard]] const std::vector<std::shared_ptr<const DiagnosticEnvelope>>& retained_diagnostic_chains() const noexcept { return retained_diagnostic_chains_; }
    [[nodiscard]] std::optional<ContainmentDiagnostic> containment_record() const;
    [[nodiscard]] std::optional<SafeHaltDiagnostic> safe_halt_record() const;

    // Execute structural normalization. Delegates to StateModel::normalize,
    // wraps the result diagnostic as a RECOVERY-stage child.
    // If already halted, returns the frozen terminal diagnostic (INV-RECOVERY-005)
    // and does not invoke the state model.
    [[nodiscard]] RecoveryResult execute_normalize_state(const Bit9State& state);

    // Execute an APPLY_CORRECTION recovery. Delegates to
    // TransitionRegistry::apply_transition, then re-classifies the
    // result. A failed transition or non-STABLE post-state preserves
    // the original state and requires fallback without automatic retry.
    // If already halted, returns the frozen terminal diagnostic.
    [[nodiscard]] RecoveryResult execute_apply_correction(const Bit9State& state,
                                                          const Codeword& correction);
    [[nodiscard]] RecoveryResult execute_correction(const StateAssessment& assessment,
                                                    const OperationalContext& context);

    // Without an explicit policy instance, no replacement is authorized;
    // FALLBACK_REQUIRED therefore enters containment. The explicit overload
    // realizes the canonical policy using application-supplied evidence.
    // If already halted, returns the frozen terminal diagnostic.
    [[nodiscard]] FallbackResult select_fallback(RecoveryCategory category);
    [[nodiscard]] FallbackResult select_fallback(const StateAssessment& assessment,
                                                const OperationalContext& context,
                                                const FallbackPolicyInstance& instance);

    // Enter containment. Does not transition out of containment
    // automatically. Emits a CONTAINMENT/ESCALATION/BLOCKED envelope.
    // If already halted, returns the frozen terminal diagnostic.
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    enter_containment(std::string_view reason);
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    enter_containment(ContainmentTrigger trigger, std::string_view reason);

    // Enter safe halt. Terminal. After this call, is_halted() returns
    // true and every other method returns this frozen terminal diagnostic.
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    enter_safe_halt(std::string_view reason);
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    enter_safe_halt(SafeHaltTrigger trigger, std::string_view reason);

    // A containment boundary breach enters terminal halt with the breach cause.
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope> breach_containment(std::string_view reason);

private:
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    observe_runtime_mode_(const StateAssessment& assessment, std::string_view attempted_action);
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    build_failed_rejection_(std::string_view attempted_action) const;
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    build_halt_rejection_(std::string_view attempted_action) const;
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    build_containment_rejection_(std::string_view attempted_action);
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    prepare_escalation_parent_(std::string_view reason);

    const StateModel* state_model_;
    const TransitionRegistry* transition_registry_;
    bool halted_{false};
    bool contained_{false};
    bool correction_failed_{false};
    std::optional<Bit9State> failed_correction_source_;
    bool failed_{false};
    std::shared_ptr<DiagnosticEnvelope> last_diagnostic_;
    std::shared_ptr<DiagnosticEnvelope> containment_diagnostic_;
    std::shared_ptr<DiagnosticEnvelope> halt_diagnostic_;
    std::vector<std::shared_ptr<const DiagnosticEnvelope>> retained_diagnostic_chains_;
    ContainmentTrigger containment_trigger_{ContainmentTrigger::OPERATOR_REQUEST};
    SafeHaltTrigger halt_trigger_{SafeHaltTrigger::OPERATOR_HALT_REQUEST};
    std::string containment_time_;
    std::string halt_time_;
};

}  // namespace ash
