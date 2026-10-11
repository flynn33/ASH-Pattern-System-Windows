#include "ash/RecoveryEngine.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <string>
#include "ash/Diagnostics.hpp"
#include "ash/RuleIds.hpp"

namespace ash {
namespace {
std::string utc_time() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm utc{};
    if (gmtime_s(&utc, &now) != 0) throw std::runtime_error("Safety entry UTC conversion failed");
    char buffer[32]{};
    if (!std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc))
        throw std::runtime_error("Safety entry UTC formatting failed");
    return buffer;
}
std::string trigger_name(ContainmentTrigger trigger) {
    switch (trigger) {
    case ContainmentTrigger::FALLBACK_FAILURE: return "FALLBACK_FAILURE";
    case ContainmentTrigger::PROPAGATION_RISK: return "PROPAGATION_RISK";
    case ContainmentTrigger::OPERATOR_REQUEST: return "OPERATOR_REQUEST";
    case ContainmentTrigger::RECOVERY_VALIDATION_FAILURE: return "RECOVERY_VALIDATION_FAILURE";
    }
    throw std::logic_error("Unknown containment trigger");
}
std::string trigger_name(SafeHaltTrigger trigger) {
    switch (trigger) {
    case SafeHaltTrigger::ESCALATION_FROM_FAILED: return "ESCALATION_FROM_FAILED";
    case SafeHaltTrigger::CONTAINMENT_BREACH: return "CONTAINMENT_BREACH";
    case SafeHaltTrigger::OPERATOR_HALT_REQUEST: return "OPERATOR_HALT_REQUEST";
    case SafeHaltTrigger::POLICY_HALT_REQUEST: return "POLICY_HALT_REQUEST";
    case SafeHaltTrigger::UNRESOLVABLE_BLOCKED_RECOVERY: return "UNRESOLVABLE_BLOCKED_RECOVERY";
    }
    throw std::logic_error("Unknown safe halt trigger");
}
OperationalContext built_in_context() {
    OperationalContext context;
    context.context_id = "aps-windows-built-in-codeword-context";
    context.context_version = "1.0";
    context.allowed_stable_states.assign(kCanonicalCodewords.begin(), kCanonicalCodewords.end());
    context.fallback_policy_instance.instance_id = "built-in-no-replacement";
    context.fallback_policy_instance.policy_id = "FALLBACK-STATE-999";
    context.propagation_risk_policy_defined = true;
    context.propagation_risk_permits_mutation = true;
    context.containment_policy_defined = true;
    context.halt_policy_defined = true;
    return context;
}
std::shared_ptr<DiagnosticEnvelope> action(DiagnosticKind kind, Severity severity,
    Stage stage, Disposition disposition, std::string_view subject,
    std::string_view rule, std::string_view summary, std::string_view note,
    std::shared_ptr<DiagnosticEnvelope> parent = {}, DiagnosticObject details = {}) {
    DiagnosticEnvelopeBuilder builder;
    if (parent && static_cast<unsigned>(parent->severity) > static_cast<unsigned>(severity)) severity = parent->severity;
    builder.kind(kind).severity(severity).stage(stage).disposition(disposition)
        .subject(subject).rule_id(rule).rule_id(rule_ids::kRecovery004).summary(summary).note(note).details(std::move(details));
    if (parent) return builder.parent(std::move(parent)).build_child();
    return builder.build_root();
}
}

std::shared_ptr<DiagnosticEnvelope> RecoveryEngine::build_halt_rejection_(std::string_view /*attempted*/) const {
    return halt_diagnostic_;
}

std::shared_ptr<DiagnosticEnvelope> RecoveryEngine::build_containment_rejection_(std::string_view attempted) {
    if (!contained_) return action(DiagnosticKind::RECOVERY, Severity::ERROR, Stage::RECOVERY,
        Disposition::BLOCKED, std::string("recovery-restricted::") + std::string(attempted),
        rule_ids::kRecovery003, "Repeated correction prohibited; original state preserved",
        "A prior correction failed. Only validated fallback or external safety escalation is allowed.", last_diagnostic_);
    last_diagnostic_ = action(DiagnosticKind::CONTAINMENT, Severity::ERROR, Stage::ESCALATION,
        Disposition::BLOCKED, std::string("contained::") + std::string(attempted),
        rule_ids::kContainment001, "Contained operation rejected; state preserved",
        "Restricted operations are inspect, diagnostics, explicit export, and safe halt. External resolution is required before mutation. Containment=" + (containment_diagnostic_ ? containment_diagnostic_->diagnostic_id : std::string("unknown")), last_diagnostic_,
        {{"attempted_operation", std::string(attempted)}, {"operation_prevented", true}, {"breach_confirmed", false}, {"propagation_observed", false}, {"system_state_class", "CONTAINED"}});
    containment_diagnostic_ = last_diagnostic_;
    return last_diagnostic_;
}

