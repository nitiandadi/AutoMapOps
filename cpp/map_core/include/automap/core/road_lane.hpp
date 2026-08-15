#pragma once

#include "automap/core/geometry.hpp"
#include "automap/core/object_id.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace automap::core {

// Geometric side of a lane relative to the direction of Road::reference_line.
enum class LaneSide {
    left,
    right,
};

// Travel direction relative to increasing distance along Road::reference_line.
enum class LaneDirection {
    along_reference_line,
    against_reference_line,
};

enum class LaneStatus {
    open,
    closed,
};

[[nodiscard]] std::string_view lane_side_name(LaneSide side) noexcept;
[[nodiscard]] std::string_view lane_direction_name(LaneDirection direction) noexcept;
[[nodiscard]] std::string_view lane_status_name(LaneStatus status) noexcept;

struct Road final {
    ObjectId id;
    std::string name;
    Polyline3d reference_line;
    std::vector<ObjectId> predecessor_ids;
    std::vector<ObjectId> successor_ids;
    std::vector<ObjectId> lane_ids;

    bool operator==(const Road&) const = default;
};

struct Lane final {
    ObjectId id;
    ObjectId road_id;
    Polyline3d centerline;

    LaneSide side{LaneSide::right};
    std::uint32_t order_from_reference{1};

    ObjectId left_boundary_id;
    ObjectId right_boundary_id;
    std::vector<ObjectId> predecessor_ids;
    std::vector<ObjectId> successor_ids;

    LaneDirection direction{LaneDirection::along_reference_line};
    LaneStatus status{LaneStatus::open};
    double width_m{0.0};
    double speed_limit_mps{0.0};

    bool operator==(const Lane&) const = default;
};

}  // namespace automap::core
