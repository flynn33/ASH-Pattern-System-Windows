// ArtifactEmitter.cpp — NOT_IMPLEMENTED stub.
//
// See ash/ArtifactEmitter.hpp for the LOCKED materialization
// boundary rules. The materialize_plan() signature takes exactly
// one GenerationPlan and one TargetConfiguration, with no reference
// to a GenerationPlanner. This is the architectural enforcement of
// "no callback to planner" (INV-BOUNDARY-001) at the header level.
//
// Branch-local: ASH-WINDOWS-STUB-001.
//
// IMPORTANT: this stub MUST NOT include <filesystem>, <fstream>, or
// any I/O header. Runtime materialization is a future expansion; for
// now the emitter is a pure function that describes what it would do
// without doing anything.

#include "ash/ArtifactEmitter.hpp"

#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"

namespace ash {

EmissionResult
ArtifactEmitter::materialize_plan(const GenerationPlan& /*plan*/,
                                  const TargetConfiguration& /*config*/) const {
    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::STATE_VALIDITY)
            .severity(Severity::INFO)
            .stage(Stage::DETECTION)
            .disposition(Disposition::PENDING)
            .subject("ArtifactEmitter::materialize_plan")
            .rule_id(rule_ids::kWindowsStub001)
            .summary("NOT_IMPLEMENTED on windows-cpp minimal conformance slice")
            .note("ArtifactEmitter is a non-slice module on windows-cpp. "
                  "The LOCKED materialization boundary is architecturally "
                  "enforced: this signature takes only the plan and target "
                  "config, with no reference to the planner. See "
                  "specs/interfaces/semantic-contracts.md on main "
                  "\u00a7 Materialization boundary (locked) and "
                  "windows/conformance/materialization-boundary-plan.md.")
            .build_root();

    return EmissionResult{{}, std::move(diagnostic)};
}

}  // namespace ash
