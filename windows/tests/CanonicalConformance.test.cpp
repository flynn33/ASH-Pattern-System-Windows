#include "test_runner.hpp"
#include "CanonicalFixtures.hpp"
#include "ash/ArtifactEmitter.hpp"
#include "ash/AxiomEvaluator.hpp"
#include "ash/Codeword.hpp"
#include "ash/Diagnostics.hpp"
#include "ash/GenerationPlanner.hpp"
#include "ash/RealmEncoder.hpp"
#include "ash/RecoveryEngine.hpp"
#include "ash/StateModel.hpp"
#include "ash/TopologyGenerator.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <set>
#include <string>
#include <string_view>

using namespace ash;
namespace cf = canonical_fixture;
namespace {
std::set<std::string> completed_cases;
Bit9State parse(std::string_view signature) {
    auto result = StateModel{}.normalize_input(signature);
    if (!result.state) throw ash::test::AssertionError("Expected fixture is not a well-formed state");
    return *result.state;
}

// Record coverage only after the registered native test returns successfully.
#define CONFORMANCE_CASE(name) \
    static void conformance_##name(); \
    static const ash::test::Register registration_##name{#name, [] { conformance_##name(); completed_cases.insert(#name); }}; \
    static void conformance_##name()
OperationalContext context_for(std::vector<Bit9State> stable, std::string policy = "FALLBACK-STATE-999") {
    OperationalContext context;
    context.context_id = "synthetic-native-conformance";
    context.context_version = "1.0";
    context.allowed_stable_states = std::move(stable);
    context.fallback_policy_instance.instance_id = "synthetic-native-policy-instance";
    context.fallback_policy_instance.policy_id = std::move(policy);
    context.propagation_risk_policy_defined = true;
    context.propagation_risk_permits_mutation = true;
    context.containment_policy_defined = true;
    context.halt_policy_defined = true;
    return context;
}
const char* class_name(SystemStateClass value) {
    constexpr const char* names[]{"STABLE", "UNSTABLE", "CORRECTABLE", "DEGRADED", "CONTAINED", "FAILED", "SAFE_HALT"};
    return names[static_cast<unsigned>(value)];
}
template<class A, class B> bool vector_equal(const char* id, const char* field, const A& actual, const B& expected) {
    if (actual == expected) return true;
    std::ostringstream message;
    message << "[VECTOR FAIL] " << id << " field=" << field << " expected=" << expected << " actual=" << actual;
    std::printf("%s\n", message.str().c_str());
    return false;
}
void evidence(const char* id, std::string_view actual, std::string_view expected) {
    std::printf("[VECTOR PASS] %s expected=%.*s actual=%.*s\n", id, static_cast<int>(expected.size()), expected.data(), static_cast<int>(actual.size()), actual.data());
}
template<class E, std::size_t N> E named(std::string_view value, const char* const (&names)[N]) {
    for (std::size_t i = 0; i < N; ++i) if (value == names[i]) return static_cast<E>(i);
    throw ash::test::AssertionError("Unknown enum name in pinned corpus");
}
}

CONFORMANCE_CASE(canonical_category_1_all_states_codewords_and_generator_closure) {
    ASSERT_EQ(std::size(cf::realms), std::size_t{512});
    ASSERT_EQ(std::size(cf::codewords), std::size_t{16});
    RealmEncoder realms;
    StateModel states;
    std::array<std::size_t, 32> orbit_counts{};
    for (const auto& row : cf::realms) {
        const auto state = from_int(row.index);
        const auto normalized = states.normalize(state);
        const auto encoded = realms.encode_realm_identity(state).identity;
        ASSERT_TRUE(vector_equal(row.id, "signature", encoded.state_signature, std::string(row.signature)));
        ASSERT_TRUE(vector_equal(row.id, "realm", encoded.realm_id, std::string(row.realm)));
        ASSERT_TRUE(vector_equal(row.id, "orbit", encoded.orbit_id, std::string(row.orbit)));
        ASSERT_TRUE(vector_equal(row.id, "index", encoded.realm_index, row.index));
        ASSERT_TRUE(vector_equal(row.id, "b8", encoded.b8, row.b8));
        ASSERT_EQ(normalized.state, state);
        ASSERT_EQ(states.normalize(state).state, normalized.state);
        ASSERT_EQ(states.classify_admissibility(state), AdmissibilityStatus::WELL_FORMED);
        ASSERT_TRUE(Diagnostics{}.validate_chain(*normalized.diagnostic));
        for (std::size_t i = 0; i < 9; ++i) ASSERT_EQ(state.test(i), row.signature[i] == '1');
        ++orbit_counts[static_cast<std::size_t>(std::stoi(encoded.orbit_id.substr(10)))];
        evidence(row.id, encoded.realm_id + ":" + encoded.orbit_id + ":" + encoded.state_signature,
            std::string(row.realm) + ":" + row.orbit + ":" + row.signature);
    }
    for (const auto count : orbit_counts) ASSERT_EQ(count, std::size_t{16});
    std::set<std::uint16_t> generated;
    for (std::size_t mask = 0; mask < 16; ++mask) {
        Bit9State state;
        for (std::size_t bit = 0; bit < 4; ++bit) if ((mask & (std::size_t{1} << bit)) != 0) state ^= kCodewordGenerators[bit];
        generated.insert(to_int(state));
    }
    ASSERT_EQ(generated.size(), std::size_t{16});
    std::size_t codeword_index = 0;
    for (const auto& row : cf::codewords) {
        ASSERT_EQ(row.rank, codeword_index++);
        const auto& codeword = kCanonicalCodewords[row.rank];
        ASSERT_TRUE(vector_equal(row.id, "signature", canonical_signature(codeword), std::string(row.signature)));
        ASSERT_TRUE(vector_equal(row.id, "weight", codeword.count(), row.weight));
        ASSERT_TRUE(vector_equal(row.id, "is_identity", codeword.none(), row.identity));
        ASSERT_TRUE(generated.contains(to_int(codeword)));
        ASSERT_EQ(codeword.count() % 4, std::size_t{0});
        ASSERT_FALSE(b8(codeword));
        ASSERT_EQ("APS-CW-" + std::string(row.rank < 10 ? "0" : "") + std::to_string(row.rank), std::string(row.codeword_id));
        for (std::size_t i = 0; i < 9; ++i) ASSERT_EQ(codeword.test(i), row.signature[i] == '1');
        for (const auto& other : kCanonicalCodewords) ASSERT_TRUE(is_canonical_codeword(codeword ^ other));
        evidence(row.id, canonical_signature(codeword), row.signature);
    }
}

