#include "automap/core/junction.hpp"
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
                    "Turn direction should have a stable serialized name.");
    passed &= check(turn_direction_name(TurnDirection::u_turn) == "u_turn",
                    "U-turn should have a stable serialized name.");
    passed &= check(left_turn.junction_id == junction.id,
                    "A lane connection should identify its owning junction.");
    passed &= check(left_turn.incoming_lane_id == "lane_gate_in" &&
                        left_turn.connecting_lane_id == "lane_gate_left_connector" &&
                        left_turn.outgoing_lane_id == "lane_west_out",
                    "A permitted movement should preserve all three lane roles.");

    MapData map;
    map.junctions.push_back(junction);
    map.lane_connections.push_back(left_turn);
    const MapData& const_map = map;
    passed &= check(const_map.find_junction(junction.id) != nullptr,
                    "MapData should find a junction.");
    const LaneConnection* found = const_map.find_lane_connection(left_turn.id);
    passed &= check(found != nullptr && *found == left_turn,
                    "MapData should find and compare a lane connection.");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M2 junction test passed.\n";
    return 0;
}
