#include "automap/core/map_core.hpp"

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
    using namespace automap::core;
    bool passed = true;

    MapData map;
    map.header.map_id = MapId{"aggregate_demo"};
    map.roads.push_back(Road{.id = "road_1"});
    map.lanes.push_back(Lane{.id = "lane_1"});
    map.lane_boundaries.push_back(LaneBoundary{.id = "boundary_1"});
    map.junctions.push_back(Junction{.id = "junction_1"});
    map.lane_connections.push_back(LaneConnection{.id = "connection_1"});
    map.operational_areas.push_back(OperationalArea{.id = "area_1"});
    map.stations.push_back(Station{.id = "station_1"});
    map.restricted_areas.push_back(RestrictedArea{.id = "restricted_1"});
    map.vehicle_profiles.push_back(VehicleProfile{.id = "vehicle_1"});

    passed &= check(map.find_road("road_1") != nullptr, "Road 查询应正常工作。");
    passed &= check(map.find_lane("lane_1") != nullptr, "Lane 查询应正常工作。");
    passed &= check(map.find_lane_boundary("boundary_1") != nullptr,
                    "LaneBoundary 查询应正常工作。");
    passed &= check(map.find_junction("junction_1") != nullptr,
                    "Junction 查询应正常工作。");
    passed &= check(map.find_lane_connection("connection_1") != nullptr,
                    "LaneConnection 查询应正常工作。");
    passed &= check(map.find_operational_area("area_1") != nullptr,
                    "OperationalArea 查询应正常工作。");
    passed &= check(map.find_station("station_1") != nullptr,
                    "Station 查询应正常工作。");
    passed &= check(map.find_restricted_area("restricted_1") != nullptr,
                    "RestrictedArea 查询应正常工作。");
    passed &= check(map.find_vehicle_profile("vehicle_1") != nullptr,
                    "VehicleProfile 查询应正常工作。");

    const MapData copied_map = map;
    passed &= check(copied_map == map,
                    "MapData 应支持整张地图的值比较。");
    passed &= check(copied_map.find_operational_area("missing") == nullptr,
                    "聚合根中查找不存在的对象应返回 nullptr。");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M2 MapData 聚合根测试通过。\n";
    return 0;
}