CONFORMANCE_CASE(canonical_category_1_and_5_all_transforms_realms_and_reachability) {
    ASSERT_EQ(std::size(cf::transformations), std::size_t{8192});
    TransitionRegistry transitions;
    RealmEncoder encoder;
    std::array<RealmIdentity, 512> identities;
    for (std::uint16_t i = 0; i < 512; ++i) identities[i] = encoder.encode_realm_identity(from_int(i)).identity;
    for (const auto& row : cf::transformations) {
        auto resolved = transitions.resolve_transition(from_int(row.source), row.codeword);
        auto result = transitions.apply_transition(from_int(row.source), resolved.codeword);
        ASSERT_TRUE(vector_equal(row.id, "target", to_int(result.state), row.target));
        ASSERT_EQ(result.state, transitions.apply_transition(from_int(row.source), resolved.codeword).state);
        ASSERT_EQ(transitions.apply_transition(result.state, resolved.codeword).state, from_int(row.source));
        ASSERT_EQ(identities[row.source].realm_id, std::string(row.source_realm));
        ASSERT_EQ(identities[row.target].realm_id, std::string(row.target_realm));
        ASSERT_EQ(identities[row.source].orbit_id, std::string(row.source_orbit));
        ASSERT_EQ(identities[row.target].orbit_id, std::string(row.target_orbit));
        ASSERT_EQ(b8(from_int(row.source)), row.source_b8);
        ASSERT_EQ(b8(result.state), row.target_b8);
        ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
        evidence(row.id, canonical_signature(result.state), canonical_signature(from_int(row.target)));
    }
    for (const auto& row : cf::realm_identities) {
        const auto& actual = identities[row.index];
        ASSERT_EQ(actual.realm_id, std::string(row.realm));
        ASSERT_EQ(actual.realm_index, row.index);
        ASSERT_EQ(actual.state_signature, std::string(row.signature));
        evidence(row.id, actual.realm_id, row.realm);
    }
    for (const auto& row : cf::reachability) {
        std::size_t reachable = 0;
        for (std::uint16_t target = 0; target < 512; ++target) {
            const bool expected = std::find(std::begin(row.targets), std::end(row.targets), target) != std::end(row.targets);
            const bool actual = is_canonical_codeword(from_int(row.source) ^ from_int(target));
            ASSERT_EQ(actual, expected);
            if (actual) ++reachable;
        }
        ASSERT_EQ(reachable, row.reachable_count);
        ASSERT_EQ(512 - reachable, row.unreachable_count);
        evidence(row.id, std::to_string(reachable) + ":" + std::to_string(512 - reachable), std::to_string(row.reachable_count) + ":" + std::to_string(row.unreachable_count));
    }
}

CONFORMANCE_CASE(canonical_category_1_averaging_all_golden_indicators) {
    StateModel states;
    ASSERT_EQ(std::size(cf::averaging), std::size_t{545});
    for (const auto& row : cf::averaging) {
        std::array<double, 512> values{};
        std::string subject_orbit;
        if (std::string_view(row.function) == "constant_one") values.fill(1);
        else if (std::string_view(row.function) == "orbit_indicator") {
            subject_orbit = row.orbit;
            for (const auto& realm : cf::realms) values[realm.index] = std::string_view(realm.orbit) == row.orbit ? 1 : 0;
        } else {
            const auto subject = parse(row.state);
            values[to_int(subject)] = 1;
            subject_orbit = cf::realms[to_int(subject)].orbit;
        }
        const auto result = states.average(values);
        const auto repeated = states.average(result.values);
        ASSERT_EQ(result.diagnostic->disposition, Disposition::RESOLVED);
        ASSERT_EQ(repeated.values, result.values);
        if (row.idempotency_specified) ASSERT_EQ(repeated.values == result.values, row.idempotent);
        if (row.member_count != 0) {
            std::size_t count = 0;
            for (const auto& realm : cf::realms) if (subject_orbit == realm.orbit) ++count;
            ASSERT_EQ(count, row.member_count);
        }
        for (const auto& realm : cf::realms) {
            const double expected = subject_orbit.empty() || subject_orbit == realm.orbit ? row.inside : row.outside;
            ASSERT_TRUE(vector_equal(row.id, "averaged_value", result.values[realm.index], expected));
        }
        evidence(row.id, std::to_string(result.values[0]), std::to_string(subject_orbit.empty() || subject_orbit == cf::realms[0].orbit ? row.inside : row.outside));
    }
    std::array<double, 512> invalid{};
    invalid[0] = std::numeric_limits<double>::quiet_NaN();
    ASSERT_EQ(states.average(invalid).diagnostic->disposition, Disposition::BLOCKED);
}

