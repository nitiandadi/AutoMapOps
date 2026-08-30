#include "automap/core/map_data.hpp"
#include "automap/core/vehicle_profile.hpp"

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

    const VehicleProfile truck{
        .id = "vehicle_truck_12m",
        .name = "12 m Logistics Truck",
        .type = VehicleType::truck,
        .width_m = 2.55,
        .height_m = 4.0,
        .length_m = 12.0,
        .minimum_turning_radius_m = 10.5,
    };

    passed &= check(vehicle_type_name(truck.type) == "truck",
                    "车辆类型应具有稳定的序列化名称。");
    passed &= check(truck.width_m == 2.55 && truck.height_m == 4.0,
                    "车辆宽度和可选高度应使用米作为单位。");
    passed &= check(truck.length_m == 12.0 && truck.minimum_turning_radius_m == 10.5,
                    "车辆模型应保存基础通行约束。");

    MapData map;
    map.vehicle_profiles.push_back(truck);
    VehicleProfile* found = map.find_vehicle_profile(truck.id);
    passed &= check(found != nullptr && *found == truck,
                    "MapData 应能查找并比较 VehicleProfile。");
    passed &= check(map.find_vehicle_profile("vehicle_missing") == nullptr,
                    "查找不存在的 VehicleProfile 应返回 nullptr。");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M2 VehicleProfile 测试通过。\n";
    return 0;
}
