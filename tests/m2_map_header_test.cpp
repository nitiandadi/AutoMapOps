#include "automap/core/map_header.hpp"

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

    const CoordinateReference default_reference;
    passed &= check(default_reference.geodetic_datum == "WGS84",
                    "The default geodetic datum should be WGS84.");
    passed &= check(default_reference.local_frame == LocalCoordinateFrame::enu,
                    "The default local coordinate frame should be ENU.");
    passed &= check(default_reference.linear_unit == "m",
                    "The default linear unit should be metre.");
    passed &= check(default_reference.angle_unit == "rad",
                    "The default angle unit should be radian.");
    passed &= check(local_coordinate_frame_name(default_reference.local_frame) == "enu",
                    "The ENU frame should have a stable serialized name.");

    const MapHeader header{
        .map_id = MapId{"logistics_park_demo"},
        .name = "Logistics Park Demo",
        .schema_version = "1.0",
        .coordinate_reference = CoordinateReference{
            .geodetic_datum = "WGS84",
            .origin = GeodeticPoint{
                .longitude_deg = 104.0668,
                .latitude_deg = 30.5728,
                .altitude_m = 500.0,
            },
            .local_frame = LocalCoordinateFrame::enu,
            .linear_unit = "m",
            .angle_unit = "rad",
        },
    };

    passed &= check(header.map_id.value() == "logistics_park_demo",
                    "MapHeader should retain its stable map ID.");
    passed &= check(header.name == "Logistics Park Demo",
                    "MapHeader should retain its display name.");
    passed &= check(header.schema_version == map_header_defaults::schema_version,
                    "MapHeader should expose Canonical schema version 1.0.");
    passed &= check(header.coordinate_reference.origin ==
                        GeodeticPoint{104.0668, 30.5728, 500.0},
                    "MapHeader should retain its WGS84 ENU origin.");

    const MapHeader copied_header = header;
    passed &= check(copied_header == header,
                    "MapHeader should support value comparison for round-trip tests.");

    if (!passed) {
        return 1;
    }

    std::cout << "AutoMapOps M2 MapHeader test passed.\n";
    return 0;
}
