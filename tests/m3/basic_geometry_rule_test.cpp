#include "automap/io/canonical_json.hpp"
#include "automap/validation/map_validation.hpp"

#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {

bool check(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "失败：" << message << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main() {
    using automap::core::MapData;
    using automap::core::MapId;
    using automap::validation::BasicGeometryRule;
    using automap::validation::Severity;
    using automap::validation::ValidationContext;
    using automap::validation::ValidationReport;

    bool passed = true;
    const BasicGeometryRule rule;
    passed &= check(rule.id() == "M3_BASIC_GEOMETRY", "基础几何规则 ID 应保持稳定。");

    const std::filesystem::path canonical_map_path =
        std::filesystem::path{AUTOMAP_SOURCE_DIR} / "maps" / "drafts" /
        "logistics_park_v0.json";
    const MapData logistics_map =
        automap::io::read_canonical_json_file(canonical_map_path);
    ValidationReport logistics_report{logistics_map.header.map_id};
    rule.validate(ValidationContext{.map = logistics_map}, logistics_report);
    passed &= check(
        logistics_report.issues().empty(),
        "物流园 V0 应通过基础几何规则。");

    MapData invalid_map;
    invalid_map.header.map_id = MapId{"invalid_geometry"};
    invalid_map.header.coordinate_reference.origin.longitude_deg = 200.0;

    invalid_map.roads.push_back({
        .id = "road_too_few_points",
        .reference_line = {{0.0, 0.0, 0.0}},
    });
    invalid_map.roads.push_back({
        .id = "road_self_intersection",
        .reference_line = {
            {0.0, 0.0, 0.0},
            {2.0, 2.0, 0.0},
            {0.0, 2.0, 0.0},
            {2.0, 0.0, 0.0},
        },
    });
    invalid_map.lanes.push_back({
        .id = "lane_zero_segment",
        .centerline = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},
        .width_m = 0.5,
    });
    invalid_map.lanes.push_back({
        .id = "lane_too_wide",
        .centerline = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}},
        .width_m = 25.0,
    });
    invalid_map.lane_boundaries.push_back({
        .id = "boundary_too_few_points",
        .geometry = {{0.0, 0.0, 0.0}},
    });
    invalid_map.operational_areas.push_back({
        .id = "area_not_closed",
        .outline = {
            {0.0, 0.0, 0.0},
            {2.0, 0.0, 0.0},
            {2.0, 2.0, 0.0},
            {0.0, 2.0, 0.0},
        },
    });
    invalid_map.restricted_areas.push_back({
        .id = "area_self_intersection",
        .outline = {
            {0.0, 0.0, 0.0},
            {2.0, 2.0, 0.0},
            {0.0, 2.0, 0.0},
            {2.0, 0.0, 0.0},
            {0.0, 0.0, 0.0},
        },
    });
    invalid_map.stations.push_back({
        .id = "station_out_of_range",
        .position = {100'001.0, 0.0, 0.0},
    });
    invalid_map.stations.push_back({
        .id = "station_non_finite",
        .position = {0.0, std::numeric_limits<double>::quiet_NaN(), 0.0},
    });

    ValidationReport invalid_report{invalid_map.header.map_id};
    rule.validate(ValidationContext{.map = invalid_map}, invalid_report);

    passed &= check(
        invalid_report.count(Severity::error) == 15U,
        "构造的 15 个基础几何问题都应产生 Error。");
    passed &= check(
        invalid_report.count(Severity::fatal) == 0U,
        "基础几何问题应为 Error，而不是 Fatal。");
    passed &= check(!invalid_report.can_publish(), "存在基础几何问题时应阻止发布。");

    bool found_point_count = false;
    bool found_length = false;
    bool found_intersection = false;
    bool found_width = false;
    bool found_coordinate_range = false;
    bool found_open_outline = false;
    bool found_area = false;
    for (const auto& issue : invalid_report.issues()) {
        passed &= check(issue.rule_id == rule.id(), "问题应记录基础几何规则 ID。");
        passed &= check(!issue.object_id.empty(), "问题应记录地图或对象 ID。");
        passed &= check(!issue.suggestion.empty(), "问题应提供几何修复建议。");
        found_point_count |= issue.message.find("少于 2 个点") != std::string::npos ||
                             issue.message.find("少于 4 个点") != std::string::npos;
        found_length |= issue.message.find("长度小于") != std::string::npos;
        found_intersection |= issue.message.find("自相交") != std::string::npos;
        found_width |= issue.message.find("widthM") != std::string::npos;
        found_coordinate_range |= issue.message.find("允许范围") != std::string::npos ||
                                  issue.message.find("原点经度") != std::string::npos;
        found_open_outline |= issue.message.find("未闭合") != std::string::npos;
        found_area |= issue.message.find("投影面积") != std::string::npos;
    }

    passed &= check(found_point_count, "应发现几何点数不足。");
    passed &= check(found_length, "应发现几何长度不足。");
    passed &= check(found_intersection, "应发现 XY 平面自相交。");
    passed &= check(found_width, "应发现 Lane 宽度越界。");
    passed &= check(found_coordinate_range, "应发现坐标越界。");
    passed &= check(found_open_outline, "应发现区域轮廓未闭合。");
    passed &= check(found_area, "应发现区域面积无效。");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M3 基础几何规则测试通过。\n";
    return 0;
}
