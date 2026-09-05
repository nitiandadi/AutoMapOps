#include "automap/validation/connection_geometry_rule.hpp"

#include "automap/core/geometry.hpp"

#include <cmath>
#include <iomanip>
#include <numbers>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace automap::validation {
namespace {

struct DirectedGeometry final {
    const core::PathGeometry3d& path;
    core::PathTraversal traversal{core::PathTraversal::forward};
};

[[nodiscard]] bool point_is_finite(const core::Point3d& point) noexcept {
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

[[nodiscard]] bool geometry_is_usable(const DirectedGeometry& geometry) noexcept {
    const double length = core::path_planar_length(geometry.path);
    if (!std::isfinite(length) ||
        length < connection_geometry_thresholds::minimum_heading_segment_xy_m) {
        return false;
    }
    const auto start = core::path_start_state(geometry.path, geometry.traversal);
    const auto end = core::path_end_state(geometry.path, geometry.traversal);
    return point_is_finite(start.position) && point_is_finite(end.position) &&
           std::isfinite(start.heading_rad) && std::isfinite(end.heading_rad);
}

[[nodiscard]] double heading_difference(double first, double second) noexcept {
    double difference = std::remainder(second - first, 2.0 * std::numbers::pi_v<double>);
    return std::abs(difference);
}

[[nodiscard]] std::string format_number(double value, int precision) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

void add_error(
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

void check_connection(
    std::string_view connection_type,
    std::string_view source_id,
    const DirectedGeometry& source,
    std::string_view target_id,
    const DirectedGeometry& target,
    std::string_view rule_id,
    ValidationReport& report) {
    if (!geometry_is_usable(source) || !geometry_is_usable(target)) {
        add_error(
            rule_id, source_id,
            std::string(connection_type) + " '" + std::string(source_id) + "' → '" +
                std::string(target_id) +
                "' 的路径缺少可形成 XY 切向的有效长度，无法计算连接航向。",
            "补充至少两个 XY 位置不同的有效点，或修正曲线长度和非有限坐标。",
            report);
        return;
    }

    const core::CurveState3d source_end = core::path_end_state(
        source.path, source.traversal);
    const core::CurveState3d target_start = core::path_start_state(
        target.path, target.traversal);
    const double endpoint_distance = core::distance(
        source_end.position, target_start.position);
    if (endpoint_distance > connection_geometry_thresholds::maximum_endpoint_distance_m) {
        add_error(
            rule_id, source_id,
            std::string(connection_type) + " '" + std::string(source_id) + "' → '" +
                std::string(target_id) + "' 的行驶端点距离为 " +
                format_number(endpoint_distance, 3) + " m，超过 0.500 m。",
            "移动连接端点，使前一对象的行驶终点与后一对象的行驶起点距离不超过 0.500 m。",
            report);
    }

    const double difference = heading_difference(
        source_end.heading_rad, target_start.heading_rad);
    if (difference > connection_geometry_thresholds::maximum_heading_difference_rad) {
        const double degrees = difference * 180.0 / std::numbers::pi_v<double>;
        add_error(
            rule_id, source_id,
            std::string(connection_type) + " '" + std::string(source_id) + "' → '" +
                std::string(target_id) + "' 的行驶航向差为 " +
                format_number(difference, 3) + " rad（" + format_number(degrees, 1) +
                "°），超过 0.524 rad（30°）。",
            "调整连接端附近的几何或修正拓扑关系，使行驶航向连续。", report);
    }
}

[[nodiscard]] DirectedGeometry road_geometry(const core::Road& road) noexcept {
    return DirectedGeometry{.path = road.reference_line};
}

[[nodiscard]] DirectedGeometry lane_geometry(const core::Lane& lane) noexcept {
    return DirectedGeometry{
        .path = lane.centerline,
        .traversal = lane.direction == core::LaneDirection::against_reference_line
            ? core::PathTraversal::reverse
            : core::PathTraversal::forward,
    };
}

void check_road_connections(
    const core::MapData& map,
    std::string_view rule_id,
    ValidationReport& report) {
    for (const auto& road : map.roads) {
        for (const auto& successor_id : road.successor_ids) {
            const core::Road* successor = map.find_road(successor_id);
            if (successor != nullptr) {
                check_connection(
                    "Road", road.id, road_geometry(road), successor->id,
                    road_geometry(*successor), rule_id, report);
            }
        }
    }
}

using LanePair = std::pair<std::string, std::string>;

void check_lane_pair(
    const core::MapData& map,
    std::string_view source_id,
    std::string_view target_id,
    std::set<LanePair>& checked_pairs,
    std::string_view rule_id,
    ValidationReport& report) {
    const LanePair pair{std::string(source_id), std::string(target_id)};
    if (!checked_pairs.insert(pair).second) {
        return;
    }
    const core::Lane* source = map.find_lane(source_id);
    const core::Lane* target = map.find_lane(target_id);
    if (source == nullptr || target == nullptr) {
        return;
    }
    check_connection(
        "Lane", source->id, lane_geometry(*source), target->id,
        lane_geometry(*target), rule_id, report);
}

void check_lane_connections(
    const core::MapData& map,
    std::string_view rule_id,
    ValidationReport& report) {
    std::set<LanePair> checked_pairs;
    for (const auto& lane : map.lanes) {
        for (const auto& successor_id : lane.successor_ids) {
            check_lane_pair(
                map, lane.id, successor_id, checked_pairs, rule_id, report);
        }
    }
    for (const auto& connection : map.lane_connections) {
        check_lane_pair(
            map, connection.incoming_lane_id, connection.connecting_lane_id,
            checked_pairs, rule_id, report);
        check_lane_pair(
            map, connection.connecting_lane_id, connection.outgoing_lane_id,
            checked_pairs, rule_id, report);
    }
}

}  // namespace

std::string_view ConnectionGeometryRule::id() const noexcept {
    return "M3_CONNECTION_GEOMETRY";
}

void ConnectionGeometryRule::validate(
    const ValidationContext& context,
    ValidationReport& report) const {
    check_road_connections(context.map, id(), report);
    check_lane_connections(context.map, id(), report);
}

}  // namespace automap::validation
