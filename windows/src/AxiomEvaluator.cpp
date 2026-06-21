// AxiomEvaluator.cpp

#include "ash/AxiomEvaluator.hpp"

#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"
#include "ash/State.hpp"

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

}  // namespace

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
