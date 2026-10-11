// RecoveryEngine.test.cpp
//
// Tests for the RecoveryEngine canonical module. Covers
// INV-RECOVERY-001..005 and INV-RECOVERY-004 (no silent healing).

#include "test_runner.hpp"
#include <algorithm>

#include "ash/Codeword.hpp"
#include "ash/DiagnosticEnvelope.hpp"
#include "ash/Diagnostics.hpp"
#include "ash/RecoveryEngine.hpp"
#include "ash/State.hpp"
#include "ash/StateModel.hpp"
#include "ash/TransitionRegistry.hpp"
#include "ash/Version.hpp"

using namespace ash;

namespace {

RecoveryEngine make_engine(StateModel& sm, TransitionRegistry& tr) {
    return RecoveryEngine{sm, tr};
}

}  // namespace

TEST_CASE(execute_normalize_state_is_deterministic) {
    // INV-RECOVERY-001.
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    const Bit9State s = from_int(0x055);
    auto a = engine.execute_normalize_state(s);
    // After the first call the engine state is unchanged (no halt, no
    // containment), so the second call should produce an equivalent
    // result.
    auto b = engine.execute_normalize_state(s);
    ASSERT_EQ(a.state, b.state);
}

TEST_CASE(execute_normalize_state_produces_diagnostic) {
    // INV-RECOVERY-004 (no silent healing).
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    auto result = engine.execute_normalize_state(from_int(0));
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_TRUE(!result.diagnostic->rule_ids.empty());
    ASSERT_TRUE(!result.diagnostic->summary.empty());
}

TEST_CASE(execute_apply_correction_produces_diagnostic) {
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    auto result = engine.execute_apply_correction(from_int(0), kCanonicalCodewords[0]);
    ASSERT_TRUE(result.diagnostic != nullptr);
}

TEST_CASE(select_fallback_empty_registry_escalates_to_containment) {
    // Canonical behavior: empty registry => escalate to containment.
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    auto result = engine.select_fallback(RecoveryCategory::FALLBACK_REQUIRED);
    ASSERT_FALSE(result.state.has_value());  // nullopt: no candidate
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::ESCALATED);
    ASSERT_EQ(result.diagnostic->stage, Stage::RECOVERY);
    ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::FALLBACK);
    ASSERT_TRUE(engine.is_contained());
    ASSERT_TRUE(result.containment_diagnostic != nullptr);
    ASSERT_EQ(result.containment_diagnostic->stage, Stage::ESCALATION);
    ASSERT_EQ(result.containment_diagnostic->diagnostic_kind, DiagnosticKind::CONTAINMENT);
    ASSERT_EQ(result.containment_diagnostic->parent_diagnostic_reference, result.diagnostic);
    ASSERT_TRUE(Diagnostics{}.validate_chain(*result.containment_diagnostic));
}

TEST_CASE(select_fallback_never_silent) {
    // INV-RECOVERY-004.
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    auto result = engine.select_fallback(RecoveryCategory::FALLBACK_REQUIRED);
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_TRUE(!result.diagnostic->rule_ids.empty());
}

TEST_CASE(enter_containment_emits_canonical_diagnostic) {
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    auto diag = engine.enter_containment("propagation risk");
    ASSERT_TRUE(diag != nullptr);
    ASSERT_EQ(diag->diagnostic_kind, DiagnosticKind::CONTAINMENT);
    ASSERT_EQ(diag->stage, Stage::ESCALATION);
    ASSERT_EQ(diag->disposition, Disposition::BLOCKED);
}

TEST_CASE(enter_safe_halt_sets_halted_and_emits_terminal) {
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    ASSERT_FALSE(engine.is_halted());
    auto diag = engine.enter_safe_halt("unrecoverable failure");
    ASSERT_TRUE(engine.is_halted());
    ASSERT_TRUE(diag != nullptr);
    ASSERT_EQ(diag->diagnostic_kind, DiagnosticKind::SAFE_HALT);
    ASSERT_EQ(diag->stage, Stage::TERMINAL);
    ASSERT_EQ(diag->disposition, Disposition::TERMINAL);
    ASSERT_EQ(diag->severity, Severity::CRITICAL);
}

TEST_CASE(post_halt_normalize_returns_frozen_terminal_diagnostic) {
    // INV-RECOVERY-005: no transitions out of SAFE_HALT.
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    const auto frozen = engine.enter_safe_halt("test");
    auto result = engine.execute_normalize_state(from_int(0));
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_EQ(result.diagnostic.get(), frozen.get());
    ASSERT_EQ(result.diagnostic->severity, Severity::CRITICAL);
    ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::SAFE_HALT);
}

TEST_CASE(post_halt_apply_correction_rejected) {
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    const auto frozen = engine.enter_safe_halt("test");
    auto result = engine.execute_apply_correction(from_int(0), kCanonicalCodewords[0]);
    ASSERT_EQ(result.diagnostic.get(), frozen.get());
    ASSERT_EQ(result.diagnostic->severity, Severity::CRITICAL);
    ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::SAFE_HALT);
}