CONFORMANCE_CASE(canonical_category_5_topology_all_node_fields) {
    TopologyGenerator generator;
    for (std::size_t depth = 0; depth <= 6; ++depth) {
        const auto actual = generator.generate_topology(from_int(1), depth);
        std::size_t expected_nodes = 0, expected_leaves = 0;
        for (const auto& row : cf::topology_nodes) if (row.request_depth == depth) {
            const auto& node = actual.topology.nodes[row.global];
            ASSERT_EQ(node.node_id, std::string(row.node_id));
            ASSERT_EQ(node.path, std::string(row.path));
            ASSERT_EQ(node.parent_path, std::string(row.parent));
            ASSERT_EQ(node.depth, row.depth);
            ASSERT_EQ(node.generation_ordinal, row.ordinal);
            ASSERT_EQ(node.global_ordinal, row.global);
            ASSERT_EQ(node.has_parent, row.depth > 0);
            ASSERT_EQ(node.is_leaf, row.leaf);
            if (node.has_parent) ASSERT_EQ(actual.topology.nodes[node.parent_index].path, node.parent_path);
            ++expected_nodes;
            if (row.leaf) {
                ASSERT_EQ(actual.topology.leaf_indices[expected_leaves], row.global);
                ++expected_leaves;
            }
        }
        ASSERT_EQ(actual.topology.nodes.size(), expected_nodes);
        ASSERT_EQ(actual.topology.leaf_indices.size(), expected_leaves);
        ASSERT_EQ(actual.topology.depth, depth);
        const auto repeated = generator.generate_topology(from_int(1), depth);
        ASSERT_EQ(repeated.topology.nodes.back().path, actual.topology.nodes.back().path);
        evidence(("APS-VEC-TOPOLOGY-" + std::to_string(depth)).c_str(), std::to_string(actual.topology.nodes.size()) + ":" + std::to_string(actual.topology.leaf_indices.size()), std::to_string(expected_nodes) + ":" + std::to_string(expected_leaves));
    }
    ASSERT_TRUE(generator.generate_topology(from_int(0), -1).topology.nodes.empty());
    ASSERT_EQ(generator.generate_topology(from_int(0), std::uint32_t{1}).topology.depth, std::size_t{1});
    const auto over = generator.generate_topology(from_int(0), std::size_t{7});
    ASSERT_EQ(over.diagnostic->disposition, Disposition::BLOCKED);
    ASSERT_TRUE(over.topology.nodes.empty());
    ASSERT_TRUE(over.diagnostic->notes.front().find("projected_nodes=3280") != std::string::npos);
}

CONFORMANCE_CASE(canonical_category_1_operational_assessment_all_golden_scenarios) {
    StateModel states;
    for (const auto& row : cf::assessments) {
        std::vector<Bit9State> stable;
        for (std::size_t i = 0; i < row.stable_count; ++i) stable.push_back(parse(row.stable[i]));
        auto context = context_for(std::move(stable));
        const auto result = states.assess_state(row.candidate, row.context_present ? &context : nullptr);
        ASSERT_TRUE(vector_equal(row.id, "blocked", result.blocked, row.blocked));
        if (*row.state_class) ASSERT_TRUE(vector_equal(row.id, "state_class", std::string(class_name(*result.state_class)), std::string(row.state_class)));
        if (*row.codeword_id) {
            ASSERT_TRUE(result.correction_codeword_id.has_value());
            const auto index = *result.correction_codeword_id;
            ASSERT_EQ("APS-CW-" + std::string(index < 10 ? "0" : "") + std::to_string(index), std::string(row.codeword_id));
        }
        if (row.blocked) ASSERT_FALSE(result.state_class.has_value());
        if (*row.input_status) ASSERT_EQ(result.state ? std::string("WELL_FORMED") : std::string("MALFORMED"), std::string(row.input_status));
        ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
        evidence(row.id, result.blocked ? "BLOCKED" : class_name(*result.state_class), row.blocked ? "BLOCKED" : row.state_class);
    }
}

