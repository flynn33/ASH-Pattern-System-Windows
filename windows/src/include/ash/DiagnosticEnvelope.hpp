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
//   - details                (required semantic JSON object)
//   - occurrence_metadata    (required occurrence object or null)
//
// Rule IDs must match the canonical rule-ID taxonomy
//   specs/interfaces/rule-id-taxonomy.md on main
// and the canonical JSON schema's ASH family allowlist. NUMBER is a
// 3-digit zero-padded integer. Example: ASH-STATE-VALIDITY-001.
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
#include <concepts>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ash {

// Canonical diagnostic kind — specs/interfaces/diagnostic-schema.md
enum class DiagnosticKind {
    STATE_VALIDITY,
    TRANSITION,
    TOPOLOGY,
    AXIOM,
    GENERATION_PLAN,
    EMISSION,
    RECOVERY,
    FALLBACK,
    CONTAINMENT,
    SAFE_HALT,
    INPUT_VALIDATION,
    STATE_ASSESSMENT,
    CLASSIFICATION,
    SCHEMA_VALIDATION,
    CONFORMANCE,
};

// Canonical severity — specs/interfaces/diagnostic-schema.md
enum class Severity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
};

// Legacy and modern stage spellings retain their numeric values for source
// compatibility. Numeric enum order is not causal order. See Diagnostics.
enum class Stage {
    DETECTION,
    CLASSIFICATION,
    RECOVERY,
    ESCALATION,
    TERMINAL,
    PARSE,
    ASSESS,
    CLASSIFY,
    TRANSFORM,
    PLAN,
    EVALUATE,
    EMIT,
    RECOVER,
    FALLBACK,
    CONTAIN,
    HALT,
    VALIDATE,
    VERIFY,
};

// Canonical disposition — how the diagnostic was handled.
enum class Disposition {
    RESOLVED,
    PENDING,
    BLOCKED,
    ESCALATED,
    TERMINAL,
    RECORDED,
};

// Canonical JSON content uses signed 64-bit integers, with no floating point
// values. Objects are sorted by UTF-8 key; strings must contain valid UTF-8.
// Arrays preserve their order. Owning recursive values require no dependency.
struct DiagnosticValue {
    using Array = std::vector<DiagnosticValue>;
    using Object = std::map<std::string, DiagnosticValue, std::less<>>;
    using Storage = std::variant<std::nullptr_t, bool, std::int64_t,
                                 std::string, Array, Object>;
    Storage value{nullptr};

    DiagnosticValue() = default;
    DiagnosticValue(std::nullptr_t) noexcept : value(nullptr) {}
    DiagnosticValue(bool v) noexcept : value(v) {}
    template <std::integral T> requires (!std::same_as<T, bool>)
    DiagnosticValue(T v) {
        if constexpr (std::is_unsigned_v<T>) {
            if (v > static_cast<std::make_unsigned_t<std::int64_t>>((std::numeric_limits<std::int64_t>::max)()))
                throw std::logic_error("DiagnosticValue: integer exceeds signed 64-bit range");
        }
        value = static_cast<std::int64_t>(v);
    }
    template <std::floating_point T> DiagnosticValue(T) = delete;
    DiagnosticValue(std::string v) : value(std::move(v)) {}
    DiagnosticValue(std::string_view v) : value(std::string(v)) {}
    DiagnosticValue(const char* v) : value(v ? std::string(v) : throw std::logic_error("DiagnosticValue: null string")) {}
    DiagnosticValue(Array v) : value(std::move(v)) {}
    DiagnosticValue(Object v) : value(std::move(v)) {}
    bool operator==(const DiagnosticValue&) const = default;
};
using DiagnosticObject = DiagnosticValue::Object;

// The canonical inner record plus native ownership links. Serialization uses
// nullable parent_diagnostic_id and the root ID, never native pointer values.
struct DiagnosticEnvelope {
    std::string schema_version{"1.0"};
    std::string diagnostic_id;
    std::string parent_diagnostic_id;
    std::string chain_root_diagnostic_id;
    std::size_t sequence_number{1};
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

    // Non-owning identity. A root observes itself; a child owns its ancestry
    // through parent_diagnostic_reference. Use lock() and check expiration.
    std::weak_ptr<DiagnosticEnvelope> chain_root_reference;

    // Canonical requirement: at least one rule ID, each matching the
    // rule-ID taxonomy pattern.
    std::vector<std::string> rule_ids;

    // Canonical requirement: non-empty.
    std::string summary;

    // Canonical requirement: at least one non-empty note.
    std::vector<std::string> notes;

    // Required semantic object; an empty object expresses no extra details.
    DiagnosticObject details;

    // Per-occurrence object or null. Excluded from semantic identity along
    // with schema version, parent/root IDs, and sequence number. Adding these
    // fields changes record ABI: applications must rebuild with this SDK.
    std::optional<DiagnosticObject> occurrence_metadata;
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
    DiagnosticEnvelopeBuilder& details(DiagnosticObject value);
    DiagnosticEnvelopeBuilder& occurrence_metadata(std::optional<DiagnosticObject> value);

    // Build a root diagnostic. chain_root_reference is set to a
    // weak_ptr pointing at the returned envelope after construction
    // (we cannot set self-reference on a stack value, so this returns
    // a shared_ptr so the self-reference can be installed).
    [[nodiscard]] std::shared_ptr<DiagnosticEnvelope> build_root();

    // Build a child diagnostic. Requires a complete valid parent chain.
    // Copies its root observer and enforces explicit causal phases, severity,
    // parent/sequence integrity, containment restrictions, and terminal freeze.
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
    DiagnosticObject details_;
    std::optional<DiagnosticObject> occurrence_metadata_;
};

}  // namespace ash
