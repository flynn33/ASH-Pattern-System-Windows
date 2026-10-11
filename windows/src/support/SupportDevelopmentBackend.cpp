#include "SupportBackend.hpp"

#if defined(NDEBUG)
#error The development support backend cannot be compiled into a Release product.
#endif

namespace ash::support::detail {
namespace {
class DevelopmentBackend final : public Backend {
public:
    Profile profile() const noexcept override { return Profile::Development; }
    bool accepts(EventKind) const noexcept override { return true; }
    void collect(const Event& input, RetainedEvent& event, Health& health) const override;
};

void DevelopmentBackend::collect(const Event& input, RetainedEvent& event, Health& health) const {
    event.state_before = input.state_before;
    event.state_after = input.state_after;
    event.duration_ns = input.duration_ns;
    const auto keep = [&](const Text& text) { return protect_text(text, event.redactions, event.truncated); };
    for (const auto& field : input.context) {
        if (event.context.size() >= 32) { event.truncated = true; ++health.omitted_fields; continue; }
        event.context.push_back({field.field, keep(field.text)});
    }
    for (const auto& error : input.errors) {
        if (event.errors.size() >= 8) { event.truncated = true; ++health.omitted_fields; continue; }
        RetainedError retained{keep(error.domain), error.code, keep(error.message), {}};
        for (const auto& cause : error.causes) {
            if (retained.causes.size() >= 8) { event.truncated = true; ++health.omitted_fields; continue; }
            retained.causes.push_back(keep(cause));
        }
        event.errors.push_back(std::move(retained));
    }
}
}
std::unique_ptr<Backend> make_backend() {
    return std::make_unique<DevelopmentBackend>();
}
} // namespace ash::support::detail
