#include "test_runner.hpp"
#include "ash/SupportDiagnostics.hpp"
#include "ash/Diagnostics.hpp"
#include "ash/TransitionRegistry.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <sddl.h>
#undef ERROR
#pragma comment(lib, "advapi32.lib")

#include <chrono>
#include <fstream>
#include <iterator>
#include <thread>

using namespace ash::support;
namespace {
std::filesystem::path evidence_directory(const char* label) {
    static std::atomic_uint64_t serial{0};
    std::wstring env(32768, L'\0');
    const auto size = GetEnvironmentVariableW(L"ASH_SUPPORT_EVIDENCE_ROOT", env.data(), static_cast<DWORD>(env.size()));
    if (size >= env.size()) throw std::runtime_error("Evidence destination environment value exceeds native limit");
    env.resize(size);
    const auto base = size ? std::filesystem::path(env) : std::filesystem::temp_directory_path();
    const auto root = std::filesystem::absolute(base) / (std::string("ash-support-test-") + label + "-" +
        std::to_string(GetCurrentProcessId()) + "-" + std::to_string(serial.fetch_add(1)));
    std::filesystem::create_directories(root);
    return root;
}
std::string read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
Event fault() {
    Event event;
    event.kind = EventKind::Fault; event.level = Level::Error;
    event.component = {"TransitionRegistry"}; event.operation_id = {"resolve-99"};
    event.correlation_id = {"controlled-transition"}; event.incident_id = {"transition-rejected"};
    event.message = {"Invalid transition rejected with unchanged canonical result"};
    event.state_before = std::uint16_t{37}; event.state_after = std::uint16_t{37};
    event.context = {{Field::Trigger, {"resolve_transition(state=37,transition_id=99)"}},
        {Field::Requirement, {"Canonical codeword set contains exactly 16 members"}},
        {Field::Expected, {"ERROR severity and BLOCKED disposition"}},
        {Field::Observed, {"ERROR severity and BLOCKED disposition"}},
        {Field::CanonicalRule, {"ASH-CODEWORD-004"}},
        {Field::CanonicalCase, {"SUPPORT-CONTROLLED-001"}},
        {Field::Fixture, {"synthetic-state-37-transition-99"}},
        {Field::RecoveryAttempt, {"No automatic recovery requested"}},
        {Field::TerminalOutcome, {"Caller receives failed canonical result"}},
        {Field::UnknownEvidence, {"No stack frames or host safety context captured"}}};
    event.errors = {{{"ASH-TRANSITION"}, 99, {"Transition id exceeds canonical codeword set"},
        {{"Observed boundary rejection; no root-cause inference"}}}};
    return event;
}
}

TEST_CASE(support_controlled_native_failure_exports_correlated_pair) {
    ash::TransitionRegistry transitions;
    const auto result = transitions.resolve_transition(ash::from_int(37), 99);
    ASSERT_TRUE(result.diagnostic != nullptr);
    ASSERT_EQ(result.diagnostic->severity, ash::Severity::ERROR);
    ASSERT_EQ(result.diagnostic->disposition, ash::Disposition::BLOCKED);
    ash::Diagnostics canonical_diagnostics;
    ASSERT_TRUE(canonical_diagnostics.validate_envelope(*result.diagnostic));
    ASSERT_TRUE(canonical_diagnostics.validate_chain(*result.diagnostic));
    auto controlled_fault = fault();
    controlled_fault.message = {result.diagnostic->summary};
    controlled_fault.errors[0].code.reset(); // the canonical result has no numeric error code
    controlled_fault.errors[0].message = {result.diagnostic->summary};
    for (auto& context : controlled_fault.context)
        if (context.field == Field::CanonicalRule) context.text = {result.diagnostic->rule_ids.front()};
    Session session;
    Event success;
    success.kind = EventKind::Operation; success.component = {"TransitionRegistry"};
    success.operation_id = {"resolve-valid"}; success.correlation_id = {"controlled-transition"};
    success.message = {"Controlled valid transition preceded rejected input"};
    const auto valid = transitions.resolve_transition(ash::from_int(37), 0);
    ASSERT_EQ(valid.diagnostic->severity, ash::Severity::INFO);
    (void)session.capture(success);
    ASSERT_EQ(session.capture(controlled_fault).status, CaptureStatus::Captured);
    const auto snapshot = session.snapshot();
    const auto output = export_snapshot(snapshot, evidence_directory("controlled"));
    ASSERT_EQ(output.error, ExportError::None);
    const auto json = read(output.json), markdown = read(output.markdown), marker = read(output.completion_marker);
    ASSERT_TRUE(json.find(output.bundle_id) != std::string::npos);
    ASSERT_TRUE(markdown.find(output.bundle_id) != std::string::npos);
    ASSERT_TRUE(marker.find(output.bundle_id) != std::string::npos);
    ASSERT_TRUE(json.find("ASH-CODEWORD-004") != std::string::npos);
    ASSERT_TRUE(markdown.find("ASH-CODEWORD-004") != std::string::npos);
    ASSERT_TRUE(json.find("SUPPORT-CONTROLLED-001") != std::string::npos);
    ASSERT_TRUE(std::filesystem::file_size(output.json) <= max_export_bytes);
    ASSERT_TRUE(std::filesystem::file_size(output.markdown) <= max_export_bytes);
    std::printf("SUPPORT_PAIR profile=%s json=%s markdown=%s marker=%s\n",
        session.profile() == Profile::Development ? "Development" : "Release",
        output.json.string().c_str(), output.markdown.string().c_str(), output.completion_marker.string().c_str());
}

