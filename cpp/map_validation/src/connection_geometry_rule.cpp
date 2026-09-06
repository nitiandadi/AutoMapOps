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

struct ConnectionEndState final {
    core::Point3d position;
    double heading_rad{0.0};
};

[[nodiscard]] bool point_is_finite(const core::Point3d& point) noexcept {
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

[[nodiscard]] double horizontal_distance(
    const core::Point3d& first,
    const core::Point3d& second) noexcept {
    return std::hypot(second.x - first.x, second.y - first.y);
}

[[nodiscard]] std::optional<ConnectionEndState> polyline_end_state(
    const core::Polyline3d& polyline,
    core::PathTraversal traversal,
    bool at_start) noexcept {
    if (polyline.empty()) {
        return std::nullopt;
    }

    const bool use_front =
        (traversal == core::PathTraversal::forward) == at_start;
    const core::Point3d& endpoint = use_front ? polyline.front() : polyline.back();
    if (!point_is_finite(endpoint)) {
        return std::nullopt;
    }

    for (std::size_t offset = 1U; offset < polyline.size(); ++offset) {
        const std::size_t index = use_front ? offset : polyline.size() - 1U - offset;
        const core::Point3d& candidate = polyline[index];
        if (!point_is_finite(candidate)) {
            return std::nullopt;
        }
        if (horizontal_distance(endpoint, candidate) <
            connection_geometry_thresholds::minimum_polyline_heading_chord_xy_m) {
            continue;
        }

        const core::Point3d& from = at_start ? endpoint : candidate;
        const core::Point3d& to = at_start ? candidate : endpoint;
        return ConnectionEndState{
            .position = endpoint,
            .heading_rad = std::atan2(to.y - from.y, to.x - from.x),
        };
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<ConnectionEndState> connection_end_state(
    const DirectedGeometry& geometry,
    bool at_start) noexcept {
    if (const core::Polyline3d* polyline = geometry.path.polyline()) {
        return polyline_end_state(*polyline, geometry.traversal, at_start);
    }

    const core::CompositeCurve3d* curve = geometry.path.composite_curve();
    if (curve == nullptr || curve->segments.empty()) {
        return std::nullopt;
    }
    const double length = core::path_planar_length(geometry.path);
    if (!std::isfinite(length) || length <= 0.0) {
        return std::nullopt;
    }
    const core::CurveState3d state = at_start
        ? core::path_start_state(geometry.path, geometry.traversal)
        : core::path_end_state(geometry.path, geometry.traversal);
    if (!point_is_finite(state.position) || !std::isfinite(state.heading_rad)) {
        return std::nullopt;
    }
    return ConnectionEndState{
        .position = state.position,
        .heading_rad = state.heading_rad,
    };
}

[[nodiscard]] double maximum_heading_difference(
    const DirectedGeometry& source,
    const DirectedGeometry& target) noexcept {
    return source.path.is_composite_curve() && target.path.is_composite_curve()
        ? connection_geometry_thresholds::maximum_curve_heading_difference_rad
        : connection_geometry_thresholds::maximum_polyline_heading_difference_rad;
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
    const auto source_end = connection_end_state(source, false);
    const auto target_start = connection_end_state(target, true);
    if (!source_end || !target_start) {
        add_error(
            rule_id, source_id,
            std::string(connection_type) + " '" + std::string(source_id) + "' → '" +
                std::string(target_id) +
                "' 的路径缺少可计算的 XY 连接切线，无法计算连接航向。",
            "点列路径应提供距连接端至少 0.01 m 的有效 XY 点；连续曲线应修正曲线段和非有限参数。",
            report);
        return;
    }

    const double endpoint_distance = core::distance(
        source_end->position, target_start->position);
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
        source_end->heading_rad, target_start->heading_rad);
    const double maximum_difference = maximum_heading_difference(source, target);
    if (difference > maximum_difference) {
        const double degrees = difference * 180.0 / std::numbers::pi_v<double>;
        const double maximum_degrees =
            maximum_difference * 180.0 / std::numbers::pi_v<double>;
        add_error(
            rule_id, source_id,
            std::string(connection_type) + " '" + std::string(source_id) + "' → '" +
                std::string(target_id) + "' 的行驶航向差为 " +
                format_number(difference, 3) + " rad（" + format_number(degrees, 1) +
                "°），超过 " + format_number(maximum_difference, 3) + " rad（" +
                format_number(maximum_degrees, 1) + "°）。",
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
