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
#include <string>
#include <vector>

#include "DiagnosticEnvelope.hpp"
#include "State.hpp"

namespace ash {

// Opaque target constraints. Concrete shape is target-class-specific
// and is not prescribed by the canonical spec at this level. The
// struct is carried by value through the planner and must not cause
// the planner to perform any side effect.
struct TargetMetadata {
    std::string target_class;  // e.g. "desktop", "mobile", "service"
    std::string runtime_hint;  // informational only
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
};

struct PlanResult {
    GenerationPlan plan;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

class GenerationPlanner {
public:
    GenerationPlanner() = default;

    [[nodiscard]] PlanResult generate_plan(const Bit9State& state,
                                           const TargetMetadata& target) const;
};

}  // namespace ash