TEST_CASE(support_composition_and_lazy_payload_match_native_configuration) {
    Session session;
#if defined(NDEBUG)
    ASSERT_EQ(session.profile(), Profile::Release);
#else
    ASSERT_EQ(session.profile(), Profile::Development);
#endif
    bool built = false;
    const auto capture = session.capture_lazy(EventKind::Decision, [&] {
        built = true; Event e; e.kind = EventKind::Decision; e.state_before = std::uint16_t{1}; return e;
    });
    ASSERT_EQ(built, session.profile() == Profile::Development);
    ASSERT_EQ(capture.status, built ? CaptureStatus::Captured : CaptureStatus::ProfileExcluded);
    ASSERT_EQ(session.capture(fault()).status, CaptureStatus::Captured);
    const auto snapshot = session.snapshot();
    const auto& e = snapshot.events().back();
    if (session.profile() == Profile::Release) {
        ASSERT_FALSE(e.state_before.has_value()); ASSERT_FALSE(e.state_after.has_value());
        ASSERT_TRUE(e.errors.front().message.empty()); ASSERT_TRUE(e.errors.front().causes.empty());
        for (const auto& f : e.context) ASSERT_TRUE(f.field == Field::CanonicalRule ||
            f.field == Field::CanonicalCase || f.field == Field::TerminalOutcome || f.field == Field::UnknownEvidence);
        ASSERT_TRUE(snapshot.health().omitted_fields > 0);
    } else {
        ASSERT_EQ(e.state_before.value(), 37); ASSERT_EQ(e.context.size(), static_cast<std::size_t>(10));
        ASSERT_FALSE(e.errors.front().causes.empty());
    }
}

TEST_CASE(support_privacy_excludes_secret_personal_and_nested_values_before_retention) {
    Session session;
    auto event = fault();
    event.component = {"opaque-test-secret", Classification::Secret};
    event.operation_id = {"Authorization: Bearer sentinel-authorization"};
    event.correlation_id = {"password=sentinel-password"};
    event.parent_operation_id = {"https://fixture.invalid/?api_key=sentinel-url"};
    event.message = {"C:\\Users\\PrivatePerson\\Documents\\sentinel-path"};
    event.context.push_back({Field::CanonicalReference, {"ghp_sentinel-github"}});
    event.context.push_back({Field::UnknownEvidence, {"opaque-personal", Classification::Personal}});
    event.errors[0].domain = {"cookie=sentinel-cookie"};
    event.errors[0].message = {"refresh_token=sentinel-nested"};
    event.errors[0].causes.push_back({"opaque-cause", Classification::Secret});
    ASSERT_EQ(session.capture(event).status, CaptureStatus::Captured);
    const auto snap = session.snapshot();
    const auto& e = snap.events().back();
    ASSERT_EQ(e.component, "[redacted]"); ASSERT_EQ(e.operation_id, "[redacted]");
    ASSERT_EQ(e.correlation_id, "[redacted]"); ASSERT_EQ(e.parent_operation_id, "[redacted]");
    ASSERT_EQ(e.message, "[redacted]"); ASSERT_EQ(e.errors.front().domain, "[redacted]");
    ASSERT_TRUE(snap.health().redacted_fields >= 8);
    const auto output = export_snapshot(snap, evidence_directory("privacy"));
    ASSERT_EQ(output.error, ExportError::None);
    for (const auto& content : {read(output.json), read(output.markdown)}) {
        ASSERT_TRUE(content.find("sentinel-") == std::string::npos);
        ASSERT_TRUE(content.find("opaque-") == std::string::npos);
        ASSERT_TRUE(content.find("PrivatePerson") == std::string::npos);
    }
}

