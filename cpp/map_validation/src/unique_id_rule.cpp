#include "automap/validation/unique_id_rule.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace automap::validation {
namespace {

using FirstOccurrence = std::unordered_map<std::string, std::string_view>;

void check_id(
    std::string_view object_id,
    std::string_view object_type,
    FirstOccurrence& first_occurrences,
    std::string_view rule_id,
    ValidationReport& report) {
    const auto [position, inserted] = first_occurrences.emplace(
        std::string(object_id), object_type);
    if (inserted) {
        return;
    }

    report.add_issue(ValidationIssue{
        .rule_id = std::string(rule_id),
        .severity = Severity::fatal,
        .object_id = std::string(object_id),
        .message = "对象 ID '" + std::string(object_id) + "' 在 " +
                   std::string(position->second) + " 和 " + std::string(object_type) +
                   " 中重复。",
        .suggestion = "为后出现的 " + std::string(object_type) +
                      " 分配新的稳定 ID，并同步更新引用。",
    });
}

template <typename Objects>
void check_collection(
    const Objects& objects,
    std::string_view object_type,
    FirstOccurrence& first_occurrences,
    std::string_view rule_id,
    ValidationReport& report) {
    for (const auto& object : objects) {
        check_id(object.id, object_type, first_occurrences, rule_id, report);
    }
}

}  // namespace

std::string_view UniqueIdRule::id() const noexcept {
    return "M3_ID_UNIQUENESS";
}

void UniqueIdRule::validate(
    const ValidationContext& context,
    ValidationReport& report) const {
    FirstOccurrence first_occurrences;
    const core::MapData& map = context.map;

    check_collection(map.roads, "Road", first_occurrences, id(), report);
    check_collection(map.lanes, "Lane", first_occurrences, id(), report);
    check_collection(map.lane_boundaries, "LaneBoundary", first_occurrences, id(), report);
    check_collection(map.junctions, "Junction", first_occurrences, id(), report);
    check_collection(map.lane_connections, "LaneConnection", first_occurrences, id(), report);
    check_collection(map.operational_areas, "OperationalArea", first_occurrences, id(), report);
    check_collection(map.stations, "Station", first_occurrences, id(), report);
    check_collection(map.restricted_areas, "RestrictedArea", first_occurrences, id(), report);
    check_collection(map.vehicle_profiles, "VehicleProfile", first_occurrences, id(), report);
}

}  // namespace automap::validation
