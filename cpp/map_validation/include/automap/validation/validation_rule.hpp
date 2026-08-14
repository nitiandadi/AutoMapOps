#pragma once

#include "automap/core/map_id.hpp"
#include "automap/validation/validation_report.hpp"

#include <string_view>

namespace automap::validation {

// Minimal rule context for the M1 scaffold. MapData will be added in M2.
struct ValidationContext {
    const core::MapId& map_id;
};

class ValidationRule {
public:
    virtual ~ValidationRule() = default;

    [[nodiscard]] virtual std::string_view id() const noexcept = 0;
    virtual void validate(
        const ValidationContext& context,
        ValidationReport& report) const = 0;
};

}  // namespace automap::validation
