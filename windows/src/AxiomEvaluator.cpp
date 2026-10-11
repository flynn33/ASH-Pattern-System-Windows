// AxiomEvaluator.cpp

#include "ash/AxiomEvaluator.hpp"

#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"
#include "ash/State.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace ash {

namespace {

AxiomResultStatus status_for(AxiomEvidence evidence) noexcept {
    switch (evidence) {
        case AxiomEvidence::PRESENT_PASS:
            return AxiomResultStatus::PASS;
        case AxiomEvidence::PRESENT_FAIL:
            return AxiomResultStatus::FAIL;
        case AxiomEvidence::MISSING:
            return AxiomResultStatus::INDETERMINATE;
    }
    return AxiomResultStatus::INDETERMINATE;
}

Severity severity_for(AxiomResultStatus status) noexcept {
    if (status == AxiomResultStatus::FAIL) return Severity::ERROR;
    if (status == AxiomResultStatus::INDETERMINATE) return Severity::WARNING;
    return Severity::INFO;
}

Disposition disposition_for(AxiomResultStatus status) noexcept {
    if (status == AxiomResultStatus::FAIL) return Disposition::BLOCKED;
    if (status == AxiomResultStatus::INDETERMINATE) return Disposition::PENDING;
    return Disposition::RESOLVED;
}

std::string summary_for(AxiomResultStatus status) {
    if (status == AxiomResultStatus::PASS) return "Axiom evidence satisfies the evaluation profile";
    if (status == AxiomResultStatus::FAIL) return "Axiom evidence fails the evaluation profile";
    return "Axiom evidence is missing or insufficient";
}

const char* status_name(AxiomResultStatus status) noexcept {
    if (status == AxiomResultStatus::PASS) return "PASS";
    if (status == AxiomResultStatus::FAIL) return "FAIL";
    return "INDETERMINATE";
}

bool available(const std::optional<double>& value) noexcept {
    return value.has_value() && std::isfinite(*value);
}

std::string measurement(const std::optional<double>& value) {
    if (!value) return "unavailable";
    if (!std::isfinite(*value)) return "nonfinite (unavailable evidence)";
    if (*value == 0.0) return "0";
    std::array<char, 64> buffer{};
    const auto converted = std::to_chars(buffer.data(), buffer.data() + buffer.size(),
                                         *value, std::chars_format::general,
                                         std::numeric_limits<double>::max_digits10);
    if (converted.ec != std::errc{}) throw std::logic_error("Finite axiom measurement conversion failed");
    return std::string(buffer.data(), converted.ptr);
}

std::string count_measurement(const std::optional<std::uint64_t>& value) {
    return value ? std::to_string(*value) : "unavailable";
}

std::string boolean_measurement(const std::optional<bool>& value) {
    return value ? (*value ? "true" : "false") : "unavailable";
}

std::shared_ptr<DiagnosticEnvelope> subject_diagnostic(
    const std::string& subject, const std::string& summary, AxiomResultStatus status,
    const std::vector<std::string>& explanations, bool invalid_identity) {
    auto builder = DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::AXIOM)
        .severity(invalid_identity ? Severity::ERROR : severity_for(status))
        .stage(Stage::EVALUATE)
        .disposition(invalid_identity ? Disposition::BLOCKED : disposition_for(status))
        .subject(subject)
        .rule_id(rule_ids::kAxiom001)
        .summary(summary);
    if (status == AxiomResultStatus::INDETERMINATE) builder.rule_id(rule_ids::kAxiom002);
    for (const auto& explanation : explanations) builder.note(explanation);
    return builder.build_root();
}

}  // namespace

