// CompletionModules.test.cpp
//
// Completion coverage for Windows semantic modules and canonical realm
// encoding helpers.

#include "test_runner.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <functional>
#include <string>
#include <vector>

#include "ash/ArtifactEmitter.hpp"
#include "ash/AxiomEvaluator.hpp"
#include "ash/Diagnostics.hpp"
#include "ash/GenerationPlanner.hpp"
#include "ash/RealmEncoder.hpp"
#include "ash/State.hpp"
#include "ash/TopologyGenerator.hpp"
#include "../src/SemanticHash.hpp"

using namespace ash;

TEST_CASE(from_int_uses_canonical_msb_bit_order) {
    ASSERT_EQ(canonical_signature(from_int(0)), std::string{"000000000"});
    ASSERT_EQ(canonical_signature(from_int(1)), std::string{"000000001"});
    ASSERT_EQ(canonical_signature(from_int(256)), std::string{"100000000"});
    ASSERT_EQ(to_int(from_int(511)), static_cast<std::uint16_t>(511));
}

TEST_CASE(realm_encoder_returns_canonical_identity) {
    RealmEncoder encoder;
    auto result = encoder.encode_realm_identity(from_int(1));
    ASSERT_EQ(result.identity.state_signature, std::string{"000000001"});
    ASSERT_EQ(result.identity.realm_index, static_cast<std::uint16_t>(1));
    ASSERT_EQ(result.identity.realm_id, std::string{"APS-REALM-001"});
    ASSERT_EQ(result.identity.orbit_id, std::string{"APS-ORBIT-01"});
    ASSERT_EQ(result.identity.b8, true);
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::RESOLVED);
}

TEST_CASE(realm_encoder_matches_canonical_orbit_partition_for_all_512_states) {
    RealmEncoder encoder;
    std::array<int, 32> orbit_counts{};

    ASSERT_EQ(encoder.encode_realm_identity(from_int(0)).identity.orbit_id,
              std::string{"APS-ORBIT-00"});
    ASSERT_EQ(encoder.encode_realm_identity(from_int(1)).identity.orbit_id,
              std::string{"APS-ORBIT-01"});
    ASSERT_EQ(encoder.encode_realm_identity(from_int(30)).identity.orbit_id,
              std::string{"APS-ORBIT-00"});
    ASSERT_EQ(encoder.encode_realm_identity(from_int(31)).identity.orbit_id,
              std::string{"APS-ORBIT-01"});
    ASSERT_EQ(encoder.encode_realm_identity(from_int(32)).identity.orbit_id,
              std::string{"APS-ORBIT-16"});
    ASSERT_EQ(encoder.encode_realm_identity(from_int(47)).identity.orbit_id,
              std::string{"APS-ORBIT-31"});

    for (std::uint16_t i = 0; i < kStateSpaceSize; ++i) {
        auto result = encoder.encode_realm_identity(from_int(i));
        ASSERT_EQ(result.identity.realm_index, i);
        const int orbit_index = std::stoi(result.identity.orbit_id.substr(10));
        ASSERT_TRUE(orbit_index >= 0 && orbit_index < 32);
        ++orbit_counts[static_cast<std::size_t>(orbit_index)];
    }

    for (const int count : orbit_counts) {
        ASSERT_EQ(count, 16);
    }
}

TEST_CASE(topology_generator_builds_ordered_ternary_tree) {
    TopologyGenerator generator;
    auto result = generator.generate_topology(from_int(0), 2);
    ASSERT_EQ(result.topology.nodes.size(), static_cast<std::size_t>(13));
    ASSERT_EQ(result.topology.leaf_indices.size(), static_cast<std::size_t>(9));
    ASSERT_EQ(result.topology.nodes[0].node_id, std::string{"APS-NODE-R"});
    ASSERT_EQ(result.topology.nodes[1].node_id, std::string{"APS-NODE-R.C"});
    ASSERT_EQ(result.topology.nodes[4].node_id, std::string{"APS-NODE-R.C.C"});
    ASSERT_EQ(result.topology.nodes[12].path, std::string{"R/N/N"});
    ASSERT_EQ(result.topology.nodes[12].global_ordinal, static_cast<std::size_t>(12));
    ASSERT_EQ(result.diagnostic->disposition, Disposition::RESOLVED);
}

TEST_CASE(topology_generator_rejects_depth_over_limit_before_output) {
    TopologyGenerator generator;
    auto result = generator.generate_topology(from_int(0), TopologyGenerator::kDefaultMaxDepth + 1);
    ASSERT_TRUE(result.topology.nodes.empty());
    ASSERT_TRUE(result.topology.leaf_indices.empty());
    ASSERT_EQ(result.diagnostic->severity, Severity::ERROR);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::BLOCKED);
}