CONFORMANCE_CASE(canonical_category_2_recovery_golden_scenarios) {
    StateModel states;
    TransitionRegistry transitions;
    for (const auto& row : cf::recovery) {
        if (*row.state_class) {
            ASSERT_EQ(states.classify_recoverability(SystemStateClass::FAILED), RecoveryCategory::EXTERNAL_ESCALATION_REQUIRED);
            evidence(row.id, "EXTERNAL_ESCALATION_REQUIRED", row.category);
        } else {
            auto context = context_for({parse(row.target)});
            auto assessment = states.assess_state(row.current, &context);
            RecoveryEngine engine(states, transitions);
            if (assessment.state_class == SystemStateClass::CORRECTABLE) {
                auto result = engine.execute_correction(assessment, context);
                ASSERT_EQ(result.outcome, RecoveryOutcome::RECOVERED);
                ASSERT_EQ(result.state, parse(row.target));
                ASSERT_EQ(canonical_signature(parse(row.current) ^ result.state), std::string(row.delta));
                ASSERT_EQ(std::string(row.category), std::string("APPLY_CORRECTION"));
            } else {
                ASSERT_EQ(assessment.state_class, std::optional<SystemStateClass>(SystemStateClass::DEGRADED));
                ASSERT_EQ(std::string(row.category), std::string("FALLBACK_REQUIRED"));
            }
            evidence(row.id, row.category, row.category);
        }
    }
}

CONFORMANCE_CASE(canonical_category_2_fallback_definition_scenarios) {
    StateModel states;
    TransitionRegistry transitions;
    for (const auto& row : cf::fallback) {
        auto context = context_for({from_int(0)}, row.policy_id);
        context.fallback_policy_instance.candidates.push_back({from_int(0), 1, 2, true, true, "synthetic-known-good-evidence"});
        auto assessment = states.assess_state(from_int(1), context);
        RecoveryEngine engine(states, transitions);
        const auto result = engine.select_fallback(assessment, context, context.fallback_policy_instance);
        const std::string actual = result.outcome == RecoveryOutcome::RECOVERED_VIA_FALLBACK_REPLACEMENT ? "RECOVERED_VIA_FALLBACK_REPLACEMENT" : "CONTAINMENT_ESCALATION";
        ASSERT_TRUE(vector_equal(row.id, "replacement_kind", actual, std::string(row.replacement_kind)));
        ASSERT_EQ(result.policy_id, std::string(row.policy_id));
        if (result.state) ASSERT_EQ(*result.state, from_int(0));
        else { ASSERT_TRUE(engine.is_contained()); ASSERT_TRUE(result.containment_diagnostic != nullptr); }
        evidence(row.id, actual, row.replacement_kind);
    }
}

CONFORMANCE_CASE(canonical_category_2_containment_literal_corpus_comparison) {
    StateModel states;
    TransitionRegistry transitions;
    for (const auto& row : cf::containment) {
        RecoveryEngine engine(states, transitions);
        std::shared_ptr<DiagnosticEnvelope> before;
        std::shared_ptr<DiagnosticEnvelope> result;
        if (std::string_view(row.initial_mode) == "DEGRADED") {
            ASSERT_EQ(states.assess_state(from_int(1), context_for({from_int(0)})).state_class, SystemStateClass::DEGRADED);
            before = engine.execute_normalize_state(from_int(1)).diagnostic;
            result = engine.enter_containment(row.risk);
        } else {
            ASSERT_EQ(std::string(row.initial_mode), std::string("CONTAINED"));
            before = engine.enter_containment("synthetic-preexisting-containment");
            ASSERT_TRUE(engine.is_contained());
            if (row.boundary_breached) {
                ASSERT_TRUE(row.propagation_observed);
                ASSERT_FALSE(row.attempt_prevented);
                result = engine.breach_containment(row.operation);
            } else {
                ASSERT_TRUE(row.attempt_prevented);
                ASSERT_FALSE(row.propagation_observed);
                const auto blocked = engine.execute_apply_correction(from_int(0), kCanonicalCodewords[1]);
                ASSERT_EQ(blocked.state, from_int(0));
                ASSERT_EQ(blocked.diagnostic->disposition, Disposition::BLOCKED);
                result = blocked.diagnostic;
            }
        }
        const std::string actual_mode = engine.is_halted() ? "SAFE_HALT" : (engine.is_contained() ? "CONTAINED" : "NORMAL");
        ASSERT_TRUE(vector_equal(row.id, "mode", actual_mode, std::string(row.mode)));
        ASSERT_TRUE(Diagnostics{}.validate_chain(*result));
        const auto before_chain = Diagnostics{}.expose_chain(before);
        const auto after_chain = Diagnostics{}.expose_chain(result);
        ASSERT_EQ(after_chain.size() - before_chain.size(), row.append_count);
        bool preserved = after_chain.size() >= before_chain.size();
        for (std::size_t index = 0; index < before_chain.size(); ++index)
            preserved = preserved && before_chain[index].get() == after_chain[index].get();
        ASSERT_EQ(preserved, row.preserved);
        ASSERT_EQ(engine.is_halted(), row.terminal);
        ASSERT_EQ(engine.is_halted(), row.frozen);
        ASSERT_EQ(row.boundary_breached && row.propagation_observed && !row.attempt_prevented, row.breach_confirmed);
        if (row.restricted) {
            auto blocked = engine.execute_apply_correction(from_int(0), kCanonicalCodewords[1]);
            ASSERT_TRUE(vector_equal(row.id, "restricted_operations_only", blocked.state == from_int(0) && blocked.diagnostic->disposition == Disposition::BLOCKED, row.restricted));
        }
        if (*row.operation) {
            const auto frozen = engine.diagnostic_chain();
            ASSERT_EQ(engine.execute_apply_correction(from_int(0), kCanonicalCodewords[1]).state, from_int(0));
            ASSERT_FALSE(row.transitions_allowed);
            if (row.frozen) ASSERT_EQ(engine.diagnostic_chain().get(), frozen.get());
            else {
                const auto retained = Diagnostics{}.expose_chain(engine.diagnostic_chain());
                ASSERT_TRUE(std::any_of(retained.begin(), retained.end(), [&](const auto& record) { return record.get() == frozen.get(); }));
            }
            if (row.terminal) {
                ASSERT_EQ(engine.enter_safe_halt("repeated").get(), frozen.get());
                ASSERT_EQ(engine.execute_normalize_state(from_int(1)).outcome, RecoveryOutcome::TERMINAL_NO_RECOVERY);
                ASSERT_EQ(engine.select_fallback(RecoveryCategory::FALLBACK_REQUIRED).outcome, RecoveryOutcome::TERMINAL_NO_RECOVERY);
                ASSERT_EQ(engine.diagnostic_chain().get(), frozen.get());
            }
        }
        evidence(row.id, actual_mode, row.mode);
    }
}

