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
// Canonical convention:
//   b0 is the most significant bit and b8 is the least significant bit.
//   The integer encoding is therefore:
//     state_index = sum(bi * 2^(8-i)) for i = 0..8
//
// This file is implementation-facing. It does not redefine canonical
// semantics. On every semantic question, the canonical spec wins.

#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <string>

namespace ash {

// Bit9State — the full 9-bit ASH state as a std::bitset.
// std::bitset<9> provides coordinate access (b[i]) and XOR (b ^ c).
using Bit9State = std::bitset<9>;

// Exact number of distinct 9-bit states in F2^9 (2^9 = 512).
inline constexpr std::size_t kStateSpaceSize = 512;

// Canonical integer encoding. Deterministic, reversible.
inline Bit9State from_int(std::uint16_t encoded) noexcept {
    Bit9State state{};
    const std::uint16_t masked = static_cast<std::uint16_t>(encoded & 0x1FFu);
    for (std::size_t i = 0; i < 9; ++i) {
        const std::uint16_t mask = static_cast<std::uint16_t>(1u << (8u - i));
        state.set(i, (masked & mask) != 0);
    }
    return state;
}

inline std::uint16_t to_int(const Bit9State& s) noexcept {
    std::uint16_t encoded = 0;
    for (std::size_t i = 0; i < 9; ++i) {
        if (s.test(i)) {
            encoded = static_cast<std::uint16_t>(encoded | (1u << (8u - i)));
        }
    }
    return encoded;
}

inline std::string canonical_signature(const Bit9State& s) {
    std::string out;
    out.reserve(9);
    for (std::size_t i = 0; i < 9; ++i) {
        out.push_back(s.test(i) ? '1' : '0');
    }
    return out;
}

inline std::string canonical_realm_id(const Bit9State& s) {
    const std::uint16_t index = to_int(s);
    std::string id = "APS-REALM-";
    if (index < 10) {
        id += "00";
    } else if (index < 100) {
        id += "0";
    }
    id += std::to_string(index);
    return id;
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
