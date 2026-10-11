// Native realization of the canonical diagnostic record and causal chain.
#include "ash/Diagnostics.hpp"
#include "ash/RecoveryEngine.hpp"
#include "SemanticHash.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <stdexcept>

namespace ash {
namespace {
constexpr std::array<std::string_view, 15> kinds{"STATE_VALIDITY", "TRANSITION", "TOPOLOGY", "AXIOM", "GENERATION_PLAN", "EMISSION", "RECOVERY", "FALLBACK", "CONTAINMENT", "SAFE_HALT", "INPUT_VALIDATION", "STATE_ASSESSMENT", "CLASSIFICATION", "SCHEMA_VALIDATION", "CONFORMANCE"};
constexpr std::array<std::string_view, 4> severities{"INFO", "WARNING", "ERROR", "CRITICAL"};
constexpr std::array<std::string_view, 18> stages{"DETECTION", "CLASSIFICATION", "RECOVERY", "ESCALATION", "TERMINAL", "PARSE", "ASSESS", "CLASSIFY", "TRANSFORM", "PLAN", "EVALUATE", "EMIT", "RECOVER", "FALLBACK", "CONTAIN", "HALT", "VALIDATE", "VERIFY"};
constexpr std::array<std::string_view, 6> dispositions{"RESOLVED", "PENDING", "BLOCKED", "ESCALATED", "TERMINAL", "RECORDED"};

template <typename T, std::size_t N>
std::string_view name(T value, const std::array<std::string_view, N>& names) {
    const auto index = static_cast<std::size_t>(value);
    if (index >= N) throw std::logic_error("Diagnostic record: invalid vocabulary value");
    return names[index];
}

bool valid_utf8(std::string_view value) noexcept {
    for (std::size_t i = 0; i < value.size();) {
        const auto first = static_cast<unsigned char>(value[i++]);
        if (first < 0x80) continue;
        unsigned count = 0;
        std::uint32_t point = 0, minimum = 0;
        if (first >= 0xc2 && first <= 0xdf) { count = 1; point = first & 0x1fu; minimum = 0x80; }
        else if (first >= 0xe0 && first <= 0xef) { count = 2; point = first & 0x0fu; minimum = 0x800; }
        else if (first >= 0xf0 && first <= 0xf4) { count = 3; point = first & 0x07u; minimum = 0x10000; }
        else return false;
        if (value.size() - i < count) return false;
        for (unsigned j = 0; j < count; ++j) {
            const auto next = static_cast<unsigned char>(value[i++]);
            if ((next & 0xc0u) != 0x80u) return false;
            point = (point << 6u) | (next & 0x3fu);
        }
        if (point < minimum || point > 0x10ffff || (point >= 0xd800 && point <= 0xdfff)) return false;
    }
    return true;
}

bool meaningful(std::string_view value) noexcept {
    if (!valid_utf8(value)) return false;
    for (std::size_t i = 0; i < value.size();) {
        const auto first = static_cast<unsigned char>(value[i++]);
        std::uint32_t point = first;
        unsigned remaining = 0;
        if (first >= 0xf0) { point &= 0x07u; remaining = 3; }
        else if (first >= 0xe0) { point &= 0x0fu; remaining = 2; }
        else if (first >= 0xc0) { point &= 0x1fu; remaining = 1; }
        while (remaining--) point = (point << 6u) | (static_cast<unsigned char>(value[i++]) & 0x3fu);
        // Unicode whitespace accepted by the canonical Python str.strip().
        const bool space = (point >= 0x09 && point <= 0x0d) || (point >= 0x1c && point <= 0x20) ||
            point == 0x85 || point == 0xa0 || point == 0x1680 || (point >= 0x2000 && point <= 0x200a) ||
            point == 0x2028 || point == 0x2029 || point == 0x202f || point == 0x205f || point == 0x3000;
        if (!space) return true;
    }
    return false;
}

void string_json(std::string& output, std::string_view value) {
    if (!valid_utf8(value)) throw std::logic_error("Diagnostic record: invalid UTF-8 string");
    constexpr char hex[] = "0123456789abcdef";
    output += '"';
    for (const unsigned char c : value) {
        switch (c) {
        case '"': output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        case '\b': output += "\\b"; break;
        case '\f': output += "\\f"; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default:
            if (c < 0x20) { output += "\\u00"; output += hex[c >> 4]; output += hex[c & 15]; }
            else output += static_cast<char>(c);
        }
    }
    output += '"';
}

template <typename T>
void integer_json(std::string& output, T value) {
    char buffer[32];
    const auto result = std::to_chars(std::begin(buffer), std::end(buffer), value);
    if (result.ec != std::errc{}) throw std::logic_error("Diagnostic record: integer conversion failed");
    output.append(buffer, result.ptr);
}

// An explicit stack avoids recursive serialization stack overflow. It owns
// no content pointers beyond the synchronous serialization call.
struct Frame {
    const DiagnosticValue* value{};
    const DiagnosticObject* object{};
    const DiagnosticValue::Array* array{};
    DiagnosticObject::const_iterator next{};
    std::size_t index{};
    bool started{};
    explicit Frame(const DiagnosticValue& v) : value(&v), object(std::get_if<DiagnosticObject>(&v.value)), array(std::get_if<DiagnosticValue::Array>(&v.value)) {}
    explicit Frame(const DiagnosticObject& v) : object(&v) {}
};

void json_frames(std::string& output, std::vector<Frame> stack) {
    while (!stack.empty()) {
        auto& frame = stack.back();
        if (frame.object) {
            if (!frame.started) { output += '{'; frame.next = frame.object->begin(); frame.started = true; }
            if (frame.next == frame.object->end()) { output += '}'; stack.pop_back(); continue; }
            if (frame.index++) output += ',';
            string_json(output, frame.next->first);
            output += ':';
            const auto* child = &frame.next++->second;
            stack.emplace_back(*child);
        } else if (frame.array) {
            if (!frame.started) { output += '['; frame.started = true; }
            if (frame.index == frame.array->size()) { output += ']'; stack.pop_back(); continue; }
            if (frame.index) output += ',';
            const auto* child = &(*frame.array)[frame.index++];
            stack.emplace_back(*child);
        } else {
            if (!frame.value || frame.value->value.valueless_by_exception()) throw std::logic_error("Diagnostic record: invalid JSON value");
            const auto& value = frame.value->value;
            if (std::holds_alternative<std::nullptr_t>(value)) output += "null";
            else if (const auto* boolean = std::get_if<bool>(&value)) output += *boolean ? "true" : "false";
            else if (const auto* integer = std::get_if<std::int64_t>(&value)) integer_json(output, *integer);
            else if (const auto* text = std::get_if<std::string>(&value)) string_json(output, *text);
            else throw std::logic_error("Diagnostic record: unsupported JSON value");
            stack.pop_back();
        }
    }
}

void object_json(std::string& output, const DiagnosticObject& value) { json_frames(output, {Frame(value)}); }
void strings_json(std::string& output, const std::vector<std::string>& values) {
    output += '[';
    for (std::size_t i = 0; i < values.size(); ++i) { if (i) output += ','; string_json(output, values[i]); }
    output += ']';
}

bool taxonomy(std::string_view id) noexcept {
    if (id.size() < 9 || !id.starts_with("ASH-") || id[id.size() - 4] != '-') return false;
    for (std::size_t i = id.size() - 3; i < id.size(); ++i) if (id[i] < '0' || id[i] > '9') return false;
    auto tokens = id.substr(4, id.size() - 8);
    const auto first_dash = tokens.find('-');
    const auto family = tokens.substr(0, first_dash);
    constexpr std::array<std::string_view, 20> families{"INPUT", "STATE", "CODEWORD", "ORBIT", "REALM", "TRANSITION", "TOPOLOGY", "AXIOM", "CLASSIFICATION", "RECOVERY", "FALLBACK", "CONTAINMENT", "HALT", "GENERATION", "EMISSION", "DIAGNOSTIC", "CONFORMANCE", "RELEASE", "SECURITY", "ADMISSIBILITY"};
    if (std::find(families.begin(), families.end(), family) == families.end()) return false;
    while (!tokens.empty()) {
        const auto dash = tokens.find('-');
        const auto token = tokens.substr(0, dash);
        if (token.empty() || !std::all_of(token.begin(), token.end(), [](char c) { return c >= 'A' && c <= 'Z'; })) return false;
        if (dash == std::string_view::npos) return true;
        tokens.remove_prefix(dash + 1);
        if (tokens.empty()) return false;
    }
    return false;
}

bool identity_shape(std::string_view id) noexcept {
    if (id.size() != 33 || !id.starts_with("APS-DIAG-")) return false;
    return std::all_of(id.begin() + 9, id.end(), [](char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'); });
}

int phase(Stage stage) noexcept {
    switch (stage) {
    case Stage::DETECTION: case Stage::PARSE: return 0;
    case Stage::CLASSIFICATION: case Stage::ASSESS: case Stage::CLASSIFY: return 1;
    case Stage::RECOVERY: case Stage::RECOVER: case Stage::FALLBACK: return 2;
    case Stage::ESCALATION: case Stage::CONTAIN: return 3;
    case Stage::TERMINAL: case Stage::HALT: return 4;
    default: return -1; // Operation label, already vocabulary validated.
    }
}

bool terminal(const DiagnosticEnvelope& value) noexcept {
    return phase(value.stage) == 4 || value.diagnostic_kind == DiagnosticKind::SAFE_HALT || value.disposition == Disposition::TERMINAL;
}
} // namespace

bool Diagnostics::validate_rule_id(std::string_view id) const noexcept { return taxonomy(id); }

std::string Diagnostics::semantic_payload(const DiagnosticEnvelope& d) const {
    const auto kind = name(d.diagnostic_kind, kinds), severity = name(d.severity, severities);
    const auto stage = name(d.stage, stages), disposition = name(d.disposition, dispositions);
    if (!meaningful(d.subject_reference) || !meaningful(d.summary) || d.summary.find_first_of("\r\n") != std::string::npos || d.rule_ids.empty() || d.notes.empty())
        throw std::logic_error("Diagnostic record: missing semantic field");
    for (const auto& id : d.rule_ids) if (!taxonomy(id)) throw std::logic_error("Diagnostic record: invalid rule ID");
    for (const auto& note : d.notes) if (!meaningful(note)) throw std::logic_error("Diagnostic record: empty semantic note");
    std::string output = "{\"details\":";
    object_json(output, d.details);
    output += ",\"diagnostic_kind\":"; string_json(output, kind);
    output += ",\"disposition\":"; string_json(output, disposition);
    output += ",\"notes\":"; strings_json(output, d.notes);
    output += ",\"rule_ids\":"; strings_json(output, d.rule_ids);
    output += ",\"severity\":"; string_json(output, severity);
    output += ",\"stage\":"; string_json(output, stage);
    output += ",\"subject_reference\":"; string_json(output, d.subject_reference);
    output += ",\"summary\":"; string_json(output, d.summary);
    output += '}';
    return output;
}

std::string Diagnostics::semantic_identity(const DiagnosticEnvelope& d) const {
    auto digest = detail::sha256_hex(semantic_payload(d)).substr(0, 24);
    for (char& c : digest) if (c >= 'a' && c <= 'f') c = static_cast<char>(c - 'a' + 'A');
    return "APS-DIAG-" + digest;
}

bool Diagnostics::validate_envelope(const DiagnosticEnvelope& d) const noexcept {
    try {
        if (d.schema_version != "1.0" || d.sequence_number == 0 || d.sequence_number > static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)()) || d.chain_root_reference.expired() ||
            !identity_shape(d.diagnostic_id) || !identity_shape(d.chain_root_diagnostic_id) ||
            (!d.parent_diagnostic_id.empty() && !identity_shape(d.parent_diagnostic_id))) return false;
        if (semantic_identity(d) != d.diagnostic_id) return false;
        if (d.occurrence_metadata) { std::string checked(0, ' '); object_json(checked, *d.occurrence_metadata); }
        return true;
    } catch (...) { return false; }
}

bool Diagnostics::validate_chain(const DiagnosticEnvelope& d) const noexcept {
    try {
        const auto root_owner = d.chain_root_reference.lock();
        if (!root_owner) return false;
        // Detect pointer cycles without an allocation. A repeated ancestor in
        // this single-parent chain is necessarily a cycle.
        const DiagnosticEnvelope* slow = &d;
        const DiagnosticEnvelope* fast = &d;
        while (fast && fast->parent_diagnostic_reference) {
            slow = slow->parent_diagnostic_reference.get();
            fast = fast->parent_diagnostic_reference->parent_diagnostic_reference.get();
            if (slow == fast) return false;
        }
        // MSVC checked iterators allocate a proxy in vector's noexcept default
        // constructor. The count constructor lets that failure reach this catch.
        std::vector<const DiagnosticEnvelope*> reversed(0);
        const DiagnosticEnvelope* current = &d;
        while (current) {
            if (!validate_envelope(*current) ||
                current->chain_root_reference.lock().get() != root_owner.get() ||
                current->chain_root_diagnostic_id != root_owner->diagnostic_id) return false;
            reversed.push_back(current);
            current = current->parent_diagnostic_reference.get();
        }
        const auto* root = reversed.back();
        if (root != root_owner.get() || !root->parent_diagnostic_id.empty() || root->sequence_number != 1 ||
            root->chain_root_diagnostic_id != root->diagnostic_id) return false;
        int causal_phase = phase(root->stage);
        if (causal_phase < 0) causal_phase = 0;
        for (std::size_t i = reversed.size() - 1; i > 0; --i) {
            const auto& parent = *reversed[i];
            const auto& child = *reversed[i - 1];
            if (terminal(parent) || child.parent_diagnostic_id != parent.diagnostic_id ||
                parent.sequence_number == (std::numeric_limits<std::size_t>::max)() ||
                child.sequence_number != parent.sequence_number + 1 ||
                static_cast<unsigned>(child.severity) < static_cast<unsigned>(parent.severity)) return false;
            const int child_phase = phase(child.stage);
            if (child_phase < 0) { if (causal_phase >= 3) return false; continue; }
            if (child_phase < causal_phase) return false;
            if (child_phase == 3) {
                if (causal_phase == 3) {
                    if (child.diagnostic_kind != DiagnosticKind::CONTAINMENT) return false;
                } else if (causal_phase != 1 && causal_phase != 2) return false;
            }
            if (child_phase == 4 && causal_phase != 2 && causal_phase != 3) return false;
            causal_phase = child_phase;
        }
        return true;
    } catch (...) { return false; }
}

std::string Diagnostics::serialize_record(const DiagnosticEnvelope& d) const {
    if (!validate_chain(d)) throw std::logic_error("Diagnostic record: invalid chain");
    std::string output = "{\"chain_root_diagnostic_id\":"; string_json(output, d.chain_root_diagnostic_id);
    output += ",\"details\":"; object_json(output, d.details);
    output += ",\"diagnostic_id\":"; string_json(output, d.diagnostic_id);
    output += ",\"diagnostic_kind\":"; string_json(output, name(d.diagnostic_kind, kinds));
    output += ",\"disposition\":"; string_json(output, name(d.disposition, dispositions));
    output += ",\"notes\":"; strings_json(output, d.notes);
    output += ",\"occurrence_metadata\":";
    if (d.occurrence_metadata) object_json(output, *d.occurrence_metadata); else output += "null";
    output += ",\"parent_diagnostic_id\":";
    if (d.parent_diagnostic_id.empty()) output += "null"; else string_json(output, d.parent_diagnostic_id);
    output += ",\"rule_ids\":"; strings_json(output, d.rule_ids);
    output += ",\"schema_version\":"; string_json(output, d.schema_version);
    output += ",\"sequence_number\":"; integer_json(output, d.sequence_number);
    output += ",\"severity\":"; string_json(output, name(d.severity, severities));
    output += ",\"stage\":"; string_json(output, name(d.stage, stages));
    output += ",\"subject_reference\":"; string_json(output, d.subject_reference);
    output += ",\"summary\":"; string_json(output, d.summary);
    output += '}';
    return output;
}

std::string Diagnostics::serialize_transport(const DiagnosticEnvelope& d, std::string_view product_version) const {
    if (!meaningful(product_version)) throw std::logic_error("Diagnostic transport: product version is required");
    std::string output = "{\"data\":" + serialize_record(d);
    output += ",\"id\":"; string_json(output, d.diagnostic_id);
    output += ",\"product_version\":"; string_json(output, product_version);
    output += ",\"schema_version\":"; string_json(output, d.schema_version);
    output += '}';
    return output;
}

namespace {
template<typename Record>
std::string safety_data(const Diagnostics& diagnostics, const Record& record) {
    if (!record.original_diagnostic || !record.diagnostic || record.full_diagnostic_chain.empty() ||
        !diagnostics.validate_chain(*record.diagnostic)) throw std::logic_error("Safety record: incomplete diagnostic chain");
    const auto expected = diagnostics.expose_chain(record.diagnostic);
    if (expected != record.full_diagnostic_chain || expected.front() != record.original_diagnostic ||
        expected.back() != record.diagnostic) throw std::logic_error("Safety record: root, tail, or ancestry mismatch");
    std::string output = "{\"diagnostic\":" + diagnostics.serialize_record(*record.diagnostic);
    output += ",\"full_diagnostic_chain\":[";
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (i) output += ',';
        output += diagnostics.serialize_record(*expected[i]);
    }
    output += "]";
    return output;
}
std::string safety_transport(std::string data, const DiagnosticEnvelope& action, std::string_view version) {
    if (!meaningful(version)) throw std::logic_error("Safety transport: product version required");
    std::string output = "{\"data\":" + std::move(data);
    output += ",\"id\":"; string_json(output, action.diagnostic_id);
    output += ",\"product_version\":"; string_json(output, version);
    output += ",\"schema_version\":\"1.0\"}";
    return output;
}
}

