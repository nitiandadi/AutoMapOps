#include "automap/core/geometry.hpp"
#include "automap/io/canonical_json.hpp"
#include "automap/validation/map_validator.hpp"
#include "automap/validation/validation_report_json.hpp"
#include "automap/version/version_publisher.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace {

using automap::core::BoundingBox3d;
using automap::core::MapData;
using automap::core::PathGeometry3d;
using automap::core::Point3d;
using automap::core::Polyline3d;

void expand_bounds(std::optional<BoundingBox3d>& bounds, const Point3d& point) {
    if (!bounds) {
        bounds = BoundingBox3d{.min = point, .max = point};
        return;
    }
    bounds->min.x = std::min(bounds->min.x, point.x);
    bounds->min.y = std::min(bounds->min.y, point.y);
    bounds->min.z = std::min(bounds->min.z, point.z);
    bounds->max.x = std::max(bounds->max.x, point.x);
    bounds->max.y = std::max(bounds->max.y, point.y);
    bounds->max.z = std::max(bounds->max.z, point.z);
}

void expand_bounds(std::optional<BoundingBox3d>& bounds, const Polyline3d& polyline) {
    for (const Point3d& point : polyline) {
        expand_bounds(bounds, point);
    }
}

void expand_bounds(std::optional<BoundingBox3d>& bounds, const PathGeometry3d& path) {
    const auto path_bounds = automap::core::path_bounding_box(path);
    if (!path_bounds) return;
    expand_bounds(bounds, path_bounds->min);
    expand_bounds(bounds, path_bounds->max);
}

struct PathStatistics final {
    std::size_t polylines{0U};
    std::size_t polyline_points{0U};
    std::size_t composite_curves{0U};
    std::size_t line_segments{0U};
    std::size_t circular_arc_segments{0U};
    std::size_t clothoid_segments{0U};
    double planar_length_m{0.0};
};

void collect_path_statistics(const PathGeometry3d& path, PathStatistics& statistics) {
    statistics.planar_length_m += automap::core::path_planar_length(path);
    if (const Polyline3d* polyline = path.polyline()) {
        ++statistics.polylines;
        statistics.polyline_points += polyline->size();
        return;
    }
    ++statistics.composite_curves;
    for (const auto& segment : path.composite_curve()->segments) {
        std::visit([&](const auto& value) {
            static_cast<void>(value);
            using Segment = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Segment, automap::core::LineCurveSegment3d>) {
                ++statistics.line_segments;
            } else if constexpr (std::is_same_v<Segment, automap::core::CircularArcSegment3d>) {
                ++statistics.circular_arc_segments;
            } else {
                ++statistics.clothoid_segments;
            }
        }, segment);
    }
}

[[nodiscard]] std::optional<BoundingBox3d> map_bounds(const MapData& map) {
    std::optional<BoundingBox3d> bounds;
    for (const auto& road : map.roads) expand_bounds(bounds, road.reference_line);
    for (const auto& lane : map.lanes) expand_bounds(bounds, lane.centerline);
    for (const auto& boundary : map.lane_boundaries) expand_bounds(bounds, boundary.geometry);
    for (const auto& area : map.operational_areas) expand_bounds(bounds, area.outline);
    for (const auto& station : map.stations) expand_bounds(bounds, station.position);
    for (const auto& area : map.restricted_areas) expand_bounds(bounds, area.outline);
    return bounds;
}

void print_point(const Point3d& point) {
    std::cout << '(' << point.x << ", " << point.y << ", " << point.z << ')';
}

void print_inspection(const MapData& map) {
    const auto& coordinate = map.header.coordinate_reference;
    std::cout << std::setprecision(15);
    std::cout << "地图 ID：" << map.header.map_id.value() << '\n';
    std::cout << "地图名称：" << map.header.name << '\n';
    std::cout << "Schema 版本：" << map.header.schema_version << '\n';
    std::cout << "坐标参考：" << coordinate.geodetic_datum << " / "
              << automap::core::local_coordinate_frame_name(coordinate.local_frame)
              << "（X 向东，Y 向北，Z 向上）\n";
    std::cout << "WGS84 原点：经度 " << coordinate.origin.longitude_deg
              << "°，纬度 " << coordinate.origin.latitude_deg
              << "°，高程 " << coordinate.origin.altitude_m << " m\n";
    std::cout << "单位：长度 " << coordinate.linear_unit
              << "，角度 " << coordinate.angle_unit << '\n';

    const auto bounds = map_bounds(map);
    if (bounds) {
        std::cout << "ENU 范围（" << coordinate.linear_unit << "）：min=";
        print_point(bounds->min);
        std::cout << "，max=";
        print_point(bounds->max);
        std::cout << '\n';
    } else {
        std::cout << "ENU 范围：无几何数据\n";
    }

    std::cout << "对象统计：\n";
    std::cout << "  Road：" << map.roads.size() << '\n';
    std::cout << "  Lane：" << map.lanes.size() << '\n';
    std::cout << "  LaneBoundary：" << map.lane_boundaries.size() << '\n';
    std::cout << "  Junction：" << map.junctions.size() << '\n';
    std::cout << "  LaneConnection：" << map.lane_connections.size() << '\n';
    std::cout << "  OperationalArea：" << map.operational_areas.size() << '\n';
    std::cout << "  Station：" << map.stations.size() << '\n';
    std::cout << "  RestrictedArea：" << map.restricted_areas.size() << '\n';
    std::cout << "  VehicleProfile：" << map.vehicle_profiles.size() << '\n';

    PathStatistics path_statistics;
    for (const auto& road : map.roads) collect_path_statistics(road.reference_line, path_statistics);
    for (const auto& lane : map.lanes) collect_path_statistics(lane.centerline, path_statistics);
    for (const auto& boundary : map.lane_boundaries) collect_path_statistics(boundary.geometry, path_statistics);
    std::cout << "路径几何统计：\n";
    std::cout << "  点列：" << path_statistics.polylines
              << "（" << path_statistics.polyline_points << " 个源点）\n";
    std::cout << "  组合曲线：" << path_statistics.composite_curves << '\n';
    std::cout << "  line / circular_arc / clothoid："
              << path_statistics.line_segments << " / "
              << path_statistics.circular_arc_segments << " / "
              << path_statistics.clothoid_segments << '\n';
    std::cout << "  XY 总弧长：" << path_statistics.planar_length_m << " m\n";
}

