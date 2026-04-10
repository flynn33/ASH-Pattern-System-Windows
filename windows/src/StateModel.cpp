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
// Behavior (from the canonical specs):
//
//   classify_admissibility(s):
//     If s is one of the 16 canonical codewords  => VALID
//     Else, since the codeword set is fully closed and orbits
//       partition F2^9, every other 9-bit state is in some orbit
//       that contains exactly one coset representative. Such a
//       state is TRANSFORMATION_COMPATIBLE with that representative.
//     The TRANSFORMATION_INCOMPATIBLE status is reachable only if
//       the codeword set is not fully specified; on this branch the
//       set is fully closed, so TRANSFORMATION_INCOMPATIBLE is only
//       returned if the input state somehow fails basic sanity
//       (it cannot, with std::bitset<9>).
//     UNCLASSIFIED is reserved for malformed inputs, which are
//       likewise unreachable via Bit9State. The enum value exists
//       so implementations can round-trip the canonical 4-status
//       enum even if future inputs become looser.
//
//   classify_system_state(s):
//     VALID                       => STABLE
//     TRANSFORMATION_COMPATIBLE   => UNSTABLE (conservative; normalization
//                                     path-finding is an expansion item)
//     TRANSFORMATION_INCOMPATIBLE => DEGRADED
//     UNCLASSIFIED                => FAILED
//
//   classify_recoverability(class):
//     STABLE         => NO_ACTION
//     UNSTABLE       => NORMALIZE_STATE
//     CORRECTABLE    => APPLY_CORRECTION
//     DEGRADED       => FALLBACK_REQUIRED
//     CONTAINED      => CONTAINMENT_REQUIRED
//     FAILED         => ESCALATION_REQUIRED
//     SAFE_HALT      => TERMINAL_NO_RECOVERY
//
//   normalize(candidate):
//     VALID                       => return (candidate, RESOLVED diagnostic)
//     TRANSFORMATION_COMPATIBLE   => return (candidate, PENDING diagnostic)
//                                    (conservative stub: real normalization
//                                    path-finding is deferred; see
//                                    deviation-log.md item 5)
//     Other                       => return (candidate, ERROR diagnostic)
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
    if (is_canonical_codeword(s)) {
        return AdmissibilityStatus::VALID;
    }
    // Codeword set is fully closed; every non-codeword 9-bit state is
    // in some orbit with a canonical coset representative, hence
    // transformation-compatible. This is the deterministic consequence
    // of "32 orbits partition F2^9" stated in codeword-set.pseudo.md.
    return AdmissibilityStatus::TRANSFORMATION_COMPATIBLE;
}

SystemStateClass
StateModel::classify_system_state(const Bit9State& s) const noexcept {
    switch (classify_admissibility(s)) {
        case AdmissibilityStatus::VALID:
            return SystemStateClass::STABLE;
        case AdmissibilityStatus::TRANSFORMATION_COMPATIBLE:
            return SystemStateClass::UNSTABLE;
        case AdmissibilityStatus::TRANSFORMATION_INCOMPATIBLE:
            return SystemStateClass::DEGRADED;
        case AdmissibilityStatus::UNCLASSIFIED:
            return SystemStateClass::FAILED;
    }
    // Unreachable; defensively map to FAILED.
    return SystemStateClass::FAILED;
}

RecoveryCategory
StateModel::classify_recoverability(SystemStateClass cls) const noexcept {
    switch (cls) {
        case SystemStateClass::STABLE:
            return RecoveryCategory::NO_ACTION;
        case SystemStateClass::UNSTABLE:
            return RecoveryCategory::NORMALIZE_STATE;
        case SystemStateClass::CORRECTABLE:
            return RecoveryCategory::APPLY_CORRECTION;
        case SystemStateClass::DEGRADED:
            return RecoveryCategory::FALLBACK_REQUIRED;
        case SystemStateClass::CONTAINED:
            return RecoveryCategory::CONTAINMENT_REQUIRED;
        case SystemStateClass::FAILED:
            return RecoveryCategory::ESCALATION_REQUIRED;
        case SystemStateClass::SAFE_HALT:
            return RecoveryCategory::TERMINAL_NO_RECOVERY;
    }
    return RecoveryCategory::ESCALATION_REQUIRED;
}

NormalizeResult StateModel::normalize(const Bit9State& candidate) const {
    const AdmissibilityStatus status = classify_admissibility(candidate);
    const std::string subject = state_as_text(candidate);

    if (status == AdmissibilityStatus::VALID) {
        auto diagnostic =
            DiagnosticEnvelopeBuilder{}
                .kind(DiagnosticKind::STATE_VALIDITY)
                .severity(Severity::INFO)
                .stage(Stage::CLASSIFICATION)
                .disposition(Disposition::RESOLVED)
                .subject(subject)
                .rule_id(rule_ids::kStateValidity001)
                .rule_id(rule_ids::kAdmissibility001)
                .summary("State is VALID; normalization is identity")
                .note("classify_admissibility returned VALID; returning "
                      "input unchanged with RESOLVED disposition.")
                .build_root();
        return NormalizeResult{candidate, std::move(diagnostic)};
    }

    if (status == AdmissibilityStatus::TRANSFORMATION_COMPATIBLE) {
        auto diagnostic =
            DiagnosticEnvelopeBuilder{}
                .kind(DiagnosticKind::STATE_VALIDITY)
                .severity(Severity::WARNING)
                .stage(Stage::RECOVERY)
                .disposition(Disposition::PENDING)
                .subject(subject)
                .rule_id(rule_ids::kStateValidity002)
                .rule_id(rule_ids::kAdmissibility001)
                .summary("State is TRANSFORMATION_COMPATIBLE; normalization path "
                         "pending in a future expansion pass")
                .note("Conservative implementation: returning input state "
                      "unchanged with PENDING disposition. The normalization "
                      "path-finding algorithm is deferred. See "
                      "windows/conformance/deviation-log.md item 5 and "
                      "specs/core/state-admissibility.pseudo.md on main.")
                .build_root();
        return NormalizeResult{candidate, std::move(diagnostic)};
    }

    // TRANSFORMATION_INCOMPATIBLE or UNCLASSIFIED — produce ERROR
    // diagnostic. INV-STATE-003 is satisfied: no silent result.
    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::STATE_VALIDITY)
            .severity(Severity::ERROR)
            .stage(Stage::DETECTION)
            .disposition(Disposition::BLOCKED)
            .subject(subject)
            .rule_id(rule_ids::kStateValidity003)
            .summary("State is not transformation-compatible; normalization blocked")
            .note("classify_admissibility returned TRANSFORMATION_INCOMPATIBLE "
                  "or UNCLASSIFIED. Normalization cannot proceed; the caller "
                  "must route to recovery, fallback, or escalation per the "
                  "canonical recoverability mapping.")
            .build_root();
    return NormalizeResult{candidate, std::move(diagnostic)};
}

}  // namespace ash
