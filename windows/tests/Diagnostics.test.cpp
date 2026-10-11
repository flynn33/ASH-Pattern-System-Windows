// Diagnostics.test.cpp
//
// Tests for the Diagnostics cross-cutting module and the
// DiagnosticEnvelopeBuilder. Covers INV-DIAG-001..004.

#include "test_runner.hpp"

#include <stdexcept>
#include <array>
#include <cstdlib>
#include <limits>
#include <locale>
#include <new>
#include <type_traits>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef ERROR
#endif

#include "ash/Diagnostics.hpp"
#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"
#include "ash/Version.hpp"

using namespace ash;

// Fault only allocations on this test thread during explicit validator probes.
// This exercises noexcept failure behavior without changing production code.
thread_local std::ptrdiff_t diagnostic_test_fail_after = -1;
thread_local std::size_t diagnostic_test_allocations = 0;
void* operator new(std::size_t size) {
    ++diagnostic_test_allocations;
    if (diagnostic_test_fail_after >= 0 && diagnostic_test_fail_after-- == 0) {
        // Fail one requested allocation. Exception reporting itself must be
        // able to allocate, especially in the checked Microsoft Debug CRT.
        diagnostic_test_fail_after = -1;
        throw std::bad_alloc{};
    }
    if (void* storage = std::malloc(size ? size : 1)) return storage;
    throw std::bad_alloc{};
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* storage) noexcept { std::free(storage); }
void operator delete[](void* storage) noexcept { std::free(storage); }
void operator delete(void* storage, std::size_t) noexcept { std::free(storage); }
void operator delete[](void* storage, std::size_t) noexcept { std::free(storage); }

namespace {

// Small helper that tries to build an envelope and returns true if a
// std::logic_error was thrown by the builder.
bool build_root_throws_logic_error(DiagnosticEnvelopeBuilder builder) {
    try {
        (void)builder.build_root();
        return false;
    } catch (const std::logic_error&) {
        return true;
    }
}

DiagnosticEnvelopeBuilder valid_builder() {
    return DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::STATE_VALIDITY)
        .severity(Severity::INFO)
        .stage(Stage::DETECTION)
        .disposition(Disposition::RESOLVED)
        .subject("test")
        .rule_id("ASH-STATE-VALIDITY-001")
        .summary("test summary")
        .note("test note");
}

}  // namespace

TEST_CASE(builder_accepts_complete_envelope) {
    auto env = valid_builder().build_root();
    ASSERT_TRUE(env != nullptr);
    ASSERT_EQ(env->chain_root_reference.lock().get(), env.get());  // root observes self
    ASSERT_TRUE(env->parent_diagnostic_reference == nullptr);
}

TEST_CASE(builder_rejects_missing_kind) {
    auto b = DiagnosticEnvelopeBuilder{}
                 .severity(Severity::INFO)
                 .stage(Stage::DETECTION)
                 .disposition(Disposition::RESOLVED)
                 .subject("s")
                 .rule_id("ASH-STATE-VALIDITY-001")
                 .summary("s")
                 .note("n");
    ASSERT_TRUE(build_root_throws_logic_error(std::move(b)));
}

TEST_CASE(builder_rejects_missing_summary) {
    auto b = DiagnosticEnvelopeBuilder{}
                 .kind(DiagnosticKind::STATE_VALIDITY)
                 .severity(Severity::INFO)
                 .stage(Stage::DETECTION)
                 .disposition(Disposition::RESOLVED)
                 .subject("s")
                 .rule_id("ASH-STATE-VALIDITY-001")
                 .note("n");
    ASSERT_TRUE(build_root_throws_logic_error(std::move(b)));
}

TEST_CASE(builder_rejects_missing_notes) {
    auto b = DiagnosticEnvelopeBuilder{}
                 .kind(DiagnosticKind::STATE_VALIDITY)
                 .severity(Severity::INFO)
                 .stage(Stage::DETECTION)
                 .disposition(Disposition::RESOLVED)
                 .subject("s")
                 .rule_id("ASH-STATE-VALIDITY-001")
                 .summary("s");
    ASSERT_TRUE(build_root_throws_logic_error(std::move(b)));
}

TEST_CASE(builder_rejects_missing_rule_ids) {
    auto b = DiagnosticEnvelopeBuilder{}
                 .kind(DiagnosticKind::STATE_VALIDITY)
                 .severity(Severity::INFO)
                 .stage(Stage::DETECTION)
                 .disposition(Disposition::RESOLVED)
                 .subject("s")
                 .summary("s")
                 .note("n");
    ASSERT_TRUE(build_root_throws_logic_error(std::move(b)));
}

