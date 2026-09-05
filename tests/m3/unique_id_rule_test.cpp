#include "automap/core/map_core.hpp"
#include "automap/validation/map_validation.hpp"

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
    using automap::validation::Severity;
    using automap::validation::UniqueIdRule;
    using automap::validation::ValidationContext;
    using automap::validation::ValidationReport;

    bool passed = true;
    const UniqueIdRule rule;
    passed &= check(rule.id() == "M3_ID_UNIQUENESS", "规则 ID 应保持稳定。");

    MapData unique_map;
    unique_map.header.map_id = MapId{"unique_map"};
    unique_map.roads.push_back({.id = "road_1"});
    unique_map.lanes.push_back({.id = "lane_1"});
    unique_map.lane_boundaries.push_back({.id = "boundary_1"});
    unique_map.junctions.push_back({.id = "junction_1"});
    unique_map.lane_connections.push_back({.id = "connection_1"});
    unique_map.operational_areas.push_back({.id = "area_1"});
    unique_map.stations.push_back({.id = "station_1"});
    unique_map.restricted_areas.push_back({.id = "restricted_1"});
    unique_map.vehicle_profiles.push_back({.id = "vehicle_1"});

    ValidationReport unique_report{unique_map.header.map_id};
    rule.validate(ValidationContext{.map = unique_map}, unique_report);
    passed &= check(unique_report.issues().empty(), "九类对象 ID 全局唯一时不应产生问题。");

    MapData duplicate_map;
    duplicate_map.header.map_id = MapId{"duplicate_map"};
    duplicate_map.roads.push_back({.id = "shared_id"});
    duplicate_map.lanes.push_back({.id = "shared_id"});
    duplicate_map.lane_boundaries.push_back({.id = "shared_id"});
    duplicate_map.junctions.push_back({.id = "shared_id"});
    duplicate_map.lane_connections.push_back({.id = "shared_id"});
    duplicate_map.operational_areas.push_back({.id = "shared_id"});
    duplicate_map.stations.push_back({.id = "shared_id"});
    duplicate_map.restricted_areas.push_back({.id = "shared_id"});
    duplicate_map.vehicle_profiles.push_back({.id = "shared_id"});

    ValidationReport duplicate_report{duplicate_map.header.map_id};
    rule.validate(ValidationContext{.map = duplicate_map}, duplicate_report);

    passed &= check(
        duplicate_report.count(Severity::fatal) == 8U,
        "同一 ID 在九类对象中出现时，后八次出现都应产生 Fatal。");
    passed &= check(!duplicate_report.can_publish(), "存在重复 ID 时应阻止地图发布。");
    for (const auto& issue : duplicate_report.issues()) {
        passed &= check(issue.rule_id == rule.id(), "重复 ID 问题应记录规则 ID。");
        passed &= check(issue.object_id == "shared_id", "重复 ID 问题应记录冲突对象 ID。");
        passed &= check(!issue.suggestion.empty(), "重复 ID 问题应提供修复建议。");
    }

    MapData repeated_in_collection;
    repeated_in_collection.header.map_id = MapId{"same_collection"};
    repeated_in_collection.roads.push_back({.id = "road_duplicate"});
    repeated_in_collection.roads.push_back({.id = "road_duplicate"});
    ValidationReport collection_report{repeated_in_collection.header.map_id};
    rule.validate(ValidationContext{.map = repeated_in_collection}, collection_report);
    passed &= check(
        collection_report.count(Severity::fatal) == 1U,
        "同一对象集合内部的重复 ID 也应产生 Fatal。");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M3 ID 唯一性规则测试通过。\n";
    return 0;
}