TEST_CASE(support_compiled_collector_retains_only_its_profile_payload) {
    Session session;
    auto event = fault();
    event.duration_ns = 123;
    event.state_after = std::uint16_t{38};
    event.context.clear();
    for (unsigned value = 0; value <= static_cast<unsigned>(Field::ResourceObservation); ++value)
        event.context.push_back({static_cast<Field>(value), {"profile-field-" + std::to_string(value)}});
    event.errors = {{{"synthetic-fault"}, 42, {"rich-only-message", Classification::Secret},
        {{"rich-only-cause", Classification::Secret}, {"\xff"}}}};
    const auto omitted_before = session.health().omitted_fields;
    ASSERT_EQ(session.capture(event).status, CaptureStatus::Captured);
    const auto snapshot = session.snapshot();
    const auto& retained = snapshot.events().back();
    ASSERT_EQ(retained.errors.size(), std::size_t{1});
    ASSERT_EQ(retained.errors[0].domain, "synthetic-fault");
    ASSERT_EQ(retained.errors[0].code.value(), std::int64_t{42});
    if (session.profile() == Profile::Release) {
        ASSERT_EQ(retained.context.size(), std::size_t{5});
        for (const auto& field : retained.context) ASSERT_TRUE(field.field == Field::CanonicalRule ||
            field.field == Field::CanonicalCase || field.field == Field::CanonicalReference ||
            field.field == Field::TerminalOutcome || field.field == Field::UnknownEvidence);
        ASSERT_FALSE(retained.state_before.has_value()); ASSERT_FALSE(retained.state_after.has_value());
        ASSERT_FALSE(retained.duration_ns.has_value());
        ASSERT_TRUE(retained.errors[0].message.empty()); ASSERT_TRUE(retained.errors[0].causes.empty());
        ASSERT_EQ(retained.redactions, std::uint64_t{0});
        ASSERT_EQ(snapshot.health().omitted_fields - omitted_before, std::uint64_t{18});
    } else {
        ASSERT_EQ(retained.context.size(), std::size_t{17});
        ASSERT_EQ(retained.state_before.value(), std::uint16_t{37});
        ASSERT_EQ(retained.state_after.value(), std::uint16_t{38});
        ASSERT_EQ(retained.duration_ns.value(), std::uint64_t{123});
        ASSERT_EQ(retained.errors[0].message, "[redacted]");
        ASSERT_EQ(retained.errors[0].causes.size(), std::size_t{2});
        ASSERT_EQ(retained.errors[0].causes[0], "[redacted]");
        ASSERT_EQ(retained.errors[0].causes[1], "[invalid-utf8]");
        ASSERT_EQ(retained.redactions, std::uint64_t{3});
        ASSERT_EQ(snapshot.health().omitted_fields - omitted_before, std::uint64_t{0});
    }
    const auto output = export_snapshot(snapshot, evidence_directory("profile-collector"));
    ASSERT_EQ(output.error, ExportError::None);
    ASSERT_TRUE(read(output.json).find("rich-only-") == std::string::npos);
    ASSERT_TRUE(read(output.markdown).find("rich-only-") == std::string::npos);
}

