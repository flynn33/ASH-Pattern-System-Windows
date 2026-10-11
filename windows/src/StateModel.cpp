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
#include <algorithm>
#include <cmath>

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

InputNormalizationResult StateModel::normalize_input(std::string_view candidate) const {
    const bool well_formed = candidate.size() == 9 &&
        std::all_of(candidate.begin(), candidate.end(), [](char c) { return c == '0' || c == '1'; });
    if (well_formed) {
        Bit9State state;
        for (std::size_t i = 0; i < 9; ++i) state.set(i, candidate[i] == '1');
        auto normalized = normalize(state);
        return {state, std::move(normalized.diagnostic)};
    }
    // Retain a bounded printable candidate reference; do not coerce malformed
    // input to Bit9State or leak raw control characters into diagnostics.
    std::string reference = "candidate[";
    const std::size_t retained = std::min(candidate.size(), std::size_t{64});
    for (std::size_t i = 0; i < retained; ++i) {
        const unsigned char c = static_cast<unsigned char>(candidate[i]);
        reference += (c >= 0x20 && c < 0x7f && c != '[' && c != ']' && c != '\\') ? std::string(1, static_cast<char>(c)) : "?";
    }
    reference += "] length=" + std::to_string(candidate.size());
    auto diagnostic = DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::INPUT_VALIDATION).severity(Severity::ERROR)
        .stage(Stage::DETECTION).disposition(Disposition::BLOCKED)
        .subject(reference).rule_id("ASH-INPUT-001")
        .summary("Malformed input; exactly nine binary coordinates required")
        .note("Parsing failed before realm, orbit, or operational assessment; no state was produced.")
        .build_root();
    return {std::nullopt, std::move(diagnostic)};
}

bool StateModel::validate_context(const OperationalContext& context) const noexcept {
    if (context.context_id.empty() || context.context_version.empty() || context.allowed_stable_states.empty() ||
        context.codeword_registry_version != "1.0" || !context.propagation_risk_policy_defined ||
        !context.containment_policy_defined || !context.halt_policy_defined || context.restricted_operations.empty()) return false;
    const auto& instance = context.fallback_policy_instance;
    if (instance.instance_id.empty() || instance.registry_version != "1.0" ||
        (instance.policy_id != "FALLBACK-STATE-001" && instance.policy_id != "FALLBACK-STATE-002" && instance.policy_id != "FALLBACK-STATE-999")) return false;
    if (context.preferred_state && std::find(context.allowed_stable_states.begin(), context.allowed_stable_states.end(), *context.preferred_state) == context.allowed_stable_states.end()) return false;
    for (std::size_t i = 0; i < context.allowed_stable_states.size(); ++i)
        for (std::size_t j = i + 1; j < context.allowed_stable_states.size(); ++j)
            if (context.allowed_stable_states[i] == context.allowed_stable_states[j]) return false;
    for (const auto& operation : context.restricted_operations)
        if (operation != "inspect" && operation != "diagnostics" && operation != "export" && operation != "safe_halt") return false;
    return true;
}

