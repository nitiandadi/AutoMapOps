#include "automap/core/map_data.hpp"
#include "automap/core/scene_objects.hpp"

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

    const OperationalArea loading_area{
        .id = "area_loading_a",
        .name = "Loading Area A",
        .type = OperationalAreaType::loading_area,
        .outline = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, {10.0, 8.0, 0.0},
                    {0.0, 8.0, 0.0}, {0.0, 0.0, 0.0}},
    };
    const Station loading_bay{
        .id = "station_loading_a1",
        .name = "Loading Bay A1",
        .type = StationType::loading_bay,
        .position = {8.0, 4.0, 0.0},
        .access_lane_id = "lane_loading_a",
    };
    const RestrictedArea narrow_passage{
        .id = "restricted_narrow_01",
        .name = "Narrow Passage",
        .outline = {{20.0, 0.0, 0.0}, {30.0, 0.0, 0.0}, {30.0, 3.0, 0.0},
                    {20.0, 3.0, 0.0}, {20.0, 0.0, 0.0}},
        .allowed_vehicle_profile_ids = {"vehicle_delivery_van"},
    };

    passed &= check(operational_area_type_name(loading_area.type) == "loading_area",
                    "OperationalArea 类型应具有稳定的序列化名称。");
    passed &= check(station_type_name(loading_bay.type) == "loading_bay",
                    "Station 类型应具有稳定的序列化名称。");
    passed &= check(loading_area.outline.front() == loading_area.outline.back(),
                    "示例区域轮廓应为闭合折线。");
    passed &= check(loading_bay.access_lane_id == "lane_loading_a",
                    "Station 应标识其接入 Lane。");

    MapData map;
    map.operational_areas.push_back(loading_area);
    map.stations.push_back(loading_bay);
    map.restricted_areas.push_back(narrow_passage);
    const MapData& const_map = map;
    passed &= check(const_map.find_operational_area(loading_area.id) != nullptr,
                    "MapData 应能查找 OperationalArea。");
    passed &= check(const_map.find_station(loading_bay.id) != nullptr,
                    "MapData 应能查找 Station。");
    const RestrictedArea* found = const_map.find_restricted_area(narrow_passage.id);
    passed &= check(found != nullptr && *found == narrow_passage,
                    "MapData 应能查找并比较 RestrictedArea。");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M2 场景对象测试通过。\n";
    return 0;
}
