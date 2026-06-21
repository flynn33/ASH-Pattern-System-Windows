// RecoveryEngine.cpp
//
// Implementation of the RecoveryEngine canonical module for the
// windows-cpp branch.
//
// Canonical source of truth:
//   specs/interfaces/contracts/recovery-engine-contract.md on main
//   specs/algorithms/recovery-fallback-semantics.pseudo.md on main
//   specs/algorithms/containment-safe-failure-semantics.pseudo.md on main
//
// Behavior summary:
//
//   execute_normalize_state(s)
//     - If halted: return ERROR diagnostic citing INV-RECOVERY-005.
//     - Otherwise: delegate to StateModel::normalize and wrap the
//       resulting diagnostic as a RECOVERY-stage child.
//
//   execute_apply_correction(s, c)
//     - If halted: return ERROR diagnostic.
//     - Otherwise: delegate to TransitionRegistry::apply_transition;
//       re-classify the result. If the result is not STABLE, emit a
//       RECOVERY_FAILED diagnostic at stage ESCALATION.
//
//   select_fallback(category)
//     - If halted: return ERROR diagnostic.
//     - Otherwise: observe the empty in-memory fallback registry and
//       return (nullopt, escalation diagnostic) with a note documenting
//       the empty-registry condition. This matches the canonical
//       recovery/fallback semantics: when the registry is unavailable
//       or empty, the correct behavior is to escalate to containment.
//       See deviation-log.md item 1.
//
//   enter_containment(reason)
//     - If halted: return ERROR diagnostic.
//     - Otherwise: set contained_ flag and emit a CONTAINMENT/
//       ESCALATION/BLOCKED envelope.
//
//   enter_safe_halt(reason)
//     - Set halted_ flag and emit a SAFE_HALT/TERMINAL/TERMINAL envelope.
//
// INV-RECOVERY-004 (no silent healing) is satisfied: every state
// mutation path returns a diagnostic. INV-RECOVERY-005 (SAFE_HALT is
// terminal) is enforced by the is_halted guard on every other method.

#include "ash/RecoveryEngine.hpp"

#include <string>

#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"

namespace ash {

namespace {

std::string state_as_text(const Bit9State& s) {
    std::string out;
    out.reserve(9);
    for (std::size_t i = 0; i < 9; ++i) {
        out.push_back(s.test(i) ? '1' : '0');
    }
    return out;
}

}  // namespace

std::shared_ptr<DiagnosticEnvelope>
RecoveryEngine::build_halt_rejection_(std::string_view attempted_action) const {
    // After safe halt, any subsequent action must be rejected with a
    // diagnostic. INV-RECOVERY-005. Note that we emit this as a root
    // diagnostic because the halted-engine rejection is a new
    // diagnostic chain reporting the attempted transition out of
    // SAFE_HALT, not a continuation of the chain that terminated
    // at SAFE_HALT.
    return DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::SAFE_HALT)
        .severity(Severity::ERROR)
        .stage(Stage::TERMINAL)
        .disposition(Disposition::TERMINAL)
        .subject(std::string{"post-halt::"} + std::string{attempted_action})
        .rule_id(rule_ids::kRecovery005)
        .rule_id(rule_ids::kHalt001)
        .summary("Rejected action after SAFE_HALT; transitions out of "
                 "SAFE_HALT are forbidden")
        .note("The engine is in SAFE_HALT. Per INV-RECOVERY-005 and "
              "specs/algorithms/containment-safe-failure-semantics.pseudo.md "
              "on main, no subsequent state transition is allowed.")
        .build_root();
}

RecoveryResult RecoveryEngine::execute_normalize_state(const Bit9State& state) {
    if (halted_) {
        return RecoveryResult{state, build_halt_rejection_("execute_normalize_state")};
    }

    auto inner = state_model_->normalize(state);
    // Wrap the inner diagnostic as a RECOVERY-stage child of the
    // StateModel's CLASSIFICATION-stage root. This preserves the
    // chain integrity rule (parent must be visible from child).
    auto wrapped =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::RECOVERY)
            .severity(Severity::INFO)
            .stage(Stage::RECOVERY)
            .disposition(Disposition::RESOLVED)
            .subject("execute_normalize_state[" + state_as_text(state) + "]")
            .parent(inner.diagnostic)
            .rule_id(rule_ids::kRecovery001)
            .rule_id(rule_ids::kRecovery004)
            .summary("RecoveryEngine.execute_normalize_state delegated to "
                     "StateModel.normalize")
            .note("Wrapping the StateModel.normalize diagnostic as a RECOVERY-"
                  "stage child. The engine does not mutate state silently.")
            .build_child();
    return RecoveryResult{inner.state, std::move(wrapped)};
}

