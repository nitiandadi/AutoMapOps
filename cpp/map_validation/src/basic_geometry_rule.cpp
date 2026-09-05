#include "automap/validation/basic_geometry_rule.hpp"

#include "automap/core/geometry.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace automap::validation {
namespace {

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

void add_warning(
    std::string_view rule_id,
    std::string_view object_id,
    std::string message,
    std::string suggestion,
    ValidationReport& report) {
    report.add_issue(ValidationIssue{
        .rule_id = std::string(rule_id),
        .severity = Severity::warning,
        .object_id = std::string(object_id),
        .message = std::move(message),
        .suggestion = std::move(suggestion),
    });
}

[[nodiscard]] bool point_is_finite(const core::Point3d& point) noexcept {
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

void check_point(
    const core::Point3d& point,
    std::size_t point_index,
    std::string_view object_type,
    std::string_view object_id,
    std::string_view geometry_field,
    std::string_view rule_id,
    ValidationReport& report) {
    const std::string point_name = std::string(geometry_field) + "[" +
                                   std::to_string(point_index) + "]";
    if (!point_is_finite(point)) {
        add_error(
            rule_id, object_id,
            std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                point_name + " 包含 NaN 或无穷值。",
            "将该点改为有限的局部 ENU 坐标。", report);
        return;
    }

    if (std::abs(point.x) > basic_geometry_thresholds::maximum_absolute_horizontal_enu_m ||
        std::abs(point.y) > basic_geometry_thresholds::maximum_absolute_horizontal_enu_m ||
        std::abs(point.z) > basic_geometry_thresholds::maximum_absolute_vertical_enu_m) {
        add_error(
            rule_id, object_id,
            std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                point_name + " 超出局部 ENU 允许范围。",
            "确认坐标已经转换为当前地图原点下的 ENU 米制坐标；水平范围应在 ±100 km，Z 应在 ±10 km。",
            report);
    }
}

[[nodiscard]] double cross_2d(
    const core::Point3d& first,
    const core::Point3d& second,
    const core::Point3d& third) noexcept {
    return (second.x - first.x) * (third.y - first.y) -
           (second.y - first.y) * (third.x - first.x);
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

[[nodiscard]] int cross_sign(double value) noexcept {
    constexpr double epsilon = core::tolerance::floating_point;
    if (value > epsilon) return 1;
    if (value < -epsilon) return -1;
    return 0;
}

[[nodiscard]] bool segments_intersect_2d(
    const core::Point3d& first_start,
    const core::Point3d& first_end,
    const core::Point3d& second_start,
    const core::Point3d& second_end) noexcept {
    const double cross_first_start = cross_2d(first_start, first_end, second_start);
    const double cross_first_end = cross_2d(first_start, first_end, second_end);
    const double cross_second_start = cross_2d(second_start, second_end, first_start);
    const double cross_second_end = cross_2d(second_start, second_end, first_end);

    const int first_start_sign = cross_sign(cross_first_start);
    const int first_end_sign = cross_sign(cross_first_end);
    const int second_start_sign = cross_sign(cross_second_start);
    const int second_end_sign = cross_sign(cross_second_end);

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

[[nodiscard]] bool is_closed(const core::Polyline3d& polyline) noexcept {
    return polyline.size() >= 2U &&
           core::points_coincident(polyline.front(), polyline.back());
}

[[nodiscard]] bool has_self_intersection(const core::Polyline3d& polyline) noexcept {
    if (polyline.size() < 4U) {
        return false;
    }
    for (const core::Point3d& point : polyline) {
        if (!point_is_finite(point)) {
            return false;
        }
    }

    const std::size_t segment_count = polyline.size() - 1U;
    const bool closed = is_closed(polyline);
    for (std::size_t first = 0; first < segment_count; ++first) {
        for (std::size_t second = first + 1U; second < segment_count; ++second) {
            const bool adjacent = second == first + 1U;
            const bool closing_pair = closed && first == 0U && second + 1U == segment_count;
            if (adjacent || closing_pair) {
                continue;
            }
            if (segments_intersect_2d(
                    polyline[first], polyline[first + 1U],
                    polyline[second], polyline[second + 1U])) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] double signed_area_2d(const core::Polyline3d& outline) noexcept {
    double twice_area = 0.0;
    for (std::size_t index = 0; index + 1U < outline.size(); ++index) {
        twice_area += outline[index].x * outline[index + 1U].y -
                      outline[index + 1U].x * outline[index].y;
    }
    return twice_area * 0.5;
}

void check_polyline(
    const core::Polyline3d& polyline,
    std::string_view object_type,
    std::string_view object_id,
    std::string_view geometry_field,
    std::string_view rule_id,
    ValidationReport& report) {
    for (std::size_t index = 0; index < polyline.size(); ++index) {
        check_point(
            polyline[index], index, object_type, object_id,
            geometry_field, rule_id, report);
    }

    if (polyline.size() < basic_geometry_thresholds::minimum_polyline_points) {
        add_error(
            rule_id, object_id,
            std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                std::string(geometry_field) + " 少于 2 个点。",
            "为折线补充至少 2 个按行驶或边界方向排列的点。", report);
    }

    const double length = core::polyline_length(polyline);
    if (std::isfinite(length) &&
        length < basic_geometry_thresholds::minimum_polyline_length_m) {
        add_error(
            rule_id, object_id,
            std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                std::string(geometry_field) + " 长度小于 0.1 m。",
            "补充或调整几何点，使折线具有可用长度。", report);
    }

    for (std::size_t index = 1; index < polyline.size(); ++index) {
        if (point_is_finite(polyline[index - 1U]) && point_is_finite(polyline[index]) &&
            core::points_coincident(polyline[index - 1U], polyline[index])) {
            add_error(
                rule_id, object_id,
                std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                    std::string(geometry_field) + " 存在相邻重合点。",
                "删除重复点，或者将相邻点间距调整到 1 mm 以上。", report);
        }
    }

    if (has_self_intersection(polyline)) {
        add_error(
            rule_id, object_id,
            std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                std::string(geometry_field) + " 在 XY 平面存在自相交。",
            "重新排序或调整几何点，消除非相邻线段交叉。", report);
    }
}

[[nodiscard]] double heading_difference(double first, double second) noexcept {
    return std::abs(std::remainder(
        second - first, 2.0 * std::numbers::pi_v<double>));
}

void check_path_geometry(
    const core::PathGeometry3d& path,
    std::string_view object_type,
    std::string_view object_id,
    std::string_view geometry_field,
    std::string_view rule_id,
    ValidationReport& report) {
    if (const core::Polyline3d* polyline = path.polyline()) {
        check_polyline(
            *polyline, object_type, object_id, geometry_field, rule_id, report);
        return;
    }

    const auto& segments = path.composite_curve()->segments;
    if (segments.empty()) {
        add_error(
            rule_id, object_id,
            std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                std::string(geometry_field) + " 没有曲线段。",
            "为 composite_curve 添加至少一个曲线段。", report);
        return;
    }

    std::optional<core::CurveState3d> previous_end;
    for (std::size_t index = 0U; index < segments.size(); ++index) {
        const auto& segment = segments[index];
        const std::string segment_name = std::string(geometry_field) + ".segments[" +
                                         std::to_string(index) + "]";
        std::visit([&](const auto& value) {
            using Segment = std::decay_t<decltype(value)>;
            check_point(
                value.start, index, object_type, object_id,
                segment_name + ".start", rule_id, report);
            if (!std::isfinite(value.heading_rad) || !std::isfinite(value.length_m) ||
                !std::isfinite(value.end_z_m)) {
                add_error(
                    rule_id, object_id,
                    std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                        segment_name + " 包含非有限参数。",
                    "将航向、长度和终点 Z 改为有限数字。", report);
            }
            if (!std::isfinite(value.length_m) || value.length_m <= 0.0) {
                add_error(
                    rule_id, object_id,
                    std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                        segment_name + ".lengthM 必须大于 0。",
                    "修正曲线段的水平弧长。", report);
            }
            if constexpr (std::is_same_v<Segment, core::CircularArcSegment3d>) {
                if (!std::isfinite(value.curvature_per_m) ||
                    std::abs(value.curvature_per_m) <= core::tolerance::floating_point) {
                    add_error(
                        rule_id, object_id,
                        std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                            segment_name + ".curvaturePerM 必须是非零有限值。",
                        "零曲率几何请改用 line。", report);
                }
            } else if constexpr (std::is_same_v<Segment, core::ClothoidSegment3d>) {
                if (!std::isfinite(value.start_curvature_per_m) ||
                    !std::isfinite(value.end_curvature_per_m)) {
                    add_error(
                        rule_id, object_id,
                        std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                            segment_name + " 曲率必须是有限值。",
                        "修正缓和曲线的起止曲率。", report);
                }
            }
        }, segment);

        const core::CurveState3d start = core::evaluate_curve_segment(segment, 0.0);
        const core::CurveState3d end = core::evaluate_curve_segment(
            segment, core::curve_segment_length(segment));
        if (previous_end) {
            const double gap = core::distance(previous_end->position, start.position);
            if (std::isfinite(gap) && gap > core::tolerance::coincident_point_m) {
                add_error(
                    rule_id, object_id,
                    std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                        segment_name + " 与前一段存在超过 0.001 m 的 G0 端点间隙。",
                    "对齐相邻曲线段的终点与起点。", report);
            }
            const double heading_gap = heading_difference(
                previous_end->heading_rad, start.heading_rad);
            if (std::isfinite(heading_gap) && heading_gap > 1e-4) {
                add_error(
                    rule_id, object_id,
                    std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                        segment_name + " 与前一段存在超过 0.0001 rad 的 G1 航向突变。",
                    "调整后一段起始航向，使切向连续。", report);
            }
            const double curvature_gap = std::abs(
                previous_end->curvature_per_m - start.curvature_per_m);
            if (std::isfinite(curvature_gap) && curvature_gap > 1e-6) {
                add_warning(
                    rule_id, object_id,
                    std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                        segment_name + " 与前一段存在 G2 曲率突变。",
                    "需要平顺曲率时，在两段之间加入 clothoid。", report);
            }
        }
        previous_end = end;
    }

    const double length = core::path_planar_length(path);
    if (std::isfinite(length) &&
        length < basic_geometry_thresholds::minimum_polyline_length_m) {
        add_error(
            rule_id, object_id,
            std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                std::string(geometry_field) + " 长度小于 0.1 m。",
            "调整曲线段长度，使路径具有可用长度。", report);
    }

    const core::Polyline3d tessellated = core::tessellate_path_geometry(
        path, {.max_chord_error_m = 0.01, .max_segment_length_m = 1.0});
    if (has_self_intersection(tessellated)) {
        add_error(
            rule_id, object_id,
            std::string(object_type) + " '" + std::string(object_id) + "' 的 " +
                std::string(geometry_field) + " 在 XY 平面存在自相交。",
            "调整曲线段参数，消除非相邻路径交叉。", report);
    }
}

void check_outline(
    const core::Polyline3d& outline,
    std::string_view object_type,
    std::string_view object_id,
    std::string_view rule_id,
    ValidationReport& report) {
    check_polyline(outline, object_type, object_id, "outline", rule_id, report);

    if (outline.size() < basic_geometry_thresholds::minimum_closed_outline_points) {
        add_error(
            rule_id, object_id,
            std::string(object_type) + " '" + std::string(object_id) +
                "' 的 outline 少于 4 个点。",
            "闭合区域至少需要 3 个不同顶点，并重复首点作为末点。", report);
    }

    const bool closed = is_closed(outline);
    if (!closed) {
        add_error(
            rule_id, object_id,
            std::string(object_type) + " '" + std::string(object_id) +
                "' 的 outline 未闭合。",
            "将 outline 的最后一个点设置为与第一个点重合。", report);
    }

    if (closed) {
        const double area = std::abs(signed_area_2d(outline));
        if (std::isfinite(area) &&
            area < basic_geometry_thresholds::minimum_outline_area_m2) {
            add_error(
                rule_id, object_id,
                std::string(object_type) + " '" + std::string(object_id) +
                    "' 的 XY 投影面积小于 0.01 m²。",
                "调整轮廓顶点，使区域具有有效的非零面积。", report);
        }
    }
}

void check_header_origin(
    const core::MapData& map,
    std::string_view rule_id,
    ValidationReport& report) {
    const auto& origin = map.header.coordinate_reference.origin;
    const std::string& map_id = map.header.map_id.value();
    if (!std::isfinite(origin.longitude_deg) ||
        origin.longitude_deg < -180.0 || origin.longitude_deg > 180.0) {
        add_error(
            rule_id, map_id, "地图 WGS84 原点经度必须是 [-180, 180] 范围内的有限数字。",
            "修正 coordinateReference.origin.longitudeDeg。", report);
    }
    if (!std::isfinite(origin.latitude_deg) ||
        origin.latitude_deg < -90.0 || origin.latitude_deg > 90.0) {
        add_error(
            rule_id, map_id, "地图 WGS84 原点纬度必须是 [-90, 90] 范围内的有限数字。",
            "修正 coordinateReference.origin.latitudeDeg。", report);
    }
    if (!std::isfinite(origin.altitude_m) ||
        std::abs(origin.altitude_m) >
            basic_geometry_thresholds::maximum_absolute_origin_altitude_m) {
        add_error(
            rule_id, map_id, "地图 WGS84 原点高程必须是 ±10 km 范围内的有限数字。",
            "修正 coordinateReference.origin.altitudeM，并确认高程单位为米。", report);
    }
}

}  // namespace

std::string_view BasicGeometryRule::id() const noexcept {
    return "M3_BASIC_GEOMETRY";
}

void BasicGeometryRule::validate(
    const ValidationContext& context,
    ValidationReport& report) const {
    const core::MapData& map = context.map;
    check_header_origin(map, id(), report);

    for (const auto& road : map.roads) {
        check_path_geometry(road.reference_line, "Road", road.id, "referenceLine", id(), report);
    }
    for (const auto& lane : map.lanes) {
        check_path_geometry(lane.centerline, "Lane", lane.id, "centerline", id(), report);
        if (!std::isfinite(lane.width_m) ||
            lane.width_m < basic_geometry_thresholds::minimum_lane_width_m ||
            lane.width_m > basic_geometry_thresholds::maximum_lane_width_m) {
            add_error(
                id(), lane.id,
                "Lane '" + lane.id + "' 的 widthM 必须是 [1, 20] m 范围内的有限数字。",
                "修正车道宽度，并确认单位为米。", report);
        }
    }
    for (const auto& boundary : map.lane_boundaries) {
        check_path_geometry(boundary.geometry, "LaneBoundary", boundary.id, "geometry", id(), report);
    }
    for (const auto& area : map.operational_areas) {
        check_outline(area.outline, "OperationalArea", area.id, id(), report);
    }
    for (const auto& station : map.stations) {
        check_point(station.position, 0U, "Station", station.id, "position", id(), report);
    }
    for (const auto& area : map.restricted_areas) {
        check_outline(area.outline, "RestrictedArea", area.id, id(), report);
    }
}

}  // namespace automap::validation
