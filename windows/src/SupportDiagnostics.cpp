#include "ash/SupportDiagnostics.hpp"
#if __has_include("ash/Version.hpp")
#include "ash/Version.hpp"
#endif
#include "support/SupportBackend.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <string_view>

namespace ash::support {
namespace {
using Clock = std::chrono::steady_clock;

bool cancelled(const std::atomic_bool* flag) noexcept {
    return flag && flag->load(std::memory_order_relaxed);
}
bool essential(EventKind kind) noexcept {
    return kind == EventKind::Fault || kind == EventKind::Health;
}
bool valid_kind(EventKind value) noexcept {
    return value >= EventKind::Lifecycle && value <= EventKind::Health;
}
bool valid_field(Field value) noexcept {
    return value >= Field::Trigger && value <= Field::ResourceObservation;
}
const char* profile_name(Profile p) noexcept {
    return p == Profile::Development ? "Development" : "Release";
}
const char* kind_name(EventKind k) noexcept {
    switch (k) {
    case EventKind::Lifecycle: return "lifecycle";
    case EventKind::Operation: return "operation";
    case EventKind::Decision: return "decision";
    case EventKind::Fault: return "fault";
    case EventKind::Health: return "health";
    }
    return "invalid";
}
const char* level_name(Level l) noexcept {
    switch (l) {
    case Level::Information: return "information";
    case Level::Warning: return "warning";
    case Level::Error: return "error";
    case Level::Critical: return "critical";
    }
    return "invalid";
}
const char* field_name(Field f) noexcept {
    switch (f) {
    case Field::Trigger: return "trigger";
    case Field::Requirement: return "requirement";
    case Field::Precondition: return "precondition";
    case Field::Expected: return "expected";
    case Field::Observed: return "observed";
    case Field::CanonicalRule: return "canonical_rule";
    case Field::CanonicalCase: return "canonical_case";
    case Field::CanonicalReference: return "canonical_reference";
    case Field::SourceLocation: return "source_location";
    case Field::RecoveryAttempt: return "recovery_attempt";
    case Field::RecoveryOutcome: return "recovery_outcome";
    case Field::TerminalOutcome: return "terminal_outcome";
    case Field::Fixture: return "fixture";
    case Field::SupportedFinding: return "supported_finding";
    case Field::Hypothesis: return "hypothesis";
    case Field::UnknownEvidence: return "unknown_evidence";
    case Field::ResourceObservation: return "resource_observation";
    }
    return "invalid";
}

std::string utc_now() {
    SYSTEMTIME t{};
    GetSystemTime(&t);
    char buffer[40]{};
    std::snprintf(buffer, sizeof(buffer), "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
        static_cast<unsigned>(t.wYear), static_cast<unsigned>(t.wMonth),
        static_cast<unsigned>(t.wDay), static_cast<unsigned>(t.wHour),
        static_cast<unsigned>(t.wMinute), static_cast<unsigned>(t.wSecond),
        static_cast<unsigned>(t.wMilliseconds));
    return buffer;
}

std::string unique_id() {
    // Identifier allocation only: no process-global event collector or store.
    static std::atomic_uint64_t serial{0};
    LARGE_INTEGER ticks{};
    QueryPerformanceCounter(&ticks);
    return std::to_string(GetCurrentProcessId()) + "-" +
        std::to_string(ticks.QuadPart) + "-" +
        std::to_string(serial.fetch_add(1, std::memory_order_relaxed));
}

std::string lower_ascii(std::string_view input) {
    std::string out(input);
    for (char& c : out) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
    return out;
}

// Explicit classifications are the primary exclusion boundary. These additional
// recognizers conservatively omit entire public fields rather than hash secrets.
std::string protect(const Text& input, std::uint64_t& redactions, bool& truncated) {
    if (input.classification != Classification::Public) {
        ++redactions;
        return "[redacted]";
    }
    const auto prefix = lower_ascii(std::string_view(input.value).substr(0, 4096));
    constexpr std::string_view secret_markers[]{"password", "authorization", "bearer ",
        "api_key", "api-key", "token=", "token:", "cookie", "private key", "ghp_",
        "github_pat_", "sk-proj-", "sk-live-", "access_token", "refresh_token"};
    constexpr std::string_view path_markers[]{"\\users\\", "/users/", "/home/", "%userprofile%"};
    for (auto marker : secret_markers) if (prefix.find(marker) != std::string::npos) {
        ++redactions;
        return "[redacted]";
    }
    for (auto marker : path_markers) if (prefix.find(marker) != std::string::npos) {
        ++redactions;
        return "[redacted]";
    }
    std::string out;
    out.reserve(256);
    for (std::size_t i = 0; i < input.value.size();) {
        const auto c = static_cast<unsigned char>(input.value[i]);
        std::size_t length = 1;
        std::uint32_t cp = c;
        if (c >= 0xC2 && c <= 0xDF) { length = 2; cp = c & 0x1Fu; }
        else if (c >= 0xE0 && c <= 0xEF) { length = 3; cp = c & 0x0Fu; }
        else if (c >= 0xF0 && c <= 0xF4) { length = 4; cp = c & 0x07u; }
        else if (c >= 0x80) { ++redactions; return "[invalid-utf8]"; }
        if (i + length > input.value.size()) { ++redactions; return "[invalid-utf8]"; }
        for (std::size_t j = 1; j < length; ++j) {
            const auto tail = static_cast<unsigned char>(input.value[i + j]);
            if ((tail & 0xC0u) != 0x80u) { ++redactions; return "[invalid-utf8]"; }
            cp = (cp << 6u) | (tail & 0x3Fu);
        }
        if ((length == 2 && cp < 0x80) || (length == 3 && cp < 0x800) ||
            (length == 4 && cp < 0x10000) || cp > 0x10FFFF ||
            (cp >= 0xD800 && cp <= 0xDFFF)) { ++redactions; return "[invalid-utf8]"; }
        std::string part;
        if (cp < 0x20 || cp == 0x7F || (cp >= 0x202A && cp <= 0x202E) ||
            (cp >= 0x2066 && cp <= 0x2069)) {
            char escaped[20]{};
            std::snprintf(escaped, sizeof(escaped), "<U+%04X>", static_cast<unsigned>(cp));
            part = escaped;
        } else part.assign(input.value, i, length);
        if (out.size() + part.size() > 256) { truncated = true; break; }
        out += part;
        i += length;
    }
    return out;
}

std::string json_string(std::string_view value) {
    std::string out{"\""};
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') { out += '\\'; out += static_cast<char>(c); }
        else if (c < 0x20) {
            char buffer[8]{};
            std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned>(c));
            out += buffer;
        } else out += static_cast<char>(c);
    }
    out += '"';
    return out;
}
std::string markdown_text(std::string_view value) {
    std::string out;
    for (unsigned char c : value) {
        switch (c) {
        case '&': case '<': case '>': case '[': case ']': case '(': case ')':
        case '#': case '*': case '_': case '|': case '`': case '\\':
        case '\n': case '\r':
            out += "&#" + std::to_string(c) + ";";
            break;
        default: out += static_cast<char>(c);
        }
    }
    return out;
}
template<class T> std::string optional_number(const std::optional<T>& n) {
    return n ? std::to_string(*n) : "null";
}
std::string event_json(const RetainedEvent& e) {
    std::string out = "{\"event_id\":" + json_string(e.event_id) +
        ",\"utc\":" + json_string(e.utc) + ",\"sequence\":" + std::to_string(e.sequence) +
        ",\"monotonic_ns\":" + std::to_string(e.monotonic_ns) +
        ",\"kind\":" + json_string(kind_name(e.kind)) + ",\"level\":" + json_string(level_name(e.level));
    const auto add = [&out](const char* name, const std::string& value) {
        out += ",\"" + std::string(name) + "\":" + json_string(value);
    };
    add("component", e.component); add("operation_id", e.operation_id);
    add("correlation_id", e.correlation_id); add("parent_operation_id", e.parent_operation_id);
    add("incident_id", e.incident_id); add("message", e.message);
    out += ",\"state_before\":" + optional_number(e.state_before) +
        ",\"state_after\":" + optional_number(e.state_after) +
        ",\"duration_ns\":" + optional_number(e.duration_ns) +
        ",\"unavailable_reason\":\"Absent values were not supplied or were excluded by profile; no stack capture\"";
    out += ",\"context\":[";
    bool first = true;
    for (const auto& f : e.context) {
        if (!first) out += ',';
        first = false;
        out += "{\"field\":" + json_string(field_name(f.field)) + ",\"value\":" + json_string(f.value) + "}";
    }
    out += "],\"errors\":["; first = true;
    for (const auto& error : e.errors) {
        if (!first) out += ',';
        first = false;
        out += "{\"domain\":" + json_string(error.domain) + ",\"code\":" + optional_number(error.code) +
            ",\"message\":" + json_string(error.message) + ",\"causes\":[";
        bool cause_first = true;
        for (const auto& cause : error.causes) {
            if (!cause_first) out += ',';
            cause_first = false; out += json_string(cause);
        }
        out += "]}";
    }
    return out + "],\"redactions\":" + std::to_string(e.redactions) +
        ",\"truncated\":" + (e.truncated ? "true" : "false") + "}";
}
std::string event_markdown(const RetainedEvent& e) {
    std::string out = "\n### Event " + std::to_string(e.sequence) + "\n\n";
    const auto line = [&out](const char* name, std::string_view value) {
        out += "- " + std::string(name) + ": " + markdown_text(value) + "\n";
    };
    line("event_id", e.event_id); line("utc", e.utc);
    line("sequence", std::to_string(e.sequence)); line("monotonic_ns", std::to_string(e.monotonic_ns));
    line("kind", kind_name(e.kind)); line("level", level_name(e.level));
    line("component", e.component); line("operation_id", e.operation_id);
    line("correlation_id", e.correlation_id); line("parent_operation_id", e.parent_operation_id);
    line("incident_id", e.incident_id); line("message", e.message);
    line("state_before", optional_number(e.state_before)); line("state_after", optional_number(e.state_after));
    line("duration_ns", optional_number(e.duration_ns));
    line("unavailable_reason", "Absent values were not supplied or were excluded by profile; no stack capture");
    for (const auto& field : e.context) line(field_name(field.field), field.value);
    for (const auto& error : e.errors) {
        line("error.domain", error.domain); line("error.code", optional_number(error.code));
        line("error.message", error.message);
        for (const auto& cause : error.causes) line("error.cause", cause);
    }
    line("redactions", std::to_string(e.redactions)); line("truncated", e.truncated ? "true" : "false");
    return out;
}
std::size_t bounded_size(const RetainedEvent& e) {
    // Reserve incident-index/framing overhead as well as both escaped views.
    return std::max(event_json(e).size(), event_markdown(e).size()) + 256;
}
void protect_again(RetainedEvent& event, std::uint64_t& redactions) {
    bool ignored = false;
    const auto check = [&](std::string& value) { value = protect(Text{value}, redactions, ignored); };
    check(event.component); check(event.operation_id); check(event.correlation_id);
    check(event.parent_operation_id); check(event.incident_id); check(event.message);
    for (auto& field : event.context) check(field.value);
    for (auto& error : event.errors) {
        check(error.domain); check(error.message);
        for (auto& cause : error.causes) check(cause);
    }
}
void protect_identity(BuildIdentity& identity, std::uint64_t& redactions) {
    bool ignored = false;
    for (auto* value : {&identity.product, &identity.product_version, &identity.source_commit,
        &identity.operating_system, &identity.architecture, &identity.execution_environment})
        *value = protect(Text{*value}, redactions, ignored);
}