TEST_CASE(validate_rule_id_accepts_canonical_examples) {
    // INV-DIAG-002.
    Diagnostics d;
    ASSERT_TRUE(d.validate_rule_id("ASH-STATE-VALIDITY-001"));
    ASSERT_TRUE(d.validate_rule_id("ASH-CODEWORD-004"));
    ASSERT_TRUE(d.validate_rule_id("ASH-RECOVERY-005"));
    ASSERT_TRUE(d.validate_rule_id("ASH-HALT-001"));
    ASSERT_TRUE(d.validate_rule_id("ASH-GENERATION-001"));
    ASSERT_TRUE(d.validate_rule_id("ASH-EMISSION-001"));
}

TEST_CASE(validate_rule_id_rejects_lowercase) {
    Diagnostics d;
    ASSERT_FALSE(d.validate_rule_id("ash-state-validity-001"));
    ASSERT_FALSE(d.validate_rule_id("Ash-State-001"));
}

TEST_CASE(validate_rule_id_rejects_short_number) {
    Diagnostics d;
    ASSERT_FALSE(d.validate_rule_id("ASH-STATE-1"));
    ASSERT_FALSE(d.validate_rule_id("ASH-STATE-01"));
}

TEST_CASE(validate_rule_id_rejects_missing_family) {
    Diagnostics d;
    ASSERT_FALSE(d.validate_rule_id("STATE-001"));  // only one dash before number
    ASSERT_FALSE(d.validate_rule_id("-STATE-001"));
    ASSERT_FALSE(d.validate_rule_id(""));
}

TEST_CASE(validate_rule_id_matches_strict_canonical_record_taxonomy) {
    Diagnostics d;
    for (const auto family : {"INPUT", "STATE", "CODEWORD", "ORBIT", "REALM", "TRANSITION", "TOPOLOGY", "AXIOM", "CLASSIFICATION", "RECOVERY", "FALLBACK", "CONTAINMENT", "HALT", "GENERATION", "EMISSION", "DIAGNOSTIC", "CONFORMANCE", "RELEASE", "SECURITY", "ADMISSIBILITY"}) {
        ASSERT_TRUE(d.validate_rule_id(std::string("ASH-") + family + "-001"));
        ASSERT_TRUE(d.validate_rule_id(std::string("ASH-") + family + "-CATEGORY-TOKEN-001"));
    }
    for (const auto invalid : {"A-B-000", "ASH-UNKNOWN-001", "ASH-STATE--001", "ASH-STATE--CATEGORY-001", "ASH-STATE-CATEGORY2-001", "ASH-STATE-0000"})
        ASSERT_FALSE(d.validate_rule_id(invalid));
}

TEST_CASE(validate_envelope_accepts_well_formed) {
    // INV-DIAG-001.
    Diagnostics d;
    auto env = valid_builder().build_root();
    ASSERT_TRUE(d.validate_envelope(*env));
}

TEST_CASE(validate_envelope_rejects_empty_subject) {
    Diagnostics d;
    auto env = valid_builder().build_root();
    env->subject_reference.clear();
    ASSERT_FALSE(d.validate_envelope(*env));
}

TEST_CASE(validate_envelope_rejects_null_chain_root) {
    Diagnostics d;
    auto env = valid_builder().build_root();
    env->chain_root_reference.reset();
    ASSERT_FALSE(d.validate_envelope(*env));
}

TEST_CASE(validate_chain_root_is_self_reference) {
    // INV-DIAG-003.
    Diagnostics d;
    auto env = valid_builder().build_root();
    ASSERT_TRUE(d.validate_chain(*env));
}

TEST_CASE(validate_chain_accepts_parent_child_link) {
    Diagnostics d;
    auto root = DiagnosticEnvelopeBuilder{}
                    .kind(DiagnosticKind::STATE_VALIDITY)
                    .severity(Severity::INFO)
                    .stage(Stage::CLASSIFICATION)
                    .disposition(Disposition::RESOLVED)
                    .subject("root")
                    .rule_id("ASH-STATE-VALIDITY-001")
                    .summary("root")
                    .note("root")
                    .build_root();
    auto child = DiagnosticEnvelopeBuilder{}
                     .kind(DiagnosticKind::RECOVERY)
                     .severity(Severity::INFO)
                     .stage(Stage::ESCALATION)  // must follow CLASSIFICATION
                     .disposition(Disposition::ESCALATED)
                     .subject("child")
                     .parent(root)
                     .rule_id("ASH-RECOVERY-001")
                     .summary("child")
                     .note("child")
                     .build_child();
    ASSERT_TRUE(child != nullptr);
    ASSERT_EQ(child->chain_root_reference.lock().get(), root.get());
    ASSERT_TRUE(d.validate_chain(*child));
    ASSERT_TRUE(d.validate_chain(*root));
}