TEST_CASE(post_halt_select_fallback_rejected) {
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    const auto frozen = engine.enter_safe_halt("test");
    auto result = engine.select_fallback(RecoveryCategory::FALLBACK_REQUIRED);
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_EQ(result.diagnostic.get(), frozen.get());
    ASSERT_EQ(result.diagnostic->severity, Severity::CRITICAL);
    ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::SAFE_HALT);
}

TEST_CASE(post_halt_enter_containment_rejected) {
    // Containment should also be unreachable after halt.
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    const auto frozen = engine.enter_safe_halt("test");
    auto diag = engine.enter_containment("too late");
    ASSERT_EQ(diag.get(), frozen.get());
    ASSERT_EQ(diag->severity, Severity::CRITICAL);
    ASSERT_EQ(diag->diagnostic_kind, DiagnosticKind::SAFE_HALT);
}

namespace {
OperationalContext recovery_context(std::string policy = "FALLBACK-STATE-001") {
    OperationalContext context;
    context.context_id = "recovery-tests";
    context.context_version = "1.0";
    context.allowed_stable_states = {from_int(0), kCanonicalCodewords[1]};
    context.fallback_policy_instance.instance_id = "configured-instance";
    context.fallback_policy_instance.policy_id = std::move(policy);
    context.propagation_risk_policy_defined = true;
    context.propagation_risk_permits_mutation = true;
    context.containment_policy_defined = true;
    context.halt_policy_defined = true;
    return context;
}
}

TEST_CASE(blocked_correction_preserves_state_and_cannot_be_reported_as_recovered) {
    StateModel model;
    TransitionRegistry transitions;
    RecoveryEngine engine(model, transitions);
    auto result = engine.execute_apply_correction(from_int(0), from_int(1));
    ASSERT_EQ(result.state, from_int(0));
    ASSERT_EQ(result.outcome, RecoveryOutcome::RECOVERY_FAILED);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::ESCALATED);
    ASSERT_EQ(result.diagnostic->parent_diagnostic_reference->disposition, Disposition::BLOCKED);
    ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
    auto retry = engine.execute_apply_correction(from_int(0), kCanonicalCodewords[1]);
    ASSERT_EQ(retry.state, from_int(0));
    ASSERT_NE(retry.outcome, RecoveryOutcome::RECOVERED);
    const auto fallback = engine.select_fallback(RecoveryCategory::FALLBACK_REQUIRED);
    const auto fallback_chain = Diagnostics{}.expose_chain(fallback.diagnostic);
    ASSERT_TRUE(std::any_of(fallback_chain.begin(), fallback_chain.end(), [&](const auto& record) { return record.get() == result.diagnostic.get(); }));
    ASSERT_TRUE(Diagnostics{}.validate_chain(*fallback.containment_diagnostic));
}

TEST_CASE(containment_restricts_normalization_correction_and_fallback_and_halt_freezes_chain) {
    StateModel model;
    TransitionRegistry transitions;
    RecoveryEngine engine(model, transitions);
    auto context = recovery_context();
    auto assessment = model.assess_state(from_int(1), context);
    auto contained = engine.enter_containment("test-boundary");
    ASSERT_TRUE(engine.is_contained());
    ASSERT_EQ(engine.execute_normalize_state(from_int(1)).state, from_int(1));
    ASSERT_EQ(engine.execute_apply_correction(from_int(0), kCanonicalCodewords[1]).state, from_int(0));
    ASSERT_EQ(engine.execute_correction(assessment, context).state, from_int(1));
    ASSERT_FALSE(engine.select_fallback(assessment, context, context.fallback_policy_instance).state.has_value());
    const auto denied_tail = engine.diagnostic_chain();
    ASSERT_NE(denied_tail.get(), contained.get());
    const auto denied_chain = Diagnostics{}.expose_chain(denied_tail);
    ASSERT_EQ(denied_chain.size(), Diagnostics{}.expose_chain(contained).size() + 4);
    ASSERT_TRUE(std::any_of(denied_chain.begin(), denied_chain.end(), [&](const auto& record) { return record.get() == contained.get(); }));
    auto halt = engine.breach_containment("UNDECLARED_WRITE");
    ASSERT_TRUE(engine.is_halted());
    ASSERT_EQ(halt->parent_diagnostic_reference->parent_diagnostic_reference.get(), denied_tail.get());
    ASSERT_EQ(halt->parent_diagnostic_reference->diagnostic_kind, DiagnosticKind::CONTAINMENT);
    ASSERT_EQ(halt->parent_diagnostic_reference->disposition, Disposition::ESCALATED);
    ASSERT_TRUE(Diagnostics{}.validate_chain(*halt));
    ASSERT_EQ(engine.enter_safe_halt("repeat").get(), halt.get());
    ASSERT_EQ(engine.execute_correction(assessment, context).outcome, RecoveryOutcome::TERMINAL_NO_RECOVERY);
    ASSERT_EQ(engine.select_fallback(assessment, context, context.fallback_policy_instance).outcome, RecoveryOutcome::TERMINAL_NO_RECOVERY);
    ASSERT_EQ(engine.diagnostic_chain().get(), halt.get());
}