CONFORMANCE_CASE(canonical_category_2_safe_halt_golden_scenarios) {
    StateModel states;
    TransitionRegistry transitions;
    for (const auto& row : cf::halt) {
        RecoveryEngine engine(states, transitions);
        auto halt = engine.enter_safe_halt("synthetic-halt");
        ASSERT_EQ(std::string(row.state_class), std::string("SAFE_HALT"));
        ASSERT_EQ(engine.is_halted(), row.terminal);
        ASSERT_TRUE(Diagnostics{}.validate_chain(*halt));
        const auto before = Diagnostics{}.serialize_record(*halt);
        const auto chain_size = Diagnostics{}.expose_chain(halt).size();
        if (*row.transition) {
            ASSERT_EQ(std::string(row.transition), std::string("APS-TRANSITION-CW-01"));
            auto rejected = engine.execute_apply_correction(from_int(0), kCanonicalCodewords[1]);
            ASSERT_EQ(rejected.outcome, RecoveryOutcome::TERMINAL_NO_RECOVERY);
            ASSERT_EQ(rejected.state, from_int(0));
            ASSERT_EQ(std::string(row.failure), std::string("TERMINAL_NO_RECOVERY"));
            ASSERT_EQ(std::string(row.mode), engine.is_halted() ? std::string("SAFE_HALT") : std::string("NORMAL"));
            ASSERT_EQ(Diagnostics{}.expose_chain(engine.diagnostic_chain()).size() - chain_size, row.append_count);
            ASSERT_EQ(rejected.diagnostic.get() == halt.get(), row.preserved);
            ASSERT_EQ(Diagnostics{}.serialize_record(*engine.diagnostic_chain()) == before, row.frozen);
            ASSERT_FALSE(row.transitions_allowed);
            ASSERT_EQ(engine.execute_normalize_state(from_int(1)).diagnostic.get(), halt.get());
            ASSERT_EQ(engine.select_fallback(RecoveryCategory::FALLBACK_REQUIRED).diagnostic.get(), halt.get());
            ASSERT_EQ(engine.enter_safe_halt("repeated").get(), halt.get());
            ASSERT_EQ(Diagnostics{}.serialize_record(*engine.diagnostic_chain()), before);
            evidence(row.id, "TERMINAL_NO_RECOVERY", row.failure);
        } else {
            ASSERT_TRUE(row.terminal);
            ASSERT_FALSE(row.transitions_allowed);
            evidence(row.id, "terminal=true transitions_allowed=false", "terminal=true transitions_allowed=false");
        }
    }
}

CONFORMANCE_CASE(canonical_category_3_axiom_golden_scenarios) {
    for (const auto& row : cf::axioms) {
        const AxiomEvidence supplied = std::string_view(row.evidence) == "profile_threshold_met" ? AxiomEvidence::PRESENT_PASS :
            (std::string_view(row.evidence) == "profile_threshold_failed" ? AxiomEvidence::PRESENT_FAIL : AxiomEvidence::MISSING);
        const auto result = AxiomEvaluator{}.evaluate_axiom(from_int(0), {"synthetic-profile-axiom", supplied, true, "Controlled corpus fixture"});
        const std::string actual = result.status == AxiomResultStatus::PASS ? "PASS" : (result.status == AxiomResultStatus::FAIL ? "FAIL" : "INDETERMINATE");
        ASSERT_TRUE(vector_equal(row.id, "axiom_result", actual, std::string(row.result)));
        ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
        evidence(row.id, actual, row.result);
    }
}

