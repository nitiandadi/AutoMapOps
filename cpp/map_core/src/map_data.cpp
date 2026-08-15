#include "automap/core/map_data.hpp"

#include <algorithm>
#include <vector>

namespace automap::core {
namespace {

template <typename T>
T* find_by_id(std::vector<T>& objects, std::string_view id) noexcept {
    const auto found = std::find_if(objects.begin(), objects.end(), [id](const T& object) {
        return object.id == id;
    });
    return found == objects.end() ? nullptr : &*found;
}

template <typename T>
const T* find_by_id(const std::vector<T>& objects, std::string_view id) noexcept {
    const auto found = std::find_if(objects.cbegin(), objects.cend(), [id](const T& object) {
        return object.id == id;
    });
    return found == objects.cend() ? nullptr : &*found;
}

}  // namespace

Road* MapData::find_road(std::string_view id) noexcept { return find_by_id(roads, id); }
const Road* MapData::find_road(std::string_view id) const noexcept { return find_by_id(roads, id); }
Lane* MapData::find_lane(std::string_view id) noexcept { return find_by_id(lanes, id); }
const Lane* MapData::find_lane(std::string_view id) const noexcept { return find_by_id(lanes, id); }
LaneBoundary* MapData::find_lane_boundary(std::string_view id) noexcept {
    return find_by_id(lane_boundaries, id);
}
const LaneBoundary* MapData::find_lane_boundary(std::string_view id) const noexcept {
    return find_by_id(lane_boundaries, id);
}
Junction* MapData::find_junction(std::string_view id) noexcept { return find_by_id(junctions, id); }
const Junction* MapData::find_junction(std::string_view id) const noexcept {
    return find_by_id(junctions, id);
}
LaneConnection* MapData::find_lane_connection(std::string_view id) noexcept {
    return find_by_id(lane_connections, id);
}
const LaneConnection* MapData::find_lane_connection(std::string_view id) const noexcept {
    return find_by_id(lane_connections, id);
}
Station* MapData::find_station(std::string_view id) noexcept { return find_by_id(stations, id); }
const Station* MapData::find_station(std::string_view id) const noexcept {
    return find_by_id(stations, id);
}
OperationalArea* MapData::find_operational_area(std::string_view id) noexcept {
    return find_by_id(operational_areas, id);
}
const OperationalArea* MapData::find_operational_area(std::string_view id) const noexcept {
    return find_by_id(operational_areas, id);
}
RestrictedArea* MapData::find_restricted_area(std::string_view id) noexcept {
    return find_by_id(restricted_areas, id);
}
const RestrictedArea* MapData::find_restricted_area(std::string_view id) const noexcept {
    return find_by_id(restricted_areas, id);
}
VehicleProfile* MapData::find_vehicle_profile(std::string_view id) noexcept {
    return find_by_id(vehicle_profiles, id);
}
const VehicleProfile* MapData::find_vehicle_profile(std::string_view id) const noexcept {
    return find_by_id(vehicle_profiles, id);
}

}  // namespace automap::core