std::string identity_json(const BuildIdentity& id) {
    const auto value = [](const std::string& text) { return text.empty() ? std::string("null") : json_string(text); };
    return "{\"product\":" + value(id.product) + ",\"product_version\":" + value(id.product_version) +
        ",\"source_commit\":" + value(id.source_commit) + ",\"source_dirty\":" +
        (id.source_dirty ? (*id.source_dirty ? "true" : "false") : "null") +
        ",\"operating_system\":" + value(id.operating_system) + ",\"architecture\":" + value(id.architecture) +
        ",\"execution_environment\":" + value(id.execution_environment) +
        ",\"unknown_reason\":\"Null identities were not supplied or measured; source dirty state covers tracked source at build time only\"}";
}
std::string health_json(const Health& h) {
    return "{\"accepted\":" + std::to_string(h.accepted) +
        ",\"dropped_capacity\":" + std::to_string(h.dropped_capacity) + ",\"evicted\":" + std::to_string(h.evicted) +
        ",\"rejected_invalid\":" + std::to_string(h.rejected_invalid) +
        ",\"excluded_profile\":" + std::to_string(h.excluded_profile) +
        ",\"omitted_fields\":" + std::to_string(h.omitted_fields) +
        ",\"redacted_fields\":" + std::to_string(h.redacted_fields) +
        ",\"truncated_events\":" + std::to_string(h.truncated_events) +
        ",\"cancelled_captures\":" + std::to_string(h.cancelled_captures) +
        ",\"collector_failures\":" + std::to_string(h.collector_failures) +
        ",\"retained_events\":" + std::to_string(h.retained_events) +
        ",\"retained_bytes\":" + std::to_string(h.retained_bytes) +
        ",\"closed\":" + (h.closed ? "true" : "false") + "}";
}