namespace {
std::shared_ptr<DiagnosticEnvelope> execute_diagnostic_fixture(const cf::DiagnosticRow& row,
    std::shared_ptr<DiagnosticEnvelope> previous = {}) {
    DiagnosticEnvelopeBuilder builder;
    builder.kind(row.kind).severity(row.severity).stage(row.stage).disposition(row.disposition)
        .subject(row.subject).summary(row.summary).details(row.details).occurrence_metadata(row.occurrence);
    for (const auto& rule : row.rules) builder.rule_id(rule);
    for (const auto& note : row.notes) builder.note(note);
    auto result = row.child ? builder.parent(previous).build_child() : builder.build_root();
    const auto serialized = Diagnostics{}.serialize_record(*result);
    ASSERT_TRUE(vector_equal(row.id, "complete_diagnostic_record", serialized, std::string(row.expected_record)));
    ASSERT_TRUE(Diagnostics{}.validate_chain(*result));
    if (row.same_identity) {
        ASSERT_TRUE(previous != nullptr);
        ASSERT_EQ(result->diagnostic_id, previous->diagnostic_id);
    }
    evidence(row.id, serialized, row.expected_record);
    return result;
}
}

CONFORMANCE_CASE(canonical_category_3_diagnostic_literal_corpus_comparison) {
    for (const auto& row : cf::diagnostics) (void)execute_diagnostic_fixture(row);
}

CONFORMANCE_CASE(canonical_category_3_complete_diagnostic_chains_and_occurrence_identity) {
    std::shared_ptr<DiagnosticEnvelope> previous;
    for (const auto& row : cf::diagnostic_chains) previous = execute_diagnostic_fixture(row, previous);
}

CONFORMANCE_CASE(canonical_category_3_five_subject_axiom_predicates_are_executed) {
    AxiomSubject subject{"independent-native-subject", 1, 1.0, 2.0, 2, 1.0, true, true};
    AxiomEvaluator evaluator;
    const auto passing = evaluator.diagnose_axioms(subject);
    ASSERT_EQ(passing.overall_status, AxiomResultStatus::PASS);
    ASSERT_EQ(passing.overall_pass, std::optional<bool>{true});
    for (const auto& result : passing.evaluations) {
        ASSERT_EQ(result.status, AxiomResultStatus::PASS);
        ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
        ASSERT_FALSE(result.diagnostic->notes.empty());
    }
    for (int index = 0; index < 5; ++index) {
        auto failing = subject;
        if (index == 0) failing.relation_count = 0;
        if (index == 1) failing.compressed_measure = *failing.raw_measure;
        if (index == 2) failing.scale_count = 1;
        if (index == 3) failing.erasure_cost = 0;
        if (index == 4) failing.self_model_available = false;
        const auto result = evaluator.diagnose_axioms(failing);
        ASSERT_EQ(result.evaluations[index].status, AxiomResultStatus::FAIL);
        ASSERT_EQ(result.overall_status, AxiomResultStatus::FAIL);
        ASSERT_EQ(result.overall_pass, std::optional<bool>{false});
    }
    subject.erasure_cost.reset();
    const auto missing = evaluator.diagnose_axioms(subject);
    ASSERT_EQ(missing.overall_status, AxiomResultStatus::INDETERMINATE);
    ASSERT_FALSE(missing.overall_pass.has_value());
}

CONFORMANCE_CASE(canonical_category_4_plans_and_materialization_golden_scenarios) {
    GenerationPlanner planner;
    TargetMetadata target{"desktop", "win32"};
    for (const auto& row : cf::plans) {
        auto selected = target;
        if (*row.profile) selected.profile_id = row.profile;
        const auto source = *row.source ? parse(row.source) : from_int(0);
        auto first = planner.generate_plan(source, selected);
        if (*row.failure) {
            ASSERT_EQ(first.diagnostic->disposition, Disposition::BLOCKED);
            ASSERT_TRUE(first.diagnostic->summary.find(row.failure) != std::string::npos);
            evidence(row.id, row.failure, row.failure);
        } else if (row.registry_changed) {
            selected.registry_version = "aps-windows-1.0-synthetic-revision";
            auto changed = planner.generate_plan(source, selected);
            ASSERT_EQ(first.plan.plan_hash != changed.plan.plan_hash, row.hash_changes);
            evidence(row.id, "plan_hash_changes=true", "plan_hash_changes=true");
        } else {
            ASSERT_EQ(first.plan.plan_hash == planner.generate_plan(source, selected).plan.plan_hash, row.hash_stable);
            evidence(row.id, first.plan.plan_hash, planner.generate_plan(source, selected).plan.plan_hash);
        }
    }
    auto plan = planner.generate_plan(from_int(30), target).plan;
    ASSERT_EQ(generation_plan_identity_payload(plan), std::string(cf::default_plan_payload));
    ASSERT_EQ(plan.plan_hash, std::string(cf::default_plan_sha256));
    for (const auto& row : cf::emission) {
        const auto result = ArtifactEmitter{}.materialize_plan(row.complete ? plan : GenerationPlan{}, {"", "desktop"});
        if (row.complete) {
            ASSERT_EQ(result.artifacts.size(), plan.artifact_descriptions.size());
            for (std::size_t i = 0; i < result.artifacts.size(); ++i) {
                ASSERT_EQ(result.artifacts[i].plan_element_reference, "artifact_descriptions[" + std::to_string(i) + "]");
                ASSERT_EQ(result.artifacts[i].summary, plan.artifact_descriptions[i]);
            }
            ASSERT_TRUE(row.traceable);
            evidence(row.id, "every_output_has_plan_reference=true", "every_output_has_plan_reference=true");
        } else {
            ASSERT_TRUE(result.artifacts.empty());
            ASSERT_EQ(result.diagnostic->disposition, Disposition::BLOCKED);
            ASSERT_EQ(std::string(row.failure), std::string("EMISSION_REFUSED"));
            evidence(row.id, "EMISSION_REFUSED", row.failure);
        }
    }
    plan.artifact_descriptions[0] = "tampered-without-rehash";
    ASSERT_EQ(ArtifactEmitter{}.materialize_plan(plan, {"", "desktop"}).diagnostic->disposition, Disposition::BLOCKED);
}

