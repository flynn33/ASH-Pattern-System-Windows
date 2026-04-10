// ash/TopologyGenerator.hpp
//
// TopologyGenerator — canonical module (non-slice, stub on this branch).
//
// Canonical source of truth:
//   specs/interfaces/contracts/topology-generator-contract.md on main
//   specs/algorithms/topology-expansion.pseudo.md on main
//   specs/algorithms/branching-semantics.pseudo.md on main
//
// Canonical purpose (paraphrased):
//   Generates deterministic topology from a normalized state,
//   preserving stable ordering and lineage relationships.
//
// Preconditions (canonical):
//   - Input state must be normalized
//
// Postconditions and invariants (canonical):
//   - Topology generation is deterministic (INV-TOPO-001)
//   - Ordering is stable and reproducible across platforms
//   - Lineage preserved across invocations
//
// Prohibited behaviors (canonical):
//   - MUST NOT produce non-deterministic topology
//   - MUST NOT allow ordering to depend on platform-specific behavior
//   - MUST NOT silently skip invalid conditions
//
// Branch-local status: STUB. Detailed branching and expansion
// algorithms are pending in the canonical R2 realignment
// (see specs/algorithms/branching-semantics.pseudo.md) and are not
// yet prescribed at the level needed for a faithful implementation.
// generate_topology() returns a NOT_IMPLEMENTED diagnostic with the
// branch-local ASH-WINDOWS-STUB-001 rule ID.
// See windows/conformance/deviation-log.md item 6.

#pragma once

#include <memory>
#include <vector>

#include "DiagnosticEnvelope.hpp"
#include "State.hpp"

namespace ash {

// Placeholder topology structure. Shape will be defined when the
// canonical branching/topology algorithms are formalized.
struct TopologyStructure {
    std::vector<Bit9State> nodes;  // empty on this branch
    std::vector<std::pair<std::size_t, std::size_t>> edges;  // empty on this branch
};

struct TopologyResult {
    TopologyStructure topology;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

class TopologyGenerator {
public:
    TopologyGenerator() = default;

    // Stub on this branch. Returns an empty topology and a
    // NOT_IMPLEMENTED diagnostic.
    [[nodiscard]] TopologyResult generate_topology(const Bit9State& state) const;
};

}  // namespace ash
