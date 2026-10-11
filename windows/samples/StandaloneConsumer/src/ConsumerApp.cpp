#include "ConsumerApp.hpp"

#include <ash/Version.hpp>
#include <ash/ArtifactEmitter.hpp>
#include <ash/Diagnostics.hpp>
#include <ash/GenerationPlanner.hpp>
#include <ash/RecoveryEngine.hpp>
#include <ash/StateModel.hpp>
#include <ash/SupportDiagnostics.hpp>
#include <ash/TransitionRegistry.hpp>

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

int ConsumerApp::run(const std::filesystem::path& export_directory) const {
    const ash::StateModel states;
    const ash::TransitionRegistry transitions;
    const ash::Diagnostics diagnostics;
    ash::RecoveryEngine recovery(states, transitions);
    ash::support::Session support;

    const auto state = ash::from_int(37);
    const auto normalized = states.normalize(state);
    require(normalized.state == state, "Normalization changed the realm");
    const auto resolved = transitions.resolve_transition(state, 1);
    const auto moved = transitions.apply_transition(state, resolved.codeword);
    const auto restored = transitions.apply_transition(moved.state, resolved.codeword);
    require(restored.state == state, "XOR transition was not an involution");

    const auto chain = recovery.execute_normalize_state(state);
    require(diagnostics.validate_chain(*chain.diagnostic), "Diagnostic chain validation failed");
    require(diagnostics.expose_chain(chain.diagnostic).size() >= 2, "Diagnostic ancestry missing");

    const auto rejected = recovery.execute_apply_correction(ash::from_int(0), ash::from_int(1));
    require(rejected.diagnostic->disposition != ash::Disposition::RESOLVED,
            "Invalid correction was reported as resolved");

    const ash::GenerationPlanner planner;
    const ash::TargetMetadata target{"desktop", "native-windows"};
    const auto planned = planner.generate_plan(state, target);
    const auto repeated = planner.generate_plan(state, target);
    require(planned.plan.plan_hash == repeated.plan.plan_hash, "Plan identity changed");
    const ash::ArtifactEmitter emitter;
    const auto emitted = emitter.materialize_plan(planned.plan, {"", "desktop"});
    require(emitted.artifacts.size() == planned.plan.artifact_descriptions.size(),
            "Emission lost plan elements");

    ash::support::Event incident;
    incident.kind = ash::support::EventKind::Fault;
    incident.level = ash::support::Level::Error;
    incident.component.value = "StandaloneConsumer";
    incident.operation_id.value = "invalid-correction";
    incident.correlation_id.value = "sample-controlled-failure";
    incident.incident_id.value = "sample-incident-1";
    incident.message.value = "Noncanonical correction rejected";
    incident.state_before = std::uint16_t{0};
    incident.state_after = ash::to_int(rejected.state);
    incident.context = {
        {ash::support::Field::CanonicalRule, {"ASH-CODEWORD-004"}},
        {ash::support::Field::CanonicalCase, {"SAMPLE-INVALID-CORRECTION"}},
        {ash::support::Field::Expected, {"Rejected correction, unchanged state"}},
        {ash::support::Field::Observed, {rejected.diagnostic->summary}},
        {ash::support::Field::UnknownEvidence, {"No crash stack captured for this controlled failure"}}};
    require(support.capture(incident).status == ash::support::CaptureStatus::Captured,
            "Support fault capture failed");
    support.close();
    const auto destination = std::filesystem::absolute(export_directory).lexically_normal();
    std::filesystem::create_directories(destination);
    const auto exported = ash::support::export_snapshot(support.snapshot(), destination);
    require(exported.error == ash::support::ExportError::None, "Support export failed");
    std::cout << "ASH Windows SDK " << ASH_SDK_VERSION << "\n"
              << "realm=" << ash::canonical_realm_id(state) << "\n"
              << "plan=" << planned.plan.plan_hash << "\n"
              << "descriptors=" << emitted.artifacts.size() << "\n"
              << "diagnostics=" << exported.completion_marker.string() << "\n";
    return 0;
}
