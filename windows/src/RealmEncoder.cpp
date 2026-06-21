// RealmEncoder.cpp

#include "ash/RealmEncoder.hpp"

#include <limits>

#include "ash/Codeword.hpp"
#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"

namespace ash {

namespace {

std::uint16_t orbit_representative_index(const Bit9State& state) {
    std::uint16_t representative = std::numeric_limits<std::uint16_t>::max();
    for (const auto& codeword : kCanonicalCodewords) {
        const auto candidate = to_int(state ^ codeword);
        if (candidate < representative) {
            representative = candidate;
        }
    }
    return representative;
}

std::uint16_t orbit_rank_for_representative(std::uint16_t representative) {
    std::uint16_t rank = 0;
    for (std::uint16_t state_index = 0; state_index < kStateSpaceSize; ++state_index) {
        if (orbit_representative_index(from_int(state_index)) == state_index) {
            if (state_index == representative) {
                return rank;
            }
            ++rank;
        }
    }
    return rank;
}

std::string canonical_orbit_id(const Bit9State& state) {
    const std::uint16_t rank = orbit_rank_for_representative(orbit_representative_index(state));
    std::string id = "APS-ORBIT-";
    if (rank < 10) {
        id += "0";
    }
    id += std::to_string(rank);
    return id;
}

}  // namespace

RealmEncodingResult RealmEncoder::encode_realm_identity(const Bit9State& state) const {
    RealmIdentity identity;
    identity.state_signature = canonical_signature(state);
    identity.realm_index = to_int(state);
    identity.realm_id = canonical_realm_id(state);
    identity.orbit_id = canonical_orbit_id(state);
    identity.b8 = b8(state);

    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::STATE_VALIDITY)
            .severity(Severity::INFO)
            .stage(Stage::CLASSIFICATION)
            .disposition(Disposition::RESOLVED)
            .subject(identity.realm_id)
            .rule_id(rule_ids::kRealm001)
            .summary("Encoded canonical realm identity")
            .note("Realm ID, orbit ID, realm index, and state signature use "
                  "the APS 1.0 canonical b0-as-most-significant bit order.")
            .build_root();

    return RealmEncodingResult{std::move(identity), std::move(diagnostic)};
}

}  // namespace ash