TEST_CASE(support_identity_is_redacted_and_unknown_values_are_explicit) {
    BuildIdentity identity;
    identity.product = "C:\\Users\\PrivatePerson\\secret-product";
    identity.operating_system.clear(); identity.execution_environment.clear();
    Session session(identity);
    ASSERT_EQ(session.snapshot().identity().product, "[redacted]");
    auto event = fault(); event.errors[0].code.reset();
    ASSERT_EQ(session.capture(event).status, CaptureStatus::Captured);
    ASSERT_FALSE(session.snapshot().events().back().errors.front().code.has_value());
    const auto output = export_snapshot(session.snapshot(), evidence_directory("identity"));
    ASSERT_EQ(output.error, ExportError::None);
    const auto json = read(output.json);
    ASSERT_TRUE(json.find("\"operating_system\":null") != std::string::npos);
    ASSERT_TRUE(json.find("unknown_reason") != std::string::npos);
    ASSERT_TRUE(json.find("\"code\":null") != std::string::npos);
    ASSERT_TRUE(json.find("PrivatePerson") == std::string::npos);
}

TEST_CASE(support_unexpected_fields_and_invalid_state_are_rejected) {
    Session session;
    auto event = fault(); event.context.push_back({static_cast<Field>(999), {"unexpected-payload"}});
    ASSERT_EQ(session.capture(event).status, CaptureStatus::InvalidEvent);
    event = fault(); event.state_before = std::uint16_t{512};
    ASSERT_EQ(session.capture(event).status, CaptureStatus::InvalidEvent);
    event = fault(); event.kind = static_cast<EventKind>(999);
    ASSERT_EQ(session.capture(event).status, CaptureStatus::InvalidEvent);
    ASSERT_EQ(session.health().rejected_invalid, static_cast<std::uint64_t>(3));
}

TEST_CASE(support_unicode_controls_and_markdown_are_safe_data) {
    Session session;
    auto event = fault();
    event.message = {"Unicode \xE2\x98\x83 \xF0\x9F\x98\x80\n# injected [link](file) <script> & `command`\x01"};
    ASSERT_EQ(session.capture(event).status, CaptureStatus::Captured);
    const auto snap = session.snapshot();
    ASSERT_TRUE(snap.events().back().message.find('\n') == std::string::npos);
    ASSERT_TRUE(snap.events().back().message.find("<U+000A>") != std::string::npos);
    const auto output = export_snapshot(snap, evidence_directory("escaping"));
    ASSERT_EQ(output.error, ExportError::None);
    const auto md = read(output.markdown);
    ASSERT_TRUE(md.find("# injected") == std::string::npos);
    ASSERT_TRUE(md.find("[link](file)") == std::string::npos);
    ASSERT_TRUE(md.find("<script>") == std::string::npos);
    ASSERT_TRUE(md.find("\xE2\x98\x83") != std::string::npos);
    auto invalid = fault(); invalid.message = {std::string("\xC0\xAF", 2)};
    ASSERT_EQ(session.capture(invalid).status, CaptureStatus::Captured);
    ASSERT_EQ(session.snapshot().events().back().message, "[invalid-utf8]");
}

TEST_CASE(support_event_truncation_is_bounded_and_disclosed) {
    Session session;
    auto event = fault(); event.message = {std::string(100000, 'X')};
    for (int i = 0; i < 100; ++i) event.context.push_back({Field::CanonicalReference, {std::string(4096, '[')}});
    ASSERT_EQ(session.capture(event).status, CaptureStatus::Captured);
    const auto snap = session.snapshot(); const auto& e = snap.events().back();
    ASSERT_TRUE(e.message.size() <= 256); ASSERT_TRUE(e.bounded_bytes <= max_event_bytes);
    ASSERT_TRUE(e.truncated); ASSERT_TRUE(snap.health().truncated_events > 0);
    ASSERT_TRUE(snap.health().omitted_fields > 0);
    const auto output = export_snapshot(snap, evidence_directory("truncation"));
    ASSERT_EQ(output.error, ExportError::None);
}