TEST_CASE(fallback_policy_order_and_realm_tie_break_are_deterministic) {
    StateModel model;
    TransitionRegistry transitions;
    for (const std::string policy : {std::string("FALLBACK-STATE-001"), std::string("FALLBACK-STATE-002")}) {
        auto context = recovery_context(policy);
        context.fallback_policy_instance.candidates = {{kCanonicalCodewords[1], 2, 5, true, true, "evidence-A"}, {from_int(0), 1, 9, true, true, "evidence-B"}};
        const auto assessment = model.assess_state(from_int(1), context);
        for (const bool reverse : {false, true}) {
            if (reverse) std::reverse(context.fallback_policy_instance.candidates.begin(), context.fallback_policy_instance.candidates.end());
            RecoveryEngine engine(model, transitions);
            const auto result = engine.select_fallback(assessment, context, context.fallback_policy_instance);
            ASSERT_EQ(result.state, std::optional<Bit9State>(from_int(0)));
            ASSERT_EQ(result.policy_id, policy);
            ASSERT_EQ(result.outcome, RecoveryOutcome::RECOVERED_VIA_FALLBACK_REPLACEMENT);
            ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
        }
        for (auto& candidate : context.fallback_policy_instance.candidates) { candidate.configured_rank = 1; candidate.verification_sequence = 1; }
        RecoveryEngine engine(model, transitions);
        ASSERT_EQ(engine.select_fallback(assessment, context, context.fallback_policy_instance).state, std::optional<Bit9State>(from_int(0)));
    }
}

TEST_CASE(fallback_validation_retries_only_authorized_candidates_and_preserves_escalation) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    context.fallback_policy_instance.candidates = {{from_int(0), 1, 1, true, false, "invalid-proof"}, {kCanonicalCodewords[1], 2, 2, true, true, "valid-proof"}};
    const auto assessment = model.assess_state(from_int(1), context);
    RecoveryEngine engine(model, transitions);
    const auto result = engine.select_fallback(assessment, context, context.fallback_policy_instance);
    ASSERT_EQ(result.state, std::optional<Bit9State>(kCanonicalCodewords[1]));
    ASSERT_EQ(result.diagnostic->parent_diagnostic_reference->disposition, Disposition::BLOCKED);
    ASSERT_EQ(result.diagnostic->severity, Severity::ERROR);
    ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
    context.fallback_policy_instance.candidates.front().escalate_on_validation_failure = true;
    RecoveryEngine immediate(model, transitions);
    const auto rejected = immediate.select_fallback(assessment, context, context.fallback_policy_instance);
    ASSERT_FALSE(rejected.state.has_value());
    ASSERT_TRUE(immediate.is_contained());
    ASSERT_TRUE(Diagnostics{}.validate_chain(*rejected.containment_diagnostic));
}

TEST_CASE(fallback_unavailable_empty_invalid_proof_and_unsafe_context_enter_containment) {
    StateModel model;
    TransitionRegistry transitions;
    for (int mode = 0; mode < 4; ++mode) {
        auto context = recovery_context();
        context.fallback_policy_instance.candidates = {{from_int(0), 1, 1, true, true, "proof"}};
        if (mode == 0) context.fallback_policy_instance.available = false;
        if (mode == 1) context.fallback_policy_instance.candidates.clear();
        if (mode == 2) context.fallback_policy_instance.candidates.front().evidence_reference.clear();
        if (mode == 3) context.propagation_risk_permits_mutation = false;
        const auto assessment = model.assess_state(from_int(1), context);
        RecoveryEngine engine(model, transitions);
        const auto result = engine.select_fallback(assessment, context, context.fallback_policy_instance);
        ASSERT_FALSE(result.state.has_value());
        ASSERT_TRUE(engine.is_contained());
        ASSERT_EQ(result.outcome, RecoveryOutcome::ESCALATE_TO_CONTAINMENT);
        ASSERT_TRUE(Diagnostics{}.validate_chain(*result.containment_diagnostic));
    }
}

TEST_CASE(fallback_immediate_reassessment_accepts_all_three_canonical_classes) {
    StateModel model;
    TransitionRegistry transitions;
    for (const bool multiple_targets : {false, true}) {
        auto context = recovery_context();
        context.allowed_stable_states = multiple_targets ? std::vector<Bit9State>{from_int(0), kCanonicalCodewords[1]} : std::vector<Bit9State>{from_int(0)};
        context.fallback_policy_instance.candidates = {{kCanonicalCodewords[2], 1, 1, true, true, "observed-replacement-evidence"}};
        const auto original = model.assess_state(from_int(1), context);
        ASSERT_EQ(original.state_class, SystemStateClass::DEGRADED);
        const auto expected = multiple_targets ? SystemStateClass::UNSTABLE : SystemStateClass::CORRECTABLE;
        ASSERT_EQ(model.assess_state(kCanonicalCodewords[2], context).state_class, expected);
        RecoveryEngine engine(model, transitions);
        const auto replacement = engine.select_fallback(original, context, context.fallback_policy_instance);
        ASSERT_EQ(replacement.state, std::optional<Bit9State>{kCanonicalCodewords[2]});
        ASSERT_EQ(replacement.outcome, RecoveryOutcome::RECOVERED_VIA_FALLBACK_REPLACEMENT);
        ASSERT_FALSE(engine.is_contained());
        ASSERT_TRUE(Diagnostics{}.validate_chain(*replacement.diagnostic));
        ASSERT_EQ(std::get<std::string>(replacement.diagnostic->details.at("post_state_class").value),
                  std::string(multiple_targets ? "UNSTABLE" : "CORRECTABLE"));
        auto correction_assessment = model.assess_state(*replacement.state, context);
        ASSERT_EQ(model.classify_recoverability(expected), multiple_targets ? RecoveryCategory::TARGET_RESOLUTION_REQUIRED : RecoveryCategory::APPLY_CORRECTION);
        if (!multiple_targets)
            ASSERT_EQ(engine.execute_correction(correction_assessment, context).state, from_int(0));
    }
}

