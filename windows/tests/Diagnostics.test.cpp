// Diagnostics.test.cpp
//
// Tests for the Diagnostics cross-cutting module and the
// DiagnosticEnvelopeBuilder. Covers INV-DIAG-001..004.

#include "test_runner.hpp"

#include <stdexcept>

#include "ash/Diagnostics.hpp"
#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"

using namespace ash;

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
    ASSERT_EQ(env->chain_root_reference.get(), env.get());  // root references self
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
    ASSERT_TRUE(d.validate_rule_id("ASH-WINDOWS-STUB-001"));
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
    ASSERT_EQ(child->chain_root_reference.get(), root.get());
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

int main() {
    return ash::test::run_all();
}