TEST_CASE(axiom_evaluator_reports_pass_fail_and_indeterminate) {
    AxiomEvaluator evaluator;
    auto pass = evaluator.evaluate_axiom(from_int(0), Axiom{"AX-REL", AxiomEvidence::PRESENT_PASS, true, ""});
    auto fail = evaluator.evaluate_axiom(from_int(0), Axiom{"AX-REL", AxiomEvidence::PRESENT_FAIL, true, ""});
    auto missing = evaluator.evaluate_axiom(from_int(0), Axiom{"AX-REL", AxiomEvidence::MISSING, true, ""});
    ASSERT_EQ(pass.status, AxiomResultStatus::PASS);
    ASSERT_EQ(fail.status, AxiomResultStatus::FAIL);
    ASSERT_EQ(missing.status, AxiomResultStatus::INDETERMINATE);
    ASSERT_EQ(missing.diagnostic->disposition, Disposition::PENDING);
}

TEST_CASE(axiom_evaluator_aggregates_required_axioms) {
    AxiomEvaluator evaluator;
    std::vector<Axiom> axioms{
        Axiom{"AX-REL", AxiomEvidence::PRESENT_PASS, true, ""},
        Axiom{"AX-COMP", AxiomEvidence::MISSING, true, ""},
    };
    auto result = evaluator.evaluate_axioms(from_int(0), axioms);
    ASSERT_EQ(result.overall_status, AxiomResultStatus::INDETERMINATE);
    ASSERT_EQ(result.results.size(), static_cast<std::size_t>(2));
}

TEST_CASE(generation_planner_produces_inspectable_hashed_plan) {
    GenerationPlanner planner;
    TargetMetadata target;
    target.target_class = "desktop";
    target.runtime_hint = "win32";
    auto result = planner.generate_plan(from_int(30), target);
    ASSERT_EQ(result.plan.source_realm_id, std::string{"APS-REALM-030"});
    ASSERT_EQ(result.plan.target_class, std::string{"desktop"});
    ASSERT_TRUE(!result.plan.artifact_descriptions.empty());
    ASSERT_EQ(result.plan.plan_hash.size(), static_cast<std::size_t>(64));
    ASSERT_EQ(result.diagnostic->disposition, Disposition::RESOLVED);
}

TEST_CASE(artifact_emitter_traces_each_artifact_to_plan_element) {
    GenerationPlanner planner;
    TargetMetadata target;
    target.target_class = "desktop";
    auto plan = planner.generate_plan(from_int(0), target).plan;

    ArtifactEmitter emitter;
    TargetConfiguration config;
    config.target_class = "desktop";
    auto result = emitter.materialize_plan(plan, config);

    ASSERT_EQ(result.artifacts.size(), plan.artifact_descriptions.size());
    ASSERT_EQ(result.artifacts.front().plan_element_reference,
              std::string{"artifact_descriptions[0]"});
    ASSERT_EQ(result.diagnostic->disposition, Disposition::RESOLVED);
}

TEST_CASE(artifact_emitter_refuses_incomplete_plan) {
    ArtifactEmitter emitter;
    TargetConfiguration config;
    config.target_class = "desktop";
    auto result = emitter.materialize_plan(GenerationPlan{}, config);
    ASSERT_TRUE(result.artifacts.empty());
    ASSERT_EQ(result.diagnostic->severity, Severity::ERROR);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::BLOCKED);
}

namespace {
AxiomSubject passing_axiom_subject() {
    return {"measured-subject", 1, 0.5, 1.0, 2, 1.0, true, true};
}

void assert_axiom_envelopes(const AxiomSubjectDiagnostic& result) {
    const Diagnostics diagnostics;
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_TRUE(diagnostics.validate_chain(*result.diagnostic));
    ASSERT_TRUE(result.diagnostic->notes.size() >= 5);
    for (const auto& evaluation : result.evaluations) {
        ASSERT_TRUE(evaluation.diagnostic != nullptr);
        ASSERT_TRUE(diagnostics.validate_chain(*evaluation.diagnostic));
        ASSERT_TRUE(!evaluation.diagnostic->summary.empty());
        ASSERT_TRUE(!evaluation.diagnostic->notes.empty());
        ASSERT_EQ(evaluation.diagnostic->diagnostic_kind, DiagnosticKind::AXIOM);
        ASSERT_EQ(evaluation.diagnostic->stage, Stage::EVALUATE);
    }
}
}

TEST_CASE(axiom_subject_evaluates_all_five_actual_predicates) {
    const auto result = AxiomEvaluator{}.diagnose_axioms(passing_axiom_subject());
    ASSERT_EQ(result.a1_relational_existence, AxiomResultStatus::PASS);
    ASSERT_EQ(result.a2_structural_compressibility, AxiomResultStatus::PASS);
    ASSERT_EQ(result.a3_multi_scale_persistence, AxiomResultStatus::PASS);
    ASSERT_EQ(result.a4_energetic_cost_of_erasure, AxiomResultStatus::PASS);
    ASSERT_EQ(result.a5_self_reference_for_consciousness, AxiomResultStatus::PASS);
    ASSERT_EQ(result.overall_status, AxiomResultStatus::PASS);
    ASSERT_TRUE(result.overall_pass.has_value() && *result.overall_pass);
    ASSERT_EQ(result.diagnostic->severity, Severity::INFO);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::RESOLVED);
    assert_axiom_envelopes(result);
}

