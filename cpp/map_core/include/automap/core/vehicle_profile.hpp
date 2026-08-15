#pragma once

#include "automap/core/object_id.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace automap::core {

enum class VehicleType {
    passenger_car,
    delivery_van,
    truck,
};

[[nodiscard]] std::string_view vehicle_type_name(VehicleType type) noexcept;

// Dimensions and turning radius are routing constraints. Validation decides
// whether they fit a lane or restricted passage; this value type stores facts.
struct VehicleProfile final {
    ObjectId id;
    std::string name;
    VehicleType type{VehicleType::passenger_car};
    double width_m{0.0};
    std::optional<double> height_m;
    std::optional<double> length_m;
    std::optional<double> minimum_turning_radius_m;

    bool operator==(const VehicleProfile&) const = default;
};

}  // namespace automap::core
