// TopologyGenerator.cpp

#include "ash/TopologyGenerator.hpp"

#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"
#include "ash/State.hpp"
#include <limits>

namespace ash {

namespace {

std::size_t pow3(std::size_t exponent) noexcept {
    std::size_t value = 1;
    for (std::size_t i = 0; i < exponent; ++i) {
        value *= 3;
    }
    return value;
}

char token_for_digit(std::size_t digit) noexcept {
    if (digit == 0) return 'C';
    if (digit == 1) return 'P';
    return 'N';
}

std::string path_for(std::size_t depth, std::size_t ordinal) {
    if (depth == 0) return "R";
    std::string path = "R";
    std::size_t divisor = pow3(depth - 1);
    for (std::size_t i = 0; i < depth; ++i) {
        const std::size_t digit = ordinal / divisor;
        ordinal %= divisor;
        path.push_back('/');
        path.push_back(token_for_digit(digit));
        if (divisor > 1) {
            divisor /= 3;
        }
    }
    return path;
}

std::string node_id_for(const std::string& path) {
    std::string id = "APS-NODE-";
    for (char c : path) {
        id.push_back(c == '/' ? '.' : c);
    }
    return id;
}

std::size_t global_ordinal_for(std::size_t depth, std::size_t generation_ordinal) noexcept {
    return ((pow3(depth) - 1) / 2) + generation_ordinal;
}

}  // namespace

TopologyResult TopologyGenerator::generate_topology(const Bit9State& state,
                                                    std::size_t requested_depth) const {
    const std::string subject = "topology[state=" + canonical_signature(state) +
                                ",depth=" + std::to_string(requested_depth) + "]";
    if (requested_depth > kDefaultMaxDepth) {
        std::size_t leaves = 1;
        bool overflow = false;
        for (std::size_t i = 0; i < requested_depth; ++i) {
            if (leaves > std::numeric_limits<std::size_t>::max() / 3) { overflow = true; break; }
            leaves *= 3;
        }
        const bool node_overflow = overflow || leaves > (std::numeric_limits<std::size_t>::max() - 1) / 3;
        auto diagnostic =
            DiagnosticEnvelopeBuilder{}
                .kind(DiagnosticKind::TOPOLOGY)
                .severity(Severity::ERROR)
                .stage(Stage::DETECTION)
                .disposition(Disposition::BLOCKED)
                .subject(subject)
                .rule_id(rule_ids::kTopology002)
                .summary("Topology request exceeds configured resource limit")
                .note("The request is rejected before partial output. The "
                      "Windows core default maximum topology depth is " +
                      std::to_string(kDefaultMaxDepth) + "; requested_depth=" + std::to_string(requested_depth) +
                      "; projected_leaves=" + (overflow ? "unrepresentable" : std::to_string(leaves)) +
                      "; projected_nodes=" + (node_overflow ? "unrepresentable" : std::to_string((leaves * 3 - 1) / 2)) + ".")
                .build_root();
        return TopologyResult{TopologyStructure{}, std::move(diagnostic)};
    }

    TopologyStructure topology;
    topology.depth = requested_depth;
    const std::size_t total_nodes = (pow3(requested_depth + 1) - 1) / 2;
    topology.nodes.reserve(total_nodes);

    for (std::size_t depth = 0; depth <= requested_depth; ++depth) {
        const std::size_t width = pow3(depth);
        for (std::size_t ordinal = 0; ordinal < width; ++ordinal) {
            TopologyNode node;
            node.path = path_for(depth, ordinal);
            node.node_id = node_id_for(node.path);
            node.depth = depth;
            node.generation_ordinal = ordinal;
            node.global_ordinal = global_ordinal_for(depth, ordinal);
            node.has_parent = depth > 0;
            if (node.has_parent) {
                node.parent_index = global_ordinal_for(depth - 1, ordinal / 3);
                node.parent_path = node.path.substr(0, node.path.rfind('/'));
            }
            node.is_leaf = depth == requested_depth;
            if (depth == requested_depth) {
                topology.leaf_indices.push_back(node.global_ordinal);
            }
            topology.nodes.push_back(std::move(node));
        }
    }

    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::TOPOLOGY)
            .severity(Severity::INFO)
            .stage(Stage::CLASSIFICATION)
            .disposition(Disposition::RESOLVED)
            .subject(subject)
            .rule_id(rule_ids::kTopology001)
            .summary("Generated deterministic ordered ternary topology")
            .note("Topology includes every node from depth 0 through the "
                  "requested depth in ascending depth and generation order.")
            .build_root();

    return TopologyResult{std::move(topology), std::move(diagnostic)};
}

TopologyResult TopologyGenerator::reject_negative_depth_(std::int64_t requested_depth) const {
    auto diagnostic = DiagnosticEnvelopeBuilder{}
        .kind(DiagnosticKind::TOPOLOGY).severity(Severity::ERROR).stage(Stage::DETECTION)
        .disposition(Disposition::BLOCKED).subject("topology[negative-depth=" + std::to_string(requested_depth) + "]")
        .rule_id(rule_ids::kTopology002).summary("Requested topology depth must be non-negative")
        .note("Request rejected before projected counts or partial topology construction.").build_root();
    return {TopologyStructure{}, std::move(diagnostic)};
}

}  // namespace ash