StateAssessment StateModel::assess_state(std::string_view candidate, const OperationalContext* context, RuntimeMode mode) const {
    auto normalized = normalize_input(candidate);
    StateAssessment assessment;
    assessment.state = normalized.state;
    assessment.runtime_mode = mode;
    assessment.diagnostic = normalized.diagnostic;
    if (!normalized.state) return assessment;
    if (!context || !validate_context(*context) || static_cast<unsigned>(mode) > static_cast<unsigned>(RuntimeMode::SAFE_HALT)) {
        assessment.diagnostic = DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::STATE_ASSESSMENT).severity(Severity::ERROR)
            .stage(Stage::CLASSIFICATION).disposition(Disposition::BLOCKED)
            .subject(canonical_signature(*normalized.state)).parent(normalized.diagnostic)
            .rule_id("ASH-CLASSIFICATION-001")
            .summary("Operational assessment blocked by missing or invalid context")
            .note("Stable targets, registry identities, fallback instance, and explicit safety policies must validate before health classification.")
            .build_child();
        return assessment;
    }
    assessment.operational_context_id = context->context_id;
    assessment.blocked = false;
    if (mode == RuntimeMode::SAFE_HALT) assessment.state_class = SystemStateClass::SAFE_HALT;
    else if (mode == RuntimeMode::CONTAINED) assessment.state_class = SystemStateClass::CONTAINED;
    else if (mode == RuntimeMode::FAILED) assessment.state_class = SystemStateClass::FAILED;
    else if (std::find(context->allowed_stable_states.begin(), context->allowed_stable_states.end(), *normalized.state) != context->allowed_stable_states.end()) assessment.state_class = SystemStateClass::STABLE;
    else {
        for (const auto& target : context->allowed_stable_states)
            if (is_canonical_codeword(*normalized.state ^ target)) assessment.candidate_targets.push_back(target);
        std::sort(assessment.candidate_targets.begin(), assessment.candidate_targets.end(), [](const Bit9State& a, const Bit9State& b) { return to_int(a) < to_int(b); });
        if (assessment.candidate_targets.empty()) assessment.state_class = SystemStateClass::DEGRADED;
        else {
            if (assessment.candidate_targets.size() == 1) assessment.selected_target = assessment.candidate_targets.front();
            else if (context->preferred_state && std::find(assessment.candidate_targets.begin(), assessment.candidate_targets.end(), *context->preferred_state) != assessment.candidate_targets.end()) assessment.selected_target = context->preferred_state;
            assessment.state_class = assessment.selected_target ? SystemStateClass::CORRECTABLE : SystemStateClass::UNSTABLE;
            if (assessment.selected_target) {
                const auto delta = *normalized.state ^ *assessment.selected_target;
                for (std::size_t i = 0; i < kCodewordCount; ++i) if (kCanonicalCodewords[i] == delta) assessment.correction_codeword_id = i;
            }
        }
    }
    const bool stable = assessment.state_class == SystemStateClass::STABLE;
    assessment.diagnostic = DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::STATE_ASSESSMENT).severity(stable ? Severity::INFO : Severity::WARNING)
        .stage(Stage::CLASSIFICATION).disposition(stable ? Disposition::RESOLVED : Disposition::PENDING)
        .subject(canonical_signature(*normalized.state)).parent(normalized.diagnostic)
        .rule_id("ASH-CLASSIFICATION-001")
        .summary("Classified state under validated operational context")
        .note("Context=" + context->context_id + "; class=" + std::to_string(static_cast<unsigned>(*assessment.state_class)) + "; candidate targets=" + std::to_string(assessment.candidate_targets.size()))
        .build_child();
    return assessment;
}

StateAssessment StateModel::assess_state(const Bit9State& state, const OperationalContext& context, RuntimeMode mode) const {
    return assess_state(canonical_signature(state), &context, mode);
}

AveragingResult StateModel::average(const std::array<double, kStateSpaceSize>& values) const {
    AveragingResult result;
    const bool finite = std::all_of(values.begin(), values.end(), [](double value) { return std::isfinite(value); });
    if (finite) {
        for (std::size_t i = 0; i < kStateSpaceSize; ++i)
            for (const auto& codeword : kCanonicalCodewords)
                result.values[i] += values[to_int(from_int(static_cast<std::uint16_t>(i)) ^ codeword)] / static_cast<double>(kCodewordCount);
    }
    const bool output_finite = finite && std::all_of(result.values.begin(), result.values.end(), [](double value) { return std::isfinite(value); });
    if (!output_finite) result.values.fill(0);
    result.diagnostic = DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::CONFORMANCE).severity(output_finite ? Severity::INFO : Severity::ERROR)
        .stage(Stage::CLASSIFICATION).disposition(output_finite ? Disposition::RESOLVED : Disposition::BLOCKED)
        .subject("canonical-averaging").rule_id("ASH-CONFORMANCE-001")
        .summary(output_finite ? "Applied canonical averaging operator over all 512 states" : "Averaging input or result is not a finite real value")
        .note("Each result averages exactly the 16 fixed codeword transforms in canonical order; double arithmetic has the host's documented floating-point precision.")
        .build_root();
    return result;
}

}  // namespace ash
