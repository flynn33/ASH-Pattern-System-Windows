// AxiomEvaluator.cpp — NOT_IMPLEMENTED stub.
//
// See ash/AxiomEvaluator.hpp for the canonical contract. The current
// branch ships a stub; every stub return is a schema-conformant
// diagnostic with disposition PENDING and a note citing the
// NOT_IMPLEMENTED status.
//
// Branch-local: ASH-WINDOWS-STUB-001.

#include "ash/AxiomEvaluator.hpp"

#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"

namespace ash {

namespace {

std::shared_ptr<DiagnosticEnvelope> make_not_implemented_axiom_diagnostic() {
    return DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::STATE_VALIDITY)
        .severity(Severity::INFO)
        .stage(Stage::DETECTION)
        .disposition(Disposition::PENDING)
        .subject("AxiomEvaluator")
        .rule_id(rule_ids::kWindowsStub001)
        .summary("NOT_IMPLEMENTED on windows-cpp minimal conformance slice")
        .note("AxiomEvaluator is a non-slice module on windows-cpp. See "
              "specs/interfaces/contracts/axiom-evaluator-contract.md on "
              "main and windows/conformance/deviation-log.md item 6.")
        .build_root();
}

}  // namespace

std::shared_ptr<DiagnosticEnvelope>
AxiomEvaluator::evaluate_axiom(const Bit9State& /*state*/,
                               const Axiom& /*axiom*/) const {
    return make_not_implemented_axiom_diagnostic();
}

std::vector<std::shared_ptr<DiagnosticEnvelope>>
AxiomEvaluator::evaluate_axioms(const Bit9State& /*state*/,
                                const std::vector<Axiom>& /*axioms*/) const {
    return {make_not_implemented_axiom_diagnostic()};
}

}  // namespace ash