TEST_CASE(support_event_overflow_preserves_recent_essential_faults) {
    Session session;
    auto fault_event = fault(); fault_event.incident_id = {"essential-reserved"};
    ASSERT_EQ(session.capture(fault_event).status, CaptureStatus::Captured);
    Event trace; trace.kind = EventKind::Operation; trace.component = {"synthetic"}; trace.message = {"routine"};
    for (std::size_t i = 0; i < max_events + 128; ++i) (void)session.capture(trace);
    auto snap = session.snapshot();
    ASSERT_TRUE(snap.events().size() <= max_events); ASSERT_TRUE(snap.health().retained_bytes <= max_retained_bytes);
    bool retained = false;
    for (const auto& e : snap.events()) if (e.incident_id == "essential-reserved") retained = true;
    ASSERT_TRUE(retained);
    if (session.profile() == Profile::Development) ASSERT_TRUE(snap.health().dropped_capacity > 0);
    else ASSERT_TRUE(snap.health().excluded_profile > 0);
    for (std::size_t i = 0; i < max_events + 128; ++i) ASSERT_EQ(session.capture(fault_event).status, CaptureStatus::Captured);
    snap = session.snapshot();
    ASSERT_TRUE(snap.events().size() <= max_events); ASSERT_TRUE(snap.health().retained_bytes <= max_retained_bytes);
    ASSERT_TRUE(snap.health().evicted > 0);
    ASSERT_EQ(snap.events().back().incident_id, "essential-reserved");
    const auto output = export_snapshot(snap, evidence_directory("event-overflow"));
    ASSERT_EQ(output.error, ExportError::None);
}

TEST_CASE(support_aggregate_content_bound_applies_before_export) {
    Session session;
    auto event = fault();
    for (int i = 0; i < 30; ++i) event.context.push_back({Field::CanonicalReference, {std::string(256, 'Z')}});
    for (std::size_t i = 0; i < 1000; ++i) ASSERT_EQ(session.capture(event).status, CaptureStatus::Captured);
    const auto snapshot = session.snapshot();
    ASSERT_TRUE(snapshot.health().retained_bytes <= max_retained_bytes);
    ASSERT_TRUE(snapshot.events().size() < 1000); ASSERT_TRUE(snapshot.health().evicted > 0);
    std::size_t bytes = 0;
    for (const auto& e : snapshot.events()) { ASSERT_TRUE(e.bounded_bytes <= max_event_bytes); bytes += e.bounded_bytes; }
    ASSERT_EQ(bytes, snapshot.health().retained_bytes);
    const auto output = export_snapshot(snapshot, evidence_directory("byte-overflow"));
    ASSERT_EQ(output.error, ExportError::None);
}

TEST_CASE(support_snapshot_lifetime_is_independent_of_session) {
    Snapshot snapshot;
    {
        auto session = std::make_unique<Session>();
        ASSERT_EQ(session->capture(fault()).status, CaptureStatus::Captured);
        snapshot = session->snapshot();
        session->close();
        ASSERT_EQ(session->capture(fault()).status, CaptureStatus::Closed);
        ASSERT_TRUE(session->health().closed);
    }
    ASSERT_TRUE(snapshot.valid()); ASSERT_EQ(snapshot.events().back().incident_id, "transition-rejected");
    const auto output = export_snapshot(snapshot, evidence_directory("lifetime"));
    ASSERT_EQ(output.error, ExportError::None);
}

TEST_CASE(support_cancellation_shutdown_and_invalid_destination_are_explicit) {
    Session session;
    std::atomic_bool cancellation{true};
    ASSERT_EQ(session.capture(fault(), &cancellation).status, CaptureStatus::Cancelled);
    const auto snapshot = session.snapshot(); const auto destination = evidence_directory("cancellation");
    ASSERT_EQ(export_snapshot(snapshot, destination, &cancellation).error, ExportError::Cancelled);
    ASSERT_TRUE(std::filesystem::is_empty(destination));
    ASSERT_EQ(export_snapshot(snapshot, "relative-folder").error, ExportError::InvalidDestination);
    ASSERT_EQ(export_snapshot(snapshot, destination / "missing").error, ExportError::InvalidDestination);
    ASSERT_EQ(export_snapshot(Snapshot{}, destination).error, ExportError::InvalidSnapshot);
    ASSERT_EQ(export_snapshot(snapshot, destination / ".." / destination.filename()).error, ExportError::InvalidDestination);
    session.close(); ASSERT_EQ(session.capture(fault()).status, CaptureStatus::Closed);
    ASSERT_EQ(export_snapshot(session.snapshot(), destination).error, ExportError::None);
}

