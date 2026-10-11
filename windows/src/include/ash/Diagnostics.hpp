// ash/Diagnostics.hpp
//
// Diagnostics — canonical cross-cutting module.
//
// Canonical source of truth:
//   specs/interfaces/contracts/diagnostics-module-contract.md on main
//   specs/interfaces/diagnostic-schema.md on main
//   specs/interfaces/rule-id-taxonomy.md on main
//
// Canonical purpose (paraphrased):
//   Cross-cutting module ensuring all diagnostic records conform to
//   the shared envelope schema, rule-ID taxonomy, and chain integrity.
//
// Rule IDs use ASH plus a family from the canonical record schema, optional
// uppercase category tokens, and an exactly three-digit number.
//
// Canonical invariants:
//   - INV-DIAG-001: Every diagnostic conforms to shared envelope schema
//   - INV-DIAG-002: Every rule_id conforms to taxonomy pattern
//   - INV-DIAG-003: Diagnostic chain is complete and reconstructable
//   - INV-DIAG-004: No silent omission
//
// Causal phases: PARSE/DETECTION=0; ASSESS/CLASSIFY/CLASSIFICATION=1;
// RECOVER/FALLBACK/RECOVERY=2; CONTAIN/ESCALATION=3; HALT/TERMINAL=4.
// Operation stages inherit their preceding causal phase. Phase 3 entry
// requires phase 1 or 2; phase 4 entry requires phase 2 or 3. Containment
// permits containment management; a terminal disposition then freezes it.
//
// Prohibited behaviors (canonical):
//   - MUST NOT emit envelopes with missing required fields
//   - MUST NOT emit envelopes with non-taxonomy-compliant rule IDs
//   - MUST NOT silently drop diagnostics from chain
//   - MUST NOT defer or batch diagnostics in a way that breaks chain integrity
//   - MUST NOT invent local diagnostic structures outside canonical schema

#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "DiagnosticEnvelope.hpp"

namespace ash {

struct ContainmentDiagnostic;
struct SafeHaltDiagnostic;

class Diagnostics {
public:
    Diagnostics() = default;

    // Returns true iff the envelope has all required fields populated
    // and non-empty: valid diagnostic_kind/severity/stage/disposition,
    // subject_reference, live chain_root_reference,
    // rule_ids non-empty and each taxonomy-compliant, summary
    // non-empty, meaningful notes, JSON-compatible details/metadata, and an
    // exact recomputed semantic diagnostic identity. Allocation failure
    // returns false; no exception escapes either validation function.
    [[nodiscard]] bool validate_envelope(const DiagnosticEnvelope& d) const noexcept;

    // Returns true iff the rule ID matches the canonical taxonomy
    // pattern. Manual parse, no stdlib regex, for compile-time and
    // runtime speed.
    [[nodiscard]] bool validate_rule_id(std::string_view id) const noexcept;

    // Walks parent_diagnostic_reference back to the root. Verifies
    // each link's chain_root_reference matches the root. Verifies
    // stage ordering at parent/child boundaries. Returns true iff
    // the chain is complete and valid.
    [[nodiscard]] bool validate_chain(const DiagnosticEnvelope& d) const noexcept;

    // Compact UTF-8, sorted-key canonical JSON. The semantic payload has
    // exactly nine keys, including notes and details. Identity is APS-DIAG-
    // plus the first 24 uppercase hexadecimal SHA-256 characters. These
    // functions throw std::logic_error for invalid semantic content and may
    // propagate allocation failure. serialize_record requires a valid chain
    // and emits the complete inner diagnostic record, including null parent
    // and occurrence metadata. It never serializes native owning pointers.
    [[nodiscard]] std::string semantic_payload(const DiagnosticEnvelope& d) const;
    [[nodiscard]] std::string semantic_identity(const DiagnosticEnvelope& d) const;
    [[nodiscard]] std::string serialize_record(const DiagnosticEnvelope& d) const;
    // Schema transport wrapper: {data, id, product_version, schema_version}.
    // Product version comes from the caller's actual build/release metadata.
    [[nodiscard]] std::string serialize_transport(const DiagnosticEnvelope& d,
                                                  std::string_view product_version) const;
    [[nodiscard]] std::string serialize_transport(const ContainmentDiagnostic& d,
                                                  std::string_view product_version) const;
    [[nodiscard]] std::string serialize_transport(const SafeHaltDiagnostic& d,
                                                  std::string_view product_version) const;
    // Export one complete causal chain as an array of canonical inner records.
    // Independent retained chains must be exported separately, never spliced.
    [[nodiscard]] std::string serialize_chain(std::shared_ptr<const DiagnosticEnvelope> leaf) const;

    // Returns the full diagnostic chain (root first, leaf last) by
    // walking parent links from `leaf` back to the root and reversing.
    // If the chain is malformed, returns an empty vector.
    [[nodiscard]] std::vector<std::shared_ptr<const DiagnosticEnvelope>>
    expose_chain(std::shared_ptr<const DiagnosticEnvelope> leaf) const;
};

}  // namespace ash
