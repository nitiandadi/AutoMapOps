#pragma once

#include "automap/validation/validation_rule.hpp"

#include <cstddef>

namespace automap::validation {

namespace basic_geometry_thresholds {

inline constexpr std::size_t minimum_polyline_points{2};
inline constexpr std::size_t minimum_closed_outline_points{4};
inline constexpr double minimum_polyline_length_m{0.1};
inline constexpr double minimum_outline_area_m2{0.01};
inline constexpr double minimum_lane_width_m{1.0};
inline constexpr double maximum_lane_width_m{20.0};
inline constexpr double maximum_absolute_horizontal_enu_m{100'000.0};
inline constexpr double maximum_absolute_vertical_enu_m{10'000.0};
inline constexpr double maximum_absolute_origin_altitude_m{10'000.0};

}  // namespace basic_geometry_thresholds

class BasicGeometryRule final : public ValidationRule {
public:
    [[nodiscard]] std::string_view id() const noexcept override;
    void validate(
        const ValidationContext& context,
        ValidationReport& report) const override;
};

}  // namespace automap::validation
