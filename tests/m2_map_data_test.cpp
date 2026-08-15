#include "automap/core/map_core.hpp"

#include <iostream>
#include <string_view>

namespace {
bool check(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
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

    passed &= check(map.find_road("road_1") != nullptr, "Road lookup should work.");
    passed &= check(map.find_lane("lane_1") != nullptr, "Lane lookup should work.");
    passed &= check(map.find_lane_boundary("boundary_1") != nullptr,
                    "Lane boundary lookup should work.");
    passed &= check(map.find_junction("junction_1") != nullptr,
                    "Junction lookup should work.");
    passed &= check(map.find_lane_connection("connection_1") != nullptr,
                    "Lane connection lookup should work.");
    passed &= check(map.find_operational_area("area_1") != nullptr,
                    "Operational area lookup should work.");
    passed &= check(map.find_station("station_1") != nullptr,
                    "Station lookup should work.");
    passed &= check(map.find_restricted_area("restricted_1") != nullptr,
                    "Restricted area lookup should work.");
    passed &= check(map.find_vehicle_profile("vehicle_1") != nullptr,
                    "Vehicle profile lookup should work.");

    const MapData copied_map = map;
    passed &= check(copied_map == map,
                    "MapData should support whole-map value comparison.");
    passed &= check(copied_map.find_operational_area("missing") == nullptr,
                    "A missing aggregate lookup should return nullptr.");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M2 MapData aggregate test passed.\n";
    return 0;
}
