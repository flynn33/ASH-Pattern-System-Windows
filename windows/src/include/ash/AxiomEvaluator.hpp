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

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
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

// Measurements belong to the caller. Missing values are unavailable evidence;
// the evaluator does not infer them from a Bit9State or perform measurements.
struct AxiomSubject {
    std::string identifier;
    std::optional<std::uint64_t> relation_count;
    std::optional<double> compressed_measure;
    std::optional<double> raw_measure;
    std::optional<std::uint64_t> scale_count;
    std::optional<double> erasure_cost;
    std::optional<bool> self_reference_capable;
    std::optional<bool> self_model_available;
};

// A caller-owned label for the fixed five canonical predicates. This label
// does not claim membership in a canonical profile registry or change thresholds.
struct AxiomEvaluationProfile {
    std::string identifier{"canonical-five-axioms"};
};

struct AxiomSubjectDiagnostic {
    AxiomResultStatus a1_relational_existence{AxiomResultStatus::INDETERMINATE};
    AxiomResultStatus a2_structural_compressibility{AxiomResultStatus::INDETERMINATE};
    AxiomResultStatus a3_multi_scale_persistence{AxiomResultStatus::INDETERMINATE};
    AxiomResultStatus a4_energetic_cost_of_erasure{AxiomResultStatus::INDETERMINATE};
    AxiomResultStatus a5_self_reference_for_consciousness{AxiomResultStatus::INDETERMINATE};
    AxiomResultStatus overall_status{AxiomResultStatus::INDETERMINATE};
    std::optional<bool> overall_pass;
    // Canonical a1..a5 order, with an explainable envelope for every predicate.
    std::array<AxiomEvaluationResult, 5> evaluations;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

class AxiomEvaluator {
public:
    AxiomEvaluator() = default;

    // Fixed canonical comparisons, without I/O. Missing/nonfinite evidence is
    // INDETERMINATE. FAIL takes precedence over missing evidence; PASS requires
    // all five predicates to pass. Empty subject/profile labels block evaluation.
    [[nodiscard]] AxiomSubjectDiagnostic
    diagnose_axioms(const AxiomSubject& subject,
                    const AxiomEvaluationProfile& profile = {}) const;

    // Compatibility helpers for caller-supplied conclusions. These preserve the
    // original API; they do not evaluate the subject's five canonical predicates.
    [[nodiscard]] AxiomEvaluationResult
    evaluate_axiom(const Bit9State& state, const Axiom& axiom) const;

    [[nodiscard]] AxiomBatchResult
    evaluate_axioms(const Bit9State& state, const std::vector<Axiom>& axioms) const;
};

}  // namespace ash
