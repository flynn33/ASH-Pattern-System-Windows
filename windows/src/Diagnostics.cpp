// Diagnostics.cpp
//
// Implementation of the Diagnostics canonical cross-cutting module.
//
// Canonical source of truth:
//   specs/interfaces/diagnostic-schema.md on main
//   specs/interfaces/rule-id-taxonomy.md on main
//   specs/interfaces/contracts/diagnostics-module-contract.md on main

#include "ash/Diagnostics.hpp"

#include <algorithm>
#include <cctype>

namespace ash {

namespace {

bool is_upper_or_dash(char c) noexcept {
    return (c >= 'A' && c <= 'Z') || c == '-';
}

bool is_ascii_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

// Validate {FAMILY}-{CATEGORY}-{NUMBER} where FAMILY and CATEGORY are
// each one or more uppercase-or-dash runs that start with an
// uppercase letter, and NUMBER is exactly three digits. The input is
// split on the last two dashes: the tail must be 4 characters
// (dash + 3 digits), the remaining prefix must contain at least one
// dash separating FAMILY from CATEGORY, and each segment must start
// with an uppercase letter.
bool validate_taxonomy_pattern(std::string_view id) noexcept {
    // Minimum length: "A-B-000" = 7 characters.
    if (id.size() < 7) return false;

    // Trailing 4 characters must be '-' followed by 3 digits.
    const std::size_t n = id.size();
    if (id[n - 4] != '-') return false;
    for (std::size_t i = n - 3; i < n; ++i) {
        if (!is_ascii_digit(id[i])) return false;
    }

    // Prefix is id[0 .. n-4). It must contain at least one dash and
    // the segments before and after the last dash in the prefix
    // must each start with an uppercase letter and consist only of
    // uppercase letters and dashes.
    const std::string_view prefix = id.substr(0, n - 4);

    // Find the last dash in the prefix (separating FAMILY and CATEGORY).
    const std::size_t last_dash = prefix.rfind('-');
    if (last_dash == std::string_view::npos) return false;

    const std::string_view family = prefix.substr(0, last_dash);
    const std::string_view category = prefix.substr(last_dash + 1);

    if (family.empty() || category.empty()) return false;

    // Each must begin with an uppercase letter.
    if (family.front() < 'A' || family.front() > 'Z') return false;
    if (category.front() < 'A' || category.front() > 'Z') return false;

    // Every character must be uppercase letter or dash. No leading or
    // trailing dash in family or category.
    if (family.back() == '-' || category.back() == '-') return false;
    for (char c : family) {
        if (!is_upper_or_dash(c)) return false;
    }
    for (char c : category) {
        if (!is_upper_or_dash(c)) return false;
    }
    return true;
}

}  // namespace

bool Diagnostics::validate_rule_id(std::string_view id) const noexcept {
    return validate_taxonomy_pattern(id);
}

bool Diagnostics::validate_envelope(const DiagnosticEnvelope& d) const noexcept {
    if (d.subject_reference.empty()) return false;
    if (d.chain_root_reference == nullptr) return false;
    if (d.rule_ids.empty()) return false;
    if (d.summary.empty()) return false;
    if (d.notes.empty()) return false;
    for (const auto& id : d.rule_ids) {
        if (id.empty()) return false;
        if (!validate_taxonomy_pattern(id)) return false;
    }
    for (const auto& n : d.notes) {
        if (n.empty()) return false;
    }
    return true;
}

bool Diagnostics::validate_chain(const DiagnosticEnvelope& d) const {
    // Walk parent links back to the root. Collect every envelope we
    // visit to check root-reference consistency at every step.
    const DiagnosticEnvelope* current = &d;
    const DiagnosticEnvelope* root = d.chain_root_reference.get();
    if (root == nullptr) return false;

    // Stage-ordering check: for every (child, parent) pair in the
    // chain, ESCALATION child requires parent stage in {RECOVERY,
    // CLASSIFICATION}; TERMINAL child requires parent stage in
    // {ESCALATION, RECOVERY}.
    int guard = 0;
    while (current->parent_diagnostic_reference != nullptr) {
        const DiagnosticEnvelope* parent = current->parent_diagnostic_reference.get();
        if (current->stage == Stage::ESCALATION) {
            if (parent->stage != Stage::RECOVERY &&
                parent->stage != Stage::CLASSIFICATION) {
                return false;
            }
        } else if (current->stage == Stage::TERMINAL) {
            if (parent->stage != Stage::ESCALATION &&
                parent->stage != Stage::RECOVERY) {
                return false;
            }
        }
        // Every envelope in the chain should carry the same
        // chain_root_reference.
        if (current->chain_root_reference.get() != root) {
            return false;
        }
        current = parent;
        if (++guard > 1024) return false;  // guard against pathological cycles
    }

    // Root must self-reference.
    if (current != root) return false;
    if (current->parent_diagnostic_reference != nullptr) return false;
    return true;
}

std::vector<std::shared_ptr<const DiagnosticEnvelope>>
Diagnostics::expose_chain(std::shared_ptr<const DiagnosticEnvelope> leaf) const {
    std::vector<std::shared_ptr<const DiagnosticEnvelope>> result;
    if (!leaf) return result;

    // Walk leaf -> root, collect, then reverse.
    auto current = leaf;
    int guard = 0;
    while (current != nullptr) {
        result.push_back(current);
        if (current->parent_diagnostic_reference == nullptr) {
            break;
        }
        current = current->parent_diagnostic_reference;
        if (++guard > 1024) {
            result.clear();
            return result;
        }
    }
    std::reverse(result.begin(), result.end());
    return result;
}

}  // namespace ash