TEST_CASE(correction_requires_assessment_target_consistent_with_current_context) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    context.allowed_stable_states = {from_int(0)};
    auto assessment = model.assess_state(kCanonicalCodewords[1], context);
    RecoveryEngine engine(model, transitions);
    auto success = engine.execute_correction(assessment, context);
    ASSERT_EQ(success.state, from_int(0));
    ASSERT_EQ(success.outcome, RecoveryOutcome::RECOVERED);
    ASSERT_TRUE(Diagnostics{}.validate_chain(*success.diagnostic));
    context.allowed_stable_states = {kCanonicalCodewords[2]};
    RecoveryEngine changed(model, transitions);
    auto rejected = changed.execute_correction(assessment, context);
    ASSERT_EQ(rejected.state, kCanonicalCodewords[1]);
    ASSERT_EQ(rejected.outcome, RecoveryOutcome::RECOVERY_FAILED);
}

TEST_CASE(fallback_rejects_forged_classification_and_target_metadata) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    context.fallback_policy_instance.candidates = {{from_int(0), 1, 1, true, true, "proof"}};
    for (int forgery = 0; forgery < 5; ++forgery) {
        auto assessment = model.assess_state(forgery == 0 ? from_int(0) : from_int(1), context);
        if (forgery == 0) assessment.state_class = SystemStateClass::DEGRADED;
        if (forgery == 1) assessment.candidate_targets.push_back(from_int(0));
        if (forgery == 2) assessment.selected_target = from_int(0);
        if (forgery == 3) assessment.correction_codeword_id = 0;
        if (forgery == 4) assessment.diagnostic = model.assess_state(from_int(0), context).diagnostic;
        RecoveryEngine engine(model, transitions);
        const auto rejected = engine.select_fallback(assessment, context, context.fallback_policy_instance);
        ASSERT_FALSE(rejected.state.has_value());
        ASSERT_EQ(rejected.outcome, RecoveryOutcome::ESCALATE_TO_CONTAINMENT);
        ASSERT_TRUE(engine.is_contained());
        ASSERT_TRUE(Diagnostics{}.validate_chain(*rejected.containment_diagnostic));
    }
}

TEST_CASE(runtime_restrictions_override_prior_correction_failure_and_remain_enforced) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    context.fallback_policy_instance.candidates = {{from_int(0), 1, 1, true, true, "proof"}};
    for (const auto mode : {RuntimeMode::CONTAINED, RuntimeMode::FAILED, RuntimeMode::SAFE_HALT}) {
        RecoveryEngine engine(model, transitions);
        ASSERT_EQ(engine.execute_apply_correction(from_int(1), from_int(1)).outcome, RecoveryOutcome::RECOVERY_FAILED);
        auto restricted = model.assess_state(from_int(1), context, mode);
        const auto fallback = engine.select_fallback(restricted, context, context.fallback_policy_instance);
        ASSERT_FALSE(fallback.state.has_value());
        ASSERT_NE(fallback.outcome, RecoveryOutcome::RECOVERED_VIA_FALLBACK_REPLACEMENT);
        const auto preserved = engine.diagnostic_chain();
        ASSERT_TRUE(preserved != nullptr);
        ASSERT_TRUE(Diagnostics{}.validate_chain(*preserved));
        auto normal = model.assess_state(from_int(1), context);
        ASSERT_FALSE(engine.select_fallback(normal, context, context.fallback_policy_instance).state.has_value());
        ASSERT_EQ(engine.execute_normalize_state(from_int(1)).state, from_int(1));
        ASSERT_NE(engine.execute_apply_correction(from_int(1), kCanonicalCodewords[1]).outcome, RecoveryOutcome::RECOVERED);
        const auto after_rejections = Diagnostics{}.expose_chain(engine.diagnostic_chain());
        ASSERT_TRUE(std::any_of(after_rejections.begin(), after_rejections.end(), [&](const auto& record) { return record.get() == preserved.get(); }));
        if (mode == RuntimeMode::SAFE_HALT) {
            ASSERT_TRUE(engine.is_halted());
            ASSERT_EQ(fallback.outcome, RecoveryOutcome::TERMINAL_NO_RECOVERY);
            ASSERT_EQ(preserved->disposition, Disposition::TERMINAL);
            ASSERT_EQ(engine.enter_safe_halt("repeat").get(), preserved.get());
        }
        if (mode == RuntimeMode::CONTAINED) ASSERT_TRUE(engine.is_contained());
    }
}

