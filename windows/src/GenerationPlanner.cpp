// GenerationPlanner.cpp
#include "ash/GenerationPlanner.hpp"

#include <stdexcept>
#include <utility>

#include "ash/RuleIds.hpp"
#include "ash/StateModel.hpp"
#include "PlanIdentity.hpp"

namespace ash {
namespace {
PlanResult blocked_plan(std::string summary, std::string reason) {
    auto diagnostic = DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::GENERATION_PLAN).severity(Severity::ERROR)
        .stage(Stage::PLAN).disposition(Disposition::BLOCKED)
        .subject("GenerationPlanner::generate_plan").rule_id(rule_ids::kGeneration001)
        .summary(std::move(summary)).note(std::move(reason)).build_root();
    return {GenerationPlan{}, std::move(diagnostic)};
}
}

std::string generation_plan_identity_payload(const GenerationPlan& plan) {
    return detail::canonical_plan_payload(plan);
}

PlanResult GenerationPlanner::generate_plan(const GenerationRequest& request,
                                             const GenerationPlanningInputs& inputs) const {
    if (request.profile_id != "aps-windows-descriptor-1.0" ||
        inputs.target.profile_id != request.profile_id)
        return blocked_plan("PROFILE_REFERENCE_INVALID", "The request and target must reference the supported aps-windows-descriptor-1.0 profile.");
    if (!detail::supported_naming(request.naming_policy_id))
        return blocked_plan("NAMING_POLICY_REFERENCE_INVALID", "The native naming registry supports aps-windows-realm-names-1.0 and aps-windows-project-names-1.0.");
    if (request.emission_target_kind != "descriptor" || !detail::supported_target(inputs.target.target_class))
        return blocked_plan("EMISSION_TARGET_REFERENCE_INVALID", "The native target registry supports descriptor output for desktop, mobile, and service classes.");
    if (!detail::project_name_valid(request.project_name) ||
        !detail::nonblank(inputs.target.registry_version) || !detail::nonblank(inputs.axiom_profile.identifier) ||
        (inputs.axiom_subject && !detail::nonblank(inputs.axiom_subject->identifier)))
        return blocked_plan("INVALID_GENERATION_INPUT", "Project names require nonempty ASCII letters, digits, dot, underscore, or hyphen; target provenance and axiom identity labels must be nonblank.");
    if (request.topology_depth < 0 || request.topology_depth > static_cast<std::int64_t>(TopologyGenerator::kDefaultMaxDepth))
        return blocked_plan("INVALID_TOPOLOGY_DEPTH", "The native structural topology supports depths zero through six; negative or excessive depths are rejected.");

    try {
        GenerationPlan plan;
        plan.request = request;
        plan.inputs = inputs;
        const auto normalized = StateModel{}.normalize(request.seed_state);
        if (!normalized.diagnostic || normalized.diagnostic->disposition != Disposition::RESOLVED)
            return blocked_plan("STATE_NORMALIZATION_FAILED", "Structural normalization did not resolve the supplied seed state.");
        plan.normalized_state = normalized.state;
        plan.transformed_state = normalized.state;
        plan.source_realm = RealmEncoder{}.encode_realm_identity(normalized.state).identity;
        const TransitionRegistry transitions;
        for (const auto& id : request.transition_sequence) {
            const auto index = detail::transition_index(id);
            if (!index) return blocked_plan("TRANSITION_REFERENCE_INVALID", "Every transition must reference APS-TRANSITION-CW-00 through APS-TRANSITION-CW-15; no transition was silently dropped.");
            const auto resolved = transitions.resolve_transition(plan.transformed_state, *index);
            if (!resolved.diagnostic || resolved.diagnostic->disposition != Disposition::RESOLVED)
                return blocked_plan("TRANSITION_RESOLUTION_FAILED", "The canonical transition registry did not resolve a requested transition.");
            const auto applied = transitions.apply_transition(plan.transformed_state, resolved.codeword);
            if (!applied.diagnostic || applied.diagnostic->disposition != Disposition::RESOLVED)
                return blocked_plan("TRANSITION_APPLICATION_FAILED", "A requested full-state XOR transformation failed; no partial plan is returned.");
            plan.transformed_state = applied.state;
        }
        plan.destination_realm = RealmEncoder{}.encode_realm_identity(plan.transformed_state).identity;
        const auto topology = TopologyGenerator{}.generate_topology(plan.transformed_state, static_cast<std::size_t>(request.topology_depth));
        if (!topology.diagnostic || topology.diagnostic->disposition != Disposition::RESOLVED)
            return blocked_plan("TOPOLOGY_GENERATION_FAILED", "The structural topology generator rejected the requested topology; no partial plan is returned.");
        plan.topology_nodes = topology.topology.nodes;
        plan.leaf_indices = topology.topology.leaf_indices;

        plan.source_signature = canonical_signature(normalized.state);
        plan.source_realm_id = plan.source_realm.realm_id;
        plan.target_class = inputs.target.target_class;
        plan.registry_version = inputs.target.registry_version;
        plan.runtime_hint = inputs.target.runtime_hint;
        plan.profile_id = request.profile_id;
        plan.role_assignments = {"source-state=" + plan.source_realm_id, "target-class=" + plan.target_class};
        std::vector<std::string> role_ids;
        for (const auto& node : plan.topology_nodes) {
            GenerationRoleAssignment role{"APS-ROLE-" + node.node_id, node.node_id,
                node.depth == 0 ? "root" : node.is_leaf ? "leaf" : "branch", plan.destination_realm.realm_id};
            role_ids.push_back(role.assignment_id);
            plan.role_assignments.push_back(detail::role_projection(role));
            plan.roles.push_back(std::move(role));
        }
        plan.axiom_subject = inputs.axiom_subject.value_or(AxiomSubject{
            request.project_name + "/" + plan.destination_realm.realm_id, {}, {}, {}, {}, {}, {}, {}});
        plan.axiom_profile = inputs.axiom_profile;
        plan.axiom_diagnostic = AxiomEvaluator{}.diagnose_axioms(plan.axiom_subject, plan.axiom_profile);
        plan.warnings = detail::generation_warnings(plan.axiom_diagnostic);
        const std::string prefix = request.naming_policy_id == "aps-windows-project-names-1.0" ? request.project_name + ":" : "";
        for (std::size_t i = 0; i < 2; ++i) {
            const std::string suffix = i == 0 ? "state-summary" : "diagnostic-chain-export";
            const std::string name = prefix + (i == 0 ? "canonical-state-summary:" : "diagnostic-chain-export:") + plan.destination_realm.realm_id;
            plan.artifacts.push_back({request.project_name + "/" + suffix, "descriptor", name, name, role_ids});
            plan.artifact_descriptions.push_back(name);
        }
        plan.metadata = detail::generation_metadata(request, inputs);
        plan.plan_hash = detail::sha256_hex(detail::canonical_plan_payload(plan));
        auto diagnostic = DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::GENERATION_PLAN).severity(Severity::INFO)
            .stage(Stage::PLAN).disposition(Disposition::RESOLVED)
            .subject("GenerationPlanner::generate_plan[" + plan.source_realm_id + "]")
            .rule_id(rule_ids::kGeneration001).rule_id(rule_ids::kGeneration002)
            .summary("Generated complete inspectable side-effect-free descriptor plan")
            .note("The plan records every requested transition, structural topology node and role, all five axiom evaluations, artifact description, target constraint, and versioned semantic identity. Missing or failing measurements remain explicit in the axiom records and warnings.")
            .build_root();
        return {std::move(plan), std::move(diagnostic)};
    } catch (const std::logic_error&) {
        return blocked_plan("INVALID_GENERATION_INPUT", "A supplied identity or target provenance value could not be represented in a valid canonical diagnostic record; no partial plan is returned.");
    }
}

PlanResult GenerationPlanner::generate_plan(const Bit9State& state, const TargetMetadata& target) const {
    const GenerationRequest request{"ash-plan", state, {}, 0, target.profile_id,
        "aps-windows-realm-names-1.0", "descriptor"};
    GenerationPlanningInputs inputs;
    inputs.target = target;
    return generate_plan(request, inputs);
}
}  // namespace ash
