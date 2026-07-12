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
// Required rule-ID pattern (from rule-id-taxonomy.md):
//   ^[A-Z][A-Z-]*-[A-Z][A-Z-]*-[0-9]{3}$
//
// Canonical invariants:
//   - INV-DIAG-001: Every diagnostic conforms to shared envelope schema
//   - INV-DIAG-002: Every rule_id conforms to taxonomy pattern
//   - INV-DIAG-003: Diagnostic chain is complete and reconstructable
//   - INV-DIAG-004: No silent omission
//
// Stage ordering (canonical):
//   ESCALATION parent must be RECOVERY or CLASSIFICATION.
//   TERMINAL parent must be ESCALATION or RECOVERY.
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

class Diagnostics {
public:
    Diagnostics() = default;

    // Returns true iff the envelope has all required fields populated
    // and non-empty: diagnostic_kind/severity/stage/disposition (enum,
    // always valid), subject_reference non-empty, chain_root_reference
    // non-null, rule_ids non-empty and each taxonomy-compliant, summary
    // non-empty, notes non-empty and each non-empty.
    [[nodiscard]] bool validate_envelope(const DiagnosticEnvelope& d) const noexcept;

    // Returns true iff the rule ID matches the canonical taxonomy
    // pattern. Manual parse, no stdlib regex, for compile-time and
    // runtime speed.
    [[nodiscard]] bool validate_rule_id(std::string_view id) const noexcept;

    // Walks parent_diagnostic_reference back to the root. Verifies
    // each link's chain_root_reference matches the root. Verifies
    // stage ordering at parent/child boundaries. Returns true iff
    // the chain is complete and valid.
    [[nodiscard]] bool validate_chain(const DiagnosticEnvelope& d) const;

    // Returns the full diagnostic chain (root first, leaf last) by
    // walking parent links from `leaf` back to the root and reversing.
    // If the chain is malformed, returns an empty vector.
    [[nodiscard]] std::vector<std::shared_ptr<const DiagnosticEnvelope>>
    expose_chain(std::shared_ptr<const DiagnosticEnvelope> leaf) const;
};

}  // namespace ash
