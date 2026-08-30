#include "automap/core/junction.hpp"
#include "automap/core/map_data.hpp"

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

    const LaneConnection left_turn{
        .id = "connection_gate_left",
        .junction_id = "junction_gate",
        .incoming_lane_id = "lane_gate_in",
        .connecting_lane_id = "lane_gate_left_connector",
        .outgoing_lane_id = "lane_west_out",
        .turn_direction = TurnDirection::left,
    };
    const Junction junction{
        .id = "junction_gate",
        .name = "Gate Junction",
        .connection_ids = {left_turn.id},
    };

    passed &= check(turn_direction_name(left_turn.turn_direction) == "left",
                    "转向类型应具有稳定的序列化名称。");
    passed &= check(turn_direction_name(TurnDirection::u_turn) == "u_turn",
                    "掉头类型应具有稳定的序列化名称。");
    passed &= check(left_turn.junction_id == junction.id,
                    "LaneConnection 应标识所属 Junction。");
    passed &= check(left_turn.incoming_lane_id == "lane_gate_in" &&
                        left_turn.connecting_lane_id == "lane_gate_left_connector" &&
                        left_turn.outgoing_lane_id == "lane_west_out",
                    "允许的通行动作应保存入口、连接和出口三种 Lane 角色。");

    MapData map;
    map.junctions.push_back(junction);
    map.lane_connections.push_back(left_turn);
    const MapData& const_map = map;
    passed &= check(const_map.find_junction(junction.id) != nullptr,
                    "MapData 应能查找 Junction。");
    const LaneConnection* found = const_map.find_lane_connection(left_turn.id);
    passed &= check(found != nullptr && *found == left_turn,
                    "MapData 应能查找并比较 LaneConnection。");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M2 Junction 测试通过。\n";
    return 0;
}
