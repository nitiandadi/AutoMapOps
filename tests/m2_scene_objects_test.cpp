#include "automap/core/map_data.hpp"
#include "automap/core/scene_objects.hpp"

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

    const OperationalArea loading_area{
        .id = "area_loading_a",
        .name = "Loading Area A",
        .type = OperationalAreaType::loading_area,
        .outline = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, {10.0, 8.0, 0.0},
                    {0.0, 8.0, 0.0}, {0.0, 0.0, 0.0}},
    };
    const Station loading_bay{
        .id = "station_loading_a1",
        .name = "Loading Bay A1",
        .type = StationType::loading_bay,
        .position = {8.0, 4.0, 0.0},
        .access_lane_id = "lane_loading_a",
    };
    const RestrictedArea narrow_passage{
        .id = "restricted_narrow_01",
        .name = "Narrow Passage",
        .outline = {{20.0, 0.0, 0.0}, {30.0, 0.0, 0.0}, {30.0, 3.0, 0.0},
                    {20.0, 3.0, 0.0}, {20.0, 0.0, 0.0}},
        .allowed_vehicle_profile_ids = {"vehicle_delivery_van"},
    };

    passed &= check(operational_area_type_name(loading_area.type) == "loading_area",
                    "Operational area type should have a stable name.");
    passed &= check(station_type_name(loading_bay.type) == "loading_bay",
                    "Station type should have a stable name.");
    passed &= check(loading_area.outline.front() == loading_area.outline.back(),
                    "The example area outline should be closed.");
    passed &= check(loading_bay.access_lane_id == "lane_loading_a",
                    "A station should identify its access lane.");

    MapData map;
    map.operational_areas.push_back(loading_area);
    map.stations.push_back(loading_bay);
    map.restricted_areas.push_back(narrow_passage);
    const MapData& const_map = map;
    passed &= check(const_map.find_operational_area(loading_area.id) != nullptr,
                    "MapData should find an operational area.");
    passed &= check(const_map.find_station(loading_bay.id) != nullptr,
                    "MapData should find a station.");
    const RestrictedArea* found = const_map.find_restricted_area(narrow_passage.id);
    passed &= check(found != nullptr && *found == narrow_passage,
                    "MapData should find and compare a restricted area.");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M2 scene objects test passed.\n";
    return 0;
}
