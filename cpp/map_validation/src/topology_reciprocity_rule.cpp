#include "automap/validation/topology_reciprocity_rule.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace automap::validation {
namespace {

[[nodiscard]] bool contains_id(
    const std::vector<core::ObjectId>& ids,
    std::string_view expected_id) {
    return std::find(ids.begin(), ids.end(), expected_id) != ids.end();
}

void add_reciprocity_error(
    std::string_view rule_id,
    std::string_view owner_id,
    std::string message,
    std::string suggestion,
    ValidationReport& report) {
    report.add_issue(ValidationIssue{
        .rule_id = std::string(rule_id),
        .severity = Severity::error,
        .object_id = std::string(owner_id),
        .message = std::move(message),
        .suggestion = std::move(suggestion),
    });
}

void check_road_topology(
    const core::MapData& map,
    std::string_view rule_id,
    ValidationReport& report) {
    for (const auto& road : map.roads) {
        for (const auto& successor_id : road.successor_ids) {
            const core::Road* successor = map.find_road(successor_id);
            if (successor != nullptr &&
                !contains_id(successor->predecessor_ids, road.id)) {
                add_reciprocity_error(
                    rule_id, road.id,
                    "Road '" + road.id + "' 将 Road '" + successor_id +
                        "' 声明为 successor，但对方的 predecessorIds 不包含 '" +
                        road.id + "'。",
                    "在 Road '" + successor_id + "' 的 predecessorIds 中补充 '" +
                        road.id + "'，或移除单向 successor 关系。",
                    report);
            }
        }
        for (const auto& predecessor_id : road.predecessor_ids) {
            const core::Road* predecessor = map.find_road(predecessor_id);
            if (predecessor != nullptr &&
                !contains_id(predecessor->successor_ids, road.id)) {
                add_reciprocity_error(
                    rule_id, road.id,
                    "Road '" + road.id + "' 将 Road '" + predecessor_id +
                        "' 声明为 predecessor，但对方的 successorIds 不包含 '" +
                        road.id + "'。",
                    "在 Road '" + predecessor_id + "' 的 successorIds 中补充 '" +
                        road.id + "'，或移除单向 predecessor 关系。",
                    report);
            }
        }
    }
}

void check_lane_topology(
    const core::MapData& map,
    std::string_view rule_id,
    ValidationReport& report) {
    for (const auto& lane : map.lanes) {
        for (const auto& successor_id : lane.successor_ids) {
            const core::Lane* successor = map.find_lane(successor_id);
            if (successor != nullptr &&
                !contains_id(successor->predecessor_ids, lane.id)) {
                add_reciprocity_error(
                    rule_id, lane.id,
                    "Lane '" + lane.id + "' 将 Lane '" + successor_id +
                        "' 声明为 successor，但对方的 predecessorIds 不包含 '" +
                        lane.id + "'。",
                    "在 Lane '" + successor_id + "' 的 predecessorIds 中补充 '" +
                        lane.id + "'，或移除单向 successor 关系。",
                    report);
            }
        }
        for (const auto& predecessor_id : lane.predecessor_ids) {
            const core::Lane* predecessor = map.find_lane(predecessor_id);
            if (predecessor != nullptr &&
                !contains_id(predecessor->successor_ids, lane.id)) {
                add_reciprocity_error(
                    rule_id, lane.id,
                    "Lane '" + lane.id + "' 将 Lane '" + predecessor_id +
                        "' 声明为 predecessor，但对方的 successorIds 不包含 '" +
                        lane.id + "'。",
                    "在 Lane '" + predecessor_id + "' 的 successorIds 中补充 '" +
                        lane.id + "'，或移除单向 predecessor 关系。",
                    report);
            }
        }
    }
}

void check_road_lane_ownership(
    const core::MapData& map,
    std::string_view rule_id,
    ValidationReport& report) {
    for (const auto& road : map.roads) {
        for (const auto& lane_id : road.lane_ids) {
            const core::Lane* lane = map.find_lane(lane_id);
            if (lane != nullptr && lane->road_id != road.id) {
                add_reciprocity_error(
                    rule_id, road.id,
                    "Road '" + road.id + "' 的 laneIds 包含 Lane '" + lane_id +
                        "'，但该 Lane 的 roadId 是 '" + lane->road_id + "'。",
                    "让 Road.laneIds 与 Lane.roadId 指向同一个所属关系。", report);
            }
        }
    }

    for (const auto& lane : map.lanes) {
        const core::Road* road = map.find_road(lane.road_id);
        if (road != nullptr && !contains_id(road->lane_ids, lane.id)) {
            add_reciprocity_error(
                rule_id, lane.id,
                "Lane '" + lane.id + "' 的 roadId 指向 Road '" + lane.road_id +
                    "'，但该 Road 的 laneIds 不包含此 Lane。",
                "在 Road '" + lane.road_id + "' 的 laneIds 中补充 '" + lane.id +
                    "'，或修正 Lane.roadId。",
                report);
        }
    }
}

void check_junction_connection_ownership(
    const core::MapData& map,
    std::string_view rule_id,
    ValidationReport& report) {
    for (const auto& junction : map.junctions) {
        for (const auto& connection_id : junction.connection_ids) {
            const core::LaneConnection* connection =
                map.find_lane_connection(connection_id);
            if (connection != nullptr && connection->junction_id != junction.id) {
                add_reciprocity_error(
                    rule_id, junction.id,
                    "Junction '" + junction.id +
                        "' 的 connectionIds 包含 LaneConnection '" + connection_id +
                        "'，但该连接的 junctionId 是 '" + connection->junction_id + "'。",
                    "让 Junction.connectionIds 与 LaneConnection.junctionId 指向同一个所属关系。",
                    report);
            }
        }
    }

    for (const auto& connection : map.lane_connections) {
        const core::Junction* junction = map.find_junction(connection.junction_id);
        if (junction != nullptr &&
            !contains_id(junction->connection_ids, connection.id)) {
            add_reciprocity_error(
                rule_id, connection.id,
                "LaneConnection '" + connection.id +
                    "' 的 junctionId 指向 Junction '" + connection.junction_id +
                    "'，但该 Junction 的 connectionIds 不包含此连接。",
                "在 Junction '" + connection.junction_id +
                    "' 的 connectionIds 中补充 '" + connection.id +
                    "'，或修正 LaneConnection.junctionId。",
                report);
        }
    }
}

}  // namespace

std::string_view TopologyReciprocityRule::id() const noexcept {
    return "M3_TOPOLOGY_RECIPROCITY";
}

void TopologyReciprocityRule::validate(
    const ValidationContext& context,
    ValidationReport& report) const {
    const core::MapData& map = context.map;
    check_road_topology(map, id(), report);
    check_lane_topology(map, id(), report);
    check_road_lane_ownership(map, id(), report);
    check_junction_connection_ownership(map, id(), report);
}

}  // namespace automap::validation
