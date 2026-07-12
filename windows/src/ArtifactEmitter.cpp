// ArtifactEmitter.cpp

#include "ash/ArtifactEmitter.hpp"

#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"

namespace ash {

EmissionResult
ArtifactEmitter::materialize_plan(const GenerationPlan& plan,
                                  const TargetConfiguration& config) const {
    if (plan.artifact_descriptions.empty() || plan.plan_hash.empty() ||
        (!config.target_class.empty() && config.target_class != plan.target_class)) {
        auto diagnostic =
            DiagnosticEnvelopeBuilder{}
                .kind(DiagnosticKind::EMISSION)
                .severity(Severity::ERROR)
                .stage(Stage::DETECTION)
                .disposition(Disposition::BLOCKED)
                .subject("ArtifactEmitter::materialize_plan")
                .rule_id(rule_ids::kEmission002)
                .summary("Incomplete or incompatible generation plan blocked")
                .note("ArtifactEmitter requires a complete plan with at least "
                      "one artifact description, a plan hash, and a compatible target class.")
                .build_root();
        return EmissionResult{{}, std::move(diagnostic)};
    }

    std::vector<Artifact> artifacts;
    artifacts.reserve(plan.artifact_descriptions.size());
    for (std::size_t i = 0; i < plan.artifact_descriptions.size(); ++i) {
        artifacts.push_back(Artifact{
            "artifact_descriptions[" + std::to_string(i) + "]",
            "descriptor",
            plan.artifact_descriptions[i],
        });
    }

    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::EMISSION)
            .severity(Severity::INFO)
            .stage(Stage::RECOVERY)
            .disposition(Disposition::RESOLVED)
            .subject("ArtifactEmitter::materialize_plan")
            .rule_id(rule_ids::kEmission001)
            .summary("Materialized every plan-described artifact descriptor")
            .note("Each emitted artifact traces directly to one plan artifact "
                  "description. The emitter did not call back into the planner "
                  "or invent plan elements.")
            .build_root();

    return EmissionResult{std::move(artifacts), std::move(diagnostic)};
}

}  // namespace ash
