// TransitionRegistry.test.cpp
//
// Tests for TransitionRegistry. Covers INV-CODEWORD-001..004 and
// INV-TRANS-001..002.

#include "test_runner.hpp"

#include "ash/Codeword.hpp"
#include "ash/State.hpp"
#include "ash/TransitionRegistry.hpp"

using namespace ash;

TEST_CASE(codeword_set_has_exactly_16_members) {
    // INV-CODEWORD-004.
    ASSERT_EQ(kCanonicalCodewords.size(), static_cast<std::size_t>(16));
}

TEST_CASE(codeword_set_b8_is_zero_for_all_16_members) {
    // Structural property from specs/core/codeword-set.pseudo.md.
    for (const auto& c : kCanonicalCodewords) {
        ASSERT_FALSE(b8(c));
    }
}

TEST_CASE(codeword_set_contains_zero_codeword) {
    // C is a subgroup of (F2^9, XOR); it must contain the identity.
    ASSERT_TRUE(canonical_set_contains_zero());
}

TEST_CASE(codeword_set_weights_are_in_0_4_8) {
    // Doubly-even code: weights divisible by 4. For this specific
    // code the allowed weights are {0, 4, 8}.
    for (const auto& c : kCanonicalCodewords) {
        const std::size_t w = c.count();
        ASSERT_TRUE(w == 0 || w == 4 || w == 8);
    }
}

TEST_CASE(codeword_set_is_closed_under_xor) {
    // C is a subgroup of (F2^9, XOR) — for every pair of codewords,
    // the XOR is also a codeword.
    for (const auto& a : kCanonicalCodewords) {
        for (const auto& b : kCanonicalCodewords) {
            Codeword xor_ab = a ^ b;
            ASSERT_TRUE(is_canonical_codeword(xor_ab));
        }
    }
}

TEST_CASE(apply_transition_is_deterministic) {
    // INV-TRANS-001.
    TransitionRegistry reg;
    const Bit9State s = from_int(0x13A);
    const Codeword c = kCanonicalCodewords[5];
    auto a = reg.apply_transition(s, c);
    auto b = reg.apply_transition(s, c);
    ASSERT_EQ(a.state, b.state);
}

TEST_CASE(apply_transition_is_involutive) {
    // INV-CODEWORD-003: (x XOR c) XOR c = x.
    TransitionRegistry reg;
    for (std::uint16_t i = 0; i < 32; ++i) {
        const Bit9State s = from_int(i);
        for (const auto& c : kCanonicalCodewords) {
            auto first = reg.apply_transition(s, c);
            auto second = reg.apply_transition(first.state, c);
            ASSERT_EQ(second.state, s);
        }
    }
}

TEST_CASE(apply_transition_matches_all_8192_canonical_pairs) {
    TransitionRegistry reg;
    std::size_t pair_count = 0;
    for (std::uint16_t state_index = 0; state_index < kStateSpaceSize; ++state_index) {
        const Bit9State state = from_int(state_index);
        for (std::size_t transition_id = 0; transition_id < kCodewordCount; ++transition_id) {
            auto resolved = reg.resolve_transition(state, transition_id);
            auto result = reg.apply_transition(state, resolved.codeword);
            ASSERT_EQ(result.state, state ^ kCanonicalCodewords[transition_id]);
            ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::TRANSITION);
            ASSERT_EQ(resolved.diagnostic->diagnostic_kind, DiagnosticKind::TRANSITION);
            ++pair_count;
        }
    }
    ASSERT_EQ(pair_count, static_cast<std::size_t>(8192));
}

TEST_CASE(apply_transition_operates_on_full_9_bits) {
    // INV-TRANS-002: verify the result has all 9 bits XORed,
    // including b8. Use a state with b8=1. In the APS canonical
    // integer encoding b0 is most significant and b8 is least significant,
    // so encoded 1 means b8=1 only.
    // and a codeword. Since every canonical codeword has b8=0, the
    // result's b8 should equal the input's b8.
    TransitionRegistry reg;
    const Bit9State state_with_b8 = from_int(1);  // b8=1 only
    ASSERT_TRUE(b8(state_with_b8));
    for (const auto& c : kCanonicalCodewords) {
        auto result = reg.apply_transition(state_with_b8, c);
        // b8 of canonical codewords is 0, so b8 of the result equals
        // b8 of the input (which is 1). This catches any implementation
        // that silently drops b8.
        ASSERT_TRUE(b8(result.state));
    }
}

TEST_CASE(apply_transition_emits_diagnostic) {
    // INV-DIAG-004: no silent transition.
    TransitionRegistry reg;
    auto result = reg.apply_transition(from_int(0), kCanonicalCodewords[0]);
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::TRANSITION);
    ASSERT_TRUE(!result.diagnostic->rule_ids.empty());
    ASSERT_TRUE(!result.diagnostic->summary.empty());
}

TEST_CASE(apply_transition_rejects_noncanonical_codeword_without_state_change) {
    TransitionRegistry reg;
    const Bit9State state = from_int(30);
    Codeword noncanonical = from_int(1);
    ASSERT_FALSE(is_canonical_codeword(noncanonical));
    auto result = reg.apply_transition(state, noncanonical);
    ASSERT_EQ(result.state, state);
    ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::TRANSITION);
    ASSERT_EQ(result.diagnostic->severity, Severity::ERROR);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::BLOCKED);
}

TEST_CASE(resolve_transition_by_id_is_deterministic) {
    TransitionRegistry reg;
    for (std::size_t i = 0; i < kCodewordCount; ++i) {
        auto a = reg.resolve_transition(from_int(0), i);
        auto b = reg.resolve_transition(from_int(0x1FF), i);
        ASSERT_EQ(a.codeword, b.codeword);
        ASSERT_EQ(a.codeword, kCanonicalCodewords[i]);
    }
}

TEST_CASE(resolve_transition_out_of_range_produces_error_diagnostic) {
    TransitionRegistry reg;
    auto result = reg.resolve_transition(from_int(0), 99);
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_EQ(result.diagnostic->diagnostic_kind, DiagnosticKind::TRANSITION);
    ASSERT_EQ(result.diagnostic->severity, Severity::ERROR);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::BLOCKED);
}

int main() {
    return ash::test::run_all();
}
