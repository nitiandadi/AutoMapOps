#include "automap/validation/reference_integrity_rule.hpp"

#include <string>
#include <string_view>
#include <unordered_set>

namespace automap::validation {
namespace {

using IdSet = std::unordered_set<std::string>;

template <typename Objects>
[[nodiscard]] IdSet collect_ids(const Objects& objects) {
    IdSet ids;
    ids.reserve(objects.size());
    for (const auto& object : objects) {
        ids.insert(object.id);
    }
    return ids;
}

void check_reference(
    std::string_view owner_type,
    std::string_view owner_id,
    std::string_view field_name,
    std::string_view target_type,
    std::string_view target_id,
    const IdSet& target_ids,
    std::string_view rule_id,
    ValidationReport& report) {
    if (target_ids.contains(std::string(target_id))) {
        return;
    }

    report.add_issue(ValidationIssue{
        .rule_id = std::string(rule_id),
        .severity = Severity::error,
        .object_id = std::string(owner_id),
        .message = std::string(owner_type) + " '" + std::string(owner_id) +
                   "' 的 " + std::string(field_name) + " 引用了不存在的 " +
                   std::string(target_type) + " '" + std::string(target_id) + "'。",
        .suggestion = "将 " + std::string(field_name) + " 改为已存在的 " +
                      std::string(target_type) + " ID，或者补充被引用对象。",
    });
}

template <typename References>
void check_references(
    std::string_view owner_type,
    std::string_view owner_id,
    std::string_view field_name,
    std::string_view target_type,
    const References& references,
    const IdSet& target_ids,
    std::string_view rule_id,
    ValidationReport& report) {
    for (const auto& target_id : references) {
        check_reference(
            owner_type, owner_id, field_name, target_type, target_id,
            target_ids, rule_id, report);
    }
}

}  // namespace

std::string_view ReferenceIntegrityRule::id() const noexcept {
    return "M3_REFERENCE_INTEGRITY";
}

void ReferenceIntegrityRule::validate(
    const ValidationContext& context,
    ValidationReport& report) const {
    const core::MapData& map = context.map;
    const IdSet road_ids = collect_ids(map.roads);
    const IdSet lane_ids = collect_ids(map.lanes);
    const IdSet boundary_ids = collect_ids(map.lane_boundaries);
    const IdSet junction_ids = collect_ids(map.junctions);
    const IdSet connection_ids = collect_ids(map.lane_connections);
    const IdSet vehicle_profile_ids = collect_ids(map.vehicle_profiles);

    for (const auto& road : map.roads) {
        check_references(
            "Road", road.id, "predecessorIds", "Road", road.predecessor_ids,
            road_ids, id(), report);
        check_references(
            "Road", road.id, "successorIds", "Road", road.successor_ids,
            road_ids, id(), report);
        check_references(
            "Road", road.id, "laneIds", "Lane", road.lane_ids,
            lane_ids, id(), report);
    }

    for (const auto& lane : map.lanes) {
        check_reference(
            "Lane", lane.id, "roadId", "Road", lane.road_id,
            road_ids, id(), report);
        check_reference(
            "Lane", lane.id, "leftBoundaryId", "LaneBoundary", lane.left_boundary_id,
            boundary_ids, id(), report);
        check_reference(
            "Lane", lane.id, "rightBoundaryId", "LaneBoundary", lane.right_boundary_id,
            boundary_ids, id(), report);
        check_references(
            "Lane", lane.id, "predecessorIds", "Lane", lane.predecessor_ids,
            lane_ids, id(), report);
        check_references(
            "Lane", lane.id, "successorIds", "Lane", lane.successor_ids,
            lane_ids, id(), report);
    }

    for (const auto& junction : map.junctions) {
        check_references(
            "Junction", junction.id, "connectionIds", "LaneConnection",
            junction.connection_ids, connection_ids, id(), report);
    }

    for (const auto& connection : map.lane_connections) {
        check_reference(
            "LaneConnection", connection.id, "junctionId", "Junction",
            connection.junction_id, junction_ids, id(), report);
        check_reference(
            "LaneConnection", connection.id, "incomingLaneId", "Lane",
            connection.incoming_lane_id, lane_ids, id(), report);
        check_reference(
            "LaneConnection", connection.id, "connectingLaneId", "Lane",
            connection.connecting_lane_id, lane_ids, id(), report);
        check_reference(
            "LaneConnection", connection.id, "outgoingLaneId", "Lane",
            connection.outgoing_lane_id, lane_ids, id(), report);
    }

    for (const auto& station : map.stations) {
        check_reference(
            "Station", station.id, "accessLaneId", "Lane", station.access_lane_id,
            lane_ids, id(), report);
    }

    for (const auto& area : map.restricted_areas) {
        check_references(
            "RestrictedArea", area.id, "allowedVehicleProfileIds", "VehicleProfile",
            area.allowed_vehicle_profile_ids, vehicle_profile_ids, id(), report);
    }
}

}  // namespace automap::validation
