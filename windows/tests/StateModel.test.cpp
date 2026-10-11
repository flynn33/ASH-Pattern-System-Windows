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
#include "ash/Diagnostics.hpp"

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

namespace {
OperationalContext explicit_context(std::vector<Bit9State> stable) {
    OperationalContext context;
    context.context_id = "state-tests";
    context.context_version = "1.0";
    context.allowed_stable_states = std::move(stable);
    context.fallback_policy_instance.instance_id = "no-replacement";
    context.fallback_policy_instance.policy_id = "FALLBACK-STATE-999";
    context.propagation_risk_policy_defined = true;
    context.propagation_risk_permits_mutation = true;
    context.containment_policy_defined = true;
    context.halt_policy_defined = true;
    return context;
}
}

TEST_CASE(input_normalization_rejects_malformed_data_without_fabricating_a_state) {
    StateModel model;
    for (const std::string value : {std::string{}, std::string("00000000"), std::string("0000000000"), std::string("00000x000"), std::string("00000000\n"), std::string(4096, '1')}) {
        auto result = model.normalize_input(value);
        ASSERT_FALSE(result.state.has_value());
        ASSERT_EQ(result.diagnostic->disposition, Disposition::BLOCKED);
        ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
        ASSERT_TRUE(result.diagnostic->subject_reference.size() < 128);
    }
    for (std::uint16_t value = 0; value < 512; ++value)
        ASSERT_EQ(model.normalize_input(canonical_signature(from_int(value))).state, std::optional<Bit9State>(from_int(value)));
}

TEST_CASE(operational_context_controls_stability_targets_and_terminal_precedence) {
    StateModel model;
    auto context = explicit_context({from_int(0)});
    ASSERT_EQ(model.assess_state(from_int(0), context).state_class, SystemStateClass::STABLE);
    ASSERT_EQ(model.assess_state(kCanonicalCodewords[1], context).state_class, SystemStateClass::CORRECTABLE);
    ASSERT_EQ(model.assess_state(from_int(1), context).state_class, SystemStateClass::DEGRADED);
    context.allowed_stable_states.push_back(kCanonicalCodewords[1]);
    auto ambiguous = model.assess_state(kCanonicalCodewords[2], context);
    ASSERT_EQ(ambiguous.state_class, SystemStateClass::UNSTABLE);
    ASSERT_FALSE(ambiguous.selected_target.has_value());
    context.preferred_state = kCanonicalCodewords[1];
    auto resolved = model.assess_state(kCanonicalCodewords[2], context);
    ASSERT_EQ(resolved.state_class, SystemStateClass::CORRECTABLE);
    ASSERT_EQ(resolved.selected_target, context.preferred_state);
    for (const auto mode : {RuntimeMode::CONTAINED, RuntimeMode::FAILED, RuntimeMode::SAFE_HALT}) {
        const auto result = model.assess_state(from_int(0), context, mode);
        ASSERT_EQ(result.state_class, mode == RuntimeMode::CONTAINED ? SystemStateClass::CONTAINED : mode == RuntimeMode::FAILED ? SystemStateClass::FAILED : SystemStateClass::SAFE_HALT);
        ASSERT_FALSE(result.selected_target.has_value());
    }
    context.preferred_state = from_int(1);
    ASSERT_FALSE(model.validate_context(context));
    auto blocked = model.assess_state(from_int(0), context);
    ASSERT_TRUE(blocked.blocked);
    ASSERT_FALSE(blocked.state_class.has_value());
}

TEST_CASE(operational_context_rejects_missing_policies_and_unsafe_restrictions) {
    StateModel model;
    auto context = explicit_context({from_int(0)});
    ASSERT_TRUE(model.validate_context(context));
    context.restricted_operations.push_back("undeclared_write");
    ASSERT_FALSE(model.validate_context(context));
    context = explicit_context({from_int(0)});
    context.containment_policy_defined = false;
    ASSERT_FALSE(model.validate_context(context));
    context = explicit_context({from_int(0), from_int(0)});
    ASSERT_FALSE(model.validate_context(context));
}

TEST_CASE(orbit_averaging_is_linear_invariant_and_idempotent) {
    std::array<double, 512> first{}, second{}, combined{};
    for (std::size_t i = 0; i < 512; ++i) {
        first[i] = static_cast<double>(i % 7);
        second[i] = -static_cast<double>(i % 3);
        combined[i] = 2 * first[i] + 4 * second[i];
    }
    StateModel model;
    const auto a = model.average(first), b = model.average(second), result = model.average(combined);
    for (std::uint16_t i = 0; i < 512; ++i) {
        ASSERT_EQ(result.values[i], 2 * a.values[i] + 4 * b.values[i]);
        for (const auto& codeword : kCanonicalCodewords) ASSERT_EQ(result.values[i], result.values[to_int(from_int(i) ^ codeword)]);
    }
    ASSERT_EQ(model.average(result.values).values, result.values);
}

int main() {
    return ash::test::run_all();
}
