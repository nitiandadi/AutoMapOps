#include "automap/io/canonical_json.hpp"
#include "automap/validation/map_validation.hpp"

#include <algorithm>
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
    using automap::core::LaneStatus;
    using automap::core::MapData;
    using automap::core::MapId;
    using automap::core::OperationalAreaType;
    using automap::core::StationType;
    using automap::core::VehicleType;
    using automap::validation::NetworkReachabilityRule;
    using automap::validation::Severity;
    using automap::validation::ValidationContext;
    using automap::validation::ValidationReport;

    bool passed = true;
    const NetworkReachabilityRule rule;
    passed &= check(
        rule.id() == "M3_NETWORK_REACHABILITY",
        "路网可达性规则 ID 应保持稳定。");

    const std::filesystem::path canonical_map_path =
        std::filesystem::path{AUTOMAP_SOURCE_DIR} / "maps" / "drafts" /
        "logistics_park_v0.json";
    const MapData logistics_map =
        automap::io::read_canonical_json_file(canonical_map_path);
    ValidationReport logistics_report{logistics_map.header.map_id};
    rule.validate(ValidationContext{.map = logistics_map}, logistics_report);
    passed &= check(
        logistics_report.issues().empty(),
        "配送厢式车应能从门岗到达物流园 A1 月台。");

    MapData closed_route_map = logistics_map;
    closed_route_map.find_lane("lane_j10_detour")->status = LaneStatus::closed;
    ValidationReport closed_route_report{closed_route_map.header.map_id};
    rule.validate(ValidationContext{.map = closed_route_map}, closed_route_report);
    passed &= check(
        closed_route_report.count(Severity::error) == 1U,
        "关闭装卸绕行连接 Lane 后，A1 月台应不可达。");

    MapData truck_only_map = logistics_map;
    std::erase_if(
        truck_only_map.vehicle_profiles,
        [](const auto& profile) { return profile.id != "vehicle_truck_12m"; });
    ValidationReport truck_only_report{truck_only_map.header.map_id};
    rule.validate(ValidationContext{.map = truck_only_map}, truck_only_report);
    passed &= check(
        truck_only_report.count(Severity::error) == 1U,
        "仅保留不在窄通道白名单中的卡车时，A1 月台应不可达。");

    MapData too_wide_map = logistics_map;
    for (auto& vehicle : too_wide_map.vehicle_profiles) {
        vehicle.width_m = 4.0;
    }
    ValidationReport too_wide_report{too_wide_map.header.map_id};
    rule.validate(ValidationContext{.map = too_wide_map}, too_wide_report);
    passed &= check(
        too_wide_report.count(Severity::error) == 1U,
        "所有车辆都宽于 Lane 时，A1 月台应不可达。");

    MapData connection_only_map;
    connection_only_map.header.map_id = MapId{"connection_only"};
    connection_only_map.operational_areas.push_back({
        .id = "warehouse",
        .type = OperationalAreaType::warehouse,
    });
    connection_only_map.lanes.push_back({
        .id = "lane_incoming",
        .status = LaneStatus::open,
        .width_m = 3.5,
    });
    connection_only_map.lanes.push_back({
        .id = "lane_connecting",
        .status = LaneStatus::open,
        .width_m = 3.5,
    });
    connection_only_map.lanes.push_back({
        .id = "lane_outgoing",
        .status = LaneStatus::open,
        .width_m = 3.5,
    });
    connection_only_map.lane_connections.push_back({
        .id = "connection",
        .incoming_lane_id = "lane_incoming",
        .connecting_lane_id = "lane_connecting",
        .outgoing_lane_id = "lane_outgoing",
    });
    connection_only_map.stations.push_back({
        .id = "gate",
        .type = StationType::gate,
        .access_lane_id = "lane_incoming",
    });
    connection_only_map.stations.push_back({
        .id = "loading_bay",
        .type = StationType::loading_bay,
        .access_lane_id = "lane_outgoing",
    });
    connection_only_map.vehicle_profiles.push_back({
        .id = "vehicle",
        .type = VehicleType::delivery_van,
        .width_m = 2.0,
    });
    ValidationReport connection_only_report{connection_only_map.header.map_id};
    rule.validate(ValidationContext{.map = connection_only_map}, connection_only_report);
    passed &= check(
        connection_only_report.issues().empty(),
        "LaneConnection 应能为可达性图补充入口、连接和出口边。");

    MapData not_applicable_map;
    not_applicable_map.header.map_id = MapId{"no_warehouse"};
    ValidationReport not_applicable_report{not_applicable_map.header.map_id};
    rule.validate(ValidationContext{.map = not_applicable_map}, not_applicable_report);
    passed &= check(
        not_applicable_report.issues().empty(),
        "不包含 Warehouse 的地图不应强制执行物流园仓库到月台场景。");

    for (const auto* report : {
             &closed_route_report, &truck_only_report, &too_wide_report}) {
        passed &= check(!report->can_publish(), "月台不可达时应阻止地图发布。");
        passed &= check(
            report->issues().front().rule_id == rule.id(),
            "不可达问题应记录路网可达性规则 ID。");
        passed &= check(
            report->issues().front().object_id == "station_loading_a1",
            "不可达问题应定位到 LoadingBay Station。");
        passed &= check(
            !report->issues().front().suggestion.empty(),
            "不可达问题应提供拓扑或车辆约束修复建议。");
    }

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M3 路网可达性规则测试通过。\n";
    return 0;
}