TEST_CASE(support_export_does_not_overwrite_unrelated_files) {
    Session session;
    const auto destination = evidence_directory("unrelated");
    const auto unrelated = destination / "evidence.json";
    { std::ofstream out(unrelated); out << "unrelated sentinel"; }
    const auto first = export_snapshot(session.snapshot(), destination);
    const auto second = export_snapshot(session.snapshot(), destination);
    ASSERT_EQ(first.error, ExportError::None); ASSERT_EQ(second.error, ExportError::None);
    ASSERT_NE(first.directory, second.directory); ASSERT_EQ(read(unrelated), "unrelated sentinel");
}

TEST_CASE(support_export_accepts_long_ordinary_destination_and_preserves_boundary) {
    auto destination = evidence_directory("long-path");
    while (destination.native().size() < 320) destination /= std::wstring(60, L'L');
    const std::filesystem::path physical(L"\\\\?\\" + destination.native());
    std::filesystem::create_directories(physical);
    ASSERT_TRUE(destination.native().size() > MAX_PATH);
    Session session;
    ASSERT_EQ(session.capture(fault()).status, CaptureStatus::Captured);
    const auto snapshot = session.snapshot();
    std::atomic_bool cancellation{true};
    ASSERT_EQ(export_snapshot(snapshot, destination, &cancellation).error, ExportError::Cancelled);
    ASSERT_TRUE(std::filesystem::is_empty(physical));
    ASSERT_EQ(export_snapshot(snapshot, destination / ".." / destination.filename()).error,
        ExportError::InvalidDestination);
    ASSERT_EQ(export_snapshot(snapshot, physical).error, ExportError::InvalidDestination);
    const auto output = export_snapshot(snapshot, destination);
    ASSERT_EQ(output.error, ExportError::None);
    ASSERT_TRUE(output.directory.native().starts_with(L"\\\\?\\"));
    ASSERT_TRUE(std::filesystem::is_regular_file(output.completion_marker));
    ASSERT_TRUE(read(output.json).find(output.bundle_id) != std::string::npos);
    ASSERT_TRUE(read(output.markdown).find(output.bundle_id) != std::string::npos);
    ASSERT_TRUE(read(output.completion_marker).find(output.bundle_id) != std::string::npos);
    ASSERT_TRUE(std::filesystem::file_size(output.json) <= max_export_bytes);
    ASSERT_TRUE(std::filesystem::file_size(output.markdown) <= max_export_bytes);
}

