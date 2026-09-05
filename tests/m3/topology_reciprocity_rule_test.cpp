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
    using automap::core::MapData;
    using automap::core::MapId;
    using automap::validation::Severity;
    using automap::validation::TopologyReciprocityRule;
    using automap::validation::ValidationContext;
    using automap::validation::ValidationReport;

    bool passed = true;
    const TopologyReciprocityRule rule;
    passed &= check(
        rule.id() == "M3_TOPOLOGY_RECIPROCITY",
        "拓扑互反规则 ID 应保持稳定。");

    const std::filesystem::path canonical_map_path =
        std::filesystem::path{AUTOMAP_SOURCE_DIR} / "maps" / "drafts" /
        "logistics_park_v0.json";
    const MapData logistics_map =
        automap::io::read_canonical_json_file(canonical_map_path);
    ValidationReport logistics_report{logistics_map.header.map_id};
    rule.validate(ValidationContext{.map = logistics_map}, logistics_report);
    passed &= check(
        logistics_report.issues().empty(),
        "物流园 V0 的道路、车道和所有权拓扑应当互反一致。");

    MapData invalid_map;
    invalid_map.header.map_id = MapId{"invalid_reciprocity"};

    invalid_map.roads.push_back({
        .id = "road_successor_owner",
        .successor_ids = {"road_successor_target"},
    });
    invalid_map.roads.push_back({.id = "road_successor_target"});
    invalid_map.roads.push_back({
        .id = "road_predecessor_owner",
        .predecessor_ids = {"road_predecessor_target"},
    });
    invalid_map.roads.push_back({.id = "road_predecessor_target"});

    invalid_map.lanes.push_back({
        .id = "lane_successor_owner",
        .successor_ids = {"lane_successor_target"},
    });
    invalid_map.lanes.push_back({.id = "lane_successor_target"});
    invalid_map.lanes.push_back({
        .id = "lane_predecessor_owner",
        .predecessor_ids = {"lane_predecessor_target"},
    });
    invalid_map.lanes.push_back({.id = "lane_predecessor_target"});

    invalid_map.roads.push_back({
        .id = "road_lists_wrong_lane",
        .lane_ids = {"lane_owned_by_other_road"},
    });
    invalid_map.roads.push_back({
        .id = "road_actual_owner",
        .lane_ids = {"lane_owned_by_other_road"},
    });
    invalid_map.lanes.push_back({
        .id = "lane_owned_by_other_road",
        .road_id = "road_actual_owner",
    });
    invalid_map.roads.push_back({.id = "road_missing_lane_membership"});
    invalid_map.lanes.push_back({
        .id = "lane_not_listed_by_road",
        .road_id = "road_missing_lane_membership",
    });

    invalid_map.junctions.push_back({
        .id = "junction_lists_wrong_connection",
        .connection_ids = {"connection_owned_by_other_junction"},
    });
    invalid_map.junctions.push_back({
        .id = "junction_actual_owner",
        .connection_ids = {"connection_owned_by_other_junction"},
    });
    invalid_map.lane_connections.push_back({
        .id = "connection_owned_by_other_junction",
        .junction_id = "junction_actual_owner",
    });
    invalid_map.junctions.push_back({.id = "junction_missing_connection_membership"});
    invalid_map.lane_connections.push_back({
        .id = "connection_not_listed_by_junction",
        .junction_id = "junction_missing_connection_membership",
    });

    invalid_map.roads.push_back({
        .id = "road_with_missing_target",
        .successor_ids = {"missing_road"},
    });

    ValidationReport invalid_report{invalid_map.header.map_id};
    rule.validate(ValidationContext{.map = invalid_map}, invalid_report);

    passed &= check(
        invalid_report.count(Severity::error) == 8U,
        "构造的 8 个互反关系错误都应产生 Error。");
    passed &= check(
        invalid_report.count(Severity::fatal) == 0U,
        "拓扑互反问题应为 Error，而不是 Fatal。");
    passed &= check(
        !invalid_report.can_publish(),
        "存在拓扑互反问题时应阻止地图发布。");

    bool found_road_successor = false;
    bool found_road_predecessor = false;
    bool found_lane_successor = false;
    bool found_lane_predecessor = false;
    bool found_road_lane_ownership = false;
    bool found_junction_ownership = false;
    bool reported_missing_target = false;
    for (const auto& issue : invalid_report.issues()) {
        passed &= check(issue.rule_id == rule.id(), "问题应记录拓扑互反规则 ID。");
        passed &= check(!issue.object_id.empty(), "问题应记录关系持有者 ID。");
        passed &= check(!issue.suggestion.empty(), "问题应提供拓扑修复建议。");
        found_road_successor |=
            issue.message.find("Road") != std::string::npos &&
            issue.message.find("successor") != std::string::npos;
        found_road_predecessor |=
            issue.message.find("Road") != std::string::npos &&
            issue.message.find("predecessor") != std::string::npos;
        found_lane_successor |=
            issue.message.find("Lane") != std::string::npos &&
            issue.message.find("successor") != std::string::npos;
        found_lane_predecessor |=
            issue.message.find("Lane") != std::string::npos &&
            issue.message.find("predecessor") != std::string::npos;
        found_road_lane_ownership |= issue.message.find("laneIds") != std::string::npos;
        found_junction_ownership |= issue.message.find("connectionIds") != std::string::npos;
        reported_missing_target |= issue.message.find("missing_road") != std::string::npos;
    }

    passed &= check(found_road_successor, "应发现 Road successor 缺少反向 predecessor。");
    passed &= check(found_road_predecessor, "应发现 Road predecessor 缺少反向 successor。");
    passed &= check(found_lane_successor, "应发现 Lane successor 缺少反向 predecessor。");
    passed &= check(found_lane_predecessor, "应发现 Lane predecessor 缺少反向 successor。");
    passed &= check(found_road_lane_ownership, "应发现 Road 与 Lane 所有权不互反。");
    passed &= check(found_junction_ownership, "应发现 Junction 与连接所有权不互反。");
    passed &= check(
        !reported_missing_target,
        "目标不存在的问题应留给引用完整性规则，不应重复报告。");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M3 拓扑互反规则测试通过。\n";
    return 0;
}
