// ash/State.hpp
//
// 9-bit state representation for the ASH Pattern System — Windows C++
// downstream implementation branch.
//
// Canonical source of truth:
//   specs/core/ash-state-space.pseudo.md on main
//
// Quote (from the canonical spec):
//   "The ASH state space is F2^9 — the full 9-dimensional binary state
//    space with 512 vertices. Each state is a 9-bit binary vector
//    (b0, b1, b2, b3, b4, b5, b6, b7, b8). All 9 coordinates participate
//    in the algebraic structure."
//
// Branch-local convention (not canonical, see windows/conformance/deviation-log.md):
//   The integer encoding used below packs a 9-bit state into a uint16_t
//   with `bit i = (encoded >> i) & 1`, so b0 is the least significant
//   bit and b8 is bit 8. The canonical spec does not prescribe an
//   integer encoding.
//
// This file is implementation-facing. It does not redefine canonical
// semantics. On every semantic question, the canonical spec wins.

#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace ash {

// Bit9State — the full 9-bit ASH state as a std::bitset.
// std::bitset<9> provides coordinate access (b[i]) and XOR (b ^ c).
using Bit9State = std::bitset<9>;

// Exact number of distinct 9-bit states in F2^9 (2^9 = 512).
inline constexpr std::size_t kStateSpaceSize = 512;

// Branch-local integer encoding. Deterministic, reversible.
// See deviation-log.md item "Branch-local integer encoding".
inline constexpr Bit9State from_int(std::uint16_t encoded) noexcept {
    // Take only the low 9 bits. Bits 9..15 are ignored.
    return Bit9State{static_cast<unsigned long long>(encoded & 0x1FFu)};
}

inline constexpr std::uint16_t to_int(const Bit9State& s) noexcept {
    return static_cast<std::uint16_t>(s.to_ulong() & 0x1FFu);
}

// Coordinate accessors. Named bN() for clarity in unit tests.
inline constexpr bool b0(const Bit9State& s) noexcept { return s.test(0); }
inline constexpr bool b1(const Bit9State& s) noexcept { return s.test(1); }
inline constexpr bool b2(const Bit9State& s) noexcept { return s.test(2); }
inline constexpr bool b3(const Bit9State& s) noexcept { return s.test(3); }
inline constexpr bool b4(const Bit9State& s) noexcept { return s.test(4); }
inline constexpr bool b5(const Bit9State& s) noexcept { return s.test(5); }
inline constexpr bool b6(const Bit9State& s) noexcept { return s.test(6); }
inline constexpr bool b7(const Bit9State& s) noexcept { return s.test(7); }
inline constexpr bool b8(const Bit9State& s) noexcept { return s.test(8); }

}  // namespace ash
