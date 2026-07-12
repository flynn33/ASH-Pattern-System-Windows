// Codeword.cpp
//
// Definitions of the canonical 16-element codeword set C ⊂ F2^9.
//
// Canonical source of truth:
//   specs/core/codeword-set.pseudo.md on main — § Exhaustive enumeration
//
// The 16 codewords below are transcribed literally from the canonical
// specification. They are a [9, 4, 4] doubly-even linear code with
// weight distribution {0: 1, 4: 14, 8: 1} and 32 orbits in F2^9.
//
// Each codeword is laid out as (b0, b1, b2, b3, b4, b5, b6, b7, b8)
// and constructed via a small helper that takes bits in that order
// (so the literal on the page reads left-to-right from b0 to b8 and
// matches the table in the spec).

#include "ash/Codeword.hpp"

namespace ash {

namespace {

// Construct a codeword from 9 booleans given in (b0..b8) order.
// This matches the spec's coordinate layout so the tables below read
// identically to specs/core/codeword-set.pseudo.md.
<<<<<<< HEAD
constexpr Codeword cw(bool v0, bool v1, bool v2, bool v3,
                      bool v4, bool v5, bool v6, bool v7, bool v8) noexcept {
    Codeword c{};
    c.set(0, v0);
    c.set(1, v1);
    c.set(2, v2);
    c.set(3, v3);
    c.set(4, v4);
    c.set(5, v5);
    c.set(6, v6);
    c.set(7, v7);
    c.set(8, v8);
    return c;
}

}  // namespace

// clang-format off
<<<<<<< HEAD
const std::array<Codeword, kCodewordCount> kCanonicalCodewords{{
    //   b0 b1 b2 b3 b4 b5 b6 b7 b8   index weight (from spec)
    cw(  0, 0, 0, 0, 0, 0, 0, 0, 0),  //  0   0
    cw(  0, 0, 0, 0, 1, 1, 1, 1, 0),  //  1   4
    cw(  0, 0, 1, 1, 0, 0, 1, 1, 0),  //  2   4
    cw(  0, 0, 1, 1, 1, 1, 0, 0, 0),  //  3   4
    cw(  0, 1, 0, 1, 0, 1, 0, 1, 0),  //  4   4
    cw(  0, 1, 0, 1, 1, 0, 1, 0, 0),  //  5   4
    cw(  0, 1, 1, 0, 0, 1, 1, 0, 0),  //  6   4
    cw(  0, 1, 1, 0, 1, 0, 0, 1, 0),  //  7   4
    cw(  1, 0, 0, 1, 0, 1, 1, 0, 0),  //  8   4
    cw(  1, 0, 0, 1, 1, 0, 0, 1, 0),  //  9   4
    cw(  1, 0, 1, 0, 0, 1, 0, 1, 0),  // 10   4
    cw(  1, 0, 1, 0, 1, 0, 1, 0, 0),  // 11   4
    cw(  1, 1, 0, 0, 0, 0, 1, 1, 0),  // 12   4
    cw(  1, 1, 0, 0, 1, 1, 0, 0, 0),  // 13   4
    cw(  1, 1, 1, 1, 0, 0, 0, 0, 0),  // 14   4
    cw(  1, 1, 1, 1, 1, 1, 1, 1, 0),  // 15   8
<<<<<<< HEAD
}};

const std::array<Codeword, kCodewordGeneratorCount> kCodewordGenerators{{
    //   b0 b1 b2 b3 b4 b5 b6 b7 b8
    cw(  1, 1, 1, 1, 0, 0, 0, 0, 0),  // g1
    cw(  1, 1, 0, 0, 1, 1, 0, 0, 0),  // g2
    cw(  1, 0, 1, 0, 1, 0, 1, 0, 0),  // g3
    cw(  1, 0, 0, 1, 1, 0, 0, 1, 0),  // g4
<<<<<<< HEAD
}};
// clang-format on

bool is_canonical_codeword(const Codeword& c) noexcept {
    for (const auto& member : kCanonicalCodewords) {
        if (member == c) {
            return true;
        }
    }
    return false;
}

bool canonical_set_contains_zero() noexcept {
    return is_canonical_codeword(Codeword{});
}

}  // namespace ash