std::shared_ptr<DiagnosticEnvelope> RecoveryEngine::build_failed_rejection_(std::string_view attempted) const {
    return action(DiagnosticKind::RECOVERY, Severity::ERROR, Stage::RECOVERY,
        Disposition::BLOCKED, std::string("failed::") + std::string(attempted),
        rule_ids::kRecovery003, "Automated recovery prohibited in FAILED mode",
        "Original state and diagnostics are preserved while awaiting external authority. Diagnostic=" +
        (last_diagnostic_ ? last_diagnostic_->diagnostic_id : std::string("unknown")), last_diagnostic_);
}

std::shared_ptr<DiagnosticEnvelope> RecoveryEngine::observe_runtime_mode_(
    const StateAssessment& assessment, std::string_view attempted) {
    if (contained_ && assessment.runtime_mode != RuntimeMode::SAFE_HALT)
        return build_containment_rejection_(attempted);
    if (assessment.runtime_mode == RuntimeMode::NORMAL) return {};
    const bool valid_diagnostic = assessment.diagnostic && Diagnostics{}.validate_chain(*assessment.diagnostic);
    if (assessment.diagnostic && !valid_diagnostic)
        throw std::logic_error("Supplied runtime diagnostic chain is invalid; evidence was not replaced");
    const bool supplied_terminal = valid_diagnostic &&
        (assessment.diagnostic->disposition == Disposition::TERMINAL || assessment.diagnostic->diagnostic_kind == DiagnosticKind::SAFE_HALT ||
         assessment.diagnostic->stage == Stage::HALT || assessment.diagnostic->stage == Stage::TERMINAL);
    if (supplied_terminal && (assessment.runtime_mode != RuntimeMode::SAFE_HALT ||
        assessment.diagnostic->diagnostic_kind != DiagnosticKind::SAFE_HALT ||
        assessment.diagnostic->disposition != Disposition::TERMINAL ||
        (assessment.diagnostic->stage != Stage::HALT && assessment.diagnostic->stage != Stage::TERMINAL)))
        throw std::logic_error("Supplied terminal evidence is not a canonical SAFE_HALT action");
    // Supplied chains can describe a different observer's history. Preserve
    // both complete chains, without inventing ancestry between their roots.
    if (valid_diagnostic && last_diagnostic_.get() != assessment.diagnostic.get()) {
        if (last_diagnostic_) retained_diagnostic_chains_.push_back(last_diagnostic_);
        last_diagnostic_ = assessment.diagnostic;
    }
    if (assessment.runtime_mode == RuntimeMode::SAFE_HALT) {
        if (supplied_terminal) {
            auto adoption_time = utc_time();
            halt_diagnostic_ = assessment.diagnostic;
            last_diagnostic_ = halt_diagnostic_;
            halt_trigger_ = SafeHaltTrigger::POLICY_HALT_REQUEST;
            halt_time_ = std::move(adoption_time);
            halted_ = true;
        } else {
            (void)enter_safe_halt(SafeHaltTrigger::POLICY_HALT_REQUEST, "Supplied assessment is already in SAFE_HALT mode");
        }
        return build_halt_rejection_(attempted);
    }
    if (assessment.runtime_mode == RuntimeMode::CONTAINED) {
        if (!contained_) (void)enter_containment("Supplied assessment is already in containment mode");
        return build_containment_rejection_(attempted);
    }
    if (assessment.runtime_mode == RuntimeMode::FAILED) failed_ = true;
    return build_failed_rejection_(attempted);
}

