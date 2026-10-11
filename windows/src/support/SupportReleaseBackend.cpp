#include "SupportBackend.hpp"

#if !defined(NDEBUG)
#error The release support backend requires a Release product configuration.
#endif

namespace ash::support::detail {
namespace {
class ReleaseBackend final : public Backend {
public:
    Profile profile() const noexcept override { return Profile::Release; }
    bool accepts(EventKind kind) const noexcept override {
        return kind == EventKind::Fault || kind == EventKind::Health;
    }
    void collect(const Event& input, RetainedEvent& event, Health& health) const override;
};

void ReleaseBackend::collect(const Event& input, RetainedEvent& event, Health& health) const {
    health.omitted_fields += static_cast<unsigned>(input.state_before.has_value()) +
        static_cast<unsigned>(input.state_after.has_value()) + static_cast<unsigned>(input.duration_ns.has_value());
    const auto keep = [&](const Text& text) { return protect_text(text, event.redactions, event.truncated); };
    for (const auto& field : input.context) {
        switch (field.field) {
        case Field::CanonicalRule: case Field::CanonicalCase: case Field::CanonicalReference:
        case Field::TerminalOutcome: case Field::UnknownEvidence:
            if (event.context.size() >= 32) { event.truncated = true; ++health.omitted_fields; }
            else event.context.push_back({field.field, keep(field.text)});
            break;
        default: ++health.omitted_fields; break;
        }
    }
    for (const auto& error : input.errors) {
        if (event.errors.size() >= 8) { event.truncated = true; ++health.omitted_fields; continue; }
        // Essential typed faults retain domain/code. Rich messages and causes
        // are counted as unavailable without reading or constructing payloads.
        health.omitted_fields += 1 + error.causes.size();
        event.errors.push_back({keep(error.domain), error.code, {}, {}});
    }
}
}
std::unique_ptr<Backend> make_backend() {
    return std::make_unique<ReleaseBackend>();
}
} // namespace ash::support::detail