TEST_CASE(correction_rejects_non_normal_runtime_and_forged_candidate_targets) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    context.allowed_stable_states = {from_int(0)};
    for (const auto mode : {RuntimeMode::CONTAINED, RuntimeMode::FAILED, RuntimeMode::SAFE_HALT}) {
        RecoveryEngine engine(model, transitions);
        auto assessment = model.assess_state(kCanonicalCodewords[1], context, mode);
        // Even an altered CORRECTABLE label must not override the runtime restriction.
        assessment.state_class = SystemStateClass::CORRECTABLE;
        assessment.selected_target = from_int(0);
        assessment.correction_codeword_id = 1;
        const auto result = engine.execute_correction(assessment, context);
        ASSERT_EQ(result.state, kCanonicalCodewords[1]);
        ASSERT_NE(result.outcome, RecoveryOutcome::RECOVERED);
    }
    auto assessment = model.assess_state(kCanonicalCodewords[1], context);
    assessment.candidate_targets.clear();
    RecoveryEngine engine(model, transitions);
    ASSERT_NE(engine.execute_correction(assessment, context).outcome, RecoveryOutcome::RECOVERED);
}

TEST_CASE(prior_correction_failure_does_not_authorize_an_unrelated_state_fallback) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    context.allowed_stable_states = {from_int(0)};
    context.fallback_policy_instance.candidates = {{from_int(0), 1, 1, true, true, "proof"}};
    RecoveryEngine engine(model, transitions);
    ASSERT_EQ(engine.execute_apply_correction(from_int(1), from_int(1)).outcome, RecoveryOutcome::RECOVERY_FAILED);
    const auto unrelated = model.assess_state(kCanonicalCodewords[1], context);
    ASSERT_EQ(unrelated.state_class, SystemStateClass::CORRECTABLE);
    ASSERT_FALSE(engine.select_fallback(unrelated, context, context.fallback_policy_instance).state.has_value());
    ASSERT_TRUE(engine.is_contained());
}

TEST_CASE(correction_rejects_valid_but_unrelated_assessment_diagnostic) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    context.allowed_stable_states = {from_int(0)};
    auto assessment = model.assess_state(kCanonicalCodewords[1], context);
    assessment.diagnostic = model.assess_state(from_int(0), context).diagnostic;
    ASSERT_TRUE(Diagnostics{}.validate_chain(*assessment.diagnostic));
    RecoveryEngine engine(model, transitions);
    ASSERT_EQ(engine.execute_correction(assessment, context).state, kCanonicalCodewords[1]);
}

TEST_CASE(external_terminal_adoption_preserves_both_independent_chains) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    RecoveryEngine external(model, transitions), target(model, transitions);
    const auto terminal = external.enter_safe_halt("External owner-directed halt");
    const auto frozen_bytes = Diagnostics{}.serialize_chain(terminal);
    const auto prior = target.execute_apply_correction(from_int(0), from_int(1)).diagnostic;
    const auto prior_bytes = Diagnostics{}.serialize_chain(prior);
    auto assessment = model.assess_state(from_int(1), context, RuntimeMode::SAFE_HALT);
    assessment.diagnostic = terminal;
    const auto denied = target.select_fallback(assessment, context, context.fallback_policy_instance);
    ASSERT_EQ(denied.diagnostic.get(), terminal.get());
    ASSERT_EQ(target.diagnostic_chain().get(), terminal.get());
    ASSERT_EQ(target.retained_diagnostic_chains().size(), std::size_t{1});
    ASSERT_EQ(Diagnostics{}.serialize_chain(target.retained_diagnostic_chains().front()), prior_bytes);
    ASSERT_EQ(Diagnostics{}.serialize_chain(target.diagnostic_chain()), frozen_bytes);
    const auto record = *target.safe_halt_record();
    ASSERT_EQ(record.trigger, SafeHaltTrigger::POLICY_HALT_REQUEST);
    ASSERT_TRUE(!record.halt_time.empty());
    const auto bytes = Diagnostics{}.serialize_transport(record, ASH_SDK_VERSION);
    ASSERT_EQ(target.execute_normalize_state(from_int(1)).diagnostic.get(), terminal.get());
    ASSERT_EQ(Diagnostics{}.serialize_transport(*target.safe_halt_record(), ASH_SDK_VERSION), bytes);
    std::printf("[JSON_SAFE_HALT] %s\n", bytes.c_str());
    std::printf("[JSON_RETAINED_CHAIN] %s\n", prior_bytes.c_str());
}