std::filesystem::path native_path(const std::filesystem::path& path) {
    const auto value = path.lexically_normal().make_preferred().native();
    return value.starts_with(L"\\\\")
        ? std::filesystem::path(L"\\\\?\\UNC\\" + value.substr(2))
        : std::filesystem::path(L"\\\\?\\" + value);
}

bool destination_valid(const std::filesystem::path& destination) {
    if (destination.empty() || !destination.is_absolute()) return false;
    for (const auto& part : destination) if (part == "..") return false;
    const auto normalized = destination.lexically_normal().make_preferred();
    const auto& value = normalized.native();
    // Accept ordinary filesystem destinations, not caller-supplied device namespaces.
    if (value.starts_with(L"\\\\?\\") || value.starts_with(L"\\\\.\\")) return false;
    std::error_code ec;
    if (!std::filesystem::is_directory(native_path(normalized), ec) || ec) return false;
    auto current = normalized;
    while (!current.empty()) {
        const auto attributes = GetFileAttributesW(native_path(current).c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
        auto parent = current.parent_path();
        // A UNC share is the filesystem root; its server name is not a directory.
        if (parent == current || (value.starts_with(L"\\\\") && parent == current.root_path())) break;
        current = std::move(parent);
    }
    return true;
}

ExportError write_file(const std::filesystem::path& path, std::string_view bytes,
    const std::atomic_bool* cancellation, std::uint32_t& system_error) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (file == INVALID_HANDLE_VALUE) { system_error = GetLastError(); return ExportError::CreateFailure; }
    ExportError result = ExportError::None;
    for (std::size_t offset = 0; offset < bytes.size();) {
        if (cancelled(cancellation)) { result = ExportError::Cancelled; break; }
        const auto count = static_cast<DWORD>(std::min<std::size_t>(65536, bytes.size() - offset));
        DWORD written = 0;
        if (!WriteFile(file, bytes.data() + offset, count, &written, nullptr) || written != count) {
            system_error = GetLastError(); result = ExportError::WriteFailure; break;
        }
        offset += written;
    }
    if (result == ExportError::None && !FlushFileBuffers(file)) {
        system_error = GetLastError(); result = ExportError::WriteFailure;
    }
    if (!CloseHandle(file) && result == ExportError::None) {
        system_error = GetLastError(); result = ExportError::WriteFailure;
    }
    return result;
}
std::string checksum(std::string_view bytes) {
    std::uint64_t value = 14695981039346656037ull;
    for (unsigned char c : bytes) { value ^= c; value *= 1099511628211ull; }
    char buffer[20]{};
    std::snprintf(buffer, sizeof(buffer), "%016llx", static_cast<unsigned long long>(value));
    return buffer;
}
void cleanup_bundle(const std::filesystem::path& directory) noexcept {
    try {
        std::error_code ignored;
        for (const wchar_t* name : {L"evidence.json.tmp", L"evidence.md.tmp", L"COMPLETED.json.tmp",
            L"evidence.json", L"evidence.md"}) std::filesystem::remove(directory / name, ignored);
        std::filesystem::remove(directory, ignored);
    } catch (...) {
        // Preserve the original export error. No completion marker was published;
        // an allocation or cleanup access failure may leave an incomplete bundle.
    }
}
} // namespace

