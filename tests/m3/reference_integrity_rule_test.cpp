#include "automap/io/canonical_json.hpp"
#include "automap/validation/map_validation.hpp"

#include <filesystem>
#include <iostream>
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
    using automap::validation::ReferenceIntegrityRule;
    using automap::validation::Severity;
    using automap::validation::ValidationContext;
    using automap::validation::ValidationReport;

    bool passed = true;
    const ReferenceIntegrityRule rule;
    passed &= check(
        rule.id() == "M3_REFERENCE_INTEGRITY",
        "引用完整性规则 ID 应保持稳定。");

    const std::filesystem::path canonical_map_path =
        std::filesystem::path{AUTOMAP_SOURCE_DIR} / "maps" / "drafts" /
        "logistics_park_v0.json";
    const MapData logistics_map =
        automap::io::read_canonical_json_file(canonical_map_path);
    ValidationReport logistics_report{logistics_map.header.map_id};
    rule.validate(ValidationContext{.map = logistics_map}, logistics_report);
    passed &= check(
        logistics_report.issues().empty(),
        "物流园 V0 的全部对象引用应当存在。");

    MapData invalid_map;
    invalid_map.header.map_id = MapId{"invalid_references"};
    invalid_map.roads.push_back({
        .id = "road_owner",
        .predecessor_ids = {"missing_predecessor_road"},
        .successor_ids = {"missing_successor_road"},
        .lane_ids = {"missing_road_lane"},
    });
    invalid_map.lanes.push_back({
        .id = "lane_owner",
        .road_id = "missing_lane_road",
        .left_boundary_id = "missing_left_boundary",
        .right_boundary_id = "missing_right_boundary",
        .predecessor_ids = {"missing_predecessor_lane"},
        .successor_ids = {"missing_successor_lane"},
    });
    invalid_map.junctions.push_back({
        .id = "junction_owner",
        .connection_ids = {"missing_connection"},
    });
    invalid_map.lane_connections.push_back({
        .id = "connection_owner",
        .junction_id = "missing_junction",
        .incoming_lane_id = "missing_incoming_lane",
        .connecting_lane_id = "missing_connecting_lane",
        .outgoing_lane_id = "missing_outgoing_lane",
    });
    invalid_map.stations.push_back({
        .id = "station_owner",
        .access_lane_id = "missing_access_lane",
    });
    invalid_map.restricted_areas.push_back({
        .id = "restricted_owner",
        .allowed_vehicle_profile_ids = {"missing_vehicle_profile"},
    });

    ValidationReport invalid_report{invalid_map.header.map_id};
    rule.validate(ValidationContext{.map = invalid_map}, invalid_report);

    passed &= check(
        invalid_report.count(Severity::error) == 15U,
        "所有 15 个悬空引用都应产生 Error。");
    passed &= check(
        invalid_report.count(Severity::fatal) == 0U,
        "引用完整性问题应为 Error，而不是 Fatal。");
    passed &= check(
        !invalid_report.can_publish(),
        "存在悬空引用时应阻止地图发布。");

    bool found_boundary_reference = false;
    bool found_station_reference = false;
    bool found_vehicle_reference = false;
    for (const auto& issue : invalid_report.issues()) {
        passed &= check(issue.rule_id == rule.id(), "问题应记录引用完整性规则 ID。");
        passed &= check(!issue.object_id.empty(), "问题应记录持有悬空引用的对象 ID。");
        passed &= check(!issue.suggestion.empty(), "问题应提供引用修复建议。");
        found_boundary_reference |= issue.message.find("leftBoundaryId") != std::string::npos;
        found_station_reference |= issue.message.find("accessLaneId") != std::string::npos;
        found_vehicle_reference |=
            issue.message.find("allowedVehicleProfileIds") != std::string::npos;
    }
    passed &= check(found_boundary_reference, "应发现不存在的车道边界引用。");
    passed &= check(found_station_reference, "应发现 Station 的接入车道引用。");
    passed &= check(found_vehicle_reference, "应发现限行区域的车辆模型引用。");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M3 引用完整性规则测试通过。\n";
    return 0;
}
