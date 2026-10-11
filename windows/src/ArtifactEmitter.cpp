// ArtifactEmitter.cpp
#include "ash/ArtifactEmitter.hpp"

#include <stdexcept>
#include "ash/RuleIds.hpp"
#include "PlanIdentity.hpp"

namespace ash {
EmissionResult ArtifactEmitter::materialize_plan(const GenerationPlan& plan,
                                                const TargetConfiguration& config) const {
    bool valid = false;
    try {
        valid = (config.target_class.empty() || config.target_class == plan.target_class) &&
                detail::complete_plan_valid(plan);
    } catch (const std::logic_error&) {
        // Invalid embedded diagnostic records cannot be serialized or emitted.
    }
    if (!valid) {
        auto diagnostic = DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::EMISSION).severity(Severity::ERROR)
            .stage(Stage::EMIT).disposition(Disposition::BLOCKED)
            .subject("ArtifactEmitter::materialize_plan").rule_id(rule_ids::kEmission002)
            .summary("Incomplete, altered, or incompatible generation plan blocked")
            .note("No descriptors were emitted. The supplied plan must contain consistent request, realm, topology, role, axiom, artifact, warning and metadata records, a matching aps-windows-plan-2 identity, and a compatible target class.")
            .build_root();
        return {{}, std::move(diagnostic)};
    }
    std::vector<Artifact> artifacts;
    artifacts.reserve(plan.artifacts.size());
    for (std::size_t i = 0; i < plan.artifacts.size(); ++i) {
        const auto& description = plan.artifacts[i];
        artifacts.push_back({"artifact_descriptions[" + std::to_string(i) + "]", description.kind,
            description.summary, description.artifact_id, description.name, description.role_references});
    }
    auto diagnostic = DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::EMISSION).severity(Severity::INFO)
        .stage(Stage::EMIT).disposition(Disposition::RESOLVED)
        .subject("ArtifactEmitter::materialize_plan").rule_id(rule_ids::kEmission001)
        .summary("Materialized every plan-described artifact descriptor")
        .note("Each descriptor copies its declared ID, kind, name, summary and role references and traces to the corresponding artifact_descriptions entry. Full-plan validation uses only stored inputs and fixed semantic modules; the emitter performed no file I/O and did not call the planner.")
        .build_root();
    return {std::move(artifacts), std::move(diagnostic)};
}
}  // namespace ash