std::string Diagnostics::serialize_transport(const ContainmentDiagnostic& d, std::string_view product_version) const {
    constexpr std::array<std::string_view, 4> triggers{"FALLBACK_FAILURE", "PROPAGATION_RISK", "OPERATOR_REQUEST", "RECOVERY_VALIDATION_FAILURE"};
    constexpr std::array<std::string_view, 4> resolutions{"PENDING", "OPERATOR_DIRECTED", "POLICY_DIRECTED", "SAFE_HALT"};
    auto output = safety_data(*this, d);
    if (d.system_state_class != SystemStateClass::CONTAINED || d.diagnostic->diagnostic_kind != DiagnosticKind::CONTAINMENT ||
        (d.diagnostic->stage != Stage::ESCALATION && d.diagnostic->stage != Stage::CONTAIN) ||
        !meaningful(d.containment_time)) throw std::logic_error("Containment record: invalid action, class, or time");
    for (const auto& operation : d.restricted_operations) if (!meaningful(operation)) throw std::logic_error("Containment record: empty restriction");
    if ((d.resolution == ContainmentResolution::PENDING) != d.awaiting_resolution)
        throw std::logic_error("Containment record: resolution contradicts pending status");
    // Keys need not be sorted for transport records; semantic payload keys remain canonical.
    output += ",\"awaiting_resolution\":"; output += d.awaiting_resolution ? "true" : "false";
    output += ",\"containment_time\":"; string_json(output, d.containment_time);
    output += ",\"original_diagnostic\":" + serialize_record(*d.original_diagnostic);
    output += ",\"resolution\":"; string_json(output, name(d.resolution, resolutions));
    output += ",\"restricted_operations\":"; strings_json(output, d.restricted_operations);
    output += ",\"system_state_class\":\"CONTAINED\",\"trigger\":"; string_json(output, name(d.trigger, triggers));
    output += '}';
    return safety_transport(std::move(output), *d.diagnostic, product_version);
}

