// ash/ArtifactEmitter.hpp
//
// ArtifactEmitter — canonical module.
//
// ┌──────────────────────────────────────────────────────────────────┐
// │  LOCKED MATERIALIZATION BOUNDARY                                 │
// │                                                                  │
// │  Quote from specs/interfaces/semantic-contracts.md on main:      │
// │                                                                  │
// │    "ArtifactEmitter materializes that plan for a target          │
// │     runtime/platform. It must not invent semantics not present  │
// │     in the plan. ... The plan is the sole interface between     │
// │     planner and emitter."                                        │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Canonical source of truth:
//   specs/interfaces/contracts/artifact-emitter-contract.md on main
//   specs/interfaces/semantic-contracts.md on main § Materialization boundary (locked)
//
// Canonical invariants (INV-BOUNDARY-002):
//   - Every emitted artifact traceable to plan element
//   - No semantics invented beyond plan definition
//   - Emission is deterministic
//
// Prohibited behaviors (canonical, LOCKED):
//   - MUST NOT invent semantics not present in plan
//   - MUST NOT skip plan elements during materialization
//   - MUST NOT substitute local defaults for plan-defined behavior
//   - MUST NOT call back to planner for additional information
//   - MUST NOT collapse planning and materialization into one step
//   - MUST NOT treat plan as optional or advisory
//
// Interface enforcement on this branch:
//   The materialize_plan() method signature takes exactly one
//   GenerationPlan and one TargetConfiguration. No reference to a
//   GenerationPlanner is accepted. This is the architectural
//   enforcement of "no callback to planner" at the header level.
//
// materialize_plan() emits deterministic artifact descriptors for the
// supplied plan and blocks incomplete plans.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "DiagnosticEnvelope.hpp"
#include "GenerationPlanner.hpp"

namespace ash {

// Placeholder target configuration. Concrete shape is target-class-specific.
struct TargetConfiguration {
    std::string output_directory;  // branch-local: not used until runtime materialization
    std::string target_class;
};

// A materialized artifact. Shape is a simple descriptor on this
// branch; the canonical spec leaves the concrete artifact type open
// to the target runtime.
struct Artifact {
    std::string plan_element_reference;  // traces back to a plan element
    std::string kind;                     // e.g. "file", "module", "manifest"
    std::string summary;                  // human-readable description
};

struct EmissionResult {
    std::vector<Artifact> artifacts;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

class ArtifactEmitter {
public:
    ArtifactEmitter() = default;

    // Stub on this branch. Signature rejects any planner callback
    // by construction — only plan and config are accepted.
    [[nodiscard]] EmissionResult
    materialize_plan(const GenerationPlan& plan,
                     const TargetConfiguration& config) const;
};

}  // namespace ash
