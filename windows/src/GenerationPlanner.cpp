// GenerationPlanner.cpp — NOT_IMPLEMENTED stub.
//
// See ash/GenerationPlanner.hpp for the LOCKED materialization
// boundary rules. The generate_plan() signature rejects any
// side-effecting input by construction (only pure data parameters),
// so this stub cannot accidentally perform I/O.
//
// Branch-local: ASH-WINDOWS-STUB-001.
//
// IMPORTANT: this stub MUST NOT include <filesystem>, <fstream>,
// <process>, or any header that would permit accidental side effects.
// The only included headers are the canonical envelope types and
// the rule-ID constants. This is part of the architectural
// enforcement of INV-PLAN-001 ("Planning produces no side effects")
// on the Windows branch.

#include "ash/GenerationPlanner.hpp"

#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"

namespace ash {

PlanResult GenerationPlanner::generate_plan(const Bit9State& /*state*/,
                                            const TargetMetadata& target) const {
    GenerationPlan empty_plan;
    empty_plan.target_class = target.target_class;

    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::STATE_VALIDITY)
            .severity(Severity::INFO)
            .stage(Stage::DETECTION)
            .disposition(Disposition::PENDING)
            .subject("GenerationPlanner::generate_plan")
            .rule_id(rule_ids::kWindowsStub001)
            .summary("NOT_IMPLEMENTED on windows-cpp minimal conformance slice")
            .note("GenerationPlanner is a non-slice module on windows-cpp. "
                  "The LOCKED materialization boundary is architecturally "
                  "enforced at the header level: this signature accepts no "
                  "side-effecting parameter. See "
                  "specs/interfaces/semantic-contracts.md on main "
                  "\u00a7 Materialization boundary (locked) and "
                  "windows/conformance/materialization-boundary-plan.md.")
            .build_root();

    return PlanResult{std::move(empty_plan), std::move(diagnostic)};
}

}  // namespace ash