std::string detail::protect_text(const Text& input, std::uint64_t& redactions, bool& truncated) {
    return protect(input, redactions, truncated);
}

struct Snapshot::Data {
    BuildIdentity identity;
    Health health;
    std::string session_id;
    Profile profile;
    std::vector<RetainedEvent> events;
};
bool Snapshot::valid() const noexcept { return static_cast<bool>(data_); }
std::span<const RetainedEvent> Snapshot::events() const noexcept {
    return data_ ? std::span<const RetainedEvent>(data_->events) : std::span<const RetainedEvent>{};
}
const BuildIdentity& Snapshot::identity() const {
    if (!data_) throw std::logic_error("Invalid support snapshot");
    return data_->identity;
}
const Health& Snapshot::health() const {
    if (!data_) throw std::logic_error("Invalid support snapshot");
    return data_->health;
}
const std::string& Snapshot::session_id() const {
    if (!data_) throw std::logic_error("Invalid support snapshot");
    return data_->session_id;
}
Profile Snapshot::profile() const {
    if (!data_) throw std::logic_error("Invalid support snapshot");
    return data_->profile;
}

struct Session::Impl {
    std::unique_ptr<detail::Backend> backend{detail::make_backend()};
    BuildIdentity identity;
    std::string id{unique_id()};
    Clock::time_point started{Clock::now()};
    mutable std::mutex mutex;
    std::deque<RetainedEvent> events;
    Health health;
    std::uint64_t next_sequence{1};
};
Session::Session(BuildIdentity identity) : impl_(std::make_unique<Impl>()) {
#if defined(ASH_SDK_VERSION)
    if (identity.product_version.empty()) identity.product_version = ASH_SDK_VERSION;
#endif
#if defined(ASH_SOURCE_COMMIT)
    if (identity.source_commit.empty()) identity.source_commit = ASH_SOURCE_COMMIT;
#endif
#if defined(ASH_SOURCE_DIRTY)
    if (!identity.source_dirty) identity.source_dirty = ASH_SOURCE_DIRTY != 0;
#endif
    if (identity.architecture.empty()) {
#if defined(_M_ARM64)
        identity.architecture = "ARM64";
#elif defined(_M_X64)
        identity.architecture = "x64";
#endif
    }
    protect_identity(identity, impl_->health.redacted_fields);
    impl_->identity = std::move(identity);
    Event start;
    start.kind = EventKind::Health;
    start.component = {"support"}; start.operation_id = {"session-start"};
    start.message = {"Caller-owned session started; capture is bounded and local"};
    (void)capture(start);
}
Session::~Session() = default;
Profile Session::profile() const noexcept { return impl_->backend->profile(); }
bool Session::accepts(EventKind kind) const noexcept {
    return valid_kind(kind) && impl_->backend->accepts(kind);
}
CaptureResult Session::capture(const Event& input, const std::atomic_bool* cancellation) noexcept {
    try {
        std::lock_guard lock(impl_->mutex);
        auto& h = impl_->health;
        if (h.closed) return {CaptureStatus::Closed, 0};
        if (cancelled(cancellation)) { ++h.cancelled_captures; return {CaptureStatus::Cancelled, 0}; }
        if (!valid_kind(input.kind) || input.level < Level::Information || input.level > Level::Critical ||
            (input.state_before && *input.state_before > 511) || (input.state_after && *input.state_after > 511)) {
            ++h.rejected_invalid; return {CaptureStatus::InvalidEvent, 0};
        }
        if (!accepts(input.kind)) { ++h.excluded_profile; return {CaptureStatus::ProfileExcluded, 0}; }
        for (const auto& field : input.context) if (!valid_field(field.field)) {
            ++h.rejected_invalid; return {CaptureStatus::InvalidEvent, 0};
        }
        RetainedEvent event{};
        event.sequence = impl_->next_sequence;
        event.event_id = impl_->id + "-event-" + std::to_string(event.sequence);
        event.utc = utc_now();
        event.monotonic_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - impl_->started).count());
        event.kind = input.kind; event.level = input.level;
        const auto keep = [&](const Text& text) { return protect(text, event.redactions, event.truncated); };
        event.component = keep(input.component); event.operation_id = keep(input.operation_id);
        event.correlation_id = keep(input.correlation_id); event.parent_operation_id = keep(input.parent_operation_id);
        event.incident_id = keep(input.incident_id); event.message = keep(input.message);
        if (input.kind == EventKind::Fault && event.incident_id.empty())
            event.incident_id = impl_->id + "-incident-" + std::to_string(event.sequence);
        impl_->backend->collect(input, event, h);
        event.bounded_bytes = bounded_size(event);
        while (event.bounded_bytes > max_event_bytes && (!event.context.empty() || !event.errors.empty())) {
            if (!event.context.empty()) event.context.pop_back(); else event.errors.pop_back();
            event.truncated = true; ++h.omitted_fields; event.bounded_bytes = bounded_size(event);
        }
        if (cancelled(cancellation)) { ++h.cancelled_captures; return {CaptureStatus::Cancelled, 0}; }
        if (event.bounded_bytes > max_event_bytes) { ++h.dropped_capacity; return {CaptureStatus::Capacity, 0}; }
        if (!essential(event.kind)) {
            std::size_t count = 0, bytes = 0;
            for (const auto& existing : impl_->events) if (!essential(existing.kind)) {
                ++count; bytes += existing.bounded_bytes;
            }
            if (count >= max_events - essential_event_reserve ||
                bytes + event.bounded_bytes > max_retained_bytes - essential_byte_reserve) {
                ++h.dropped_capacity; return {CaptureStatus::Capacity, 0};
            }
        }
        while (impl_->events.size() >= max_events || h.retained_bytes + event.bounded_bytes > max_retained_bytes) {
            const auto victim = std::find_if(impl_->events.begin(), impl_->events.end(),
                [](const RetainedEvent& e) { return !essential(e.kind); });
            if (!essential(event.kind) && victim == impl_->events.end()) {
                ++h.dropped_capacity; return {CaptureStatus::Capacity, 0};
            }
            auto selected = victim == impl_->events.end() ? impl_->events.begin() : victim;
            h.retained_bytes -= selected->bounded_bytes;
            impl_->events.erase(selected); ++h.evicted;
            h.retained_events = impl_->events.size();
        }
        const auto sequence = event.sequence;
        const auto bytes = event.bounded_bytes;
        const auto redactions = event.redactions;
        const auto was_truncated = event.truncated;
        impl_->events.push_back(std::move(event));
        ++impl_->next_sequence; ++h.accepted; h.retained_bytes += bytes;
        h.retained_events = impl_->events.size(); h.redacted_fields += redactions;
        if (was_truncated) ++h.truncated_events;
        return {CaptureStatus::Captured, sequence};
    } catch (...) {
        std::lock_guard lock(impl_->mutex);
        ++impl_->health.collector_failures;
        return {CaptureStatus::ResourceFailure, 0};
    }
}
Snapshot Session::snapshot() const {
    auto data = std::make_shared<Snapshot::Data>();
    std::lock_guard lock(impl_->mutex);
    data->identity = impl_->identity; data->health = impl_->health;
    data->session_id = impl_->id; data->profile = profile();
    data->events.assign(impl_->events.begin(), impl_->events.end());
    Snapshot result; result.data_ = std::move(data); return result;
}
Health Session::health() const {
    std::lock_guard lock(impl_->mutex); return impl_->health;
}
void Session::close() noexcept {
    std::lock_guard lock(impl_->mutex);
    impl_->health.closed = true;
}

