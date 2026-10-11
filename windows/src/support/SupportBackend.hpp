#pragma once

#include "ash/SupportDiagnostics.hpp"

namespace ash::support::detail {

class Backend {
public:
    virtual ~Backend() = default;
    [[nodiscard]] virtual Profile profile() const noexcept = 0;
    [[nodiscard]] virtual bool accepts(EventKind kind) const noexcept = 0;
    // Actual profile-specific collection is compiled in the selected source;
    // excluded content is never constructed behind shared runtime flags.
    virtual void collect(const Event& input, RetainedEvent& event, Health& health) const = 0;
};

[[nodiscard]] std::string protect_text(const Text& input, std::uint64_t& redactions, bool& truncated);
[[nodiscard]] std::unique_ptr<Backend> make_backend();

} // namespace ash::support::detail