TEST_CASE(axiom_subject_threshold_boundaries_fail_without_short_circuiting) {
    const AxiomSubject subject{"boundary-subject", 0, 1.0, 1.0, 1, 0.0, false, false};
    const auto result = AxiomEvaluator{}.diagnose_axioms(subject);
    for (const auto& evaluation : result.evaluations) ASSERT_EQ(evaluation.status, AxiomResultStatus::FAIL);
    ASSERT_EQ(result.overall_status, AxiomResultStatus::FAIL);
    ASSERT_TRUE(result.overall_pass.has_value() && !*result.overall_pass);
    ASSERT_EQ(result.diagnostic->severity, Severity::ERROR);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::BLOCKED);
    assert_axiom_envelopes(result);
}

TEST_CASE(axiom_subject_numeric_comparisons_use_exact_supplied_measurements) {
    auto subject = passing_axiom_subject();
    subject.relation_count = std::numeric_limits<std::uint64_t>::max();
    subject.scale_count = std::numeric_limits<std::uint64_t>::max();
    subject.compressed_measure = std::nextafter(1.0, 0.0);
    subject.erasure_cost = std::numeric_limits<double>::denorm_min();
    ASSERT_EQ(AxiomEvaluator{}.diagnose_axioms(subject).overall_status, AxiomResultStatus::PASS);
    subject.compressed_measure = std::nextafter(1.0, 2.0);
    subject.erasure_cost = -1.0;
    const auto result = AxiomEvaluator{}.diagnose_axioms(subject);
    ASSERT_EQ(result.a2_structural_compressibility, AxiomResultStatus::FAIL);
    ASSERT_EQ(result.a4_energetic_cost_of_erasure, AxiomResultStatus::FAIL);
    ASSERT_EQ(result.overall_status, AxiomResultStatus::FAIL);
}

TEST_CASE(axiom_subject_requires_both_self_reference_measurements_true) {
    const std::array<AxiomResultStatus, 4> expected{
        AxiomResultStatus::FAIL, AxiomResultStatus::FAIL, AxiomResultStatus::FAIL, AxiomResultStatus::PASS};
    for (std::size_t index = 0; index < expected.size(); ++index) {
        auto subject = passing_axiom_subject();
        subject.self_reference_capable = (index & 2) != 0;
        subject.self_model_available = (index & 1) != 0;
        const auto result = AxiomEvaluator{}.diagnose_axioms(subject);
        ASSERT_EQ(result.a5_self_reference_for_consciousness, expected[index]);
        ASSERT_EQ(result.overall_status, expected[index]);
    }
}

TEST_CASE(axiom_subject_each_missing_measurement_stays_indeterminate) {
    const std::array<std::size_t, 7> affected{0, 1, 1, 2, 3, 4, 4};
    for (std::size_t missing = 0; missing < affected.size(); ++missing) {
        auto subject = passing_axiom_subject();
        switch (missing) {
            case 0: subject.relation_count.reset(); break;
            case 1: subject.compressed_measure.reset(); break;
            case 2: subject.raw_measure.reset(); break;
            case 3: subject.scale_count.reset(); break;
            case 4: subject.erasure_cost.reset(); break;
            case 5: subject.self_reference_capable.reset(); break;
            case 6: subject.self_model_available.reset(); break;
        }
        const auto result = AxiomEvaluator{}.diagnose_axioms(subject);
        ASSERT_EQ(result.overall_status, AxiomResultStatus::INDETERMINATE);
        ASSERT_TRUE(!result.overall_pass.has_value());
        for (std::size_t index = 0; index < result.evaluations.size(); ++index) {
            ASSERT_EQ(result.evaluations[index].status,
                      index == affected[missing] ? AxiomResultStatus::INDETERMINATE : AxiomResultStatus::PASS);
        }
        ASSERT_EQ(result.diagnostic->severity, Severity::WARNING);
        ASSERT_EQ(result.diagnostic->disposition, Disposition::PENDING);
        assert_axiom_envelopes(result);
    }
}

TEST_CASE(axiom_subject_known_failure_takes_precedence_over_missing_evidence) {
    auto subject = passing_axiom_subject();
    subject.relation_count = 0;
    subject.raw_measure.reset();
    const auto result = AxiomEvaluator{}.diagnose_axioms(subject);
    ASSERT_EQ(result.a1_relational_existence, AxiomResultStatus::FAIL);
    ASSERT_EQ(result.a2_structural_compressibility, AxiomResultStatus::INDETERMINATE);
    ASSERT_EQ(result.overall_status, AxiomResultStatus::FAIL);
    ASSERT_TRUE(result.overall_pass.has_value() && !*result.overall_pass);
    ASSERT_EQ(result.diagnostic->severity, Severity::ERROR);
    assert_axiom_envelopes(result);
}

