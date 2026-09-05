#pragma once

#include "automap/core/map_data.hpp"
#include "automap/validation/validation_report.hpp"

#include <string_view>

namespace automap::validation {

struct ValidationContext {
    const core::MapData& map;
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
