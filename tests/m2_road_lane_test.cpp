#include "automap/core/map_data.hpp"
#include "automap/core/road_lane.hpp"

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

    const Road road{
        .id = "road_main_01",
        .name = "Main Road 01",
        .reference_line = {{0.0, 0.0, 0.0}, {50.0, 0.0, 0.0}},
        .predecessor_ids = {"road_gate_entry"},
        .successor_ids = {"road_junction_01"},
        .lane_ids = {"lane_main_01_forward", "lane_main_01_reverse"},
    };

    const Lane forward_lane{
        .id = "lane_main_01_forward",
        .road_id = "road_main_01",
        .centerline = {{0.0, -1.75, 0.0}, {50.0, -1.75, 0.0}},
        .side = LaneSide::right,
        .order_from_reference = 1,
        .left_boundary_id = "boundary_main_center",
        .right_boundary_id = "boundary_main_right",
        .predecessor_ids = {"lane_gate_entry"},
        .successor_ids = {"lane_junction_straight"},
        .direction = LaneDirection::along_reference_line,
        .status = LaneStatus::open,
        .width_m = 3.5,
        .speed_limit_mps = 5.0,
    };

    const Lane reverse_lane{
        .id = "lane_main_01_reverse",
        .road_id = "road_main_01",
        .centerline = {{0.0, 1.75, 0.0}, {50.0, 1.75, 0.0}},
        .side = LaneSide::left,
        .order_from_reference = 1,
        .left_boundary_id = "boundary_main_left",
        .right_boundary_id = "boundary_main_center",
        .direction = LaneDirection::against_reference_line,
        .status = LaneStatus::open,
        .width_m = 3.5,
        .speed_limit_mps = 5.0,
    };

    passed &= check(polyline_length(road.reference_line) == 50.0,
                    "Road should retain its 50 m reference line.");
    passed &= check(road.lane_ids.size() == 2,
                    "Road should retain all owned lane references.");
    passed &= check(forward_lane.predecessor_ids == std::vector<ObjectId>{"lane_gate_entry"},
                    "Lane should retain predecessor topology.");
    passed &= check(forward_lane.successor_ids ==
                        std::vector<ObjectId>{"lane_junction_straight"},
                    "Lane should retain successor topology.");
    passed &= check(forward_lane.width_m == 3.5 && forward_lane.speed_limit_mps == 5.0,
                    "Lane should retain width and speed limit in SI units.");
    passed &= check(lane_side_name(forward_lane.side) == "right",
                    "Lane side should have a stable serialized name.");
    passed &= check(lane_direction_name(forward_lane.direction) == "along_reference_line",
                    "Lane direction should be relative to the road reference line.");
    passed &= check(lane_direction_name(reverse_lane.direction) == "against_reference_line",
                    "Opposing traffic should be explicit.");
    passed &= check(lane_status_name(LaneStatus::closed) == "closed",
                    "Lane status should have a stable serialized name.");

    MapData map;
    map.roads.push_back(road);
    map.lanes.push_back(forward_lane);
    map.lanes.push_back(reverse_lane);

    const MapData& const_map = map;
    passed &= check(const_map.find_road("road_main_01") != nullptr,
                    "MapData should find a road by stable ID.");
    const Lane* found_reverse_lane = const_map.find_lane("lane_main_01_reverse");
    passed &= check(found_reverse_lane != nullptr,
                    "MapData should find a lane by stable ID.");
    if (found_reverse_lane != nullptr) {
        passed &= check(*found_reverse_lane == reverse_lane,
                        "Lane should support value comparison for round-trip tests.");
    }

    if (!passed) {
        return 1;
    }

    std::cout << "AutoMapOps M2 road and lane test passed.\n";
    return 0;
}
