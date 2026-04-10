// StateModel.test.cpp
//
// Tests for the StateModel canonical module. Covers INV-STATE-*,
// INV-ADMISSIBILITY-*, and parts of INV-RECOVERY-001 (classification
// determinism).
//
// Each test case maps to one or more canonical invariants documented
// in specs/verification/invariant-spec.md on main.

#include "test_runner.hpp"

#include "ash/Codeword.hpp"
#include "ash/DiagnosticEnvelope.hpp"
#include "ash/State.hpp"
#include "ash/StateModel.hpp"

using namespace ash;

TEST_CASE(classify_admissibility_all_16_codewords_are_valid) {
    // INV-CODEWORD-004, INV-ADMISSIBILITY-001: every canonical codeword
    // classifies as VALID.
    StateModel sm;
    for (const auto& c : kCanonicalCodewords) {
        ASSERT_EQ(sm.classify_admissibility(c), AdmissibilityStatus::VALID);
    }
}

TEST_CASE(classify_admissibility_is_deterministic) {
    // INV-ADMISSIBILITY-001: identical input => identical status.
    StateModel sm;
    const Bit9State s = from_int(0x123);
    const auto first = sm.classify_admissibility(s);
    const auto second = sm.classify_admissibility(s);
    ASSERT_EQ(first, second);
}

TEST_CASE(classify_admissibility_is_total_over_512_states) {
    // INV-ADMISSIBILITY-002: every 9-bit vector maps to exactly one
    // of the four admissibility statuses. On this branch with a
    // fully-closed codeword set, every state classifies as VALID or
    // TRANSFORMATION_COMPATIBLE — no state should ever be UNCLASSIFIED.
    StateModel sm;
    for (std::uint16_t i = 0; i < kStateSpaceSize; ++i) {
        const Bit9State s = from_int(i);
        const auto status = sm.classify_admissibility(s);
        ASSERT_TRUE(status == AdmissibilityStatus::VALID ||
                    status == AdmissibilityStatus::TRANSFORMATION_COMPATIBLE);
    }
}

TEST_CASE(classify_admissibility_16_of_512_are_valid) {
    // Exactly 16 states are VALID (one per canonical codeword).
    StateModel sm;
    int valid_count = 0;
    for (std::uint16_t i = 0; i < kStateSpaceSize; ++i) {
        const Bit9State s = from_int(i);
        if (sm.classify_admissibility(s) == AdmissibilityStatus::VALID) {
            ++valid_count;
        }
    }
    ASSERT_EQ(valid_count, 16);
}

TEST_CASE(classify_system_state_total_over_512_states) {
    // Every state maps to a system-state class with no exceptions.
    StateModel sm;
    for (std::uint16_t i = 0; i < kStateSpaceSize; ++i) {
        const Bit9State s = from_int(i);
        const auto cls = sm.classify_system_state(s);
        ASSERT_TRUE(cls == SystemStateClass::STABLE ||
                    cls == SystemStateClass::UNSTABLE ||
                    cls == SystemStateClass::CORRECTABLE ||
                    cls == SystemStateClass::DEGRADED ||
                    cls == SystemStateClass::CONTAINED ||
                    cls == SystemStateClass::FAILED ||
                    cls == SystemStateClass::SAFE_HALT);
    }
}

TEST_CASE(classify_recoverability_maps_all_7_classes) {
    // Deterministic mapping from class to category per the canonical
    // table in specs/core/recoverability-semantics.pseudo.md.
    StateModel sm;
    ASSERT_EQ(sm.classify_recoverability(SystemStateClass::STABLE),
              RecoveryCategory::NO_ACTION);
    ASSERT_EQ(sm.classify_recoverability(SystemStateClass::UNSTABLE),
              RecoveryCategory::NORMALIZE_STATE);
    ASSERT_EQ(sm.classify_recoverability(SystemStateClass::CORRECTABLE),
              RecoveryCategory::APPLY_CORRECTION);
    ASSERT_EQ(sm.classify_recoverability(SystemStateClass::DEGRADED),
              RecoveryCategory::FALLBACK_REQUIRED);
    ASSERT_EQ(sm.classify_recoverability(SystemStateClass::CONTAINED),
              RecoveryCategory::CONTAINMENT_REQUIRED);
    ASSERT_EQ(sm.classify_recoverability(SystemStateClass::FAILED),
              RecoveryCategory::ESCALATION_REQUIRED);
    ASSERT_EQ(sm.classify_recoverability(SystemStateClass::SAFE_HALT),
              RecoveryCategory::TERMINAL_NO_RECOVERY);
}

TEST_CASE(normalize_valid_state_is_identity_with_resolved_diagnostic) {
    // INV-STATE-002, INV-STATE-003.
    StateModel sm;
    const Bit9State valid_state = kCanonicalCodewords[0];  // zero codeword
    auto result = sm.normalize(valid_state);
    ASSERT_EQ(result.state, valid_state);
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::RESOLVED);
    ASSERT_EQ(result.diagnostic->stage, Stage::CLASSIFICATION);
    ASSERT_TRUE(!result.diagnostic->rule_ids.empty());
}

TEST_CASE(normalize_always_produces_a_diagnostic) {
    // INV-STATE-003: no silent result.
    StateModel sm;
    for (std::uint16_t i = 0; i < 32; ++i) {  // sample 32 states
        const Bit9State s = from_int(i);
        auto result = sm.normalize(s);
        ASSERT_TRUE(result.diagnostic != nullptr);
        ASSERT_TRUE(!result.diagnostic->summary.empty());
        ASSERT_TRUE(!result.diagnostic->rule_ids.empty());
    }
}

TEST_CASE(normalize_is_deterministic) {
    // INV-STATE-002.
    StateModel sm;
    const Bit9State s = from_int(0x0AA);
    auto a = sm.normalize(s);
    auto b = sm.normalize(s);
    ASSERT_EQ(a.state, b.state);
    ASSERT_EQ(a.diagnostic->disposition, b.diagnostic->disposition);
    ASSERT_EQ(a.diagnostic->stage, b.diagnostic->stage);
}

int main() {
    return ash::test::run_all();
}
