#pragma once

#include "automap/validation/validation_rule.hpp"

namespace automap::validation {

class NetworkReachabilityRule final : public ValidationRule {
public:
    [[nodiscard]] std::string_view id() const noexcept override;
    void validate(
        const ValidationContext& context,
        ValidationReport& report) const override;
};

}  // namespace automap::validation