TEST_CASE(support_export_rejects_read_only_destination_with_actual_io_failure) {
    Session session;
    const auto destination = evidence_directory("io-failure");
    DWORD old_size = 0;
    (void)GetFileSecurityW(destination.c_str(), DACL_SECURITY_INFORMATION, nullptr, 0, &old_size);
    ASSERT_TRUE(old_size > 0);
    std::vector<std::byte> original(old_size);
    ASSERT_TRUE(GetFileSecurityW(destination.c_str(), DACL_SECURITY_INFORMATION,
        original.data(), old_size, &old_size));
    PSECURITY_DESCRIPTOR denied = nullptr;
    ASSERT_TRUE(ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:P(D;;0x00000004;;;WD)(A;;FA;;;WD)", SDDL_REVISION_1, &denied, nullptr));
    const bool changed = SetFileSecurityW(destination.c_str(), DACL_SECURITY_INFORMATION, denied) != 0;
    LocalFree(denied);
    ASSERT_TRUE(changed);
    const auto output = export_snapshot(session.snapshot(), destination);
    const bool restored = SetFileSecurityW(destination.c_str(), DACL_SECURITY_INFORMATION, original.data()) != 0;
    ASSERT_TRUE(restored);
    ASSERT_EQ(output.error, ExportError::CreateFailure);
    ASSERT_EQ(output.system_error, static_cast<std::uint32_t>(ERROR_ACCESS_DENIED));
    ASSERT_TRUE(std::filesystem::is_empty(destination));
}

TEST_CASE(support_cancellation_during_export_leaves_no_completed_pair) {
    Session session;
    auto event = fault();
    for (int i = 0; i < 30; ++i) event.context.push_back({Field::CanonicalReference, {std::string(256, 'M')}});
    for (int i = 0; i < 600; ++i) ASSERT_EQ(session.capture(event).status, CaptureStatus::Captured);
    const auto snapshot = session.snapshot();
    const auto destination = evidence_directory("cancel-during-export");
    std::atomic_bool cancellation{false};
    std::thread canceller([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        cancellation.store(true);
    });
    const auto output = export_snapshot(snapshot, destination, &cancellation);
    canceller.join();
    ASSERT_EQ(output.error, ExportError::Cancelled);
    ASSERT_TRUE(std::filesystem::is_empty(destination));
}

TEST_CASE(support_shutdown_can_race_capture_without_mutating_old_snapshot) {
    Session session;
    const auto snapshot = session.snapshot();
    std::atomic_bool running{false};
    std::atomic_int invalid{0};
    std::thread writer([&] {
        running.store(true);
        for (int i = 0; i < 1000; ++i) {
            const auto status = session.capture(fault()).status;
            if (status != CaptureStatus::Captured && status != CaptureStatus::Closed) ++invalid;
        }
    });
    while (!running.load()) std::this_thread::yield();
    session.close(); writer.join();
    ASSERT_EQ(invalid.load(), 0); ASSERT_FALSE(snapshot.health().closed);
    ASSERT_EQ(snapshot.events().size(), static_cast<std::size_t>(1));
    ASSERT_TRUE(session.health().closed);
}

TEST_CASE(support_concurrent_capture_close_snapshot_and_exports_are_consistent) {
    Session session;
    const auto destination = evidence_directory("concurrent");
    std::atomic_int failures{0};
    std::vector<std::thread> threads;
    for (int worker = 0; worker < 4; ++worker) threads.emplace_back([&, worker] {
        for (int i = 0; i < 200; ++i) {
            auto event = fault(); event.operation_id = {"worker-" + std::to_string(worker)};
            if (session.capture(event).status != CaptureStatus::Captured) ++failures;
        }
    });
    for (int worker = 0; worker < 2; ++worker) threads.emplace_back([&] {
        for (int i = 0; i < 4; ++i) {
            const auto snap = session.snapshot();
            std::uint64_t previous = 0;
            for (const auto& event : snap.events()) {
                if (event.sequence <= previous) ++failures;
                previous = event.sequence;
            }
            if (snap.events().size() != snap.health().retained_events) ++failures;
            if (export_snapshot(snap, destination).error != ExportError::None) ++failures;
        }
    });
    for (auto& thread : threads) thread.join();
    ASSERT_EQ(failures.load(), 0);
    const auto before = session.snapshot();
    std::thread closer([&] { session.close(); });
    std::thread exporter([&] { if (export_snapshot(before, destination).error != ExportError::None) ++failures; });
    closer.join(); exporter.join(); ASSERT_EQ(failures.load(), 0);
    ASSERT_EQ(session.capture(fault()).status, CaptureStatus::Closed);
    ASSERT_TRUE(before.events().size() <= max_events);
}

TEST_CASE(support_measure_capture_snapshot_and_export_on_assigned_host) {
    Session session;
    const auto event = fault();
    constexpr int iterations = 2000;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) ASSERT_EQ(session.capture(event).status, CaptureStatus::Captured);
    const auto captured = std::chrono::steady_clock::now();
    const auto snapshot = session.snapshot();
    const auto snapped = std::chrono::steady_clock::now();
    const auto output = export_snapshot(snapshot, evidence_directory("measurement"));
    const auto exported = std::chrono::steady_clock::now();
    ASSERT_EQ(output.error, ExportError::None);
    const auto capture_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(captured - start).count();
    std::printf("METRIC support_profile=%s iterations=%d capture_total_ns=%lld ns_per_capture=%lld snapshot_ns=%lld export_ns=%lld retained_events=%zu retained_bytes=%zu\n",
        session.profile() == Profile::Development ? "Development" : "Release", iterations,
        static_cast<long long>(capture_ns), static_cast<long long>(capture_ns / iterations),
        static_cast<long long>(std::chrono::duration_cast<std::chrono::nanoseconds>(snapped - captured).count()),
        static_cast<long long>(std::chrono::duration_cast<std::chrono::nanoseconds>(exported - snapped).count()),
        snapshot.events().size(), snapshot.health().retained_bytes);
}

int main() { return ash::test::run_all(); }