std::shared_ptr<DiagnosticEnvelope> RecoveryEngine::prepare_escalation_parent_(std::string_view reason) {
    if (last_diagnostic_ && Diagnostics{}.validate_chain(*last_diagnostic_)) {
        const auto stage = last_diagnostic_->stage;
        if (stage == Stage::RECOVERY || stage == Stage::RECOVER || stage == Stage::FALLBACK ||
            stage == Stage::CLASSIFICATION || stage == Stage::ASSESS || stage == Stage::CLASSIFY ||
            stage == Stage::ESCALATION || stage == Stage::CONTAIN) return last_diagnostic_;
    }
    auto root = last_diagnostic_;
    last_diagnostic_ = action(DiagnosticKind::RECOVERY, Severity::WARNING, Stage::RECOVERY,
        Disposition::PENDING, "external-safety-request", rule_ids::kRecovery003,
        "Explicit safety action requested", std::string(reason) + (root ? "; supplied diagnostic evidence is preserved." : "; no source state or prior diagnostic was supplied; state validity is unknown."), std::move(root));
    return last_diagnostic_;
}

RecoveryResult RecoveryEngine::execute_normalize_state(const Bit9State& state) {
    if (halted_) return {state, build_halt_rejection_("execute_normalize_state"), RecoveryOutcome::TERMINAL_NO_RECOVERY};
    if (contained_) return {state, build_containment_rejection_("execute_normalize_state"), RecoveryOutcome::RECOVERY_FAILED};
    if (failed_) return {state, build_failed_rejection_("execute_normalize_state"), RecoveryOutcome::RECOVERY_FAILED};
    if (correction_failed_) return {state, build_containment_rejection_("execute_normalize_state"), RecoveryOutcome::RECOVERY_FAILED};
    auto result = state_model_->normalize(state);
    last_diagnostic_ = action(DiagnosticKind::RECOVERY, result.diagnostic->severity,
        Stage::RECOVERY, result.diagnostic->disposition, canonical_signature(state),
        rule_ids::kRecovery001, "Structural normalization completed without realm change",
        "Normalization is representation-only; it does not establish application health.", result.diagnostic);
    return {result.state, last_diagnostic_, RecoveryOutcome::RECOVERED};
}

RecoveryResult RecoveryEngine::execute_apply_correction(const Bit9State& state, const Codeword& correction) {
    if (halted_) return {state, build_halt_rejection_("execute_apply_correction"), RecoveryOutcome::TERMINAL_NO_RECOVERY};
    if (contained_) return {state, build_containment_rejection_("execute_apply_correction"), RecoveryOutcome::RECOVERY_FAILED};
    if (failed_) return {state, build_failed_rejection_("execute_apply_correction"), RecoveryOutcome::RECOVERY_FAILED};
    if (correction_failed_) return {state, build_containment_rejection_("execute_apply_correction"), RecoveryOutcome::RECOVERY_FAILED};
    auto transition = transition_registry_->apply_transition(state, correction);
    const bool success = transition.diagnostic->disposition == Disposition::RESOLVED &&
        state_model_->classify_system_state(transition.state) == SystemStateClass::STABLE;
    last_diagnostic_ = action(DiagnosticKind::RECOVERY, success ? Severity::INFO : Severity::ERROR,
        Stage::RECOVERY,
        success ? Disposition::RESOLVED : Disposition::ESCALATED,
        canonical_signature(state), rule_ids::kRecovery001,
        success ? "Correction validated under built-in codeword context" : "Correction failed; fallback required",
        "The transition disposition is checked before post-state classification. Failed attempts preserve the original state and cannot be retried automatically.", transition.diagnostic);
    correction_failed_ = !success;
    failed_correction_source_ = success ? std::nullopt : std::optional<Bit9State>{state};
    return {success ? transition.state : state, last_diagnostic_, success ? RecoveryOutcome::RECOVERED : RecoveryOutcome::RECOVERY_FAILED};
}

