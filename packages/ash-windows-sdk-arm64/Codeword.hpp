// ash/Codeword.hpp
//
// Canonical codeword set C ⊂ F2^9 — declarations only. Definitions in
// windows/src/Codeword.cpp, transcribed verbatim from
// specs/core/codeword-set.pseudo.md on main.
//
// Canonical source of truth:
//   specs/core/codeword-set.pseudo.md on main
//
// Quote (from the canonical spec):
//   "The canonical codeword set C has been extracted from published
//    research materials. ... Length 9, Dimension 4, |C| = 16, minimum
//    distance 4. [9, 4, 4] doubly-even. Weight distribution
//    {0: 1, 4: 14, 8: 1}. Number of orbits 32 (= 512 / 16)."
//
// Structural property (canonical): every codeword has b8 = 0. This is
// a verified algebraic fact of the specific [9, 4, 4] code. It does
// NOT re-establish the 9th coordinate as a derived control or parity
// dimension; all 9 coordinates remain structurally equal at the
// foundational level.
//
// Implementation constraints (from the canonical spec):
//   - Must use exactly the 16-codeword set specified — no more, no fewer
//   - Must treat C as a fixed canonical constant, not a runtime choice
//   - Must enforce that transformation codewords are members of C
//   - Must NOT invent codewords outside C
//   - Must NOT extend C beyond the 16 specified members
//   - Must NOT use the b8 = 0 property to re-derive a control/parity bit

#pragma once

#include <array>
#include <cstddef>

#include "State.hpp"

namespace ash {

// A codeword is a 9-bit vector in F2^9. Same underlying type as a state,
// but semantically distinct: codewords are members of the fixed set C.
using Codeword = Bit9State;

// |C| = 16 by the canonical [9, 4, 4] code parameters.
inline constexpr std::size_t kCodewordCount = 16;

// Number of independent generators of C (dimension 4).
inline constexpr std::size_t kCodewordGeneratorCount = 4;

// Canonical codeword set C ⊂ F2^9. Defined in Codeword.cpp with all
// 16 codewords transcribed literally from
// specs/core/codeword-set.pseudo.md.
extern const std::array<Codeword, kCodewordCount> kCanonicalCodewords;

// The 4 independent generators of C. Useful for property-based tests
// that exercise XOR-closure.
extern const std::array<Codeword, kCodewordGeneratorCount> kCodewordGenerators;

// Returns true if `c` is a member of the canonical codeword set.
// Linear scan over the 16-element table. Deterministic.
[[nodiscard]] bool is_canonical_codeword(const Codeword& c) noexcept;

// Returns true if the canonical set contains the zero codeword (identity).
// Always true by construction; exposed for defensive tests.
[[nodiscard]] bool canonical_set_contains_zero() noexcept;

}  // namespace ash
