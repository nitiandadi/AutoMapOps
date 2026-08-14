#include "automap/core/map_core.hpp"

#include <cmath>
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
    const Polyline3d polyline{{0.0, 0.0, 0.0}, {3.0, 4.0, 0.0}, {3.0, 4.0, 12.0}};

    passed &= check(almost_equal(polyline_length(polyline), 17.0), "Polyline length should be 17 m.");
    passed &= check(points_coincident({1.0, 2.0, 3.0}, {1.0005, 2.0, 3.0}),
                    "Points within 1 mm should coincide.");
    passed &= check(!bounding_box({}).has_value(), "An empty polyline should have no bounding box.");

    const auto bounds = bounding_box(polyline);
    passed &= check(bounds.has_value(), "A non-empty polyline should have a bounding box.");
    if (bounds) {
        passed &= check(bounds->min == Point3d{0.0, 0.0, 0.0}, "Bounding-box minimum is incorrect.");
        passed &= check(bounds->max == Point3d{3.0, 4.0, 12.0}, "Bounding-box maximum is incorrect.");
        passed &= check(bounds->contains({1.0, 2.0, 6.0}), "Bounding box should contain an inner point.");
    }

    MapData map{
        .header = MapHeader{
            .map_id = MapId{"logistics_park_demo"},
            .name = "Logistics Park Demo",
            .schema_version = "1.0",
            .coordinate_reference = CoordinateReference{
                .origin = GeodeticPoint{104.0668, 30.5728, 500.0},
            },
        },
    };
    map.roads.push_back(Road{.id = "road_main", .name = "Main Road", .lane_ids = {"lane_main"}});
    map.lanes.push_back(Lane{
        .id = "lane_main",
        .road_id = "road_main",
        .centerline = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}},
        .width_m = 3.5,
        .speed_limit_mps = 5.0,
    });

    passed &= check(map.find_road("road_main") != nullptr, "MapData should find an existing road.");
    passed &= check(map.find_lane("lane_main") != nullptr, "MapData should find an existing lane.");
    passed &= check(map.find_lane("missing_lane") == nullptr, "MapData should return null for a missing lane.");

    if (!passed) {
        return 1;
    }

    std::cout << "AutoMapOps M2 model skeleton test passed.\n";
    return 0;
}

