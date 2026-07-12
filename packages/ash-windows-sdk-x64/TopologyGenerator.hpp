// ash/TopologyGenerator.hpp
//
// TopologyGenerator — canonical module.
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
// Implements the APS 1.0 ordered ternary structural topology:
// root path R, child tokens C/P/N, and deterministic path-based IDs.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "DiagnosticEnvelope.hpp"
#include "State.hpp"

namespace ash {

struct TopologyNode {
    std::string path;
    std::string node_id;
    std::size_t depth{0};
    std::size_t generation_ordinal{0};
    std::size_t global_ordinal{0};
    std::size_t parent_index{0};
    bool has_parent{false};
};

struct TopologyStructure {
    std::vector<TopologyNode> nodes;
    std::vector<std::size_t> leaf_indices;
};

struct TopologyResult {
    TopologyStructure topology;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

class TopologyGenerator {
public:
    static constexpr std::size_t kDefaultMaxDepth = 6;

    TopologyGenerator() = default;

    [[nodiscard]] TopologyResult generate_topology(const Bit9State& state,
                                                   std::size_t requested_depth = 0) const;
};

}  // namespace ash