TEST_CASE(modern_containment_ancestry_is_retained_and_blocked_attempt_is_not_breach) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    auto root = DiagnosticEnvelopeBuilder{}.kind(DiagnosticKind::STATE_ASSESSMENT).severity(Severity::WARNING)
        .stage(Stage::ASSESS).disposition(Disposition::PENDING).subject("supplied-context").rule_id("ASH-STATE-001")
        .summary("External assessment requests containment").note("Observed propagation risk requires restricted operation").build_root();
    auto contained = DiagnosticEnvelopeBuilder{}.kind(DiagnosticKind::CONTAINMENT).severity(Severity::ERROR)
        .stage(Stage::CONTAIN).disposition(Disposition::BLOCKED).subject("supplied-context").rule_id("ASH-CONTAINMENT-001")
        .summary("External containment is active").note("No propagation has crossed the containment boundary").parent(root).build_child();
    auto assessment = model.assess_state(from_int(1), context, RuntimeMode::CONTAINED);
    assessment.diagnostic = contained;
    RecoveryEngine engine(model, transitions);
    const auto denied = engine.select_fallback(assessment, context, context.fallback_policy_instance);
    const auto chain = Diagnostics{}.expose_chain(denied.diagnostic);
    ASSERT_EQ(chain.front().get(), root.get());
    ASSERT_EQ(chain[1].get(), contained.get());
    ASSERT_TRUE(engine.is_contained());
    ASSERT_FALSE(engine.is_halted());
    ASSERT_EQ(std::get<bool>(denied.diagnostic->details.at("operation_prevented").value), true);
    ASSERT_EQ(std::get<bool>(denied.diagnostic->details.at("breach_confirmed").value), false);
    std::printf("[JSON_CONTAINMENT] %s\n", Diagnostics{}.serialize_transport(*engine.containment_record(), ASH_SDK_VERSION).c_str());
}

TEST_CASE(malformed_input_fallback_preserves_parse_failure_without_fabricated_state) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    const auto assessment = model.assess_state("malformed", &context);
    ASSERT_FALSE(assessment.state.has_value());
    RecoveryEngine engine(model, transitions);
    const auto rejected = engine.select_fallback(assessment, context, context.fallback_policy_instance);
    const auto chain = Diagnostics{}.expose_chain(rejected.containment_diagnostic);
    ASSERT_EQ(chain.front().get(), assessment.diagnostic.get());
    ASSERT_EQ(chain.size(), std::size_t{3});
    ASSERT_EQ(engine.containment_record()->trigger, ContainmentTrigger::FALLBACK_FAILURE);
    std::printf("[JSON_CONTAINMENT] %s\n", Diagnostics{}.serialize_transport(*engine.containment_record(), ASH_SDK_VERSION).c_str());
}

TEST_CASE(safety_requests_without_state_record_explicitly_unknown_source) {
    StateModel model;
    TransitionRegistry transitions;
    RecoveryEngine engine(model, transitions);
    const auto record = engine.enter_containment(ContainmentTrigger::PROPAGATION_RISK, "Unobserved source; risk reported by operator");
    const auto chain = Diagnostics{}.expose_chain(record);
    ASSERT_EQ(chain.size(), std::size_t{2});
    ASSERT_EQ(chain.front()->subject_reference, std::string("external-safety-request"));
    ASSERT_TRUE(chain.front()->notes[0].find("state validity is unknown") != std::string::npos);
    ASSERT_EQ(engine.containment_record()->trigger, ContainmentTrigger::PROPAGATION_RISK);
}

TEST_CASE(confirmed_breach_exports_full_terminal_snapshot_and_freezes_it) {
    StateModel model;
    TransitionRegistry transitions;
    RecoveryEngine engine(model, transitions);
    ASSERT_FALSE(engine.containment_record().has_value());
    ASSERT_FALSE(engine.safe_halt_record().has_value());
    (void)engine.enter_containment(ContainmentTrigger::RECOVERY_VALIDATION_FAILURE, "Caller supplied invalid recovery evidence");
    const auto contained = *engine.containment_record();
    const auto containment_time = contained.containment_time;
    std::printf("[JSON_CONTAINMENT] %s\n", Diagnostics{}.serialize_transport(contained, ASH_SDK_VERSION).c_str());
    const auto halt = engine.breach_containment("Observed error crossed declared boundary at write gate");
    const auto record = *engine.safe_halt_record();
    ASSERT_EQ(record.trigger, SafeHaltTrigger::CONTAINMENT_BREACH);
    ASSERT_EQ(record.full_diagnostic_chain.size(), contained.full_diagnostic_chain.size() + 2);
    ASSERT_EQ(record.original_diagnostic.get(), contained.original_diagnostic.get());
    ASSERT_EQ(std::get<bool>(halt->parent_diagnostic_reference->details.at("breach_confirmed").value), true);
    ASSERT_TRUE(record.is_terminal && record.frozen);
    const auto bytes = Diagnostics{}.serialize_transport(record, ASH_SDK_VERSION);
    ASSERT_EQ(engine.enter_containment("attempt de-escalation").get(), halt.get());
    ASSERT_EQ(engine.enter_safe_halt("attempt repeat").get(), halt.get());
    ASSERT_EQ(engine.breach_containment("attempt repeat").get(), halt.get());
    ASSERT_EQ(Diagnostics{}.serialize_transport(*engine.safe_halt_record(), ASH_SDK_VERSION), bytes);
    ASSERT_EQ(engine.containment_record()->containment_time, containment_time);
    ASSERT_FALSE(engine.containment_record()->awaiting_resolution);
    ASSERT_EQ(engine.containment_record()->resolution, ContainmentResolution::SAFE_HALT);
    std::printf("[JSON_SAFE_HALT] %s\n", bytes.c_str());
    std::printf("[JSON_CONTAINMENT] %s\n", Diagnostics{}.serialize_transport(*engine.containment_record(), ASH_SDK_VERSION).c_str());
}

