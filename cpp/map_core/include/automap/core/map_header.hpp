#pragma once

#include "automap/core/map_id.hpp"

#include <string>

namespace automap::core {

struct GeodeticPoint final {
    double longitude_deg{0.0};
    double latitude_deg{0.0};
    double altitude_m{0.0};
};

enum class LocalCoordinateFrame {
    enu,
};

struct CoordinateReference final {
    std::string geodetic_datum{"WGS84"};
    GeodeticPoint origin;
    LocalCoordinateFrame local_frame{LocalCoordinateFrame::enu};
    std::string linear_unit{"m"};
    std::string angle_unit{"rad"};
};

struct MapHeader final {
    MapId map_id;
    std::string name;
    std::string schema_version{"1.0"};
    CoordinateReference coordinate_reference;
};

}  // namespace automap::core

