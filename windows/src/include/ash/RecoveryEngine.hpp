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
// Branch-local behavior (documented in deviation-log.md item 1):
//   The canonical fallback-policy registry content is unresolved. This
//   branch ships with an empty in-memory registry. select_fallback()
//   always observes an empty candidate list and escalates to
//   CONTAINMENT_ACTIVE — this is exactly the behavior the canonical
//   recovery/fallback semantics prescribe when the registry is
//   unavailable or empty.

#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "Codeword.hpp"
#include "DiagnosticEnvelope.hpp"
#include "State.hpp"
#include "StateModel.hpp"
#include "TransitionRegistry.hpp"

namespace ash {

struct RecoveryResult {
    Bit9State state;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

struct FallbackResult {
    std::optional<Bit9State> state;  // empty => no candidate chosen, escalating
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
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

    // Execute structural normalization. Delegates to StateModel::normalize,
    // wraps the result diagnostic as a RECOVERY-stage child.
    // If already halted, returns an ERROR diagnostic (INV-RECOVERY-005)
    // and does not invoke the state model.
    [[nodiscard]] RecoveryResult execute_normalize_state(const Bit9State& state);

    // Execute an APPLY_CORRECTION recovery. Delegates to
    // TransitionRegistry::apply_transition, then re-classifies the
    // result. If the result is not STABLE, emits a RECOVERY_FAILED
    // diagnostic at stage ESCALATION.
    // If already halted, returns an ERROR diagnostic.
    [[nodiscard]] RecoveryResult execute_apply_correction(const Bit9State& state,
                                                          const Codeword& correction);

    // Select a fallback state for the given recovery category. On
    // this branch the registry is empty, so the result is always
    // (nullopt, escalation_diagnostic) with a note documenting the
    // empty-registry condition.
    // If already halted, returns an ERROR diagnostic.
    [[nodiscard]] FallbackResult select_fallback(RecoveryCategory category);

    // Enter containment. Does not transition out of containment
    // automatically. Emits a CONTAINMENT/ESCALATION/BLOCKED envelope.
    // If already halted, returns an ERROR diagnostic.
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    enter_containment(std::string_view reason);

    // Enter safe halt. Terminal. After this call, is_halted() returns
    // true and every other method returns an ERROR diagnostic citing
    // INV-RECOVERY-005.
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    enter_safe_halt(std::string_view reason);

private:
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    build_halt_rejection_(std::string_view attempted_action) const;

    const StateModel* state_model_;
    const TransitionRegistry* transition_registry_;
    bool halted_{false};
    bool contained_{false};
};

}  // namespace ash