TEST_CASE(imported_runtime_evidence_is_retained_and_invalid_actions_are_rejected) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    for (const auto mode : {RuntimeMode::CONTAINED, RuntimeMode::FAILED, RuntimeMode::SAFE_HALT}) {
        RecoveryEngine engine(model, transitions);
        const auto prior = engine.execute_apply_correction(from_int(0), from_int(1)).diagnostic;
        auto supplied = model.assess_state(from_int(1), context, mode);
        const auto observed = supplied.diagnostic;
        (void)engine.select_fallback(supplied, context, context.fallback_policy_instance);
        const auto active = Diagnostics{}.expose_chain(engine.diagnostic_chain());
        ASSERT_TRUE(std::any_of(active.begin(), active.end(), [&](const auto& record) { return record.get() == observed.get(); }));
        ASSERT_EQ(engine.retained_diagnostic_chains().size(), std::size_t{1});
        ASSERT_EQ(engine.retained_diagnostic_chains()[0].get(), prior.get());
        RecoveryEngine invalid(model, transitions);
        const auto stable_prior = invalid.execute_normalize_state(from_int(1)).diagnostic;
        supplied.diagnostic->summary += " (stale identity)";
        bool rejected = false;
        try { (void)invalid.select_fallback(supplied, context, context.fallback_policy_instance); }
        catch (const std::logic_error&) { rejected = true; }
        ASSERT_TRUE(rejected);
        ASSERT_FALSE(invalid.is_contained());
        ASSERT_FALSE(invalid.is_halted());
        ASSERT_EQ(invalid.diagnostic_chain().get(), stable_prior.get());
    }
    auto assessment = model.assess_state(from_int(1), context, RuntimeMode::SAFE_HALT);
    assessment.diagnostic = DiagnosticEnvelopeBuilder{}.kind(DiagnosticKind::STATE_VALIDITY).stage(Stage::RECOVERY)
        .severity(Severity::ERROR).disposition(Disposition::TERMINAL).subject("invalid-halt-action")
        .rule_id("ASH-STATE-001").summary("Frozen state evidence without a halt action")
        .note("This valid frozen envelope does not assert the required SAFE_HALT action").build_root();
    ASSERT_TRUE(Diagnostics{}.validate_chain(*assessment.diagnostic));
    RecoveryEngine engine(model, transitions);
    bool rejected = false;
    try { (void)engine.select_fallback(assessment, context, context.fallback_policy_instance); }
    catch (const std::logic_error&) { rejected = true; }
    ASSERT_TRUE(rejected);
    ASSERT_FALSE(engine.is_halted());
    ASSERT_FALSE(engine.safe_halt_record().has_value());
    const auto missing = model.assess_state("malformed", &context);
    rejected = false;
    try { (void)engine.execute_correction(missing, context); }
    catch (const std::invalid_argument&) { rejected = true; }
    ASSERT_TRUE(rejected);
    ASSERT_TRUE(engine.diagnostic_chain() == nullptr);
}

TEST_CASE(safety_serialization_rejects_tampered_projections) {
    StateModel model;
    TransitionRegistry transitions;
    RecoveryEngine engine(model, transitions);
    (void)engine.enter_containment("Test restricted operations");
    const auto original = *engine.containment_record();
    for (int mutation = 0; mutation < 8; ++mutation) {
        auto record = original;
        if (mutation == 0) record.full_diagnostic_chain.clear();
        if (mutation == 1) record.original_diagnostic = record.diagnostic;
        if (mutation == 2) record.containment_time.clear();
        if (mutation == 3) record.system_state_class = SystemStateClass::STABLE;
        if (mutation == 4) record.trigger = static_cast<ContainmentTrigger>(99);
        if (mutation == 5) record.resolution = static_cast<ContainmentResolution>(99);
        if (mutation == 6) record.awaiting_resolution = false;
        if (mutation == 7) record.restricted_operations.push_back("  ");
        bool rejected = false;
        try { (void)Diagnostics{}.serialize_transport(record, ASH_SDK_VERSION); } catch (const std::logic_error&) { rejected = true; }
        ASSERT_TRUE(rejected);
    }
    (void)engine.enter_safe_halt(SafeHaltTrigger::UNRESOLVABLE_BLOCKED_RECOVERY, "No authorized recovery path remains");
    const auto terminal = *engine.safe_halt_record();
    for (int mutation = 0; mutation < 6; ++mutation) {
        auto record = terminal;
        if (mutation == 0) record.frozen = false;
        if (mutation == 1) record.is_terminal = false;
        if (mutation == 2) record.halt_time.clear();
        if (mutation == 3) record.system_state_class = SystemStateClass::FAILED;
        if (mutation == 4) record.trigger = static_cast<SafeHaltTrigger>(99);
        if (mutation == 5) record.full_diagnostic_chain.erase(record.full_diagnostic_chain.begin());
        bool rejected = false;
        try { (void)Diagnostics{}.serialize_transport(record, ASH_SDK_VERSION); } catch (const std::logic_error&) { rejected = true; }
        ASSERT_TRUE(rejected);
    }
    std::printf("[JSON_SAFE_HALT] %s\n", Diagnostics{}.serialize_transport(terminal, ASH_SDK_VERSION).c_str());
}

