// RealmEncoder.cpp — NOT_IMPLEMENTED stub.
//
// See ash/RealmEncoder.hpp for the canonical contract this module
// will satisfy in a future expansion pass. The current branch ships
// a stub that returns a schema-conformant NOT_IMPLEMENTED diagnostic.
//
// Branch-local: ASH-WINDOWS-STUB-001. See
// windows/conformance/deviation-log.md item 6.

#include "ash/RealmEncoder.hpp"

#include "ash/DiagnosticEnvelope.hpp"
#include "ash/RuleIds.hpp"

namespace ash {

RealmEncodingResult RealmEncoder::encode_realm_identity(const Bit9State& /*state*/) const {
    auto diagnostic =
        DiagnosticEnvelopeBuilder{}
            .kind(DiagnosticKind::STATE_VALIDITY)
            .severity(Severity::INFO)
            .stage(Stage::DETECTION)
            .disposition(Disposition::PENDING)
            .subject("RealmEncoder::encode_realm_identity")
            .rule_id(rule_ids::kWindowsStub001)
            .summary("NOT_IMPLEMENTED on windows-cpp minimal conformance slice")
            .note("RealmEncoder is a non-slice module on windows-cpp. See "
                  "windows/conformance/deviation-log.md item 6 and the "
                  "canonical contract at specs/interfaces/contracts/"
                  "realm-encoder-contract.md on main.")
            .build_root();

    return RealmEncodingResult{RealmIdentity{}, std::move(diagnostic)};
}

}  // namespace ash