TEST_CASE(axiom_subject_nonfinite_measurements_do_not_become_false_facts) {
    auto subject = passing_axiom_subject();
    subject.raw_measure = std::numeric_limits<double>::quiet_NaN();
    subject.compressed_measure = std::numeric_limits<double>::infinity();
    subject.erasure_cost = -std::numeric_limits<double>::infinity();
    const auto result = AxiomEvaluator{}.diagnose_axioms(subject);
    ASSERT_EQ(result.a2_structural_compressibility, AxiomResultStatus::INDETERMINATE);
    ASSERT_EQ(result.a4_energetic_cost_of_erasure, AxiomResultStatus::INDETERMINATE);
    ASSERT_EQ(result.overall_status, AxiomResultStatus::INDETERMINATE);
    ASSERT_TRUE(!result.overall_pass.has_value());
    ASSERT_TRUE(result.evaluations[1].diagnostic->notes.front().find("nonfinite") != std::string::npos);
    assert_axiom_envelopes(result);
}

TEST_CASE(axiom_subject_empty_identity_and_profile_block_positive_conclusions) {
    auto subject = passing_axiom_subject();
    auto result = AxiomEvaluator{}.diagnose_axioms(subject, AxiomEvaluationProfile{""});
    ASSERT_EQ(result.overall_status, AxiomResultStatus::INDETERMINATE);
    ASSERT_TRUE(!result.overall_pass.has_value());
    ASSERT_EQ(result.diagnostic->severity, Severity::ERROR);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::BLOCKED);
    subject.identifier.clear();
    result = AxiomEvaluator{}.diagnose_axioms(subject);
    for (const auto& evaluation : result.evaluations) ASSERT_EQ(evaluation.status, AxiomResultStatus::INDETERMINATE);
    ASSERT_TRUE(!result.overall_pass.has_value());
    assert_axiom_envelopes(result);
}

TEST_CASE(axiom_subject_equal_inputs_produce_equal_complete_diagnostic_semantics) {
    auto subject = passing_axiom_subject();
    subject.erasure_cost = 0.0;
    const AxiomEvaluationProfile profile{"caller-qualified-profile"};
    const AxiomEvaluator evaluator;
    const auto first = evaluator.diagnose_axioms(subject, profile);
    const auto repeated = evaluator.diagnose_axioms(subject, profile);
    ASSERT_EQ(first.overall_status, repeated.overall_status);
    ASSERT_EQ(first.overall_pass, repeated.overall_pass);
    ASSERT_EQ(first.diagnostic->summary, repeated.diagnostic->summary);
    ASSERT_EQ(first.diagnostic->notes, repeated.diagnostic->notes);
    ASSERT_EQ(first.diagnostic->subject_reference, repeated.diagnostic->subject_reference);
    for (std::size_t index = 0; index < first.evaluations.size(); ++index) {
        ASSERT_EQ(first.evaluations[index].status, repeated.evaluations[index].status);
        ASSERT_EQ(first.evaluations[index].diagnostic->summary, repeated.evaluations[index].diagnostic->summary);
        ASSERT_EQ(first.evaluations[index].diagnostic->notes, repeated.evaluations[index].diagnostic->notes);
        ASSERT_TRUE(first.evaluations[index].diagnostic->notes.front().find("a" + std::to_string(index + 1) + "_") != std::string::npos);
    }
}

TEST_CASE(axiom_subject_equal_signed_zero_measurements_have_equal_diagnostics) {
    auto positive = passing_axiom_subject();
    positive.compressed_measure = 0.0;
    positive.erasure_cost = 0.0;
    auto negative = positive;
    negative.compressed_measure = -0.0;
    negative.erasure_cost = -0.0;
    const AxiomEvaluator evaluator;
    const auto first = evaluator.diagnose_axioms(positive);
    const auto repeated = evaluator.diagnose_axioms(negative);
    ASSERT_EQ(first.overall_status, repeated.overall_status);
    ASSERT_EQ(first.diagnostic->notes, repeated.diagnostic->notes);
    for (std::size_t index = 0; index < first.evaluations.size(); ++index) {
        ASSERT_EQ(first.evaluations[index].diagnostic->notes, repeated.evaluations[index].diagnostic->notes);
    }
}

namespace {
GenerationRequest measured_plan_request() {
    return {"example", from_int(1), {"APS-TRANSITION-CW-01", "APS-TRANSITION-CW-02"}, 2,
            "aps-windows-descriptor-1.0", "aps-windows-project-names-1.0", "descriptor"};
}
GenerationPlanningInputs measured_plan_inputs() {
    GenerationPlanningInputs inputs;
    inputs.target = {"desktop", "win32"};
    inputs.axiom_subject = passing_axiom_subject();
    return inputs;
}
void assert_emission_blocked(const GenerationPlan& plan) {
    const auto result = ArtifactEmitter{}.materialize_plan(plan, {});
    ASSERT_TRUE(result.artifacts.empty());
    ASSERT_EQ(result.diagnostic->disposition, Disposition::BLOCKED);
    ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
}
void assert_plan_blocked(const GenerationRequest& request, const GenerationPlanningInputs& inputs = {}) {
    const auto result = GenerationPlanner{}.generate_plan(request, inputs);
    ASSERT_EQ(result.diagnostic->disposition, Disposition::BLOCKED);
    ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
    ASSERT_TRUE(result.plan.plan_hash.empty());
    ASSERT_TRUE(result.plan.topology_nodes.empty());
    ASSERT_TRUE(result.plan.artifacts.empty());
}
}

