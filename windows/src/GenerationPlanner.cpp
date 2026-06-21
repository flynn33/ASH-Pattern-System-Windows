// GenerationPlanner.cpp

#include "ash/GenerationPlanner.hpp"

#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"
#include "ash/State.hpp"

namespace ash {

namespace {

std::uint32_t rotr(std::uint32_t value, std::uint32_t bits) noexcept {
    return (value >> bits) | (value << (32u - bits));
}

std::string sha256_hex(const std::string& input) {
    static constexpr std::array<std::uint32_t, 64> k{
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u,
        0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
        0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u,
        0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
        0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
        0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
        0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au,
        0x5b9cca4fu, 0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
        0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

    std::vector<std::uint8_t> data(input.begin(), input.end());
    const std::uint64_t bit_length = static_cast<std::uint64_t>(data.size()) * 8u;
    data.push_back(0x80u);
    while ((data.size() % 64u) != 56u) {
        data.push_back(0u);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        data.push_back(static_cast<std::uint8_t>((bit_length >> shift) & 0xffu));
    }

    std::array<std::uint32_t, 8> h{
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};

    for (std::size_t chunk = 0; chunk < data.size(); chunk += 64u) {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16u; ++i) {
            const std::size_t offset = chunk + i * 4u;
            w[i] = (static_cast<std::uint32_t>(data[offset]) << 24u) |
                   (static_cast<std::uint32_t>(data[offset + 1u]) << 16u) |
                   (static_cast<std::uint32_t>(data[offset + 2u]) << 8u) |
                   static_cast<std::uint32_t>(data[offset + 3u]);
        }
        for (std::size_t i = 16u; i < 64u; ++i) {
            const std::uint32_t s0 = rotr(w[i - 15u], 7u) ^ rotr(w[i - 15u], 18u) ^ (w[i - 15u] >> 3u);
            const std::uint32_t s1 = rotr(w[i - 2u], 17u) ^ rotr(w[i - 2u], 19u) ^ (w[i - 2u] >> 10u);
            w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
        }

        std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        std::uint32_t e = h[4], f = h[5], g = h[6], hash = h[7];
        for (std::size_t i = 0; i < 64u; ++i) {
            const std::uint32_t s1 = rotr(e, 6u) ^ rotr(e, 11u) ^ rotr(e, 25u);
            const std::uint32_t ch = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = hash + s1 + ch + k[i] + w[i];
            const std::uint32_t s0 = rotr(a, 2u) ^ rotr(a, 13u) ^ rotr(a, 22u);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + maj;
            hash = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hash;
    }

    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const std::uint32_t part : h) {
        out << std::setw(8) << part;
    }
    return out.str();
}

std::string canonical_plan_payload(const GenerationPlan& plan, const std::string& runtime_hint) {
    std::ostringstream out;
    out << "source_realm_id=" << plan.source_realm_id << '\n';
    out << "source_signature=" << plan.source_signature << '\n';
    out << "target_class=" << plan.target_class << '\n';
    out << "runtime_hint=" << runtime_hint << '\n';
    out << "registry_version=" << plan.registry_version << '\n';
    for (std::size_t i = 0; i < plan.artifact_descriptions.size(); ++i) {
        out << "artifact[" << i << "]=" << plan.artifact_descriptions[i] << '\n';
    }
    for (std::size_t i = 0; i < plan.role_assignments.size(); ++i) {
        out << "role[" << i << "]=" << plan.role_assignments[i] << '\n';
    }
    return out.str();
}

}  // namespace

PlanResult GenerationPlanner::generate_plan(const Bit9State& state,
                                            const TargetMetadata& target) const {
    GenerationPlan plan;
    plan.source_signature = canonical_signature(state);
    plan.source_realm_id = canonical_realm_id(state);
    plan.target_class = target.target_class;
    plan.registry_version = "aps-windows-1.0";
    plan.artifact_descriptions = {
        "canonical-state-summary:" + plan.source_realm_id,
        "diagnostic-chain-export:" + plan.source_realm_id,
    };
    plan.role_assignments = {
        "source-state=" + plan.source_realm_id,
        "target-class=" + target.target_class,
    };
    plan.plan_hash = sha256_hex(canonical_plan_payload(plan, target.runtime_hint));

    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::GENERATION_PLAN)
            .severity(Severity::INFO)
            .stage(Stage::CLASSIFICATION)
            .disposition(Disposition::RESOLVED)
            .subject("GenerationPlanner::generate_plan[" + plan.source_realm_id + "]")
            .rule_id(rule_ids::kGeneration001)
            .rule_id(rule_ids::kGeneration002)
            .summary("Generated inspectable side-effect-free plan")
            .note("The planner produced only an in-memory GenerationPlan and "
                  "a deterministic SHA-256 plan hash; it performed no materialization.")
            .build_root();

    return PlanResult{std::move(plan), std::move(diagnostic)};
}

}  // namespace ash