TEST_CASE(builder_rejects_escalation_child_of_detection_parent) {
    // Stage ordering: ESCALATION parent must be RECOVERY or CLASSIFICATION.
    auto root = valid_builder().build_root();  // stage DETECTION
    bool threw = false;
    try {
        (void)DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::RECOVERY)
            .severity(Severity::ERROR)
            .stage(Stage::ESCALATION)
            .disposition(Disposition::ESCALATED)
            .subject("bad")
            .parent(root)
            .rule_id("ASH-RECOVERY-001")
            .summary("bad")
            .note("bad")
            .build_child();
    } catch (const std::logic_error&) {
        threw = true;
    }
    ASSERT_TRUE(threw);
}

TEST_CASE(expose_chain_returns_ordered_root_to_leaf) {
    Diagnostics d;
    auto root = DiagnosticEnvelopeBuilder{}
                    .kind(DiagnosticKind::STATE_VALIDITY)
                    .severity(Severity::INFO)
                    .stage(Stage::CLASSIFICATION)
                    .disposition(Disposition::RESOLVED)
                    .subject("r")
                    .rule_id("ASH-STATE-VALIDITY-001")
                    .summary("r")
                    .note("r")
                    .build_root();
    auto leaf = DiagnosticEnvelopeBuilder{}
                    .kind(DiagnosticKind::RECOVERY)
                    .severity(Severity::INFO)
                    .stage(Stage::ESCALATION)
                    .disposition(Disposition::ESCALATED)
                    .subject("l")
                    .parent(root)
                    .rule_id("ASH-RECOVERY-001")
                    .summary("l")
                    .note("l")
                    .build_child();
    auto chain = d.expose_chain(leaf);
    ASSERT_EQ(chain.size(), static_cast<std::size_t>(2));
    ASSERT_EQ(chain.front().get(), root.get());
    ASSERT_EQ(chain.back().get(), leaf.get());
}

TEST_CASE(diagnostic_roots_release_and_children_retain_ancestry) {
    std::weak_ptr<DiagnosticEnvelope> root_observer;
    {
        auto root = valid_builder().build_root();
        root_observer = root;
    }
    ASSERT_TRUE(root_observer.expired());
    std::shared_ptr<DiagnosticEnvelope> child;
    {
        auto root = valid_builder().build_root();
        root_observer = root;
        child = valid_builder().stage(Stage::CLASSIFICATION).parent(root).build_child();
    }
    ASSERT_FALSE(root_observer.expired());
    ASSERT_TRUE(Diagnostics{}.validate_chain(*child));
    child.reset();
    ASSERT_TRUE(root_observer.expired());
}

TEST_CASE(diagnostics_reject_cycles_expired_roots_and_forged_chain_identity) {
    auto root = valid_builder().build_root();
    auto child = valid_builder().stage(Stage::CLASSIFICATION).parent(root).build_child();
    child->parent_diagnostic_id = "APS-DIAG-FORGED";
    ASSERT_FALSE(Diagnostics{}.validate_chain(*child));
    child->parent_diagnostic_id = root->diagnostic_id;
    root->parent_diagnostic_reference = child;
    ASSERT_FALSE(Diagnostics{}.validate_chain(*child));
    ASSERT_TRUE(Diagnostics{}.expose_chain(child).empty());
    root->parent_diagnostic_reference.reset();
    child->chain_root_reference.reset();
    ASSERT_FALSE(Diagnostics{}.validate_chain(*child));
    bool rejected = false;
    try { (void)valid_builder().stage(Stage::CLASSIFICATION).parent(child).build_child(); }
    catch (const std::logic_error&) { rejected = true; }
    ASSERT_TRUE(rejected);
}

TEST_CASE(diagnostics_support_complete_chains_beyond_1024_and_release_them) {
    std::weak_ptr<DiagnosticEnvelope> observer;
    {
        auto leaf = valid_builder().build_root();
        observer = leaf;
        for (std::size_t i = 0; i < 1100; ++i)
            leaf = valid_builder().subject("step-" + std::to_string(i)).parent(leaf).build_child();
        const auto exposed = Diagnostics{}.expose_chain(leaf);
        ASSERT_EQ(exposed.size(), std::size_t{1101});
        ASSERT_EQ(leaf->sequence_number, std::size_t{1101});
        ASSERT_EQ(exposed.front()->diagnostic_id, leaf->chain_root_diagnostic_id);
        ASSERT_TRUE(Diagnostics{}.validate_chain(*leaf));
    }
    ASSERT_TRUE(observer.expired());
}