RecoveryResult RecoveryEngine::execute_correction(const StateAssessment& assessment, const OperationalContext& context) {
    if (!assessment.state) throw std::invalid_argument("Correction requires an observed source state; malformed input has no state to mutate");
    const Bit9State original = *assessment.state;
    if (halted_) return {original, build_halt_rejection_("execute_correction"), RecoveryOutcome::TERMINAL_NO_RECOVERY};
    if (auto rejection = observe_runtime_mode_(assessment, "execute_correction"))
        return {original, rejection, halted_ ? RecoveryOutcome::TERMINAL_NO_RECOVERY : RecoveryOutcome::RECOVERY_FAILED};
    if (failed_) return {original, build_failed_rejection_("execute_correction"), RecoveryOutcome::RECOVERY_FAILED};
    if (contained_) return {original, build_containment_rejection_("execute_correction"), RecoveryOutcome::RECOVERY_FAILED};
    if (correction_failed_) return {original, build_containment_rejection_("execute_correction"), RecoveryOutcome::RECOVERY_FAILED};
    auto current = state_model_->assess_state(original, context);
    if (!assessment.state || assessment.blocked || !state_model_->validate_context(context) ||
        assessment.operational_context_id != context.context_id || assessment.state_class != SystemStateClass::CORRECTABLE ||
        !assessment.selected_target || !assessment.correction_codeword_id ||
        current.state_class != SystemStateClass::CORRECTABLE || current.selected_target != assessment.selected_target ||
        current.candidate_targets != assessment.candidate_targets ||
        current.correction_codeword_id != assessment.correction_codeword_id ||
        !assessment.diagnostic || !Diagnostics{}.validate_chain(*assessment.diagnostic) ||
        assessment.diagnostic->diagnostic_id != current.diagnostic->diagnostic_id) {
        auto pre = assessment.diagnostic;
        last_diagnostic_ = action(DiagnosticKind::RECOVERY, Severity::ERROR, Stage::RECOVERY,
            Disposition::BLOCKED, canonical_signature(original), rule_ids::kRecovery003,
            "Correction assessment invalid or no longer matches context", "No state mutation occurred; fallback or external target resolution is required.", pre);
        correction_failed_ = true;
        failed_correction_source_ = original;
        return {original, last_diagnostic_, RecoveryOutcome::RECOVERY_FAILED};
    }
    if (!context.propagation_risk_permits_mutation) {
        last_diagnostic_ = assessment.diagnostic;
        auto diagnostic = enter_containment(ContainmentTrigger::PROPAGATION_RISK, "Propagation risk policy does not permit correction");
        return {original, diagnostic, RecoveryOutcome::ESCALATE_TO_CONTAINMENT};
    }
    const auto delta = original ^ *assessment.selected_target;
    if (*assessment.correction_codeword_id >= kCodewordCount || kCanonicalCodewords[*assessment.correction_codeword_id] != delta) {
        correction_failed_ = true;
        failed_correction_source_ = original;
        last_diagnostic_ = action(DiagnosticKind::RECOVERY, Severity::ERROR, Stage::RECOVERY,
            Disposition::BLOCKED, canonical_signature(original), rule_ids::kRecovery003,
            "Correction codeword does not match selected target", "No state mutation occurred.", assessment.diagnostic);
        return {original, last_diagnostic_, RecoveryOutcome::RECOVERY_FAILED};
    }
    auto transition = transition_registry_->apply_transition(original, delta);
    const auto post = state_model_->assess_state(transition.state, context);
    const bool success = transition.diagnostic->disposition == Disposition::RESOLVED &&
        transition.state == *assessment.selected_target && post.state_class == SystemStateClass::STABLE;
    last_diagnostic_ = action(DiagnosticKind::RECOVERY, success ? Severity::INFO : Severity::ERROR,
        Stage::RECOVERY, success ? Disposition::RESOLVED : Disposition::ESCALATED,
        canonical_signature(original), rule_ids::kRecovery001,
        success ? "Applied canonical correction to selected stable target" : "Correction post-validation failed",
        "Current=" + canonical_signature(original) + "; target=" + canonical_signature(*assessment.selected_target) +
        "; codeword=APS-CW-" + (*assessment.correction_codeword_id < 10 ? "0" : "") + std::to_string(*assessment.correction_codeword_id) +
        "; policy context=" + context.context_id + "; post=" + canonical_signature(transition.state), assessment.diagnostic);
    correction_failed_ = !success;
    failed_correction_source_ = success ? std::nullopt : std::optional<Bit9State>{original};
    return {success ? transition.state : original, last_diagnostic_, success ? RecoveryOutcome::RECOVERED : RecoveryOutcome::RECOVERY_FAILED};
}

