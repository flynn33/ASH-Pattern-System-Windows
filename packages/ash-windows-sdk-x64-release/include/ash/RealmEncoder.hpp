// ash/RealmEncoder.hpp
//
// RealmEncoder — canonical module.
//
// Canonical source of truth:
//   specs/interfaces/contracts/realm-encoder-contract.md on main
//   specs/core/realm-identity.pseudo.md on main
//
// Canonical purpose (paraphrased):
//   Encodes deterministic realm identity from a well-formed 9-bit ASH state.
//
// Preconditions (canonical):
//   - Input state must be well formed
//   - State must have a deterministic realm identity
//
// Postconditions and invariants (canonical):
//   - Encoding is deterministic (INV-REALM-001)
//   - Encoding operates only on well-formed states (INV-REALM-002)
//   - All 9 coordinates participate
//
// Prohibited behaviors (canonical):
//   - MUST NOT encode from malformed state
//   - MUST NOT silently produce default encoding on malformed input
//   - MUST NOT decompose 9-bit state for encoding
//
// Realm identity format:
//   state_signature: 9 ASCII characters in b0..b8 order
//   realm_index: unsigned integer 0..511 with b0 as most significant
//   realm_id: APS-REALM-000 through APS-REALM-511

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "DiagnosticEnvelope.hpp"
#include "State.hpp"

namespace ash {

struct RealmIdentity {
    std::string state_signature;
    std::uint16_t realm_index{0};
    std::string realm_id;
    std::string orbit_id;
    bool b8{false};
};

struct RealmEncodingResult {
    RealmIdentity identity;
    std::shared_ptr<DiagnosticEnvelope> diagnostic;
};

class RealmEncoder {
public:
    RealmEncoder() = default;

    [[nodiscard]] RealmEncodingResult encode_realm_identity(const Bit9State& state) const;
};

}  // namespace ash
