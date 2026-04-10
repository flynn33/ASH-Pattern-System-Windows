// ash/DiagnosticEnvelope.hpp
//
// Shared diagnostic envelope for the ASH Pattern System — Windows C++
// downstream implementation branch.
//
// Canonical source of truth:
//   specs/interfaces/diagnostic-schema.md on main
//
// Every diagnostic record produced by any module on this branch MUST
// be a DiagnosticEnvelope. The envelope carries:
//   - diagnostic_kind        (what kind of event)
//   - severity               (impact)
//   - stage                  (where in the pipeline)
//   - disposition            (how it was handled)
//   - subject_reference      (what the diagnostic is about)
//   - parent_diagnostic_reference  (NONE for root; required otherwise)
//   - chain_root_reference   (never NONE after construction)
//   - rule_ids               (>= 1 taxonomy-compliant rule IDs)
//   - summary                (non-empty)
//   - notes                  (>= 1 non-empty notes)
//
// Rule IDs must match the canonical rule-ID taxonomy
//   specs/interfaces/rule-id-taxonomy.md on main
// pattern: {FAMILY}-{CATEGORY}-{NUMBER} where NUMBER is a 3-digit
// zero-padded integer. Example: ASH-STATE-VALIDITY-001.
//
// Prohibited behaviors (from the canonical Diagnostics contract):
//   - MUST NOT emit envelopes with missing required fields
//   - MUST NOT emit envelopes with non-taxonomy-compliant rule IDs
//   - MUST NOT silently drop diagnostics from the chain
//   - MUST NOT defer or batch diagnostics in a way that breaks chain integrity
//   - MUST NOT invent local diagnostic structures outside canonical schema
//
// Builder discipline: this branch uses DiagnosticEnvelopeBuilder to
// construct envelopes. The builder enforces that all required fields
// are set before build() returns. A direct-construction path exists
// for tests that deliberately construct malformed envelopes to
// verify validator rejection.

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ash {

// Canonical diagnostic kind — specs/interfaces/diagnostic-schema.md
enum class DiagnosticKind {
    STATE_VALIDITY,
    RECOVERY,
    FALLBACK,
    CONTAINMENT,
    SAFE_HALT,
};

// Canonical severity — specs/interfaces/diagnostic-schema.md
enum class Severity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
};

// Canonical stage — monotonically ordered.
// Stage order constraint (from spec): ESCALATION parent must be
// RECOVERY or CLASSIFICATION; TERMINAL parent must be ESCALATION or
// RECOVERY.
enum class Stage {
    DETECTION,
    CLASSIFICATION,
    RECOVERY,
    ESCALATION,
    TERMINAL,
};

// Canonical disposition — how the diagnostic was handled.
enum class Disposition {
    RESOLVED,
    PENDING,
    BLOCKED,
    ESCALATED,
    TERMINAL,
};

// The canonical envelope. All fields are required; parent_diagnostic_reference
// may be null only for a root diagnostic.
struct DiagnosticEnvelope {
    DiagnosticKind diagnostic_kind{DiagnosticKind::STATE_VALIDITY};
    Severity severity{Severity::INFO};
    Stage stage{Stage::DETECTION};
    Disposition disposition{Disposition::PENDING};

    // A human-readable reference to the subject of the diagnostic
    // (for example, a state encoding as text, or an action name).
    // Canonical spec is open on the reference type; this branch uses
    // a short string for portability.
    std::string subject_reference;

    // Null for root diagnostics only.
    std::shared_ptr<DiagnosticEnvelope> parent_diagnostic_reference;

    // Never null after construction. For a root, this points to self.
    std::shared_ptr<DiagnosticEnvelope> chain_root_reference;

    // Canonical requirement: at least one rule ID, each matching the
    // rule-ID taxonomy pattern.
    std::vector<std::string> rule_ids;

    // Canonical requirement: non-empty.
    std::string summary;

    // Canonical requirement: at least one non-empty note.
    std::vector<std::string> notes;
};

// Fluent builder that enforces required fields. Throws
// std::logic_error on build() if any required field is missing or
// empty. Use this for all production envelope construction.
class DiagnosticEnvelopeBuilder {
public:
    DiagnosticEnvelopeBuilder() = default;

    DiagnosticEnvelopeBuilder& kind(DiagnosticKind k) noexcept;
    DiagnosticEnvelopeBuilder& severity(Severity s) noexcept;
    DiagnosticEnvelopeBuilder& stage(Stage s) noexcept;
    DiagnosticEnvelopeBuilder& disposition(Disposition d) noexcept;
    DiagnosticEnvelopeBuilder& subject(std::string_view s);
    DiagnosticEnvelopeBuilder& parent(std::shared_ptr<DiagnosticEnvelope> p) noexcept;
    DiagnosticEnvelopeBuilder& rule_id(std::string_view id);
    DiagnosticEnvelopeBuilder& summary(std::string_view s);
    DiagnosticEnvelopeBuilder& note(std::string_view n);

    // Build a root diagnostic. chain_root_reference is set to a
    // shared_ptr pointing at the returned envelope after construction
    // (we cannot set self-reference on a stack value, so this returns
    // a shared_ptr so the self-reference can be installed).
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope> build_root();

    // Build a child diagnostic. Requires a non-null parent to have
    // been set via parent(). chain_root_reference is copied from the
    // parent's chain_root_reference. Stage ordering is enforced.
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope> build_child();

private:
    void validate_required_fields_() const;

    DiagnosticKind kind_{DiagnosticKind::STATE_VALIDITY};
    bool kind_set_{false};
    Severity severity_{Severity::INFO};
    bool severity_set_{false};
    Stage stage_{Stage::DETECTION};
    bool stage_set_{false};
    Disposition disposition_{Disposition::PENDING};
    bool disposition_set_{false};
    std::string subject_;
    std::shared_ptr<DiagnosticEnvelope> parent_;
    std::vector<std::string> rule_ids_;
    std::string summary_;
    std::vector<std::string> notes_;
};

}  // namespace ash