FallbackResult RecoveryEngine::select_fallback(RecoveryCategory category) {
    if (halted_) return {std::nullopt, build_halt_rejection_("select_fallback"), {}, RecoveryOutcome::TERMINAL_NO_RECOVERY, {}};
    if (contained_) return {std::nullopt, build_containment_rejection_("select_fallback"), {}, RecoveryOutcome::RECOVERY_FAILED, {}};
    if (failed_) return {std::nullopt, build_failed_rejection_("select_fallback"), {}, RecoveryOutcome::RECOVERY_FAILED, {}};
    auto context = built_in_context();
    context.fallback_policy_instance.available = false;
    auto assessment = state_model_->assess_state(from_int(1), context);
    if (category != RecoveryCategory::FALLBACK_REQUIRED) {
        auto diagnostic = action(DiagnosticKind::FALLBACK, Severity::ERROR, Stage::RECOVERY,
            Disposition::BLOCKED, "select_fallback", rule_ids::kFallback001,
            "Fallback requested outside FALLBACK_REQUIRED", "No replacement selected and no state mutation occurred.", assessment.diagnostic);
        return {std::nullopt, diagnostic, {}, RecoveryOutcome::NOT_APPLICABLE, {}};
    }
    return select_fallback(assessment, context, context.fallback_policy_instance);
}

FallbackResult RecoveryEngine::select_fallback(const StateAssessment& assessment,
    const OperationalContext& context, const FallbackPolicyInstance& instance) {
    if (halted_) return {std::nullopt, build_halt_rejection_("select_fallback"), {}, RecoveryOutcome::TERMINAL_NO_RECOVERY, {}};
    if (auto rejection = observe_runtime_mode_(assessment, "select_fallback"))
        return {std::nullopt, rejection, instance.policy_id,
            halted_ ? RecoveryOutcome::TERMINAL_NO_RECOVERY : RecoveryOutcome::RECOVERY_FAILED, {}};
    if (failed_) return {std::nullopt, build_failed_rejection_("select_fallback"), {}, RecoveryOutcome::RECOVERY_FAILED, {}};
    if (contained_) return {std::nullopt, build_containment_rejection_("select_fallback"), {}, RecoveryOutcome::RECOVERY_FAILED, {}};
    auto parent = correction_failed_ && last_diagnostic_ ? last_diagnostic_ : assessment.diagnostic;
    auto escalate = [&](std::string_view reason) {
        last_diagnostic_ = action(DiagnosticKind::FALLBACK, Severity::ERROR, Stage::RECOVERY,
            Disposition::ESCALATED, "select_fallback", rule_ids::kFallback001,
            "No validated fallback replacement; containment required", reason, parent);
        auto fallback_diagnostic = last_diagnostic_;
        auto containment = enter_containment(ContainmentTrigger::FALLBACK_FAILURE, reason);
        return FallbackResult{std::nullopt, fallback_diagnostic, instance.policy_id,
            RecoveryOutcome::ESCALATE_TO_CONTAINMENT, containment};
    };
    const auto current = state_model_->assess_state(assessment.state.value_or(Bit9State{}), context);
    if (!assessment.state || assessment.blocked || current.blocked ||
        current.state_class != assessment.state_class || current.candidate_targets != assessment.candidate_targets ||
        current.selected_target != assessment.selected_target || current.correction_codeword_id != assessment.correction_codeword_id ||
        !assessment.diagnostic || !Diagnostics{}.validate_chain(*assessment.diagnostic) ||
        assessment.diagnostic->diagnostic_id != current.diagnostic->diagnostic_id ||
        (correction_failed_ && failed_correction_source_ != assessment.state) ||
        (assessment.state_class != SystemStateClass::DEGRADED &&
            !(correction_failed_ && (assessment.state_class == SystemStateClass::CORRECTABLE || assessment.state_class == SystemStateClass::UNSTABLE))) ||
        !state_model_->validate_context(context) || assessment.operational_context_id != context.context_id ||
        instance.instance_id != context.fallback_policy_instance.instance_id || instance.policy_id != context.fallback_policy_instance.policy_id ||
        instance.registry_version != "1.0" || !instance.available) return escalate("Fallback policy instance invalid or unavailable");
    if (instance.policy_id == "FALLBACK-STATE-999") return escalate("Canonical policy FALLBACK-STATE-999 selects containment without replacement");
    if (instance.policy_id != "FALLBACK-STATE-001" && instance.policy_id != "FALLBACK-STATE-002") return escalate("Unknown canonical policy identifier");
    if (!context.propagation_risk_permits_mutation) return escalate("Propagation risk policy does not permit fallback replacement");
    std::vector<const FallbackCandidate*> candidates;
    for (const auto& candidate : instance.candidates) if (candidate.applicable) candidates.push_back(&candidate);
    std::sort(candidates.begin(), candidates.end(), [&](const auto* a, const auto* b) {
        if (instance.policy_id == "FALLBACK-STATE-001" && a->configured_rank != b->configured_rank) return a->configured_rank < b->configured_rank;
        if (instance.policy_id == "FALLBACK-STATE-002" && a->verification_sequence != b->verification_sequence) return a->verification_sequence > b->verification_sequence;
        return to_int(a->state) < to_int(b->state);
    });
    for (const auto* candidate : candidates) {
        const auto post = state_model_->assess_state(candidate->state, context);
        const bool admissible_replacement = post.state_class == SystemStateClass::STABLE ||
            post.state_class == SystemStateClass::CORRECTABLE || post.state_class == SystemStateClass::UNSTABLE;
        if (!candidate->verification_valid || candidate->evidence_reference.empty() || post.blocked || !admissible_replacement) {
            parent = action(DiagnosticKind::FALLBACK, Severity::ERROR, Stage::RECOVERY,
                Disposition::BLOCKED, canonical_signature(candidate->state), rule_ids::kFallback001,
                "Fallback candidate failed post-selection validation",
                "Policy=" + instance.policy_id + "; candidate=" + canonical_signature(candidate->state) + "; verification evidence and STABLE, CORRECTABLE, or UNSTABLE immediate reassessment required.", parent);
            if (candidate->escalate_on_validation_failure) return escalate("Registry candidate requires immediate containment on validation failure");
            continue;
        }
        last_diagnostic_ = action(DiagnosticKind::FALLBACK, Severity::WARNING, Stage::RECOVERY,
            Disposition::RESOLVED, canonical_signature(*assessment.state), rule_ids::kFallback001,
            "Recovered via authorized fallback replacement",
            "Policy=" + instance.policy_id + "; instance=" + instance.instance_id + "; original=" + canonical_signature(*assessment.state) +
            "; replacement=" + canonical_signature(candidate->state) + "; evidence=" + candidate->evidence_reference +
            "; post-class=" + std::to_string(static_cast<unsigned>(*post.state_class)) +
            "; replacement is a policy action, not XOR-by-codeword motion. A non-STABLE replacement still requires its declared target-resolution or correction action.", parent,
            {{"original_state", canonical_signature(*assessment.state)}, {"replacement_state", canonical_signature(candidate->state)},
             {"fallback_policy_id", instance.policy_id}, {"evidence_reference", candidate->evidence_reference},
             {"post_state_class", post.state_class == SystemStateClass::STABLE ? "STABLE" : post.state_class == SystemStateClass::CORRECTABLE ? "CORRECTABLE" : "UNSTABLE"}});
        correction_failed_ = false;
        failed_correction_source_.reset();
        return {candidate->state, last_diagnostic_, instance.policy_id, RecoveryOutcome::RECOVERED_VIA_FALLBACK_REPLACEMENT, {}};
    }
    return escalate(candidates.empty() ? "No candidates in registry instance" : "All candidates failed immediate canonical reassessment");
}

