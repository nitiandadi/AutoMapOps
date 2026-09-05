#include "automap/core/map_header.hpp"

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

    const CoordinateReference default_reference;
    passed &= check(default_reference.geodetic_datum == "WGS84",
                    "默认大地基准应为 WGS84。");
    passed &= check(default_reference.local_frame == LocalCoordinateFrame::enu,
                    "默认局部坐标系应为 ENU。");
    passed &= check(default_reference.linear_unit == "m",
                    "默认长度单位应为米。");
    passed &= check(default_reference.angle_unit == "rad",
                    "默认角度单位应为弧度。");
    passed &= check(local_coordinate_frame_name(default_reference.local_frame) == "enu",
                    "ENU 坐标系应具有稳定的序列化名称。");

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
                    "MapHeader 应保存稳定的地图 ID。");
    passed &= check(header.name == "Logistics Park Demo",
                    "MapHeader 应保存地图显示名称。");
    passed &= check(header.schema_version == "1.0",
                    "显式指定时 MapHeader 应保留 Canonical Schema 版本 1.0。");
    passed &= check(MapHeader{}.schema_version == map_header_defaults::schema_version &&
                        map_header_defaults::schema_version == "1.1",
                    "新建 MapHeader 应默认使用最新 Canonical Schema 版本 1.1。");
    passed &= check(header.coordinate_reference.origin ==
                        GeodeticPoint{104.0668, 30.5728, 500.0},
                    "MapHeader 应保存 WGS84 ENU 原点。");

    const MapHeader copied_header = header;
    passed &= check(copied_header == header,
                    "MapHeader 应支持用于往返测试的值比较。");

    if (!passed) {
        return 1;
    }

    std::cout << "AutoMapOps M2 MapHeader 测试通过。\n";
    return 0;
}
