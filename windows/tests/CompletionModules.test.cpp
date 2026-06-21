// CompletionModules.test.cpp
//
// Completion coverage for Windows semantic modules and canonical realm
// encoding helpers.

#include "test_runner.hpp"

#include <array>
#include <string>

#include "ash/ArtifactEmitter.hpp"
#include "ash/AxiomEvaluator.hpp"
#include "ash/Diagnostics.hpp"
#include "ash/GenerationPlanner.hpp"
#include "ash/RealmEncoder.hpp"
#include "ash/State.hpp"
#include "ash/TopologyGenerator.hpp"

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

int main() {
    return ash::test::run_all();
}