TEST_CASE(generation_request_applies_full_state_transitions_and_complete_topology) {
    const auto result = GenerationPlanner{}.generate_plan(measured_plan_request(), measured_plan_inputs());
    const auto& plan = result.plan;
    // CW-01=30, CW-02=102: 1 XOR 30 XOR 102 = 121.
    ASSERT_EQ(to_int(plan.normalized_state), static_cast<std::uint16_t>(1));
    ASSERT_EQ(to_int(plan.transformed_state), static_cast<std::uint16_t>(121));
    ASSERT_EQ(plan.source_realm.realm_id, std::string{"APS-REALM-001"});
    ASSERT_EQ(plan.destination_realm.realm_id, std::string{"APS-REALM-121"});
    ASSERT_EQ(plan.topology_nodes.size(), static_cast<std::size_t>(13));
    ASSERT_EQ(plan.leaf_indices, (std::vector<std::size_t>{4, 5, 6, 7, 8, 9, 10, 11, 12}));
    ASSERT_EQ(plan.topology_nodes[4].path, std::string{"R/C/C"});
    ASSERT_EQ(plan.topology_nodes[12].path, std::string{"R/N/N"});
    ASSERT_EQ(plan.topology_nodes[12].parent_index, static_cast<std::size_t>(3));
    ASSERT_EQ(plan.roles.size(), plan.topology_nodes.size());
    ASSERT_EQ(plan.roles[0].role, std::string{"root"});
    ASSERT_EQ(plan.roles[1].role, std::string{"branch"});
    ASSERT_EQ(plan.roles[4].role, std::string{"leaf"});
    for (std::size_t i = 0; i < plan.roles.size(); ++i) {
        ASSERT_EQ(plan.roles[i].node_id, plan.topology_nodes[i].node_id);
        ASSERT_EQ(plan.roles[i].realm_id, std::string{"APS-REALM-121"});
        ASSERT_EQ(plan.artifacts[0].role_references[i], plan.roles[i].assignment_id);
        ASSERT_EQ(plan.artifacts[1].role_references[i], plan.roles[i].assignment_id);
    }
    ASSERT_EQ(plan.artifacts[0].name, std::string{"example:canonical-state-summary:APS-REALM-121"});
    ASSERT_EQ(plan.artifacts[0].artifact_id, std::string{"example/state-summary"});
    ASSERT_EQ(plan.metadata.at("identity_format"), std::string{"aps-windows-plan-2"});
    ASSERT_EQ(plan.axiom_diagnostic.overall_status, AxiomResultStatus::PASS);
    ASSERT_TRUE(plan.warnings.empty());
    ASSERT_EQ(result.diagnostic->disposition, Disposition::RESOLVED);
    ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
    assert_axiom_envelopes(plan.axiom_diagnostic);
}

TEST_CASE(generation_legacy_defaults_preserve_unknown_measured_axioms) {
    for (const auto seed : {0, 30, 511}) {
        const auto plan = GenerationPlanner{}.generate_plan(from_int(static_cast<std::uint16_t>(seed)), {"desktop", "win32"}).plan;
        ASSERT_EQ(plan.request.project_name, std::string{"ash-plan"});
        ASSERT_TRUE(plan.request.transition_sequence.empty());
        ASSERT_EQ(plan.request.topology_depth, static_cast<std::int64_t>(0));
        ASSERT_EQ(plan.request.naming_policy_id, std::string{"aps-windows-realm-names-1.0"});
        ASSERT_EQ(plan.request.emission_target_kind, std::string{"descriptor"});
        ASSERT_EQ(plan.transformed_state, plan.normalized_state);
        ASSERT_EQ(plan.topology_nodes.size(), static_cast<std::size_t>(1));
        ASSERT_EQ(plan.topology_nodes[0].path, std::string{"R"});
        ASSERT_TRUE(plan.topology_nodes[0].is_leaf);
        ASSERT_TRUE(!plan.inputs.axiom_subject.has_value());
        ASSERT_TRUE(!plan.axiom_subject.relation_count.has_value());
        ASSERT_TRUE(!plan.axiom_subject.raw_measure.has_value());
        ASSERT_TRUE(!plan.axiom_subject.compressed_measure.has_value());
        ASSERT_TRUE(!plan.axiom_subject.scale_count.has_value());
        ASSERT_TRUE(!plan.axiom_subject.erasure_cost.has_value());
        ASSERT_TRUE(!plan.axiom_subject.self_reference_capable.has_value());
        ASSERT_TRUE(!plan.axiom_subject.self_model_available.has_value());
        ASSERT_EQ(plan.axiom_diagnostic.overall_status, AxiomResultStatus::INDETERMINATE);
        ASSERT_TRUE(!plan.axiom_diagnostic.overall_pass.has_value());
        ASSERT_EQ(plan.warnings.size(), static_cast<std::size_t>(5));
        assert_axiom_envelopes(plan.axiom_diagnostic);
        ASSERT_EQ(ArtifactEmitter{}.materialize_plan(plan, {}).artifacts.size(), static_cast<std::size_t>(2));
    }
}