TEST_CASE(diagnostics_reject_stage_severity_and_terminal_regressions) {
    auto root = valid_builder().stage(Stage::RECOVERY).severity(Severity::ERROR).build_root();
    for (const bool lower_stage : {true, false}) {
        bool rejected = false;
        try {
            auto child = valid_builder().parent(root).stage(lower_stage ? Stage::CLASSIFICATION : Stage::RECOVERY)
                .severity(lower_stage ? Severity::ERROR : Severity::INFO).build_child();
            (void)child;
        } catch (const std::logic_error&) { rejected = true; }
        ASSERT_TRUE(rejected);
    }
    auto halt = valid_builder().kind(DiagnosticKind::SAFE_HALT).stage(Stage::HALT).severity(Severity::CRITICAL)
        .disposition(Disposition::TERMINAL).build_root();
    bool rejected = false;
    try { (void)valid_builder().stage(Stage::VERIFY).severity(Severity::CRITICAL).parent(halt).build_child(); }
    catch (const std::logic_error&) { rejected = true; }
    ASSERT_TRUE(rejected);
    ASSERT_TRUE(build_root_throws_logic_error(valid_builder().kind(static_cast<DiagnosticKind>(255))));
    ASSERT_TRUE(build_root_throws_logic_error(valid_builder().rule_id("not-a-rule")));
}

TEST_CASE(diagnostics_reject_arbitrary_and_stale_semantic_identity) {
    auto root = valid_builder().build_root();
    root->diagnostic_id = "not-a-digest";
    root->chain_root_diagnostic_id = root->diagnostic_id;
    ASSERT_FALSE(Diagnostics{}.validate_envelope(*root));
    ASSERT_FALSE(Diagnostics{}.validate_chain(*root));
    root = valid_builder().build_root();
    root->summary = "Semantic payload changed without recomputing its identity";
    ASSERT_FALSE(Diagnostics{}.validate_envelope(*root));
    ASSERT_FALSE(Diagnostics{}.validate_chain(*root));
    ASSERT_TRUE(Diagnostics{}.expose_chain(root).empty());
}

TEST_CASE(diagnostics_identity_covers_every_semantic_field) {
    using Mutation = void (*)(DiagnosticEnvelope&);
    const std::array<Mutation, 9> mutations{
        [](DiagnosticEnvelope& d) { d.diagnostic_kind = DiagnosticKind::INPUT_VALIDATION; },
        [](DiagnosticEnvelope& d) { d.disposition = Disposition::RECORDED; },
        [](DiagnosticEnvelope& d) { d.rule_ids.push_back("ASH-STATE-VALIDITY-002"); },
        [](DiagnosticEnvelope& d) { d.severity = Severity::WARNING; },
        [](DiagnosticEnvelope& d) { d.stage = Stage::PARSE; },
        [](DiagnosticEnvelope& d) { d.subject_reference = "changed-subject"; },
        [](DiagnosticEnvelope& d) { d.summary = "Changed semantic condition"; },
        [](DiagnosticEnvelope& d) { d.notes.push_back("Additional semantic evidence"); },
        [](DiagnosticEnvelope& d) { d.details.emplace("state", 37); }
    };
    for (const auto mutate : mutations) {
        auto root = valid_builder().build_root();
        const auto original_id = root->diagnostic_id;
        mutate(*root);
        ASSERT_NE(Diagnostics{}.semantic_identity(*root), original_id);
        ASSERT_FALSE(Diagnostics{}.validate_envelope(*root));
        root->diagnostic_id = Diagnostics{}.semantic_identity(*root);
        root->chain_root_diagnostic_id = root->diagnostic_id;
        ASSERT_TRUE(Diagnostics{}.validate_chain(*root));
    }
}

