// StateModel.cpp
//
// Implementation of the StateModel canonical module for the
// windows-cpp branch.
//
// Canonical source of truth:
//   specs/core/state-admissibility.pseudo.md on main
//   specs/core/system-state-classification.pseudo.md on main
//   specs/core/recoverability-semantics.pseudo.md on main
//   specs/interfaces/contracts/state-model-contract.md on main
//
// Behavior:
//   classify_admissibility(s) => WELL_FORMED for every Bit9State.
//   classify_system_state(s) keeps a default in-memory mapping for
//   existing recovery tests: canonical codewords are treated as STABLE
//   under the branch's built-in context; all other well-formed
//   states are UNSTABLE until an explicit operational context selects a
//   target.
//
//   classify_recoverability(class):
//     STABLE      => NO_ACTION
//     UNSTABLE    => TARGET_RESOLUTION_REQUIRED
//     CORRECTABLE => APPLY_CORRECTION
//     DEGRADED    => FALLBACK_REQUIRED
//     CONTAINED   => CONTAINMENT_ACTIVE
//     FAILED      => EXTERNAL_ESCALATION_REQUIRED
//     SAFE_HALT   => TERMINAL_NO_RECOVERY
//
//   normalize(candidate):
//     WELL_FORMED                 => return (candidate, RESOLVED diagnostic)
//     MALFORMED                   => return (candidate, BLOCKED diagnostic)
//
// Every call to normalize() produces a DiagnosticEnvelope. No silent
// result is ever returned. INV-STATE-003.

#include "ash/StateModel.hpp"

#include "ash/Codeword.hpp"
#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"
#include "ash/State.hpp"

namespace ash {

namespace {

std::string state_as_text(const Bit9State& s) {
    // Render as 9 characters b0..b8 for subject_reference. Deterministic.
    std::string out;
    out.reserve(9);
    for (std::size_t i = 0; i < 9; ++i) {
        out.push_back(s.test(i) ? '1' : '0');
    }
    return out;
}

}  // namespace

AdmissibilityStatus
StateModel::classify_admissibility(const Bit9State& s) const noexcept {
    (void)s;
    return AdmissibilityStatus::WELL_FORMED;
}

SystemStateClass
StateModel::classify_system_state(const Bit9State& s) const noexcept {
    if (classify_admissibility(s) == AdmissibilityStatus::MALFORMED) {
        return SystemStateClass::FAILED;
    }
    if (is_canonical_codeword(s)) {
        return SystemStateClass::STABLE;
    }
    return SystemStateClass::UNSTABLE;
}

RecoveryCategory
StateModel::classify_recoverability(SystemStateClass cls) const noexcept {
    switch (cls) {
        case SystemStateClass::STABLE:
            return RecoveryCategory::NO_ACTION;
        case SystemStateClass::UNSTABLE:
            return RecoveryCategory::TARGET_RESOLUTION_REQUIRED;
        case SystemStateClass::CORRECTABLE:
            return RecoveryCategory::APPLY_CORRECTION;
        case SystemStateClass::DEGRADED:
            return RecoveryCategory::FALLBACK_REQUIRED;
        case SystemStateClass::CONTAINED:
            return RecoveryCategory::CONTAINMENT_ACTIVE;
        case SystemStateClass::FAILED:
            return RecoveryCategory::EXTERNAL_ESCALATION_REQUIRED;
        case SystemStateClass::SAFE_HALT:
            return RecoveryCategory::TERMINAL_NO_RECOVERY;
    }
    return RecoveryCategory::EXTERNAL_ESCALATION_REQUIRED;
}

NormalizeResult StateModel::normalize(const Bit9State& candidate) const {
    const AdmissibilityStatus status = classify_admissibility(candidate);
    const std::string subject = state_as_text(candidate);

    if (status == AdmissibilityStatus::WELL_FORMED) {
        auto diagnostic =
            DiagnosticEnvelopeBuilder{}
                .kind(DiagnosticKind::STATE_VALIDITY)
                .severity(Severity::INFO)
                .stage(Stage::CLASSIFICATION)
                .disposition(Disposition::RESOLVED)
                .subject(subject)
                .rule_id(rule_ids::kStateValidity001)
                .rule_id(rule_ids::kAdmissibility001)
                .summary("State is well formed; structural normalization is identity")
                .note("Bit9State already represents exactly nine binary coordinates. "
                      "Structural normalization preserves realm identity and returns "
                      "the input unchanged with RESOLVED disposition.")
                .build_root();
        return NormalizeResult{candidate, std::move(diagnostic)};
    }

    // MALFORMED states produce an ERROR diagnostic. INV-STATE-003 is
    // satisfied: no silent result.
    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::STATE_VALIDITY)
            .severity(Severity::ERROR)
            .stage(Stage::DETECTION)
            .disposition(Disposition::BLOCKED)
            .subject(subject)
            .rule_id(rule_ids::kStateValidity003)
            .summary("State is malformed; normalization blocked")
            .note("classify_admissibility returned MALFORMED. Normalization "
                  "cannot proceed; the caller must route to recovery, "
                  "fallback, or escalation per the canonical recoverability "
                  "mapping.")
            .build_root();
    return NormalizeResult{candidate, std::move(diagnostic)};
}

}  // namespace ash
