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

TEST_CASE(classify_admissibility_all_16_codewords_are_well_formed) {
    // Every canonical codeword is a well-formed realm.
    StateModel sm;
    for (const auto& c : kCanonicalCodewords) {
        ASSERT_EQ(sm.classify_admissibility(c), AdmissibilityStatus::WELL_FORMED);
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
    // Every Bit9State is exactly nine binary coordinates and therefore
    // structurally well formed.
    StateModel sm;
    for (std::uint16_t i = 0; i < kStateSpaceSize; ++i) {
        const Bit9State s = from_int(i);
        const auto status = sm.classify_admissibility(s);
        ASSERT_EQ(status, AdmissibilityStatus::WELL_FORMED);
    }
}

TEST_CASE(canonical_codeword_membership_is_16_of_512_states) {
    int codeword_count = 0;
    for (std::uint16_t i = 0; i < kStateSpaceSize; ++i) {
        const Bit9State s = from_int(i);
        if (is_canonical_codeword(s)) {
            ++codeword_count;
        }
    }
    ASSERT_EQ(codeword_count, 16);
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
              RecoveryCategory::TARGET_RESOLUTION_REQUIRED);
    ASSERT_EQ(sm.classify_recoverability(SystemStateClass::CORRECTABLE),
              RecoveryCategory::APPLY_CORRECTION);
    ASSERT_EQ(sm.classify_recoverability(SystemStateClass::DEGRADED),
              RecoveryCategory::FALLBACK_REQUIRED);
    ASSERT_EQ(sm.classify_recoverability(SystemStateClass::CONTAINED),
              RecoveryCategory::CONTAINMENT_ACTIVE);
    ASSERT_EQ(sm.classify_recoverability(SystemStateClass::FAILED),
              RecoveryCategory::EXTERNAL_ESCALATION_REQUIRED);
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

TEST_CASE(normalize_all_well_formed_states_is_identity_with_resolved_diagnostic) {
    StateModel sm;
    for (std::uint16_t i = 0; i < kStateSpaceSize; ++i) {
        const Bit9State state = from_int(i);
        auto result = sm.normalize(state);
        ASSERT_EQ(result.state, state);
        ASSERT_TRUE(result.diagnostic != nullptr);
        ASSERT_EQ(result.diagnostic->disposition, Disposition::RESOLVED);
    }
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