CONFORMANCE_CASE(canonical_invariant_contracts_input_realm_recovery_and_diagnostics) {
    StateModel states;
    TransitionRegistry transitions;
    RealmEncoder encoder;
    for (const std::string_view malformed : {"", "00000000", "0000000000", "00000x000"}) {
        const auto result = states.normalize_input(malformed);
        ASSERT_FALSE(result.state.has_value());
        ASSERT_EQ(result.diagnostic->disposition, Disposition::BLOCKED);
        ASSERT_TRUE(Diagnostics{}.validate_chain(*result.diagnostic));
    }
    for (std::uint16_t i = 0; i < 512; ++i) {
        ASSERT_EQ(states.classify_admissibility(from_int(i)), states.classify_admissibility(from_int(i)));
        ASSERT_EQ(encoder.encode_realm_identity(from_int(i)).identity.realm_id, encoder.encode_realm_identity(from_int(i)).identity.realm_id);
    }
    const RecoveryCategory expected[]{RecoveryCategory::NO_ACTION, RecoveryCategory::TARGET_RESOLUTION_REQUIRED, RecoveryCategory::APPLY_CORRECTION,
        RecoveryCategory::FALLBACK_REQUIRED, RecoveryCategory::CONTAINMENT_ACTIVE, RecoveryCategory::EXTERNAL_ESCALATION_REQUIRED, RecoveryCategory::TERMINAL_NO_RECOVERY};
    for (unsigned i = 0; i < 7; ++i) {
        const auto state_class = static_cast<SystemStateClass>(i);
        ASSERT_EQ(states.classify_recoverability(state_class), expected[i]);
        ASSERT_EQ(states.classify_recoverability(state_class), states.classify_recoverability(state_class));
    }
    const auto invalid = transitions.apply_transition(from_int(0), from_int(1));
    ASSERT_EQ(invalid.state, from_int(0));
    ASSERT_EQ(invalid.diagnostic->disposition, Disposition::BLOCKED);
    RecoveryEngine engine(states, transitions);
    auto correction = engine.execute_apply_correction(from_int(0), from_int(1));
    ASSERT_EQ(correction.state, from_int(0));
    ASSERT_EQ(correction.outcome, RecoveryOutcome::RECOVERY_FAILED);
    auto fallback = engine.select_fallback(RecoveryCategory::FALLBACK_REQUIRED);
    ASSERT_TRUE(fallback.containment_diagnostic != nullptr);
    ASSERT_EQ(fallback.diagnostic->parent_diagnostic_reference.get(), correction.diagnostic.get());
    auto halt = engine.enter_safe_halt("mandatory-invariant-sequence");
    const auto chain = Diagnostics{}.expose_chain(halt);
    ASSERT_FALSE(chain.empty());
    for (std::size_t i = 0; i < chain.size(); ++i) {
        ASSERT_TRUE(Diagnostics{}.validate_envelope(*chain[i]));
        for (const auto& rule : chain[i]->rule_ids) ASSERT_TRUE(Diagnostics{}.validate_rule_id(rule));
        ASSERT_FALSE(chain[i]->notes.empty());
        if (i) ASSERT_TRUE(static_cast<unsigned>(chain[i]->severity) >= static_cast<unsigned>(chain[i - 1]->severity));
    }
    ASSERT_EQ(engine.execute_apply_correction(from_int(0), kCanonicalCodewords[1]).state, from_int(0));
    ASSERT_EQ(engine.diagnostic_chain().get(), halt.get());
    std::weak_ptr<DiagnosticEnvelope> root_observer;
    { auto normalized = states.normalize(from_int(0)); root_observer = normalized.diagnostic; }
    ASSERT_TRUE(root_observer.expired());
}

CONFORMANCE_CASE(canonical_invariant_planner_has_no_filesystem_effect_and_plan_is_standalone) {
    const auto snapshot = [] {
        std::vector<std::string> values;
        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
            if (entry.path().extension() == ".log") continue; // The external runner writes its capture concurrently.
            values.push_back(entry.path().filename().string() + ":" + (entry.is_regular_file() ? std::to_string(entry.file_size()) : "directory"));
        }
        std::sort(values.begin(), values.end());
        return values;
    };
    const auto before = snapshot();
    GenerationPlan plan;
    { GenerationPlanner planner; plan = planner.generate_plan(from_int(0), {"desktop", "win32"}).plan; }
    ASSERT_EQ(snapshot(), before);
    const auto emitted = ArtifactEmitter{}.materialize_plan(plan, {"", "desktop"});
    ASSERT_EQ(emitted.artifacts.size(), plan.artifact_descriptions.size());
    ASSERT_EQ(snapshot(), before);
    for (std::size_t i = 0; i < emitted.artifacts.size(); ++i) {
        ASSERT_EQ(emitted.artifacts[i].summary, plan.artifact_descriptions[i]);
        ASSERT_EQ(emitted.artifacts[i].kind, std::string("descriptor"));
        ASSERT_EQ(emitted.artifacts[i].plan_element_reference, "artifact_descriptions[" + std::to_string(i) + "]");
    }
}

