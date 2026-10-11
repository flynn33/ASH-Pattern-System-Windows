#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace ash::support {

inline constexpr std::size_t max_events = 4096;
inline constexpr std::size_t max_retained_bytes = 4 * 1024 * 1024;
inline constexpr std::size_t max_event_bytes = 16 * 1024;
inline constexpr std::size_t max_export_bytes = 8 * 1024 * 1024;
inline constexpr std::size_t essential_event_reserve = 256;
inline constexpr std::size_t essential_byte_reserve = 256 * 1024;
inline constexpr char schema_version[] = "1.0";

enum class Profile { Development, Release };
enum class EventKind { Lifecycle, Operation, Decision, Fault, Health };
enum class Level { Information, Warning, Error, Critical };
enum class Classification { Public, Personal, Secret };

// Only public synthetic or deliberately reviewed non-sensitive evidence belongs
// in Public text. Classify personal/secret input at its owning boundary.
struct Text {
    std::string value;
    Classification classification{Classification::Public};
};

enum class Field {
    Trigger, Requirement, Precondition, Expected, Observed, CanonicalRule,
    CanonicalCase, CanonicalReference, SourceLocation, RecoveryAttempt,
    RecoveryOutcome, TerminalOutcome, Fixture, SupportedFinding, Hypothesis,
    UnknownEvidence, ResourceObservation
};

struct Context {
    Field field{Field::UnknownEvidence};
    Text text;
};

struct ErrorEvidence {
    Text domain;
    std::optional<std::int64_t> code;
    Text message;
    // Ordered underlying errors; no arbitrary object graph is retained.
    std::vector<Text> causes;
};

struct Event {
    EventKind kind{EventKind::Operation};
    Level level{Level::Information};
    Text component;
    Text operation_id;
    Text correlation_id;
    Text parent_operation_id;
    Text incident_id;
    Text message;
    std::optional<std::uint16_t> state_before;
    std::optional<std::uint16_t> state_after;
    std::optional<std::uint64_t> duration_ns;
    std::vector<Context> context;
    std::vector<ErrorEvidence> errors;
};

struct BuildIdentity {
    std::string product{"ASH Pattern System for Windows"};
    std::string product_version;
    std::string source_commit;
    std::optional<bool> source_dirty;
    std::string operating_system;
    std::string architecture;
    std::string execution_environment;
};

struct Health {
    std::uint64_t accepted{0};
    std::uint64_t dropped_capacity{0};
    std::uint64_t evicted{0};
    std::uint64_t rejected_invalid{0};
    std::uint64_t excluded_profile{0};
    std::uint64_t omitted_fields{0};
    std::uint64_t redacted_fields{0};
    std::uint64_t truncated_events{0};
    std::uint64_t cancelled_captures{0};
    std::uint64_t collector_failures{0};
    std::size_t retained_events{0};
    std::size_t retained_bytes{0};
    bool closed{false};
};

enum class CaptureStatus { Captured, ProfileExcluded, InvalidEvent, Capacity,
                           Cancelled, Closed, ResourceFailure };
struct CaptureResult {
    CaptureStatus status{CaptureStatus::InvalidEvent};
    std::uint64_t sequence{0};
};

struct RetainedContext { Field field; std::string value; };
struct RetainedError {
    std::string domain;
    std::optional<std::int64_t> code;
    std::string message;
    std::vector<std::string> causes;
};
struct RetainedEvent {
    std::string event_id;
    std::string utc;
    std::uint64_t sequence{0};
    std::uint64_t monotonic_ns{0};
    EventKind kind;
    Level level;
    std::string component;
    std::string operation_id;
    std::string correlation_id;
    std::string parent_operation_id;
    std::string incident_id;
    std::string message;
    std::optional<std::uint16_t> state_before;
    std::optional<std::uint16_t> state_after;
    std::optional<std::uint64_t> duration_ns;
    std::vector<RetainedContext> context;
    std::vector<RetainedError> errors;
    std::size_t bounded_bytes{0};
    std::uint64_t redactions{0};
    bool truncated{false};
};

class Session;
class Snapshot final {
public:
    Snapshot() = default;
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] std::span<const RetainedEvent> events() const noexcept;
    [[nodiscard]] const BuildIdentity& identity() const;
    [[nodiscard]] const Health& health() const;
    [[nodiscard]] const std::string& session_id() const;
    [[nodiscard]] Profile profile() const;
private:
    struct Data;
    std::shared_ptr<const Data> data_;
    friend class Session;
};

// One caller owns the session. Capture/snapshot/close may run concurrently.
// Destruction requires the caller to join all operations using this object;
// previously obtained snapshots have independent lifetime.
class Session final {
public:
    explicit Session(BuildIdentity identity = {});
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    [[nodiscard]] Profile profile() const noexcept;
    [[nodiscard]] bool accepts(EventKind kind) const noexcept;
    [[nodiscard]] CaptureResult capture(const Event& event,
        const std::atomic_bool* cancellation = nullptr) noexcept;
    template<class Builder>
    [[nodiscard]] CaptureResult capture_lazy(EventKind kind, Builder&& builder) {
        if (!accepts(kind)) {
            Event excluded;
            excluded.kind = kind;
            return capture(excluded);
        }
        return capture(std::forward<Builder>(builder)());
    }
    // May throw std::bad_alloc; no mutable semantic object/graph is retained.
    [[nodiscard]] Snapshot snapshot() const;
    [[nodiscard]] Health health() const;
    void close() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

enum class ExportError { None, InvalidSnapshot, InvalidDestination, Cancelled,
                         SizeLimit, CreateFailure, WriteFailure, RenameFailure,
                         ResourceFailure };
struct ExportResult {
    ExportError error{ExportError::InvalidSnapshot};
    std::string bundle_id;
    std::filesystem::path directory;
    std::filesystem::path json;
    std::filesystem::path markdown;
    std::filesystem::path completion_marker;
    std::uint32_t system_error{0};
};

// The destination must be an existing absolute directory, with no traversal or
// reparse points. The exporter reserves a new child directory, never overwrites
// existing files, and publishes COMPLETED.json only after both files are durable.
// Long ordinary drive/UNC destinations do not require caller long-path opt-in;
// returned bundle paths may carry the Windows extended-length namespace prefix.
// Caller-supplied device/extended namespace destinations are rejected.
// Cancellation is cooperative; the flag must outlive this call.
[[nodiscard]] ExportResult export_snapshot(const Snapshot& snapshot,
    const std::filesystem::path& destination,
    const std::atomic_bool* cancellation = nullptr) noexcept;

} // namespace ash::support
