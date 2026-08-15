#include "automap/core/lane_boundary.hpp"
#include "automap/core/map_data.hpp"

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

    const LaneBoundary shared_center{
        .id = "boundary_main_center",
        .geometry = {{0.0, 0.0, 0.0}, {20.0, 0.0, 0.0}, {50.0, 0.0, 0.0}},
        .type = LaneBoundaryType::dashed_line,
        .crossing_allowed = true,
    };
    const LaneBoundary outer_edge{
        .id = "boundary_main_right",
        .geometry = {{0.0, -3.5, 0.0}, {50.0, -3.5, 0.0}},
        .type = LaneBoundaryType::solid_line,
        .crossing_allowed = false,
    };
    const LaneBoundary curb{
        .id = "boundary_loading_curb",
        .geometry = {{0.0, -7.0, 0.0}, {50.0, -7.0, 0.0}},
        .type = LaneBoundaryType::curb,
        .crossing_allowed = false,
    };

    passed &= check(polyline_length(shared_center.geometry) == 50.0,
                    "Boundary should retain its geometry in metres.");
    const auto bounds = bounding_box(shared_center.geometry);
    passed &= check(bounds.has_value() &&
                        bounds->contains(Point3d{20.0, 0.0, 0.0}),
                    "Boundary geometry should work with the common geometry algorithms.");
    passed &= check(lane_boundary_type_name(shared_center.type) == "dashed_line",
                    "Boundary type should have a stable serialized name.");
    passed &= check(lane_boundary_type_name(LaneBoundaryType::double_solid_line) ==
                        "double_solid_line",
                    "Double solid line should have a stable serialized name.");
    passed &= check(lane_boundary_type_name(LaneBoundaryType::virtual_boundary) ==
                        "virtual_boundary",
                    "Logical boundaries should be represented explicitly.");
    passed &= check(shared_center.crossing_allowed && !outer_edge.crossing_allowed &&
                        !curb.crossing_allowed,
                    "Crossing permission should be independent and explicit.");

    MapData map;
    map.lane_boundaries = {shared_center, outer_edge, curb};

    LaneBoundary* mutable_boundary = map.find_lane_boundary("boundary_main_center");
    passed &= check(mutable_boundary != nullptr,
                    "MapData should find a mutable boundary by stable ID.");
    if (mutable_boundary != nullptr) {
        passed &= check(*mutable_boundary == shared_center,
                        "Boundary should support value comparison for round-trip tests.");
        mutable_boundary->crossing_allowed = false;
    }

    const MapData& const_map = map;
    const LaneBoundary* found_boundary =
        const_map.find_lane_boundary("boundary_main_center");
    passed &= check(found_boundary != nullptr && !found_boundary->crossing_allowed,
                    "Const lookup should observe a boundary update.");
    passed &= check(const_map.find_lane_boundary("boundary_missing") == nullptr,
                    "Missing boundary lookup should return nullptr.");

    if (!passed) {
        return 1;
    }

    std::cout << "AutoMapOps M2 lane boundary test passed.\n";
    return 0;
}