std::shared_ptr<DiagnosticEnvelope> RecoveryEngine::enter_containment(std::string_view reason) {
    return enter_containment(ContainmentTrigger::OPERATOR_REQUEST, reason);
}

std::shared_ptr<DiagnosticEnvelope> RecoveryEngine::enter_containment(ContainmentTrigger trigger, std::string_view reason) {
    if (halted_) return build_halt_rejection_("enter_containment");
    if (contained_) return build_containment_rejection_("enter_containment");
    auto parent = prepare_escalation_parent_(reason);
    const auto trigger_text = trigger_name(trigger);
    auto entry_time = utc_time();
    containment_diagnostic_ = action(DiagnosticKind::CONTAINMENT, Severity::ERROR,
        Stage::ESCALATION, Disposition::BLOCKED, "enter_containment", rule_ids::kContainment001,
        "Entered containment; operations restricted", "Trigger=" + trigger_text + ": " + std::string(reason) + "; allowed operations: inspect, diagnostics, export, safe_halt; awaiting external resolution.", parent,
        {{"trigger", trigger_text}, {"system_state_class", "CONTAINED"}, {"awaiting_resolution", true}, {"resolution", "PENDING"},
         {"restricted_operations", DiagnosticValue::Array{"inspect", "diagnostics", "export", "safe_halt"}}});
    containment_trigger_ = trigger;
    containment_time_ = std::move(entry_time);
    contained_ = true;
    last_diagnostic_ = containment_diagnostic_;
    return last_diagnostic_;
}

