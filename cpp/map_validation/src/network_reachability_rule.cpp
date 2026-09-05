#include "automap/validation/network_reachability_rule.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace automap::validation {
namespace {

using Adjacency = std::unordered_map<std::string, std::vector<std::string>>;

[[nodiscard]] bool point_is_finite(const core::Point3d& point) noexcept {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

[[nodiscard]] double cross_2d(
    const core::Point3d& first,
    const core::Point3d& second,
    const core::Point3d& third) noexcept {
    return (second.x - first.x) * (third.y - first.y) -
           (second.y - first.y) * (third.x - first.x);
}

[[nodiscard]] int cross_sign(double value) noexcept {
    constexpr double epsilon = core::tolerance::floating_point;
    if (value > epsilon) return 1;
    if (value < -epsilon) return -1;
    return 0;
}

[[nodiscard]] bool on_segment_2d(
    const core::Point3d& first,
    const core::Point3d& second,
    const core::Point3d& point) noexcept {
    constexpr double epsilon = core::tolerance::floating_point;
    return point.x >= std::min(first.x, second.x) - epsilon &&
           point.x <= std::max(first.x, second.x) + epsilon &&
           point.y >= std::min(first.y, second.y) - epsilon &&
           point.y <= std::max(first.y, second.y) + epsilon;
}

[[nodiscard]] bool segments_intersect_2d(
    const core::Point3d& first_start,
    const core::Point3d& first_end,
    const core::Point3d& second_start,
    const core::Point3d& second_end) noexcept {
    const double first_cross_start = cross_2d(first_start, first_end, second_start);
    const double first_cross_end = cross_2d(first_start, first_end, second_end);
    const double second_cross_start = cross_2d(second_start, second_end, first_start);
    const double second_cross_end = cross_2d(second_start, second_end, first_end);
    const int first_start_sign = cross_sign(first_cross_start);
    const int first_end_sign = cross_sign(first_cross_end);
    const int second_start_sign = cross_sign(second_cross_start);
    const int second_end_sign = cross_sign(second_cross_end);

    if (first_start_sign * first_end_sign < 0 &&
        second_start_sign * second_end_sign < 0) {
        return true;
    }
    if (first_start_sign == 0 && on_segment_2d(first_start, first_end, second_start)) return true;
    if (first_end_sign == 0 && on_segment_2d(first_start, first_end, second_end)) return true;
    if (second_start_sign == 0 && on_segment_2d(second_start, second_end, first_start)) return true;
    if (second_end_sign == 0 && on_segment_2d(second_start, second_end, first_end)) return true;
    return false;
}

[[nodiscard]] bool point_in_outline_2d(
    const core::Point3d& point,
    const core::Polyline3d& outline) noexcept {
    if (!point_is_finite(point) || outline.size() < 3U) {
        return false;
    }

    bool inside = false;
    for (std::size_t current = 0U, previous = outline.size() - 1U;
         current < outline.size(); previous = current++) {
        const core::Point3d& first = outline[previous];
        const core::Point3d& second = outline[current];
        if (!point_is_finite(first) || !point_is_finite(second)) {
            return false;
        }
        if (cross_sign(cross_2d(first, second, point)) == 0 &&
            on_segment_2d(first, second, point)) {
            return true;
        }
        const bool crosses_ray = (first.y > point.y) != (second.y > point.y);
        if (crosses_ray) {
            const double intersection_x =
                first.x + (point.y - first.y) * (second.x - first.x) /
                              (second.y - first.y);
            if (intersection_x >= point.x) {
                inside = !inside;
            }
        }
    }
    return inside;
}

[[nodiscard]] bool lane_intersects_area(
    const core::Lane& lane,
    const core::RestrictedArea& area) {
    const core::Polyline3d lane_geometry = core::tessellate_path_geometry(
        lane.centerline,
        {.max_chord_error_m = 0.01, .max_segment_length_m = 1.0});
    for (const core::Point3d& point : lane_geometry) {
        if (point_in_outline_2d(point, area.outline)) {
            return true;
        }
    }
    if (lane_geometry.size() < 2U || area.outline.size() < 2U) {
        return false;
    }
    for (std::size_t lane_index = 0U;
         lane_index + 1U < lane_geometry.size(); ++lane_index) {
        const core::Point3d& lane_start = lane_geometry[lane_index];
        const core::Point3d& lane_end = lane_geometry[lane_index + 1U];
        if (!point_is_finite(lane_start) || !point_is_finite(lane_end)) {
            return false;
        }
        for (std::size_t area_index = 0U;
             area_index + 1U < area.outline.size(); ++area_index) {
            const core::Point3d& area_start = area.outline[area_index];
            const core::Point3d& area_end = area.outline[area_index + 1U];
            if (!point_is_finite(area_start) || !point_is_finite(area_end)) {
                return false;
            }
            if (segments_intersect_2d(lane_start, lane_end, area_start, area_end)) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] bool contains_id(
    const std::vector<core::ObjectId>& ids,
    std::string_view expected_id) {
    return std::find(ids.begin(), ids.end(), expected_id) != ids.end();
}

[[nodiscard]] bool lane_is_allowed(
    const core::MapData& map,
    const core::Lane& lane,
    const core::VehicleProfile& vehicle) {
    if (lane.status != core::LaneStatus::open ||
        !std::isfinite(lane.width_m) || !std::isfinite(vehicle.width_m) ||
        vehicle.width_m <= 0.0 || lane.width_m < vehicle.width_m) {
        return false;
    }

    for (const auto& area : map.restricted_areas) {
        if (area.allowed_vehicle_profile_ids.empty()) {
            continue;
        }
        if (lane_intersects_area(lane, area) &&
            !contains_id(area.allowed_vehicle_profile_ids, vehicle.id)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] Adjacency build_adjacency(const core::MapData& map) {
    Adjacency adjacency;
    for (const auto& lane : map.lanes) {
        auto& successors = adjacency[lane.id];
        successors.insert(
            successors.end(), lane.successor_ids.begin(), lane.successor_ids.end());
    }
    for (const auto& connection : map.lane_connections) {
        adjacency[connection.incoming_lane_id].push_back(connection.connecting_lane_id);
        adjacency[connection.connecting_lane_id].push_back(connection.outgoing_lane_id);
    }
    return adjacency;
}

[[nodiscard]] bool path_exists(
    const core::MapData& map,
    const Adjacency& adjacency,
    std::string_view start_lane_id,
    std::string_view target_lane_id,
    const core::VehicleProfile& vehicle) {
    const core::Lane* start_lane = map.find_lane(start_lane_id);
    const core::Lane* target_lane = map.find_lane(target_lane_id);
    if (start_lane == nullptr || target_lane == nullptr ||
        !lane_is_allowed(map, *start_lane, vehicle) ||
        !lane_is_allowed(map, *target_lane, vehicle)) {
        return false;
    }

    std::queue<std::string> frontier;
    std::unordered_set<std::string> visited;
    frontier.push(start_lane->id);
    visited.insert(start_lane->id);

    while (!frontier.empty()) {
        const std::string current_id = std::move(frontier.front());
        frontier.pop();
        if (current_id == target_lane_id) {
            return true;
        }

        const auto neighbors = adjacency.find(current_id);
        if (neighbors == adjacency.end()) {
            continue;
        }
        for (const std::string& next_id : neighbors->second) {
            if (visited.contains(next_id)) {
                continue;
            }
            const core::Lane* next_lane = map.find_lane(next_id);
            if (next_lane == nullptr || !lane_is_allowed(map, *next_lane, vehicle)) {
                continue;
            }
            visited.insert(next_id);
            frontier.push(next_id);
        }
    }
    return false;
}

[[nodiscard]] bool map_has_warehouse(const core::MapData& map) noexcept {
    return std::any_of(
        map.operational_areas.begin(), map.operational_areas.end(),
        [](const core::OperationalArea& area) {
            return area.type == core::OperationalAreaType::warehouse;
        });
}

void add_error(
    std::string_view rule_id,
    std::string_view object_id,
    std::string message,
    std::string suggestion,
    ValidationReport& report) {
    report.add_issue(ValidationIssue{
        .rule_id = std::string(rule_id),
        .severity = Severity::error,
        .object_id = std::string(object_id),
        .message = std::move(message),
        .suggestion = std::move(suggestion),
    });
}

}  // namespace

std::string_view NetworkReachabilityRule::id() const noexcept {
    return "M3_NETWORK_REACHABILITY";
}

void NetworkReachabilityRule::validate(
    const ValidationContext& context,
    ValidationReport& report) const {
    const core::MapData& map = context.map;
    if (!map_has_warehouse(map)) {
        return;
    }

    std::vector<const core::Station*> starts;
    std::vector<const core::Station*> targets;
    bool has_gate_station = false;
    bool has_loading_bay_station = false;
    for (const auto& station : map.stations) {
        if (station.type == core::StationType::gate) {
            has_gate_station = true;
            if (map.find_lane(station.access_lane_id) != nullptr) {
                starts.push_back(&station);
            }
        } else if (station.type == core::StationType::loading_bay) {
            has_loading_bay_station = true;
            if (map.find_lane(station.access_lane_id) != nullptr) {
                targets.push_back(&station);
            }
        }
    }

    if (!has_loading_bay_station) {
        add_error(
            id(), map.header.map_id.value(),
            "地图包含 Warehouse，但没有 LoadingBay Station。",
            "补充 type=loading_bay 且 accessLaneId 有效的 Station。", report);
        return;
    }
    if (!has_gate_station) {
        add_error(
            id(), map.header.map_id.value(),
            "地图包含 Warehouse，但没有 Gate Station 作为业务起点。",
            "补充 type=gate 且 accessLaneId 有效的 Station。", report);
        return;
    }
    if (targets.empty() || starts.empty()) {
        return;
    }

    const Adjacency adjacency = build_adjacency(map);
    for (const core::Station* target : targets) {
        bool reachable = false;
        for (const core::VehicleProfile& vehicle : map.vehicle_profiles) {
            for (const core::Station* start : starts) {
                if (path_exists(
                        map, adjacency, start->access_lane_id,
                        target->access_lane_id, vehicle)) {
                    reachable = true;
                    break;
                }
            }
            if (reachable) {
                break;
            }
        }

        if (!reachable) {
            add_error(
                id(), target->id,
                "LoadingBay Station '" + target->id +
                    "' 无法从任一 Gate Station 通过符合车辆约束的 Lane 路径到达。",
                "检查 Lane 前后继、开放状态、车道宽度、RestrictedArea 白名单和 VehicleProfile。",
                report);
        }
    }
}

}  // namespace automap::validation
