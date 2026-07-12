// ash/AxiomEvaluator.hpp
//
// AxiomEvaluator — canonical module.
//
// Canonical source of truth:
//   specs/interfaces/contracts/axiom-evaluator-contract.md on main
//   specs/algorithms/axiom-evaluation.pseudo.md on main
//
// Canonical purpose (paraphrased):
//   Evaluates axioms against current state, producing full
//   explainable diagnostic records (not bare booleans).
//
// Postconditions and invariants (canonical):
//   - INV-AXIOM-001: Every axiom evaluation produces a diagnostic
//   - Results are explainable: pass and fail both include explanations
//   - No silent pass/fail
//   - Evaluation is deterministic
//
// Prohibited behaviors (canonical):
//   - MUST NOT return bare pass/fail without diagnostics
//   - MUST NOT silently skip axiom evaluation
//   - MUST NOT suppress failure diagnostics
//   - MUST NOT produce unexplained results
//
// Uses the APS 1.0 tri-state result model: PASS, FAIL, INDETERMINATE.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "DiagnosticEnvelope.hpp"
#include "State.hpp"

namespace ash {

enum class AxiomEvidence {
    PRESENT_PASS,
    PRESENT_FAIL,
    MISSING,
};

enum class AxiomResultStatus {
    PASS,
    FAIL,
    INDETERMINATE,
};

struct Axiom {
    std::string identifier;
    AxiomEvidence evidence{AxiomEvidence::MISSING};
    bool required{true};
    std::string limitation;
};

struct AxiomEvaluationResult {
    AxiomResultStatus status{AxiomResultStatus::INDETERMINATE};
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

struct AxiomBatchResult {
    AxiomResultStatus overall_status{AxiomResultStatus::INDETERMINATE};
    std::vector<AxiomEvaluationResult> results;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

class AxiomEvaluator {
public:
    AxiomEvaluator() = default;

    [[nodiscard]] AxiomEvaluationResult
    evaluate_axiom(const Bit9State& state, const Axiom& axiom) const;

    [[nodiscard]] AxiomBatchResult
    evaluate_axioms(const Bit9State& state, const std::vector<Axiom>& axioms) const;
};

}  // namespace ash