RecoveryResult
RecoveryEngine::execute_apply_correction(const Bit9State& state,
                                         const Codeword& correction) {
    if (halted_) {
        return RecoveryResult{state, build_halt_rejection_("execute_apply_correction")};
    }

    auto inner = transition_registry_->apply_transition(state, correction);
    const auto post_class = state_model_->classify_system_state(inner.state);

    if (post_class == SystemStateClass::STABLE) {
        auto wrapped =
            DiagnosticEnvelopeBuilder{}
                .kind(DiagnosticKind::RECOVERY)
                .severity(Severity::INFO)
                .stage(Stage::RECOVERY)
                .disposition(Disposition::RESOLVED)
                .subject("execute_apply_correction[" + state_as_text(state) + "]")
                .parent(inner.diagnostic)
                .rule_id(rule_ids::kRecovery001)
                .rule_id(rule_ids::kRecovery004)
                .summary("Correction applied; post-state classifies as STABLE")
                .note("RecoveryEngine.execute_apply_correction wraps the "
                      "TransitionRegistry.apply_transition diagnostic; the "
                      "resulting state passes the STABLE re-classification.")
                .build_child();
        return RecoveryResult{inner.state, std::move(wrapped)};
    }

    // RECOVERY_FAILED: escalate.
    auto escalated =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::RECOVERY)
            .severity(Severity::ERROR)
            .stage(Stage::ESCALATION)
            .disposition(Disposition::ESCALATED)
            .subject("execute_apply_correction[" + state_as_text(state) + "]")
            .parent(inner.diagnostic)
            .rule_id(rule_ids::kRecovery003)
            .rule_id(rule_ids::kRecovery004)
            .summary("Correction applied but post-state is not STABLE; escalating")
            .note("RecoveryEngine.execute_apply_correction produced a post-"
                  "state that does not classify as STABLE. Per "
                  "recovery-fallback-semantics.pseudo.md the engine escalates.")
            .build_child();
    return RecoveryResult{inner.state, std::move(escalated)};
}

FallbackResult RecoveryEngine::select_fallback(RecoveryCategory category) {
    if (halted_) {
        return FallbackResult{std::nullopt, build_halt_rejection_("select_fallback")};
    }

    // Empty in-memory registry. Canonical behavior when the registry
    // is empty or unavailable is to escalate to containment.
    // See deviation-log.md item 1.
    (void)category;  // category is observed but the empty registry short-circuits
    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::FALLBACK)
            .severity(Severity::WARNING)
            .stage(Stage::ESCALATION)
            .disposition(Disposition::ESCALATED)
            .subject("select_fallback")
            .rule_id(rule_ids::kFallback001)
            .rule_id(rule_ids::kRecovery002)
            .rule_id(rule_ids::kRecovery004)
            .summary("Fallback policy registry is empty; escalating to containment")
            .note("Canonical recovery/fallback semantics prescribe escalation "
                  "to containment when the fallback registry is unavailable "
                  "or empty. The windows-cpp branch ships with an empty "
                  "in-memory registry. See "
                  "windows/conformance/deviation-log.md item 1.")
            .build_root();
    return FallbackResult{std::nullopt, std::move(diagnostic)};
}

std::shared_ptr<DiagnosticEnvelope>
RecoveryEngine::enter_containment(std::string_view reason) {
    if (halted_) {
        return build_halt_rejection_("enter_containment");
    }

    contained_ = true;
    return DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::CONTAINMENT)
        .severity(Severity::ERROR)
        .stage(Stage::ESCALATION)
        .disposition(Disposition::BLOCKED)
        .subject("enter_containment")
        .rule_id(rule_ids::kContainment001)
        .rule_id(rule_ids::kRecovery003)
        .rule_id(rule_ids::kRecovery004)
        .summary("Entered containment; operations restricted")
        .note(std::string{"Containment reason: "} + std::string{reason})
        .build_root();
}

std::shared_ptr<DiagnosticEnvelope>
RecoveryEngine::enter_safe_halt(std::string_view reason) {
    halted_ = true;
    return DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::SAFE_HALT)
        .severity(Severity::CRITICAL)
        .stage(Stage::TERMINAL)
        .disposition(Disposition::TERMINAL)
        .subject("enter_safe_halt")
        .rule_id(rule_ids::kHalt001)
        .rule_id(rule_ids::kRecovery005)
        .summary("Entered SAFE_HALT; no further transitions permitted")
        .note(std::string{"Safe-halt reason: "} + std::string{reason})
        .build_root();
}

}  // namespace ash