TEST_CASE(breach_declaration_requires_an_active_containment_boundary) {
    StateModel model;
    TransitionRegistry transitions;
    RecoveryEngine engine(model, transitions);
    const auto rejected = engine.breach_containment("Caller attempted a declaration without an established boundary");
    ASSERT_TRUE(Diagnostics{}.validate_chain(*rejected));
    ASSERT_EQ(rejected->diagnostic_kind, DiagnosticKind::RECOVERY);
    ASSERT_EQ(rejected->disposition, Disposition::BLOCKED);
    ASSERT_FALSE(std::get<bool>(rejected->details.at("containment_active").value));
    ASSERT_FALSE(std::get<bool>(rejected->details.at("breach_confirmed").value));
    ASSERT_TRUE(rejected->notes.front().find("prior correction") == std::string::npos);
    ASSERT_FALSE(engine.is_contained());
    ASSERT_FALSE(engine.is_halted());
    ASSERT_FALSE(engine.containment_record().has_value());
    ASSERT_FALSE(engine.safe_halt_record().has_value());
    ASSERT_EQ(engine.diagnostic_chain().get(), rejected.get());
    const auto corrected = engine.execute_apply_correction(from_int(0), from_int(0));
    ASSERT_EQ(corrected.outcome, RecoveryOutcome::RECOVERED);
}

TEST_CASE(explicit_containment_after_failed_mode_preserves_restrictions_and_chain_phase) {
    StateModel model;
    TransitionRegistry transitions;
    auto context = recovery_context();
    RecoveryEngine engine(model, transitions);
    const auto failed = model.assess_state(from_int(1), context, RuntimeMode::FAILED);
    (void)engine.select_fallback(failed, context, context.fallback_policy_instance);
    const auto entered = engine.enter_containment("Operator establishes containment after failed recovery");
    ASSERT_TRUE(engine.is_contained());
    const auto verify_restriction = [&](const std::shared_ptr<DiagnosticEnvelope>& diagnostic) {
        ASSERT_TRUE(Diagnostics{}.validate_chain(*diagnostic));
        ASSERT_EQ(diagnostic->diagnostic_kind, DiagnosticKind::CONTAINMENT);
        ASSERT_EQ(diagnostic->disposition, Disposition::BLOCKED);
        const auto chain = Diagnostics{}.expose_chain(diagnostic);
        ASSERT_TRUE(std::any_of(chain.begin(), chain.end(), [&](const auto& record) { return record.get() == entered.get(); }));
        ASSERT_TRUE(std::get<bool>(diagnostic->details.at("operation_prevented").value));
        ASSERT_FALSE(std::get<bool>(diagnostic->details.at("breach_confirmed").value));
    };
    const auto normalized = engine.execute_normalize_state(from_int(1));
    ASSERT_EQ(normalized.state, from_int(1));
    verify_restriction(normalized.diagnostic);
    const auto legacy_correction = engine.execute_apply_correction(from_int(1), from_int(0));
    ASSERT_EQ(legacy_correction.state, from_int(1));
    verify_restriction(legacy_correction.diagnostic);
    verify_restriction(engine.select_fallback(RecoveryCategory::FALLBACK_REQUIRED).diagnostic);
    const auto normal = model.assess_state(from_int(1), context);
    const auto correction = engine.execute_correction(normal, context);
    ASSERT_EQ(correction.state, from_int(1));
    verify_restriction(correction.diagnostic);
    verify_restriction(engine.select_fallback(normal, context, context.fallback_policy_instance).diagnostic);
    verify_restriction(engine.select_fallback(failed, context, context.fallback_policy_instance).diagnostic);
    ASSERT_FALSE(engine.is_halted());
    ASSERT_TRUE(engine.containment_record()->awaiting_resolution);
    auto terminal = model.assess_state(from_int(1), context, RuntimeMode::SAFE_HALT);
    RecoveryEngine external(model, transitions);
    terminal.diagnostic = external.enter_safe_halt("External operator provides a terminal action for adoption");
    const auto halted = engine.select_fallback(terminal, context, context.fallback_policy_instance);
    ASSERT_EQ(halted.outcome, RecoveryOutcome::TERMINAL_NO_RECOVERY);
    ASSERT_TRUE(engine.is_halted());
    ASSERT_EQ(halted.diagnostic.get(), terminal.diagnostic.get());
    ASSERT_TRUE(Diagnostics{}.validate_chain(*halted.diagnostic));
}

int main() {
    return ash::test::run_all();
}
