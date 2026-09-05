#pragma once

#include "automap/core/map_id.hpp"

#include <string>
#include <string_view>

namespace automap::core {

namespace map_header_defaults {

inline constexpr std::string_view schema_version{"1.1"};
inline constexpr std::string_view geodetic_datum{"WGS84"};
inline constexpr std::string_view linear_unit{"m"};
inline constexpr std::string_view angle_unit{"rad"};

}  // namespace map_header_defaults

struct GeodeticPoint final {
    double longitude_deg{0.0};
    double latitude_deg{0.0};
    double altitude_m{0.0};

    bool operator==(const GeodeticPoint&) const = default;
};

enum class LocalCoordinateFrame {
    enu,
};

[[nodiscard]] std::string_view local_coordinate_frame_name(
    LocalCoordinateFrame frame) noexcept;

struct CoordinateReference final {
    std::string geodetic_datum{map_header_defaults::geodetic_datum};
    GeodeticPoint origin;
    LocalCoordinateFrame local_frame{LocalCoordinateFrame::enu};
    std::string linear_unit{map_header_defaults::linear_unit};
    std::string angle_unit{map_header_defaults::angle_unit};

    bool operator==(const CoordinateReference&) const = default;
};

struct MapHeader final {
    MapId map_id;
    std::string name;
    std::string schema_version{map_header_defaults::schema_version};
    CoordinateReference coordinate_reference;

    bool operator==(const MapHeader&) const = default;
};

}  // namespace automap::core