ExportResult export_snapshot(const Snapshot& snapshot, const std::filesystem::path& destination,
    const std::atomic_bool* cancellation) noexcept {
    ExportResult result;
    bool reserved = false;
    try {
        if (!snapshot.valid()) return result;
        if (cancelled(cancellation)) { result.error = ExportError::Cancelled; return result; }
        if (!destination_valid(destination)) { result.error = ExportError::InvalidDestination; return result; }
        const auto generated = utc_now();
        result.bundle_id = snapshot.session_id() + "-export-" + unique_id();
        auto identity = snapshot.identity();
        std::uint64_t export_redactions = 0;
        protect_identity(identity, export_redactions);
        std::string json = "{\"schema_version\":" + json_string(schema_version) +
            ",\"bundle_id\":" + json_string(result.bundle_id) +
            ",\"generated_at_utc\":" + json_string(generated) +
            ",\"session_id\":" + json_string(snapshot.session_id()) +
            ",\"profile\":" + json_string(profile_name(snapshot.profile())) +
            ",\"identity\":" + identity_json(identity) + ",\"health\":" + health_json(snapshot.health()) +
            ",\"limits\":{\"events\":4096,\"retained_bytes\":4194304,\"event_bytes\":16384,\"export_bytes\":8388608}";
        const auto snapshot_events = snapshot.events();
        std::vector<RetainedEvent> events(snapshot_events.begin(), snapshot_events.end());
        for (auto& event : events) protect_again(event, export_redactions);
        const std::string start = events.empty() ? "null" : json_string(events.front().utc);
        const std::string end = events.empty() ? "null" : json_string(events.back().utc);
        json += ",\"capture_start_utc\":" + start + ",\"capture_end_utc\":" + end +
            ",\"event_count\":" + std::to_string(events.size()) + ",\"incidents\":[";
        std::string md = "# ASH support evidence\n\n";
        const auto line = [&md](const char* name, std::string_view value) {
            md += "- " + std::string(name) + ": " + markdown_text(value) + "\n";
        };
        line("schema_version", schema_version); line("bundle_id", result.bundle_id);
        line("generated_at_utc", generated); line("session_id", snapshot.session_id());
        line("profile", profile_name(snapshot.profile()));
        line("capture_start_utc", events.empty() ? "null" : events.front().utc);
        line("capture_end_utc", events.empty() ? "null" : events.back().utc);
        line("event_count", std::to_string(events.size()));
        line("identity_json", identity_json(identity)); line("health_json", health_json(snapshot.health()));
        line("limits", "4096 events; 4194304 retained bytes; 16384 bytes/event; 8388608 bytes/export file");
        md += "\n## Coverage and interpretation\n\nObserved values and caller-provided findings, hypotheses, and unknowns are separately labeled fields. "
            "Clock order is process-local ordering, not proof of causality. Capture is caller-instrumented; no automatic crash flush, stacks, "
            "ambient input, memory scan, network upload, or attachments. Export failures are returned to the caller. "
            "Release excludes operation/state/detail capture. Null values are unavailable, not zero measurements.\n\n## Incident index\n\n";
        bool first_incident = true;
        for (const auto& e : events) if (e.kind == EventKind::Fault) {
            if (!first_incident) json += ',';
            first_incident = false;
            json += "{\"incident_id\":" + json_string(e.incident_id) + ",\"trigger_event_id\":" + json_string(e.event_id) +
                ",\"first_retained_sequence\":" + std::to_string(events.front().sequence) +
                ",\"terminal_sequence\":" + std::to_string(e.sequence) +
                ",\"account_location\":\"Referenced event context/errors and correlation-filtered retained timeline\"}";
            md += "- " + markdown_text(e.incident_id) + ": event " + std::to_string(e.sequence) +
                " (" + markdown_text(e.event_id) + "); full account in retained-evidence appendix.\n";
        }
        json += "],\"events\":[";
        md += "\n## Full retained-evidence appendix\n";
        bool first = true;
        for (const auto& event : events) {
            if (cancelled(cancellation)) { result.error = ExportError::Cancelled; return result; }
            if (!first) json += ',';
            first = false;
            json += event_json(event); md += event_markdown(event);
            if (json.size() > max_export_bytes || md.size() > max_export_bytes) {
                result.error = ExportError::SizeLimit; return result;
            }
        }
        json += "],\"export_redactions\":" + std::to_string(export_redactions) +
            ",\"attachments\":[],\"limitations\":[\"Caller-instrumented coverage only\",\"No automatic crash flush or stack capture\","
            "\"Absent fields were not supplied or were excluded by profile\",\"Dropped and evicted evidence is unavailable\"]}\n";
        md += "\n## Export boundary\n\n";
        line("export_redactions", std::to_string(export_redactions));
        line("attachments", "none; attachment capture is unsupported");
        if (json.size() > max_export_bytes || md.size() > max_export_bytes) {
            result.error = ExportError::SizeLimit; return result;
        }
        result.directory = (destination / ("ash-support-" + result.bundle_id)).lexically_normal().make_preferred();
        constexpr std::wstring_view longest_suffix = L"\\COMPLETED.json.tmp";
        if (result.directory.native().size() + longest_suffix.size() >= MAX_PATH)
            result.directory = native_path(result.directory);
        std::error_code ec;
        if (!std::filesystem::create_directory(result.directory, ec) || ec) {
            result.error = ExportError::CreateFailure; result.system_error = static_cast<std::uint32_t>(ec.value()); return result;
        }
        reserved = true;
        result.json = result.directory / "evidence.json";
        result.markdown = result.directory / "evidence.md";
        result.completion_marker = result.directory / "COMPLETED.json";
        const auto temp_json = result.directory / "evidence.json.tmp";
        const auto temp_md = result.directory / "evidence.md.tmp";
        const auto temp_marker = result.directory / "COMPLETED.json.tmp";
        result.error = write_file(temp_json, json, cancellation, result.system_error);
        if (result.error == ExportError::None) result.error = write_file(temp_md, md, cancellation, result.system_error);
        if (result.error == ExportError::None && cancelled(cancellation)) result.error = ExportError::Cancelled;
        const auto rename = [&](const std::filesystem::path& from, const std::filesystem::path& to) {
            if (!MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_WRITE_THROUGH)) {
                result.system_error = GetLastError(); result.error = ExportError::RenameFailure;
            }
        };
        if (result.error == ExportError::None) rename(temp_json, result.json);
        if (result.error == ExportError::None) rename(temp_md, result.markdown);
        const std::string marker = "{\"schema_version\":\"1.0\",\"bundle_id\":" + json_string(result.bundle_id) +
            ",\"session_id\":" + json_string(snapshot.session_id()) +
            ",\"event_count\":" + std::to_string(events.size()) +
            ",\"checksum_algorithm\":\"fnv1a64-noncryptographic\",\"files\":[{\"path\":\"evidence.json\",\"bytes\":" +
            std::to_string(json.size()) + ",\"checksum\":" + json_string(checksum(json)) +
            "},{\"path\":\"evidence.md\",\"bytes\":" + std::to_string(md.size()) +
            ",\"checksum\":" + json_string(checksum(md)) + "}]}\n";
        if (result.error == ExportError::None) result.error = write_file(temp_marker, marker, cancellation, result.system_error);
        if (result.error == ExportError::None && cancelled(cancellation)) result.error = ExportError::Cancelled;
        if (result.error == ExportError::None) rename(temp_marker, result.completion_marker);
        if (result.error != ExportError::None) cleanup_bundle(result.directory);
        return result;
    } catch (...) {
        if (reserved) cleanup_bundle(result.directory);
        result.error = ExportError::ResourceFailure;
        return result;
    }
}
} // namespace ash::support