TEST_CASE(diagnostics_occurrence_and_chain_position_do_not_change_identity) {
    const DiagnosticObject details{{"state", 37}, {"nested", DiagnosticObject{{"required", true}, {"absent", nullptr}}}};
    auto root = valid_builder().details(details).occurrence_metadata(DiagnosticObject{{"time", "2026-10-11T00:00:00Z"}, {"pid", 1}}).build_root();
    auto later = valid_builder().details(details).occurrence_metadata(DiagnosticObject{{"time", "2026-10-12T00:00:00Z"}, {"pid", 2}}).parent(root).build_child();
    ASSERT_EQ(root->diagnostic_id, later->diagnostic_id);
    ASSERT_EQ(later->sequence_number, std::size_t{2});
    ASSERT_TRUE(Diagnostics{}.validate_chain(*later));
    later->occurrence_metadata = DiagnosticObject{{"host", "different reviewed host"}};
    ASSERT_TRUE(Diagnostics{}.validate_envelope(*later));
    ASSERT_EQ(Diagnostics{}.semantic_identity(*later), root->diagnostic_id);
    later->sequence_number = 3;
    ASSERT_EQ(Diagnostics{}.semantic_identity(*later), root->diagnostic_id);
    ASSERT_FALSE(Diagnostics{}.validate_chain(*later));
    later->sequence_number = 2;
    later->schema_version = "unknown";
    ASSERT_EQ(Diagnostics{}.semantic_identity(*later), root->diagnostic_id);
    ASSERT_FALSE(Diagnostics{}.validate_envelope(*later));
}

TEST_CASE(diagnostics_canonical_payload_matches_independent_sha256_fixture) {
    auto root = valid_builder().build_root();
    ASSERT_EQ(Diagnostics{}.semantic_payload(*root),
        std::string("{\"details\":{},\"diagnostic_kind\":\"STATE_VALIDITY\",\"disposition\":\"RESOLVED\",\"notes\":[\"test note\"],\"rule_ids\":[\"ASH-STATE-VALIDITY-001\"],\"severity\":\"INFO\",\"stage\":\"DETECTION\",\"subject_reference\":\"test\",\"summary\":\"test summary\"}"));
    ASSERT_EQ(root->diagnostic_id, std::string("APS-DIAG-BD60E389D9A0C7F67D1F97BA"));
    auto modern = valid_builder().kind(DiagnosticKind::STATE_ASSESSMENT).build_root();
    ASSERT_NE(root->diagnostic_id, modern->diagnostic_id); // Legacy spelling is preserved.
}

TEST_CASE(diagnostics_nested_json_is_utf8_sorted_and_integer_exact) {
    static_assert(!std::is_constructible_v<DiagnosticValue, double>);
    DiagnosticObject facts{
        {"é", "UTF-8 scalar"},
        {"z", DiagnosticValue::Array{nullptr, true, false, 0, -1,
            (std::numeric_limits<std::int64_t>::min)(), (std::numeric_limits<std::int64_t>::max)(),
            DiagnosticObject{{"z", "\b\f\n\r\t\x01\"\\"}, {"a", "😀"}}}},
        {"a", "α"}
    };
    auto root = valid_builder().details(facts).build_root();
    const auto payload = Diagnostics{}.semantic_payload(*root);
    ASSERT_TRUE(payload.starts_with("{\"details\":{\"a\":\"α\",\"z\":[null,true,false,0,-1,-9223372036854775808,9223372036854775807,{\"a\":\"😀\",\"z\":\"\\b\\f\\n\\r\\t\\u0001\\\"\\\\\"}],\"é\":\"UTF-8 scalar\"}"));
    ASSERT_TRUE(Diagnostics{}.validate_chain(*root));
    bool overflow_rejected = false;
    try { (void)DiagnosticValue{(std::numeric_limits<std::uint64_t>::max)()}; }
    catch (const std::logic_error&) { overflow_rejected = true; }
    ASSERT_TRUE(overflow_rejected);
    std::printf("[JSON_RECORD] %s\n", Diagnostics{}.serialize_transport(*root, ASH_SDK_VERSION).c_str());
}