AxiomSubjectDiagnostic AxiomEvaluator::diagnose_axioms(
    const AxiomSubject& subject, const AxiomEvaluationProfile& profile) const {
    static constexpr std::array<const char*, 5> identifiers{
        "a1_relational_existence", "a2_structural_compressibility",
        "a3_multi_scale_persistence", "a4_energetic_cost_of_erasure",
        "a5_self_reference_for_consciousness"};
    const bool invalid_identity = subject.identifier.empty() || profile.identifier.empty();
    const std::string reference = "axiom-subject[" + subject.identifier + "] profile[" + profile.identifier + "]";
    std::array<AxiomResultStatus, 5> statuses{};
    statuses.fill(AxiomResultStatus::INDETERMINATE);
    std::array<std::string, 5> explanations{
        "relation_count=" + count_measurement(subject.relation_count) + "; required relation_count > 0",
        "compressed_measure=" + measurement(subject.compressed_measure) + "; raw_measure=" + measurement(subject.raw_measure) + "; required compressed_measure < raw_measure",
        "scale_count=" + count_measurement(subject.scale_count) + "; required scale_count > 1",
        "erasure_cost=" + measurement(subject.erasure_cost) + "; required erasure_cost > 0",
        "self_reference_capable=" + boolean_measurement(subject.self_reference_capable) + "; self_model_available=" + boolean_measurement(subject.self_model_available) + "; required both values true"};
    if (!invalid_identity) {
        if (subject.relation_count) statuses[0] = *subject.relation_count > 0 ? AxiomResultStatus::PASS : AxiomResultStatus::FAIL;
        if (available(subject.compressed_measure) && available(subject.raw_measure)) statuses[1] = *subject.compressed_measure < *subject.raw_measure ? AxiomResultStatus::PASS : AxiomResultStatus::FAIL;
        if (subject.scale_count) statuses[2] = *subject.scale_count > 1 ? AxiomResultStatus::PASS : AxiomResultStatus::FAIL;
        if (available(subject.erasure_cost)) statuses[3] = *subject.erasure_cost > 0 ? AxiomResultStatus::PASS : AxiomResultStatus::FAIL;
        if (subject.self_reference_capable && subject.self_model_available) statuses[4] = *subject.self_reference_capable && *subject.self_model_available ? AxiomResultStatus::PASS : AxiomResultStatus::FAIL;
    }
    AxiomSubjectDiagnostic result;
    result.overall_status = AxiomResultStatus::PASS;
    bool missing = false;
    std::vector<std::string> notes;
    for (std::size_t index = 0; index < statuses.size(); ++index) {
        if (statuses[index] == AxiomResultStatus::FAIL) result.overall_status = AxiomResultStatus::FAIL;
        if (statuses[index] == AxiomResultStatus::INDETERMINATE) missing = true;
        const std::string explanation = std::string(identifiers[index]) + ": " + status_name(statuses[index]) + "; " +
            (invalid_identity ? "subject and profile identifiers must be nonempty; no positive conclusion is available" : explanations[index]);
        result.evaluations[index] = {statuses[index], subject_diagnostic(reference,
            std::string(identifiers[index]) + ": " + status_name(statuses[index]),
            statuses[index], {explanation}, invalid_identity)};
        notes.push_back(explanation);
    }
    if (result.overall_status != AxiomResultStatus::FAIL && missing) result.overall_status = AxiomResultStatus::INDETERMINATE;
    if (result.overall_status != AxiomResultStatus::INDETERMINATE) result.overall_pass = result.overall_status == AxiomResultStatus::PASS;
    result.a1_relational_existence = statuses[0];
    result.a2_structural_compressibility = statuses[1];
    result.a3_multi_scale_persistence = statuses[2];
    result.a4_energetic_cost_of_erasure = statuses[3];
    result.a5_self_reference_for_consciousness = statuses[4];
    notes.push_back("All five canonical predicates are required: a known failure takes precedence over unavailable evidence; no missing measurement is inferred from state.");
    result.diagnostic = subject_diagnostic(reference,
        std::string("Canonical five-axiom diagnosis: ") + status_name(result.overall_status),
        result.overall_status, notes, invalid_identity);
    return result;
}

AxiomEvaluationResult
AxiomEvaluator::evaluate_axiom(const Bit9State& /*state*/,
                               const Axiom& axiom) const {
    const AxiomResultStatus status = status_for(axiom.evidence);
    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::AXIOM)
            .severity(severity_for(status))
            .stage(Stage::CLASSIFICATION)
            .disposition(disposition_for(status))
            .subject("axiom[" + axiom.identifier + "]")
            .rule_id(rule_ids::kAxiom001)
            .rule_id(status == AxiomResultStatus::INDETERMINATE ? rule_ids::kAxiom002
                                                                : rule_ids::kAxiom001)
            .summary(summary_for(status))
            .note(axiom.limitation.empty()
                      ? "Axiom evaluation uses explicit evidence supplied by the caller; missing evidence is not guessed."
                      : axiom.limitation)
            .build_root();
    return AxiomEvaluationResult{status, std::move(diagnostic)};
}

AxiomBatchResult
AxiomEvaluator::evaluate_axioms(const Bit9State& /*state*/,
                                const std::vector<Axiom>& axioms) const {
    AxiomBatchResult batch;
    batch.overall_status = AxiomResultStatus::PASS;

    bool any_indeterminate = false;
    for (const auto& axiom : axioms) {
        auto result = evaluate_axiom(Bit9State{}, axiom);
        if (axiom.required && result.status == AxiomResultStatus::FAIL) {
            batch.overall_status = AxiomResultStatus::FAIL;
        } else if (axiom.required && result.status == AxiomResultStatus::INDETERMINATE) {
            any_indeterminate = true;
        }
        batch.results.push_back(std::move(result));
    }

    if (batch.overall_status != AxiomResultStatus::FAIL && any_indeterminate) {
        batch.overall_status = AxiomResultStatus::INDETERMINATE;
    }
    if (axioms.empty()) {
        batch.overall_status = AxiomResultStatus::INDETERMINATE;
    }

    batch.diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::AXIOM)
            .severity(severity_for(batch.overall_status))
            .stage(Stage::CLASSIFICATION)
            .disposition(disposition_for(batch.overall_status))
            .subject("axiom-batch[count=" + std::to_string(axioms.size()) + "]")
            .rule_id(rule_ids::kAxiom001)
            .summary(summary_for(batch.overall_status))
            .note("Overall axiom status is FAIL if any required axiom fails, "
                  "INDETERMINATE if none fail and at least one required axiom lacks evidence, "
                  "and PASS only when every required axiom passes.")
            .build_root();
    return batch;
}

}  // namespace ash