TEST_CASE(generation_plan_preserves_known_failures_and_missing_measurements) {
    auto inputs = measured_plan_inputs();
    inputs.axiom_subject->relation_count = 0;
    inputs.axiom_subject->compressed_measure.reset();
    const auto plan = GenerationPlanner{}.generate_plan(measured_plan_request(), inputs).plan;
    ASSERT_EQ(plan.axiom_diagnostic.a1_relational_existence, AxiomResultStatus::FAIL);
    ASSERT_EQ(plan.axiom_diagnostic.a2_structural_compressibility, AxiomResultStatus::INDETERMINATE);
    ASSERT_EQ(plan.axiom_diagnostic.overall_status, AxiomResultStatus::FAIL);
    ASSERT_EQ(plan.warnings.size(), static_cast<std::size_t>(2));
    ASSERT_TRUE(plan.warnings[0].find("relation_count=0") != std::string::npos);
    ASSERT_TRUE(plan.warnings[1].find("compressed_measure=unavailable") != std::string::npos);
    ASSERT_TRUE(plan.axiom_diagnostic.overall_pass.has_value() && !*plan.axiom_diagnostic.overall_pass);
    assert_axiom_envelopes(plan.axiom_diagnostic);
    // An abstract descriptor reports failures; it does not authorize runtime execution.
    ASSERT_EQ(ArtifactEmitter{}.materialize_plan(plan, {}).artifacts.size(), static_cast<std::size_t>(2));
}

TEST_CASE(generation_plan_nonfinite_measurements_remain_unavailable) {
    auto inputs = measured_plan_inputs();
    inputs.axiom_subject->erasure_cost = std::numeric_limits<double>::infinity();
    const auto plan = GenerationPlanner{}.generate_plan(measured_plan_request(), inputs).plan;
    ASSERT_EQ(plan.axiom_diagnostic.a4_energetic_cost_of_erasure, AxiomResultStatus::INDETERMINATE);
    ASSERT_EQ(plan.warnings.size(), static_cast<std::size_t>(1));
    ASSERT_TRUE(plan.warnings[0].find("nonfinite") != std::string::npos);
    ASSERT_EQ(ArtifactEmitter{}.materialize_plan(plan, {}).artifacts.size(), static_cast<std::size_t>(2));
}

TEST_CASE(generation_plan_rejects_unknown_registry_references_and_invalid_inputs) {
    const auto original = measured_plan_request();
    for (const auto& edit : std::vector<std::function<void(GenerationRequest&)>>{
        [](auto& r) { r.profile_id = "DEFAULT"; },
        [](auto& r) { r.naming_policy_id = "BASELINE"; },
        [](auto& r) { r.emission_target_kind = "file"; },
        [](auto& r) { r.project_name.clear(); },
        [](auto& r) { r.project_name = "contains/path"; },
        [](auto& r) { r.topology_depth = -1; },
        [](auto& r) { r.topology_depth = 7; },
        [](auto& r) { r.transition_sequence.push_back("APS-TRANSITION-CW-16"); },
        [](auto& r) { r.transition_sequence.push_back("APS-TRANSITION-CW-1"); },
        [](auto& r) { r.transition_sequence.push_back(""); }}) {
        auto request = original;
        edit(request);
        assert_plan_blocked(request, measured_plan_inputs());
    }
    for (const auto& edit : std::vector<std::function<void(GenerationPlanningInputs&)>>{
        [](auto& i) { i.target.target_class = "unknown"; },
        [](auto& i) { i.target.registry_version = " \t"; },
        [](auto& i) { i.target.profile_id = "unknown"; },
        [](auto& i) { i.axiom_profile.identifier.clear(); },
        [](auto& i) { i.axiom_subject->identifier.clear(); }}) {
        auto inputs = measured_plan_inputs();
        edit(inputs);
        assert_plan_blocked(original, inputs);
    }
}