int main() {
    const int failures = ash::test::run_all();
    struct Coverage { const char* invariant; const char* test; unsigned category; };
    const Coverage mapping[]{
        {"INV-STATE-001", "canonical_category_1_all_states_codewords_and_generator_closure", 1},
        {"INV-STATE-002", "canonical_category_1_all_states_codewords_and_generator_closure", 1},
        {"INV-STATE-003", "canonical_invariant_contracts_input_realm_recovery_and_diagnostics", 1},
        {"INV-CODEWORD-001", "canonical_category_1_and_5_all_transforms_realms_and_reachability", 1},
        {"INV-CODEWORD-002", "canonical_category_1_and_5_all_transforms_realms_and_reachability", 1},
        {"INV-CODEWORD-003", "canonical_category_1_and_5_all_transforms_realms_and_reachability", 1},
        {"INV-CODEWORD-004", "canonical_category_1_all_states_codewords_and_generator_closure", 1},
        {"INV-ADMISSIBILITY-001", "canonical_invariant_contracts_input_realm_recovery_and_diagnostics", 1},
        {"INV-ADMISSIBILITY-002", "canonical_category_1_all_states_codewords_and_generator_closure", 1},
        {"INV-REALM-001", "canonical_invariant_contracts_input_realm_recovery_and_diagnostics", 5},
        {"INV-REALM-002", "canonical_invariant_contracts_input_realm_recovery_and_diagnostics", 5},
        {"INV-TRANS-001", "canonical_category_1_and_5_all_transforms_realms_and_reachability", 5},
        {"INV-TRANS-002", "canonical_category_1_and_5_all_transforms_realms_and_reachability", 5},
        {"INV-TOPO-001", "canonical_category_5_topology_all_node_fields", 5},
        {"INV-AXIOM-001", "canonical_category_3_five_subject_axiom_predicates_are_executed", 3},
        {"INV-RECOVERY-001", "canonical_invariant_contracts_input_realm_recovery_and_diagnostics", 2},
        {"INV-RECOVERY-002", "canonical_category_2_fallback_definition_scenarios", 2},
        {"INV-RECOVERY-003", "canonical_invariant_contracts_input_realm_recovery_and_diagnostics", 2},
        {"INV-RECOVERY-004", "canonical_invariant_contracts_input_realm_recovery_and_diagnostics", 2},
        {"INV-RECOVERY-005", "canonical_category_2_safe_halt_golden_scenarios", 2},
        {"INV-PLAN-001", "canonical_invariant_planner_has_no_filesystem_effect_and_plan_is_standalone", 4},
        {"INV-PLAN-002", "canonical_invariant_planner_has_no_filesystem_effect_and_plan_is_standalone", 4},
        {"INV-BOUNDARY-001", "canonical_invariant_planner_has_no_filesystem_effect_and_plan_is_standalone", 4},
        {"INV-BOUNDARY-002", "canonical_category_4_plans_and_materialization_golden_scenarios", 4},
        {"INV-DIAG-001", "canonical_category_3_diagnostic_literal_corpus_comparison", 3},
        {"INV-DIAG-002", "canonical_invariant_contracts_input_realm_recovery_and_diagnostics", 3},
        {"INV-DIAG-003", "canonical_invariant_contracts_input_realm_recovery_and_diagnostics", 3},
        {"INV-DIAG-004", "canonical_invariant_contracts_input_realm_recovery_and_diagnostics", 3},
    };
    std::array<bool, 5> categories{true, true, true, true, true};
    for (const auto& item : mapping) {
        const bool passed = completed_cases.contains(item.test);
        std::printf("[COVERAGE] invariant=%s category=%u test=%s status=%s\n", item.invariant, item.category, item.test, passed ? "passed" : "failed");
        categories[item.category - 1] = categories[item.category - 1] && passed;
    }
    categories[1] = categories[1] && completed_cases.contains("canonical_category_2_containment_literal_corpus_comparison");
    categories[2] = categories[2] && completed_cases.contains("canonical_category_3_complete_diagnostic_chains_and_occurrence_identity")
        && completed_cases.contains("canonical_category_3_axiom_golden_scenarios");
    for (unsigned i = 0; i < 5; ++i) std::printf("[CATEGORY] category=%u status=%s\n", i + 1, categories[i] ? "passed" : "failed");
    std::printf("[COVERAGE] invariants=%zu categories=%zu scope=native_assertions authority_commit=%s judgment=%s\n", std::size(mapping), categories.size(), cf::source_commit, failures == 0 ? "TESTS_PASSED" : "NON-CONFORMANT");
    return failures;
}
