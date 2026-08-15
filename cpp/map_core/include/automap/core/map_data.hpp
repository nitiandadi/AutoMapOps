#pragma once

#include "automap/core/map_header.hpp"
#include "automap/core/map_objects.hpp"

#include <string_view>
#include <vector>

namespace automap::core {

struct MapData final {
    MapHeader header;
    std::vector<Road> roads;
    std::vector<Lane> lanes;
    std::vector<LaneBoundary> lane_boundaries;
    std::vector<Junction> junctions;
    std::vector<LaneConnection> lane_connections;
    std::vector<OperationalArea> operational_areas;
    std::vector<Station> stations;
    std::vector<RestrictedArea> restricted_areas;
    std::vector<VehicleProfile> vehicle_profiles;

    [[nodiscard]] Road* find_road(std::string_view id) noexcept;
    [[nodiscard]] const Road* find_road(std::string_view id) const noexcept;
    [[nodiscard]] Lane* find_lane(std::string_view id) noexcept;
    [[nodiscard]] const Lane* find_lane(std::string_view id) const noexcept;
    [[nodiscard]] LaneBoundary* find_lane_boundary(std::string_view id) noexcept;
    [[nodiscard]] const LaneBoundary* find_lane_boundary(std::string_view id) const noexcept;
    [[nodiscard]] Junction* find_junction(std::string_view id) noexcept;
    [[nodiscard]] const Junction* find_junction(std::string_view id) const noexcept;
    [[nodiscard]] LaneConnection* find_lane_connection(std::string_view id) noexcept;
    [[nodiscard]] const LaneConnection* find_lane_connection(std::string_view id) const noexcept;
    [[nodiscard]] Station* find_station(std::string_view id) noexcept;
    [[nodiscard]] const Station* find_station(std::string_view id) const noexcept;
    [[nodiscard]] OperationalArea* find_operational_area(std::string_view id) noexcept;
    [[nodiscard]] const OperationalArea* find_operational_area(
        std::string_view id) const noexcept;
    [[nodiscard]] RestrictedArea* find_restricted_area(std::string_view id) noexcept;
    [[nodiscard]] const RestrictedArea* find_restricted_area(
        std::string_view id) const noexcept;
    [[nodiscard]] VehicleProfile* find_vehicle_profile(std::string_view id) noexcept;
    [[nodiscard]] const VehicleProfile* find_vehicle_profile(
        std::string_view id) const noexcept;

    bool operator==(const MapData&) const = default;
};

}  // namespace automap::core