TEST_CASE(generation_plan_transition_involution_preserves_ninth_coordinate) {
    auto request = measured_plan_request();
    request.seed_state = from_int(511);
    request.transition_sequence = {"APS-TRANSITION-CW-15", "APS-TRANSITION-CW-15"};
    const auto plan = GenerationPlanner{}.generate_plan(request).plan;
    ASSERT_EQ(to_int(plan.transformed_state), static_cast<std::uint16_t>(511));
    ASSERT_TRUE(plan.destination_realm.b8);
    request.transition_sequence.pop_back();
    const auto once = GenerationPlanner{}.generate_plan(request).plan;
    ASSERT_EQ(to_int(once.transformed_state), static_cast<std::uint16_t>(1));
    ASSERT_TRUE(once.destination_realm.b8);
}

TEST_CASE(generation_plan_supports_declared_depth_and_target_class_limits) {
    auto request = measured_plan_request();
    request.topology_depth = 6;
    for (const auto& target : {"desktop", "mobile", "service"}) {
        auto inputs = measured_plan_inputs();
        inputs.target.target_class = target;
        const auto plan = GenerationPlanner{}.generate_plan(request, inputs).plan;
        ASSERT_EQ(plan.topology_nodes.size(), static_cast<std::size_t>(1093));
        ASSERT_EQ(plan.leaf_indices.size(), static_cast<std::size_t>(729));
        ASSERT_EQ(plan.roles.size(), plan.topology_nodes.size());
        ASSERT_EQ(ArtifactEmitter{}.materialize_plan(plan, {"", target}).artifacts.size(), static_cast<std::size_t>(2));
    }
}

TEST_CASE(generation_identity_binds_each_request_target_and_evidence_input) {
    const auto request = measured_plan_request();
    const auto inputs = measured_plan_inputs();
    const auto base = GenerationPlanner{}.generate_plan(request, inputs).plan;
    const auto repeated = GenerationPlanner{}.generate_plan(request, inputs).plan;
    ASSERT_EQ(base.plan_hash, repeated.plan_hash);
    ASSERT_EQ(generation_plan_identity_payload(base), generation_plan_identity_payload(repeated));
    ASSERT_EQ(base.plan_hash, detail::sha256_hex(generation_plan_identity_payload(base)));
    for (const auto& edit : std::vector<std::function<void(GenerationRequest&)>>{
        [](auto& r) { r.project_name = "other"; },
        [](auto& r) { r.seed_state = from_int(257); },
        [](auto& r) { r.transition_sequence.push_back("APS-TRANSITION-CW-00"); },
        [](auto& r) { r.topology_depth = 1; },
        [](auto& r) { r.naming_policy_id = "aps-windows-realm-names-1.0"; }}) {
        auto changed = request;
        edit(changed);
        const auto result = GenerationPlanner{}.generate_plan(changed, inputs);
        ASSERT_EQ(result.diagnostic->disposition, Disposition::RESOLVED);
        ASSERT_TRUE(result.plan.plan_hash != base.plan_hash);
    }
    for (const auto& edit : std::vector<std::function<void(GenerationPlanningInputs&)>>{
        [](auto& i) { i.target.target_class = "service"; },
        [](auto& i) { i.target.runtime_hint = "other-runtime"; },
        [](auto& i) { i.target.registry_version = "caller-revision-2"; },
        [](auto& i) { i.axiom_profile.identifier = "other-evidence-profile"; },
        [](auto& i) { i.axiom_subject->identifier = "other-measured-subject"; },
        [](auto& i) { i.axiom_subject->relation_count = 2; },
        [](auto& i) { i.axiom_subject->raw_measure = 2.0; },
        [](auto& i) { i.axiom_subject->compressed_measure = 0.25; },
        [](auto& i) { i.axiom_subject->scale_count = 3; },
        [](auto& i) { i.axiom_subject->erasure_cost = 2.0; },
        [](auto& i) { i.axiom_subject->self_reference_capable = false; },
        [](auto& i) { i.axiom_subject->self_model_available = false; },
        [](auto& i) { i.axiom_subject.reset(); }}) {
        auto changed = inputs;
        edit(changed);
        const auto result = GenerationPlanner{}.generate_plan(request, changed);
        ASSERT_EQ(result.diagnostic->disposition, Disposition::RESOLVED);
        ASSERT_TRUE(result.plan.plan_hash != base.plan_hash);
    }
}

TEST_CASE(generation_identity_has_unambiguous_length_prefixes_and_exact_double_bits) {
    auto left = measured_plan_inputs();
    auto right = left;
    left.target.runtime_hint = "a\nb";
    left.target.registry_version = "c";
    right.target.runtime_hint = "a";
    right.target.registry_version = "b\nc";
    const GenerationPlanner planner;
    const auto left_plan = planner.generate_plan(measured_plan_request(), left).plan;
    const auto right_plan = planner.generate_plan(measured_plan_request(), right).plan;
    ASSERT_TRUE(left_plan.plan_hash != right_plan.plan_hash);
    ASSERT_TRUE(generation_plan_identity_payload(left_plan).find("inputs.target.runtime_hint:3:a\nb\n") != std::string::npos);
    left = measured_plan_inputs();
    right = left;
    left.axiom_subject->compressed_measure = 0.0;
    right.axiom_subject->compressed_measure = -0.0;
    const auto positive = planner.generate_plan(measured_plan_request(), left).plan;
    const auto negative = planner.generate_plan(measured_plan_request(), right).plan;
    ASSERT_EQ(positive.axiom_diagnostic.diagnostic->notes, negative.axiom_diagnostic.diagnostic->notes);
    ASSERT_TRUE(positive.plan_hash != negative.plan_hash);
}

