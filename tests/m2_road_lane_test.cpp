#include "automap/core/map_data.hpp"
#include "automap/core/road_lane.hpp"

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
                    "Road 应保存长度为 50 m 的参考线。");
    passed &= check(road.lane_ids.size() == 2,
                    "Road 应保存其全部 Lane 引用。");
    passed &= check(forward_lane.predecessor_ids == std::vector<ObjectId>{"lane_gate_entry"},
                    "Lane 应保存前驱拓扑。");
    passed &= check(forward_lane.successor_ids ==
                        std::vector<ObjectId>{"lane_junction_straight"},
                    "Lane 应保存后继拓扑。");
    passed &= check(forward_lane.width_m == 3.5 && forward_lane.speed_limit_mps == 5.0,
                    "Lane 应以 SI 单位保存宽度和限速。");
    passed &= check(lane_side_name(forward_lane.side) == "right",
                    "Lane 侧别应具有稳定的序列化名称。");
    passed &= check(lane_direction_name(forward_lane.direction) == "along_reference_line",
                    "Lane 方向应相对于 Road 参考线定义。");
    passed &= check(lane_direction_name(reverse_lane.direction) == "against_reference_line",
                    "逆参考线行驶方向应被明确表达。");
    passed &= check(lane_status_name(LaneStatus::closed) == "closed",
                    "Lane 状态应具有稳定的序列化名称。");

    MapData map;
    map.roads.push_back(road);
    map.lanes.push_back(forward_lane);
    map.lanes.push_back(reverse_lane);

    const MapData& const_map = map;
    passed &= check(const_map.find_road("road_main_01") != nullptr,
                    "MapData 应能通过稳定 ID 查找 Road。");
    const Lane* found_reverse_lane = const_map.find_lane("lane_main_01_reverse");
    passed &= check(found_reverse_lane != nullptr,
                    "MapData 应能通过稳定 ID 查找 Lane。");
    if (found_reverse_lane != nullptr) {
        passed &= check(*found_reverse_lane == reverse_lane,
                        "Lane 应支持用于往返测试的值比较。");
    }

    if (!passed) {
        return 1;
    }

    std::cout << "AutoMapOps M2 Road 与 Lane 测试通过。\n";
    return 0;
}
