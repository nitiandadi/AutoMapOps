#include "automap/core/road_lane.hpp"

namespace automap::core {

std::string_view lane_side_name(LaneSide side) noexcept {
    switch (side) {
    case LaneSide::left:
        return "left";
    case LaneSide::right:
        return "right";
    }

    return "unknown";
}

std::string_view lane_direction_name(LaneDirection direction) noexcept {
    switch (direction) {
    case LaneDirection::along_reference_line:
        return "along_reference_line";
    case LaneDirection::against_reference_line:
        return "against_reference_line";
    }

    return "unknown";
}

std::string_view lane_status_name(LaneStatus status) noexcept {
    switch (status) {
    case LaneStatus::open:
        return "open";
    case LaneStatus::closed:
        return "closed";
    }

    return "unknown";
}

}  // namespace automap::core
