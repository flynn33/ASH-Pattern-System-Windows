// TopologyGenerator.cpp — NOT_IMPLEMENTED stub.
//
// See ash/TopologyGenerator.hpp for the canonical contract. The
// detailed branching and expansion algorithms are pending in the
// canonical R2 realignment and are not yet prescribed at a level
// that supports a faithful implementation.
//
// Branch-local: ASH-WINDOWS-STUB-001.

#include "ash/TopologyGenerator.hpp"

#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"

namespace ash {

TopologyResult TopologyGenerator::generate_topology(const Bit9State& /*state*/) const {
    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::STATE_VALIDITY)
            .severity(Severity::INFO)
            .stage(Stage::DETECTION)
            .disposition(Disposition::PENDING)
            .subject("TopologyGenerator::generate_topology")
            .rule_id(rule_ids::kWindowsStub001)
            .summary("NOT_IMPLEMENTED on windows-cpp minimal conformance slice")
            .note("TopologyGenerator is a non-slice module on windows-cpp. "
                  "Canonical branching algorithms are pending R2. See "
                  "specs/algorithms/branching-semantics.pseudo.md on main "
                  "and windows/conformance/deviation-log.md item 6.")
            .build_root();

    return TopologyResult{TopologyStructure{}, std::move(diagnostic)};
}

}  // namespace ash
