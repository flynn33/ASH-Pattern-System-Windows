// TransitionRegistry.cpp
//
// Implementation of the TransitionRegistry canonical module.
//
// Canonical source of truth:
//   specs/algorithms/transition-system.pseudo.md on main
//   specs/algorithms/codeword-transformation-semantics.pseudo.md on main
//   specs/interfaces/contracts/transition-registry-contract.md on main
//
// Canonical rule:
//   x' = x ⊕ c on all 9 bits simultaneously.
//
// INV-TRANS-001: deterministic.
// INV-TRANS-002: operates on full 9-bit state.
// INV-CODEWORD-003: (x ⊕ c) ⊕ c = x (involution) — a property of XOR.

#include "ash/TransitionRegistry.hpp"

#include <string>

#include "ash/Codeword.hpp"
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

std::string transition_subject(const Bit9State& state, std::size_t id) {
    return "transition[id=" + std::to_string(id) + ",state=" + state_as_text(state) + "]";
}

}  // namespace

ResolveResult
TransitionRegistry::resolve_transition(const Bit9State& state,
                                       std::size_t transition_id) const {
    if (transition_id >= kCodewordCount) {
        auto diagnostic =
            DiagnosticEnvelopeBuilder{}
                .kind(DiagnosticKind::TRANSITION)
                .severity(Severity::ERROR)
                .stage(Stage::DETECTION)
                .disposition(Disposition::BLOCKED)
                .subject(transition_subject(state, transition_id))
                .rule_id(rule_ids::kCodeword004)
                .summary("transition_id out of range of canonical codeword set")
                .note("Canonical codeword set C has exactly 16 members per "
                      "specs/core/codeword-set.pseudo.md. transition_id must "
                      "be in [0, 16). Returning zero codeword with BLOCKED "
                      "disposition.")
                .build_root();
        return ResolveResult{Codeword{}, std::move(diagnostic)};
    }

    const Codeword& c = kCanonicalCodewords[transition_id];

    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::TRANSITION)
            .severity(Severity::INFO)
            .stage(Stage::CLASSIFICATION)
            .disposition(Disposition::RESOLVED)
            .subject(transition_subject(state, transition_id))
            .rule_id(rule_ids::kTrans001)
            .rule_id(rule_ids::kCodeword004)
            .summary("Resolved transition to canonical codeword")
            .note("transition_id is within [0, 16); returning "
                  "kCanonicalCodewords[transition_id] with RESOLVED disposition.")
            .build_root();

    return ResolveResult{c, std::move(diagnostic)};
}

TransitionResult
TransitionRegistry::apply_transition(const Bit9State& state,
                                     const Codeword& codeword) const {
    // XOR is applied to all 9 bits by the std::bitset implementation.
    // INV-CODEWORD-001, INV-TRANS-002.
    const bool is_canonical = is_canonical_codeword(codeword);
    const Severity sev = is_canonical ? Severity::INFO : Severity::ERROR;
    const Disposition disp = is_canonical ? Disposition::RESOLVED : Disposition::BLOCKED;
    const Bit9State result = is_canonical ? (state ^ codeword) : state;

    DiagnosticEnvelopeBuilder builder;
    builder.kind(DiagnosticKind::TRANSITION)
        .severity(sev)
        .stage(Stage::RECOVERY)
        .disposition(disp)
        .subject("apply_transition[state=" + state_as_text(state) + "]")
        .rule_id(rule_ids::kCodeword001)
        .rule_id(rule_ids::kTrans002);

    if (is_canonical) {
        builder.summary("Applied XOR-by-codeword on full 9-bit state")
            .note("x' = x XOR c computed with canonical codeword; all 9 bits "
                  "participate.");
    } else {
        builder.rule_id(rule_ids::kCodeword004)
            .summary("Codeword is not a member of the canonical set C")
            .note("apply_transition rejected the supplied codeword because it "
                  "is not in kCanonicalCodewords. Per "
                  "specs/core/codeword-set.pseudo.md downstream implementations "
                  "must not use non-canonical codewords. The input state is "
                  "returned unchanged with BLOCKED disposition so the caller "
                  "may escalate.");
    }

    return TransitionResult{result, builder.build_root()};
}

}  // namespace ash
