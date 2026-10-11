// ash/GenerationPlanner.hpp
//
// GenerationPlanner — canonical module.
//
// ┌──────────────────────────────────────────────────────────────────┐
// │  LOCKED MATERIALIZATION BOUNDARY                                 │
// │                                                                  │
// │  Quote from specs/interfaces/semantic-contracts.md on main:      │
// │                                                                  │
// │    "The boundary between GenerationPlanner and ArtifactEmitter   │
// │     is a locked design decision:                                 │
// │       - GenerationPlanner produces an abstract, target-aware     │
// │         but non-materialized plan. It must not emit artifacts    │
// │         or perform side effects.                                 │
// │       - ArtifactEmitter materializes that plan for a target      │
// │         runtime/platform. It must not invent semantics not       │
// │         present in the plan.                                     │
// │       - The plan is the sole interface between planner and      │
// │         emitter.                                                 │
// │       - Planning and materialization must not be collapsed into │
// │         a single opaque step."                                   │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Canonical source of truth:
//   specs/interfaces/contracts/generation-planner-contract.md on main
//   specs/algorithms/generation-planning.pseudo.md on main
//   specs/interfaces/semantic-contracts.md on main § Materialization boundary (locked)
//
// Canonical invariants (INV-PLAN-001, INV-PLAN-002, INV-BOUNDARY-001):
//   - Planning produces no side effects
//   - Plan is self-contained (emitter needs only the plan)
//   - Plan is inspectable before materialization
//   - No artifacts emitted by planner
//
// Prohibited behaviors (canonical, LOCKED):
//   - MUST NOT emit artifacts directly
//   - MUST NOT perform side effects (no file I/O, no directory creation, no process start)
//   - MUST NOT produce a plan requiring the emitter to call back
//   - MUST NOT leave the plan incomplete or implicit
//   - MUST NOT collapse planning and materialization into one opaque step
//
// Interface enforcement on this branch:
//   The generate_plan() method signature returns a GenerationPlan
//   value and takes only pure-data inputs. No I/O stream, filesystem
//   path, or side-effecting callback is accepted as a parameter. This
//   is the architectural enforcement of the LOCKED boundary at the
//   header level; runtime tests cover plan hash and emitter traceability.
//
// Produces an inspectable plan with a deterministic SHA-256 hash.

#pragma once

#include <memory>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "AxiomEvaluator.hpp"
#include "DiagnosticEnvelope.hpp"
#include "RealmEncoder.hpp"
#include "State.hpp"
#include "TopologyGenerator.hpp"

namespace ash {

// Opaque target constraints. Concrete shape is target-class-specific
// and is not prescribed by the canonical spec at this level. The
// struct is carried by value through the planner and must not cause
// the planner to perform any side effect.
struct TargetMetadata {
    std::string target_class;  // e.g. "desktop", "mobile", "service"
    std::string runtime_hint;  // informational only
    std::string profile_id{"aps-windows-descriptor-1.0"};
    std::string registry_version{"aps-windows-1.0"};
};

// Exact canonical request fields. Native references are explicit downstream
// registry IDs, not canonical defaults. Transitions are APS-TRANSITION-CW-00..15.
struct GenerationRequest {
    std::string project_name;
    Bit9State seed_state;
    std::vector<std::string> transition_sequence;
    std::int64_t topology_depth{0};
    std::string profile_id{"aps-windows-descriptor-1.0"};
    std::string naming_policy_id{"aps-windows-realm-names-1.0"};
    std::string emission_target_kind{"descriptor"};
};

// Caller measurements are optional; absent measurements stay INDETERMINATE.
// Registry version is a nonblank caller provenance label, not a policy selector.
// The axiom profile label identifies evidence for the fixed five predicates.
struct GenerationPlanningInputs {
    TargetMetadata target{"desktop", ""};
    std::optional<AxiomSubject> axiom_subject;
    AxiomEvaluationProfile axiom_profile;
};

struct GenerationRoleAssignment {
    std::string assignment_id;
    std::string node_id;
    std::string role;  // root, branch, or leaf, in structural topology order
    std::string realm_id;
};

struct ArtifactDescription {
    std::string artifact_id;
    std::string kind;
    std::string name;
    std::string summary;
    std::vector<std::string> role_references;
};

// A GenerationPlan is abstract, inspectable, and self-contained.
// The emitter must be able to materialize it with only this value
// plus a TargetConfiguration — no planner callback.
struct GenerationPlan {
    std::string source_signature;
    std::string source_realm_id;
    std::vector<std::string> artifact_descriptions;
    std::vector<std::string> role_assignments;
    std::string target_class;
    std::string registry_version;
    std::string plan_hash;
    std::string runtime_hint;
    std::string profile_id{"aps-windows-descriptor-1.0"};
    GenerationRequest request;
    GenerationPlanningInputs inputs;
    Bit9State normalized_state;
    Bit9State transformed_state;
    RealmIdentity source_realm;
    RealmIdentity destination_realm;
    std::vector<TopologyNode> topology_nodes;
    std::vector<std::size_t> leaf_indices;
    // Complete typed assignments; role_assignments above retains the legacy
    // source/target projection followed by each typed node assignment.
    std::vector<GenerationRoleAssignment> roles;
    AxiomSubject axiom_subject;
    AxiomEvaluationProfile axiom_profile;
    AxiomSubjectDiagnostic axiom_diagnostic;
    std::vector<ArtifactDescription> artifacts;
    std::vector<std::string> warnings;
    std::map<std::string, std::string, std::less<>> metadata;
};

// Versioned native wire payload (aps-windows-plan-2). Ordered fields are encoded
// as name:byte_count:value followed by LF; arrays carry counts; optional evidence
// carries presence. Double measurements bind their exact IEEE-754 bits. The
// plan hash is lowercase SHA-256 over these bytes, excluding only plan_hash.
// Diagnostic records are full canonical JSON and require valid causal chains.
[[nodiscard]] std::string generation_plan_identity_payload(const GenerationPlan& plan);

struct PlanResult {
    GenerationPlan plan;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

class GenerationPlanner {
public:
    GenerationPlanner() = default;

    // Supported profile: aps-windows-descriptor-1.0. Naming policies:
    // aps-windows-realm-names-1.0 and aps-windows-project-names-1.0.
    // Target kind: descriptor; target classes: desktop, mobile, service.
    // Project names use ASCII letters, digits, dot, underscore, and hyphen.
    [[nodiscard]] PlanResult generate_plan(const GenerationRequest& request,
                                           const GenerationPlanningInputs& inputs = {}) const;

    // Compatibility request: project ash-plan, no transitions, depth zero,
    // realm naming, descriptor output, and no measured axiom evidence.
    [[nodiscard]] PlanResult generate_plan(const Bit9State& state,
                                           const TargetMetadata& target) const;
};

}  // namespace ash