TEST_CASE(artifact_emitter_copies_all_declared_descriptor_fields) {
    const auto plan = GenerationPlanner{}.generate_plan(measured_plan_request(), measured_plan_inputs()).plan;
    const auto emitted = ArtifactEmitter{}.materialize_plan(plan, {"unused-descriptor-path", "desktop"});
    ASSERT_EQ(emitted.diagnostic->disposition, Disposition::RESOLVED);
    ASSERT_EQ(emitted.artifacts.size(), plan.artifacts.size());
    for (std::size_t i = 0; i < emitted.artifacts.size(); ++i) {
        ASSERT_EQ(emitted.artifacts[i].artifact_id, plan.artifacts[i].artifact_id);
        ASSERT_EQ(emitted.artifacts[i].name, plan.artifacts[i].name);
        ASSERT_EQ(emitted.artifacts[i].summary, plan.artifacts[i].summary);
        ASSERT_EQ(emitted.artifacts[i].kind, plan.artifacts[i].kind);
        ASSERT_EQ(emitted.artifacts[i].role_references, plan.artifacts[i].role_references);
    }
    ASSERT_TRUE(ArtifactEmitter{}.materialize_plan(plan, {"", "mobile"}).artifacts.empty());
}

TEST_CASE(artifact_emitter_rejects_tampering_in_every_plan_section_before_output) {
    for (const auto& edit : std::vector<std::function<void(GenerationPlan&)>>{
        [](auto& p) { p.request.project_name = "changed"; },
        [](auto& p) { p.inputs.target.runtime_hint = "changed"; },
        [](auto& p) { p.source_signature[0] = '1'; },
        [](auto& p) { p.source_realm_id = "APS-REALM-002"; },
        [](auto& p) { p.normalized_state = from_int(2); },
        [](auto& p) { p.transformed_state = from_int(2); },
        [](auto& p) { p.source_realm.orbit_id = "changed"; },
        [](auto& p) { p.destination_realm.b8 = !p.destination_realm.b8; },
        [](auto& p) { p.topology_nodes[4].parent_index = 2; },
        [](auto& p) { p.leaf_indices.pop_back(); },
        [](auto& p) { p.roles[0].role = "invented"; },
        [](auto& p) { p.role_assignments[0] = "changed"; },
        [](auto& p) { p.axiom_subject.relation_count = 0; },
        [](auto& p) { p.axiom_profile.identifier = "changed"; },
        [](auto& p) { p.axiom_diagnostic.a1_relational_existence = AxiomResultStatus::FAIL; },
        [](auto& p) { p.axiom_diagnostic.evaluations[0].diagnostic->notes[0] = "changed"; },
        [](auto& p) { p.artifacts[1].summary = "changed"; },
        [](auto& p) { p.artifact_descriptions.pop_back(); },
        [](auto& p) { p.warnings.push_back("invented warning"); },
        [](auto& p) { p.metadata["profile_registry_version"] = "changed"; },
        [](auto& p) { p.plan_hash[0] = p.plan_hash[0] == '0' ? '1' : '0'; }}) {
        auto plan = GenerationPlanner{}.generate_plan(measured_plan_request(), measured_plan_inputs()).plan;
        edit(plan);
        assert_emission_blocked(plan);
    }
}

TEST_CASE(artifact_emitter_rejects_rehashed_incomplete_or_inconsistent_plans) {
    for (const auto& edit : std::vector<std::function<void(GenerationPlan&)>>{
        [](auto& p) { p.topology_nodes.pop_back(); },
        [](auto& p) { p.roles.clear(); },
        [](auto& p) { p.artifacts[0].role_references.clear(); },
        [](auto& p) { p.axiom_diagnostic.diagnostic.reset(); },
        [](auto& p) { p.axiom_diagnostic.evaluations[0].diagnostic.reset(); },
        [](auto& p) { p.axiom_diagnostic.a1_relational_existence = AxiomResultStatus::FAIL; },
        [](auto& p) { p.artifacts.pop_back(); p.artifact_descriptions.pop_back(); },
        [](auto& p) { p.metadata.clear(); },
        [](auto& p) { p.destination_realm.realm_id = "APS-REALM-000"; },
        [](auto& p) { p.warnings.push_back("invented warning"); }}) {
        auto plan = GenerationPlanner{}.generate_plan(measured_plan_request(), measured_plan_inputs()).plan;
        edit(plan);
        plan.plan_hash = detail::sha256_hex(generation_plan_identity_payload(plan));
        assert_emission_blocked(plan);
    }
}

int main() {
    return ash::test::run_all();
}