TEST_CASE(diagnostics_reject_invalid_utf8_at_every_json_boundary) {
    const std::array<std::string, 6> invalid{{"\xc0\x80", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x82", "\x80", "\xff"}};
    for (const auto& text : invalid) {
        ASSERT_TRUE(build_root_throws_logic_error(valid_builder().note(text)));
        ASSERT_TRUE(build_root_throws_logic_error(valid_builder().details({{text, "invalid key"}})));
        ASSERT_TRUE(build_root_throws_logic_error(valid_builder().details({{"invalid value", text}})));
        ASSERT_TRUE(build_root_throws_logic_error(valid_builder().occurrence_metadata(DiagnosticObject{{"invalid metadata", text}})));
        auto root = valid_builder().build_root();
        root->occurrence_metadata = DiagnosticObject{{"invalid", text}};
        ASSERT_FALSE(Diagnostics{}.validate_envelope(*root));
    }
    ASSERT_TRUE(build_root_throws_logic_error(valid_builder().note(" \t\r\n")));
    ASSERT_TRUE(build_root_throws_logic_error(valid_builder().note("\xc2\xa0\xe3\x80\x80")));
    ASSERT_TRUE(build_root_throws_logic_error(valid_builder().summary("two\nlines")));
}

TEST_CASE(diagnostics_vocabulary_acceptance_and_rejection_are_explicit) {
    for (unsigned i = 0; i <= static_cast<unsigned>(DiagnosticKind::CONFORMANCE); ++i)
        ASSERT_TRUE(Diagnostics{}.validate_chain(*valid_builder().kind(static_cast<DiagnosticKind>(i)).build_root()));
    for (unsigned i = 0; i <= static_cast<unsigned>(Stage::VERIFY); ++i)
        ASSERT_TRUE(Diagnostics{}.validate_chain(*valid_builder().stage(static_cast<Stage>(i)).build_root()));
    for (unsigned i = 0; i <= static_cast<unsigned>(Severity::CRITICAL); ++i)
        ASSERT_TRUE(Diagnostics{}.validate_chain(*valid_builder().severity(static_cast<Severity>(i)).build_root()));
    for (unsigned i = 0; i <= static_cast<unsigned>(Disposition::RECORDED); ++i)
        ASSERT_TRUE(Diagnostics{}.validate_chain(*valid_builder().disposition(static_cast<Disposition>(i)).build_root()));
    ASSERT_TRUE(build_root_throws_logic_error(valid_builder().kind(static_cast<DiagnosticKind>(255))));
    ASSERT_TRUE(build_root_throws_logic_error(valid_builder().stage(static_cast<Stage>(255))));
    ASSERT_TRUE(build_root_throws_logic_error(valid_builder().severity(static_cast<Severity>(255))));
    ASSERT_TRUE(build_root_throws_logic_error(valid_builder().disposition(static_cast<Disposition>(255))));
    auto invalid = valid_builder().build_root();
    invalid->stage = static_cast<Stage>(255);
    ASSERT_FALSE(Diagnostics{}.validate_envelope(*invalid));
    invalid = valid_builder().build_root();
    invalid->sequence_number = (std::numeric_limits<std::size_t>::max)();
    ASSERT_FALSE(Diagnostics{}.validate_envelope(*invalid));
}

TEST_CASE(diagnostics_validators_contain_allocation_failure) {
    auto root = valid_builder()
        .details({{"nested", DiagnosticValue::Array{true, nullptr, DiagnosticObject{{"value", 17}}}}})
        .occurrence_metadata(DiagnosticObject{{"timestamp", "2026-10-11T00:00:00Z"}}).build_root();
    auto child = valid_builder().details(root->details).occurrence_metadata(root->occurrence_metadata).parent(root).build_child();
    static_assert(noexcept(Diagnostics{}.validate_envelope(*root)));
    static_assert(noexcept(Diagnostics{}.validate_chain(*root)));
    for (const auto& record : {root, child}) for (const bool chain : {false, true}) {
        const auto before = diagnostic_test_allocations;
        ASSERT_TRUE(chain ? Diagnostics{}.validate_chain(*record) : Diagnostics{}.validate_envelope(*record));
        const auto allocations = diagnostic_test_allocations - before;
        ASSERT_TRUE(allocations > 0);
        for (std::size_t offset = 0; offset < allocations; ++offset) {
            diagnostic_test_fail_after = static_cast<std::ptrdiff_t>(offset);
            const bool accepted = chain ? Diagnostics{}.validate_chain(*record) : Diagnostics{}.validate_envelope(*record);
            const bool failed_allocation = diagnostic_test_fail_after == -1;
            diagnostic_test_fail_after = -1;
            ASSERT_TRUE(failed_allocation);
            if (accepted) throw ash::test::AssertionError(std::string(chain ? "chain" : "envelope") +
                " validator accepted after allocation failure at offset " + std::to_string(offset) +
                " of " + std::to_string(allocations));
        }
    }
    ASSERT_TRUE(Diagnostics{}.validate_chain(*root));
}

TEST_CASE(diagnostics_modern_safety_phases_and_legacy_aliases_are_causal) {
    auto root = valid_builder().kind(DiagnosticKind::INPUT_VALIDATION).stage(Stage::PARSE).build_root();
    auto assessed = valid_builder().kind(DiagnosticKind::STATE_ASSESSMENT).stage(Stage::ASSESS).parent(root).build_child();
    auto classified = valid_builder().kind(DiagnosticKind::CLASSIFICATION).stage(Stage::CLASSIFY).parent(assessed).build_child();
    auto recovered = valid_builder().kind(DiagnosticKind::RECOVERY).stage(Stage::RECOVER).parent(classified).build_child();
    auto fallback = valid_builder().kind(DiagnosticKind::FALLBACK).stage(Stage::FALLBACK).parent(recovered).build_child();
    auto contained = valid_builder().kind(DiagnosticKind::CONTAINMENT).stage(Stage::CONTAIN).disposition(Disposition::ESCALATED).parent(fallback).build_child();
    auto blocked = valid_builder().kind(DiagnosticKind::CONTAINMENT).stage(Stage::ESCALATION).disposition(Disposition::BLOCKED).parent(contained).build_child();
    auto halted = valid_builder().kind(DiagnosticKind::SAFE_HALT).stage(Stage::HALT).disposition(Disposition::TERMINAL).parent(blocked).build_child();
    ASSERT_TRUE(Diagnostics{}.validate_chain(*halted));
    ASSERT_EQ(Diagnostics{}.expose_chain(halted).size(), std::size_t{8});
    for (const auto& envelope : Diagnostics{}.expose_chain(halted))
        std::printf("[JSON_RECORD] %s\n", Diagnostics{}.serialize_transport(*envelope, ASH_SDK_VERSION).c_str());
    std::printf("[JSON_DIAGNOSTIC_CHAIN] %s\n", Diagnostics{}.serialize_chain(halted).c_str());
}

TEST_CASE(diagnostics_operations_inherit_phase_without_enum_ordinal_order) {
    auto root = valid_builder().stage(Stage::CLASSIFY).build_root();
    auto leaf = root;
    for (const auto operation : {Stage::VERIFY, Stage::PLAN, Stage::TRANSFORM, Stage::VALIDATE, Stage::EMIT, Stage::EVALUATE})
        leaf = valid_builder().stage(operation).parent(leaf).build_child();
    leaf = valid_builder().kind(DiagnosticKind::CONTAINMENT).stage(Stage::CONTAIN).parent(leaf).build_child();
    ASSERT_TRUE(Diagnostics{}.validate_chain(*leaf));
    bool regression_rejected = false;
    auto recovery = valid_builder().stage(Stage::RECOVER).build_root();
    auto planned = valid_builder().stage(Stage::PLAN).parent(recovery).build_child();
    try { (void)valid_builder().stage(Stage::ASSESS).parent(planned).build_child(); }
    catch (const std::logic_error&) { regression_rejected = true; }
    ASSERT_TRUE(regression_rejected);
    std::printf("[JSON_DIAGNOSTIC_CHAIN] %s\n", Diagnostics{}.serialize_chain(leaf).c_str());
}

TEST_CASE(diagnostics_containment_management_preserves_restricted_phase) {
    auto containment = valid_builder().kind(DiagnosticKind::CONTAINMENT).stage(Stage::CONTAIN).build_root();
    for (const auto disposition : {Disposition::BLOCKED, Disposition::RECORDED, Disposition::PENDING, Disposition::RESOLVED, Disposition::ESCALATED}) {
        auto managed = valid_builder().kind(DiagnosticKind::CONTAINMENT).stage(Stage::ESCALATION).disposition(disposition).parent(containment).build_child();
        ASSERT_TRUE(Diagnostics{}.validate_chain(*managed));
        for (const auto forbidden : {Stage::PLAN, Stage::RECOVER, Stage::CLASSIFY}) {
            bool rejected = false;
            try { (void)valid_builder().stage(forbidden).parent(managed).build_child(); }
            catch (const std::logic_error&) { rejected = true; }
            ASSERT_TRUE(rejected);
        }
        std::printf("[JSON_RECORD] %s\n", Diagnostics{}.serialize_transport(*managed, ASH_SDK_VERSION).c_str());
        std::printf("[JSON_DIAGNOSTIC_CHAIN] %s\n", Diagnostics{}.serialize_chain(managed).c_str());
    }
    bool wrong_kind_rejected = false;
    try { (void)valid_builder().kind(DiagnosticKind::RECOVERY).stage(Stage::CONTAIN).parent(containment).build_child(); }
    catch (const std::logic_error&) { wrong_kind_rejected = true; }
    ASSERT_TRUE(wrong_kind_rejected);
    auto final_management = valid_builder().kind(DiagnosticKind::CONTAINMENT).stage(Stage::CONTAIN)
        .disposition(Disposition::TERMINAL).parent(containment).build_child();
    ASSERT_TRUE(Diagnostics{}.validate_chain(*final_management));
    bool terminal_rejected = false;
    try { (void)valid_builder().kind(DiagnosticKind::CONTAINMENT).stage(Stage::CONTAIN).parent(final_management).build_child(); }
    catch (const std::logic_error&) { terminal_rejected = true; }
    ASSERT_TRUE(terminal_rejected);
}

TEST_CASE(diagnostics_terminal_stage_kind_or_disposition_freezes_chain) {
    const std::array<DiagnosticEnvelopeBuilder, 4> terminal_builders{
        valid_builder().stage(Stage::HALT), valid_builder().stage(Stage::TERMINAL),
        valid_builder().kind(DiagnosticKind::SAFE_HALT), valid_builder().disposition(Disposition::TERMINAL)
    };
    for (auto builder : terminal_builders) {
        auto terminal_root = builder.build_root();
        bool rejected = false;
        try { (void)valid_builder().stage(Stage::VERIFY).parent(terminal_root).build_child(); }
        catch (const std::logic_error&) { rejected = true; }
        ASSERT_TRUE(rejected);
        ASSERT_TRUE(Diagnostics{}.validate_chain(*terminal_root));
    }
}

TEST_CASE(diagnostics_chain_rejects_phase_skips_and_link_metadata_tampering) {
    auto detection = valid_builder().stage(Stage::PARSE).build_root();
    for (const auto skipped : {Stage::CONTAIN, Stage::HALT}) {
        bool rejected = false;
        try { (void)valid_builder().stage(skipped).parent(detection).build_child(); }
        catch (const std::logic_error&) { rejected = true; }
        ASSERT_TRUE(rejected);
    }
    auto root = valid_builder().build_root();
    auto child = valid_builder().parent(root).build_child();
    child->sequence_number = 0;
    ASSERT_FALSE(Diagnostics{}.validate_chain(*child));
    child->sequence_number = 2;
    child->chain_root_diagnostic_id = "APS-DIAG-000000000000000000000000";
    ASSERT_FALSE(Diagnostics{}.validate_chain(*child));
    child->chain_root_diagnostic_id = root->diagnostic_id;
    root->notes[0] = "Ancestral semantic evidence changed";
    ASSERT_FALSE(Diagnostics{}.validate_chain(*child));
}

TEST_CASE(diagnostics_serialize_complete_transport_without_native_pointers) {
    auto root = valid_builder().details({{"condition", "assessed"}}).build_root();
    const auto text = Diagnostics{}.serialize_record(*root);
    ASSERT_TRUE(text.find("\"occurrence_metadata\":null") != std::string::npos);
    ASSERT_TRUE(text.find("\"parent_diagnostic_id\":null") != std::string::npos);
    ASSERT_TRUE(text.find("\"sequence_number\":1") != std::string::npos);
    ASSERT_TRUE(text.find("parent_diagnostic_reference") == std::string::npos);
    ASSERT_TRUE(text.find("chain_root_reference") == std::string::npos);
    auto child = valid_builder().details({{"condition", "action recorded"}})
        .occurrence_metadata(DiagnosticObject{{"timestamp", "2026-10-11T00:00:00Z"}, {"nested", DiagnosticValue::Array{true, nullptr}}})
        .parent(root).build_child();
    ASSERT_TRUE(Diagnostics{}.serialize_record(*child).find("\"parent_diagnostic_id\":\"" + root->diagnostic_id + "\"") != std::string::npos);
    std::printf("[JSON_RECORD] %s\n", Diagnostics{}.serialize_transport(*root, ASH_SDK_VERSION).c_str());
    std::printf("[JSON_RECORD] %s\n", Diagnostics{}.serialize_transport(*child, ASH_SDK_VERSION).c_str());
    child->details["condition"] = "tampered after identity generation";
    bool rejected = false;
    try { (void)Diagnostics{}.serialize_record(*child); }
    catch (const std::logic_error&) { rejected = true; }
    ASSERT_TRUE(rejected);
}

namespace {
class GroupedHexDigits final : public std::numpunct<char> {
    std::string do_grouping() const override { return "\1"; }
    char do_thousands_sep() const override { return '_'; }
};
}
TEST_CASE(diagnostics_identity_is_independent_of_global_numeric_locale) {
    auto root = valid_builder().details({{"value", 123456789}}).build_root();
    const auto original = std::locale();
    struct RestoreLocale { std::locale value; ~RestoreLocale() { std::locale::global(value); } } restore{original};
    std::locale::global(std::locale(original, new GroupedHexDigits));
    ASSERT_EQ(Diagnostics{}.semantic_identity(*root), root->diagnostic_id);
    ASSERT_TRUE(Diagnostics{}.validate_chain(*root));
}

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
#if defined(_MSC_VER) && defined(_DEBUG)
    // Native unattended verification must report a CRT assertion as a failed
    // process, rather than wait indefinitely for a desktop assertion dialog.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    return ash::test::run_all();
}