std::string Diagnostics::serialize_transport(const SafeHaltDiagnostic& d, std::string_view product_version) const {
    constexpr std::array<std::string_view, 5> triggers{"ESCALATION_FROM_FAILED", "CONTAINMENT_BREACH", "OPERATOR_HALT_REQUEST", "POLICY_HALT_REQUEST", "UNRESOLVABLE_BLOCKED_RECOVERY"};
    auto output = safety_data(*this, d);
    if (d.system_state_class != SystemStateClass::SAFE_HALT || d.diagnostic->diagnostic_kind != DiagnosticKind::SAFE_HALT ||
        (d.diagnostic->stage != Stage::TERMINAL && d.diagnostic->stage != Stage::HALT) ||
        d.diagnostic->disposition != Disposition::TERMINAL || !d.is_terminal || !d.frozen || !meaningful(d.halt_time))
        throw std::logic_error("Safe halt record: invalid terminal action, class, time, or freeze");
    output += ",\"frozen\":true,\"halt_time\":"; string_json(output, d.halt_time);
    output += ",\"is_terminal\":true,\"original_diagnostic\":" + serialize_record(*d.original_diagnostic);
    output += ",\"system_state_class\":\"SAFE_HALT\",\"trigger\":"; string_json(output, name(d.trigger, triggers));
    output += '}';
    return safety_transport(std::move(output), *d.diagnostic, product_version);
}

std::vector<std::shared_ptr<const DiagnosticEnvelope>>
Diagnostics::expose_chain(std::shared_ptr<const DiagnosticEnvelope> leaf) const {
    std::vector<std::shared_ptr<const DiagnosticEnvelope>> result;
    if (!leaf || !validate_chain(*leaf)) return result;
    for (auto current = leaf; current; current = current->parent_diagnostic_reference) result.push_back(current);
    std::reverse(result.begin(), result.end());
    return result;
}

std::string Diagnostics::serialize_chain(std::shared_ptr<const DiagnosticEnvelope> leaf) const {
    const auto chain = expose_chain(std::move(leaf));
    if (chain.empty()) throw std::logic_error("Diagnostic chain export: missing or invalid chain");
    std::string output = "[";
    for (std::size_t i = 0; i < chain.size(); ++i) {
        if (i) output += ',';
        output += serialize_record(*chain[i]);
    }
    return output + ']';
}
} // namespace ash
