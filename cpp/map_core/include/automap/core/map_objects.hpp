#pragma once

#include "automap/core/geometry.hpp"

#include <optional>
#include <string>
#include <vector>

namespace automap::core {

using ObjectId = std::string;

enum class LaneDirection { forward, reverse };
enum class LaneStatus { open, closed };
enum class LaneBoundaryType { unknown, dashed_line, solid_line, curb };
enum class VehicleType { passenger_car, delivery_van, truck };

struct Road final {
    ObjectId id;
    std::string name;
    std::vector<ObjectId> lane_ids;
};

struct Lane final {
    ObjectId id;
    ObjectId road_id;
    Polyline3d centerline;
    ObjectId left_boundary_id;
    ObjectId right_boundary_id;
    std::vector<ObjectId> predecessor_ids;
    std::vector<ObjectId> successor_ids;
    LaneDirection direction{LaneDirection::forward};
    LaneStatus status{LaneStatus::open};
    double width_m{0.0};
    double speed_limit_mps{0.0};
};

struct LaneBoundary final {
    ObjectId id;
    Polyline3d geometry;
    LaneBoundaryType type{LaneBoundaryType::unknown};
    bool crossing_allowed{false};
};

struct LaneConnection final {
    ObjectId id;
    ObjectId incoming_lane_id;
    ObjectId connecting_lane_id;
    ObjectId outgoing_lane_id;
};

struct Junction final {
    ObjectId id;
    std::string name;
    std::vector<ObjectId> connection_ids;
};

struct OperationalArea final {
    ObjectId id;
    std::string name;
    Polyline3d outline;
};

struct Station final {
    ObjectId id;
    std::string name;
    Point3d position;
    ObjectId access_lane_id;
};

struct RestrictedArea final {
    ObjectId id;
    std::string name;
    Polyline3d outline;
    std::vector<ObjectId> allowed_vehicle_profile_ids;
};

struct VehicleProfile final {
    ObjectId id;
    std::string name;
    VehicleType type{VehicleType::passenger_car};
    double width_m{0.0};
    std::optional<double> height_m;
};

}  // namespace automap::core

