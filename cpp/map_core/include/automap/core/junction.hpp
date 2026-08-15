#pragma once

#include "automap/core/object_id.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace automap::core {

enum class TurnDirection {
    straight,
    left,
    right,
    u_turn,
};

[[nodiscard]] std::string_view turn_direction_name(TurnDirection direction) noexcept;

// The presence of a LaneConnection declares one permitted movement through a
// junction. Geometry belongs to the referenced connecting lane.
struct LaneConnection final {
    ObjectId id;
    ObjectId junction_id;
    ObjectId incoming_lane_id;
    ObjectId connecting_lane_id;
    ObjectId outgoing_lane_id;
    TurnDirection turn_direction{TurnDirection::straight};

    bool operator==(const LaneConnection&) const = default;
};

struct Junction final {
    ObjectId id;
    std::string name;
    std::vector<ObjectId> connection_ids;

    bool operator==(const Junction&) const = default;
};

}  // namespace automap::core
