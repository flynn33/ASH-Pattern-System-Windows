#pragma once
#include <algorithm>
#include <bit>
#include <optional>
#include <string>
#include <string_view>
#include "ash/Diagnostics.hpp"
#include "ash/GenerationPlanner.hpp"
#include "ash/TransitionRegistry.hpp"
#include "SemanticHash.hpp"

namespace ash::detail {
inline constexpr std::string_view kPlanIdentityVersion = "aps-windows-plan-2";

inline bool nonblank(std::string_view value) {
    return value.find_first_not_of(" \t\r\n") != std::string_view::npos;
}
inline bool project_name_valid(std::string_view value) {
    return !value.empty() && std::all_of(value.begin(), value.end(), [](char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
    });
}
inline bool supported_target(std::string_view value) {
    return value == "desktop" || value == "mobile" || value == "service";
}
inline bool supported_naming(std::string_view value) {
    return value == "aps-windows-realm-names-1.0" || value == "aps-windows-project-names-1.0";
}
inline std::optional<std::size_t> transition_index(std::string_view id) {
    constexpr std::string_view prefix = "APS-TRANSITION-CW-";
    if (id.size() != prefix.size() + 2 || id.substr(0, prefix.size()) != prefix ||
        id[prefix.size()] < '0' || id[prefix.size()] > '9' ||
        id[prefix.size() + 1] < '0' || id[prefix.size() + 1] > '9') return std::nullopt;
    const auto value = static_cast<std::size_t>((id[prefix.size()] - '0') * 10 + id[prefix.size() + 1] - '0');
    return value < kCodewordCount ? std::optional<std::size_t>{value} : std::nullopt;
}
inline void plan_field(std::string& out, std::string_view name, std::string_view value) {
    out.append(name);
    out += ':' + std::to_string(value.size()) + ':';
    out.append(value);
    out += '\n';
}
template<class T> inline void plan_number(std::string& out, std::string_view name, T value) {
    plan_field(out, name, std::to_string(value));
}
inline void plan_strings(std::string& out, std::string_view name, const std::vector<std::string>& values) {
    plan_number(out, std::string(name) + ".count", values.size());
    for (const auto& value : values) plan_field(out, std::string(name) + ".item", value);
}
inline void plan_realm(std::string& out, std::string_view name, const RealmIdentity& realm) {
    const std::string prefix = std::string(name) + '.';
    plan_field(out, prefix + "state_signature", realm.state_signature);
    plan_number(out, prefix + "realm_index", realm.realm_index);
    plan_field(out, prefix + "realm_id", realm.realm_id);
    plan_field(out, prefix + "orbit_id", realm.orbit_id);
    plan_number(out, prefix + "b8", realm.b8);
}
template<class T> inline void plan_optional(std::string& out, std::string_view name, const std::optional<T>& value) {
    plan_number(out, std::string(name) + ".present", value.has_value());
    if (value) plan_number(out, name, *value);
}
inline void plan_measurement(std::string& out, std::string_view name, const std::optional<double>& value) {
    plan_number(out, std::string(name) + ".present", value.has_value());
    if (value) plan_number(out, std::string(name) + ".ieee754_bits", std::bit_cast<std::uint64_t>(*value));
}
inline std::string subject_payload(const AxiomSubject& subject) {
    std::string out;
    plan_field(out, "identifier", subject.identifier);
    plan_optional(out, "relation_count", subject.relation_count);
    plan_measurement(out, "compressed_measure", subject.compressed_measure);
    plan_measurement(out, "raw_measure", subject.raw_measure);
    plan_optional(out, "scale_count", subject.scale_count);
    plan_measurement(out, "erasure_cost", subject.erasure_cost);
    plan_optional(out, "self_reference_capable", subject.self_reference_capable);
    plan_optional(out, "self_model_available", subject.self_model_available);
    return out;
}
inline std::string_view axiom_status_name(AxiomResultStatus status) {
    switch (status) {
        case AxiomResultStatus::PASS: return "PASS";
        case AxiomResultStatus::FAIL: return "FAIL";
        case AxiomResultStatus::INDETERMINATE: return "INDETERMINATE";
    }
    return "INVALID";
}
inline void plan_diagnostic(std::string& out, std::string_view name,
                            const std::shared_ptr<DiagnosticEnvelope>& diagnostic) {
    plan_number(out, std::string(name) + ".present", diagnostic != nullptr);
    if (diagnostic) plan_field(out, name, Diagnostics{}.serialize_record(*diagnostic));
}
inline std::string axiom_payload(const AxiomSubjectDiagnostic& result) {
    std::string out;
    plan_field(out, "a1_relational_existence", axiom_status_name(result.a1_relational_existence));
    plan_field(out, "a2_structural_compressibility", axiom_status_name(result.a2_structural_compressibility));
    plan_field(out, "a3_multi_scale_persistence", axiom_status_name(result.a3_multi_scale_persistence));
    plan_field(out, "a4_energetic_cost_of_erasure", axiom_status_name(result.a4_energetic_cost_of_erasure));
    plan_field(out, "a5_self_reference_for_consciousness", axiom_status_name(result.a5_self_reference_for_consciousness));
    plan_field(out, "overall_status", axiom_status_name(result.overall_status));
    plan_optional(out, "overall_pass", result.overall_pass);
    plan_number(out, "evaluations.count", result.evaluations.size());
    for (const auto& evaluation : result.evaluations) {
        plan_field(out, "evaluation.status", axiom_status_name(evaluation.status));
        plan_diagnostic(out, "evaluation.diagnostic", evaluation.diagnostic);
    }
    plan_diagnostic(out, "diagnostic", result.diagnostic);
    return out;
}
inline std::string canonical_plan_payload(const GenerationPlan& plan) {
    std::string out;
    plan_field(out, "identity_format", kPlanIdentityVersion);
    plan_field(out, "request.project_name", plan.request.project_name);
    plan_field(out, "request.seed_state", canonical_signature(plan.request.seed_state));
    plan_strings(out, "request.transition_sequence", plan.request.transition_sequence);
    plan_number(out, "request.topology_depth", plan.request.topology_depth);
    plan_field(out, "request.profile_id", plan.request.profile_id);
    plan_field(out, "request.naming_policy_id", plan.request.naming_policy_id);
    plan_field(out, "request.emission_target_kind", plan.request.emission_target_kind);
    plan_field(out, "inputs.target.target_class", plan.inputs.target.target_class);
    plan_field(out, "inputs.target.runtime_hint", plan.inputs.target.runtime_hint);
    plan_field(out, "inputs.target.profile_id", plan.inputs.target.profile_id);
    plan_field(out, "inputs.target.registry_version", plan.inputs.target.registry_version);
    plan_number(out, "inputs.axiom_subject.present", plan.inputs.axiom_subject.has_value());
    if (plan.inputs.axiom_subject) plan_field(out, "inputs.axiom_subject", subject_payload(*plan.inputs.axiom_subject));
    plan_field(out, "inputs.axiom_profile.identifier", plan.inputs.axiom_profile.identifier);
    plan_field(out, "source_signature", plan.source_signature);
    plan_field(out, "source_realm_id", plan.source_realm_id);
    plan_field(out, "target_class", plan.target_class);
    plan_field(out, "registry_version", plan.registry_version);
    plan_field(out, "runtime_hint", plan.runtime_hint);
    plan_field(out, "profile_id", plan.profile_id);
    plan_field(out, "normalized_state", canonical_signature(plan.normalized_state));
    plan_field(out, "transformed_state", canonical_signature(plan.transformed_state));
    plan_realm(out, "source_realm", plan.source_realm);
    plan_realm(out, "destination_realm", plan.destination_realm);
    plan_number(out, "topology_nodes.count", plan.topology_nodes.size());
    for (const auto& node : plan.topology_nodes) {
        plan_field(out, "node.path", node.path);
        plan_field(out, "node.node_id", node.node_id);
        plan_number(out, "node.depth", node.depth);
        plan_number(out, "node.generation_ordinal", node.generation_ordinal);
        plan_number(out, "node.global_ordinal", node.global_ordinal);
        plan_number(out, "node.parent_index", node.parent_index);
        plan_number(out, "node.has_parent", node.has_parent);
        plan_field(out, "node.parent_path", node.parent_path);
        plan_number(out, "node.is_leaf", node.is_leaf);
    }
    plan_number(out, "leaf_indices.count", plan.leaf_indices.size());
    for (const auto value : plan.leaf_indices) plan_number(out, "leaf_indices.item", value);
    plan_number(out, "roles.count", plan.roles.size());
    for (const auto& role : plan.roles) {
        plan_field(out, "role.assignment_id", role.assignment_id);
        plan_field(out, "role.node_id", role.node_id);
        plan_field(out, "role.role", role.role);
        plan_field(out, "role.realm_id", role.realm_id);
    }
    plan_strings(out, "role_assignments", plan.role_assignments);
    plan_field(out, "axiom_subject", subject_payload(plan.axiom_subject));
    plan_field(out, "axiom_profile.identifier", plan.axiom_profile.identifier);
    plan_field(out, "axiom_diagnostic", axiom_payload(plan.axiom_diagnostic));
    plan_number(out, "artifacts.count", plan.artifacts.size());
    for (const auto& artifact : plan.artifacts) {
        plan_field(out, "artifact.artifact_id", artifact.artifact_id);
        plan_field(out, "artifact.kind", artifact.kind);
        plan_field(out, "artifact.name", artifact.name);
        plan_field(out, "artifact.summary", artifact.summary);
        plan_strings(out, "artifact.role_references", artifact.role_references);
    }
    plan_strings(out, "artifact_descriptions", plan.artifact_descriptions);
    plan_strings(out, "warnings", plan.warnings);
    plan_number(out, "metadata.count", plan.metadata.size());
    for (const auto& [key, value] : plan.metadata) {
        plan_field(out, "metadata.key", key);
        plan_field(out, "metadata.value", value);
    }
    return out;
}
inline std::vector<std::string> generation_warnings(const AxiomSubjectDiagnostic& result) {
    std::vector<std::string> warnings;
    for (const auto& evaluation : result.evaluations) {
        if (evaluation.status != AxiomResultStatus::PASS && evaluation.diagnostic)
            warnings.push_back(evaluation.diagnostic->notes.front());
    }
    return warnings;
}
inline std::map<std::string, std::string, std::less<>> generation_metadata(
    const GenerationRequest& request, const GenerationPlanningInputs& inputs) {
    return {{"identity_format", std::string(kPlanIdentityVersion)},
            {"project_name", request.project_name}, {"profile_id", request.profile_id},
            {"profile_registry_version", "aps-windows-profiles-1.0"},
            {"naming_policy_id", request.naming_policy_id}, {"naming_registry_version", "aps-windows-naming-1.0"},
            {"emission_target_kind", request.emission_target_kind}, {"target_registry_version", "aps-windows-targets-1.0"},
            {"codeword_registry_version", "aps-canonical-codewords-1.0"},
            {"topology_registry_version", "aps-canonical-ternary-1.0"},
            {"axiom_predicate_version", "aps-canonical-five-axioms-1.0"},
            {"registry_version", inputs.target.registry_version}, {"runtime_hint", inputs.target.runtime_hint},
            {"target_class", inputs.target.target_class}, {"axiom_profile_id", inputs.axiom_profile.identifier},
            {"axiom_evidence_source", inputs.axiom_subject ? "CALLER_MEASUREMENTS" : "UNAVAILABLE"}};
}
inline std::string role_projection(const GenerationRoleAssignment& role) {
    return "node-role=" + role.assignment_id + ";node=" + role.node_id + ";role=" + role.role + ";realm=" + role.realm_id;
}
inline bool same_realm(const RealmIdentity& a, const RealmIdentity& b) {
    return a.state_signature == b.state_signature && a.realm_index == b.realm_index &&
           a.realm_id == b.realm_id && a.orbit_id == b.orbit_id && a.b8 == b.b8;
}
inline bool same_node(const TopologyNode& a, const TopologyNode& b) {
    return a.path == b.path && a.node_id == b.node_id && a.depth == b.depth &&
           a.generation_ordinal == b.generation_ordinal && a.global_ordinal == b.global_ordinal &&
           a.parent_index == b.parent_index && a.has_parent == b.has_parent &&
           a.parent_path == b.parent_path && a.is_leaf == b.is_leaf;
}

// Pure validation from the complete plan, with no planner callback or additional
// evidence. Fixed semantic modules audit stored results before descriptor output.
inline bool complete_plan_valid(const GenerationPlan& plan) {
    const auto& request = plan.request;
    const auto& inputs = plan.inputs;
    if (!project_name_valid(request.project_name) || request.profile_id != "aps-windows-descriptor-1.0" ||
        !supported_naming(request.naming_policy_id) || request.emission_target_kind != "descriptor" ||
        request.topology_depth < 0 || request.topology_depth > static_cast<std::int64_t>(TopologyGenerator::kDefaultMaxDepth) ||
        !supported_target(inputs.target.target_class) || !nonblank(inputs.target.registry_version) ||
        inputs.target.profile_id != request.profile_id || !nonblank(inputs.axiom_profile.identifier) ||
        (inputs.axiom_subject && !nonblank(inputs.axiom_subject->identifier)) ||
        plan.normalized_state != request.seed_state || plan.source_signature != canonical_signature(plan.normalized_state) ||
        plan.source_realm_id != canonical_realm_id(plan.normalized_state) || plan.profile_id != request.profile_id ||
        plan.target_class != inputs.target.target_class || plan.registry_version != inputs.target.registry_version ||
        plan.runtime_hint != inputs.target.runtime_hint || plan.axiom_profile.identifier != inputs.axiom_profile.identifier ||
        plan.metadata != generation_metadata(request, inputs)) return false;
    const RealmEncoder encoder;
    if (!same_realm(plan.source_realm, encoder.encode_realm_identity(plan.normalized_state).identity) ||
        !same_realm(plan.destination_realm, encoder.encode_realm_identity(plan.transformed_state).identity)) return false;
    Bit9State transformed = plan.normalized_state;
    const TransitionRegistry transitions;
    for (const auto& id : request.transition_sequence) {
        const auto index = transition_index(id);
        if (!index) return false;
        const auto resolved = transitions.resolve_transition(transformed, *index);
        if (!resolved.diagnostic || resolved.diagnostic->disposition != Disposition::RESOLVED) return false;
        const auto applied = transitions.apply_transition(transformed, resolved.codeword);
        if (!applied.diagnostic || applied.diagnostic->disposition != Disposition::RESOLVED) return false;
        transformed = applied.state;
    }
    if (transformed != plan.transformed_state) return false;
    const auto topology = TopologyGenerator{}.generate_topology(transformed, static_cast<std::size_t>(request.topology_depth));
    if (plan.topology_nodes.size() != topology.topology.nodes.size() || plan.leaf_indices != topology.topology.leaf_indices ||
        plan.roles.size() != plan.topology_nodes.size() || plan.role_assignments.size() != plan.roles.size() + 2 ||
        plan.role_assignments[0] != "source-state=" + plan.source_realm_id ||
        plan.role_assignments[1] != "target-class=" + plan.target_class) return false;
    for (std::size_t i = 0; i < plan.topology_nodes.size(); ++i) {
        const auto& node = plan.topology_nodes[i];
        const auto& role = plan.roles[i];
        if (!same_node(node, topology.topology.nodes[i]) || role.assignment_id != "APS-ROLE-" + node.node_id ||
            role.node_id != node.node_id || role.role != (node.depth == 0 ? "root" : node.is_leaf ? "leaf" : "branch") ||
            role.realm_id != plan.destination_realm.realm_id || plan.role_assignments[i + 2] != role_projection(role)) return false;
    }
    const AxiomSubject expected_subject = inputs.axiom_subject.value_or(AxiomSubject{
        request.project_name + "/" + plan.destination_realm.realm_id, {}, {}, {}, {}, {}, {}, {}});
    if (subject_payload(expected_subject) != subject_payload(plan.axiom_subject)) return false;
    const auto expected_axioms = AxiomEvaluator{}.diagnose_axioms(plan.axiom_subject, plan.axiom_profile);
    if (axiom_payload(expected_axioms) != axiom_payload(plan.axiom_diagnostic) ||
        plan.warnings != generation_warnings(expected_axioms)) return false;
    if (plan.artifacts.size() != 2 || plan.artifact_descriptions.size() != plan.artifacts.size()) return false;
    std::vector<std::string> role_ids;
    for (const auto& role : plan.roles) role_ids.push_back(role.assignment_id);
    const std::string prefix = request.naming_policy_id == "aps-windows-project-names-1.0" ? request.project_name + ":" : "";
    for (std::size_t i = 0; i < plan.artifacts.size(); ++i) {
        const auto& artifact = plan.artifacts[i];
        const std::string suffix = i == 0 ? "state-summary" : "diagnostic-chain-export";
        const std::string name = prefix + (i == 0 ? "canonical-state-summary:" : "diagnostic-chain-export:") + plan.destination_realm.realm_id;
        if (artifact.artifact_id != request.project_name + "/" + suffix || artifact.kind != "descriptor" ||
            artifact.name != name || artifact.summary != name || artifact.role_references != role_ids ||
            plan.artifact_descriptions[i] != artifact.summary) return false;
    }
    return plan.plan_hash == sha256_hex(canonical_plan_payload(plan));
}
}  // namespace ash::detail
