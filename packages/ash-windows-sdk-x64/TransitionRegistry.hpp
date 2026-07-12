// ash/TransitionRegistry.hpp
//
// TransitionRegistry — canonical module.
//
// Canonical source of truth:
//   specs/interfaces/contracts/transition-registry-contract.md on main
//   specs/algorithms/transition-system.pseudo.md on main
//   specs/algorithms/codeword-transformation-semantics.pseudo.md on main
//
// Canonical purpose (paraphrased):
//   Deterministic resolution and application of state transitions via
//   XOR-by-codeword transformation on full 9-bit states.
//
// Canonical rule (verbatim from transition-system.pseudo.md):
//   "Transitions operate on the full 9-bit state via XOR-by-codeword:
//    x' = x ⊕ c where x ∈ F2^9, c ∈ C ⊂ F2^9."
//
// Postconditions and invariants:
//   - Transition resolution is deterministic (INV-TRANS-001)
//   - XOR-by-codeword operates on all 9 bits simultaneously (INV-TRANS-002)
//   - Involution: (x ⊕ c) ⊕ c = x (INV-CODEWORD-003)
//   - Invalid inputs produce explainable diagnostics, not silent failure
//
// Prohibited behaviors:
//   - MUST NOT decompose state into sub-components for transformation
//   - MUST NOT silently drop a failed transition
//   - MUST NOT assume a codeword outside the canonical set C

#pragma once

#include <cstddef>
#include <memory>
#include <utility>

#include "Codeword.hpp"
#include "DiagnosticEnvelope.hpp"
#include "State.hpp"

namespace ash {

struct ResolveResult {
    Codeword codeword;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

struct TransitionResult {
    Bit9State state;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

class TransitionRegistry {
public:
    TransitionRegistry() = default;

    // Resolve a transition ID to a canonical codeword. `transition_id`
    // is an index into the canonical 16-element set. Out-of-range
    // IDs produce a zero codeword and an ERROR-severity diagnostic.
    // The `state` argument is accepted for contract symmetry but does
    // not influence resolution in the current branch implementation.
    [[nodiscard]] ResolveResult resolve_transition(const Bit9State& state,
                                                   std::size_t transition_id) const;

    // Apply a canonical transformation x' = x ⊕ c on the full 9-bit
    // state. If `codeword` is not a member of the canonical set, the
    // input state is returned unchanged with ERROR severity and
    // ASH-CODEWORD-004.
    [[nodiscard]] TransitionResult apply_transition(const Bit9State& state,
                                                    const Codeword& codeword) const;
};

}  // namespace ash