void print_usage(std::string_view program) {
    std::cout << "AutoMapOps " << AUTOMAP_PROJECT_VERSION << '\n';
    std::cout << "Canonical format: "
              << automap::io::canonical_json_format_name() << '\n';
    std::cout << "用法：\n";
    std::cout << "  " << program << " inspect <canonical-json-path>\n";
    std::cout << "  " << program
              << " validate <canonical-json-path> <validation-report-path>\n";
    std::cout << "  " << program
              << " publish <canonical-json-path> <version-directory> [version-id]\n";
}

int inspect_command(const std::filesystem::path& path) {
    try {
        print_inspection(automap::io::read_canonical_json_file(path));
        return 0;
    } catch (const automap::io::CanonicalJsonError& error) {
        std::cerr << "读取 Canonical 地图失败：" << error.what() << '\n';
        return 1;
    }
}

int validate_command(
    const std::filesystem::path& map_path,
    const std::filesystem::path& report_path) {
    try {
        const MapData map = automap::io::read_canonical_json_file(map_path);
        const auto report = automap::validation::validate_map(map);
        if (!report_path.parent_path().empty()) {
            std::filesystem::create_directories(report_path.parent_path());
        }
        automap::validation::write_validation_report_json_file(report_path, report);
        std::cout << "质检报告已写入：" << report_path.string() << '\n'
                  << "Warning/Error/Fatal："
                  << report.count(automap::validation::Severity::warning) << '/'
                  << report.count(automap::validation::Severity::error) << '/'
                  << report.count(automap::validation::Severity::fatal) << '\n';
        return report.can_publish() ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "生成质检报告失败：" << error.what() << '\n';
        return 1;
    }
}

int publish_command(
    const std::filesystem::path& map_path,
    const std::filesystem::path& version_directory,
    std::string version_id) {
    try {
        const MapData map = automap::io::read_canonical_json_file(map_path);
        const auto report = automap::validation::validate_map(map);
        const automap::version::VersionPublisher publisher;
        const auto result = publisher.publish(
            map, report,
            {
                .output_directory = version_directory,
                .version_id = automap::version::VersionId{std::move(version_id)},
            });
        if (!result.published) {
            std::cerr << "发布失败：" << result.message << '\n';
            std::cerr << "Fatal/Error："
                      << report.count(automap::validation::Severity::fatal) << '/'
                      << report.count(automap::validation::Severity::error) << '\n';
            return 1;
        }
        std::cout << "MapVersion 发布成功：" << result.output_directory.string() << '\n'
                  << "内容哈希：" << result.content_hash << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "发布 MapVersion 失败：" << error.what() << '\n';
        return 1;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    if (std::string_view{argv[1]} == "inspect") {
        if (argc != 3) {
            std::cerr << "inspect 命令需要且仅需要一个 Canonical JSON 文件路径。\n";
            print_usage(argv[0]);
            return 2;
        }
        return inspect_command(std::filesystem::path{argv[2]});
    }
    if (std::string_view{argv[1]} == "validate") {
        if (argc != 4) {
            std::cerr << "validate 命令需要地图路径和报告输出路径。\n";
            print_usage(argv[0]);
            return 2;
        }
        return validate_command(argv[2], argv[3]);
    }
    if (std::string_view{argv[1]} == "publish") {
        if (argc != 4 && argc != 5) {
            std::cerr << "publish 命令需要地图路径、版本目录和可选版本号。\n";
            print_usage(argv[0]);
            return 2;
        }
        return publish_command(argv[2], argv[3], argc == 5 ? argv[4] : "V1");
    }

    std::cerr << "未知命令：" << argv[1] << '\n';
    print_usage(argv[0]);
    return 2;
}
