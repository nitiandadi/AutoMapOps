#include "automap/io/canonical_json.hpp"
#include "automap/validation/map_validation.hpp"

#include <filesystem>
#include <iostream>
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
    using automap::core::LaneDirection;
    using automap::core::MapData;
    using automap::core::MapId;
    using automap::validation::ConnectionGeometryRule;
    using automap::validation::Severity;
    using automap::validation::ValidationContext;
    using automap::validation::ValidationReport;

    bool passed = true;
    const ConnectionGeometryRule rule;
    passed &= check(
        rule.id() == "M3_CONNECTION_GEOMETRY",
        "连接几何规则 ID 应保持稳定。");

    const std::filesystem::path canonical_map_path =
        std::filesystem::path{AUTOMAP_SOURCE_DIR} / "maps" / "drafts" /
        "logistics_park_v0.json";
    const MapData logistics_map =
        automap::io::read_canonical_json_file(canonical_map_path);
    ValidationReport logistics_report{logistics_map.header.map_id};
    rule.validate(ValidationContext{.map = logistics_map}, logistics_report);
    passed &= check(
        logistics_report.issues().empty(),
        "物流园 V0 的道路和车道连接几何应在容差内连续。");

    MapData invalid_map;
    invalid_map.header.map_id = MapId{"invalid_connections"};

    invalid_map.roads.push_back({
        .id = "road_gap_source",
        .reference_line = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}},
        .successor_ids = {"road_gap_target"},
    });
    invalid_map.roads.push_back({
        .id = "road_gap_target",
        .reference_line = {{11.0, 0.0, 0.0}, {20.0, 0.0, 0.0}},
    });
    invalid_map.roads.push_back({
        .id = "road_heading_source",
        .reference_line = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}},
        .successor_ids = {"road_heading_target"},
    });
    invalid_map.roads.push_back({
        .id = "road_heading_target",
        .reference_line = {{10.0, 0.0, 0.0}, {10.0, 10.0, 0.0}},
    });

    invalid_map.lanes.push_back({
        .id = "lane_gap_source",
        .centerline = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}},
        .successor_ids = {"lane_gap_target"},
    });
    invalid_map.lanes.push_back({
        .id = "lane_gap_target",
        .centerline = {{11.0, 0.0, 0.0}, {20.0, 0.0, 0.0}},
    });
    invalid_map.lanes.push_back({
        .id = "lane_heading_source",
        .centerline = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}},
        .successor_ids = {"lane_heading_target"},
    });
    invalid_map.lanes.push_back({
        .id = "lane_heading_target",
        .centerline = {{10.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},
    });
    invalid_map.lanes.push_back({
        .id = "lane_vertical_source",
        .centerline = {{0.0, 0.0, 0.0}, {0.0, 0.0, 2.0}},
        .successor_ids = {"lane_vertical_target"},
    });
    invalid_map.lanes.push_back({
        .id = "lane_vertical_target",
        .centerline = {{0.0, 0.0, 2.0}, {0.0, 0.0, 4.0}},
    });

    invalid_map.lanes.push_back({
        .id = "lane_against_source",
        .centerline = {{10.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},
        .successor_ids = {"lane_against_target"},
        .direction = LaneDirection::against_reference_line,
    });
    invalid_map.lanes.push_back({
        .id = "lane_against_target",
        .centerline = {{20.0, 0.0, 0.0}, {10.0, 0.0, 0.0}},
        .direction = LaneDirection::against_reference_line,
    });

    invalid_map.lanes.push_back({
        .id = "lane_connection_incoming",
        .centerline = {{0.0, 20.0, 0.0}, {10.0, 20.0, 0.0}},
    });
    invalid_map.lanes.push_back({
        .id = "lane_connection_connecting",
        .centerline = {{11.0, 20.0, 0.0}, {20.0, 20.0, 0.0}},
    });
    invalid_map.lanes.push_back({
        .id = "lane_connection_outgoing",
        .centerline = {{20.0, 20.0, 0.0}, {30.0, 20.0, 0.0}},
    });
    invalid_map.lane_connections.push_back({
        .id = "connection_with_gap",
        .incoming_lane_id = "lane_connection_incoming",
        .connecting_lane_id = "lane_connection_connecting",
        .outgoing_lane_id = "lane_connection_outgoing",
    });

    invalid_map.roads.push_back({
        .id = "road_missing_target_owner",
        .reference_line = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}},
        .successor_ids = {"missing_road"},
    });

    ValidationReport invalid_report{invalid_map.header.map_id};
    rule.validate(ValidationContext{.map = invalid_map}, invalid_report);

    passed &= check(
        invalid_report.count(Severity::error) == 6U,
        "构造的 6 个连接几何问题都应产生 Error。");
    passed &= check(
        invalid_report.count(Severity::fatal) == 0U,
        "连接几何问题应为 Error，而不是 Fatal。");
    passed &= check(
        !invalid_report.can_publish(),
        "存在连接几何问题时应阻止地图发布。");

    std::size_t distance_issue_count = 0U;
    std::size_t heading_issue_count = 0U;
    bool found_missing_heading = false;
    bool reported_against_reference = false;
    bool reported_missing_target = false;
    for (const auto& issue : invalid_report.issues()) {
        passed &= check(issue.rule_id == rule.id(), "问题应记录连接几何规则 ID。");
        passed &= check(!issue.object_id.empty(), "问题应记录连接源对象 ID。");
        passed &= check(!issue.suggestion.empty(), "问题应提供连接几何修复建议。");
        distance_issue_count += issue.message.find("端点距离") != std::string::npos ? 1U : 0U;
        heading_issue_count += issue.message.find("航向差") != std::string::npos ? 1U : 0U;
        found_missing_heading |= issue.message.find("无法计算连接航向") != std::string::npos;
        reported_against_reference |= issue.message.find("lane_against") != std::string::npos;
        reported_missing_target |= issue.message.find("missing_road") != std::string::npos;
    }

    passed &= check(distance_issue_count == 3U, "应发现 3 个端点距离超差连接。");
    passed &= check(heading_issue_count == 2U, "应发现 2 个航向差超差连接。");
    passed &= check(found_missing_heading, "应发现无法形成 XY 航向的连接。");
    passed &= check(
        !reported_against_reference,
        "against_reference_line Lane 应按实际行驶方向正确连接。");
    passed &= check(
        !reported_missing_target,
        "目标不存在的问题应留给引用完整性规则，不应重复报告。");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M3 连接几何规则测试通过。\n";
    return 0;
}
