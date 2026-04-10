// ash/AxiomEvaluator.hpp
//
// AxiomEvaluator — canonical module (non-slice, stub on this branch).
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
// Branch-local status: STUB. Returns a NOT_IMPLEMENTED diagnostic
// with the branch-local ASH-WINDOWS-STUB-001 rule ID. See
// windows/conformance/deviation-log.md item 6.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "DiagnosticEnvelope.hpp"
#include "State.hpp"

namespace ash {

// Placeholder axiom identifier. Shape will be defined when the
// canonical axiom set is formalized on a target.
struct Axiom {
    std::string identifier;
};

class AxiomEvaluator {
public:
    AxiomEvaluator() = default;

    // Stub on this branch. Returns a NOT_IMPLEMENTED diagnostic.
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope>
    evaluate_axiom(const Bit9State& state, const Axiom& axiom) const;

    // Stub on this branch. Returns a single NOT_IMPLEMENTED diagnostic
    // that covers the entire axiom list, rather than pretending to
    // evaluate each axiom individually.
    [[nodiscard]] std::vector<std::shared_ptr<DiagnosticEnvelope>>
    evaluate_axioms(const Bit9State& state, const std::vector<Axiom>& axioms) const;
};

}  // namespace ash