std::shared_ptr<DiagnosticEnvelope> RecoveryEngine::enter_safe_halt(std::string_view reason) {
    return enter_safe_halt(SafeHaltTrigger::OPERATOR_HALT_REQUEST, reason);
}

std::shared_ptr<DiagnosticEnvelope> RecoveryEngine::enter_safe_halt(SafeHaltTrigger trigger, std::string_view reason) {
    if (halted_) return halt_diagnostic_;
    auto parent = prepare_escalation_parent_(reason);
    const auto trigger_text = trigger_name(trigger);
    auto entry_time = utc_time();
    if (parent->stage == Stage::CLASSIFICATION || parent->stage == Stage::ASSESS || parent->stage == Stage::CLASSIFY) {
        parent = action(DiagnosticKind::RECOVERY, Severity::ERROR, Stage::RECOVERY,
            Disposition::ESCALATED, "external-safety-escalation", rule_ids::kRecovery003,
            "Assessment requires terminal safety escalation", "No state mutation or fallback was attempted. Trigger=" + std::string(reason), parent);
    }
    halt_diagnostic_ = action(DiagnosticKind::SAFE_HALT, Severity::CRITICAL, Stage::TERMINAL,
        Disposition::TERMINAL, "enter_safe_halt", rule_ids::kHalt001,
        "Entered SAFE_HALT; no further transitions permitted", "Trigger=" + trigger_text + ": " + std::string(reason) + "; chain frozen and exportable.", parent,
        {{"trigger", trigger_text}, {"system_state_class", "SAFE_HALT"}, {"is_terminal", true}, {"frozen", true}});
    halt_trigger_ = trigger;
    halt_time_ = std::move(entry_time);
    halted_ = true;
    last_diagnostic_ = halt_diagnostic_;
    return last_diagnostic_;
}

std::shared_ptr<DiagnosticEnvelope> RecoveryEngine::breach_containment(std::string_view reason) {
    if (halted_) return build_halt_rejection_("breach_containment");
    if (!contained_) {
        last_diagnostic_ = action(DiagnosticKind::RECOVERY, Severity::ERROR, Stage::RECOVERY,
            Disposition::BLOCKED, "breach_containment", rule_ids::kContainment001,
            "Containment breach declaration rejected; containment is not active",
            "No active containment boundary exists in this engine. The declaration supplies no confirmed boundary violation: " + std::string(reason), last_diagnostic_,
            {{"containment_active", false}, {"breach_confirmed", false}, {"declaration_rejected", true}});
        return last_diagnostic_;
    }
    last_diagnostic_ = action(DiagnosticKind::CONTAINMENT, Severity::CRITICAL, Stage::ESCALATION,
        Disposition::ESCALATED, "breach_containment", rule_ids::kContainment001,
        "Confirmed containment boundary violation; terminal halt required",
        "Caller-confirmed propagation despite containment restrictions: " + std::string(reason), last_diagnostic_,
        {{"breach_confirmed", true}, {"operation_prevented", false}, {"propagation_observed", true}, {"evidence", std::string(reason)}, {"system_state_class", "CONTAINED"}});
    containment_diagnostic_ = last_diagnostic_;
    return enter_safe_halt(SafeHaltTrigger::CONTAINMENT_BREACH, "Containment boundary breached: " + std::string(reason));
}

std::optional<ContainmentDiagnostic> RecoveryEngine::containment_record() const {
    if (!containment_diagnostic_) return std::nullopt;
    auto chain = Diagnostics{}.expose_chain(containment_diagnostic_);
    if (chain.empty()) throw std::logic_error("Containment evidence chain invalid");
    return ContainmentDiagnostic{containment_trigger_, chain.front(), chain.back(), SystemStateClass::CONTAINED,
        std::move(chain), {"inspect", "diagnostics", "export", "safe_halt"}, containment_time_, !halted_,
        halted_ ? ContainmentResolution::SAFE_HALT : ContainmentResolution::PENDING};
}

std::optional<SafeHaltDiagnostic> RecoveryEngine::safe_halt_record() const {
    if (!halt_diagnostic_) return std::nullopt;
    auto chain = Diagnostics{}.expose_chain(halt_diagnostic_);
    if (chain.empty()) throw std::logic_error("Safe halt evidence chain invalid");
    return SafeHaltDiagnostic{halt_trigger_, chain.front(), chain.back(), SystemStateClass::SAFE_HALT,
        std::move(chain), halt_time_, true, true};
}
} // namespace ash
