#pragma once

#include "automap/validation/validation_rule.hpp"

#include <numbers>

namespace automap::validation {

namespace connection_geometry_thresholds {

inline constexpr double maximum_endpoint_distance_m{0.5};
inline constexpr double maximum_polyline_heading_difference_rad{
    std::numbers::pi_v<double> / 6.0};
inline constexpr double maximum_curve_heading_difference_rad{
    std::numbers::pi_v<double> / 18.0};
inline constexpr double minimum_polyline_heading_chord_xy_m{0.01};

}  // namespace connection_geometry_thresholds

class ConnectionGeometryRule final : public ValidationRule {
public:
    [[nodiscard]] std::string_view id() const noexcept override;
    void validate(
        const ValidationContext& context,
        ValidationReport& report) const override;
};

}  // namespace automap::validation
