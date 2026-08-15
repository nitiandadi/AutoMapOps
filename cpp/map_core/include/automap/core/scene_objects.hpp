#pragma once

#include "automap/core/geometry.hpp"
#include "automap/core/object_id.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace automap::core {

enum class OperationalAreaType {
    unknown,
    warehouse,
    loading_area,
    parking_area,
    charging_area,
};

enum class StationType {
    unknown,
    gate,
    loading_bay,
    parking_space,
    charging_point,
    waypoint,
};

[[nodiscard]] std::string_view operational_area_type_name(
    OperationalAreaType type) noexcept;
[[nodiscard]] std::string_view station_type_name(StationType type) noexcept;

struct OperationalArea final {
    ObjectId id;
    std::string name;
    OperationalAreaType type{OperationalAreaType::unknown};
    Polyline3d outline;

    bool operator==(const OperationalArea&) const = default;
};

struct Station final {
    ObjectId id;
    std::string name;
    StationType type{StationType::unknown};
    Point3d position;
    ObjectId access_lane_id;

    bool operator==(const Station&) const = default;
};

struct RestrictedArea final {
    ObjectId id;
    std::string name;
    Polyline3d outline;
    std::vector<ObjectId> allowed_vehicle_profile_ids;

    bool operator==(const RestrictedArea&) const = default;
};

}  // namespace automap::core
