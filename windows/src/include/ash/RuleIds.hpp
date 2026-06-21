// ash/RuleIds.hpp
//
// Canonical rule IDs used by the Windows branch semantic core modules.
//
// Canonical source of truth:
//   specs/interfaces/rule-id-taxonomy.md on main
//   specs/verification/invariant-spec.md on main
//
// Each constant below is a string literal matching the canonical
// pattern {FAMILY}-{CATEGORY}-{NUMBER}. The comment on each constant
// cites the canonical invariant or rule it reflects.
//
// Rule constants used by the Windows semantic core.

#pragma once

#include <string_view>

namespace ash::rule_ids {

// ASH-STATE family — state-space structure, validity, normalization
inline constexpr std::string_view kStateValidity001 = "ASH-STATE-VALIDITY-001";
// INV-STATE-001: "State uses all 9 coordinates in F2^9."
inline constexpr std::string_view kStateValidity002 = "ASH-STATE-VALIDITY-002";
// INV-STATE-002: "Normalization is deterministic."
inline constexpr std::string_view kStateValidity003 = "ASH-STATE-VALIDITY-003";
// INV-STATE-003: "Normalization fails with diagnostic for incompatible
// states; no silent result."

// ASH-ADMISSIBILITY family — admissibility classification
inline constexpr std::string_view kAdmissibility001 = "ASH-ADMISSIBILITY-001";
// INV-ADMISSIBILITY-001: "Admissibility classification is deterministic."
inline constexpr std::string_view kAdmissibility002 = "ASH-ADMISSIBILITY-002";
// INV-ADMISSIBILITY-002: "Every 9-bit vector maps to exactly one status."

// ASH-CLASSIFICATION family — system-state classification
inline constexpr std::string_view kClassification001 = "ASH-CLASSIFICATION-001";
// Deterministic class mapping.

// ASH-CODEWORD family — codeword structure and transformation
inline constexpr std::string_view kCodeword001 = "ASH-CODEWORD-001";
// INV-CODEWORD-001: "x' = x ⊕ c on all 9 bits."
inline constexpr std::string_view kCodeword002 = "ASH-CODEWORD-002";
// INV-CODEWORD-002: "Transformations are deterministic and pure."
inline constexpr std::string_view kCodeword003 = "ASH-CODEWORD-003";
// INV-CODEWORD-003: "(x ⊕ c) ⊕ c = x (involution)."
inline constexpr std::string_view kCodeword004 = "ASH-CODEWORD-004";
// INV-CODEWORD-004: "Codeword set matches research baseline [9,4,4]."

// ASH-TRANS family — transition system
inline constexpr std::string_view kTrans001 = "ASH-TRANS-001";
// INV-TRANS-001: "Equal state + equal transition = equal output."
inline constexpr std::string_view kTrans002 = "ASH-TRANS-002";
// INV-TRANS-002: "Transitions operate on full 9-bit states via XOR."

// ASH-REALM family — realm identity and canonical encoding
inline constexpr std::string_view kRealm001 = "ASH-REALM-001";
// Canonical realm ID, index, and signature encoding.

// ASH-TOPOLOGY family — deterministic structural topology
inline constexpr std::string_view kTopology001 = "ASH-TOPOLOGY-001";
// Ordered ternary topology generation.
inline constexpr std::string_view kTopology002 = "ASH-TOPOLOGY-002";
// Resource limit rejection before partial output.

// ASH-AXIOM family — evidence-based axiom evaluation
inline constexpr std::string_view kAxiom001 = "ASH-AXIOM-001";
// PASS/FAIL/INDETERMINATE result model.
inline constexpr std::string_view kAxiom002 = "ASH-AXIOM-002";
// Missing evidence remains indeterminate.

// ASH-GENERATION family — side-effect-free planning
inline constexpr std::string_view kGeneration001 = "ASH-GENERATION-001";
// Generation plan is inspectable and self-contained.
inline constexpr std::string_view kGeneration002 = "ASH-GENERATION-002";
// Deterministic plan hash.

// ASH-EMISSION family — plan-bound materialization
inline constexpr std::string_view kEmission001 = "ASH-EMISSION-001";
// Every artifact traces to a plan element.
inline constexpr std::string_view kEmission002 = "ASH-EMISSION-002";
// Incomplete plan is blocked.

// ASH-RECOVERY family — recovery semantics
inline constexpr std::string_view kRecovery001 = "ASH-RECOVERY-001";
// INV-RECOVERY-001: "Recovery classification is deterministic."
inline constexpr std::string_view kRecovery002 = "ASH-RECOVERY-002";
// INV-RECOVERY-002: "Fallback uses only canonical registry."
inline constexpr std::string_view kRecovery003 = "ASH-RECOVERY-003";
// INV-RECOVERY-003: "Escalation is monotonic."
inline constexpr std::string_view kRecovery004 = "ASH-RECOVERY-004";
// INV-RECOVERY-004: "No silent healing."
inline constexpr std::string_view kRecovery005 = "ASH-RECOVERY-005";
// INV-RECOVERY-005: "SAFE_HALT is terminal — no transitions out."

// ASH-FALLBACK family — fallback policy
inline constexpr std::string_view kFallback001 = "ASH-FALLBACK-001";
// Empty-registry escalation to containment.

// ASH-CONTAINMENT family — containment behavior
inline constexpr std::string_view kContainment001 = "ASH-CONTAINMENT-001";
// Canonical containment entry.

// ASH-HALT family — safe halt behavior
inline constexpr std::string_view kHalt001 = "ASH-HALT-001";
// Canonical safe halt entry.

// ASH-DIAG family — diagnostic envelope schema
inline constexpr std::string_view kDiag001 = "ASH-DIAG-001";
// INV-DIAG-001: "Every diagnostic conforms to envelope schema."
inline constexpr std::string_view kDiag002 = "ASH-DIAG-002";
// INV-DIAG-002: "Rule IDs conform to taxonomy."
inline constexpr std::string_view kDiag003 = "ASH-DIAG-003";
// INV-DIAG-003: "Chain is complete; references intact."
inline constexpr std::string_view kDiag004 = "ASH-DIAG-004";
// INV-DIAG-004: "No silent omission."

}  // namespace ash::rule_ids
