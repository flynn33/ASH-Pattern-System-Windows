// RecoveryEngine.test.cpp
//
// Tests for the RecoveryEngine canonical module. Covers
// INV-RECOVERY-001..005 and INV-RECOVERY-004 (no silent healing).

#include "test_runner.hpp"

#include "ash/Codeword.hpp"
#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RecoveryEngine.hpp"
#include "ash/State.hpp"
#include "ash/StateModel.hpp"
#include "ash/TransitionRegistry.hpp"

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
    ASSERT_EQ(result.diagnostic->stage, Stage::ESCALATION);
    ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::FALLBACK);
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

TEST_CASE(post_halt_normalize_rejected_with_error_diagnostic) {
    // INV-RECOVERY-005: no transitions out of SAFE_HALT.
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    (void)engine.enter_safe_halt("test");
    auto result = engine.execute_normalize_state(from_int(0));
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_EQ(result.diagnostic->severity, Severity::ERROR);
    ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::SAFE_HALT);
}

TEST_CASE(post_halt_apply_correction_rejected) {
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    (void)engine.enter_safe_halt("test");
    auto result = engine.execute_apply_correction(from_int(0), kCanonicalCodewords[0]);
    ASSERT_EQ(result.diagnostic->severity, Severity::ERROR);
    ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::SAFE_HALT);
}

TEST_CASE(post_halt_select_fallback_rejected) {
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    (void)engine.enter_safe_halt("test");
    auto result = engine.select_fallback(RecoveryCategory::FALLBACK_REQUIRED);
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_EQ(result.diagnostic->severity, Severity::ERROR);
    ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::SAFE_HALT);
}

TEST_CASE(post_halt_enter_containment_rejected) {
    // Containment should also be unreachable after halt.
    StateModel sm;
    TransitionRegistry tr;
    auto engine = make_engine(sm, tr);
    (void)engine.enter_safe_halt("test");
    auto diag = engine.enter_containment("too late");
    ASSERT_EQ(diag->severity, Severity::ERROR);
    ASSERT_EQ(diag->diagnostic_kind, DiagnosticKind::SAFE_HALT);
}

int main() {
    return ash::test::run_all();
}
